"""Emit exact T527 segment capacities and a bounded, segment-local pager."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[2]
DEST=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def generate():
 d=json.loads((ROOT/'docs/architecture-T527-v5-cold-manifest-v1.json').read_text(encoding='utf-8'))
 lines=['#pragma once','#include "profiles_generated.h"','namespace grs::m3b::cold {',
        'struct SegmentSpec {const char* name;size_t begin,bytes;uint32_t stride;};']
 for p in d['profiles']:
  for mode in ['root','full']:
   ss=p[mode+'Segments'];key=f"{mode}{p['K']}_{p['Kw']}"
   lines.append(f'inline constexpr std::array<SegmentSpec,{len(ss)}> {key}={{{{')
   for s in ss:lines.append('{"'+s['name']+'",'+','.join(map(str,[s['byteStart'],s['bytes'],s['strideBytes']//4]))+'},')
   lines.append('}};')
 lines+=['struct Layout {const SegmentSpec* segments=nullptr;size_t count=0,words=0;bool fullBranch=false;',
         'static bool Make(const Profile& p,size_t alignment,Layout& out,bool full=false){if(!alignment||alignment%4)return false;']
 for p in d['profiles']:
  key=f"{p['K']}_{p['Kw']}"
  lines.append(f'if(p.k=={p["K"]}&&p.kw=={p["Kw"]}){{if(full)out={{full{key}.data(),full{key}.size(),{p["fullSegments"][-1]["byteEnd"]//4},true}};else out={{root{key}.data(),root{key}.size(),{p["rootSegments"][-1]["byteEnd"]//4},false}};return true;}}')
 lines+=['return false;}','size_t Pages()const{size_t n=1;for(size_t i=0;i<count;++i){const auto& s=segments[i];const size_t rows=s.bytes/(4*s.stride),per=1024/s.stride;n+=(rows+per-1)/per;}return n;}',
         'size_t Find(const char* name)const{for(size_t i=0;i<count;++i)if(std::strcmp(segments[i].name,name)==0)return i;return count;}','};','}']
 (DEST/'cold_layout_generated.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
generate()
