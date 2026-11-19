"""Offline-only signed artifact handoff. Does not invoke hdc or any device API."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, sys, zipfile
ROOT=Path(__file__).resolve().parents[2]
READELF=Path('C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/native/llvm/bin/llvm-readelf.exe')
def sha(b): return hashlib.sha256(b).hexdigest()
def file(p):
 p=Path(p);return {'path':str(p.resolve()),'bytes':p.stat().st_size,'sha256':sha(p.read_bytes())}
def build_id(data):
 # GNU build-id note: little endian n_namesz=4, type=3.
 for m in re.finditer(b'GNU\x00',data):
  at=m.start()-12
  if at>=0 and int.from_bytes(data[at:at+4],'little')==4 and int.from_bytes(data[at+8:at+12],'little')==3:
   n=int.from_bytes(data[at+4:at+8],'little')
   if 0<n<=64:return data[m.end():m.end()+n].hex()
 return None
def collect(task, project, expected=None):
 project=Path(project).resolve();assert project.is_relative_to(ROOT/'.codex/local-test')
 unsigned=list((project/'entry/build/default/outputs').rglob('*unsigned.hap'));assert len(unsigned)==1
 if expected:assert sha(unsigned[0].read_bytes())==expected,'UNSIGNED_SOURCE_IDENTITY_MISMATCH'
 receipt=ROOT/'test/reports/t257'/f'{task}.json'
 if not receipt.exists():subprocess.run([sys.executable,str(ROOT/'test/t257/sign.py'),task,str(project)],cwd=ROOT,check=True,capture_output=True)
 signed=json.loads(receipt.read_text());assert signed['verifyApp']=='PASS' and signed['input']['sha256']==sha(unsigned[0].read_bytes())
 signed_path=Path(signed['signed']['path']);assert file(signed_path)['sha256']==signed['signed']['sha256']
 with zipfile.ZipFile(signed_path) as z:
  entries={n:{'bytes':len(z.read(n)),'sha256':sha(z.read(n)),**({'buildId':build_id(z.read(n))} if n.endswith('.so') else {})} for n in z.namelist() if n.endswith(('.so','.abc','module.json'))}
 source={str(p.relative_to(project)):file(p) for base in ['entry/src/main','tools','docs'] if (project/base).exists() for p in (project/base).rglob('*') if p.is_file()}
 generated={};commands=[]
 cache=project/'entry/.cxx'
 if cache.exists():
  for p in cache.rglob('*'):
   if p.is_file() and (p.suffix in ('.h','.inc','.o','.obj','.rsp','.ninja') or p.name=='compile_commands.json'):
    generated[str(p.relative_to(project))]=file(p)
    if p.suffix in ('.rsp','.ninja') or p.name=='compile_commands.json':commands.append({'file':str(p.relative_to(project)),'rawUTF8':p.read_text(encoding='utf8',errors='replace')})
 out=ROOT/'test/reports'/task.lower();out.mkdir(parents=True,exist_ok=True)
 data={'schema':'OFFLINE_HANDOFF_V1','task':task,'device':'NOT_RUN','productPASS':False,'unsigned':file(unsigned[0]),'signed':file(signed_path),'signingReceipt':str(receipt.relative_to(ROOT)),'entries':entries,'source':source,'generatedAndObjects':generated,'nativeCommands':commands,'commandInterpretation':'raw native CMake/Ninja argv/rsp retained; no shlex or flags deduplication','runtimeLoadedModule':'UNKNOWN; package ELF is not runtime maps evidence'}
 (out/'handoff.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
 print(json.dumps({'task':task,'signed':data['signed'],'entries':entries,'sources':len(source),'objectsAndGenerated':len(generated)},ensure_ascii=False))
 return data
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('task');p.add_argument('project');p.add_argument('--unsigned-sha');a=p.parse_args();collect(a.task,a.project,a.unsigned_sha)
