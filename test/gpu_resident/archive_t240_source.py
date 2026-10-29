from pathlib import Path
import json,hashlib,gzip
root=Path(__file__).resolve().parents[2];out=root/'test/reports/t240';src=root/'entry/src/main/cpp/gpu_resident'
sha=lambda b:hashlib.sha256(b).hexdigest()
tests=[]
for k,kw in [(192,64),(192,128),(512,64),(512,128)]:
 p=out/f'transport-{k}-{kw}.txt';s=p.read_text(encoding='utf-8-sig');row=json.loads(s.strip().splitlines()[-1]);assert row['passed']==562 and row['failed']==0;tests.append({'file':str(p.relative_to(root)).replace('\\','/'),'sha256':sha(p.read_bytes()),'result':row})
core=out/'source-host-v5.txt';row=json.loads(core.read_text(encoding='utf-8-sig').strip().splitlines()[-1]);assert row['passed']==20 and row['failed']==0
log=out/'source-transport-package.log';assert 'BUILD SUCCESSFUL' in log.read_text(encoding='utf-8-sig');(out/'source-transport-package.log.gz').write_bytes(gzip.compress(log.read_bytes(),mtime=0))
files=['m1/source_core.comp','m1/source_seal.glsl','m1/source_audit.glsl','m1/source_owner.cpp','m1/source_owner.h','m1/source_transport.cpp','m1/source_transport.h','m1/source_kernel.cpp','m1/profile.cpp','m1/layout_generated.h']
actual=(root/'.codex/local-test/t240-source/source_core.comp').read_bytes()
result={'revision':'T240_SOURCE_CORE_CLOSED_V4','scope':'SOURCE_ONLY_GPU_CORE_NATIVE_FENCE_OWNER_PUBLICATION_LEASE','fullM1Quantum':'NOT_IMPLEMENTED','productionDefaultRoute':'OFF','sceneM1':'NOT_IMPLEMENTED_SEPARATE_CARD','hostExecution':'PC_SWIFTSHADER_NOT_OHOS_DEVICE','profiles':tests,'core':{'file':str(core.relative_to(root)).replace('\\','/'),'sha256':sha(core.read_bytes()),'result':row},'kernelSha256':sha(actual),'sourceFiles':[{'file':f,'sha256':sha((src/f).read_bytes())} for f in files],'auditMask':1023,'passes':8,'barriersPerRoot':9,'mappedBytesPerNonTombstonedRoot':1024,'lateTombstonedRootMappedBytes':0,'sourceOnlyWorkspace':'explicit standalone source layout, no claim of full M1 resources/optics/quantum allocation','build':'BUILD_SUCCESSFUL','device':'NOT_RUN'}
(out/'source-closure-v4.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'profiles':len(tests),'transportAssertions':sum(t['result']['passed'] for t in tests),'core':row['passed'],'build':'PASS'}))
