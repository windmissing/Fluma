"""G9 cold archive reader; strict independent version, bounded streaming storage.

This is an evidence reader, not a host dynamics implementation. It writes no
simulation state and never substitutes a product CPU solver for the GPU.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

SEAL = 3235776769
PROFILE = 826691667
SCHEMA = Path(__file__).resolve().parents[2] / 'entry/src/main/cpp/gpu_resident/g9spf/reader_schema.json'


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def pair(words, at):
    return words[at] | words[at + 1] << 32


def read_words(file, at, count):
    file.seek(at * 4)
    raw = file.read(count * 4)
    require(len(raw) == count * 4, 'TRUNCATED_RAW_SEGMENT')
    return struct.unpack('<' + 'I' * count, raw)


def digest_range(file, at, count):
    file.seek(at * 4)
    sha = hashlib.sha256()
    remaining = count * 4
    while remaining:
        raw = file.read(min(65536, remaining))
        require(bool(raw), 'TRUNCATED_DIGEST_RANGE')
        sha.update(raw)
        remaining -= len(raw)
    return sha.digest()


def read_archive(path):
    schema = json.loads(SCHEMA.read_text(encoding='utf8'))
    archive_sha = hashlib.sha256()
    page_count = 0
    word_offset = 0
    metadata = None
    scope = None
    with tempfile.TemporaryFile() as raw, path.open('rb') as archive:
        for line in archive:
            require(0 < len(line) <= 16384 and line.endswith(b'\n'), 'PAGE_UTF8_BOUND_OR_TRUNCATED')
            archive_sha.update(line)
            page = json.loads(line.decode('ascii'))
            require(page.get('revision') == 'G9_SPF_SEGMENT_PAGE_V1' and page.get('payloadRevision') == 'G9_SPF_COLD_V1', 'UNSUPPORTED_COLD_REVISION')
            require(page.get('state') == 'TRACE_READY' and page.get('page') == page_count, 'PAGE_ORDER_OR_STATE')
            if metadata is None:
                metadata = page
                scope = page['scope']
                require(page['K'] in (192, 512) and page['words'] == [], 'SUMMARY_SCHEMA')
                require(isinstance(page['pages'], int) and 9 <= page['pages'] <= 50000, 'PAGE_COUNT')
                require(0 < int(page['capacityBytes']) <= 179671040, 'CAPACITY_LIMIT')
            else:
                require(page['traceId'] == metadata['traceId'] and page['scope'] == scope and page['pages'] == metadata['pages'], 'IMMUTABLE_SCOPE_CHANGED')
                words = page['words']
                require(page.get('wordOffset') == word_offset and 1 <= len(words) <= 1024 and page.get('stride') == 1, 'SEGMENT_ORDER')
                require(all(type(w) is int and 0 <= w <= 0xffffffff for w in words), 'NON_U32_WORD')
                require(word_offset * 4 + len(words) * 4 <= int(metadata['capacityBytes']), 'CAPACITY_OVERFLOW')
                raw.write(struct.pack('<' + 'I' * len(words), *words))
                word_offset += len(words)
            page_count += 1
        require(metadata is not None and page_count == metadata['pages'] and word_offset * 4 == int(metadata['capacityBytes']), 'INCOMPLETE_ARCHIVE')
        prefix = read_words(raw, 0, 8192)
        require(prefix[:5] == (0x31433947, 1, 8192, SEAL, PROFILE), 'PREFIX_IDENTITY')
        require(prefix[7] == metadata['K'] and bool(prefix[5]) == metadata['fullTrialTrace'], 'PROFILE_MISMATCH')
        require(pair(prefix, 8) == word_offset * 4 and str(pair(prefix, 12)) == metadata['traceId'], 'PREFIX_LENGTH_OR_TRACE')
        profile = next(p for p in schema['profiles'] if p['K'] == prefix[7])
        state_words = sum(s['bytes'] for s in profile['stateSegments']) // 4
        arena_words = profile['eventCapacity'] * 64 + profile['pathCapacity'] * 16
        sizes = [state_words, state_words, arena_words, arena_words] + ([arena_words] * 6 if prefix[5] else [])
        require(prefix[6] == len(sizes), 'DESCRIPTOR_COUNT')
        cursor = 8192
        descriptors = []
        for i, count in enumerate(sizes):
            d = prefix[2048 + i * 32:2048 + (i + 1) * 32]
            require(d[0] == i + 1 and pair(d, 8) == cursor and pair(d, 10) == count and d[12] == 1 and d[31] == SEAL, 'DESCRIPTOR_LAYOUT')
            require(d[3] in (1, 2), 'SEGMENT_STATUS')
            sha = digest_range(raw, cursor, count)
            require(sha == struct.pack('<8I', *d[23:31]), 'SEGMENT_SHA256_MISMATCH')
            if d[3] == 1:
                raw.seek(cursor * 4)
                remaining = count * 4
                while remaining:
                    block = raw.read(min(65536, remaining))
                    require(bool(block) and not any(block), 'NOT_EXECUTED_NONZERO')
                    remaining -= len(block)
            descriptors.append({'id': i + 1, 'wordOffset': cursor, 'words': count, 'sha256': sha.hex(), 'status': d[3]})
            cursor += count
        require(cursor == word_offset, 'UNACCOUNTED_SEGMENT')
        tail = prefix[256:512]
        require(tail[0] == 826489159 and tail[1] == 1 and tail[2] == 256 and tail[3] == SEAL and tail[4] == PROFILE and tail[42] == 0xffff, 'COMPLETION_SCHEMA')
        require(not any(tail[156:]), 'COMPLETION_RESERVED_NONZERO')
        require(tail[43] == tail[44] <= profile['eventCapacity'] and tail[45] == tail[46] <= profile['pathCapacity'], 'UNSEALED_PREFIX')
        require(tail[39] == tail[40] == tail[41] == (12 if tail[5] == 1 else 2024), 'PASS_TABLE_MISMATCH')
        require(prefix[768:772] == (860963661, 3, 256, 0xc0de0003), 'QUOTA_REVISION')
        for descriptor in descriptors[:2]:
            h = read_words(raw, descriptor['wordOffset'], 256)
            require(h[0] == 827537735 and h[1] == 1 and h[2] == PROFILE and h[24] == prefix[7] and pair(h, 4) == pair(prefix, 16) and pair(h, 8) == pair(prefix, 18), 'STATE_SCOPE_MISMATCH')
        # Full selected/authoritative original streams, role bits only normalized.
        event_masks = {e['code']: e['usedMaskU32'] for e in schema['event']['kinds']}
        selected, authority = descriptors[2]['wordOffset'], descriptors[3]['wordOffset']
        for row in range(tail[44]):
            a = list(read_words(raw, selected + row * 64, 64))
            b = list(read_words(raw, authority + row * 64, 64))
            mask = event_masks.get(a[2])
            require(mask is not None and a[0] == b[0] == 826620231 and a[1] == b[1] == 1 and a[63] == b[63] == SEAL, 'EVENT_SCHEMA')
            for word in range(64):
                require((mask[word // 32] >> (word % 32)) & 1 or a[word] == b[word] == 0, 'EVENT_UNUSED_NONZERO')
            a[14] &= ~3
            b[14] &= ~3
            require(a == b, 'SELECTED_AUTHORITATIVE_EVENT_MISMATCH')
        path_at = profile['eventCapacity'] * 64
        for row in range(tail[46]):
            a = read_words(raw, selected + path_at + row * 16, 16)
            b = read_words(raw, authority + path_at + row * 16, 16)
            require(a == b and a[15] == SEAL and a[14] == 0, 'SELECTED_AUTHORITATIVE_PATH_MISMATCH')
        return {'revision': 'T529_G9_COLD_READER_V1', 'archiveSha256': archive_sha.hexdigest(), 'prefixSha256': digest_range(raw, 0, 8192).hex(), 'scope': scope, 'pages': page_count, 'capacityBytes': word_offset * 4, 'segments': descriptors, 'physicsOrDeviceVerdict': 'NOT_EVALUATED'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    try:
        receipt = read_archive(args.archive)
    except (ValueError, KeyError, OSError, StopIteration, struct.error) as error:
        receipt = {'revision': 'T529_G9_COLD_READER_V1', 'state': 'INVALID_OR_INCOMPLETE_ARCHIVE', 'reason': str(error)}
        args.receipt.write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
        raise SystemExit(1)
    args.receipt.write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
