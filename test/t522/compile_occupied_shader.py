"""Link the versioned GPU motion core and new global water passes together."""
from pathlib import Path
from generate_occupied_core import generate,ROOT,GPU
from generate_occupied_replay import generate as generate_replay
generate()
generate_replay()
text=(GPU/'m3a/occupied_core.glsl').read_text(encoding='utf-8')
text+='\nuniform uint uM3SourceEvents,uM3SourceMeta,uM3CandidateBank;\n'
for name in ['occupied_solvers.glsl','water_event.glsl','water_writer.glsl','inventory_audit.glsl','occupied_replay.glsl','planner.glsl','publication.glsl']:
    part=(GPU/'m3a'/name).read_text(encoding='utf-8')
    if name=='water_event.glsl':part=part.replace('uniform uint uM3Role,uM3EventCapacity','uniform uint uM3EventCapacity')
    text+='\n'+part
text+='''
void main(){
 uint i=gl_GlobalInvocationID.x;
 m3ScopeAngle=int(W[m3RoleAddress(uM3Role)+1u]);
 if(uPass==0u){if(i<uWords)C[i]=B[i];if(i>0u&&i<32u)W[i]=0u;if(i<uK+uKw)W[uRows+i]=0u;return;}
 if(W[0]!=0u)return;
 if(uPass==1u){if(i==0u)prepare();return;}
 if(uPass==2u){if(i==0u&&!m3WaterTarget(int(W[5])))qfail(30u);return;}
 if(uPass==3u){if(i==0u&&!m3WaterMatch())qfail(30u);return;}
 if(uPass==4u){if(i==0u&&!m3WaterProjectionAudit())qfail(30u);return;}
 if(uPass==5u){if(i<128u){W[uFrozen+i]=eta(i);W[uFrozen+128u+i]=col(6u,i)>0u?1u:0u;W[uFrozen+256u+i]=2u*col(10u,i)-(2u*i+1u)*W[5];W[uFrozen+384u+i]=2u*eta(i)-(2u*i+1u)*W[5];}return;}
 if(uPass==6u){if(i<uPad)W[uSandIndex+i]=i;return;}
 if(uPass==7u){sortIndex(i,uSandIndex,uPad,0u,false);return;}
 if(uPass==8u){if(i<uKw)W[uWaterIndex+i]=i;return;}
 if(uPass==9u){sortIndex(i,uWaterIndex,uKw,1u,false);return;}
 if(uPass==10u){if(i==0u)resolvePool(1u);return;}
 if(uPass==11u){if(i==0u)resolvePool(0u);return;}
 if(uPass==12u){if(i<127u)bedPropose(i);return;}
 if(uPass==13u){if(i==0u)bedPrefix();return;}
 if(uPass==14u){if(i<127u)edgeApply(i,true);return;}
 if(uPass==15u){if(i==0u)finalSand();return;}
 if(uPass==16u){if(i<uKw)W[uArrivalIndex+i]=i;return;}
 if(uPass==17u){sortIndex(i,uArrivalIndex,uKw,1u,true);return;}
 if(uPass==18u){if(i==0u)arrivals();return;}
 if(uPass==19u){if(i==0u)finishCandidate();return;}
 if(uPass==21u){if(i==0u){if(quantumAudit())for(uint word=0u;word<uWords;++word)W[uSpare+word]=C[word];}return;}
 if(uPass==22u){if(i<uWords)C[i]=B[i];if(i<uK+uKw)W[uRows+i]=0u;if(i<127u)W[uEdges+16u*i+15u]=0u;return;}
 if(uPass==23u){if(i==0u)replayQuantum();return;}
 if(uPass==30u){if(i==0u)m3PlanInitialize();return;}
 if(uPass==31u){if(i==0u)m3PackPublication();return;}
 if(uPass==32u){if(i==0u&&!m3AuditPublication())qfail(15u);return;}
 if(uPass==33u){if(i==0u&&!m3AuditInventory())qfail(15u);return;}
}
'''
out=ROOT/'.codex/local-test/t522-solver/occupied.glsl'
out.parent.mkdir(parents=True,exist_ok=True);out.write_text(text,encoding='utf-8')
