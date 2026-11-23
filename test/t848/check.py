"""Read the native T830 envelope and join it to an accepted WSI receipt."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

HEADER = struct.Struct('<8s8I32s')
TAIL = struct.Struct('<256I')
SEAL = (1381192263, 2, 256, 3235823838)
STATE_BYTES = 4 * (16 * (192 + 128) + 2048 + 256)
PUBLICATION_BYTES = STATE_BYTES + 4 * 4 * 128
CAPACITY = 4 * (48 * 192 + 16 * 128 + 254) + 192
LIMIT = 16 * 1024 * 1024
NAMES = ('quantum_completion', 'quantum_bank', 'quantum_events', 'quantum_publication',
         'source_bank', 'source_events', 'source_completion', 'source_command')
SCOPE = ('pid', 'processStartTick', 'ticket', 'ownerTid', 'surface', 'geometry',
         'm1SourceModuleSHA')


def need(ok, code):
    if not ok:
        raise ValueError(code)


def pair(words, i):
    return words[i] | words[i + 1] << 32


def read_archive(path):
    with path.open('rb') as stream:
        header = stream.read(HEADER.size)
        need(len(header) == HEADER.size, 'TRUNCATED_HEADER')
        magic, *sizes, digest = HEADER.unpack(header)
        need(magic == b'M1PCOLD1', 'MAGIC')
        need(tuple(sizes[i] for i in (0, 1, 3, 4, 6, 7)) ==
             (1024, STATE_BYTES, PUBLICATION_BYTES, STATE_BYTES, 1024, 128)
             and sizes[2] % 256 == sizes[5] % 256 == 0
             and sizes[2] <= CAPACITY * 256 and sizes[5] <= 193 * 256
             and sum(sizes) <= LIMIT, 'SECTION_LENGTH')
        data = stream.read(sum(sizes) + 1)
    need(len(data) >= sum(sizes), 'TRUNCATED_PAYLOAD')
    need(len(data) == sum(sizes), 'TRAILING_BYTES')
    need(hashlib.sha256(data).digest() == digest, 'PAYLOAD_SHA')
    sections = {}
    offset = 0
    for name, size in zip(NAMES, sizes):
        sections[name] = data[offset:offset + size]
        offset += size
    q, s = (TAIL.unpack(sections[name]) for name in ('quantum_completion', 'source_completion'))
    cmd = struct.unpack('<32I', sections['source_command'])
    need(q[:4] == s[:4] == SEAL and q[27:32] == (2, 1, 0, 0, 0)
         and s[26:32] == (0, 1, 1, 0, 0, 0) and s[61] > 0
         and 1 <= q[26] <= 4, 'UNACCEPTED_COMPLETION')
    need(q[52] <= q[53] <= q[54] == CAPACITY and s[52] == 0
         and 0 < s[53] <= s[54] == 193
         and q[53] * 256 == sizes[2] and s[53] * 256 == sizes[5], 'EVENT_LENGTH')
    need(all(pair(q, i) == pair(s, i) for i in (6, 8, 10, 12, 18, 38, 40))
         and 0 < pair(s, 4) < pair(q, 4)
         and q[26] == 1 and pair(q, 14) == pair(s, 18)
         and pair(q, 16) == pair(s, 20) and pair(q, 20) == pair(q, 16) + 1
         and pair(q, 36) == pair(s, 36) + 1 and q[32] == s[33] and q[35] in (0, 1),
         'ROOT_LINEAGE')
    lineage(q, s, sections)
    need(cmd[:2] == (1129534023, 2) and pair(cmd, 4) == pair(s, 6)
         and pair(cmd, 12) == pair(s, 38) and pair(cmd, 28) == pair(s, 40)
         and cmd[14:16] == s[59:61], 'SOURCE_COMMAND')
    return sections, digest.hex(), q, s


def lineage(q, s, sections):
    qframe, sframe = pair(q, 24), pair(s, 24)
    qpermit, spermit = pair(q, 22), pair(s, 22)
    need(sframe > 0 and spermit > 0, 'SOURCE_FRAME_OR_PERMIT')
    need(s[51] == sframe % 2 and q[51] == qframe % 2, 'EVENT_ARENA')
    if qframe == sframe:
        need(qpermit == spermit, 'SAME_FRAME_PERMIT')
        prefix = sections['source_events']
        need(q[52] == s[53] and sections['quantum_events'].startswith(prefix), 'SAME_FRAME_PREFIX')
        return 'SAME_FRAME_PREFIX'
    need(qframe == sframe + 1 and qpermit == spermit + 1, 'NEXT_FRAME_OR_PERMIT')
    need(q[52] == 0, 'NEXT_FRAME_EVENT_START')
    return 'NEXT_FRAME'


def legacy_container_bytes(original, archive):
    """Prove T856's diagnostic normalization preserved every payload byte."""
    sections, digest, _, _ = archive
    payload = b''.join(sections[n] for n in NAMES)
    prefix = HEADER.pack(b'M1PCOLD1', *map(len, sections.values()), bytes.fromhex(digest))
    expected = prefix[:48] + payload
    with original.open('rb') as stream:
        actual = stream.read(len(expected) + 1)
    need(actual == expected, 'LEGACY_CONTAINER_OR_PAYLOAD_CHANGED')
    return len(expected)


