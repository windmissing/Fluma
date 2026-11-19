#!/usr/bin/env python3
# T648 default DM2 construction from frozen correct integer physics + T647.
from pathlib import Path
import argparse,hashlib,json,struct
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'entry/src/main/cpp/gpu_resident/g9dm'
def sha(b):return hashlib.sha256(b).hexdigest()
def source_bytes(p):return p.read_bytes().replace(b"\r\n",b"\n")
def generate(out,canonical=False):
 out.mkdir(parents=True,exist_ok=True);frozen=MOD/'frozen_t647';receipt=json.loads((frozen/'receipt.json').read_text(encoding='utf8'));deps={}
 for name,expected in receipt['sha256'].items():
  raw=(frozen/name).read_bytes()
  if sha(raw)!=expected:raise ValueError('FROZEN_DM_REFERENCE_CHANGED '+name)
  deps['frozen/'+name]=expected
 man=json.loads((ROOT/'docs/architecture-T554-g9-dm-manifest-v2.json').read_text(encoding='utf8'));acceptance=json.loads((ROOT/'docs/architecture-T647-acceptance-v1.json').read_text(encoding='utf8'))
 for n in ['architecture-T554-g9-dm-manifest-v2.json','architecture-T554-g9-dm-dispatch-v2.json','architecture-T647-acceptance-v1.json','architecture-T552-g9-seed-manifest-v2.json']:
  deps['docs/'+n]=sha((ROOT/'docs'/n).read_bytes())
 s=(frozen/'corrected_reference.comp').read_text(encoding='utf8');old=(frozen/'parent_input.comp').read_text(encoding='utf8')
 s=s.replace('G9_PROFILE=827477575u','G9_PROFILE=843924551u').replace('G9_INPUT_MAGIC=1481788999u','G9_INPUT_MAGIC=843662407u').replace('W[uConfig+76u+f]','W[uConfig+112u+f]')
 a=s.index('bool g9FlickVerify(){');b=s.index('bool g9FlowTerms(',a);oa=old.index('bool g9FlickVerify(){');ob=old.index('bool g9FlowTerms(',oa);s=s[:a]+old[oa:ob]+s[b:]
 s=s.replace('g9Input(1u)!=3u','g9Input(1u)!=2u').replace('g9Input(1u)==3u','g9Input(1u)==2u')
 s=s.replace('// T644/T647: genuine DM48 implementation, private to the GNR1 variant.','// T648/T647: default DM2 genuine DM48 implementation.')
 (out/'pipeline.comp').write_text(s,encoding='utf8')
 schema=(frozen/'schema_parent.h').read_text(encoding='utf8');schema=schema.replace('t555-g9dm-v2-initial-construction','T648_T647_DM2_IMPLEMENTATION_1')
 # Exact formal table rows remain default-DM, never the composite counts.
 schema=schema.replace('kDM48ByteStart=1286784,kDM48Bytes=24576', 'kDM48ByteStart192=1286784,kDM48ByteStart512=1289344,kDM48Bytes=24576')
 schema=schema.replace('namespace dm48 {', 'inline constexpr size_t DM48ByteStart(const Profile& p){return p.workBytes-kDM48Bytes;}\nnamespace dm48 {')
 extension='namespace audit647 {\ninline constexpr uint32_t kRevision=1,kDefaultSourcePasses=12,kDefaultQuantumPasses=2404,kSourceMask=65535,kQuantumMask=262143;\ninline constexpr size_t kFullConfigWord=64,kDmParametersWord=112,kColdControlsWord=1408,kColdControlWords=128;\n'
 for field in acceptance['DMControlWords']:extension+='inline constexpr size_t '+field['name']+'='+str(field['word'])+';\n'
 extension+='}\n'
 schema+='\nnamespace grs::g9dm {\n'+extension+'}\n'

 (out/'schema_generated.h').write_text(schema,encoding='utf8')
 constants='#pragma once\n#include <array>\n#include <cstdint>\nnamespace grs::g9dm {\n'
 for profile in man['parameterProfiles']:
  k=profile['K'];config=profile['config64'];raw=struct.pack('<64I',*config)
  if sha(raw)!=profile['parameterSha256']:raise ValueError('DM_CONFIG_SHA')
  constants+=f'inline constexpr std::array<uint32_t,64> kConfig{k}={{{",".join(map(str,config))}}};\n'
 constants+='inline constexpr const char* kStaticContractSHA="'+deps['docs/architecture-T647-acceptance-v1.json']+'";\n'
 constants+='inline constexpr const char* kPhysicalSpecSHA="b17060c0441e481ec99f3279c76c8b8c94a1013b0fd9045d6b01b6cb827dac77";\n'
 constants+='inline constexpr const char* kPhysicalLayoutSHA="'+sha(json.dumps({'resources':man['resources'],'acceptance':acceptance},sort_keys=True,separators=(',',':')).encode())+'";\n'
 for name in ['completion_gate.h','completion_gate.cpp']:
  deps['g9spf/'+name]=sha(source_bytes(MOD.parent/'g9spf'/name))
 for p in sorted((MOD.parent/'acceptance_t647').glob('*')):
  if p.is_file():deps['acceptance_t647/'+p.name]=sha(source_bytes(p))
 for p in sorted(Path(__file__).parent.rglob('*')):
  if p.is_file() and '__pycache__' not in p.parts and p.name not in ['generate_assets.py','identity.json','acceptance_dictionary.json']:deps['tools/g9dm/'+p.relative_to(Path(__file__).parent).as_posix()]=sha(source_bytes(p))
 implementation=hashlib.sha256(source_bytes(Path(__file__)))
 for p in sorted(MOD.glob('*')):
  if p.is_file() and p.name not in ['pipeline.comp','schema_generated.h','dm_contract_generated.h','dm_seed_generated.h']:implementation.update(p.name.encode()+b'\0'+source_bytes(p))
 implementation.update(json.dumps(deps,sort_keys=True).encode());identity=implementation.hexdigest();constants+='inline constexpr const char* kImplementationSHA="'+identity+'";\n}\n'
 (out/'dm_contract_generated.h').write_text(constants,encoding='utf8')
 # Keep legacy cold Scope's two exact64-character source components safe.
 schema=schema.replace('T648_T647_DM2_IMPLEMENTATION_1',identity);(out/'schema_generated.h').write_text(schema,encoding='utf8')
 assignments=json.loads((ROOT/'docs/architecture-T552-g9-seed-manifest-v2.json').read_text(encoding='utf8'))['seed']['canonicalAssignments'];seed='#pragma once\n#include "dm_contract_generated.h"\n#include <algorithm>\nnamespace grs::g9dm { inline bool FillDmTemplate(uint32_t* w,size_t n,uint32_t k){if((k!=192&&k!=512)||n!=(542720+256*k)/4)return false;std::fill(w,w+n,0u);'
 for at,value in assignments['header'].items():seed+=f'w[{at}]={value}u;'
 seed+='w[0]=844317767u;w[1]=2u;w[2]=843924551u;w[24]=k;const auto& config=k==192?kConfig192:kConfig512;for(size_t i=0;i<8;++i)w[106+i]=config[48+i];for(size_t c=0;c<128;++c){'
 for at,value in assignments['columnEvery32Words'].items():seed+=f'w[512+32*c+{at}]={value}u;'
 for i,value in enumerate(assignments['bedRank0Every64Cohort4']):seed+=f'w[4608+256*c+{i}]={value}u;'
 seed+='}'
 for i,value in enumerate(assignments['sourceRow0']):seed+=f'w[(280576+256*k)/4+{i}]={value}u;'
 # Verify the literal canonical LE seed constructor against the formal template SHA.
 for profile in man['parameterProfiles']:
  k=profile['K'];w=[0]*((542720+256*k)//4)
  for at,value in assignments['header'].items():w[int(at)]=value
  w[0:3]=[844317767,2,843924551];w[24]=k;w[106:114]=profile['config64'][48:56]
  for c in range(128):
   for at,value in assignments['columnEvery32Words'].items():w[512+32*c+int(at)]=value
   for i,value in enumerate(assignments['bedRank0Every64Cohort4']):w[4608+256*c+i]=value
  at=(280576+256*k)//4;w[at:at+len(assignments['sourceRow0'])]=assignments['sourceRow0']
  if sha(struct.pack('<%dI'%len(w),*w))!=profile['seedTemplateSha256']:raise ValueError('DM_SEED_TEMPLATE_SHA '+str(k))
 seed+='return true;} }\n';(out/'dm_seed_generated.h').write_text(seed,encoding='utf8')
 # Generate the shared immutable row table from the one formal machine source.
 table='#pragma once\nnamespace grs::accept647 {\ninline constexpr Row kRows[]={\n'
 for row in acceptance['rows']:
  keys=['rootKind','physicalProfileCode','tailProfileCode','actualPasses','requiredAudits','completionMagic','tailRevision','seal']
  table+='{Family::'+row['family']+','+','.join(str(row[k])+'u' for k in keys)+'},\n'
 table+='};\ninline constexpr const char* kContractSHA="'+deps['docs/architecture-T647-acceptance-v1.json']+'";\n}\n'
 if table.encode()!=(MOD.parent/'acceptance_t647/table_generated.h').read_bytes().replace(b'\r\n',b'\n'):raise ValueError('SHARED_CONTRACT_TABLE_STALE')
 if canonical:
  for name in ['pipeline.comp','schema_generated.h','dm_contract_generated.h','dm_seed_generated.h']:
   if (out/name).resolve()!=(MOD/name).resolve():(MOD/name).write_bytes((out/name).read_bytes())
 (out/'acceptance_dictionary.json').write_text(json.dumps({'schema':'T648_READER_DICTIONARY_V1','staticContractSHA':deps['docs/architecture-T647-acceptance-v1.json'],'expectedWords':224,'expectedBytes':896,'encoding':'u32 little endian','rows':acceptance['rows'],'FrozenExpected224':acceptance['FrozenExpected224'],'DMControlWords':acceptance['DMControlWords'],'scopeBits':acceptance['scopeBits'],'DMWork':acceptance['DMWork']},indent=2)+'\n',encoding='utf8')
 (out/'identity.json').write_text(json.dumps({'schema':'T648_DM2_IDENTITY_V1','implementationSHA':identity,'shaderSHA':sha(s.encode()),'staticContractSHA':deps['docs/architecture-T647-acceptance-v1.json'],'inputs':deps},indent=2)+'\n',encoding='utf8')
 if canonical:
  for name in ['identity.json','acceptance_dictionary.json']:(Path(__file__).parent/name).write_bytes((out/name).read_bytes())
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=MOD);p.add_argument('--canonical',action='store_true');a=p.parse_args();generate(a.out,a.canonical)
