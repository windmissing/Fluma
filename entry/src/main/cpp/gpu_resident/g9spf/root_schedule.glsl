uint g9TrialIndex(uint j){return uint(findMSB(j));}
bool g9ScopeEnabled(){
 uint selected=g9Ctl(11u);
 if(uRole==1u)return selected==0u||selected==uJslot&&g9Ctl(21u)==0u;
 if(uRole==2u||uRole==4u)return selected!=0u;
 if(uRole==3u)return selected==0u||uJslot<selected;
 return true;
}
bool g9IsMicro(){return uMicro!=0xffffffffu;}
void g9ClearTrial(){for(uint i=gl_LocalInvocationIndex;i<uEventCapacity*64u+uPathCapacity*16u;i+=64u)E[i]=0u;memoryBarrierBuffer();barrier();}
void g9CopyBase(){for(uint i=gl_LocalInvocationIndex;i<uStateWords;i+=64u)C[i]=B[i];memoryBarrierBuffer();barrier();}
bool g9GravityWork(){
 int oldGx=int(B[22u]);uint oldGn=B[23u];uvec4 gxWide,gnWide;
 if(uAcceptedAngle< -7680||uAcceptedAngle>7680||uAngleCos<=0||uAngleCos>1048576||uAngleSin< -1048576||uAngleSin>1048576||
    !g9RneProduct(g9U32(W[uConfig+7u]),g9S32(uAngleSin),1048576u,gxWide)||!g9RneProduct(g9U32(W[uConfig+7u]),g9S32(uAngleCos),1048576u,gnWide)){g9Fault(29u,0u,22u);return false;}
 int gx;uint gn;if(!g9I32(gxWide,gx)||!g9Uint(gnWide,gn)||gn==0u){g9Fault(29u,0u,23u);return false;}
 if(oldGx==gx&&oldGn==gn){C[22u]=uint(gx);C[23u]=gn;C[20u]=uint(uAcceptedAngle);return true;}
 C[22u]=uint(oldGx);C[23u]=oldGn;
 uvec4 before=uvec4(0u),after=uvec4(0u),work;
 // Momentum is unchanged by an external-field change. Its rational kinetic
 // term cancels symbolically, leaving exact potential differences.
 for(uint c=0u;c<128u;++c){uvec4 p;if(!g9ColumnPotential(c,g9Column(c,0u),g9Column(c,1u),p)||!g9SAdd(before,p,before)){g9Fault(29u,c,0u);return false;}}
 for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u){uvec4 p;if(!g9PacketEnergy(g9Packet(slot,0u),g9Packet(slot,1u),g9Packet(slot,2u),0,0,p)||!g9SAdd(before,p,before)){g9Fault(29u,slot,1u);return false;}}
 C[22u]=uint(gx);C[23u]=gn;C[20u]=uint(uAcceptedAngle);
 for(uint c=0u;c<128u;++c){uvec4 p;if(!g9ColumnPotential(c,g9Column(c,0u),g9Column(c,1u),p)||!g9SAdd(after,p,after)){g9Fault(29u,c,2u);return false;}}
 for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u){uvec4 p;if(!g9PacketEnergy(g9Packet(slot,0u),g9Packet(slot,1u),g9Packet(slot,2u),0,0,p)||!g9SAdd(after,p,after)){g9Fault(29u,slot,3u);return false;}}
 if(!g9SSub(after,before,work)||!g9Accumulate(82u,work)){g9Fault(29u,0u,82u);return false;}
 uint row[64];g9Event(row,G9_EVENT_GRAVITY_WORK,0u,0u,0u);row[15]=B[20u];row[16]=uint(uAcceptedAngle);for(uint i=0u;i<4u;++i)row[17u+i]=work[i];return g9OneEvent(row);
}
bool g9ConfigValid(){
 return W[uConfig]<=262144u&&W[uConfig+1u]<=W[uConfig]&&W[uConfig+2u]>0u&&W[uConfig+2u]<=268435456u&&W[uConfig+3u]<=62914560u&&W[uConfig+4u]<=262144u&&W[uConfig+5u]>0u&&W[uConfig+6u]>0u&&W[uConfig+7u]>=120u&&W[uConfig+7u]<=1073741824u;
}
bool g9RootEnd(uint outcome){
 uint row[64];g9Event(row,G9_EVENT_ROOT_END,0u,0u,0u);row[15]=outcome;row[16]=uJ;row[17]=g9Ctl(7u)+1u;row[18]=g9Ctl(9u);row[19]=C[12u];row[23]=65535u;
 uvec4 work=uvec4(0u),loss,part;
 const uint ledgers[3]=uint[3](82u,86u,90u);
 for(uint k=0u;k<3u;++k){uint at=ledgers[k];if(!g9SSub(uvec4(C[at],C[at+1u],C[at+2u],C[at+3u]),uvec4(B[at],B[at+1u],B[at+2u],B[at+3u]),part)||!g9SAdd(work,part,work)){g9Fault(29u,0u,at);return false;}}
 if(!g9SSub(uvec4(C[94u],C[95u],C[96u],C[97u]),uvec4(B[94u],B[95u],B[96u],B[97u]),loss)){g9Fault(29u,0u,94u);return false;}
 for(uint i=0u;i<4u;++i){row[24u+i]=work[i];row[28u+i]=loss[i];}
 return g9OneEvent(row);
}
bool g9SourceTransaction(){
 uint row[64];g9Event(row,G9_EVENT_ROOT_BEGIN,0u,0u,0u);row[15]=0u;row[16]=C[20u];row[17]=C[20u];row[18]=g9Input(14u);row[19]=uBaseCommit.x;row[20]=uBaseCommit.y;
 if(!g9OneEvent(row)||!g9SourceBirth())return false;
 return g9RootEnd(g9Ctl(18u)!=0u?2u:g9Ctl(49u)==0u?3u:1u);
}
void g9BeginScope(){
 if(gl_LocalInvocationIndex!=0u)return;
 g9Set(14u,0u);g9Set(6u,0u);g9Set(7u,0u);g9Set(8u,0u);g9Set(9u,0u);g9Set(10u,0u);g9Set(56u,0u);g9Set(13u,uRole==2u||uRole==4u?g9Ctl(20u):g9TrialIndex(uJ));
 for(uint i=2u;i<6u;++i)g9Set(i,0u);
 for(uint c=0u;c<128u;++c)W[uColumnStage+c*64u+21u]=0u;
 uint row[64];g9Event(row,G9_EVENT_ROOT_BEGIN,0u,0u,0u);row[15]=uJ;row[16]=C[20u];row[17]=uint(uAcceptedAngle);row[18]=g9Input(14u);row[19]=uBaseCommit.x;row[20]=uBaseCommit.y;g9OneEvent(row);
}
void g9ScopeSeal(){
 uint reason=g9Ctl(14u),summary=uSummary+g9TrialIndex(uJ)*64u;
 if(uRole==1u){
  W[summary]=uJ;W[summary+1u]=reason==0u?1u:4u;W[summary+2u]=reason;W[summary+3u]=reason==0u?0u:g9Ctl(2u);W[summary+4u]=reason==0u?0u:g9Ctl(3u);W[summary+5u]=reason==0u?0u:g9Ctl(4u);W[summary+6u]=reason==0u?0u:g9Ctl(5u);W[summary+7u]=g9Ctl(7u);W[summary+8u]=g9Ctl(9u);
  if(reason==0u&&g9Ctl(11u)==0u){g9Set(11u,uJ);g9Set(20u,g9TrialIndex(uJ));g9Set(22u,g9Ctl(7u));g9Set(23u,g9Ctl(9u));g9Set(21u,1u);}
 }else if(uRole==3u){
  if(reason==0u||W[summary+1u]!=4u||W[summary+2u]!=reason||W[summary+3u]!=g9Ctl(2u)||W[summary+4u]!=g9Ctl(3u)||W[summary+5u]!=g9Ctl(4u)||W[summary+6u]!=g9Ctl(5u)){g9Fault(25u,0u,summary);return;}
 }else if(reason!=0u)g9Fault(25u,g9Ctl(4u),g9Ctl(5u));
}
// These compare passes have explicit alternative binding descriptors. The
// native owner binds two streams (B/C) + WORK + BASE, or two states + WORK +
// event pair. They never require a fifth SSBO.
bool g9CompareStream(){
 uint events=g9Ctl(22u),paths=g9Ctl(23u);if(events!=g9Ctl(7u)||paths!=g9Ctl(9u)){g9Fault(25u,0u,0u);return false;}
 for(uint row=0u;row<events;++row){uint at=row*64u;uvec2 mask=g9EventMask(B[at+2u]);
  if(all(equal(mask,uvec2(0u)))||B[at+63u]!=G9_SEAL||C[at+63u]!=G9_SEAL){g9Fault(26u,row,63u);return false;}
  uint expectedRole=B[at+14u]&3u,actualRole=C[at+14u]&3u;if(expectedRole!=0u||actualRole!=(uRole==2u?1u:2u)){g9Fault(25u,row,14u);return false;}
  for(uint word=0u;word<64u;++word){uint a=B[at+word],b=C[at+word];if(word==14u){a&=~3u;b&=~3u;}if(((mask[word/32u]>>(word%32u))&1u)==0u&&(a!=0u||b!=0u)||a!=b){g9Fault(25u,row,word);return false;}}
 }
 for(uint i=0u;i<paths*16u;++i)if(B[uPathWord+i]!=C[uPathWord+i]){g9Fault(25u,i/16u,i%16u);return false;}
 g9Set(uRole==2u?24u:25u,1u);return true;
}
bool g9CompareState(){for(uint i=0u;i<uStateWords;++i)if(B[i]!=C[i]){g9Fault(25u,i,0u);return false;}return true;}
void g9EndAcceptedMetadata(uint outcome){
 if(outcome==2u)return;
 uvec4 commit;if(!g9UAdd(g9U64(uBaseCommit),g9U32(1u),commit)||any(notEqual(commit.zw,uvec2(0u)))){g9Fault(29u,0u,10u);return;}
 C[10u]=commit.x;C[11u]=commit.y;C[16u]=uNonce.x;C[17u]=uNonce.y;
 if(uRequiredPasses==2024u){uvec4 quanta;if(!g9UAdd(g9U64(uvec2(C[12u],C[13u])),g9U32(1u),quanta)||any(notEqual(quanta.zw,uvec2(0u)))){g9Fault(29u,0u,12u);return;}C[12u]=quanta.x;C[13u]=quanta.y;}
}
void g9Completion(){
 for(uint i=0u;i<256u;++i)W[uCompletion+i]=0u;
 uint outcome=g9Ctl(0u)!=0u?5u:uRequiredPasses==12u?(g9Ctl(18u)!=0u?2u:g9Ctl(49u)==0u?3u:1u):g9Ctl(11u)==0u?4u:1u;
 W[uCompletion]=G9_COMPLETION_MAGIC;W[uCompletion+1u]=1u;W[uCompletion+2u]=256u;W[uCompletion+4u]=G9_PROFILE;W[uCompletion+5u]=uRequiredPasses==12u?1u:2u;W[uCompletion+6u]=outcome;W[uCompletion+7u]=g9Ctl(1u)!=0u?g9Ctl(1u):outcome==4u?g9Ctl(14u):0u;
 uvec2 ids[11]=uvec2[11](uNonce,uRun,uOwner,uContext,uSurface,uGeometry,uConfigId,uInputGeneration,uSession,uPlan,uBaseCommit);
 for(uint i=0u;i<11u;++i){W[uCompletion+8u+2u*i]=ids[i].x;W[uCompletion+9u+2u*i]=ids[i].y;}
 W[uCompletion+30u]=C[10u];W[uCompletion+31u]=C[11u];W[uCompletion+32u]=uBaseBank;W[uCompletion+33u]=uCandidateBank;W[uCompletion+34u]=uArena;W[uCompletion+35u]=uArenaGeneration.x;W[uCompletion+36u]=uArenaGeneration.y;
 W[uCompletion+37u]=g9Ctl(11u);W[uCompletion+38u]=g9Ctl(12u);W[uCompletion+39u]=g9Ctl(16u);W[uCompletion+40u]=uRequiredPasses;W[uCompletion+41u]=uRequiredPasses;
 W[uCompletion+42u]=g9Ctl(17u);W[uCompletion+43u]=g9Ctl(6u);W[uCompletion+44u]=g9Ctl(7u);W[uCompletion+45u]=g9Ctl(8u);W[uCompletion+46u]=g9Ctl(9u);
 uint valid=0u;for(uint i=0u;i<6u;++i)if(W[uSummary+i*64u]!=0u)valid|=1u<<i;W[uCompletion+47u]=valid;
 for(uint i=0u;i<4u;++i)W[uCompletion+48u+i]=g9Ctl(2u+i);
 W[uCompletion+54u]=C[12u];W[uCompletion+55u]=C[13u];W[uCompletion+56u]=C[40u];W[uCompletion+57u]=C[41u];W[uCompletion+58u]=C[28u];W[uCompletion+59u]=C[29u];W[uCompletion+60u]=C[30u];W[uCompletion+61u]=C[31u];W[uCompletion+62u]=uRequiredPasses==12u?g9Ctl(49u):0u;
 for(uint i=0u;i<8u;++i){W[uCompletion+64u+i]=W[uConfig+16u+i];W[uCompletion+72u+i]=W[uConfig+24u+i];W[uCompletion+80u+i]=W[uConfig+32u+i];W[uCompletion+88u+i]=W[uConfig+40u+i];}
 uvec4 px,py;if(g9MomentumTotal(false,px,py))for(uint i=0u;i<4u;++i){W[uCompletion+96u+i]=px[i];W[uCompletion+100u+i]=py[i];}
 const uint ledger[4]=uint[4](82u,86u,90u,94u);for(uint category=0u;category<4u;++category)for(uint i=0u;i<4u;++i)W[uCompletion+104u+category*4u+i]=C[ledger[category]+i];
 for(uint i=0u;i<6u;++i){uint at=uSummary+i*64u,dst=uCompletion+120u+i*6u;W[dst]=W[at];W[dst+1u]=W[at+1u];for(uint f=0u;f<4u;++f)W[dst+2u+f]=W[at+3u+f];}
 memoryBarrierBuffer();W[uCompletion+3u]=G9_SEAL;
}
void g9Run(){
 uint lane=gl_LocalInvocationIndex;
 // R2 cold initialization only: second dispatch, before CURRENT eligibility.
 // Uses the existing four bindings and private controls; no root pass added.
 if(uOp==0x7fffff00u){
  if(lane==0u){bool valid=all(equal(uvec2(C[4u],C[5u]),uRun))&&all(equal(uvec2(C[6u],C[7u]),uOwner))&&all(equal(uvec2(C[8u],C[9u]),uContext))&&all(equal(uvec2(C[14u],C[15u]),uInputGeneration));
   if(!valid||!g9AuditMassGeometry()||!g9AuditMomentum())g9Fault(24u,0u,0u);else W[uControls+127u]=0xc01d0002u;
  }return;
 }
 if(uOp==G9_OP_ROOT_INIT||uOp==G9_OP_SOURCE_INIT_BASE){
  for(uint i=lane;i<128u;i+=64u)W[uControls+i]=0u;for(uint i=lane;i<384u;i+=64u)W[uSummary+i]=0u;memoryBarrierBuffer();barrier();if(lane==0u)g9Set(16u,1u);return;
 }
 if(lane==0u){if(g9Ctl(16u)!=uPass)g9Fault(25u,0u,uPass);g9Set(16u,g9Ctl(16u)+1u);}memoryBarrierBuffer();barrier();
 uJ=(uRole==2u||uRole==4u)?g9Ctl(11u):uJslot;
 if(uOp==G9_OP_ROOT_SEAL||uOp==G9_OP_SOURCE_SEAL){if(lane==0u)g9Completion();return;}
 if(g9Ctl(0u)!=0u)return;
 if(uRole!=0u&&!g9ScopeEnabled())return;
 if(g9IsMicro()&&(uMicro>=uJ||g9Ctl(14u)!=0u))return;
 if(uOp==G9_OP_INIT_COPY){g9CopyBase();g9ClearTrial();g9BeginScope();return;}
 if(uOp==G9_OP_SOURCE_AUDIT_REPLAY){
  g9CopyBase();if(lane==0u){uint events=g9Ctl(7u);g9Set(6u,0u);g9Set(7u,0u);g9Set(8u,0u);g9Set(9u,0u);g9Set(10u,0u);
   if(!g9Dedup()||!g9SourceDecode()||!g9SourceTransaction())return;uint outcome=g9Ctl(18u)!=0u?2u:g9Ctl(49u)==0u?3u:1u;
   if(!g9ConsumeCommand(outcome,g9Ctl(49u)))return;g9EndAcceptedMetadata(outcome);
   if(g9Ctl(7u)!=events||!g9AuditMassGeometry()||!g9AuditMomentum()||!g9AuditStream())g9Fault(25u,0u,0u);
  }return;
 }
 if(uOp==G9_OP_BASE_FREEZE||uOp==G9_OP_SOURCE_VALIDATE_INPUT){g9CopyBase();if(uOp==G9_OP_SOURCE_VALIDATE_INPUT)g9ClearTrial();if(lane==0u&&(!g9ConfigValid()||!g9ValidateInput(uRequiredPasses==12u?1u:2u)||!g9AuditMassGeometry()))g9Fault(22u,0u,0u);return;}
 if(uOp==G9_OP_FLIGHT_PATH){g9Flight();return;}
 if(lane!=0u)return;
 if(uOp==G9_OP_CONFIG_INPUT_GATE||uOp==G9_OP_SOURCE_DEDUP){if(!g9Dedup())return;}
 else if(uOp==G9_OP_INPUT_AND_CFL){
  uvec4 gx,gn;int x;uint n;if(!g9RneProduct(g9U32(W[uConfig+7u]),g9S32(uAngleSin),1048576u,gx)||!g9RneProduct(g9U32(W[uConfig+7u]),g9S32(uAngleCos),1048576u,gn)||!g9I32(gx,x)||!g9Uint(gn,n)||n==0u){g9Fault(29u,0u,22u);return;}C[22u]=uint(x);C[23u]=n;C[20u]=uint(uAcceptedAngle);
  bool eligible=g9Cfl();if(eligible&&(g9Ctl(12u)==0u||uJ<g9Ctl(12u)))g9Set(12u,uJ);
 }
 else if(uOp==G9_OP_GRAVITY_WORK){if(g9Ctl(14u)==0u)g9GravityWork();}
 else if(uOp==G9_OP_TOUCH_ATOMIC){if(g9Ctl(14u)==0u)g9Touch();}
 else if(uOp==G9_OP_AFTER_TOUCH_CFL){if(g9Ctl(14u)==0u)g9Cfl();}
 else if(uOp==G9_OP_ACTIVATE_REQUEST){if(g9Cfl()&&g9SnapshotMicro())g9ActivationRequest();}
 else if(uOp==G9_OP_ACTIVATE_APPLY)g9ActivationApply();

 else if(uOp==G9_OP_PARTICLE_FOLD)g9Fold();
 else if(uOp==G9_OP_FACE_FLUX_MOBILITY)g9FaceFlux();
 else if(uOp==G9_OP_FLOW_ORIGIN_RESERVE)g9FlowReserve();
 else if(uOp==G9_OP_FLOW_PAIR_APPLY)g9FlowApply();
 else if(uOp==G9_OP_FRICTION_REST_DEPOSIT)g9Friction();
 else if(uOp==G9_OP_ORDERED_CAPTURE)g9Capture();
 else if(uOp==G9_OP_MICRO_AUDIT_SEAL){
  if(!g9Fold()||!g9Cfl()||!g9AuditMassGeometry()||!g9AuditMomentum()){if(g9Ctl(14u)==0u)g9Fault(24u,0u,0u);return;}if(!g9AuditMicroEnergy())return;
  uint row[64];g9Event(row,G9_EVENT_STEP_SEAL,0u,0u,0u);row[15]=uJ;row[16]=32u/uJ;row[17]=g9Ctl(7u);row[18]=g9Ctl(9u);uvec4 delta=g9ReadWide(uControls+40u),loss;if(!g9SNeg(delta,loss)){g9Fault(29u,0u,0u);return;}for(uint i=0u;i<4u;++i){row[19u+i]=delta[i];row[23u+i]=loss[i];}g9OneEvent(row);
 }
 else if(uOp==G9_OP_FINAL_MASS_ORIGIN_MOMENTUM){if(g9Ctl(14u)==0u){if(!g9AuditMassGeometry()||!g9AuditMomentum())g9Fault(24u,0u,0u);else g9RootEnd(1u);}}
 else if(uOp==G9_OP_FINAL_ENERGY_CURSORS){if(g9Ctl(14u)==0u){if(uRole==2u||uRole==4u)g9CompareStream();else if(!g9AuditStream())g9Fault(26u,0u,0u);}}
 else if(uOp==G9_OP_SCOPE_SEAL_AND_CHOICE){
  if(uRole==4u&&g9Ctl(14u)==0u&&!g9CompareState())return;g9ScopeSeal();
 }
 else if(uOp==G9_OP_COMPARE_SELECTED_AUTHORITATIVE){if(g9Ctl(11u)!=0u)g9CompareState();}
 else if(uOp==G9_OP_COMPARE_REPLAY_STREAM){if(g9Ctl(11u)!=0u&&(g9Ctl(24u)!=1u||g9Ctl(25u)!=1u))g9Fault(25u,0u,0u);}
 else if(uOp==G9_OP_PACK_SCENE_METADATA){
  if(g9Ctl(11u)!=0u){if(!g9ConsumeCommand(1u,0u))return;g9EndAcceptedMetadata(1u);g9Set(17u,65535u);}
  else g9Set(17u,65535u);
 }
 else if(uOp==G9_OP_SOURCE_DECODE)g9SourceDecode();
 else if(uOp==G9_OP_SOURCE_PRIVATE_BIRTH)g9SourceTransaction();
 else if(uOp==G9_OP_SOURCE_LEDGER){uint outcome=g9Ctl(18u)!=0u?2u:g9Ctl(49u)==0u?3u:1u;if(g9ConsumeCommand(outcome,g9Ctl(49u)))g9EndAcceptedMetadata(outcome);}
 else if(uOp==G9_OP_SOURCE_ORIGIN_CERTIFICATE){if(!g9AuditMassGeometry()||!g9AuditMomentum()||!g9AuditStream())g9Fault(24u,0u,0u);}
 else if(uOp==G9_OP_SOURCE_COMPARE)g9CompareState();
 else if(uOp==G9_OP_SOURCE_PACK_SCENE)g9Set(17u,65535u);
}
void main(){g9Run();}
