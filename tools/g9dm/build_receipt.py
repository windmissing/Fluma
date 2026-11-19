#!/usr/bin/env python3
"""Static ELF/HAP compile receipt. Does not load the native library or execute tests."""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def elf(path):
 b=path.read_bytes();h=struct.unpack_from('<16sHHIQQQIHHHHHH',b)
 assert h[0][:6]==b'\x7fELF\x02\x01' and h[2]==183
 secs=[struct.unpack_from('<IIQQQQIIQQ',b,h[6]+i*h[11]) for i in range(h[12])];abi=None;build_id=None
 for sec in secs:
  if sec[1] not in (2,11):continue
  st=secs[sec[6]];names=b[st[4]:st[4]+st[5]]
  for at in range(sec[4],sec[4]+sec[5],sec[9]):
   name,info,other,index,value,size=struct.unpack_from('<IBBHQQ',b,at)
   name=names[name:names.index(0,name)]
   if name==b'grs_dm_t647_proof_abi':
    s=secs[index];offset=s[4]+value-s[3];abi=list(struct.unpack_from('<%dQ'%(size//8),b,offset))
 for i in range(h[10]):
  ph=struct.unpack_from('<IIQQQQQQ',b,h[5]+i*h[9])
  if ph[0]!=4:continue
  at,end=ph[2],ph[2]+ph[5]
  while at<end:
   ns,ds,kind=struct.unpack_from('<III',b,at);at+=12;name=b[at:at+ns];at+=(ns+3)&~3;data=b[at:at+ds];at+=(ds+3)&~3
   if kind==3 and name==b'GNU\0':build_id=data.hex()
 assert abi and abi[:3]==[1,224,896] and build_id
 return {'machine':'AARCH64','buildId':build_id,'proofAbiWords':abi}
def receipt(build,log,out):
 assert 'BUILD SUCCESSFUL' in log.read_text(encoding='utf-8-sig',errors='replace')
 lib=build/'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrs_dm_t647.so';hap=build/'entry/build/default/outputs/default/entry-default-unsigned.hap'
 def file(p):return {'path':str(p.relative_to(ROOT)).replace('\\','/'),'bytes':p.stat().st_size,'sha256':sha(p)}
 data={'schema':'T648_DEVELOPER_COMPILE_RECEIPT_V1','result':'COMPILE_PASS_NOT_PRODUCT_PASS','deviceUsed':False,'testsRun':False,'productExecuted':False,'signed':False,'deployed':False,'scope':'actual default DM library, native shared proof core, additive R1 composite gates; unsigned HAP compile container, not a registered research UI/product deployment','elf':{**file(lib),**elf(lib)},'unsignedHap':file(hap),'buildLog':file(log),'construction':json.loads((ROOT/'tools/g9dm/identity.json').read_text(encoding='utf8')),'sources':{},'generated':{}}
 for folder in ['entry/src/main/cpp/gpu_resident/g9dm','entry/src/main/cpp/gpu_resident/acceptance_t647','tools/g9dm']:
  for p in sorted((ROOT/folder).rglob('*')):
   if p.is_file() and '__pycache__' not in p.parts:data['sources'][str(p.relative_to(ROOT)).replace('\\','/')]=sha(p)
 for name in ['completion_gate.h','completion_gate.cpp']:
  p=ROOT/'entry/src/main/cpp/gpu_resident/g9spf'/name;data['sources'][str(p.relative_to(ROOT)).replace('\\','/')]=sha(p)
 assets=build/'entry/.cxx/default/default/debug/arm64-v8a/dm_native/dm_assets'
 for p in assets.iterdir():
  if p.is_file():data['generated'][p.name]=file(p)
 # Preserve the original default R1 implementation body verbatim; composite code is additive.
 old=subprocess.check_output(['git','show','HEAD:entry/src/main/cpp/gpu_resident/g9spf/completion_gate.cpp'],cwd=ROOT)
 current=(ROOT/'entry/src/main/cpp/gpu_resident/g9spf/completion_gate.cpp').read_bytes()
 assert current.replace(b'\r\n',b'\n').startswith(old.replace(b'\r\n',b'\n'))
 data['R1DefaultImplementationPrefixUnchanged']=True
 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8');return data
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('build',type=Path);p.add_argument('log',type=Path);p.add_argument('out',type=Path);a=p.parse_args();r=receipt(a.build.resolve(),a.log.resolve(),a.out);print(json.dumps({'result':r['result'],'elf':r['elf'],'unsignedHap':r['unsignedHap']},indent=2))
