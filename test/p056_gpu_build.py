from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
out=root/'.codex/local-test/t56-gpu'
out.mkdir(exist_ok=True,parents=True)
p=root/'entry/src/main/cpp/p013renderer'
v=(p/'glsl/p1_material.vert').read_text(encoding='utf-8');f=(p/'glsl/p1_material.frag').read_text(encoding='utf-8').replace('/* P013_MATERIAL_FUNCTIONS_V1 */',(root/'entry/src/main/ets/render/preparation/p1_material_functions.glsl').read_text(encoding='utf-8'))
(out/'p055_shaders.h').write_text('#pragma once\nconst char* kMaterialVert=R"P055('+v+')P055";\nconst char* kMaterialFrag=R"P055('+f+')P055";\n',encoding='utf-8')
sources=''.join((p/n).read_text(encoding='utf-8') for n in ['scene_layers.cpp','fullframe_targets.cpp','scene_gpu.cpp','dualfield_pass.cpp','material_pass.cpp','material_uploader.cpp'])+(root/'test/p013_native/t56_scene_gpu_test.cpp').read_text(encoding='utf-8-sig')
for prefix,lib in [('gl','libGLESv2'),('egl','libEGL')]:
 names=sorted(set(re.findall(r'\b('+prefix+r'[A-Z]\w*)\(',sources)))
 (out/(lib+'.def')).write_text('LIBRARY '+lib+'.dll\nEXPORTS\n'+'\n'.join(names)+'\n')
