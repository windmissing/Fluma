"""Architecture-only schema/receipt audit. Never builds, executes shaders or uses a device.
--record writes only this batch's evidence; default verifies receipts without rewriting them.
"""
from pathlib import Path
import hashlib, json, math, struct, sys
from fractions import Fraction

ROOT = Path(__file__).resolve().parents[1]
D = ROOT / 'docs'
def sha(b): return hashlib.sha256(b).hexdigest()
def read(p): return json.loads((ROOT / p).read_text(encoding='utf-8-sig'))
def canon(x): return json.dumps(x, ensure_ascii=False, sort_keys=True, separators=(',', ':')).encode()
def receipt(p):
    b=(ROOT/p).read_bytes()
    return dict(path=p, bytes=len(b), sha256=sha(b))
def emit(p,x): (ROOT/p).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def checks():
    a=read('docs/architecture-T619-v5-public-identity-v1.json')
    c=a['rootCursor64']; cap=a['fullColdConfigCapsule']
    assert [f['word'] for f in c['fields']]==list(range(64))
    assert c['usedMaskLo']==0xffffffff and c['usedMaskHi']==0x3fffffff
    assert c['fields'][58]['name']=='lastGood' and c['fields'][58]['unit']=='boolean U32'
    assert c['fields'][61]['name']=='publicationSlot'
    assert [f['word'] for f in cap['fields']]==list(range(1024))
    assert all(f['constant']==0 for f in cap['fields'][70:])
    assert cap['fields'][21]['name']=='flux' and cap['fields'][21]['unit'].startswith('Q20/s')
    assert a['budget']['extraFrozenHostMetadataMaxBytes']>=7*(64+18)*4
    assert sum(p['words'] for p in a['rootTokenSha']['preimage'])==730
    assert a['rootTokenSha']['bytes']==2920
    assert a['rootCursor64']['prefixByteStart']+5*64*4==22016
    assert cap['prefixByteStart']+4096==30848
    # Finite encoding examples, independent of the product writer/reader.
    words=list(range(730)); b=struct.pack('<730I',*words); h=sha(b)
    words[2]^=1
    assert sha(struct.pack('<730I',*words))!=h
    assert struct.unpack('<8I',bytes.fromhex(h))==tuple(int.from_bytes(bytes.fromhex(h)[i:i+4],'little') for i in range(0,32,4))
    p=read('docs/architecture-T527-v5-cold-manifest-v1.json')
    w=read('docs/architecture-T552-v5-segment-wire-v2.json')
    assert w['parent']==receipt('docs/architecture-T527-v5-cold-manifest-v1.json')
    assert p['publicIdentityContract']['path']=='docs/architecture-T619-v5-public-identity-v1.json'
    assert p['publicIdentityContract']['sha256']==receipt(p['publicIdentityContract']['path'])['sha256']
    assert p['prefixSegments'][-1]['byteEnd']==32768
    assert len(p['rootRecord128'])==128 and p['rootRecord128'][58]=='rootTokenSha0'
    arms=[]
    for arm in ['c0','c1','c2']:
        f=f'.codex/local-test/t496/gltrace-t496v4c-{arm}.ndjson'
        rows=[json.loads(s) for s in (ROOT/f).read_text(encoding='utf-8-sig').splitlines() if s.strip()]
        enters={}; exits={}
        for r in rows:
            phase=r.get('phase',r.get('event'))
            # Preserve schema's identity and its actual callId key.
            if phase not in ('ENTER','EXIT'): continue
            key=tuple(r.get(k) for k in ['runId','arm','createAttemptId','pid','callId'])
            (enters if phase=='ENTER' else exits)[key]=r
        opened=[v for k,v in enters.items() if k not in exits]
        assert len(opened)==1 and opened[0]['functionId']==411
        arms.append(dict(arm=arm,enters=len(enters),exits=len(exits),openCall=opened[0],raw=receipt(f),
                         stages=receipt(f'.codex/local-test/t496/t496-stages_t496v4c-{arm}.ndjson')))
    typed=[]
    for f in sorted((ROOT/'test/reports/t491/t590a').glob('gpu-input-*.json')):
        x=json.loads(f.read_text(encoding='utf-8-sig')); s=x.get('snapshot',x.get('selected'))
        if not s: continue
        s=json.loads(s) if isinstance(s,str) else s
        if 'elementLength' not in s: continue
        assert s['elementLength']==4*s['expectedN']
        typed.append(dict(file=receipt(f.relative_to(ROOT).as_posix()), identity={k:s[k] for k in ['api','route','bootStamp','diagGeneration','attemptId']},
                          rawLength=s['elementLength'],expectedN=s['expectedN'],expectedBytes=s['expectedBytes'],
                          kind=s['elementKind'],offset=s['byteOffset'],firstFailedStage=s['firstFailedStage']))
    assert len(typed)==4
    ext=read('test/reports/t491/t591a/external.json')
    assert 'M0' in ext['process']['note'] and 'libgrsm0.so' in str(ext['timeline'])
    compiled=[]
    cc=read('.codex/local-test/t564-stage-tree/entry/.cxx/default/default/debug/arm64-v8a/compile_commands.json')
    for q in cc:
        if 'g9spf' in q.get('file','') and 'kernel_assets' in q['file']:
            compiled.append(dict(file=q['file'],directory=q['directory'],argvLength=len(q['command']),argvSHA=sha(q['command'].encode())))
    assert len(compiled)==1
    mathspec=read('docs/architecture-T607-g13-math-v1.json'); mathsha=mathspec.pop('mathSHA')
    assert mathsha==sha(canon(mathspec))
    e=read('docs/architecture-T607-g13-engineering-v1.json')
    assert e['hashes']['mathSHA']==mathsha
    assert e['T628Supplement']==receipt('docs/architecture-T628-math-domain-v1.json')
    assert mathspec['domainContract']['firstFault']['seal']==0xc0de1311
    # Explicit no-layout/no-gate/no-resource-growth checks for supplemental semantics.
    design=read('docs/design-G13-pure-fluid-manifest-v1.json')
    assert e['gates']==design['gates'] and e['support']==design['support']
    assert e['hashes']['algorithmSHA']==sha(canon(dict(designMD=receipt('docs/algorithm-G13-pure-fluid-gpu-v1.md')['sha256'],
          designManifest=receipt('docs/design-G13-pure-fluid-manifest-v1.json')['sha256'],mathSHA=mathsha)))
    a=math.pi**2/(64*120)
    f06={str(n):.25*math.exp(-a*n*n)/(math.pi*a*n*n) for n in [40,128]}
    expv=math.exp(-103); tiny=2**-149
    return dict(schema='T619_T628_ARCHITECTURE_AUDIT_V1',date='2026-11-19',
        deviceUsed=False,productTestsRun=False,buildRun=False,
        cursorEncodingChecks=True,tokenPreimageBytes=2920,byteMutationChangesToken=True,
        t620=arms,t621=typed,t622=dict(process=ext['process'],timeline=ext['timeline'],linkedOriginal=ext['linkedOriginal']),
        t624=compiled,t628=dict(F06EarliestTimeTailBounds=f06,F07ContinuousFlux=str(Fraction(1,32)/(12*Fraction(1,64))),
        expMinus103=expv,nearestF32=tiny,relativeError=abs(tiny/expv-1),gate=2**-20,
        scalarChecksOnly=True,mathSHA=mathsha))

