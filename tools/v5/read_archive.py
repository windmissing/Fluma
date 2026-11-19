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

def validate_identity(prefix,table):
    capsule=prefix[6688:7712]
    require(capsule[0]==1178809686 and capsule[2]==1024 and capsule[3]==3235775761,'IDENTITY_CONTRACT_MISSING')
    require(capsule[1]==1,'UNSUPPORTED_IDENTITY_REVISION')
    for field in table['fullColdConfigCapsule']['fields']:
        if 'constant' in field: require(capsule[field['word']]==field['constant'],'CAPSULE_CONSTANT_OR_RESERVED:'+field['name'])
    require(capsule[4:6]==prefix[8:10] and capsule[11]==int(prefix[7]==2),'CAPSULE_PROFILE')
    for at,lo,hi in ((8,1,2**32-1),(9,0,2),(10,1,1),(12,0,1966080),(13,0,1006632960),(14,120,1073741824),(15,1,1073741824),(16,0,1073741824),(17,0,960),(18,0,960),(19,1,960),(20,0,605393),(22,0,62914560),(23,1,1048576),(24,1,7864320),(25,1,62914560),(26,1,67108864),(27,0,314572800)):
        require(lo<=capsule[at]<=hi,'CAPSULE_RANGE:'+str(at))
    require(capsule[58:66]==prefix[52:60] and capsule[34:42]==prefix[60:68] and capsule[42:50]==prefix[68:76] and not any(prefix[7712:]),'CAPSULE_BINDING_OR_TAIL')
    capsule_sha=struct.unpack('<8I',hashlib.sha256(struct.pack('<1024I',*capsule)).digest())
    for root in range(prefix[10]):
        rec=prefix[6048+128*root:6176+128*root];cursor=prefix[5184+64*root:5248+64*root]
        require(rec[30:34]==(2304+256*root,3584+64*root,3904+256*root,5184+64*root) and not any(rec[66:]),'ROOT_POINTERS_OR_RESERVED')
        data=[prefix[rec[30]:rec[30]+256],prefix[rec[31]:rec[31]+64],prefix[rec[32]:rec[32]+256]]
        if rec[7]==1:
            require(not any(cursor) and not any(rec[1:7]) and not any(rec[8:30]) and not any(rec[42:66]) and all(not any(x) for x in data),'NOT_EXECUTED_IDENTITY')
            continue
        require(rec[7] in (2,4),'ROOT_STATE')
        require(cursor[0]==826488146 and cursor[2]==64 and cursor[3]==3235775761,'IDENTITY_CONTRACT_MISSING')
        require(cursor[1]==1,'UNSUPPORTED_IDENTITY_REVISION')
        for field in table['rootCursor64']['fields']:
            if 'constant' in field: require(cursor[field['word']]==field['constant'],'CURSOR_REVISION_OR_RESERVED:'+field['name'])
        require(cursor[15]==3 and cursor[4]==rec[0] and cursor[58] in (0,1) and cursor[59] in (0,1),'CURSOR_AUTHORIZATION')
        for a,b in ((16,8),(18,20),(26,10),(28,12),(32,24)):
            require(pair(cursor,a)==pair(rec,b),'CURSOR_RECORD_ID')
        for a,b in ((34,28),(38,30),(40,40),(42,34),(44,38),(50,32)):
            require(pair(cursor,a)==pair(prefix,b),'CURSOR_SCOPE')
        require(rec[42:50]==capsule[58:66] and hashlib.sha256(struct.pack('<64I',*data[1])).digest()==struct.pack('<8I',*rec[50:58]),'ROOT_INPUT_OR_PARAMETER_SHA')
        preimage=(827609653,1)+prefix[24:42]+cursor+data[1]+data[2]+data[0]+rec[:30]+rec[42:50]+capsule[42:50]+capsule[34:42]+capsule[50:58]+capsule_sha
        require(len(preimage)==730 and hashlib.sha256(struct.pack('<730I',*preimage)).digest()==struct.pack('<8I',*rec[58:66]),'ROOT_TOKEN_SHA')
        require(rec[7]!=2 or prefix[12]&1,'FORENSIC_ROOT_FLAG')
    require(not any(prefix[6048+128*prefix[10]:6688]),'UNUSED_ROOT_RECORDS')
    for start,stride in ((2304,256),(3584,64),(3904,256),(5184,64)):
        require(not any(prefix[start+stride*prefix[10]:start+stride*5]),'UNUSED_ROOT_RAW_SLOTS')

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
        while True:
            line=lines.readline(16385)
            if not line: break
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
                expected_pages=1+sum((d['capacityRows']+(1024//d['fixed']['strideWords'])-1)//(1024//d['fixed']['strideWords']) for d in directory)
                require(page['pages']==expected_pages,'PAGE_CAPACITY_BOUND')
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
            require(count<summary['pages'],'EXCESS_PAGES')
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
        identity_table=json.loads((ROOT/'docs/architecture-T619-v5-public-identity-v1.json').read_text(encoding='utf8'))
        validate_identity(prefix,identity_table)
        require(bool(prefix[12]&1)==(manifest['state']=='FORENSIC_INCOMPLETE'),'MANIFEST_FORENSIC_STATE')
        return {'revision': 'T596_V5_ARCHIVE_READER_V2', 'state': manifest['state'], 'knownContainerAndMeta2': 'CONSISTENT', 'physicsVerdict': 'NOT_EVALUATED', 'productPass': False, 'traceId': summary['traceId'], 'mode': summary['mode'], 'bytes': offset}

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
