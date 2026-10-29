"""Reproducible offline ES310 semantic compilation; not a device execution test."""
from pathlib import Path
import hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2];SRC=ROOT/'entry/src/main/cpp/gpu_resident';OUT=ROOT/'test/reports/t236/resume';LOCAL=ROOT/'.codex/local-test/t236-shaders';LOCAL.mkdir(parents=True,exist_ok=True);OUT.mkdir(parents=True,exist_ok=True)
validator=Path('C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe')
text=(SRC/'m0_source.comp').read_text(encoding='utf-8').replace('/* GRS_LAYOUT */',(SRC/'layout_generated.glsl').read_text(encoding='utf-8')).replace('/* GRS_INTEGER */',(SRC/'integer_primitives.glsl').read_text(encoding='utf-8')).replace('/* GRS_SOURCE_ROOT */',(SRC/'m0_source_root.glsl').read_text(encoding='utf-8')).replace('/* GRS_QUANTUM */',(SRC/'m0_quantum.glsl').read_text(encoding='utf-8'))
p=LOCAL/'m0_source.comp';p.write_bytes(text.encode());run=subprocess.run([str(validator),'-S','comp',str(p)],capture_output=True,text=True);(OUT/'shader-compile.txt').write_text(run.stdout+run.stderr,encoding='utf-8')
r={'revision':'T236_ES310_M0_ROOT_SYNTAX_V1','sha256':hashlib.sha256(text.encode()).hexdigest(),'exitCode':run.returncode,'bindings':4,'localSize':64,'device':False,'execution':'NOT_EXECUTED','validator':str(validator)};(OUT/'shader-source.json').write_text(json.dumps(r,indent=2),encoding='utf-8');print(json.dumps(r,indent=2));print(run.stdout+run.stderr);assert run.returncode==0

scene_header='#pragma once\n'
for name,symbol in [('m0_scene_full.vert','kGrsFullVertex'),('m0_scene.frag','kGrsSceneFragment'),('m0_scene_particle.vert','kGrsParticleVertex'),('m0_scene_particle.frag','kGrsParticleFragment')]:
 body=(SRC/name).read_text(encoding='utf-8');path=LOCAL/name;path.write_bytes(body.encode())
 result=subprocess.run([str(validator),'-S',path.suffix[1:],str(path)],capture_output=True,text=True)
 print(name,result.returncode,result.stdout,result.stderr);assert result.returncode==0
 scene_header+='static constexpr const char* '+symbol+'=R"GRSC('+body+')GRSC";\n'
scene_sha=hashlib.sha256(scene_header.encode()).hexdigest()
scene_header+='static constexpr const char* kGrsSceneShaderSha="'+scene_sha+'";\n'
(LOCAL/'grs_scene_sources.h').write_bytes(scene_header.encode())
(OUT/'scene-shaders.json').write_text(json.dumps({'sha256':scene_sha,'stages':4,'compiled':True,'device':False},indent=2),encoding='utf-8')

# Deliberate test-only unlogged mutation, a distinct source artifact and SHA.
needle='finalContactFold();metadata(false);'
assert text.count(needle)==1
fault=text.replace(needle,'finalContactFold();if(C[0]!=0u)C[uK]+=1u;metadata(false);')
(LOCAL/'m0_unlogged_fault.comp').write_bytes(fault.encode())
(OUT/'fault-shader.json').write_text(json.dumps({'sha256':hashlib.sha256(fault.encode()).hexdigest(),'scope':'TEST_ONLY_UNLOGGED_WRITE_MUTATION','production':False},indent=2),encoding='utf-8')
