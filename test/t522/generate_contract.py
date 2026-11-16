"""Generate the independent T497/T498 V4 ABI and pass/resource tables."""
from pathlib import Path
import hashlib
import json
import re

ROOT=Path(__file__).resolve().parents[2]
DEST=ROOT/'entry/src/main/cpp/gpu_resident/m3a'
DESIGN=ROOT/'docs/design-T497-gpu-M3-manifest-v1.json'
ENGINEERING=ROOT/'docs/architecture-T498-m3-engineering-manifest-v1.json'

def identifier(text):
    value=re.sub('[^A-Za-z0-9_]','_',text)
    return 'field_'+value if value[0].isdigit() else value

def generate():
    design=json.loads(DESIGN.read_text(encoding='utf-8'))
    engineering=json.loads(ENGINEERING.read_text(encoding='utf-8'))
    assert engineering['executionReady'] and design['versions']['features']==15
    quota=json.loads((ROOT/'docs/design-T508-blocked-quota-manifest-v1.json').read_text(encoding='utf-8'))['snapshot']
    assert quota['revision']==3 and quota['words']==256 and quota['bytes']==1024
    quota_lines=['#pragma once','#include <cstddef>','namespace grs::m3a::quota {']
    for word,name in quota['fields'].items():
        if not name.startswith('reserved'):
            assert identifier(name)==name
            quota_lines.append(f'inline constexpr size_t {name}={word};')
    quota_lines.append('}')
    (DEST/'quota_fields_generated.h').write_text('\n'.join(quota_lines)+'\n',encoding='utf-8')
    lines=['#pragma once','#include <array>','#include <cstddef>','#include <cstdint>',
           'namespace grs::m3a {', '// Generated from the formal T497/T498 machine contracts.',
           'inline constexpr uint32_t kWireRevision=4,kFeatures=15,kChannelBitmap=7;',
           'inline constexpr uint32_t kQ=1048576,kColumnCapacity=268435456,kSourcePasses=72;',
           'inline constexpr const char* kDesignSha="'+hashlib.sha256(DESIGN.read_bytes()).hexdigest()+'";',
           'inline constexpr const char* kEngineeringSha="'+hashlib.sha256(ENGINEERING.read_bytes()).hexdigest()+'";']
    for name,fields in design['layout'].items():
        if isinstance(fields,list):
            assert len(set(fields))==len(fields)
            lines+=['namespace '+name+' {']
            lines += ['inline constexpr size_t '+identifier(field)+'='+str(i)+';' for i,field in enumerate(fields)]
            lines+=['}']
    for name in ['roles','sourceStopCodes','trialReasons','fatalReasons','plannerStatuses','rootOutcomes','auditFamilies']:
        lines+=['namespace '+name+' {']
        lines+=['inline constexpr uint32_t '+identifier(key)+'='+str(value)+';' for key,value in design[name].items()]
        lines+=['}']
    for name in ['sourceEvent128','quantumEvent64','control128']:
        obj=design[name];lines+=['namespace '+name+' {']
        for key in ['magic','revision','seal']:
            if key in obj:lines+=['inline constexpr uint32_t '+key+'='+str(obj[key])+'u;']
        for key,kind in obj.get('kinds',{}).items():
            lines+=['inline constexpr uint32_t '+identifier(key)+'='+str(kind['code'] if isinstance(kind,dict) else kind)+';']
            mask=kind.get('usedMaskU32') if isinstance(kind,dict) else None
            if mask:lines+=['inline constexpr std::array<uint32_t,'+str(len(mask))+'> '+identifier(key)+'Mask={'+','.join(str(v)+'u' for v in mask)+'};']
        lines+=['}']
    lines+=['}']
    (DEST/'protocol_generated.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    glsl=['// Generated V4 source event field masks.']
    for name,kind in design['sourceEvent128']['kinds'].items():
        glsl+=['const uint M3_'+name+'='+str(kind['code'])+'u;']
        glsl+=['const uvec4 M3_'+name+'_MASK=uvec4('+','.join(str(v)+'u' for v in kind['usedMaskU32'])+');']
    glsl+=['uvec4 m3SourceMask(uint kind){']
    for name,kind in design['sourceEvent128']['kinds'].items():
        glsl+=['if(kind==M3_'+name+')return M3_'+name+'_MASK;']
    glsl+=['return uvec4(0u);','}']
    (DEST/'protocol_generated.glsl').write_text('\n'.join(glsl)+'\n',encoding='utf-8')
    opcodes=sorted({p['opcode'] for profile in engineering['profiles'] for p in profile['quantumPasses']})
    lines=['#pragma once','#include "protocol_generated.h"','namespace grs::m3a {',
           'enum class Opcode:uint32_t{'+','.join(opcodes)+'};',
           'struct Pass {Opcode opcode;uint32_t role,nativeStage,count,sortK,sortJ,color;};',
           'struct Segment {size_t begin,bytes;constexpr size_t End()const{return begin+bytes;}};',
           'struct Profile {uint32_t k,kw,sourceRows,scopeRows,quantumFrameRows;size_t stateBytes,payloadBytes,sourceWorkBytes,quantumWorkBytes;std::array<Segment,4> source;std::array<Segment,3> quantum;const Pass* passes;size_t passCount;};']
    for profile in engineering['profiles']:
        key=str(profile['K'])+'_'+str(profile['Kw'])
        passes=profile['quantumPasses'];assert len(passes)==profile['quantumDispatch']
        lines+=['inline constexpr std::array<Pass,'+str(len(passes))+'> kPasses'+key+'={{']
        for index,p in enumerate(passes):
            assert p['index']==index and p['bindingCount']==4
            core=p.get('core',{})
            lines+=['{Opcode::'+p['opcode']+','+','.join(str(v)+'u' for v in [p['role'],core.get('nativeStage',0xffffffff),core.get('n',1),core.get('sortK',0),core.get('sortJ',0),core.get('color',0)])+'},']
        lines+=['}};']
    lines+=['inline constexpr std::array<Profile,4> kProfiles={{']
    for p in engineering['profiles']:
        k=str(p['K'])+'_'+str(p['Kw'])
        groups=[]
        for name in ['sourceWork','quantumWork']:
            previous=p[name]['baseBytes'];segments=p[name]['segments']
            for segment in segments:
                assert segment['byteStart']==previous and segment['byteEnd']==segment['byteStart']+segment['bytes']
                assert segment['byteStart']%16==0
                previous=segment['byteEnd']
            assert previous==p[name]['finalBytes']
            groups.append('{{'+','.join('{'+str(s['byteStart'])+','+str(s['bytes'])+'}' for s in segments)+'}}')
        scalar=[p['K'],p['Kw'],p['source128Rows'],p['scopeQuantumRows'],p['quantumFrameRowsAllocated'],p['stateBytes'],p['payloadBytes'],p['sourceWork']['finalBytes'],p['quantumWork']['finalBytes']]
        lines+=['{'+','.join(map(str,scalar))+','+','.join(groups)+',kPasses'+k+'.data(),kPasses'+k+'.size()},']
    lines+=['}};','inline const Profile* SelectProfile(uint32_t k,uint32_t kw){for(const auto& p:kProfiles)if(p.k==k&&p.kw==kw)return &p;return nullptr;}','}']
    (DEST/'profiles_generated.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__':generate()
