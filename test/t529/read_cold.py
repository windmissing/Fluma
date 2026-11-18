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
    manifest = None
    revision = None
    with tempfile.TemporaryFile() as raw, path.open('rb') as archive:
        for line in archive:
            require(0 < len(line) <= 16384 and line.endswith(b'\n'), 'PAGE_UTF8_BOUND_OR_TRUNCATED')
            archive_sha.update(line)
            page = json.loads(line.decode('ascii'))
            if page.get('revision') == 'G9_COLD_ARCHIVE_MANIFEST_V2':
                require(revision == 2 and manifest is None and metadata is not None and page_count == metadata['pages'], 'MANIFEST_ORDER')
                manifest = page
                continue
            require(manifest is None, 'PAGE_AFTER_MANIFEST')
            if revision is None:
                revision = 2 if page.get('payloadRevision') == 'G9_SPF_COLD_V2' else 1
            require(page.get('revision') == f'G9_SPF_SEGMENT_PAGE_V{revision}' and page.get('payloadRevision') == f'G9_SPF_COLD_V{revision}', 'UNSUPPORTED_COLD_REVISION')
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
        require(prefix[:5] == (0x31433947, revision, 8192, SEAL, PROFILE), 'PREFIX_IDENTITY')
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
        touch = validate_touch_v2(raw, prefix, metadata, manifest, descriptors) if revision == 2 else {'inputDomain': 'LEGACY_SYNTHETIC'}
        return {'touchEvidence': touch, 'revision': 'T599_G9_COLD_READER_V2', 'archiveSha256': archive_sha.hexdigest(), 'prefixSha256': digest_range(raw, 0, 8192).hex(), 'scope': scope, 'pages': page_count, 'capacityBytes': word_offset * 4, 'segments': descriptors, 'physicsOrDeviceVerdict': 'NOT_EVALUATED'}


def raw_sha(words):
    return hashlib.sha256(struct.pack('<' + 'I' * len(words), *words)).hexdigest()


def binary64(words, at):
    return struct.unpack('<d', struct.pack('<2I', *words[at:at + 2]))[0]


