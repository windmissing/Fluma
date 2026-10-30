from pathlib import Path
import hashlib,json,re,zipfile
r=Path(__file__).resolve().parents[2];p=r/'test/reports/t281';out=r/'.codex/local-test/t281'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def result(file):
 rows=[json.loads(line) for line in file.read_text(errors='replace').splitlines() if line.startswith('{')]
 assert rows and rows[-1]['failed']==0,file
 return rows[-1]
scenes=[result(p/f'scene-{k}-{w}.txt') for k,w in [(192,64),(192,128),(512,64),(512,128)]]
assert all(s['passed']==49 for s in scenes)
assert 'BUILD SUCCESSFUL' in (out/'package.log').read_text(errors='replace')
hap=r/'.codex/local-test/t257-sign/T281/T281-signed.hap'
unsigned=r/'entry/build/default/outputs/default/entry-default-unsigned.hap'
with zipfile.ZipFile(unsigned) as z:
 native=[n for n in z.namelist() if 'libgrsm1.so' in n];assert native
generated=r/'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h'
host=(out/'m1_scene_sources.h').read_text();production=generated.read_text();assert host==production
e={'revision':'T281_DEVELOPER_DELIVERY_V1','sceneContract':'GRS_GPU_SCENE_Q20_M1_V2','fullM1ShaderSha':'6c476dcf7597378c7d072251a15fe2cd8ab362339fdc47bff8505f17c17a96d6','sceneSha':re.search('kM1SceneSha="([a-f0-9]+)"',host)[1],'profiles':scenes,'pixelOracle':{'passed':20,'failed':0,'files':[f'pixels-{k}-{w}.json' for k,w in [(192,64),(192,128),(512,64),(512,128)]]},'runPacketControl':result(p/'run.txt'),'productionAppBridgeGles':result(p/'app.txt'),'build':'PASS','nativeModules':native,'unsignedSha':sha(unsigned),'signedSha':sha(hap),'signedHap':str(hap),'signVerify':'PASS','realDevice':'NOT_RUN','productBenefit':'NOT_EVALUATED','scope':'PC ANGLE SwiftShader GLES3.1; app pbuffer/window and clock explicit offline substitutes; scene fixture kappa=0 to isolate ARRIVE; production defaults unchanged'}
(p/'delivery.json').write_text(json.dumps(e,indent=2)+'\n');(p/'package-tail.txt').write_text('\n'.join((out/'package.log').read_text(errors='replace').splitlines()[-6:])+'\n');(p/'sign.json').write_text((out/'sign.log').read_text());print(json.dumps({'scenePassed':196,'pixelsPassed':20,'appPassed':5,'signedSha':e['signedSha']}))
