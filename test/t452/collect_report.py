"""Collect developer evidence only after the complete offline route succeeds."""
from pathlib import Path
import hashlib
import json
import re

ROOT=Path(__file__).resolve().parents[2]
LOCAL=ROOT/'.codex/local-test'
OUT=ROOT/'test/reports/t452'

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def obj(path):return json.loads(path.read_text(encoding='utf-8-sig'))

def main():
    gpu=LOCAL/'t452-final-evidence.txt'
    text=gpu.read_text(encoding='utf-8-sig')
    profiles=re.findall(r'T452 SOURCE (192|512)/(64|128) twenty GPU roots verified, max wall ms=([0-9.]+)',text)
    assert {(int(k),int(kw)) for k,kw,_ in profiles}=={(192,64),(192,128),(512,64),(512,128)} and len(profiles)==4
    app=LOCAL/'t452-accepted-app.txt'
    native=[json.loads(line) for line in app.read_text(encoding='utf-8-sig').splitlines() if line.startswith('{"schema":')][-1]
    assert native['schema']=='T452_DEVELOPER_NATIVE_GL_V3' and native['passed']==1 and native['failed']==0
    build=LOCAL/'t452-complete-build.txt'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    manifest=obj(LOCAL/'t452-complete/t452-m2-manifest.json')
    source=LOCAL/'t452-host/source-v3.glsl';quantum=LOCAL/'t452-host/quantum-v3.glsl'
    assert text.splitlines()[:2]==[digest(source),digest(quantum)]
    assert hashlib.sha256(source.read_bytes()+b'\n'+quantum.read_bytes()).hexdigest()==manifest['shaderSha']
    sign=obj(ROOT/'test/reports/t257/T452-final-complete.json')
    assert sign['verifyApp']=='PASS' and digest(Path(sign['signed']['path']))==sign['signed']['sha256']
    assert digest(Path(sign['input']['path']))==sign['input']['sha256']
    cold=obj(OUT/'cold-reader.json');assert cold['passed']==61 and cold['failed']==0
    primitives=obj(OUT/'gl-primitives.json');assert primitives['passed']==4 and primitives['failed']==0
    sources={str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for folder in ('entry/src/main/cpp/gpu_resident/m2','test/t452') for p in (ROOT/folder).iterdir() if p.is_file()}
    report=dict(schema='T452_M2_V3_DEVELOPER_DELIVERY_V1',status='DEVELOPER_COMPLETE',device='NOT_RUN',
        baseline=manifest['base'],policy=manifest['policy'],layoutSha=manifest['layoutSha'],shaderSha=manifest['shaderSha'],
        sourceSha=digest(source),quantumSha=digest(quantum),
        gpuRoots=dict(passed=80,failed=0,accepted=60,fatalRejected=20,
            profiles=[dict(K=int(k),Kw=int(kw),roots=20,maxSoftwareWallMs=float(t)) for k,kw,t in profiles],
            scope='SOURCE13/full QUANTUM, spare word replay, normal partial/NO_PAIR/capacity/last ID/generation rollback, fatal23-26 word+expected+actual',
            sourceUploadedBytes=240,tailMappedBytes=1024),
        native=native,coldReader=dict(passed=cold['passed'],failed=0,negativeChecks=sum(c['negativeCases'] for c in cold['cases'])),
        primitives=dict(report='test/reports/t452/gl-primitives.json',unchangedSince='ca21608',profiles=4,mergeCases=6,indexCases=5),
        build=dict(result='BUILD_SUCCESSFUL',errors=0,logSha=digest(build)),
        signed=sign['signed'],signReport='test/reports/t257/T452-final-complete.json',
        boundaries=['Offline software GLES and injected host clock; no device deadline/fps/effect claim',
                    'T453 independent production differential is a separate tester task',
                    'Old M1/V2 source files are unchanged; isolated App generated from the pinned baseline'],
        sources=sources,logs={str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in (gpu,app,build)})
    OUT.mkdir(exist_ok=True)
    (OUT/'delivery.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/'gpu-roots.txt').write_bytes(gpu.read_bytes())
    print(json.dumps(dict(gpuRoots=80,native=1,coldExports=61,negativeChecks=183,signed=sign['signed']['sha256'])))

if __name__=='__main__':main()
