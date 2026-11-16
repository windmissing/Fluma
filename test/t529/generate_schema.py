"""Single source for G9 native/GLSL layouts, masks and fixed schedules."""
from pathlib import Path
import hashlib,json,re
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/g9spf'
def identifier(s):return re.sub(r'[^A-Za-z0-9_]','_',s)
def generate():
 p=ROOT/'docs/architecture-T524-g9-spf-manifest-v1.json';e=json.loads(p.read_text(encoding='utf-8'));d=e['designTables'];D.mkdir(parents=True,exist_ok=True)
 cpp=['#pragma once','#include <array>','#include <cstddef>','#include <cstdint>','namespace grs::g9spf {',f'inline constexpr const char* kEngineeringSha="{hashlib.sha256(p.read_bytes()).hexdigest()}";',f'inline constexpr uint32_t kProfileCode={e["profileCode"]}u,kSeal={e["seal"]}u;']
 gl=['// T529 generated ABI: independent G9 namespace, no legacy wire alias.',f'const uint G9_PROFILE={e["profileCode"]}u,G9_SEAL={e["seal"]}u;']
 for n,v in e['identities'].items():cpp.append(f'inline constexpr uint32_t k{n.title()}Magic={v["magic"]}u;');gl.append(f'const uint G9_{n.upper()}_MAGIC={v["magic"]}u;')
 fields={'header':d['state']['headerWords'],'column':d['state']['columnWords'],'packet':d['state']['packetWords'],'source':d['state']['sourceRow16'],'input':d['input']['wordNames'],'completion':e['completion256']}
 for ns,words in fields.items():
  cpp.append(f'namespace {ns} {{')
  for i,w in enumerate(words):cpp.append(f'inline constexpr size_t {identifier(w)}={i};');gl.append(f'const uint G9_{ns.upper()}_{identifier(w)}={i}u;')
  cpp.append('}')
 for ns,enums in d['enums'].items():
  cpp.append(f'namespace {ns} {{')
  for name,code in enums.items():cpp.append(f'inline constexpr uint32_t {name}={code}u;');gl.append(f'const uint G9_{ns.upper()}_{name}={code}u;')
  cpp.append('}')
 for event in d['event']['kinds']:
  name=event['name'];code=event['code'];mask=event['usedMaskU32'];cpp.append(f'inline constexpr uint32_t kEvent{name}={code}u;');gl.append(f'const uint G9_EVENT_{name}={code}u;');gl.append(f'const uvec2 G9_MASK_{name}=uvec2({mask[0]}u,{mask[1]}u);')
 gl+=['uvec2 g9EventMask(uint kind){']+[f'if(kind=={v["code"]}u)return G9_MASK_{v["name"]};' for v in d['event']['kinds']]+['return uvec2(0u);','}']
 ops=list(dict.fromkeys([s['op'] for s in e['quantumPasses']]+[s['op'] for s in e['sourcePasses']]))
 cpp+=['enum class Op:uint32_t{'+','.join(ops)+'};','struct Pass {Op op;uint32_t role,j;int32_t micro;};']
 roles={'ROOT':0,'DISCOVERY':1,'AUTHORITATIVE':2,'REJECTED_PREFIX_CHECK':3,'REPLAY':4}
 for i,op in enumerate(ops):gl.append(f'const uint G9_OP_{op}={i}u;')
 for name,passes in [('Quantum',e['quantumPasses']),('Source',e['sourcePasses'])]:
  cpp.append(f'inline constexpr std::array<Pass,{len(passes)}> k{name}Passes={{{{')
  for s in passes:cpp.append('{Op::'+s['op']+','+','.join(map(str,[roles.get(s.get('role','ROOT'),0),s.get('Jslot',0),s.get('micro',-1)]))+'},')
  cpp.append('}};')
 cpp+=['struct Range{size_t begin,bytes;constexpr size_t End()const{return begin+bytes;}};',
       'struct Profile{uint32_t k,eventCapacity,pathCapacity;size_t stateBytes,workBytes,arenaBytes,payloadBytes,rootColdBytes,fullColdBytes;std::array<Range,8> state;std::array<Range,15> work;};',
       'inline constexpr std::array<Profile,2> kProfiles={{']
 for p in e['profiles']:
  arrays=[]
  for field in ['stateSegments','workSegments']:arrays.append('{{'+','.join('{'+str(s['byteStart'])+','+str(s['bytes'])+'}' for s in p[field])+'}}')
  scalar=[p['K'],p['eventCapacity'],p['pathCapacity'],p['stateBytes'],p['workBytes'],p['arenaBytes'],p['payloadBytes'],p['coldRootSlotBytes'],p['coldFullSlotBytes']]
  cpp.append('{'+','.join(map(str,scalar))+','+','.join(arrays)+'},')
 cpp+=['}};','inline const Profile* SelectProfile(uint32_t k){for(const auto& p:kProfiles)if(p.k==k)return &p;return nullptr;}','}']
 (D/'schema_generated.h').write_text('\n'.join(cpp)+'\n',encoding='utf-8');(D/'schema_generated.glsl').write_text('\n'.join(gl)+'\n',encoding='utf-8')
 (D/'reader_schema.json').write_text(json.dumps({'revision':'G9_SPF_Q20_V1','identities':e['identities'],'seal':e['seal'],'profile':e['profileCode'],'fields':fields,'event':d['event'],'profiles':e['profiles'],'cold':e['cold']},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':generate()
