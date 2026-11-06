from pathlib import Path
import hashlib,re
out=Path('.codex/local-test/t364-host')
out.mkdir(parents=True,exist_ok=True)
root=Path('entry/src/main/cpp/p013renderer')
v,f=[(root/'glsl'/f'single_sand.{ext}').read_text() for ext in ('vert','frag')]
(out/'single_sand_shaders.h').write_text(f'static constexpr auto kSingleSandVert=R"SS({v})SS";\nstatic constexpr auto kSingleSandFrag=R"SS({f})SS";\nstatic constexpr auto kSingleSandHash="{hashlib.sha256((v+f).encode()).hexdigest()}";\n')
sources=[root/n for n in ('single_sand_pass.cpp','fullframe_targets.cpp','material_uploader.cpp')]
source='\n'.join(p.read_text() for p in sources+[Path('test/t364/host.cpp')])
names=set(re.findall(r'\bgl[A-Z]\w+(?=\()',source))
sdk=Path('.codex/local-test/t55-gpu/sdk/GLES3/gl3.h').read_text()
prototypes={name:(ret,params) for ret,name,params in re.findall(r'GL_APICALL\s+(.+?)\s*GL_APIENTRY\s+(gl\w+)\s*\(([^;]+?)\);',sdk)}
lines=['#include <EGL/egl.h>','#include <GLES3/gl3.h>','#include <cassert>']
for name in sorted(names):
 if name not in prototypes:continue
 ret,params=prototypes[name]
 args=[] if params=='void' else [re.search(r'([A-Za-z_]\w*)\s*(?:\[[^]]*\])?$',p.strip()).group(1) for p in params.split(',')]
 lines.append(f'extern "C" {ret} GL_APIENTRY {name}({params}){{static auto fn=reinterpret_cast<PFN{name.upper()}PROC>(eglGetProcAddress("{name}"));assert(fn);'+('return ' if ret!='void' else '')+f'fn({",".join(args)});}}')
(out/'gl.cpp').write_text('\n'.join(lines))
