"""Exercise the reader using original T842 SOURCE GPU words and synthetic QUANTUM."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

from check import (CAPACITY, HEADER, PUBLICATION_BYTES, STATE_BYTES, TAIL,
                   join, need, pair, read_archive)

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'test/reports/t830'


def fixture():
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
    q[16:18] = s[20:22]
    q[20] += 1
    q[26:28] = [1, 2]
    q[32:34] = [s[33], s[32]]
    q[35] = 1 - s[35]
    q[36] += 1
    q[52:55] = [0, 0, CAPACITY]
    q[59:69] = [0] * 10
    cmd = [0] * 32
    cmd[:2] = [1129534023, 2]
    cmd[4:6], cmd[12:14], cmd[14:16], cmd[28:30] = s[6:8], s[38:40], s[59:61], s[40:42]
    parts = (TAIL.pack(*q), bytes(STATE_BYTES), b'', bytes(PUBLICATION_BYTES), bytes(STATE_BYTES),
             bytes(s[53] * 256), TAIL.pack(*s), struct.pack('<32I', *cmd))
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

    def rejects(fn, reason):
        try:
            fn()
        except ValueError as error:
            need(str(error) == reason, f'{reason}: {error}')
        else:
            raise AssertionError(reason)
        negatives.append(reason)

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
    print(json.dumps({'syntheticArchive': positive, 'originalSource': original,
                      'negativeControls': negatives, 'nativeCold': 'T847_PENDING'}, indent=2))


if __name__ == '__main__':
    main()
