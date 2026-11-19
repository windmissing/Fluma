"""Immutable input fixtures from existing formal seed/config documents; no stepper."""
from pathlib import Path
import argparse,hashlib,json,struct,sys,re
ROOT=Path(__file__).resolve().parents[2]
def raw(words):return struct.pack('<%dI'%len(words),*words)
def sha(words):return hashlib.sha256(raw(words)).hexdigest()
def make(kind,k,angle,full,out,roots=1,foreign=False,parentCredit=False):
 out=Path(out);out.mkdir(parents=True,exist_ok=False)
 seedDoc=json.loads((ROOT/'docs/architecture-T552-g9-seed-manifest-v2.json').read_text(encoding='utf8'));dm=json.loads((ROOT/'docs/architecture-T554-g9-dm-manifest-v2.json').read_text(encoding='utf8'))
 profile=next(p for p in dm['parameterProfiles'] if p['K']==k)
 state=[0]*((542720+256*k)//4);a=seedDoc['seed']['canonicalAssignments']
 for i,v in a['header'].items():state[int(i)]=v
 state[24]=k
 for c in range(128):
  for i,v in a['columnEvery32Words'].items():state[512+c*32+int(i)]=v
  for i,v in enumerate(a['bedRank0Every64Cohort4']):state[4608+c*256+i]=v
 for i,v in enumerate(a['sourceRow0']):state[(280576+256*k)//4+i]=v
 if kind=='dm':
  ass=profile['seedAssignments'];state[:3]=[ass['headerMagic'],ass['headerRevision'],ass['headerProfile']];state[106:114]=ass['header106_113'];config=profile['config64'];expected=profile['seedTemplateSha256'];param=sha(config)
  assert sha(state)==expected,'FORMAL_DM_TEMPLATE_SHA'
  schema=ROOT/'entry/src/main/cpp/gpu_resident/g9dm/schema_generated.h';magic=843662407;code=843924551;revision=config[11]
 else:
  # Frozen SPF fixture protocol is the T564 legacy16 research path, not RX64.
  code=826691667;config=[k,536870912,32768,24576,262144,262144,65536,576,1,1,1,int(full),0,0,0,0];param=sha([code,*config[:9],config[9],config[10]])
  # ParameterHash order is profile,K,g,muStart,muStop,band,entrain,shear,settle,revision,research,synthetic.
  assert len([code,*config[:9],config[9],config[10]])==12
  if k==512:assert sha(state)==seedDoc['seed']['templateSha256'],'FORMAL_SPF_TEMPLATE_SHA'
  schema=ROOT/'.codex/local-test/t670-spf/entry/src/main/cpp/gpu_resident/g9spf/schema_generated.h';magic=826882375;revision=1
 layout=re.search(r'kEngineeringSha="([0-9a-f]{64})"',schema.read_text(encoding='utf8')).group(1)
 identity=[7,0,1,0,11,0,2,0,1,0,1,0,1,0,1,0,1,0];command=[0]*192
 command[:3]=[magic,2 if kind=='dm' else 1,code]
 command[4:6]=[7,0];command[6:8]=[1,0];command[8:10]=[11,0];command[10:12]=[1,0];command[12:14]=[2,0];command[24]=angle&0xffffffff;command[25]=revision;command[26:34]=struct.unpack('<8I',bytes.fromhex(param))
 for at in (128,130,132,134,136,141):command[at]=1
 command[138:141]=[2,1,0];command[144:152]=struct.unpack('<8I',bytes.fromhex(layout));quota=[860963661,3,256,0xc0de0003]+[0]*252
 commands=[]
 for ordinal in range(1,roots+1):
  row=list(command)
  for at in (10,128,132,134):row[at]=ordinal
  row[139]=ordinal
  if foreign:row[2]=826691667 if kind=='dm' else 843924551
  commands+=row
 values={'config.u32':config,'identity.u32':identity,'seed.u32':state,'commands.u32':commands,'quota.u32':quota}
 if kind=='dm':values['settings.u32']=[int(full)]
 if parentCredit:
  resource=next(r for r in dm['resources'] if r['K']==k);credit=resource['FULL']['bytes']//4-2097152;values['cold-credit.u32']=[credit,0]
 receipts={}
 for name,words in values.items():(out/name).write_bytes(raw(words));receipts[name]={'sha256':sha(words),'bytes':len(words)*4}
 (out/'fixture.json').write_text(json.dumps({'schema':'T670_EXISTING_FORMAL_SEED_V1','family':kind,'K':k,'angleIndex':angle,'full':full,'roots':roots,'foreignPolicy':foreign,'parentCredit':parentCredit,'files':receipts,'seedOrigin':'T552 canonical corrected S40; DM header per T554','expected':'Independent GPU capture required; no generated output/physics PASS','quotaOrigin':'caller-declared raw fixture only, NOT production clock authorization'},indent=2)+'\n')
 return receipts
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('kind',choices=['spf','dm']);p.add_argument('K',type=int,choices=[192,512]);p.add_argument('angle',type=int,choices=[0,3840,7680]);p.add_argument('out');p.add_argument('--full',action='store_true');p.add_argument('--roots',type=int,choices=[1,2,3,4],default=1);p.add_argument('--foreign-policy',action='store_true');p.add_argument('--parent-credit',action='store_true');a=p.parse_args();print(json.dumps(make(a.kind,a.K,a.angle,a.full,a.out,a.roots,a.foreign_policy,a.parent_credit)))
