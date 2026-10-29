from pathlib import Path
import json,hashlib,re
root=Path(__file__).resolve().parents[2]
raw=(root/'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json').read_bytes()
m=json.loads(raw.decode('utf-8-sig'));lines=['#pragma once','#include <cstdint>','#include <cstddef>','namespace grs { namespace m1 {',f'inline constexpr const char* manifestSha="{hashlib.sha256(raw).hexdigest()}";','inline constexpr uint32_t revision=2,features=3;']
for name,layout in m['layouts'].items():
    lines.append('namespace '+name+' {')
    for i,field in enumerate(layout['words']):
        field=re.sub(r'[^a-zA-Z0-9_]','_',field)
        lines.append(f'inline constexpr size_t {field}={i};')
    lines.append('}')
lines+=['struct Profile {uint32_t K,Kw,events,sourceStages,quantumStages;uint64_t payload,stateBytes,publicationBytes;};','inline constexpr Profile profiles[]={']
for p in m['resourceTemplates']:
    lines.append('{'+','.join(str(p[k]) for k in ['K','Kw','frameEventCapacity','sourceDispatchDesignStages','quantumDispatchDesignStages','totalDesignPayloadBytes','stateBankRawBytes','publicationSlotRawBytes'])+'},')
lines+=['};','}}']
out=root/'entry/src/main/cpp/gpu_resident/m1';out.mkdir(exist_ok=True)
(out/'layout_generated.h').write_text('\n'.join(lines)+'\n')
print('T240 M1 manifest layouts/profiles generated')
