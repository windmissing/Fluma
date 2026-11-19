// T639 exact rank transport and owner transitions. No inventory or physics writes.
uint g11MarkerBank(uint b){return b==0u?uVisMarkerCurrent:b==1u?uVisMarkerActual:uVisMarkerReplay;}
uint g11MarkerAt(uint b,uint slot){return g11MarkerBank(b)+slot*32u;}
uint g11Blend(){return W[uVisControl+G11_CFG+9u];}
bool g11Divide(uint a,uint b,uint d,out uint q){uvec4 p,r,v;if(d==0u||!g9UMul(g9U32(a),g9U32(b),p)||!g9UDiv(p,g9U32(d),v,r)||any(notEqual(v.yzw,uvec3(0u))))return false;q=v.x;return true;}
uvec3 g11MarkerKey(uint m){return uvec3(W[m+6u],W[m+7u],W[m+8u]);}
void g11KeyPut(uint m,uvec3 key){W[m+6u]=key.x;W[m+7u]=key.y;W[m+8u]=key.z;}
uvec2 g11PhaseTick(){if(uRootKind==1u||uVisPhase==1u)return uVisBaseTicks;uvec4 t;if(uJ==0u||256u%uJ!=0u||uMicro>=uJ||!g9UAdd(g9U64(uVisBaseTicks),g9U32((uMicro+1u)*(256u/uJ)),t)||any(notEqual(t.zw,uvec2(0u)))){g11Fault(29u,uMicro,uJ);return uvec2(0u);}return t.xy;}
bool g11XY(uint m,uint bed,uint flow,uint mapTable,out uvec2 point){uint kind=W[m],c=W[m+1u];point=uvec2(0u);if(kind==0u)return true;if(c>=128u)return g11Fault(24u,c,1u);
 if(kind==3u){uint slot=W[m+3u];if(slot>=uK||g9Packet(slot,0u)==0u||g9Packet(slot,6u)!=W[m+5u]||g9Packet(slot,7u)!=W[m+4u]||any(notEqual(uvec3(g9Packet(slot,8u),g9Packet(slot,9u),g9Packet(slot,10u)),g11MarkerKey(m))))return g11Fault(24u,slot,3u);point=uvec2(g9Packet(slot,1u)*16u,g9Packet(slot,2u)*4u);W[m+1u]=g9Packet(slot,1u)>>20u;return true;}
 uint fraction=0u;if(kind==1u){if(W[m+12u]==0u||!g11Divide(W[m+11u],16777216u,W[m+12u],fraction))return g11Fault(24u,c,12u);uint rank=W[m+2u],below=0u;if(rank>=64u||any(notEqual(g11MapKey(mapTable,c,rank),g11MarkerKey(m)))||W[m+9u]>=g11MapMass(mapTable,c,rank))return g11Fault(24u,c,rank);for(uint r=0u;r<rank;++r)below+=g11MapMass(mapTable,c,r);point=uvec2(c*16777216u+fraction,(below+W[m+9u])*8u+4u);return true;}
 uint mass=W[m+10u],rank=W[m+9u];if(kind!=2u||mass==0u||rank>=mass||flow==0u||!g11Divide(2u*rank+1u,8388608u,mass,fraction))return g11Fault(24u,c,9u);
 uint key=W[m+24u],samples=key>>16u,pair=key&65535u,band=min(flow,131072u),depth=0u;if(samples!=uVisV/128u||2u*pair+1u>=samples||!g11Divide(2u*pair+1u,2u*band-1u,samples,depth))return g11Fault(24u,c,24u);point=uvec2(c*16777216u+fraction,(2u*(bed+flow)-1u-depth)*4u);return true;
}
bool g11SetPoint(uint m,bool intermediate,uint mass){uint c=W[m+1u];uvec2 point;uint s=g11MapTotal(2u,c),f=g11MapTotal(4u,c);if(intermediate){f=g11MapTotal(3u,c);for(uint r=0u;r<64u;++r){uint a=g11PlanAt(32768u+c*64u+r);if(W[a]==7u)f+=W[a+5u];}}if(!g11XY(m,s,f,W[m]==1u?2u:4u,point))return false;W[m+15u]=point.x;W[m+16u]=point.y;if((W[m+22u]&1u)==0u){W[m+13u]=point.x;W[m+14u]=point.y;}return true;}
bool g11ChangeOwner(uint m,uint bank,uint next,uvec2 oldPoint){uint old=W[m];if(old==next)return true;uint counter=g11RoleCounter(bank);uvec2 tick=g11PhaseTick();if(g9Ctl(0u)!=0u)return false;uint flags=W[m+22u]&~(1u|2u|16u);
 if((W[m+22u]&1u)!=0u){uvec4 elapsed;if(!g9USub(g9U64(tick),g9U64(uvec2(W[m+17u],W[m+18u])),elapsed))return g11Fault(29u,m,17u);if(g9UCmp(elapsed,g9U32(W[m+19u]))<0){flags|=2u;if(atomicAdd(W[counter+3u],1u)==0xffffffffu)return g11Fault(29u,m,3u);}}
 W[m+29u]=old;W[m]=next;W[m+13u]=oldPoint.x;W[m+14u]=oldPoint.y;W[m+17u]=tick.x;W[m+18u]=tick.y;W[m+19u]=g11Blend();W[m+22u]=flags|(g11Blend()>0u?1u:0u);return true;}
