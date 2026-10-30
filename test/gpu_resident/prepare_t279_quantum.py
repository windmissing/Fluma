"""Build the isolated candidate shader; no device or product routing."""
from pathlib import Path
import hashlib,json,subprocess
r=Path(__file__).resolve().parents[2]
src=r/'entry/src/main/cpp/gpu_resident'
out=r/'.codex/local-test/t279';out.mkdir(parents=True,exist_ok=True)
manifest=json.loads((r/'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json').read_text())
masks=['uvec2(0u)']*15
for e in manifest['eventKinds']:
    v=int(e['usedMaskHex'],16);masks[e['id']]=f'uvec2(0x{v&0xffffffff:08x}u,0x{v>>32:08x}u)'
mask='// Generated from T238 EVENT64 V2; unused words must be zero.\nconst uvec2 m1Masks[15]=uvec2[15]('+','.join(masks)+');\n'
(src/'m1/quantum_event_masks.glsl').write_text(mask)
s=(src/'m1/quantum_core.comp').read_text()
for marker,path in [('INTEGER_PRIMITIVES','integer_primitives.glsl'),('EVENT_MASKS','m1/quantum_event_masks.glsl'),('T16','m1/quantum_t16.glsl'),('MOTION','m1/quantum_motion.glsl'),('STAGES','m1/quantum_stages.glsl')]:
    s=s.replace('/* '+marker+' */',(src/path).read_text())
p=out/'quantum_core.comp';p.write_text(s)
(out/'quantum_core_kernel.h').write_text('#pragma once\nstatic constexpr const char* kM1QuantumCore=R"M1QC('+s+')M1QC";\nstatic constexpr const char* kM1QuantumCoreSha="'+hashlib.sha256(s.encode()).hexdigest()+'";\n')
t=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S','comp',str(p)],capture_output=True,text=True)
print(t.stdout+t.stderr);assert t.returncode==0
print(json.dumps({'semanticCompile':'PASS','sha256':hashlib.sha256(s.encode()).hexdigest(),'scope':'T279_CANDIDATE_ONLY','device':'NOT_RUN'}))
