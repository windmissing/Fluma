"""Read bounded Windows minidump identity/exception/thread streams; no symbols guessed."""
import hashlib,json,struct,sys
from pathlib import Path
def read(path):
 p=Path(path);b=p.read_bytes();assert len(b)<=64*1024*1024 and b[:4]==b'MDMP'
 u32=lambda at:struct.unpack_from('<I',b,at)[0]
 u64=lambda at:struct.unpack_from('<Q',b,at)[0]
 count=u32(8);directory=u32(12);assert count<256
 streams={}
 for i in range(count):
  kind,size,rva=struct.unpack_from('<III',b,directory+12*i);assert rva+size<=len(b);streams[kind]=(size,rva)
 result={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'symbolicStacks':'UNKNOWN; raw stack memory/context retained in dump','device':'NOT_RUN'}
 modules=[]
 if 4 in streams:
  _,at=streams[4];n=u32(at);assert n<4096
  for i in range(n):
   off=at+4+108*i;name=u32(off+20);length=u32(name);assert length<32768
   modules.append({'base':u64(off),'size':u32(off+8),'path':b[name+4:name+4+length].decode('utf-16-le')})
 result['modules']=modules
 if 6 in streams:
  _,at=streams[6];address=u64(at+24)
  result['exception']={'threadId':u32(at),'code':hex(u32(at+8)),'address':hex(address),'module':next((m for m in modules if m['base']<=address<m['base']+m['size']),None)}
 if 3 in streams:
  _,at=streams[3];n=u32(at);assert n<4096;threads=[]
  for i in range(n):
   off=at+4+48*i;size=u32(off+32);rva=u32(off+36);assert rva+size<=len(b)
   threads.append({'threadId':u32(off),'stackStart':hex(u64(off+24)),'rawStackBytes':size,'rawStackSHA':hashlib.sha256(b[rva:rva+size]).hexdigest(),'contextBytes':u32(off+40)})
  result['threads']=threads
 return result
if __name__=='__main__':print(json.dumps(read(sys.argv[1]),indent=2))
