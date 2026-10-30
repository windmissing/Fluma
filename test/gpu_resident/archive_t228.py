from pathlib import Path
import hashlib,json,zipfile
r=Path(__file__).resolve().parents[2];p=r/'test/reports/t228';local=r/'.codex/local-test/t228'
def result(name):
 rows=[json.loads(s) for s in (p/name).read_text(errors='replace').splitlines() if s.startswith('{')];assert rows and rows[-1]['failed']==0,name
 return rows[-1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
app=result('app.txt');assert app['passed']==20
assert app['knownGpuBytesPeak']>=app['gpuBufferAllocatedBytes']+app['frameTargetBytes']
assert 'BUILD SUCCESSFUL' in (local/'package.log').read_text(errors='replace')
sign=json.loads((local/'sign.log').read_text());assert sign['sign']==sign['verify']=='PASS'
hap=r/'.codex/local-test/t257-sign/T228-final/T228-final-signed.hap';unsigned=r/'entry/build/default/outputs/default/entry-default-unsigned.hap'
assert sha(hap)==sign['signedSha'] and sha(unsigned)==sign['inputSha']
with zipfile.ZipFile(unsigned) as z:
 modules=[n for n in z.namelist() if 'libgrsm1.so' in n];assert modules
delivery={'revision':'T228_DEVELOPER_DELIVERY_V1','uiContract':'GRS_M1_DEMO_UI_V4','sceneContract':'GRS_GPU_SCENE_Q20_M1_V2','controller':result('controller.txt'),'native':result('native-units.txt'),'productionApp':app,'m0Bridge':result('m0-bridge.txt'),'ready':result('ready.txt'),'clock':result('clock.txt'),'build':'PASS','signVerify':sign,'signedHap':str(hap),'nativeModules':modules,'resourceScope':'Actual GL buffer allocations + dual full-frame textures; known GPU payload peak excludes driver allocation/JIT/RSS. Separate PC process working-set peak from GetProcessMemoryInfo; not device memory. JS fixed typed payload192B, objects/views/status-string heap not measured. Old/new native contexts retired serially; driver residency overlap not measured.','realDevice':'NOT_RUN','independentValidation':'SEPARATE_T229_T300_T301','productBenefit':'NOT_EVALUATED'}
(p/'delivery.json').write_text(json.dumps(delivery,indent=2)+'\n');(p/'package-tail.txt').write_text('\n'.join((local/'package.log').read_text(errors='replace').splitlines()[-6:])+'\n');(p/'sign.json').write_text(json.dumps(sign,indent=2)+'\n');print(json.dumps({'appPassed':app['passed'],'signedSha':sign['signedSha']}))
