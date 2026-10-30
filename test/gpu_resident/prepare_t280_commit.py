from pathlib import Path
import hashlib,subprocess
r=Path(__file__).resolve().parents[2];src=r/'entry/src/main/cpp/gpu_resident';out=r/'.codex/local-test/t280';out.mkdir(parents=True,exist_ok=True)
s=(src/'m1/quantum_core.comp').read_text()
for marker,path in [('INTEGER_PRIMITIVES','integer_primitives.glsl'),('EVENT_MASKS','m1/quantum_event_masks.glsl'),('T16','m1/quantum_t16.glsl'),('MOTION','m1/quantum_motion.glsl'),('STAGES','m1/quantum_stages.glsl')]:s=s.replace('/* '+marker+' */',(src/path).read_text())
s=s.replace('void main(){','void candidateMain(){')
s=s.replace('W[3]=W[1]+1u;','W[3]=uPass==23u?W[28]:W[1]+1u;')
s+='\n'+(src/'m1/quantum_replay.glsl').read_text()+'\n'+(src/'m1/quantum_geometry_audit.glsl').read_text()
s+='\n'+(src/'m1/quantum_seal.glsl').read_text()
seal=(src/'m1/source_seal.glsl').read_text();oldAudit=(src/'m1/source_audit.glsl').read_text()
pub=oldAudit[oldAudit.index('bool auditPublication(){'):]
pack=seal[seal.index('void packPublication(){'):seal.index('void sealCompletion(){')]
pack=pack.replace('if(W[S]==4u)','if(W[0]!=0u)')
pub=pub.replace('if(W[S]==4u)','if(W[0]!=0u)').replace('W[S+15u]|=512u;return W[S+15u]==1023u;','W[18]|=512u;return W[18]==1023u;')
s+='\n'+pack+'\n'+pub+'\n'+(src/'m1/quantum_commit_main.glsl').read_text()
fs=(src/'m1/source_core.comp').read_text().replace('/* INTEGER_PRIMITIVES */',(src/'integer_primitives.glsl').read_text()).replace('/* SOURCE_SEAL */',seal).replace('/* SOURCE_AUDIT */',(src/'m1/full_source_audit.glsl').read_text()+oldAudit[oldAudit.index('bool auditPublication(){'):])
fs=fs.replace('W[T+1u]=reason;}','W[T+1u]=reason;W[T+4u]=W[S+2u];}') # known quantized requested units on full-route rejection
fs=fs.replace('births>0u&&(next==0u||','births>0u&&(next==0u||(kind==1u&&C[uHeader+127u]!=0u)||')
fs=fs.replace('=next+births;if(next+births==0u)','=kind==1u&&next+births==0u?0xffffffffu:next+births;if(next+births==0u)')
fs=fs.replace('=next+rows;if(next+rows==0u)','=kind==1u&&next+rows==0u?0xffffffffu:next+rows;if(next+rows==0u)')
p=out/'commit.comp';p.write_text(s)
t=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S','comp',str(p)],text=True,capture_output=True);print(t.stdout+t.stderr);assert t.returncode==0
(out/'quantum_commit_kernel.h').write_text('#pragma once\nstatic constexpr const char* kM1QuantumCommit=R"M1QC('+s+')M1QC";\nstatic constexpr const char* kM1QuantumCommitSha="'+hashlib.sha256(s.encode()).hexdigest()+'";\n')
print(hashlib.sha256(s.encode()).hexdigest())
for name,text in [('full_source',fs)]:
    p=out/(name+'.comp');p.write_text(text);t=subprocess.run(['C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/toolchains/glslang_validator.exe','-S','comp',str(p)],text=True,capture_output=True);print(t.stdout+t.stderr);assert t.returncode==0
fullSha=hashlib.sha256((fs+'\n'+s).encode()).hexdigest()
(out/'full_m1_kernels.h').write_text('#pragma once\nstatic constexpr const char* kM1FullSource=R"M1FS('+fs+')M1FS";\nstatic constexpr const char* kM1QuantumCommit=R"M1QC('+s+')M1QC";\nstatic constexpr const char* kM1FullSha="'+fullSha+'";\n')