INPUTS={
619:['docs/developer-T596-progress.md','test/reports/t596/development-t614.json','entry/src/main/cpp/gpu_resident/m3b/cold_capture.cpp',
     'entry/src/main/cpp/gpu_resident/m3b/completion_gate.h','entry/src/main/cpp/gpu_resident/m3b/transport.h','entry/src/main/cpp/gpu_resident/m3b/transport.cpp',
     'entry/src/main/cpp/gpu_resident/m3b/controller.cpp','entry/src/main/cpp/gpu_resident/m3b/root_owner.h'],
620:['docs/test-report-T496-create-only-v5.md','test/t496/external_source_diff.cpp','test/t496/t561_seams.h','test/t496/gl_sink.inc','test/t496/gl_forward_sink.cpp',
     '.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/app_bridge.cpp'],
621:['docs/test-report-T590-m3-g9-firstreject-v1.md','.codex/local-test/t582-g9-final/entry/src/main/cpp/gpu_resident/app_bridge.cpp'],
622:['docs/test-report-T591-external-receipt-v1.md','test/reports/t491/t591a/external.json',
     '.codex/local-test/t562-m2-fix/entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h','.codex/local-test/t562-m2-fix/entry/src/main/cpp/gpu_resident/app_bridge.cpp'],
623:['docs/test-report-T593-typed-source-v1.md','test/reports/t491/t593a/gpu-snapshot-65439-1795008307876-execution.json',
     '.codex/local-test/t597-fixed/entry/src/main/ets/pages/GpuSandWaterExperience.ets',
     '.codex/local-test/t597-fixed/entry/src/main/ets/physics/GpuM1DemoController.ets'],
624:['docs/test-report-T541-differential-toolchain-v2.md','.codex/local-test/t564-stage-tree/build.log',
     '.codex/local-test/t564-stage-tree/entry/.cxx/default/default/debug/arm64-v8a/compile_commands.json',
     '.codex/local-test/t564-stage-tree/entry/src/main/cpp/gpu_resident/g9spf/kernel_assets.cpp'],
625:['docs/test-report-T592-prepare-hold-v1.md','test/reports/t491/t592a/prepare-failed-layout.json',
     '.codex/local-test/t598-final/entry/src/main/ets/render/PrepareReadiness.ets',
     '.codex/local-test/t598-final/entry/src/main/ets/render/PrepareMainWindow.ets',
     '.codex/local-test/t598-final/entry/src/main/ets/pages/P051DerivedV2Demo.ets',
     '.codex/local-test/t598-final/entry/src/main/ets/render/PrepareFrameObservation.ets'],
626:['docs/test-report-T615-platform-clock-v1.md','test/reports/t615/platform-evidence.json',
     'entry/src/main/cpp/gpu_resident/g9spf/clock_producer.h'],
628:['docs/test-report-T606-independent-baseline-v1.md','docs/developer-T608-progress.md',
     'docs/algorithm-G13-pure-fluid-gpu-v1.md','docs/design-G13-pure-fluid-fixtures-v1.json',
     'entry/src/main/cpp/gpu_resident/g13flow/math_soft128.glsl','entry/src/main/cpp/gpu_resident/g13flow/cold.cpp',
     'entry/src/main/cpp/gpu_resident/g13flow/trace.glsl','entry/src/main/cpp/gpu_resident/g13flow/bridge.cpp',
     'entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp']}
def main():
    audit=checks()
    path='docs/analysis-T619-T628-contract-audit-v1.json'
    if '--record' in sys.argv:
        emit(path,audit)
        for n,paths in INPUTS.items():
            emit(f'docs/architecture-T{n}-evidence-v1.json',dict(schema=f'T{n}_ARCHITECTURE_SOURCE_RECEIPTS_V1',
                 date='2026-11-19',deviceUsed=False,productTestsRun=False,inputs=[receipt(p) for p in paths]))
    else:
        assert read(path)==audit,'audit changed; inspect original receipts before updating'
        for n in INPUTS:
            for r in read(f'docs/architecture-T{n}-evidence-v1.json')['inputs']:
                assert r==receipt(r['path']),f'input changed after attribution: {r["path"]}'
    print('PASS: architecture schema/joins/receipt checks only; 9 cards, no product execution')
if __name__=='__main__': main()
