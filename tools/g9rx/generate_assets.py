#!/usr/bin/env python3
# T644 isolated GNR1 compiler inputs. No runtime or test execution.
from pathlib import Path
import json,hashlib,argparse,struct,sys
from capture_scope import convert, REVISION
from coherent_values import convert_values, REVISION as COHERENT_REVISION
from readonly_values import (convert_readonly_values, QUALIFIED_VALUES,
                             REVISION as READONLY_REVISION)
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'entry/src/main/cpp/gpu_resident/g9rx'
def sha(b):return hashlib.sha256(b).hexdigest()
def one(s,a,b):
 if s.count(a)!=1:raise ValueError('RX_ANCHOR '+a[:100])
 return s.replace(a,b)
def generate(out,test_hooks=False,boot_only=False):
 if boot_only and test_hooks:
  raise ValueError('BOOT_ONLY does not include test hooks')
 out.mkdir(parents=True,exist_ok=True);contract=json.loads((ROOT/'docs/architecture-T665-contract-v1.json').read_text(encoding='utf8'));receipt=json.loads((MOD/'frozen/source_receipt.json').read_text(encoding='utf8'));deps={}
 for n,h in receipt['sha256'].items():
  b=(MOD/'frozen'/n).read_bytes()
  if sha(b)!=h:raise ValueError('FROZEN_PARENT_CHANGED '+n)
  deps['frozen/'+n]=h
 for relative in ['entry/src/main/cpp/gpu_resident/napi_words_contract.h','entry/src/main/cpp/gpu_resident/napi_provider_profile.h','entry/src/main/cpp/gpu_resident/napi_provider_profile.cmake','entry/src/main/cpp/gpu_resident/angles_generated.h','entry/src/main/cpp/gpu_resident/cold_slot_broker.h','entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp']:
  deps[relative]=sha((ROOT/relative).read_bytes())
 tables={}
 for n in ['wire','runtime','storage-cold','seed-config']:
  p=ROOT/f'docs/architecture-T641-{n}-v1.json';raw=p.read_bytes();tables[n]=json.loads(raw);deps[str(p.relative_to(ROOT))]=sha(raw)
 for name in ['wire','runtime','storage-cold','seed-config']:
  descriptor=tables[name]['design'];rawDesign=(ROOT/descriptor['path']).read_bytes()
  if len(rawDesign)!=descriptor['bytes'] or sha(rawDesign)!=descriptor['sha256']:raise ValueError('DESIGN_IDENTITY_CHANGED '+descriptor['path'])
 for relative in ['docs/design-T637-manifest-v1.json','docs/architecture-T552-g9-seed-manifest-v2.json','docs/design-G9-dynamic-mobilization-manifest-v2.json','entry/src/main/ets/pages/GpuResidentG9Rx.ets','entry/src/main/ets/entryability/G9RxResearchAbility.ets','entry/src/main/ets/physics/G9RxColdArchiveCore.ets','entry/src/main/ets/physics/GpuG9RxCold.ets','entry/src/main/ets/workers/G9RxColdWorker.ets']:
  deps[relative]=sha((ROOT/relative).read_bytes())
 raw=(ROOT/'docs/architecture-T647-acceptance-v1.json').read_bytes();tables['acceptance']=json.loads(raw);deps['docs/architecture-T647-acceptance-v1.json']=sha(raw)
 if sha((MOD/'frozen/pipeline.comp').read_bytes())!=contract['transformation']['originalFrozenPipelineSHA']:raise ValueError('T665_FROZEN_PIPELINE_IDENTITY')
 s=(MOD/'frozen/pipeline.comp').read_text(encoding='utf8');dm=(MOD/'physical_dm.glsl').read_text(encoding='utf8');deps['physical_dm.glsl']=sha((MOD/'physical_dm.glsl').read_bytes())
 a=s.index('uint dmWord(');b=s.index('bool g9StartThreshold(',a);s=s[:a]+dm+'\n'+s[b:]
 s=one(s,'uniform uint uOp,uRole,uJslot,uMicro,uPass,uRequiredPasses,','uniform uint uRootKind,uRequiredAudits;\nuniform uint uOp,uRole,uJslot,uMicro,uPass,uRequiredPasses,')
 s=s.replace('uRequiredPasses==2024u','uRootKind==2u').replace('uRequiredPasses==12u','uRootKind==1u')
 s=s.replace('const uint G9_PROFILE=843924551u,','const uint G9_PROFILE=827477575u,').replace('const uint G9_INPUT_MAGIC=826882375u;','const uint G9_INPUT_MAGIC=1481788999u;')
 s=one(s,'const uint G9_COMPLETION_MAGIC=826489159u;','const uint G9_COMPLETION_MAGIC=843269191u;')
 s=one(s,'const uint G9_PATH_MAGIC=827341127u;','const uint G9_PATH_MAGIC=844121159u;')
 s=s.replace('(g9Input(1u)!=1u&&g9Input(1u)!=2u)','g9Input(1u)!=3u').replace('C[1]!=1u','C[1]!=2u')
 s=s.replace('for(uint i=106u;i<256u;++i)if(C[i]!=0u)return false;', 'for(uint i=114u;i<256u;++i)if(C[i]!=0u)return false;for(uint i=0u;i<8u;++i)if(C[106u+i]!=dmParam(i))return false;')
 s=s.replace('W[uCompletion+1u]=1u;','W[uCompletion+1u]=2u;')
 s=one(s,'if(C[0]!=G9_STATE_MAGIC||C[1]!=2u||C[2]!=G9_PROFILE', 'if(any(notEqual(uvec2(C[4u],C[5u]),uRun))||any(notEqual(uvec2(C[6u],C[7u]),uOwner))||any(notEqual(uvec2(C[8u],C[9u]),uContext)))return false;for(uint i=0u;i<8u;++i)if(C[42u+i]!=W[uConfig+16u+i])return false;for(uint i=0u;i<6u;++i)if(C[98u+i]!=W[uConfig+i])return false;if(C[21u]!=W[uConfig+7u])return false;\n if(C[0]!=G9_STATE_MAGIC||C[1]!=2u||C[2]!=G9_PROFILE')
 s=one(s,'C[10u]=commit.x;C[11u]=commit.y;C[16u]=uNonce.x;C[17u]=uNonce.y;', 'C[10u]=commit.x;C[11u]=commit.y;C[16u]=uNonce.x;C[17u]=uNonce.y;C[14u]=uInputGeneration.x;C[15u]=uInputGeneration.y;')
 # The predictor face reads ENTRY. Only corrector reconstruction reads MID;
 # the actual momentum accumulation below still starts from C entryP.
 s=one(s,'g9RneProduct(g9Momentum(c),g9U32(f),original,p)', 'g9RneProduct(uOp==G9_OP_CORRECTOR_FACE?dmRead64(c,40u):g9Momentum(c),g9U32(f),original,p)')
 s=one(s,'W[stage+14u]=g9Column(c,4u)==0u&&g9UCmp(g9Magnitude(p),limit)<=0?1u:0u;', 'W[stage+14u]=g9Column(c,4u)==0u&&g9UCmp(g9Magnitude(p),limit)<=0?1u:0u;if(uOp==G9_OP_CORRECTOR_FACE){dmPut(c,43u,W[stage+14u]);W[stage+14u]&=dmWord(c,42u);}')
 s=one(s,'else if(uOp==G9_OP_PREDICTOR_FACE){g9FaceFlux();if(!dmPredictor())return;}', 'else if(uOp==G9_OP_PREDICTOR_FACE){if(!g9FaceFlux()||!dmPredictor()){if(g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9Fault(29u,0u,0u);return;}}')
 s=one(s,'else if(uOp==G9_OP_CORRECTOR_FACE){g9FaceFlux();}', 'else if(uOp==G9_OP_CORRECTOR_FACE){if(!g9FaceFlux()||!dmPredictorEvents()){if(g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9Fault(29u,0u,0u);return;}}')
 s=one(s,'else if(uOp==G9_OP_DM_REQUEST){if(g9Cfl()&&g9SnapshotMicro())dmRequest();}', 'else if(uOp==G9_OP_DM_REQUEST){if(g9Cfl()&&g9SnapshotMicro()&&!dmRequest()&&g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9Fault(29u,0u,0u);}')
 s=one(s,'else if(uOp==G9_OP_DM_APPLY){for(uint c=0u;c<128u;++c)if(!dmApplyColumn(c))return;}', 'else if(uOp==G9_OP_DM_APPLY){for(uint c=0u;c<128u;++c)if(!dmApplyColumn(c)){if(g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9Fault(29u,c,0u);return;}if(!g9Cfl())return;}')
 s=one(s,'else if(uOp==G9_OP_MIDPOINT_BUILD){if(!dmMidpoint())return;}', 'else if(uOp==G9_OP_MIDPOINT_BUILD){if(!dmMidpoint()){if(g9Ctl(0u)==0u&&g9Ctl(14u)==0u)g9Fault(29u,0u,40u);return;}}')
 # post-friction stopping uses actual zero p and the same geometric yield.
 s=one(s,'uvec4 drive,threshold;if(!g9Drive(c,drive)||!g9StartThreshold(threshold)){g9Fault(29u,c,4u);return false;}\n  bool still=flow>0u&&g9Zero(newP)&&g9Column(c,13u)==0u&&g9SCmp(drive,threshold)<=0;', 'uvec4 chi,hg,og;if(!dmGeometry(c,chi,hg,og)){g9Fault(29u,c,4u);return false;}\n  bool still=flow>0u&&g9Zero(newP)&&g9Column(c,13u)==0u&&g9Zero(chi);')
 s=one(s,' if(uOp==G9_OP_FLIGHT_PATH){g9Flight();return;}', ' if(uOp==G9_OP_FLIGHT_PATH){if(lane==0u&&!dmStageEnter()){}memoryBarrierBuffer();barrier();if(g9Ctl(0u)==0u)g9Flight();return;}')
 s=one(s,'bool g9FlickVerify();','bool dmStageEnter();\nuint dmScopeAudit();\nbool g9FlickVerify();')
 s=one(s,'return uvec2(0u);\n}\nconst uint G9_OP_ROOT_INIT', 'if(kind==15u)return uvec2(4294967295u,2148007935u);if(kind==16u)return uvec2(1073741823u,2147483648u);return uvec2(0u);\n}\nconst uint G9_OP_ROOT_INIT')
 s=s.replace('row[1]=1u;row[2]=kind;', 'row[1]=2u;row[2]=kind;').replace('E[at+1u]!=1u','E[at+1u]!=2u')
 s=s.replace('g9Input(1u)==2u&&!g9FlickVerify()', 'g9Input(1u)==3u&&!g9FlickVerify()')
 s=s.replace('if(adapter==0u)return flags==0u;','if(adapter==0u)return false;')
 s=s.replace('adapter!=53401u||(flags&4u)==0u||(flags&~7u)!=0u','adapter!=63701u||(flags&8u)==0u||(flags&~11u)!=0u')
 s=s.replace('g9UCmp(duration,g9U32(8333334u))<0||g9UCmp(duration,g9U32(1000000000u))>0', 'any(notEqual(duration,g9U32(125000000u)))')
 s=s.replace('flags!=(4u|', 'flags!=(8u|')
 audit=(MOD/'physical_audit.glsl').read_text(encoding='utf-8-sig');deps['physical_audit.glsl']=sha((MOD/'physical_audit.glsl').read_bytes())
 # Forward declarations are ordinary function declarations; the bodies follow all parent helpers.
 s=one(s,'void g9BeginScope(){','void dmBeginScope();\nvoid dmSealScope();\nvoid g9BeginScope(){')
 s=one(s,' uint row[64];g9Event(row,G9_EVENT_ROOT_BEGIN,0u,0u,0u);row[15]=uJ;', ' dmBeginScope();\n uint row[64];g9Event(row,G9_EVENT_ROOT_BEGIN,0u,0u,0u);row[15]=uJ;')
 s=one(s,'void g9ScopeSeal(){','void g9ScopeSeal(){\n dmSealScope();if(g9Ctl(0u)!=0u)return;')
 s=one(s,'void g9Run(){',audit+'\nvoid g9Run(){')
 s=one(s,' uint lane=gl_LocalInvocationIndex;\n if(uOp==G9_OP_ROOT_INIT', ' uint lane=gl_LocalInvocationIndex;\n if(uOp==0x7fffff00u){if(lane==0u){g9Set(0u,0u);g9Set(1u,g9ConfigValid()&&all(equal(uvec2(C[14u],C[15u]),uInputGeneration))&&g9AuditMassGeometry()&&g9AuditMomentum()?0x641647u:0u);}return;}\n if(uOp==G9_OP_ROOT_INIT')
 s=one(s,'if(lane==0u)g9Set(16u,1u);return;', 'if(lane==0u){g9Set(16u,1u);g9Set(127u,1u);}return;')
 s=one(s,' if(lane!=0u)return;',' if(lane!=0u)return;\n if(!dmStageEnter())return;')
 s=one(s,'uint row[64];g9Event(row,G9_EVENT_STEP_SEAL,0u,0u,0u);','if(!dmMicroCertificate()){g9Fault(25u,0u,126u);return;}uint row[64];g9Event(row,G9_EVENT_STEP_SEAL,0u,0u,0u);')
 s=one(s,'uint flow=g9Column(c,1u),mode=g9Column(c,4u),oldRest=', 'uint flow=g9Column(c,1u),mode=dmWord(c,33u),oldRest=')
 s=s.replace('row[23]=65535u;', 'row[23]=dmScopeAudit();').replace('g9Set(17u,65535u);','g9Set(17u,dmRootAudit());')
 s,hygiene=convert(s,contract)
 coherent=[]
 for function, buffer, amount, address in [('g9MapAdd','C','q','g9MapAddress(flow,c,rank)'),('g9ProvisionalAdd','W','mass','g9Provisional(c,rank)')]:
  signature='bool '+function+'('
  if s.count(signature)!=1:raise ValueError('T691_FUNCTION_COUNT '+function)
  start=s.index(signature);end=s.index('\n}',start)
  slot=buffer+'[at+3u]'
  old=f'if(merge){{uint at={address},next;if(!g9AddMass({slot},{amount},next))return false;{slot}=next;return true;}}'
  new=f'if(merge){{uint at={address},next;uint prior={slot};if(!g9AddMass(prior,{amount},next))return false;{slot}=next;return true;}}'
  if s.count(old)!=1 or s[start:end].count(old)!=1 or new in s:raise ValueError('T691_MERGE_ANCHOR '+function)
  coherent.append({'function':function,'line':s[:s.index(old)].count('\n')+1,'before':old,'after':new})
  s=one(s,old,new)

 coherent_input_sha=sha(s.encode('utf8'))
 s,coherent_arguments=convert_values(s)
 readonly_input_sha=sha(s.encode('utf8'))
 s,readonly_arguments=convert_readonly_values(s)
 readonly_anchor=s[:s.index('uint rxReadBaseWord(uint at)')].count('\n')+1
 for argument in coherent_arguments:
  if argument['canonicalLine']>=readonly_anchor:argument['canonicalLine']+=1
 added_lines=s[:s.index('bool g9Capture(){')].count('\n')+1-hygiene['firstLine']
 hygiene['firstLine']+=added_lines
 hygiene['lastLine']+=added_lines
 if test_hooks:s+='\n// GNR1_NATIVE_HARNESS_NOT_PRODUCT\n'
 canonical=s.encode('utf8')
 if b'\r' in canonical or b'\0' in canonical or canonical.startswith(b'\xef\xbb\xbf') or '#line' in s:raise ValueError('RX_NON_CANONICAL_SOURCE')
 (out/'pipeline.comp').write_bytes(canonical)
 container='#pragma once\n#include <cstdint>\ninline constexpr const char* kRxPipeline=R"RXGPU('+s+')RXGPU";\ninline constexpr const char* kRxPipelineSHA="'+sha(canonical)+'";\ninline constexpr uint64_t kRxCanonicalShaderBytes='+str(len(canonical))+'ull;\ninline constexpr const char* kRxCaptureHygiene="'+REVISION+'";\n'
 (out/'pipeline_source.h').write_bytes(container.encode('utf8'))
 header='#pragma once\n#include <array>\n#include <cstdint>\nnamespace grs::g9rx {\n'
 seeds=tables['seed-config']['profiles']
 for profile in seeds:
  k=profile['K'];header+='inline constexpr std::array<uint32_t,64> kConfig'+str(k)+'={'+','.join(map(str,profile['config64']))+'};\n'
 header+='inline constexpr const char* kStaticContractSHA="'+deps['docs/architecture-T647-acceptance-v1.json']+'";\n'
 header+='inline constexpr const char* kLayoutSHA="'+sha(json.dumps(tables,sort_keys=True,separators=(',',':')).encode())+'";\n'
 specs=json.loads((ROOT/'docs/design-T637-manifest-v1.json').read_text(encoding='utf8'))['specHashes']
 for name,key in [('ClockSpec','nativeClockSpecSha256'),('MappingSpec','mappingSpecSha256'),('InputPolicy','inputPolicySha256')]:header+='inline constexpr const char* k'+name+'SHA="'+specs[key]+'";\n'
 for name,file in [('ClockCode','receipt_clock.h'),('MappingCode','receipt_recipe.h')]:header+='inline constexpr const char* k'+name+'SHA=\"'+sha((MOD/file).read_bytes())+'\";\n'
 header+='}\n';(out/'contract_generated.h').write_text(header,encoding='utf8')
 schema=(MOD/'frozen/schema_generated.h').read_text(encoding='utf8').replace('grs::g9dm','grs::g9rx').replace('kProfileCode=843924551u','kProfileCode=827477575u')
 schema=schema.replace('kInputMagic=843662407u','kInputMagic=1481788999u').replace('kConfigMagic=843331911u','kConfigMagic=826495559u').replace('kColdMagic=844383303u','kColdMagic=1129861703u').replace('gpu-spf-dynamic-mobilization-v2','gpu-g9rx-native-receipt-v1').replace('G9_SPF_DM_COLD_V2','G9_DM_RX_COLD_V1').replace('G9_SPF_DM_SEGMENT_PAGE_V2','G9_DM_RX_SEGMENT_PAGE_V1')
 (out/'schema_generated.h').write_text(schema,encoding='utf8')
 (out/'reader_schema.json').write_text(json.dumps(tables,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
 # Generated literal seed assignments; no CPU simulation.
 assignments=json.loads((ROOT/'docs/architecture-T552-g9-seed-manifest-v2.json').read_text(encoding='utf8'))['seed']['canonicalAssignments']
 seedReceipts=[]
 for profile in seeds:
  k=profile['K'];words=[0]*(profile['seedBytes']//4)
  for key,value in assignments['header'].items():words[int(key)]=value
  words[0]=844317767;words[1]=2;words[2]=827477575;words[24]=k;words[106:114]=profile['config64'][12:20]
  for c in range(128):
   for key,value in assignments['columnEvery32Words'].items():words[512+32*c+int(key)]=value
   words[4608+256*c:4612+256*c]=assignments['bedRank0Every64Cohort4']
  base=(280576+256*k)//4;words[base:base+16]=assignments['sourceRow0']
  actual=sha(struct.pack('<'+'I'*len(words),*words))
  if actual!=profile['templateSHA']:raise ValueError('SEED_TEMPLATE_IDENTITY '+str(k)+' '+actual)
  seedReceipts.append({'K':k,'bytes':len(words)*4,'actualTemplateSHA':actual})
 (out/'seed_constructor_receipt.json').write_text(json.dumps(seedReceipts,indent=2)+'\n',encoding='utf8')
 seed='#pragma once\n#include <algorithm>\n#include <cstddef>\n#include <cstdint>\nnamespace grs::g9rx {\ninline bool FillTemplate(uint32_t* w,size_t n,uint32_t k){if((k!=192&&k!=512)||n!=(542720+256*k)/4)return false;std::fill(w,w+n,0u);\n'
 for at,value in assignments['header'].items():seed+=f'w[{at}]={value}u;'
 seed+='w[0]=844317767u;w[1]=2;w[2]=827477575u;w[24]=k;\n'
 seed+='const auto& config=k==192?kConfig192:kConfig512;for(size_t i=0;i<8;++i)w[106+i]=config[12+i];\nfor(size_t c=0;c<128;++c){'
 for at,value in assignments['columnEvery32Words'].items():seed+=f'w[512+c*32+{at}]={value}u;'
 for i,value in enumerate(assignments['bedRank0Every64Cohort4']):seed+=f'w[4608+c*256+{i}]={value}u;'
 seed+='}\n'
 for i,value in enumerate(assignments['sourceRow0']):seed+=f'w[(280576+256*k)/4+{i}]={value}u;'
 seed+='return true;}\n}\n'
 (out/'seed_generated.h').write_text(seed,encoding='utf8')
 scene='#pragma once\n'
 for name,symbol in [('scene.vert','SceneVertex'),('scene.frag','SceneFragment'),('packet.vert','PacketVertex'),('packet.frag','PacketFragment')]:
  scene+='inline constexpr const char* kRx'+symbol+'=R"RXSCENE('+(MOD/'frozen'/name).read_text(encoding='utf8')+')RXSCENE";\n'
 scene+='inline constexpr const char* kRxSceneShaderSha="'+sha(scene.encode())+'";\n'
 (out/'scene_generated.h').write_text(scene,encoding='utf8')
 cold='#pragma once\n#include <array>\nnamespace grs::g9rx {\nstruct ColdSegment {size_t byteStart,bytes;};\n'
 for profile in tables['storage-cold']['profiles']:
  for mode,data in profile['modes'].items():
   cold+=f'inline constexpr std::array<ColdSegment,{len(data["segments"])}> kCold{profile["K"]}{mode}={{{{'+','.join('{'+str(seg['byteStart'])+','+str(seg['bytes'])+'}' for seg in data['segments'])+'}};\n'
 cold+='}\n';(out/'cold_generated.h').write_text(cold,encoding='utf8')
 for directory in [MOD, ROOT/'tools/g9rx', ROOT/'entry/src/main/cpp/gpu_resident/program_cache']:
  for file in sorted(directory.glob('*')):
   if file.is_file():deps[file.relative_to(ROOT).as_posix()]=sha(file.read_bytes())
 for relative in ['docs/architecture-T665-contract-v1.json','docs/architecture-T665-v1.md','docs/architecture-T691-v1.md','entry/src/main/cpp/gpu_resident/m2/create_diagnostics.h','entry/src/main/cpp/gpu_resident/uniform_locations.h','entry/src/main/cpp/gpu_resident/g9rx/frozen/source_receipt.json']:
  deps[relative]=sha((ROOT/relative).read_bytes())
 deps[QUALIFIED_VALUES.relative_to(ROOT).as_posix()]=sha(QUALIFIED_VALUES.read_bytes())
 deps={name.replace("\\","/"):value for name,value in deps.items()}
 implementation=hashlib.sha256();implementation.update(b'HARNESS' if test_hooks else b'PRODUCTION');implementation.update(Path(__file__).read_bytes());implementation.update(json.dumps(deps,sort_keys=True).encode())
 for path in sorted(MOD.glob('*')):
  if path.is_file():implementation.update(path.name.encode()+b'\0'+path.read_bytes())
 schema=schema.replace('t555-g9dm-v2-initial-construction','t644-gnr1-t641-t647-v1:'+implementation.hexdigest()).replace('kContainerRevision=2u','kContainerRevision=1u')
 (out/'schema_generated.h').write_text(schema,encoding='utf8')
 (out/'implementation_generated.h').write_text('#pragma once\n#define kRxImplementationSHA "'+implementation.hexdigest()+'"\n#define kRxCoherentRevision "'+COHERENT_REVISION+'"\n',encoding='utf8')
 identity={'schema':'GNR1_PIPELINE_IDENTITY_V2','shaderSHA':sha(canonical),'canonicalBytes':len(canonical),'canonicalLines':s.count('\n'),'hygieneRevision':REVISION,'scopeTransform':hygiene,'coherentRevision':COHERENT_REVISION,'coherentTransforms':coherent,'coherentArgumentInputSHA':coherent_input_sha,'coherentArgumentTransforms':coherent_arguments,'variant':'HARNESS_NOT_PRODUCT' if test_hooks else 'PRODUCTION','sceneSHA':sha(scene.encode()),'sourceManifestSHA':implementation.hexdigest(),'staticContractSHA':deps['docs/architecture-T647-acceptance-v1.json']}
 identity.update(readonlyRevision=READONLY_REVISION,readonlyArgumentInputSHA=readonly_input_sha,
                 readonlyArgumentTransforms=readonly_arguments)
 pipeline=sha(canonical)
 with (out/'contract_generated.h').open('a',encoding='utf8',newline='\n') as f:f.write('namespace grs::g9rx { inline constexpr const char* kPipelineSHA="'+pipeline+'"; }\n')
 identity['pipelineSHA']=pipeline
 if boot_only:
  sys.path.insert(0,str(ROOT/'tools/glsl'))
  from g9_boot import write_boot
  write_boot(out,canonical,schema,'RX')
  identity['selectedVariant']='BOOT_ONLY'
 reader=dict(tables);reader['sourceIdentity']={key:identity[key] for key in ['schema','shaderSHA','canonicalBytes','hygieneRevision','coherentRevision','coherentTransforms','coherentArgumentInputSHA','coherentArgumentTransforms','readonlyRevision','readonlyArgumentInputSHA','readonlyArgumentTransforms','sourceManifestSHA','variant']}
 (out/'reader_schema.json').write_bytes((json.dumps(reader,ensure_ascii=False,indent=2)+'\n').encode('utf8'))
 (out/'capture_line_map.json').write_bytes((json.dumps(hygiene,indent=2)+'\n').encode())
 (out/'inputs.json').write_bytes((json.dumps(deps,indent=2)+'\n').encode('utf8'))
 paths=[(ROOT/name if name.startswith(('entry/','docs/','tools/')) else MOD/name).resolve().as_posix() for name in deps]
 (out/'rx_dependencies.cmake').write_bytes(('set(RX_GENERATED_INPUTS\n'+''.join('"'+name+'"\n' for name in sorted(set(paths)))+')\n').encode('utf8'))
 for file in out.iterdir():
  if file.is_file() and file.suffix in ['.h','.json','.comp','.cmake']:file.write_bytes(file.read_bytes().replace(b'\r\n',b'\n'))
 identity['generatedFiles']={file.name:sha(file.read_bytes()) for file in sorted(out.iterdir()) if file.is_file() and file.name!='pipeline_identity.json'}
 identity['bundleSHA']=sha(json.dumps(identity,sort_keys=True,separators=(',',':')).encode())
 (out/'pipeline_identity.json').write_bytes((json.dumps(identity,indent=2)+'\n').encode())
 return s
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=MOD/'generated');p.add_argument('--test-hooks',action='store_true');p.add_argument('--boot-only',action='store_true');a=p.parse_args();generate(a.out,a.test_hooks,a.boot_only)