bool g11Project(uint numerator,uint denominator,uint mass,out uint rank){if(mass==0u||numerator>=denominator||!g11Divide(numerator,mass,denominator,rank)||rank>=mass)return g11Fault(24u,mass,9u);return true;}
bool g11MoveFlow(uint m,uvec3 key){uint c=W[m+1u],r=g11FindMap(3u,c,key);if(r>=64u)return g11Fault(24u,c,2u);uint mass=g11MapMass(3u,c,r),rank=W[m+9u],a=g11SideShare(0u,c,key),b=g11SideShare(1u,c,key);if(W[m+10u]!=mass||rank>=mass||a>mass||b>mass-a)return g11Fault(24u,c,9u);
 uint dst=c,nextRank=0u;if(rank<a){if(c==0u)return g11Fault(24u,c,0u);dst=c-1u;uint peer=g11FindMap(3u,dst,key),old=peer<64u?g11MapMass(3u,dst,peer):0u,stay=old-g11SideShare(0u,dst,key)-g11SideShare(1u,dst,key);nextRank=(dst>0u?g11SideShare(1u,dst-1u,key):0u)+stay+rank;}
 else if(rank>=mass-b){if(c==127u)return g11Fault(24u,c,1u);dst=c+1u;nextRank=rank-(mass-b);}
 else nextRank=(c>0u?g11SideShare(1u,c-1u,key):0u)+rank-a;
 uint nr=g11FindMap(4u,dst,key);if(nr>=64u||nextRank>=g11MapMass(4u,dst,nr))return g11Fault(24u,dst,nextRank);W[m+1u]=dst;W[m+2u]=nr;W[m+9u]=nextRank;W[m+10u]=g11MapMass(4u,dst,nr);return true;
}
uint g11LaunchFor(uint c,uvec3 key){for(uint slot=0u;slot<uK;++slot){uint a=g11PlanAt(49152u+slot);if(W[a]==9u&&W[a+1u]==g11Ref(7u,c)&&all(equal(key,g11PacketKey(W[a+6u],9u))))return slot;}return uK;}
bool g11BindPacket(uint m,uint bank,uint slot,uint event,uint type){uint e=event*64u;uvec2 oldPoint=uvec2(W[m+15u],W[m+16u]);if(!g11ChangeOwner(m,bank,3u,oldPoint))return false;W[m+3u]=slot;W[m+4u]=E[e+13u];W[m+5u]=E[e+12u];W[m+10u]=E[e+15u];W[m+9u]=0u;W[m+2u]=0u;g11KeyPut(m,g11PacketKey(event,type));return true;}
bool g11RecruitMarker(uint m,uint bank){uint kind=W[m],c=W[m+1u];uvec3 key=g11MarkerKey(m);uint cuts=g11Cuts(c,key),oldFlowRank=g11FindMap(3u,c,key),oldMass=oldFlowRank<64u?g11MapMass(3u,c,oldFlowRank):0u;uint middle=oldMass+cuts,rank=W[m+9u];bool changed=false;
 if(kind==1u){uint br=W[m+2u],a=g11PlanAt(32768u+c*64u+br);if(W[a]!=7u||rank<W[a+6u])return true;if(rank>=W[a+6u]+W[a+5u])return g11Fault(24u,c,br);uvec2 oldPoint=uvec2(W[m+15u],W[m+16u]);if(!g11Project(W[m+11u],W[m+12u],middle,rank)||!g11ChangeOwner(m,bank,2u,oldPoint))return false;changed=true;}
 else if(kind==2u){if(oldMass==0u||W[m+10u]!=oldMass||rank>=oldMass)return g11Fault(24u,c,9u);if(cuts>0u){if(!g11Project(2u*rank+1u,2u*oldMass,middle,rank))return false;changed=true;}}else return true;
 if(changed){W[m+22u]|=8u;uint ctr=g11RoleCounter(bank);if(atomicAdd(W[ctr+5u],1u)==0xffffffffu)return g11Fault(29u,m,5u);}
 W[m+9u]=rank;W[m+10u]=middle;if(!g11SetPoint(m,true,middle))return false;
 uint slot=g11LaunchFor(c,key);if(slot<uK){uint a=g11PlanAt(49152u+slot),q=W[a+5u];if(q>middle)return g11Fault(24u,c,q);if(rank<q)return g11BindPacket(m,bank,slot,W[a+6u],9u);W[m+9u]=rank-q;}
 uint nr=g11FindMap(4u,c,key);if(nr>=64u||W[m+9u]>=g11MapMass(4u,c,nr))return g11Fault(24u,c,9u);W[m+2u]=nr;W[m+10u]=g11MapMass(4u,c,nr);return true;
}
bool g11DepositMarker(uint m,uint bank){if(W[m]!=2u)return true;uint c=W[m+1u],r=g11FindMap(3u,c,g11MarkerKey(m));if(r>=64u)return g11Fault(24u,c,2u);uint a=g11PlanAt(32768u+c*64u+r);if(W[a]!=8u)return true;uint oldMass=g11MapMass(3u,c,r),rank=W[m+9u];if(rank>=oldMass||W[m+10u]!=oldMass||oldMass>2147483647u)return g11Fault(24u,c,9u);
 uint key=W[m+24u],samples=key>>16u,pair=key&65535u,vertical=0u;if(!g11Divide(2u*pair+1u,W[a+5u],samples,vertical))return g11Fault(29u,c,24u);uvec2 oldPoint=uvec2(W[m+15u],W[m+16u]);if(!g11ChangeOwner(m,bank,1u,oldPoint))return false;W[m+11u]=2u*rank+1u;W[m+12u]=2u*oldMass;W[m+2u]=(W[a+4u]&0x0fffffffu)%64u;W[m+9u]=W[a+6u]+vertical;W[m+10u]=g11MapMass(2u,c,W[m+2u]);W[m+3u]=W[m+4u]=W[m+5u]=0u;return true;
}
bool g11CaptureMarker(uint m,uint bank){if(W[m]!=3u)return true;uint slot=W[m+3u];if(slot>=uK)return g11Fault(24u,slot,3u);uint a=g11PlanAt(49152u+slot);if(W[a]!=10u)return true;uint e=W[a+6u]*64u;if(E[e+12u]!=W[m+5u]||E[e+13u]!=W[m+4u])return g11Fault(24u,slot,4u);
 // T639 contract question: no field may impersonate persistent old-P mass.
 // Blend=0 is fully specified. Blend>0 cannot publish until its legal union is frozen.
 if(g11Blend()!=0u)return g11Fault(23u,slot,10u);
 uint c=W[a+1u]&0x0fffffffu,r=(W[a+4u]&0x0fffffffu)%64u,mass=g11MapMass(4u,c,r),rank=0u;uvec3 key=g11PacketKey(W[a+6u],10u);uint fraction=E[e+34u]&1048575u;if(!g11Project(fraction,1048576u,mass,rank))return false;
 uvec2 oldPoint=uvec2(E[e+34u]*16u,E[e+19u]*4u);if(!g11ChangeOwner(m,bank,2u,oldPoint))return false;W[m+1u]=c;W[m+2u]=r;W[m+9u]=rank;W[m+10u]=mass;g11KeyPut(m,key);W[m+3u]=W[m+4u]=W[m+5u]=0u;return true;
}
bool g11ApplyMarker(uint bank,uint slot){uint m=g11MarkerAt(bank,slot),kind=W[m];if(W[m+31u]!=3235778817u||W[m+30u]!=0u||kind>3u)return g11Fault(26u,slot,31u);if(kind==0u)return true;uint c=W[m+1u];if(c>=128u)return g11Fault(24u,slot,1u);
 uvec2 oldPoint;
 if(kind!=3u){if(!g11XY(m,g11MapTotal(1u,c),g11MapTotal(3u,c),1u,oldPoint))return false;W[m+15u]=oldPoint.x;W[m+16u]=oldPoint.y;}
 if(uVisPhase==1u||uVisPhase==2u){if(!g11RecruitMarker(m,bank))return false;}
 else if(uVisPhase==3u&&kind==2u){if(!g11MoveFlow(m,g11MarkerKey(m)))return false;}
 else if(uVisPhase==4u){if(!g11DepositMarker(m,bank))return false;}
 else if(uVisPhase==5u){if(kind==2u&&g11PacketTransfer(c,g11MarkerKey(m),10u)>0u){uint old=W[m+10u],next=g11FindMap(4u,c,g11MarkerKey(m)),rank=0u;if(next>=64u||old==0u||!g11Project(2u*W[m+9u]+1u,2u*old,g11MapMass(4u,c,next),rank))return false;W[m+9u]=rank;W[m+22u]|=8u;}if(!g11CaptureMarker(m,bank))return false;}
 if(W[m]==2u){uint nr=g11FindMap(4u,W[m+1u],g11MarkerKey(m));if(nr>=64u||W[m+9u]>=g11MapMass(4u,W[m+1u],nr))return g11Fault(24u,slot,9u);W[m+2u]=nr;W[m+10u]=g11MapMass(4u,W[m+1u],nr);}
 if(!g11SetPoint(m,false,0u))return false;W[m+25u]=uNonce.x;W[m+26u]=uNonce.y;W[m+27u]=g11Header(13u);W[m+28u]=uVisPhase;return true;
}
