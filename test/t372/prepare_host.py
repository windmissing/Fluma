from pathlib import Path
import re, hashlib, sys
project=Path(sys.argv[1])
out=Path('.codex/local-test/t372-host')
out.mkdir(parents=True,exist_ok=True)
root=project/'entry/src/main/cpp/p013renderer'
v,f=[(root/'glsl'/f'single_sand.{ext}').read_text() for ext in ('vert','frag')]
(out/'single_sand_shaders.h').write_text(f'static constexpr auto kSingleSandVert=R"SS({v})SS";\nstatic constexpr auto kSingleSandFrag=R"SS({f})SS";\nstatic constexpr auto kSingleSandHash="{hashlib.sha256((v+f).encode()).hexdigest()}";\n')
s=Path('test/t364/host.cpp').read_text()
a=' auto old=p.token;p.frame++;packetHash(p);failSwap=true;r=pass.Draw(egl,p);'
assert s.count(a)==1
s=s.replace(a,Path('test/t372/host_probe.inc').read_text())
s=s.replace('static bool failSwap=false;', 'static bool failSwap=false; extern int readCalls;')
s=s.replace('auto r=pass.Draw(egl,p);','const int beforeNormal=readCalls;auto r=pass.Draw(egl,p);assert(readCalls-beforeNormal==3);')
(out/'host.cpp').write_text(s)
sdk=Path('.codex/local-test/t55-gpu/sdk/GLES3/gl3.h').read_text()
sources=s+'\n'+'\n'.join((root/name).read_text() for name in ('window_probe.h','single_sand_pass.cpp','fullframe_targets.cpp','material_uploader.cpp'))
gl='#include <EGL/egl.h>\n#include <GLES3/gl3.h>\n#include <cassert>\nint readCalls=0;\n'
for ret,name,params in re.findall(r'GL_APICALL\s+(.+?)\s*GL_APIENTRY\s+(gl\w+)\s*\(([^;]+?)\);',sdk):
 if not re.search(r'\b'+name+r'\(',sources) or 'GL_APIENTRY '+name+'(' in gl:continue
 args=[] if params=='void' else [re.search(r'([A-Za-z_]\w*)\s*(?:\[[^]]*\])?$',p.strip()).group(1) for p in params.split(',')]
 gl+=f'\nextern "C" {ret} GL_APIENTRY {name}({params}){{static auto fn=reinterpret_cast<PFN{name.upper()}PROC>(eglGetProcAddress("{name}"));assert(fn);'+('readCalls++;' if name=='glReadPixels' else '')+('return ' if ret!='void' else '')+f'fn({",".join(args)});}}'
(out/'gl.cpp').write_text(gl)
