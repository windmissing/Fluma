"""T596 bounded archive/Meta2 reader. Static evidence only, never a stepper.
This executable is delivered without running it in the developer task.
"""
import argparse
import hashlib
import json
import struct
import tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def require(ok, reason):
    if not ok:
        raise ValueError(reason)

def pair(words, at):
    return words[at] | words[at + 1] << 32

def words_at(raw, offset, count):
    raw.seek(offset)
    data = raw.read(count * 4)
    require(len(data) == count * 4, 'TRUNCATED_PAYLOAD')
    return struct.unpack('<' + 'I' * count, data)

def digest(raw, offset, length, zero=False):
    raw.seek(offset)
    sha = hashlib.sha256()
    while length:
        chunk = raw.read(min(length, 65536))
        require(bool(chunk), 'TRUNCATED_SEGMENT')
        if zero:
            require(not any(chunk), 'NOT_EXECUTED_NONZERO')
        sha.update(chunk)
        length -= len(chunk)
    return sha.digest()

def validate_meta(raw, directory, descriptors, root):
    root_record = words_at(raw, (6048 + root * 128) * 4, 128)
    control_descriptor = descriptors[6 + root]
    if control_descriptor[21] == 1:
        return
    control = words_at(raw, directory[6 + root]['fixed']['byteOffset'], 2048)
    for role in range(1, 6):
        row = control[128 * (role - 1 if role <= 3 else role):][:128]
        meta = control[768 + 32 * (role - 1):][:32]
        require(row[0:2] == (876825680, 5) and row[7] == role and row[8] == 0 and row[31] == 3235774468, 'CONTROL128_IDENTITY')
        p = descriptors[21 + 10 * (root - 1) + 2 * (role - 1)]
        a = descriptors[22 + 10 * (root - 1) + 2 * (role - 1)]
        if row[44] & 8:
            require(not any(meta) and p[21] == a[21] == 1, 'SKIPPED_SCOPE_NOT_ZERO')
            continue
        require(meta[:4] == (894848323, 2, root, role) and meta[14] == 1 and meta[17] == 2 and meta[18] == 0 and meta[31] == 3235775745 and not any(meta[19:31]), 'META2_DICTIONARY')
        require(pair(meta, 4) == pair(root_record, 8) == pair(row, 4) and pair(meta, 6) == pair(row, 10) == pair(p, 4) == pair(a, 4), 'META2_NONCE_BRANCH')
        require(meta[8] <= meta[10] <= meta[9] and meta[11] <= meta[13] <= meta[12], 'META2_CURSOR_ORDER')
        ps, az = meta[10] - meta[8], meta[13] - meta[11]
        require(p[14:17] == (ps, meta[9] - meta[8], ps) and a[14:17] == (az, meta[12] - meta[11], az), 'PAIRED_COUNTS')
        ranges = (meta[8] + 1 if ps else 0, meta[10] if ps else 0, meta[11] + 1 if az else 0, meta[13] if az else 0)
        require(p[17:21] == a[17:21] == ranges, 'PAIRED_ORDINAL_RANGES')
        if meta[15]:
            require(meta[16] == 0 and meta[9] == meta[10] and meta[12] == meta[13] and row[47] == 0 and row[27] == ps and p[21] == a[21] == 2, 'FALSE_COMMON_SEAL')
        else:
            require(p[21] == a[21] == 4, 'FORENSIC_SCOPE_FLAG')
        for index, stride, magic, ordinal in ((21 + 10 * (root - 1) + 2 * (role - 1), 64, 1163088455, 7), (22 + 10 * (root - 1) + 2 * (role - 1), 32, 894456396, 6)):
            desc = descriptors[index]
            for n in range(desc[14]):
                event = words_at(raw, pair(desc, 9) + n * stride * 4, stride)
                require(event[0] == magic and event[1] == 5, 'INNER_EVENT_VERSION')
                if stride == 32:
                    require(event[3] == role and pair(event, 4) == pair(meta, 4) and event[31] == 3235774469 and event[ordinal] == meta[11] + n + 1, 'AUX_IDENTITY_OR_SEAL')
                else:
                    require(pair(event, 2) == pair(meta, 4) and event[ordinal] == meta[8] + n + 1, 'PHYSICAL_IDENTITY')
    require(not any(control[928:]), 'RAW_CAPTURE_LEAKED_IN_PUBLIC_CONTROL')

