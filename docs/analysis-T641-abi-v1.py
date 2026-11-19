"""Compile only the isolated engineering ABI specimen, never product sources."""
from pathlib import Path
import subprocess,json,hashlib,re
R=Path(__file__).resolve().parents[1]
compiler=Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/llvm/bin/clang++.exe')
sysroot=compiler.parents[2]/'sysroot'
header=R/'docs/architecture-T641-protocol-layout-v1.hpp'
argv=[str(compiler),'--target=aarch64-linux-ohos','--sysroot='+sysroot.as_posix(),'-D__MUSL__','-std=c++17','-x','c++','-fsyntax-only',
 '-Xclang','-fdump-record-layouts-simple',str(header)]
r=subprocess.run(argv,cwd=R,capture_output=True,timeout=60)
raw=r.stdout+r.stderr
log=R/'docs/analysis-T641-abi-aarch64-v1.log';log.write_bytes(raw)
def fp(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return dict(path=str(p),bytes=p.stat().st_size,sha256=h.hexdigest())
text=raw.decode('utf-8',errors='replace');layouts={}
for name in ['RxProtocolPayload','RxTypedStorage','RxSyncState']:
 found=re.search(r'Type: struct t641_abi::'+name+r'\s+Layout: <ASTRecordLayout\s+Size:(\d+)\s+DataSize:\d+\s+Alignment:(\d+)\s+FieldOffsets: \[([^\]]*)\]>',text)
 if found:layouts[name]=dict(sizeofBytes=int(found[1])//8,alignofBytes=int(found[2])//8,fieldByteOffsets=[int(s)//8 for s in found[3].split(',') if s.strip()])
assert r.returncode==0,text[-3000:]
assert layouts['RxProtocolPayload']['sizeofBytes']==40960 and layouts['RxTypedStorage']['sizeofBytes']==40960
assert layouts['RxTypedStorage']['fieldByteOffsets']==[0,20480,21504,22528,23552,24576,34304]
assert layouts['RxSyncState']['sizeofBytes']<=256
receipt=dict(schema='T641_AARCH64_ENGINEERING_ABI_V1',target='aarch64-linux-ohos',argv=argv,exitCode=r.returncode,
 compiler=fp(compiler),specimen=fp(header),log=fp(log),layouts=layouts,
 productBuildsRun=False,productTestsRun=False,deviceUsed=False,programExecuted=False,
 scope='syntax-only architecture ABI specimen, no product sources or GPU/physics execution')
(R/'docs/analysis-T641-abi-v1.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(exitCode=r.returncode,layouts=layouts,scope=receipt['scope']),ensure_ascii=False))