def validate_touch_v2(raw, prefix, summary, manifest, descriptors):
    import math
    root = Path(__file__).resolve().parents[2]
    contract = json.loads((root / 'docs/architecture-T543-g9-adapter-manifest-v1.json').read_text(encoding='utf8'))
    seed = json.loads((root / 'docs/architecture-T552-g9-seed-manifest-v2.json').read_text(encoding='utf8'))
    require(manifest is not None and manifest.get('state') == 'COMPLETE' and manifest.get('traceId') == summary['traceId'] and manifest.get('pages') == summary['pages'] and manifest.get('words') * 4 == int(summary['capacityBytes']), 'MISSING_OR_INVALID_V2_MANIFEST')
    prefix_sha = raw_sha(prefix)
    require(prefix_sha == summary.get('rawPrefixSha256') == manifest.get('rawPrefixSha256'), 'RAW_PREFIX_SHA_MISMATCH')
    config = prefix[3584:3648]
    require(tuple(seed['config64']) == config, 'CONFIG64_R2_IDENTITY')
    parameter_sha = raw_sha(config)
    require(parameter_sha == struct.pack('<8I', *prefix[656:664]).hex(), 'CONFIG64_PARAMETER_SHA')
    meta = prefix[3648:3712]
    require(meta[:4] == (844314951, 1, 64, 3235776770) and meta[36] == 2 and pair(meta, 28) == 128 * 41943040 and pair(meta, 30) == 1 and meta[32:34] == (1, 1) and pair(meta, 34) == 673792 and not any(meta[37:]) and not any(prefix[3712:]), 'SEED_META64_R2')
    require(meta[4:12] == config[32:40] and raw_sha(config) == struct.pack('<8I', *meta[20:28]).hex(), 'SEED_PARAMETER_BINDING')
    fields = {name: i for i, name in enumerate(contract['touchEvidence']['fields'])}
    input_words = prefix[512:640]
    base_capsule = read_words(raw, descriptors[0]['wordOffset'] + 384, 128)
    result = {'inputDomain': 'REAL_TOUCH_R2', 'clockQualification': 'PLATFORM_SOURCE_CHAIN_REQUIRES_INDEPENDENT_EVIDENCE', 'records': []}
    for index, start in enumerate((3072, 3328)):
        record = prefix[start:start + 256]
        key = 'currentTouch' if index == 0 else 'baseTouch'
        present = bool(record[0])
        require(summary.get('clockProofState') == manifest.get('clockProofState') == 'RECORDED_PROOF_REQUIRES_PLATFORM_EVIDENCE', 'CLOCK_PROOF_STATE')
        for suffix, offset in (('ClockProofSha256', 166), ('PlatformSourceSha256', 190)):
            name = ('current' if index == 0 else 'base') + suffix
            require(summary.get(name) == manifest.get(name) == struct.pack('<8I', *record[offset:offset + 8]).hex(), 'CLOCK_PROOF_SOURCE_BINDING')
        require(summary.get(key + 'Present') is present and manifest.get(key + 'Present') is present, 'TOUCH_PRESENCE_MISMATCH')
        require(raw_sha(record) == summary.get(key + 'Sha256') == manifest.get(key + 'Sha256'), 'TOUCH_RAW_SHA_MISMATCH')
        if not present:
            require(not any(record), 'ABSENT_TOUCH_NONZERO')
            if index == 0:
                require(not (input_words[40] == 2 and input_words[45] & 4), 'CURRENT_TOUCH_MISSING')
            else:
                require(not (base_capsule[40] == 2 and base_capsule[45] & 4), 'BASE_TOUCH_MISSING')
            result['records'].append({'present': False})
            continue
        require(record[:4] == (844380487, 1, 256, 3235776770), 'TOUCH_RECORD_DICTIONARY')
        require(all(record[i] == 0 for name, i in fields.items() if name.startswith('reserved_')), 'TOUCH_RESERVED_NONZERO')
        require(record[99] == 53401 and record[198:202] == (1, 1, 1, 2) and pair(record, 38) > 0, 'TOUCH_PRODUCER_IDENTITY')
        body = input_words if index == 0 else base_capsule
        require(body[1] == 2 and body[25] == 3 and body[40] == 2 and body[77] == 53401 and pair(record, 26) == pair(body, 41), 'TOUCH_COMMAND_BINDING')
        require(record[118:126] == body[26:34] == meta[20:28] and record[126:134] == config[24:32] and record[134:142] == meta[4:12] and record[142:150] == meta[12:20], 'TOUCH_CONFIG_SEED_BINDING')
        require(record[8:10] == body[4:6] and record[10:12] == body[6:8] and record[12:14] == body[8:10] and record[14:16] == body[12:14], 'TOUCH_SCOPE_BINDING')
        stable = tuple(body[i] for i in contract['input']['dedupWords'])
        require(raw_sha(stable) == struct.pack('<8I', *record[150:158]).hex(), 'TOUCH_STABLE_BODY_SHA')
        require(raw_sha(body) == struct.pack('<8I', *record[182:190]).hex() and pair(record, 36) == pair(body, 20), 'TOUCH_FROZEN_INPUT_SHA')
        down, up = pair(record, 28), pair(record, 30)
        require(0 < down <= up <= 9007199254740991 and 8333334 <= up - down <= 1000000000 and pair(record, 90) == up - down and pair(body, 43) == up and pair(record, 92) == down and pair(record, 94) == up, 'TOUCH_DURATION_OR_CAPTURE')
        require(binary64(record, 232) == down and binary64(record, 234) == up and pair(record, 32) >= down and pair(record, 34) >= up and pair(record, 34) - up <= 250000000 and pair(record, 36) >= up and pair(record, 36) - up <= 250000000, 'TOUCH_CLOCK_OR_AGE')
        require(record[103] == record[104] + 2 and record[104] <= 256 and record[102] == 1 and record[105] == 0, 'TOUCH_EVENT_COUNTS')
        scale = binary64(record, 64)
        left, top, width, height = (binary64(record, at) for at in (70, 72, 74, 76))
        require(all(math.isfinite(x) for x in (scale, left, top, width, height)) and scale > 0 and width > 0 and height > 0, 'TOUCH_GEOMETRY_FINITE')
        for local, model in ((40, 78), (44, 82)):
            px, py = binary64(record, local) * scale, binary64(record, local + 2) * scale
            require(left <= px < left + width and top <= py < top + height, 'TOUCH_HALF_OPEN_BOUNDARY')
            x = ((px - left) / width) * 128
            y = (((top + height) - py) / height) * 256
            require(struct.pack('<d', x) == struct.pack('<d', binary64(record, model)) and struct.pack('<d', y) == struct.pack('<d', binary64(record, model + 2)), 'TOUCH_EXACT_MAPPING')
        dx, dy = binary64(record, 82) - binary64(record, 78), binary64(record, 84) - binary64(record, 80)
        require(struct.pack('<d', dx) == struct.pack('<d', binary64(record, 86)) == struct.pack('<d', binary64(body, 50)) and struct.pack('<d', dy) == struct.pack('<d', binary64(record, 88)) == struct.pack('<d', binary64(body, 52)), 'TOUCH_DELTA_BITS')
        require(binary64(body, 46) == binary64(record, 82) and binary64(body, 48) == 4 and pair(body, 54) == up - down and record[216:224] == body[56:64], 'TOUCH_RECIPE_INPUT')
        qx, qy = math.floor(abs(dx) * 1048576), math.floor(abs(dy) * 1048576)
        require(abs(dx) <= 128 and abs(dy) <= 256 and qx * qx + qy * qy >= 262144 * 262144, 'TOUCH_RECIPE_DOMAIN')
        def rne(n, d):
            quotient, remainder = divmod(n, d)
            return quotient + (remainder * 2 > d or remainder * 2 == d and quotient % 2 == 1)
        vx, vy = rne(qx * 1000000000, 4 * (up - down)), rne((qy if dy > 0 else 0) * 1000000000, 4 * (up - down))
        flags = 4 | (vx > 24 * 1048576) | ((vy > 32 * 1048576) << 1)
        vx, vy = min(vx, 24 * 1048576), min(vy, 32 * 1048576)
        jx, jy = 65536 * vx * (-1 if dx < 0 else 1), 65536 * vy
        cap = 8 * 65536 * (vx * vx + vy * vy) * (1 << 24)
        require(body[45] == flags and record[106] == flags & 3 and pair(body, 56) == jx % (1 << 64) and pair(body, 58) == jy and sum(body[60 + i] << (32 * i) for i in range(4)) == cap and (jx or jy), 'TOUCH_GPU_RECIPE_FIELDS')
        require(record[158:166] == record[208:216] and pair(record, 108) == pair(record, 202) and pair(record, 110) == pair(record, 204) and any(record[166:174]) and any(record[190:198]), 'TOUCH_SCENE_OR_PROOF_BINDING')
        require(raw_sha(record[40:86]) == struct.pack('<8I', *record[174:182]).hex(), 'TOUCH_COORDINATE_SHA')
        result['records'].append({'present': True, 'rawSha256': raw_sha(record), 'commandSeq': str(pair(record, 26))})
    return result


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
