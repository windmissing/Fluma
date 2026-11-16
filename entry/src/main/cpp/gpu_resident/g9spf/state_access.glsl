// Five complete banks are selected by the owner, four SSBOs per pass.
layout(std430,binding=0)readonly buffer ReadState{uint B[];};
layout(std430,binding=1)coherent buffer WriteState{uint C[];};
layout(std430,binding=2)coherent buffer Workspace{uint W[];};
layout(std430,binding=3)coherent buffer EventPath{uint E[];};
uniform uint uK,uStateWords,uColumn,uBed,uFlow,uPacket,uSources;
uniform uint uInput,uConfig,uControls,uSummary,uStageDiagnostic,uOriginAudit,uColumnStage,uHamilton,uContacts,uFaces,uWideLedger,uCompletion;
uniform uint uOp,uRole,uJslot,uMicro,uPass,uRequiredPasses,uEventCapacity,uPathCapacity,uPathWord;
uint uJ;
uniform uvec2 uNonce,uRun,uOwner,uContext,uSurface,uGeometry,uConfigId,uInputGeneration,uSession,uPlan,uBaseCommit;
uniform uint uCandidateBank,uBaseBank,uArena;
uniform uvec2 uArenaGeneration;
uniform int uAcceptedAngle,uAngleSin,uAngleCos;
uint g9Ctl(uint field){return W[uControls+field];}
void g9Set(uint field,uint value){W[uControls+field]=value;}
// Controls: 0 fatal,1 reason,2 stage,3 micro,4 entity,5 operand,
// 6 event planned,7 event sealed,8 path planned,9 path sealed,
// 10 compound pending,11 selectedJ,12 eligibleJ,13 selected trial,
// 14 current trial reason,15 current outcome,16 executed dispatch,
// 17 audit mask,18 source/impulse duplicate,19 stage cursor.
void g9Fault(uint reason,uint entity,uint operand){
 if(g9Ctl(0u)!=0u)return;g9Set(0u,1u);g9Set(1u,reason);g9Set(2u,uOp);g9Set(3u,uMicro);g9Set(4u,entity);g9Set(5u,operand);g9Set(15u,5u);
}
void g9Reject(uint reason,uint entity,uint operand){
 if(g9Ctl(0u)!=0u||g9Ctl(14u)!=0u)return;
 // A normal rejected private compound retains only the last sealed prefix.
 g9Set(6u,g9Ctl(7u));g9Set(8u,g9Ctl(9u));g9Set(10u,0u);
 g9Set(14u,reason);g9Set(2u,uOp);g9Set(3u,uMicro);g9Set(4u,entity);g9Set(5u,operand);
}
uint g9Column(uint c,uint field){return C[uColumn+c*32u+field];}
void g9PutColumn(uint c,uint field,uint value){C[uColumn+c*32u+field]=value;}
uint g9Packet(uint slot,uint field){return C[uPacket+slot*64u+field];}
void g9PutPacket(uint slot,uint field,uint value){C[uPacket+slot*64u+field]=value;}
uvec4 g9Momentum(uint c){return g9S64(uvec2(g9Column(c,2u),g9Column(c,3u)));}
bool g9PutMomentum(uint c,uvec4 p){uvec2 narrow;if(!g9I64(p,narrow)){g9Fault(29u,c,2u);return false;}g9PutColumn(c,2u,narrow.x);g9PutColumn(c,3u,narrow.y);g9PutColumn(c,4u,g9Zero(p)?0u:1u);return true;}
uvec4 g9ReadWide(uint at){return uvec4(W[at],W[at+1u],W[at+2u],W[at+3u]);}
void g9WriteWide(uint at,uvec4 value){for(uint i=0u;i<4u;++i)W[at+i]=value[i];}
uvec4 g9HeaderWide(uint at){return uvec4(C[at],C[at+1u],C[at+2u],C[at+3u]);}
bool g9Accumulate(uint header,uvec4 delta){uvec4 next;if(!g9SAdd(g9HeaderWide(header),delta,next)){g9Fault(29u,0u,header);return false;}for(uint i=0u;i<4u;++i)C[header+i]=next[i];return true;}
uint g9Material(uint c){return g9Column(c,0u)+g9Column(c,1u);}
uint g9Occupied(uint c){return g9Material(c)+g9Column(c,8u);}
void g9Derived(uint c){g9PutColumn(c,9u,g9Material(c));}
bool g9AddMass(uint a,uint b,out uint result){result=0u;if(a>268435456u||b>268435456u-a)return false;result=a+b;return true;}
bool g9Reserve(uint events,uint paths,out uint eventStart,out uint pathStart){
 eventStart=g9Ctl(6u);pathStart=g9Ctl(8u);
 if(g9Ctl(10u)!=0u||eventStart>uEventCapacity||events>uEventCapacity-eventStart||pathStart>uPathCapacity||paths>uPathCapacity-pathStart){g9Fault(27u,0u,0u);return false;}
 g9Set(6u,eventStart+events);g9Set(8u,pathStart+paths);g9Set(10u,1u);return true;
}
void g9Event(out uint row[64],uint kind,uint entity,uint from,uint to){
 for(uint i=0u;i<64u;++i)row[i]=0u;
 row[0]=G9_EVENT_MAGIC;row[1]=1u;row[2]=kind;row[3]=g9Ctl(13u);row[4]=uNonce.x;row[5]=uNonce.y;row[6]=g9Ctl(7u)+1u;row[7]=uOp==G9_OP_SOURCE_AUDIT_REPLAY?G9_OP_SOURCE_PRIVATE_BIRTH:uOp;row[8]=uMicro;
 row[9]=from;row[10]=to;row[11]=entity;row[14]=(uRole==4u?2u:uRole==2u?1u:0u);row[63]=G9_SEAL;
}
bool g9WriteEvent(uint at,uint row[64]){
 if(at>=g9Ctl(6u)){g9Fault(27u,0u,at);return false;}
 uvec2 mask=g9EventMask(row[2]);if(all(equal(mask,uvec2(0u)))){g9Fault(23u,0u,row[2]);return false;}
 if(uOp==G9_OP_SOURCE_AUDIT_REPLAY){
  for(uint i=0u;i<64u;++i){uint expected=((mask[i/32u]>>(i%32u))&1u)!=0u?row[i]:0u;if(E[at*64u+i]!=expected){g9Fault(25u,at,i);return false;}}return true;
 }
 for(uint i=0u;i<63u;++i)E[at*64u+i]=(((mask[i/32u]>>(i%32u))&1u)!=0u?row[i]:0u);
 memoryBarrierBuffer();E[at*64u+63u]=G9_SEAL;return true;
}
bool g9Seal(){
 if(g9Ctl(0u)!=0u||g9Ctl(14u)!=0u||g9Ctl(10u)==0u)return false;
 g9Set(7u,g9Ctl(6u));g9Set(9u,g9Ctl(8u));g9Set(10u,0u);return true;
}
bool g9OneEvent(uint row[64]){uint event,path;return g9Reserve(1u,0u,event,path)&&g9WriteEvent(event,row)&&g9Seal();}
