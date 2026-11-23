"""Exercise the reader using original T842 SOURCE GPU words and synthetic QUANTUM."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

from check import (CAPACITY, HEADER, PUBLICATION_BYTES, STATE_BYTES, TAIL,
                   join, legacy_container_bytes, need, pair, read_archive)

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'test/reports/t830'


def fixture(next_frame=False):
    raw = (BASE / 'm1-source-window.jsonl').read_bytes()
    receipt = json.loads((BASE / 'm1-source-window-receipt.json').read_bytes())
    need(hashlib.sha256(raw).hexdigest() == receipt['stdoutSHA'], 'SOURCE_ORIGINAL_SHA')
    rows = list(map(json.loads, raw.splitlines()))
    states = ('RUNNING', 'M1_DRY_SOURCE_PRESENT_QUEUED', 'M1_DRY_SOURCE_PRESENT_RETIRED')
    source_rows = [next(r for r in rows if r['stage'] == 'M1_SOURCE_WSI' and r['state'] == state)
                   for state in states]
    # Load the sibling explicitly because this directory also has a check.py.
    from runpy import run_path
    original = run_path(str(ROOT / 'test/t842/check.py'))['join'](*source_rows)
    source = source_rows[1]
    s = source['m1Completion']
    q = s.copy()
    q[4] += 1
    q[14:16] = s[18:20]
    q[16:18] = s[20:22]
    q[20] += 1
    q[26:28] = [1, 2]
    q[32:34] = [s[33], s[32]]
    q[35] = 1 - s[35]
    q[36] += 1
    source_events = bytes([29]) * (s[53] * 256)
    if next_frame:
        q[22] += 1
        q[24] += 1
        q[51] = q[24] % 2
    q[52:55] = [0 if next_frame else s[53], s[53]+1, CAPACITY]
    q[59:69] = [0] * 10
    cmd = [0] * 32
    cmd[:2] = [1129534023, 2]
    cmd[4:6], cmd[12:14], cmd[14:16], cmd[28:30] = s[6:8], s[38:40], s[59:61], s[40:42]
    prefix = b'' if next_frame else source_events
    parts = (TAIL.pack(*q), bytes(STATE_BYTES), prefix + bytes([17]) * ((q[53] - q[52]) * 256),
             bytes(PUBLICATION_BYTES), bytes(STATE_BYTES), source_events,
             TAIL.pack(*s), struct.pack('<32I', *cmd))
    payload = b''.join(parts)
    digest = hashlib.sha256(payload).digest()
    blob = HEADER.pack(b'M1PCOLD1', *map(len, parts), digest) + payload
    frame = dict(source['windowFrame'], kind='M1_QUANTUM_WINDOW', acceptedRoot=str(pair(q, 4)),
                 run=str(pair(q, 6)), config=str(pair(q, 40)), sourceCommit=str(pair(q, 18)),
                 physicsCommit=str(pair(q, 20)), generation=str(pair(q, 36)), publicationSlot=q[35])
    queued = dict(source, state='M1_QUANTUM_PRESENT_QUEUED', windowFrame=frame,
                  m1ColdBytes=len(blob), m1ColdPayloadSHA=digest.hex(),
                  m1QuantumNonce=str(pair(q, 4)), m1QuantumCompletion=q, m1QuantumOwnerCode=0,
                  m1QuantumFenceVkResult=0, m1QuantumFenceNs='1', m1QuantumDeadlineNs='2')
    retired = dict(queued, state='M1_QUANTUM_PRESENT_RETIRED', m1PublicationReleased=True,
                   m1WsiResult=0, m1WsiRetireVkResult=0)
    return blob, [queued, retired], original


def main():
    blob, rows, original = fixture()
    negatives = []
    branch_negatives = []
    actual = {}

    def rejects(fn, reason, results=negatives):
        try:
            fn()
        except ValueError as error:
            need(str(error) == reason, f'{reason}: {error}')
        else:
            raise AssertionError(reason)
        results.append(reason)

    with TemporaryDirectory() as tmp:
        path = Path(tmp) / 'm1.bin'
        path.write_bytes(blob)
        archive = read_archive(path)
        positive = join(archive, rows)
        need(archive[0]['source_completion'] == TAIL.pack(*rows[0]['m1Completion']), 'RAW_WORDS')
        for altered, reason in (
            (blob[: HEADER.size - 1], 'TRUNCATED_HEADER'),
            (blob[:-1], 'TRUNCATED_PAYLOAD'),
            (blob + b'0', 'TRAILING_BYTES'),
            (blob[:8] + struct.pack('<I', 1028) + blob[12:], 'SECTION_LENGTH'),
            (blob[:-1] + bytes([blob[-1] ^ 1]), 'PAYLOAD_SHA'),
        ):
            path.write_bytes(altered)
            rejects(lambda: read_archive(path), reason)
        for idx, key, value, reason in (
            (0, 'm1QuantumNonce', '42', 'NONCE_OR_GPU_WORDS'),
            (0, 'm1QuantumOwnerCode', 1, 'OWNER_OR_FENCE'),
            (0, 'windowFrame', dict(rows[0]['windowFrame'], generation='42'), 'PRESENT_IDENTITY'),
            (1, 'geometry', '42', 'RETIRE_OR_SCOPE'),
            (1, 'm1PublicationReleased', False, 'RETIRE_OR_SCOPE'),
        ):
            altered = deepcopy(rows)
            altered[idx][key] = value
            rejects(lambda: join(archive, altered), reason)
        next_blob, next_rows, _ = fixture(next_frame=True)
        path.write_bytes(next_blob)
        following = join(read_archive(path), next_rows)
        need(positive['lineage'] == 'SAME_FRAME_PREFIX' and following['lineage'] == 'NEXT_FRAME',
             'FIXTURE_LINEAGE')

        def reseal(source, offset, value):
            payload = bytearray(source[HEADER.size:])
            struct.pack_into('<I', payload, offset, value)
            return source[:40] + hashlib.sha256(payload).digest() + payload

        for source, offset, value, reason in (
            (blob, 1024 + STATE_BYTES, 0, 'SAME_FRAME_PREFIX'),
            (blob, 52*4, 0, 'SAME_FRAME_PREFIX'),
            (blob, 22*4, 2, 'SAME_FRAME_PERMIT'),
            (next_blob, 24*4, 4, 'NEXT_FRAME_OR_PERMIT'),
            (next_blob, 22*4, 3, 'NEXT_FRAME_OR_PERMIT'),
            (next_blob, 51*4, 1, 'EVENT_ARENA'),
            (next_blob, 52*4, 1, 'NEXT_FRAME_EVENT_START'),
        ):
            path.write_bytes(reseal(source, offset, value))
            rejects(lambda: read_archive(path), reason, branch_negatives)

        base = ROOT / 'test/reports/t856'
        for n in (2, 3):
            original_file = base / f'm1pcold1-run{n}.bin'
            fixed_header = base / f'm1pcold1-run{n}-normalized.bin'
            receipts = list(map(json.loads, (base / f'run{n}.jsonl').read_bytes().splitlines()))
            rejects(lambda: read_archive(original_file), 'TRUNCATED_PAYLOAD', branch_negatives)
            decoded = read_archive(fixed_header)
            rejects(lambda: join(decoded, receipts), 'COLD_RECEIPT_SIZE', branch_negatives)
            actual[f'run{n}'] = join(decoded, receipts, legacy_bytes=legacy_container_bytes(original_file, decoded))
            need(actual[f'run{n}']['lineage'] == 'NEXT_FRAME', 'T856_LINEAGE')
            raw = original_file.read_bytes()
            path.write_bytes(raw[:-1] + bytes([raw[-1] ^ 1]))
            rejects(lambda: legacy_container_bytes(path, decoded), 'LEGACY_CONTAINER_OR_PAYLOAD_CHANGED',
                    branch_negatives)
            other = 3 if n == 2 else 2
            other_rows = list(map(json.loads, (base / f'run{other}.jsonl').read_bytes().splitlines()))
            rejects(lambda: join(decoded, other_rows, legacy_bytes=len(raw)), 'COLD_RECEIPT', branch_negatives)
    print(json.dumps({'syntheticArchive': positive, 'originalSource': original,
                      'syntheticNextFrame': following, 'negativeControls': negatives,
                      'branchAndT856Controls': branch_negatives, 't856': actual,
                      'nativeCold': 'T856_ORIGINAL_REJECTED_T859_PENDING'}, indent=2))


if __name__ == '__main__':
    main()
