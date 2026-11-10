from pathlib import Path
import argparse
import importlib.util
import copy
import hashlib
import json
import struct

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('reader', root / 'entry/src/main/cpp/gpu_resident/m2/cold_reader.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def export(raw):
    words = list(struct.unpack('<' + 'I'*(raw.stat().st_size//4), raw.read_bytes()))
    c = words[:256]
    if raw.name == 'trace.bin':
        identity=json.loads((project/'t452-m2-manifest.json').read_text())
        assert c[114:122] == list(struct.unpack('<8I',bytes.fromhex(identity['shaderSha'])))
        assert c[130:138] == list(struct.unpack('<8I',bytes.fromhex(identity['layoutSha'])))
    k, kw = c[88]+c[89], c[90]+c[91]
    state = 16*(k+kw)+2304
    kind = c[27]
    stride = 128 if kind == 1 else 64
    rows = c[169] if kind == 1 else c[53]
    assert len(words) == 256+state+rows*stride
    token = 'GRS_GPU_ROOT_Q20_M2_V3' + ''.join('|'+str(c[i]) for i in list(range(4,28))+list(range(32,42))+list(range(114,138))+list(range(160,173)))
    meta = dict(state='TRACE_READY', revision='T445_M2_TRACE_V3', traceId='1', epoch='1', page=0,
                pages=(len(words)+1023)//1024, rootKind=kind, stride=stride, rootToken=token,
                rows=rows, arena=c[172 if kind==1 else 51], generationLo=c[36],generationHi=c[37],
                stateWords=state,eventWords=rows*stride,words=[])
    directory=raw.parent/(raw.stem+'-export');directory.mkdir(exist_ok=True)
    receipts=[]
    for i in range(meta['pages']):
        page={**meta,'page':i+1,'words':words[i*1024:(i+1)*1024]}
        body=json.dumps(page,separators=(',',':')).encode()
        name=f'page-{i+1}.json';(directory/name).write_bytes(body)
        receipts.append(dict(name=name,bytes=len(body),sha256=hashlib.sha256(body).hexdigest()))
    manifest=directory/'manifest.json';report=dict(header=meta,files=receipts)
    manifest.write_text(json.dumps(report),encoding='utf-8')
    result=reader.read_export(manifest)
    for field,value in [('stride',64 if kind==1 else 128),('revision','V2'),('rootToken','STALE')]:
        bad=copy.deepcopy(report);bad['header'][field]=value
        manifest.write_text(json.dumps(bad),encoding='utf-8')
        try:reader.read_export(manifest)
        except ValueError:pass
        else:raise AssertionError(field)
    manifest.write_text(json.dumps(report),encoding='utf-8')
    return dict(file=str(raw.relative_to(root)),K=k,Kw=kw,rootKind=kind,rows=rows,negativeCases=3)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--project',type=Path,default=root/'.codex/local-test/t452-complete')
    project=parser.parse_args().project
    paths=list((root/'.codex/local-test/t452-host').glob('raw-*.bin'))
    app=root/'.codex/local-test/t452-route-host/trace.bin'
    if app.exists():paths.append(app)
    assert paths
    results=[export(p) for p in paths]
    out=root/'test/reports/t452/cold-reader.json'
    out.write_text(json.dumps(dict(passed=len(results),failed=0,cases=results),indent=2),encoding='utf-8')
    print(f'cold exports {len(results)} / 0; negative checks {3*len(results)} / 0')
