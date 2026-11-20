"""Validate an original G11 worker manifest and its streamed archive."""
import hashlib
import json
from pathlib import Path
import re
import sys

def archive(manifest, path, mode):
    revision = manifest.get('revision', manifest.get('schema'))
    if revision == 'G11_VIS_COLD_V2':
        assert mode in ('ROOT', 'FULL') and manifest['state'] == 'TRACE_READY'
        assert manifest['headerWords'] == 8192
        assert len(manifest['segments']) == {'ROOT': 11, 'FULL': 17}[mode]
        segments = [(0, 32768, None, 1)]
        for i, s in enumerate(manifest['segments'], 1):
            assert s['id'] == i
            segments.append((s['byteOffset'], s['bytes'], s['sha256'], s['strideWords']))
        length, expected, ident = int(manifest['bytes']), manifest['dataSHA256'], manifest['traceId']
        assert 32768 < length <= 194154496
    elif revision == 'G11_VIS_SEED_RAW_V1':
        assert mode == 'BOOTSTRAP' and manifest['state'] == 3
        assert len(manifest['segments']) == 4
        segments = []
        for i, s in enumerate(manifest['segments'], 1):
            assert s['kind'] == i
            segments.append((s['preimageOffset'], s['bytes'], s['sha256'], 1))
        length, expected, ident = int(manifest['rawBytes']), manifest['actualVisualSeedSHA'], manifest['id']
        assert length in (271104, 1057536)
    elif revision == 'G11_PHASE_WITNESS_V2':
        assert mode == 'WITNESS' and manifest['state'] == 'READY'
        length, expected, ident = manifest['bytes'], manifest['sha256'], manifest['id']
        assert length in (3149824, 4722688)
        segments = [(0, length, expected, 1)]
    else:
        raise ValueError('UNSUPPORTED_ARCHIVE_REVISION')
    assert isinstance(ident,str) and re.fullmatch('[1-9][0-9]*', ident) and int(ident) < 2**64
    assert path.stat().st_size == length and re.fullmatch('[0-9a-f]{64}',expected)
    aggregate = hashlib.sha256()
    with path.open('rb') as stream:
        for offset, size, digest, stride in segments:
            assert stream.tell() == offset and stride in (1,4,8,32,64)
            assert type(size) is int and 0 < size <= length-offset and size % (4*stride) == 0
            local = hashlib.sha256()
            remaining = size
            while remaining:
                block = stream.read(min(remaining, 1024*1024))
                assert block
                local.update(block)
                aggregate.update(block)
                remaining -= len(block)
            if digest is not None:
                assert local.hexdigest() == digest, 'SEGMENT_SHA'
        assert stream.tell() == length and not stream.read(1)
    assert aggregate.hexdigest() == expected, 'ARCHIVE_SHA'
    return {'mode':mode,'id':ident,'bytes':length,'archiveIntegrity':'PASS','productPASS':False}

if __name__ == '__main__':
    mode, manifest, raw = sys.argv[1:]
    data = Path(manifest).read_bytes()
    assert len(data) <= 16384
    print(json.dumps(archive(json.loads(data), Path(raw), mode), indent=2))
