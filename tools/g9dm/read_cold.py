#!/usr/bin/env python3
"""T648 DM2 cold archive reader; validates captured bytes, never creates a live proof."""
from pathlib import Path
import argparse,hashlib,json,struct
ROOT=Path(__file__).resolve().parents[2]
def digest(words):return hashlib.sha256(struct.pack('<%dI'%len(words),*words)).digest()
def u64(w,n):return w[n]|w[n+1]<<32
def read(path):
 raw=path.read_bytes()
 if len(raw)%4:raise ValueError('UNALIGNED_COLD')
 w=struct.unpack('<%dI'%(len(raw)//4),raw)
 if len(w)<8192 or w[:5]!=(844383303,2,8192,3235776802,843924551):raise ValueError('OLD_OR_INVALID_DM_READER_IDENTITY')
 k=w[7];full=w[5]
 if k not in (192,512) or full not in (0,1) or w[6]!=(10 if full else 4) or u64(w,8)!=len(raw):raise ValueError('COLD_LAYOUT')
 manifest=json.loads((ROOT/'docs/architecture-T554-g9-dm-manifest-v2.json').read_text(encoding='utf8'))
 config=w[704:768];profile=next(p for p in manifest['parameterProfiles'] if p['K']==k)
 if list(config)!=profile['config64'] or digest(config).hex()!=profile['parameterSha256']:raise ValueError('CONFIG64_SHA')
 t=w[256:512];kind=t[5];count=12 if kind==1 else 2404 if kind==2 else 0;mask=65535 if kind==1 else 262143
 if t[:5]!=(843269191,2,256,3235776802,843924551) or t[39:43]!=(count,count,count,mask) or t[6] not in (1,2,3,4):raise ValueError('UNTRUSTED_DM_COMPLETION')
 if any(t[156:]) or t[52] or t[53] or t[63]:raise ValueError('TAIL_RESERVED')
 if t[43]!=t[44] or t[45]!=t[46]:raise ValueError('UNSEALED_CURSOR')
 if tuple(w[656:664])!=tuple(t[64:72]) or digest(config)!=struct.pack('<8I',*t[64:72]):raise ValueError('PARAMETER_IDENTITY')
 for tailAt,configAt in [(72,664),(80,672),(88,680)]:
  if tuple(t[tailAt:tailAt+8])!=tuple(w[configAt:configAt+8]):raise ValueError('TAIL_HASH_IDENTITY')
 if digest(w[512:640])!=struct.pack('<8I',*t[88:96]):raise ValueError('INPUT_SHA')
 controls=w[1408:1536]
 if controls[127]!=1:raise ValueError('MISSING_REAL_SCOPE_PROTOCOL')
 if kind==2:
  if t[6] not in (1,4):raise ValueError('QUANTUM_OUTCOME')
  j=t[37]
  if t[6]==4:expected=63|(63<<7)
  else:
   if j not in (1,2,4,8,16,32):raise ValueError('SELECTED_J')
   kbit=j.bit_length()-1;expected=((1<<(kbit+1))-1)|64|8192|(((1<<kbit)-1)<<7)
  if any(controls[at]!=expected for at in (120,121,122,123)):raise ValueError('DM_SCOPE_CERTIFICATE')
 end=8192
 for i in range(w[6]):
  d=w[2048+32*i:2080+32*i];begin=u64(d,8);size=u64(d,10)
  if d[0]!=i+1 or d[12]!=1 or d[31]!=3235776802 or begin!=end or begin+size>len(w):raise ValueError('SEGMENT_LAYOUT')
  if digest(w[begin:begin+size])!=struct.pack('<8I',*d[23:31]):raise ValueError('SEGMENT_SHA')
  end+=size
 if end!=len(w):raise ValueError('TRAILING_COLD_BYTES')
 identity=json.loads((ROOT/'tools/g9dm/identity.json').read_text(encoding='utf8'))
 if struct.pack('<8I',*t[80:88]).hex()!=identity['shaderSHA']:raise ValueError('MIXED_SHADER_BUILD')
 return {'schema':'T648_DM2_COLD_READER_V1','kind':kind,'outcome':t[6],'K':k,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'capturedArchiveIntegrity':'VALID','liveProofCreated':False,'productPass':False,'implementationSHA':identity['implementationSHA']}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('archive',type=Path);a=p.parse_args();print(json.dumps(read(a.archive),indent=2))
