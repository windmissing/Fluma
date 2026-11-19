# T653 build-only receipt. Reads ELF/HAP bytes; never loads the product.
from pathlib import Path
import hashlib,json,struct,zipfile,subprocess,sys,shutil,re
ROOT=Path(__file__).resolve().parents[2]
def sha(b):return hashlib.sha256(b).hexdigest()
def elf(b):
 h=struct.unpack_from('<16sHHIQQQIHHHHHH',b);assert h[0][:6]==b'\x7fELF\x02\x01' and h[2]==183
 ss=[struct.unpack_from('<IIQQQQIIQQ',b,h[6]+i*h[11]) for i in range(h[12])];abi=None;bid=None
 for s in ss:
  if s[1] not in (2,11):continue
  t=ss[s[6]];names=b[t[4]:t[4]+t[5]]
  for off in range(s[4],s[4]+s[5],s[9]):
   n,info,other,index,value,size=struct.unpack_from('<IBBHQQ',b,off)
   if names[n:names.index(0,n)]==b'GrsM5InitAbi':
    q=ss[index];abi=list(struct.unpack_from('<%dQ'%(size//8),b,q[4]+value-q[3]))
 for i in range(h[10]):
  ph=struct.unpack_from('<IIQQQQQQ',b,h[5]+i*h[9])
  if ph[0]!=4:continue
  at,end=ph[2],ph[2]+ph[5]
  while at<end:
   ns,ds,kind=struct.unpack_from('<III',b,at);at+=12;name=b[at:at+ns];at+=(ns+3)&~3;data=b[at:at+ds];at+=(ds+3)&~3
   if kind==3 and name==b'GNU\0':bid=data.hex()
 assert abi and abi[:5]==[1,16384,256,512,2048] and bid
 return {'sha256':sha(b),'bytes':len(b),'buildId':bid,'abiWords':abi}
def receipt(build,log,output):
 subprocess.run([sys.executable,str(ROOT/'tools/v5/verify_init_manifest.py'),str(build)],check=True)
 assert 'BUILD SUCCESSFUL' in log.read_text(encoding='utf-8-sig',errors='replace')
 lib=build/'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrsm1.so';hap=build/'entry/build/default/outputs/default/entry-default-unsigned.hap'
 with zipfile.ZipFile(hap) as z:
  abiEntry=next(n for n in z.namelist() if n.endswith('modules.abc'));abc=z.read(abiEntry);libEntry=next(n for n in z.namelist() if n.endswith('/libgrsm1.so'));packed=elf(z.read(libEntry))
 debug=elf(lib.read_bytes());assert debug['buildId']==packed['buildId'] and debug['abiWords']==packed['abiWords']
 manifest=json.loads((build/'m5-init-source-manifest.json').read_text(encoding='utf8'))
 names=subprocess.check_output(['git','diff','--name-only','--','entry/src/main/cpp/gpu_resident/m3b','entry/src/main/cpp/gpu_resident/program_cache','entry/src/main/ets/pages/GpuResidentM5.ets'],cwd=ROOT).decode().splitlines()
 names+=['entry/src/main/cpp/gpu_resident/m3b/init_diagnostics.h','entry/src/main/cpp/gpu_resident/m3b/init_diagnostics.cpp','entry/src/main/cpp/gpu_resident/m3b/init_program_watch.h','entry/src/main/ets/physics/GpuM5Init.ets']
 for n in names:assert sha((ROOT/n).read_bytes())==sha((build/n).read_bytes()),n
 data={'schema':'T653_COMPILE_DELIVERY_V1','result':'COMPILE_PASS_NOT_PRODUCT_PASS','deviceUsed':False,'testsRun':False,'productExecuted':False,'signed':False,'deployed':False,'elapsed':re.search(r'BUILD SUCCESSFUL in ([^\r\n]+)',log.read_text(encoding='utf-8-sig',errors='replace')).group(1).strip(),'packagedELF':{'entry':libEntry,**packed},'unstrippedELF':{'path':str(lib.relative_to(ROOT)),**debug},'unsignedHap':{'path':str(hap.relative_to(ROOT)),'sha256':sha(hap.read_bytes()),'bytes':hap.stat().st_size},'abc':{'entry':abiEntry,'sha256':sha(abc),'bytes':len(abc)},'sourceManifestSHA':manifest['sourceManifestSHA'],'buildLogSHA':sha(log.read_bytes()),'actualCompiledOwnedSources':{n:sha((ROOT/n).read_bytes()) for n in sorted(set(names))},'contracts':{n:sha((ROOT/n).read_bytes()) for n in ['docs/architecture-T650-contract-v1.json','docs/architecture-T619-v5-public-identity-v1.json','docs/architecture-T614-v5-entry-cursor-v1.json']},'nativeLoadedModuleSHA':'UNKNOWN, known flag off; build SHA does not substitute runtime evidence','FULL':'formal flag0/1 admission, actual existing T619 writer/reservation/capsule/reader preserved, runtime not executed'}
 output.mkdir(parents=True,exist_ok=True);(output/'compile-receipt.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8');shutil.copyfile(build/'m5-init-source-manifest.json',output/'generated-app-source-manifest.json');shutil.copyfile(log,output/'hvigor-build.log');return data
if __name__=='__main__':
 r=receipt(Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve(),Path(sys.argv[3]).resolve());print(json.dumps({k:r[k] for k in ['result','packagedELF','abc','sourceManifestSHA','unsignedHap']},indent=2))
