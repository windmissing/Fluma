"""Assemble a real GPU trial/seed entry point for non-device execution checks."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
GPU=ROOT/'entry/src/main/cpp/gpu_resident'
read=lambda p:p.read_text(encoding='utf-8-sig')
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
for p in ['integer_primitives.glsl','m2/layout_generated.glsl','m2/merge_primitives.glsl','m2/pair_index.glsl','m3a/protocol_generated.glsl','m3a/occupied_solvers.glsl','m3a/source_base.glsl','m3a/source_events.glsl','m3a/source_seed.glsl']:text+='\n'+read(GPU/p)
text+='''
uint m3Input(uint i){return W[uM3Input+i];}
void main(){
 uint lane=gl_LocalInvocationIndex;
 if(uPass==0u){
  if(lane==0u){
   W[0]=0u;
   for(uint i=0u;i<128u;++i){m3Put(M3_REQUESTED+i,B[i]);m3Put(M3_ADMITTED+i,0u);}
   m3Put(31u,uM3YRequest);m3Put(M3_TRIAL_REASON,0u);m3Put(M3_TRIAL_BIRTHS,0u);m3Put(M3_TRIAL_MERGES,0u);m3Put(M3_TRIAL_BORN,0u);m3Put(M3_EVENT_CURSOR,0u);m3Put(M3_SCOPE_ORDINAL,0u);m3Put(M3_SEED,0u);m3Put(M3_BRANCH_LO,1u);m3Put(M3_BRANCH_HI,0u);m3Put(M3_PENDING_ORIGIN_LO,1u);m3Put(M3_PENDING_ORIGIN_HI,0u);
   if(!m3CaptureColumns()){m3SourceFail(15u,0u);return;}
   uint status=m3TrialEta(65536u);if(status!=0u){m3SourceFail(30u+status,0u);return;}
   for(uint c=0u;c<128u;++c){C[uColumn+6u*128u+c]=m3Get(M3_TARGET_WATER+c);m3SourceDerived(c);}
  }
  return;
 }
 if(uPass==1u){if(W[0]==0u)m3ExecuteSeeds();return;}
}
'''
out=ROOT/'.codex/local-test/t522-solver/seed.glsl';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(text,encoding='utf-8')
