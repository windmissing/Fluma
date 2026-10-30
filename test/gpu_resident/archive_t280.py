from pathlib import Path
import json,hashlib,gzip,re
r=Path(__file__).resolve().parents[2];out=r/'test/reports/t280';sha=lambda b:hashlib.sha256(b).hexdigest();profiles=[]
for k,w in [(192,64),(192,128),(512,64),(512,128)]:
 p=out/f'full-{k}-{w}.txt';text=p.read_text();row=json.loads(text.strip().splitlines()[-1]);assert row['passed']==465 and row['failed']==0
 row['steadyAllocatedBufferBytes']=int(re.search(r'bytes=(\d+)',text)[1]);row['trace']=[json.loads(x[6:]) for x in text.splitlines() if x.startswith('TRACE ')]
 assert len(row['trace'])==5 and all(len(x['passes'])==len(x['groups']) for x in row['trace'])
 row['dispatches']=len(row['trace'][0]['passes']);row['barriers']=row['dispatches'];row['fullLogSha256']=sha(p.read_bytes());rp=out/f'replay-check-{k}-{w}.txt';assert 'FULL_REPLAY_CASES 38 PASS' in rp.read_text();row['independentReplayCases']=38;row['replayLogSha256']=sha(rp.read_bytes());profiles.append(row)
source=out/'source-regression.txt';assert json.loads(source.read_text().strip().splitlines()[-1])['passed']==562
log=(r/'.codex/local-test/t280/package.log').read_bytes();assert b'BUILD SUCCESSFUL' in log;(out/'package.log.gz').write_bytes(gzip.compress(log,mtime=0))
h=r/'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/full_m1_kernels.h';host=r/'.codex/local-test/t280/full_m1_kernels.h';assert h.read_text()==host.read_text()
signature=json.loads((r/'test/reports/t257/T280.json').read_text())
files=[p for p in (r/'entry/src/main/cpp/gpu_resident/m1').glob('*') if p.name.startswith(('full_','quantum_'))]+[r/'entry/src/main/cpp/gpu_resident'/x for x in ['CMakeLists.txt','trace.h']]
result={'revision':'T280_FULL_M1_COMMIT_V1','date':'2026-10-30','scope':'SOURCE_AND_QUANTUM_ACCEPTED_PUBLICATION_LEASE','defaultProductRoute':'OFF','device':'NOT_RUN','scene':'T281_NOT_IMPLEMENTED','execution':'PC_ANGLE_SWIFTSHADER_REAL_GLES','fullShaderSha256':re.search(r'kM1FullSha="([0-9a-f]+)"',host.read_text())[1],'sourceRegressionPassed':562,'nativeAssertions':sum(x['passed'] for x in profiles),'independentReplayCases':152,'profiles':profiles,'build':'BUILD_SUCCESSFUL','signature':signature,'ordinaryCompletionMappedBytes':1024,'cancelledLateMappedBytes':0,'quantumTailFences':1,'gpuPhysicsPoolReadback':'NONE_ON_HOT_PATH','allocationScope':'steady glBufferData payload including separate SOURCE, candidate, expanded WORK/spare, double frame arenas, publications/tails; excludes driver/JIT/RSS/cold resize overlap/T281 scene','files':[{'path':str(p.relative_to(r)).replace('\\','/'),'sha256':sha(p.read_bytes())} for p in files]}
(out/'delivery.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:result[k] for k in ['fullShaderSha256','nativeAssertions','independentReplayCases','build']}))