def join(archive, rows, *, legacy_bytes=None):
    sections, digest, q, s = archive
    matching = [r for r in rows if r.get('stage') == 'M1_SOURCE_WSI'
                and r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED'
                and r.get('m1ColdPayloadSHA') == digest]
    need(len(matching) == 1, 'COLD_RECEIPT')
    row = matching[0]
    full_bytes = HEADER.size + sum(map(len, sections.values()))
    receipt_bytes = full_bytes if legacy_bytes is None else legacy_bytes
    need(row['m1ColdBytes'] == receipt_bytes, 'COLD_RECEIPT_SIZE')
    need(str(pair(s, 4)) == row.get('m1Nonce') and str(pair(q, 4)) == row.get('m1QuantumNonce')
         and tuple(row.get('m1Completion', ())) == s
         and tuple(row.get('m1QuantumCompletion', ())) == q, 'NONCE_OR_GPU_WORDS')
    need(all(row.get(k) == 0 for k in ('m1OwnerCode', 'm1QueueVkResult', 'm1FenceVkResult',
                                      'm1QuantumOwnerCode', 'm1QuantumFenceVkResult'))
         and 0 < int(row['m1FenceNs']) <= int(row['m1DeadlineNs'])
         and 0 < int(row['m1QuantumFenceNs']) <= int(row['m1QuantumDeadlineNs']), 'OWNER_OR_FENCE')
    frame = row['windowFrame']
    keys = {'acceptedRoot': pair(q, 4), 'run': pair(q, 6), 'config': pair(q, 40),
            'sourceCommit': pair(q, 18), 'physicsCommit': pair(q, 20),
            'generation': pair(q, 36), 'publicationSlot': q[35]}
    need(frame.get('kind') == 'M1_QUANTUM_WINDOW'
         and all(int(frame[k]) == v for k, v in keys.items())
         and int(row['surface']) == pair(q, 12) and int(row['geometry']) == pair(q, 38)
         and int(row['m1PublicationGeneration']) == (pair(s, 36) & 0xffffffff), 'PRESENT_IDENTITY')
    need(all(frame[k] == 0 for k in ('acquire', 'submit', 'fence', 'present', 'imagePresent'))
         and 0 < int(frame['presentEnterQpc']) <= int(frame['presentExitQpc']), 'PRESENT_RESULT')
    retired = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_RETIRED'
               and r.get('stage') == row['stage'] and r.get('windowFrame') == frame
               and all(k in r and r[k] == row[k] for k in SCOPE)
               and r.get('m1ColdPayloadSHA') == digest
               and r.get('m1ColdBytes') == receipt_bytes
               and r.get('m1PublicationReleased') is True
               and r.get('m1WsiResult') == r.get('m1WsiRetireVkResult') == 0]
    need(len(retired) == 1, 'RETIRE_OR_SCOPE')
    return {'root': keys, 'scope': {k: row[k] for k in SCOPE}, 'payloadSHA256': digest,
            'lineage': lineage(q, s, sections),
            'sourceFrame': pair(s, 24), 'quantumFrame': pair(q, 24),
            'sourcePermit': pair(s, 22), 'quantumPermit': pair(q, 22),
            'sourceEventEnd': s[53], 'quantumEventStart': q[52], 'quantumEventEnd': q[53],
            'container': 'T856_PAYLOAD_ONLY_ORIGINAL_REJECTED' if legacy_bytes is not None else 'FULL_SHA256',
            'receiptBytes': receipt_bytes, 'decodedBytes': full_bytes,
            'sections': {k: {'bytes': len(v), 'sha256': hashlib.sha256(v).hexdigest()}
                         for k, v in sections.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('jsonl', type=Path)
    parser.add_argument('--t856-short-original', type=Path,
                        help='prove payload-only normalization against a separate original short-digest file')
    args = parser.parse_args()
    try:
        rows = [json.loads(line) for line in args.jsonl.read_bytes().splitlines() if line.strip()]
        archive = read_archive(args.archive)
        legacy_bytes = legacy_container_bytes(args.t856_short_original, archive) if args.t856_short_original else None
        result = join(archive, rows, legacy_bytes=legacy_bytes)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f'{error}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
