"""Version the existing owner/control transport for the independent V5 ABI.
No existing route, default profile, threshold or physical shader is edited.
"""
from pathlib import Path
import json
import re
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
DEST=GPU/'m3b'

def generate():
    contract=json.loads((ROOT/'docs/architecture-T512-v5-engineering-manifest-v1.json').read_text(encoding='utf-8'))
    previous=json.loads((ROOT/'docs/architecture-T498-m3-engineering-manifest-v1.json').read_text(encoding='utf-8'))
    for path in (GPU/'m3a').iterdir():
        if path.suffix not in ['.h','.cpp','.inc','.cmake','.frag'] or path.name in ['profiles_generated.h','protocol_generated.h','cold_contract.h','cold_trace.h']:
            continue
        text=path.read_text(encoding='utf-8-sig').replace('m3a','m3b').replace('M3a','M3b').replace('V4','V5').replace('v4','v5')
        (DEST/path.name.replace('v4','v5')).write_text(text,encoding='utf-8')
    # The public V5 fields come from its own frozen dictionary; retain only the
    # inherited kind/enumeration namespaces, not V4 header reserved names.
    old=(GPU/'m3a/protocol_generated.h').read_text(encoding='utf-8')
    enums=old[old.index('namespace sourceMeta32 {'):old.rindex('}')]
    pre='''#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "lineage_schema.h"
namespace grs::m3b {
inline constexpr uint32_t kColumnCapacity=268435456,kSourcePasses=72;
inline constexpr const char* kDesignSha=kEngineeringSha;
'''
    (DEST/'protocol_generated.h').write_text(pre+enums+'}\n',encoding='utf-8')
    opcodes=['OCCUPIED_SCOPE_CORE','PACK_PUBLICATION','PLAN_CONTROL_0','PLAN_CONTROL_1','PLAN_CONTROL_2','PLAN_CONTROL_3','PRIVATE_PROMOTE','PUBLICATION_AUDIT','SCOPE_FINALIZE','SCOPE_INIT','SEAL_COMPLETION','SEMANTIC_COMPARE','WATER_MATCH_ORDERED','WATER_PROJECTION_AUDIT','WATER_TARGET_40','INPUT_E_REQUEST','LIFT_LOOP','LINEAGE_U_AUDIT','POST_REPLAY_LINEAGE_COMPARE']
    lines=['#pragma once','#include "protocol_generated.h"','namespace grs::m3b {',
           'enum class Opcode:uint32_t{'+','.join(opcodes)+'};',
           'struct Pass {Opcode opcode;uint32_t role,nativeStage,count,sortK,sortJ,color;};',
           'struct Segment {size_t begin,bytes;constexpr size_t End()const{return begin+bytes;}};',
           'struct Profile {uint32_t k,kw,sourceRows,scopeRows,quantumFrameRows;size_t stateBytes,payloadBytes,sourceWorkBytes,quantumWorkBytes;std::array<Segment,4> source;std::array<Segment,3> quantum;const Pass* passes;size_t passCount;};']
    for p in contract['profiles']:
        key=f"{p['K']}_{p['Kw']}";passes=p['quantumPasses']
        lines.append(f'inline constexpr std::array<Pass,{len(passes)}> kPasses{key}={{{{')
        for slot in passes:
            c=slot.get('core',{});stage=c.get('nativeStage',0xffffffff)
            n=c.get('n',1)
            # V5 copies every extension word. The three-colour apply is one
            # group, its lane maps edge=3*lane+colour in the V5 shader.
            if slot['opcode']=='OCCUPIED_SCOPE_CORE' and stage in [0,22]:n=p['stateBytesV5']//4
            if slot['opcode']=='OCCUPIED_SCOPE_CORE' and stage==14:n=64
            lines.append('{Opcode::'+slot['opcode']+','+','.join(str(v)+'u' for v in [slot['role'],stage,n,c.get('sortK',0),c.get('sortJ',0),c.get('color',0)])+'},')
        lines.append('}};')
    lines.append('inline constexpr std::array<Profile,4> kProfiles={{')
    for p,oldp in zip(contract['profiles'],previous['profiles']):
        ext=p['stateExtensionBytes'];ea=p['provenanceRowsPerScope'];e=p['physicalRowsPerScope']
        source=[];at=oldp['sourceWork']['baseBytes']+ext
        for s in oldp['sourceWork']['segments']:
            source.append((at,s['bytes']));at+=s['bytes']
        sourceBytes=at+32768+256
        quantum=[];at=oldp['quantumWork']['baseBytes']+ext
        for s in oldp['quantumWork']['segments']:
            size=e*256+ea*128 if s['name']=='spareExpectedEvents' else s['bytes']
            quantum.append((at,size));at+=size
        quantumBytes=at+32768+256+256
        groups=[]
        for ss in [source,quantum]:groups.append('{{'+','.join('{'+str(a)+','+str(b)+'}' for a,b in ss)+'}}')
        scalars=[p['K'],p['Kw'],oldp['source128Rows'],e,p['K']+4*e,p['stateBytesV5'],p['payloadBytes'],sourceBytes,quantumBytes]
        key=f"{p['K']}_{p['Kw']}"
        lines.append('{'+','.join(map(str,scalars))+','+','.join(groups)+f',kPasses{key}.data(),kPasses{key}.size()'+'},')
    lines+=['}};','inline const Profile* SelectProfile(uint32_t k,uint32_t kw){for(const auto& p:kProfiles)if(p.k==k&&p.kw==kw)return &p;return nullptr;}','}']
    (DEST/'profiles_generated.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__':generate()
