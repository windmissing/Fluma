from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).resolve().parents[2];src=r/'entry/src/main/cpp/gpu_resident';out=r/'.codex/local-test/t240-source';out.mkdir(exist_ok=True,parents=True)
s=(src/'m1/source_core.comp').read_text().replace('/* INTEGER_PRIMITIVES */',(src/'integer_primitives.glsl').read_text());p=out/'source_core.comp';p.write_text(s)
t=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S','comp',str(p)],capture_output=True,text=True);print(t.stdout+t.stderr);assert t.returncode==0
target=r/'test/reports/t240/source-shader.json';target.write_text(json.dumps({'sha256':hashlib.sha256(s.encode()).hexdigest(),'revision':'T240_SOURCE_CORE_V1','semanticCompile':'PASS','productRoute':'NOT_IMPLEMENTED'},indent=2))
