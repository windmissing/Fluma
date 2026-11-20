#!/usr/bin/env python3
"""Compile-artifact receipt only; does not load/execute the product."""
from pathlib import Path
import argparse,hashlib,json,re,struct,subprocess,zipfile,difflib,sys
from verify_assets import verify
ROOT=Path(__file__).resolve().parents[2]
def sha(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  while chunk:=f.read(65536):h.update(chunk)
 return h.hexdigest()
def elf(path):
 b=path.read_bytes();h=struct.unpack_from('<16sHHIQQQIHHHHHH',b);assert h[0][:6]==b'\x7fELF\x02\x01' and h[2]==183
 sections=[struct.unpack_from('<IIQQQQIIQQ',b,h[6]+i*h[11]) for i in range(h[12])];abi=None;app=None;build_id=None
 for sec in sections:
  if sec[1] not in [2,11]:continue
  st=sections[sec[6]];strings=b[st[4]:st[4]+st[5]]
  for at in range(sec[4],sec[4]+sec[5],sec[9]):
   name,info,other,index,value,size=struct.unpack_from('<IBBHQQ',b,at);name=strings[name:strings.index(0,name)].decode('utf8',errors='replace')
   if name=='GrsG9RxAbi':
    section=sections[index];offset=section[4]+value-section[3];current=list(struct.unpack_from('<16I',b,offset));assert abi is None or abi==current;abi=current
   if 'Product' in name and name.endswith('E3app') and size>40960:app=size
 for i in range(h[10]):
  ph=struct.unpack_from('<IIQQQQQQ',b,h[5]+i*h[9]);
  if ph[0]!=4:continue
  at,end=ph[2],ph[2]+ph[5]
  while at<end:
   names,desc,kind=struct.unpack_from('<III',b,at);at+=12;name=b[at:at+names];at+=(names+3)&~3;data=b[at:at+desc];at+=(desc+3)&~3
   if kind==3 and name==b'GNU\0':build_id=data.hex()
 assert abi and abi[1:3]==[40960,64] and abi[3]<=256 and abi[4]==8 and abi[9:]==[0,20480,21504,22528,23552,24576,34304] and build_id and app
 return {'machine':'AARCH64','buildId':build_id,'abiWords':abi,'nativeAppStaticBytes':app,'nativeAppExcludingProtocolPayloadBytes':app-40960}
def receipt(build,log,out,validator=None):
 assert 'BUILD SUCCESSFUL' in log.read_text(encoding='utf-8-sig',errors='replace')
 lib=build/'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrs_g9rx.so';hap=build/'entry/build/default/outputs/default/entry-default-unsigned.hap';assets=build/'entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/g9rx/rx_assets';compile_commands=build/'entry/.cxx/default/default/debug/arm64-v8a/compile_commands.json'
 data={'schema':'T644_DEVELOPER_COMPILE_RECEIPT_V1','task':'T644','deviceUsed':False,'testsRun':False,'productExecuted':False,'signed':False,'deployed':False,'result':'COMPILE_PASS_NOT_PRODUCT_PASS','elf':{'path':str(lib.relative_to(ROOT)),'bytes':lib.stat().st_size,'sha256':sha(lib),**elf(lib)},'unsignedHap':{'path':str(hap.relative_to(ROOT)),'bytes':hap.stat().st_size,'sha256':sha(hap)},'buildLogSHA':sha(log),'compileCommandsSHA':sha(compile_commands),'pipelineIdentity':json.loads((assets/'pipeline_identity.json').read_text()),'seedConstructor':json.loads((assets/'seed_constructor_receipt.json').read_text()),'generated':{},'sources':{}}
 for name in ['schema_generated.h','contract_generated.h','implementation_generated.h','pipeline_source.h','pipeline.comp','reader_schema.json','inputs.json','seed_generated.h','scene_generated.h','cold_generated.h']:
  data['generated'][name]={'bytes':(assets/name).stat().st_size,'sha256':sha(assets/name)}
 for directory in ['entry/src/main/cpp/gpu_resident/g9rx','tools/g9rx','entry/src/main/cpp/types/libgrs_g9rx']:
  for path in sorted((ROOT/directory).rglob('*')):
   if path.is_file() and '__pycache__' not in path.parts and 'generated' not in path.parts:data['sources'][str(path.relative_to(ROOT)).replace('\\','/')]=sha(path)
 for path in list((ROOT/'entry/src/main/ets').rglob('*G9Rx*.ets')):data['sources'][str(path.relative_to(ROOT)).replace('\\','/')]=sha(path)
 # T674: the actual final production bytes, including the packaged ELF, are authoritative.
 identity=verify(assets);raw=(assets/'pipeline.comp').read_bytes();assert identity['variant']=='PRODUCTION'
 def extract(binary):
  matches=[]
  for marker in re.finditer(b'#version 310 es',binary):
   end=binary.find(b'\0',marker.start());candidate=binary[marker.start():end]
   if b'bool g9Capture(){' in candidate:matches.append((marker.start(),candidate))
  assert matches and all(value==raw for _,value in matches),'RX_LINKED_SOURCE_IDENTITY_MISMATCH'
  return {'offsets':[at for at,_ in matches],'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'lines':raw.count(b'\n')}
 data['linkedCanonicalSource']=extract(lib.read_bytes())
 with zipfile.ZipFile(hap) as z:
  packaged=z.read('libs/arm64-v8a/libgrs_g9rx.so');data['packagedELF']={'bytes':len(packaged),'sha256':hashlib.sha256(packaged).hexdigest(),'canonicalSource':extract(packaged)}
 data['schema']='T699_DEVELOPER_COMPILE_RECEIPT_V1';data['task']='T699'
 data['captureLineMap']=json.loads((assets/'capture_line_map.json').read_text(encoding='utf8'))
 for name in ['pipeline_identity.json','capture_line_map.json','rx_dependencies.cmake']:
  data['generated'][name]={'bytes':(assets/name).stat().st_size,'sha256':sha(assets/name)}
 # Keep complete compile/link invocation sources, not merely a command digest.
 evidence=out.parent;evidence.mkdir(parents=True,exist_ok=True)
 for name,path in [('compile_commands.json',compile_commands),('build.ninja',compile_commands.parent/'build.ninja'),('rules.ninja',compile_commands.parent/'CMakeFiles/rules.ninja')]:
  (evidence/name).write_bytes(path.read_bytes());data[name]={'sha256':sha(path),'bytes':path.stat().st_size}
 for name in ['pipeline.comp','pipeline_source.h','capture_line_map.json','pipeline_identity.json','reader_schema.json','inputs.json']:(evidence/name).write_bytes((assets/name).read_bytes())
 if validator is None:raise ValueError('FINAL_PRODUCTION_STATIC_COMPILER_REQUIRED')
 tool=validator.resolve();version=subprocess.run([str(tool),'--version'],capture_output=True);command=[str(tool),'-S','comp',str((assets/'pipeline.comp').resolve())];compiled=subprocess.run(command,capture_output=True)
 static={'tool':str(tool),'toolSHA256':sha(tool),'versionCommand':[str(tool),'--version'],'versionExit':version.returncode,'versionStdout':version.stdout.decode('utf8',errors='replace'),'versionStderr':version.stderr.decode('utf8',errors='replace'),'command':command,'macros':[],'target':'GLSL ES 3.10 from #version 310 es; syntax validation; no SPIR-V target','exit':compiled.returncode,'sourceSHA256':hashlib.sha256(raw).hexdigest()}
 (evidence/'glsl-static.log').write_bytes(version.stdout+version.stderr+b'\nCOMMAND '+json.dumps(command).encode()+b'\n'+compiled.stdout+compiled.stderr+b'\nEXIT '+str(compiled.returncode).encode()+b'\n')
 data['staticCompiler']=static;assert version.returncode==0 and compiled.returncode==0,compiled.stdout+compiled.stderr
 for relative,digest in json.loads((assets/'inputs.json').read_text(encoding='utf8')).items():
  path=ROOT/relative if relative.startswith(('entry/','docs/','tools/')) else ROOT/'entry/src/main/cpp/gpu_resident/g9rx'/relative
  actual=build/path.relative_to(ROOT);assert actual.read_bytes()==path.read_bytes(),str(path)
 hooks=build/'rx-hooks-assets';subprocess.run([sys.executable,str(ROOT/'tools/g9rx/generate_assets.py'),'--out',str(hooks),'--test-hooks'],check=True)
 hooks_identity=verify(hooks);assert hooks_identity['variant']=='HARNESS_NOT_PRODUCT' and hooks_identity['shaderSHA']!=identity['shaderSHA']
 (evidence/'harness-pipeline-identity.json').write_bytes((hooks/'pipeline_identity.json').read_bytes())
 (evidence/'production-harness.diff').write_text(''.join(difflib.unified_diff(raw.decode().splitlines(True),(hooks/'pipeline.comp').read_text(encoding='utf8').splitlines(True),fromfile='PRODUCTION',tofile='FRESH_HARNESS_NOT_PRODUCT')),encoding='utf8',newline='\n')
 data['harnessDifference']={'productionSHA':identity['shaderSHA'],'harnessSHA':hooks_identity['shaderSHA'],'diffSHA':sha(evidence/'production-harness.diff'),'harnessExecuted':False}

 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8');return data
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('build',type=Path);p.add_argument('log',type=Path);p.add_argument('out',type=Path);p.add_argument('--validator',type=Path,required=True);a=p.parse_args();r=receipt(a.build.resolve(),a.log.resolve(),a.out,a.validator);print(json.dumps({'result':r['result'],'elf':r['elf'],'unsignedHap':r['unsignedHap']},indent=2))