def read_archive(path):
    table = json.loads((ROOT / 'docs/architecture-T552-v5-segment-wire-v2.json').read_text(encoding='utf8'))
    with tempfile.TemporaryFile() as raw, path.open('rb') as lines:
        pages_sha = hashlib.sha256()
        payload_sha = hashlib.sha256()
        summary = None
        manifest = None
        offset = 0
        count = 0
        file_bytes = 0
        directory = None
        cursor_segment = 0
        for line in lines:
            require(len(line) <= 16384 and line.endswith(b'\n'), 'JSONL_RECORD_BOUND')
            page = json.loads(line)
            if page.get('schema') == 'T535_V5_ARCHIVE_MANIFEST_V2':
                require(manifest is None, 'DUPLICATE_MANIFEST')
                manifest = page
                continue
            require(manifest is None, 'PAGE_AFTER_MANIFEST')
            require(page.get('revision') == 'T527_SEGMENT_PAGE_V2' and page.get('payloadRevision') == 'GRS_M3B_LINEAGE_COLD_V5_2' and page.get('state') == 'TRACE_READY' and page.get('page') == count, 'PAGE_VERSION_OR_ORDER')
            if summary is None:
                summary = page
                require(page['mode'] in (1, 2), 'MODE')
                profile = next((p for p in table['profiles'] if (p['K'], p['Kw']) == (page['K'], page['Kw'])), None)
                require(profile is not None, 'PROFILE')
                directory = profile['fullDescriptors' if page['mode'] == 2 else 'rootDescriptors']
                require(page['segmentCount'] == len(directory) and int(page['capacityBytes']) == sum(s['fixed']['byteLength'] for s in directory) and page.get('words') == [], 'SUMMARY_DIRECTORY')
            else:
                for key in ('requestId', 'traceId', 'scope', 'mode', 'K', 'Kw', 'pages'):
                    require(page.get(key) == summary.get(key), 'PAGE_SCOPE_DRIFT:' + key)
                require(type(page['segmentIndex']) is int and page['segmentIndex'] == cursor_segment, 'SEGMENT_ORDER')
                segment = directory[cursor_segment]['fixed']
                values = page['words']
                require(isinstance(values, list) and 1 <= len(values) <= 1024 and all(type(v) is int and 0 <= v <= 0xffffffff for v in values), 'PAGE_WORDS')
                stride = segment['strideWords']
                require(page['stride'] == stride and len(values) % stride == 0 and page['rowCount'] == len(values) // stride and page['firstRow'] * stride * 4 + segment['byteOffset'] == offset and page['byteOffset'] == offset, 'WHOLE_ROW_OFFSET')
                data = struct.pack('<' + 'I' * len(values), *values)
                require(offset + len(data) <= segment['byteOffset'] + segment['byteLength'], 'SEGMENT_OVERFLOW')
                raw.write(data)
                payload_sha.update(data)
                offset += len(data)
                if offset == segment['byteOffset'] + segment['byteLength']:
                    cursor_segment += 1
            pages_sha.update(line)
            file_bytes += len(line)
            count += 1
        require(summary is not None and manifest is not None and manifest.get('state') in ('COMPLETE', 'FORENSIC_INCOMPLETE') and count == summary['pages'] and cursor_segment == len(directory), 'INCOMPLETE_DIRECTORY')
        require(manifest['pages'] == count and manifest['payloadBytes'] == offset and manifest['pagesFileBytes'] == file_bytes and manifest['pagesFileSha256'] == pages_sha.hexdigest() and manifest['payloadRawSha256'] == payload_sha.hexdigest(), 'MANIFEST_LENGTH_OR_SHA')
        for key in ('requestId', 'traceId', 'scope', 'mode', 'K', 'Kw'):
            require(manifest.get(key) == summary.get(key), 'MANIFEST_SCOPE:' + key)
        prefix = words_at(raw, 0, 8192)
        require(prefix[:4] == (826488150, 2, 256, 3235775746) and prefix[15] == 1 and prefix[7] == summary['mode'] and pair(prefix, 18) == int(summary['traceId']) and pair(prefix, 16) == int(summary['requestId']) and pair(prefix, 42) == pair(prefix, 44) == offset, 'HEADER_SCOPE_OR_DIGEST_POLICY')
        require(not any(prefix[100:256]) and not any(prefix[256 + len(directory) * 32:2304]), 'HEADER_OR_DESCRIPTOR_RESERVED')
        original = struct.pack('<8192I', *prefix)
        canonical = original[:1116] + bytes(32) + original[1148:]
        sha = hashlib.sha256(canonical).hexdigest()
        require(sha == struct.pack('<8I', *prefix[279:287]).hex() == summary['canonicalPrefixSha256'] == manifest['canonicalPrefixSha256'], 'CANONICAL_PREFIX_SHA')
        require(hashlib.sha256(original).hexdigest() == summary['rawPrefixSha256'] == manifest['rawPrefixSha256'], 'RAW_PREFIX_SHA')
        descriptors = [prefix[256 + i * 32:288 + i * 32] for i in range(len(directory))]
        positions = {'kind': 0, 'rootIndex': 1, 'role': 2, 'trial': 3, 'arena': 6, 'strideWords': 13, 'segmentRevision': 22, 'seal': 31}
        for i, (schema, d) in enumerate(zip(directory, descriptors)):
            fixed = schema['fixed']
            for name, at in positions.items():
                require(d[at] == fixed[name], 'DESCRIPTOR_FIXED:' + name)
            require(pair(d, 9) == fixed['byteOffset'] and pair(d, 11) == fixed['byteLength'] and pair(d, 7) == (int(summary['traceId']) if i else 0) and d[21] in (1, 2, 4) and d[14] == d[16] <= d[15] <= schema['capacityRows'], 'DESCRIPTOR_RANGE_OR_COUNTS')
            if i:
                require(digest(raw, pair(d, 9), pair(d, 11), d[21] == 1) == struct.pack('<8I', *d[23:31]), 'SEGMENT_SHA')
            if d[21] == 1:
                require(not any(d[14:21]) and pair(d, 4) == 0, 'NOT_EXECUTED_COUNTS')
            if d[21] == 4:
                require(prefix[12] & 1, 'FORENSIC_HEADER_FLAG')
        if summary['mode'] == 2:
            for root in range(1, 5):
                validate_meta(raw, directory, descriptors, root)
        # The remaining undefined cursor64/config-capsule/root-token preimage
        # is a real producer contract prerequisite, never defaulted to zero.
        return {'revision': 'T596_V5_ARCHIVE_READER_V2', 'state': 'PREFIX_IDENTITY_CONTRACT_REQUIRED', 'knownContainerAndMeta2': 'CONSISTENT', 'physicsVerdict': 'NOT_EVALUATED', 'productPass': False, 'traceId': summary['traceId'], 'mode': summary['mode'], 'bytes': offset}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('--receipt', required=True, type=Path)
    args = parser.parse_args()
    try:
        report = read_archive(args.archive)
    except (ValueError, KeyError, TypeError, OSError, struct.error) as error:
        report = {'state': 'INVALID_OR_INCOMPLETE_ARCHIVE', 'reason': str(error), 'productPass': False}
    args.receipt.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    raise SystemExit(0 if report['state'] == 'COMPLETE' else 1)
