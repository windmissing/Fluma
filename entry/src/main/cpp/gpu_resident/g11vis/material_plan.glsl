// T639: pending rows are produced only by the real material writer hooks.
const uint G11_NONE=0xffffffffu,G11_PLAN_SEAL_VALUE=3235778818u;
const uint G11_PH=15008u,G11_SC=15200u,G11_CFG=15136u,G11_EXPECT=15328u,G11_EXPECT_SHA=15552u;
uint g11Header(uint w){return W[uVisControl+G11_PH+w];}
void g11HeaderPut(uint w,uint x){W[uVisControl+G11_PH+w]=x;}
uint g11RoleCounter(uint bank){return uVisControl+G11_SC+(bank==2u?64u:0u);}
uint g11PlanAt(uint slot){return uVisPlan+slot*8u;}
uint g11Ref(uint tag,uint index){return tag<<28u|index;}
uvec3 g11MapKey(uint table,uint c,uint rank){uint a=uVisMaps+((table-1u)*8192u+c*64u+rank)*4u;return uvec3(W[a],W[a+1u],W[a+2u]);}
uint g11MapMass(uint table,uint c,uint rank){return W[uVisMaps+((table-1u)*8192u+c*64u+rank)*4u+3u];}
uint g11FindMap(uint table,uint c,uvec3 key){for(uint r=0u;r<64u;++r)if(g11MapMass(table,c,r)>0u&&all(equal(key,g11MapKey(table,c,r))))return r;return 64u;}
uint g11MapCount(uint table,uint c){uint n=0u;for(uint r=0u;r<64u;++r)if(g11MapMass(table,c,r)>0u)n++;return n;}
uint g11MapTotal(uint table,uint c){uint n=0u;for(uint r=0u;r<64u;++r)n+=g11MapMass(table,c,r);return n;}
bool g11Fault(uint reason,uint entity,uint field){g9Fault(reason,entity,field);return false;}
bool g11Pending(){if(g11Header(0u)!=1345401159u||g11Header(44u)!=1u||g11Header(127u)!=0u||any(notEqual(uvec2(g11Header(6u),g11Header(7u)),uNonce)))return false;
 if(uRootKind==1u)return uOp==G9_OP_SOURCE_PRIVATE_BIRTH||uOp==G9_OP_SOURCE_AUDIT_REPLAY;
 return (uRole==2u||uRole==4u)&&g11Header(10u)==uRole&&g9ScopeEnabled()&&g9Ctl(14u)==0u;
}
bool g11PendingWrite(uint slot,uint type,uint owner,uint origin,uint oldRef,uint newRef,uint amount,uint range){if(slot>=65536u||amount==0u)return g11Fault(24u,slot,type);uint a=g11PlanAt(slot);if(W[a]!=0u)return g11Fault(24u,slot,0u);W[a]=type;W[a+1u]=owner;W[a+2u]=origin;W[a+3u]=oldRef;W[a+4u]=newRef;W[a+5u]=amount;W[a+6u]=range;W[a+7u]=0u;return true;}
bool g11TapCut(uint c,uint rank,uint amount){if(!g11Pending()||amount==0u)return true;uint phase=g11Header(12u);if(phase!=1u&&phase!=2u)return g11Fault(23u,c,phase);uint m=g11MapMass(1u,c,rank);if(amount>m)return g11Fault(24u,c,rank);uint ref=g11Ref(1u,c*64u+rank);return g11PendingWrite(32768u+c*64u+rank,7u,g11Ref(7u,c),ref,ref,G11_NONE,amount,m-amount);}
bool g11TapDonor(uint c,uint side,uint rank,uint share){if(!g11Pending()||share==0u)return true;if(g11Header(12u)!=3u||side>1u||rank>=64u||side==0u&&c==0u||side==1u&&c==127u)return g11Fault(24u,c,side);uint m=g11MapMass(3u,c,rank);if(share>m)return g11Fault(24u,c,rank);uint ref=g11Ref(3u,c*64u+rank);return g11PendingWrite(32768u+side*8192u+c*64u+rank,5u+side,g11Ref(7u,c),ref,ref,G11_NONE,share,side==0u?0u:m-share);}
bool g11TapDeposit(uint c,uint oldRank,uint newRank,uint amount,uint offset){if(!g11Pending()||amount==0u)return true;if(g11Header(12u)!=4u)return g11Fault(23u,c,4u);uint old=g11Ref(3u,c*64u+oldRank);return g11PendingWrite(32768u+c*64u+oldRank,8u,g11Ref(7u,c),old,old,g11Ref(2u,c*64u+newRank),amount,offset);}
bool g11TapPacketEvent(uint event,uint row[64]){if(!g11Pending())return true;uint type=row[2u]==G9_EVENT_LAUNCH?9u:row[2u]==G9_EVENT_CAPTURE?10u:row[2u]==G9_EVENT_SOURCE_BIRTH?11u:0u;if(type==0u)return true;
 uint phase=g11Header(12u),slot=row[11u],c=row[9u];if(slot>=uK||c>=128u||row[15u]==0u||(row[14u]&4u)==0u||event>=g9Ctl(6u)||type==9u&&phase!=1u||type==10u&&phase!=5u||type==11u&&phase!=6u)return g11Fault(24u,event,type);
 uint old=type==9u?G11_NONE:type==10u?g11Ref(5u,slot):G11_NONE;uint next=type==10u?G11_NONE:g11Ref(5u,slot);
 return g11PendingWrite(49152u+slot,type,g11Ref(7u,c),g11Ref(6u,event),old,next,row[15u],event);
}
void g11CancelTouchColumn(uint c){if(!g11Pending()||g11Header(12u)!=1u)return;for(uint r=0u;r<64u;++r)for(uint f=0u;f<8u;++f)W[g11PlanAt(32768u+c*64u+r)+f]=0u;for(uint slot=0u;slot<uK;++slot){uint a=g11PlanAt(49152u+slot);if(W[a+1u]==g11Ref(7u,c))for(uint f=0u;f<8u;++f)W[a+f]=0u;}}
uvec3 g11PacketKey(uint event,uint type){uint a=event*64u;return type==9u?uvec3(E[a+17u],E[a+18u],E[a+19u]):uvec3(E[a+16u],E[a+17u],E[a+18u]);}
uint g11SideShare(uint side,uint c,uvec3 key){uint r=g11FindMap(3u,c,key);if(r>=64u)return 0u;uint a=g11PlanAt(32768u+side*8192u+c*64u+r);return W[a]==5u+side?W[a+5u]:0u;}
uint g11Cuts(uint c,uvec3 key){uint q=0u;for(uint r=0u;r<64u;++r){uint a=g11PlanAt(32768u+c*64u+r);if(W[a]==7u&&all(equal(key,g11MapKey(1u,c,r))))q+=W[a+5u];}return q;}
uint g11PacketTransfer(uint c,uvec3 key,uint type){uint q=0u;for(uint slot=0u;slot<uK;++slot){uint a=g11PlanAt(49152u+slot);if(W[a]==type&&W[a+1u]==g11Ref(7u,c)&&all(equal(key,g11PacketKey(W[a+6u],type))))q+=W[a+5u];}return q;}
uint g11Deposits(uint c,uvec3 key){uint q=0u;for(uint r=0u;r<64u;++r){uint a=g11PlanAt(32768u+c*64u+r);if(W[a]==8u&&all(equal(key,g11MapKey(3u,c,r))))q+=W[a+5u];}return q;}
bool g11BuildMapRefs(){for(uint table=1u;table<=4u;++table)for(uint c=0u;c<128u;++c)for(uint r=0u;r<64u;++r){uint m=g11MapMass(table,c,r);if(m==0u)continue;uvec3 key=g11MapKey(table,c,r);uint peer=64u;
 if(table==1u||table==2u){uint other=table==1u?2u:1u;if(g11MapMass(other,c,r)>0u&&all(equal(key,g11MapKey(other,c,r))))peer=r;}else peer=g11FindMap(table==3u?4u:3u,c,key);
 uint own=g11Ref(table,c*64u+r),other=peer<64u?g11Ref(table==1u?2u:table==2u?1u:table==3u?4u:3u,c*64u+peer):G11_NONE;
 if(!g11PendingWrite((table-1u)*8192u+c*64u+r,table,g11Ref(7u,c),own,(table==1u||table==3u)?own:other,(table==1u||table==3u)?other:own,m,0u))return false;
 }return true;}
