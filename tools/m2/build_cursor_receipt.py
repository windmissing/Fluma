# T663 static artifact inspection only; no product loading or tests.
from pathlib import Path
import hashlib,json,struct,zipfile,sys,shutil,re
ROOT=Path(__file__).resolve().parents[2]
def sha(b):return hashlib.sha256(b).hexdigest()
def elf(b):
 h=struct.unpack_from('<16sHHIQQQIHHHHHH',b);assert h[0][:6]==b'\x7fELF\x02\x01' and h[2]==183
 sections=[struct.unpack_from('<IIQQQQIIQQ',b,h[6]+i*h[11]) for i in range(h[12])];abi=None;buildId=None
 for s in sections:
  if s[1] not in (2,11):continue
  t=sections[s[6]];names=b[t[4]:t[4]+t[5]]
  for off in range(s[4],s[4]+s[5],s[9]):
   n,info,other,index,value,size=struct.unpack_from('<IBBHQQ',b,off)
   if names[n:names.index(0,n)]==b'GrsM2CreateApiCursorAbi':
    q=sections[index];abi=list(struct.unpack_from('<%dQ'%(size//8),b,q[4]+value-q[3]))
 for i in range(h[10]):
  ph=struct.unpack_from('<IIQQQQQQ',b,h[5]+i*h[9])
  if ph[0]!=4:continue
  at,end=ph[2],ph[2]+ph[5]
  while at<end:
   ns,ds,kind=struct.unpack_from('<III',b,at);at+=12;name=b[at:at+ns];at+=(ns+3)&~3;data=b[at:at+ds];at+=(ds+3)&~3
   if kind==3 and name==b'GNU\0':buildId=data.hex()
 assert abi and abi==[1,128,3,384,22512,67584,40,22384,32,96] and buildId
 return {'sha256':sha(b),'bytes':len(b),'buildId':buildId,'abi':abi}
def main():
 build=Path(sys.argv[1]).resolve();output=Path(sys.argv[2]).resolve();output.mkdir(parents=True,exist_ok=True)
 manifest=json.loads((build/'t663-sources.json').read_text(encoding='utf8'))
 for p,h in manifest['files'].items():assert sha((build/p).read_bytes())==h,p
 own=['entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h','entry/src/main/cpp/gpu_resident/m2/create_diagnostics_abi.cpp']
 for p in own:assert (build/p).read_bytes()==(ROOT/p).read_bytes()
 hap=build/'entry/build/default/outputs/default/entry-default-unsigned.hap'
 with zipfile.ZipFile(hap) as z:
  module=elf(z.read('libs/arm64-v8a/libgrsm1.so'));abc=z.read('ets/modules.abc')
 record={'schema':'T663_COMPILE_DELIVERY_V1','deviceUsed':False,'testsRun':False,'productExecuted':False,'signed':False,'deployed':False,'productPASS':False,'status':'DEVELOPMENT_COMPILE_COMPLETE','buildDirectory':str(build),'unsignedHap':{'path':str(hap),'bytes':hap.stat().st_size,'sha256':sha(hap.read_bytes())},'packagedELF':module,'abc':{'bytes':len(abc),'sha256':sha(abc)},'store':{'previousCompiledBytes':67200,'actualBytes':67584,'mutexBytes':40,'cursorBytes':128,'slots':3,'addedPayload':384,'actualStoreIncrease':384,'externalFallbackTriggered':False},'actualCompiledOwnedSources':{p:sha((ROOT/p).read_bytes()) for p in own},'sourceManifestSHA':sha((build/'t663-sources.json').read_bytes()),'mainRevision':'T462_M2_CREATE_DIAGNOSTIC_V1 unchanged','newField':'optional apiCursor revision1; u64 decimal strings; raw unknown -1','newGLQueries':0,'newWaits':0,'newThreads':0,'stageEventsFinish':'preserved; cursor does not authorize success','unmeasured':'Actual GL API returnedness/creation outcomes require T591 same-new-attempt device evidence'}
 for n,f in [('full-build.log','.codex/local-test/t663-build.log'),('final-build.log','.codex/local-test/t663-final-build.log')]:
  data=(ROOT/f).read_bytes();assert b'BUILD SUCCESSFUL' in data;shutil.copyfile(ROOT/f,output/n);record[n]={'sha256':sha(data),'elapsed':re.search(r'BUILD SUCCESSFUL in ([^\r\n]+)',data.decode('utf-8-sig',errors='replace')).group(1).strip()}
 shutil.copyfile(build/'t663-sources.json',output/'generated-sources.json');shutil.copyfile(ROOT/'docs/architecture-T662-contract-v1.json',output/'formal-contract.json')
 (output/'compile-receipt.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8');print(json.dumps(record,ensure_ascii=False,indent=2))
if __name__=='__main__':main()
