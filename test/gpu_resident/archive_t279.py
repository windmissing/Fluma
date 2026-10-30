"""Archive exact candidate source, actual allocation/dispatch and build evidence."""
from pathlib import Path
import json,hashlib,gzip,struct
r=Path(__file__).resolve().parents[2];src=r/'entry/src/main/cpp/gpu_resident';out=r/'test/reports/t279'
sha=lambda b:hashlib.sha256(b).hexdigest()
profiles=[]
for k,w in [(192,64),(192,128),(512,64),(512,128)]:
    row=json.loads((out/f'quantum-{k}-{w}.json').read_text())['profiles'][0]
    assert row['passed']==38 and row['failed']==0
    text=(out/f'gpu-{k}-{w}.txt').read_text();assert 'SOURCE_GPU_HANDOFF_PREFIX_PIN_UNCHANGED_OWNER=PASS' in text
    row['handoffAssertions']='SOURCE_PREFIX_SUCCESS_AND_FAILURE_BANK_ROLLBACK_NEW_FRAME_OWNER_UNCHANGED'
    row['inputSha256']=sha((r/f'.codex/local-test/t279/cases-{k}-{w}.bin').read_bytes())
    row['outputSha256']=sha((r/f'.codex/local-test/t279/output-{k}-{w}.bin').read_bytes())
    row['logSha256']=sha(text.encode());profiles.append(row)
log=(r/'.codex/local-test/t279/package.log').read_bytes()
assert b'BUILD SUCCESSFUL' in log
(out/'package.log.gz').write_bytes(gzip.compress(log,mtime=0))
hap=r/'entry/build/default/outputs/default/entry-default-unsigned.hap';assert hap.exists()
files=['CMakeLists.txt','m1/source_transport.cpp','m1/source_transport.h','m1/quantum_candidate.h','m1/quantum_candidate.cpp','m1/quantum_kernel.cpp','m1/quantum_core.comp','m1/quantum_t16.glsl','m1/quantum_motion.glsl','m1/quantum_stages.glsl','m1/quantum_event_masks.glsl']
tests=['test/gpu_resident/prepare_t279_quantum.py','test/gpu_resident/build_t279_host.cmd','test/gpu_resident/t279_quantum_host.cpp','test/gpu_resident/check_t279_quantum.py']
regression=json.loads((out/'source-regression.txt').read_text().strip().splitlines()[-1]);assert regression['passed']==562 and regression['failed']==0
result={'revision':'T279_QUANTUM_CANDIDATE_V1','date':'2026-10-30','scope':'CANDIDATE_BANK_EVENT64_ONLY','kernelSha256':sha((r/'.codex/local-test/t279/quantum_core.comp').read_bytes()),'hostExecution':'PC_ANGLE_SWIFTSHADER_REAL_GLES_DISPATCH_NOT_OHOS_DEVICE','build':'BUILD_SUCCESSFUL','hapSha256':sha(hap.read_bytes()),'unsignedHapBytes':hap.stat().st_size,'profiles':profiles,'sourceRegression':regression,'sources':[{'path':'entry/src/main/cpp/gpu_resident/'+f,'sha256':sha((src/f).read_bytes())} for f in files]+[{'path':f,'sha256':sha((r/f).read_bytes())} for f in tests],'candidateDispatchFormula':'27+L(nextPow2(K))+2*L(Kw); one barrier per dispatch; final pass restores candidate from immutable base on failure','mappedStatusBytesPerPollAfterFence':64,'coldCapture':'state/event output only under explicit Freeze; host fixtures excluded from HAP','allocationScope':'candidate+workspace+one frame event arena only; excludes separate SOURCE transport, driver/JIT/host/RSS, T280 audit/replay/double arenas/publication and T281 scene','nativeQuantumAcceptance':'T280_NOT_IMPLEMENTED','sceneCurrent':'T281_NOT_IMPLEMENTED','defaultRoute':'OFF','device':'NOT_RUN'}
result['kernelSha256']=sha((r/'.codex/local-test/t279/quantum_core.comp').read_text().encode()) # same LF policy as CMake/GLSL
(out/'delivery.json').write_text(json.dumps(result,indent=2));print(json.dumps({'passed':sum(p['passed'] for p in profiles),'sourceRegressionPassed':regression['passed'],'kernelSha256':result['kernelSha256'],'hapSha256':result['hapSha256']}))