bool g11ResolveAndSeal(){uint count=0u,phase=g11Header(12u);if(g11Header(44u)!=1u||g9Ctl(6u)!=g9Ctl(7u)||g9Ctl(8u)!=g9Ctl(9u)||g9Ctl(10u)!=0u)return g11Fault(26u,uRole,44u);
 if(!g11BuildMapRefs())return false;
 for(uint slot=0u;slot<65536u;++slot){uint a=g11PlanAt(slot),type=W[a];if(type==0u){for(uint f=1u;f<8u;++f)if(W[a+f]!=0u)return g11Fault(26u,slot,f);continue;}uint c=W[a+1u]&0x0fffffffu,ref=W[a+2u],amount=W[a+5u];if(c>=128u||W[a+1u]!=g11Ref(7u,c)||amount==0u||W[a+7u]!=0u)return g11Fault(24u,slot,1u);
 if(slot<32768u){uint table=slot/8192u+1u,r=slot%64u;if(type!=table||ref!=g11Ref(table,c*64u+r)||slot%8192u/64u!=c||amount!=g11MapMass(table,c,r)||W[a+6u]!=0u)return g11Fault(24u,slot,2u);}
 else if(slot<49152u){uint side=(slot-32768u)/8192u,r=slot%64u;if(phase<1u||phase>4u)return g11Fault(23u,slot,phase);uint expected=phase==3u?5u+side:phase==4u?8u:7u;if(type!=expected||phase!=3u&&side!=0u||slot%8192u/64u!=c)return g11Fault(23u,slot,phase);
  uint oldTable=type==7u?1u:3u;uvec3 key=g11MapKey(oldTable,c,r);if(ref!=g11Ref(oldTable,c*64u+r)||W[a+3u]!=ref||g11MapMass(oldTable,c,r)<amount)return g11Fault(24u,slot,3u);
  if(type==8u){uint next=W[a+4u];uint nr=next&0x0fffffffu;if(next>>28u!=2u||nr/64u!=c||nr%64u<max(1u,g11MapCount(1u,c))-1u||any(notEqual(g11MapKey(2u,c,nr%64u),key))||W[a+6u]+amount>g11MapMass(2u,c,nr%64u))return g11Fault(24u,slot,4u);}
  else{if(type==5u&&c==0u||type==6u&&c==127u)return g11Fault(24u,c,type);uint dst=type==5u?c-1u:type==6u?c+1u:c,nr=g11FindMap(4u,dst,key);W[a+4u]=nr<64u?g11Ref(4u,dst*64u+nr):G11_NONE;if(type!=7u&&nr>=64u||W[a+6u]!=(type==5u?0u:g11MapMass(oldTable,c,r)-amount))return g11Fault(24u,slot,6u);}
 }else{if(slot>=49152u+uK||type<9u||type>11u||phase!=(type==9u?1u:type==10u?5u:6u))return g11Fault(23u,slot,phase);uint event=W[a+6u],e=event*64u;if(ref!=g11Ref(6u,event)||event>=g9Ctl(7u)||E[e+63u]!=G9_SEAL||E[e+11u]!=slot-49152u||E[e+15u]!=amount||any(notEqual(uvec2(E[e+4u],E[e+5u]),uNonce)))return g11Fault(26u,slot,event);
  uvec3 key=g11PacketKey(event,type);uint oldRank=g11FindMap(3u,c,key),newRank=g11FindMap(4u,c,key);if(type==9u)W[a+3u]=oldRank<64u?g11Ref(3u,c*64u+oldRank):G11_NONE;else if(type==10u){if(newRank>=64u)return g11Fault(24u,c,newRank);W[a+4u]=g11Ref(4u,c*64u+newRank);}
 }
 W[a+7u]=G11_PLAN_SEAL_VALUE;++count;
 }
 g11HeaderPut(14u,count);g11HeaderPut(44u,2u);memoryBarrierBuffer();g11HeaderPut(127u,3235778820u);return true;
}
bool g11AuditFlowKey(uint c,uvec3 key){uint orank=g11FindMap(3u,c,key),nrank=g11FindMap(4u,c,key);uint old=orank<64u?g11MapMass(3u,c,orank):0u;
 uint terms[8]=uint[8](c>0u?g11SideShare(1u,c-1u,key):0u,c<127u?g11SideShare(0u,c+1u,key):0u,g11Cuts(c,key),g11PacketTransfer(c,key,10u),g11SideShare(0u,c,key),g11SideShare(1u,c,key),g11Deposits(c,key),g11PacketTransfer(c,key,9u));
 uvec4 total=g9U32(old);for(uint i=0u;i<4u;++i)if(!g9UAdd(total,g9U32(terms[i]),total))return g11Fault(29u,c,i);
 for(uint i=4u;i<8u;++i)if(g9UCmp(total,g9U32(terms[i]))<0||!g9USub(total,g9U32(terms[i]),total))return g11Fault(24u,c,i);
 return all(equal(total,g9U32(nrank<64u?g11MapMass(4u,c,nrank):0u)))||g11Fault(24u,c,9u);
}
bool g11AuditTransfers(){for(uint c=0u;c<128u;++c){
 for(uint r=0u;r<64u;++r){uint old=g11MapMass(1u,c,r),next=g11MapMass(2u,c,r);if(old==0u&&next==0u)continue;uint expected=old,cut=g11PlanAt(32768u+c*64u+r);if(W[cut]==7u){if(W[cut+5u]>expected)return g11Fault(24u,c,r);expected-=W[cut+5u];}
  for(uint f=0u;f<64u;++f){uint a=g11PlanAt(32768u+c*64u+f);if(W[a]==8u&&W[a+4u]==g11Ref(2u,c*64u+r)){uint q=W[a+5u];if(q>268435456u-expected)return g11Fault(29u,c,r);expected+=q;}}
  if(expected!=next)return g11Fault(24u,c,r);
 }
 // Include cut and packet keys even when S->F->P leaves neither old nor new F.
 for(uint table=1u;table<=4u;++table)for(uint r=0u;r<64u;++r)if(g11MapMass(table,c,r)>0u&&!g11AuditFlowKey(c,g11MapKey(table,c,r)))return false;
 for(uint slot=0u;slot<uK;++slot){uint a=g11PlanAt(49152u+slot),type=W[a];if((type==9u||type==10u)&&W[a+1u]==g11Ref(7u,c)&&!g11AuditFlowKey(c,g11PacketKey(W[a+6u],type)))return false;}
 }return true;}
