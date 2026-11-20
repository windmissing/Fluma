#!/usr/bin/env python3
"""T648 DM2 cold archive reader; validates captured bytes, never creates a live proof."""
from pathlib import Path
import argparse,hashlib,json,struct
ROOT=Path(__file__).resolve().parents[2]
def digest(words):return hashlib.sha256(struct.pack('<%dI'%len(words),*words)).digest()
def u64(w,n):return w[n]|w[n+1]<<32
def source_arm(manifest_path, receipt_path, w, t):
 raw=(ROOT/'docs/architecture-T736-source-contract-v1.json').read_bytes()
 contract=json.loads(raw)
 manifest=json.loads(manifest_path.read_bytes())
 roles=manifest.pop('roleSetSHA')
 if roles!=hashlib.sha256(json.dumps(manifest,sort_keys=True,separators=(',',':')).encode()).hexdigest():
  raise ValueError('MIXED_SOURCE_ROLE_SET')
 expected=contract['families']['DM']['sourceRole']
 if (manifest['schema'],manifest['family'],manifest['backend'],manifest['variant'],manifest['supportsRoots'],
     manifest['t736ContractSHA'],manifest['source'],manifest['reflection'],manifest['passTable'])!=(
        'G9_BOOT_SOURCE_V1','DM','GLES','BOOT_SOURCE',['SOURCE'],hashlib.sha256(raw).hexdigest(),
        {**expected,'opcodes':list(range(12))},contract['reflection'],contract['passTable']):
  raise ValueError('UNTRUSTED_T736_SOURCE_MANIFEST')
 rows=[json.loads(line) for line in receipt_path.read_bytes().splitlines() if line.strip()]
 root,=[row for row in rows if row.get('stage')=='SOURCE_ROOT']
 configure,=[row for row in rows if row.get('stage')=='CONFIGURE_ENTER']
 if (configure['roleSetSHA']!=roles or root['sourceSHA']!=expected['sha256'] or t[5]!=1 or
     struct.pack('<8I',*t[80:88]).hex()!=expected['sha256'] or
     root['inputSHA']!=digest(w[512:640]).hex() or
     root['seedSHA']!=struct.pack('<8I',*w[688:696]).hex() or
     not root['committed'] or not root['typedAccepted'] or root['workReadBytes']!=1024 or
     root['fenceWaitRaw'] not in (0x911a,0x911c) or
     not 0<int(root['rootBeginNs'])<=int(root['fenceNs'])<=int(root['commitNs'])<=int(root['deadlineNs'])):
  raise ValueError('UNBOUND_SOURCE_ROOT')
 for field, at in [('scope',16),('context',18),('surface',20),('session',22),('inputEpoch',24),('nonce',26)]:
  if int(root[field])!=u64(w,at):raise ValueError('SOURCE_COLD_SCOPE_MISMATCH_'+field)
 if int(root['commit'])!=u64(t,30) or int(root['quanta'])!=u64(t,54):
  raise ValueError('SOURCE_COLD_COMMIT_MISMATCH')
 return roles
def read(path,source_manifest=None,source_receipt=None):
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
 if bool(source_manifest)!=bool(source_receipt):raise ValueError('SOURCE_MANIFEST_AND_RECEIPT_REQUIRED')
 identity=json.loads((ROOT/'tools/g9dm/identity.json').read_text(encoding='utf8'))
 result={'schema':'T648_DM2_COLD_READER_V1','kind':kind,'outcome':t[6],'K':k,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'capturedArchiveIntegrity':'VALID','liveProofCreated':False,'productPass':False}
 if source_manifest:
  result['sourceRoleSetSHA']=source_arm(source_manifest,source_receipt,w,t)
 else:
  if struct.pack('<8I',*t[80:88]).hex()!=identity['shaderSHA']:raise ValueError('MIXED_SHADER_BUILD')
  result['implementationSHA']=identity['implementationSHA']
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('archive',type=Path);p.add_argument('--source-manifest',type=Path);p.add_argument('--source-receipt',type=Path);a=p.parse_args();print(json.dumps(read(a.archive,a.source_manifest,a.source_receipt),indent=2))
