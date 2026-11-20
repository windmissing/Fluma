"""Validate committed blocks and replay the archived T496 prefix losslessly."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

EDGES = {'ENTER': 1, 'EXIT': 2, 'CPU_MAP_RETURN_BYTES': 3}
ROW = struct.Struct('<QQIHBBII')

def read(stem):
    files = {k: Path(stem + ext).read_bytes() for k, ext in
             [('R','.rows'), ('D','.dict'), ('F','.fn')]}
    marks = dict.fromkeys(files, 0)
    footer = Path(stem + '.end')
    state = footer.read_text().split()[0] if footer.exists() else 'INCOMPLETE'
    index = Path(stem + '.index').read_bytes()
    if state != 'SEALED' and index and not index.endswith(b'\n'):
        index = index[:index.rfind(b'\n') + 1]
    for line in index.decode('ascii').splitlines():
        kind, start, size, digest = line.split()
        start, size = int(start), int(size)
        assert start == marks[kind] and size > 0
        block = files[kind][start:start+size]
        assert len(block) == size and hashlib.sha256(block).hexdigest() == digest
        marks[kind] += size
    if state == 'SEALED':
        assert all(marks[k] == len(b) for k,b in files.items())
    dictionary = [None]
    at = 0
    while at < marks['D']:
        size, = struct.unpack_from('<I', files['D'], at)
        at += 4
        assert at + size <= marks['D']
        dictionary.append(files['D'][at:at+size])
        at += size
    functions = [x[0] for x in struct.iter_unpack('<I', files['F'][:marks['F']])]
    for seq, wall, tid, fn, edge, flags, detail, stamp in ROW.iter_unpack(files['R'][:marks['R']]):
        assert flags == 0
        yield (seq, wall, dictionary[stamp] or str(wall).encode(), tid, functions[fn], edge, dictionary[detail])

def compare(source, exe, stem):
    expected = []
    payload = bytearray()
    for line in Path(source).read_bytes().splitlines():
        r = json.loads(line)
        body = r['detail'].encode('utf8')
        stamp = r['scriptNs'].encode('ascii')
        seq, wall = r['callId'], int(r['wallNs'])
        tid, fn, edge = r['ownerTid'], r['functionId'], EDGES[r['event']]
        expected.append((seq, wall, stamp, tid, fn, edge, body))
        payload.extend(f'{seq} {wall} {int(stamp)} {tid} {fn} {edge} {len(body)}\n'.encode())
        payload.extend(body + b'\n')
    result = subprocess.run([exe,stem],input=payload,capture_output=True,check=True)
    assert int(result.stdout) == len(expected)
    assert list(read(stem)) == expected
    size = sum(p.stat().st_size for p in Path(stem).parent.glob(Path(stem).name + '.*'))
    assert size <= 12*1024*1024
    print(json.dumps({'rows':len(expected),'encodedBytes':size,'exactRoundTrip':True,'device':'NOT_RUN'}))

if __name__ == '__main__':
    if sys.argv[1] == '--decode':
        stem = sys.argv[2]
        schema, identity = Path(stem + '.meta').read_text().splitlines()
        assert schema == 'T693_LE32_V1'
        run, arm, attempt, pid = identity.split()
        names = {value:key for key,value in EDGES.items()}
        for seq, wall, stamp, tid, fn, edge, detail in read(stem):
            print(json.dumps({'schema': schema, 'runId': run, 'arm': arm,
                              'createAttemptId': attempt, 'pid': int(pid),
                              'callId': seq, 'wallNs': str(wall), 'scriptNs': stamp.decode(),
                              'ownerTid': tid, 'functionId': fn, 'event': names[edge],
                              'detail': detail.decode('utf8')}))
    else:
        compare(*sys.argv[1:])
