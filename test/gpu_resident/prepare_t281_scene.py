from pathlib import Path
import hashlib,subprocess
r=Path(__file__).resolve().parents[2];src=r/'entry/src/main/cpp/gpu_resident';out=r/'.codex/local-test/t281';out.mkdir(exist_ok=True,parents=True)
pairs=[('m0_scene_full.vert','kM1FullVertex'),('m1/scene.frag','kM1SceneFragment'),('m0_scene_particle.vert','kM1ParticleVertex'),('m0_scene_particle.frag','kM1ParticleFragment'),('m1/water_glyph.vert','kM1WaterVertex'),('m1/water_glyph.frag','kM1WaterFragment')]
header='#pragma once\n'
for file,symbol in pairs:
 text=(src/file).read_text().replace('#version 300 es','#version 310 es');header+='static constexpr const char* '+symbol+'=R"M1SC('+text+')M1SC";\n'
 p=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S',file.split('.')[-1],str(src/file)],capture_output=True,text=True);print(p.stdout+p.stderr);assert p.returncode==0
header+='static constexpr const char* kM1SceneSha="'+hashlib.sha256(header.encode()).hexdigest()+'";\n';(out/'m1_scene_sources.h').write_text(header)
