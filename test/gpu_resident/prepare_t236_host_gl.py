from pathlib import Path
import re,json
ROOT=Path(__file__).resolve().parents[2];LOCAL=ROOT/'.codex/local-test/t236-source-host';LOCAL.mkdir(parents=True,exist_ok=True);SRC=ROOT/'entry/src/main/cpp/gpu_resident';SDK=ROOT/'.codex/local-test/t55-gpu/sdk'
text='\n'.join(p.read_text(encoding='utf-8') for p in [SRC/'source_transport.cpp',SRC/'scene.cpp',SRC.parent/'p013renderer/fullframe_targets.cpp']);names=set(re.findall(r'\bgl\w+(?=\()',text));names.add('glGetString')
prototypes={}
for name in ['gl3.h','gl31.h']:
 raw=(SDK/'GLES3'/name).read_text(encoding='utf-8')
 for ret,fun,params in re.findall(r'GL_APICALL\s+(.+?)\s*GL_APIENTRY\s+(gl\w+)\s*\(([^;]+?)\);',raw):prototypes[fun]=(ret.strip(),params.strip())
lines=['#include <cstdint>','#include <EGL/egl.h>','#include <GLES3/gl31.h>','#include <cassert>']
for name in sorted(names):
 ret,params=prototypes[name];args=[]
 if params!='void':
  for param in params.split(','):args.append(re.search(r'([A-Za-z_]\w*)\s*(?:\[[^]]*\])?$',param.strip()).group(1))
 lines.append('extern "C" '+ret+' GL_APIENTRY '+name+'('+params+'){ static auto fn=reinterpret_cast<PFN'+name.upper()+'PROC>(eglGetProcAddress("'+name+'"));assert(fn);'+('return ' if ret!='void' else '')+'fn('+','.join(args)+');}')
(LOCAL/'gl_forward.cpp').write_text('\n'.join(lines),encoding='utf-8')
raw=(ROOT/'test/p013_native/t80_libEGL.def').read_text();(LOCAL/'egl.def').write_text(raw+'\neglGetCurrentContext\neglGetProcAddress\neglSwapBuffers\neglGetCurrentDisplay\neglGetCurrentSurface\n',encoding='utf-8')
r=json.loads((ROOT/'test/reports/t236/resume/shader-source.json').read_text(encoding='utf-8'));(LOCAL/'source_hash.h').write_text('#pragma once\nconstexpr char kHostSourceHash[]="'+r['sha256']+'";\n',encoding='utf-8')

(LOCAL/'host_clock.h').write_text('#pragma once\n#include <ctime>\n#include <chrono>\n#ifndef CLOCK_MONOTONIC\n#define CLOCK_MONOTONIC 1\ninline int clock_gettime(int, timespec* t){ auto n=std::chrono::steady_clock::now().time_since_epoch(); auto s=std::chrono::duration_cast<std::chrono::seconds>(n);t->tv_sec=s.count();t->tv_nsec=std::chrono::duration_cast<std::chrono::nanoseconds>(n-s).count();return 0;}\n#endif\n',encoding='utf-8')


fault=json.loads((ROOT/'test/reports/t236/resume/fault-shader.json').read_text(encoding='utf-8'))
with (LOCAL/'source_hash.h').open('a',encoding='utf-8') as file:file.write('constexpr char kHostFaultHash[]="'+fault['sha256']+'";\n')
