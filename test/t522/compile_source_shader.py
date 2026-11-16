"""Compile every V4 SOURCE helper together on actual ES 3.1 (no device)."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
text='''#version 310 es
precision highp int;
layout(local_size_x=64)in;
layout(std430,binding=0)readonly buffer Base{uint B[];};
layout(std430,binding=1)coherent buffer Candidate{uint C[];};
layout(std430,binding=2)coherent buffer Workspace{uint W[];};
layout(std430,binding=3)coherent buffer Events{uint E[];};
uniform uint uK,uKw,uWater,uColumn,uHeader,uPass;
const uint Q=1048576u,CQ=268435456u;
uint col(uint f,uint c){return C[uColumn+f*128u+c];}
void m2LoadSand(uint slot,out uint row[16]);
uint m3Input(uint i);
'''
for name in ['integer_primitives.glsl','m2/layout_generated.glsl','m2/merge_primitives.glsl','m2/pair_index.glsl','m3a/protocol_generated.glsl','m3a/occupied_solvers.glsl','m3a/source_base.glsl','m3a/source_events.glsl','m3a/source_seed.glsl','m3a/source_admission.glsl','m3a/source_trial_audit.glsl','m3a/source_trial.glsl','m3a/source_ordinary.glsl','m3a/source_finalize.glsl','m3a/inventory_audit.glsl','m3a/source_root_audit.glsl','m3a/publication.glsl']:
    text+='\n'+(GPU/name).read_text(encoding='utf-8-sig')
# These dispatches exercise compiler/linker reachability, not a substitute for
# the production 72-slot sequencing or its root acceptance tests.
text+='''
void main(){
 if(uPass==100u){m3ExecuteSeeds();return;}
 if(gl_LocalInvocationIndex!=0u)return;
 if(uPass==0u){if(!m3DecodeSource())return;m3TrialOpen();}
 else if(uPass==1u)m3TrialSolve();
 else if(uPass==2u)m3TrialAudit();
 else if(uPass==3u)m3TrialReplayBegin();
 else if(uPass==4u)m3TrialReplayEnd();
 else if(uPass==5u)m3TrialClose();
 else if(uPass==6u)m3OrdinaryInitialize();
 else if(uPass==7u)m3OrdinaryWater();
 else if(uPass==8u)m3OrdinaryEnd();
 else if(uPass==9u)m3FinalizeSource();
 else if(uPass==10u){if(!m3AuditInventory()||!m3AuditSourceEvents()||!m3AuditRawCommand())m3SourceFail(15u,0u);}
 else if(uPass==11u)m3PackPublication();
 else if(uPass==12u){if(!m3AuditPublication())m3SourceFail(15u,0u);}
}
'''
out=ROOT/'.codex/local-test/t522-solver/source-all.glsl'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(text,encoding='utf-8')
driver=text[:text.index('\nvoid main(){')]
driver+='\n'+(GPU/'m3a/source_completion.glsl').read_text(encoding='utf-8')
driver+='\n'+(GPU/'m3a/source_driver.glsl').read_text(encoding='utf-8')
driver+='\nvoid main(){m3SourceSlot();}\n'
# Exercise the exact product entry and fragment ordering used by kernels.cmake.
template=(GPU/'m3a/source_v4.comp').read_text(encoding='utf-8-sig')
driver=re.sub(r'/\* ([A-Za-z0-9_/]+\.glsl) \*/',lambda m:(GPU/m[1]).read_text(encoding='utf-8-sig'),template)
out.with_name('source-driver.glsl').write_text(driver,encoding='utf-8')
