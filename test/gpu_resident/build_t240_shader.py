from pathlib import Path
import subprocess,json,hashlib
root=Path(__file__).resolve().parents[2];src=root/'entry/src/main/cpp/gpu_resident'
s=(src/'m1/water_edge.comp').read_text().replace('/* INTEGER_PRIMITIVES */',(src/'integer_primitives.glsl').read_text())
out=root/'.codex/local-test/t240-host';out.mkdir(exist_ok=True,parents=True);p=out/'water_edge.comp';p.write_text(s)
r=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S','comp',str(p)],capture_output=True,text=True)
result={'revision':'T240_WATER_EDGE_PROPOSAL_V1','sha256':hashlib.sha256(s.encode()).hexdigest(),'compileExitCode':r.returncode,'log':r.stdout+r.stderr,'execution':'NOT_RUN','productM1':'NOT_IMPLEMENTED'}
target=root/'test/reports/t240';target.mkdir(exist_ok=True,parents=True);(target/'shader.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));assert r.returncode==0
