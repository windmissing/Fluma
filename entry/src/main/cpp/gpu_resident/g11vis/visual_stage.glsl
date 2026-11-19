// T639/T647 composite-only GPU state. No parent inventory write in VIS functions.
#ifndef G11VIS_STAGE_GLSL
#define G11VIS_STAGE_GLSL
uniform uint uVisMarkerCurrent,uVisMarkerActual,uVisMarkerReplay,uVisGlyph,uVisPlan,uVisControl,uVisMaps,uVisV;
uniform uint uVisOldBank,uVisNewBank,uVisPhase,uVisOrdinal;
uniform uvec2 uVisTicks,uVisBaseTicks,uVisBaseGeneration;
const uint G11_COLD_INIT=0x7fffff01u;
/* g11vis/material_plan.glsl */
/* g11vis/marker_map.glsl */
uint g11Swap(uint x){return x>>24u|(x>>8u&65280u)|(x<<8u&16711680u)|x<<24u;}
uint g11Ror(uint x,uint n){return x>>n|x<<(32u-n);}
void g11Hash(uint start,uint count,out uint result[8]){
 uint h[8]=uint[8](0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u);
 const uint k[64]=uint[64](0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u);
 uint blocks=(count+18u)/16u;for(uint block=0u;block<blocks;++block){uint schedule[64];for(uint t=0u;t<16u;++t){uint at=block*16u+t;schedule[t]=at<count?g11Swap(W[start+at]):at==count?0x80000000u:0u;if(block==blocks-1u&&t==15u)schedule[t]=count*32u;}
 for(uint t=16u;t<64u;++t){uint x=schedule[t-15u],y=schedule[t-2u];schedule[t]=(g11Ror(x,7u)^g11Ror(x,18u)^(x>>3u))+schedule[t-16u]+(g11Ror(y,17u)^g11Ror(y,19u)^(y>>10u))+schedule[t-7u];}
 uint a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];for(uint t=0u;t<64u;++t){uint z=hh+(g11Ror(e,6u)^g11Ror(e,11u)^g11Ror(e,25u))+((e&f)^((~e)&g))+k[t]+schedule[t],y=(g11Ror(a,2u)^g11Ror(a,13u)^g11Ror(a,22u))+((a&b)^(a&c)^(b&c));hh=g;g=f;f=e;e=d+z;d=c;c=b;b=a;a=z+y;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;}
 for(uint i=0u;i<8u;++i)result[i]=g11Swap(h[i]);
}
bool g11ConfigValid(){uint a=uVisControl+G11_CFG,e=uVisControl+G11_EXPECT;if(W[a]!=1127297351u||W[a+1u]!=1u||W[a+2u]!=W[e+6u]||W[a+3u]!=W[e+7u]||W[a+4u]!=uK||W[a+5u]!=uVisV||W[a+6u]!=32u||W[a+7u]!=8u||W[a+8u]!=65536u||(W[a+9u]!=0u&&W[a+9u]!=256u)||W[a+10u]!=131072u||W[a+11u]!=32768u||W[a+12u]!=1u||W[a+13u]!=1u||W[a+14u]!=0u||W[a+15u]!=1u)return false;
 for(uint i=16u;i<20u;++i)if(W[a+i]!=1u)return false;for(uint i=44u;i<64u;++i)if(W[a+i]!=0u)return false;uint digest[8];g11Hash(a,64u,digest);for(uint i=0u;i<8u;++i)if(digest[i]!=W[e+128u+i]||W[a+20u+i]!=W[e+72u+i]||W[a+36u+i]!=W[e+136u+i])return false;return true;}
uint g11BankHeader(uint bank,uint field){return W[uVisControl+bank*128u+field];}
void g11BankPut(uint bank,uint field,uint value){W[uVisControl+bank*128u+field]=value;}
bool g11NextId(uint bank,out uvec2 id){id=uvec2(g11BankHeader(bank,10u),g11BankHeader(bank,11u));uvec4 next;if(all(equal(id,uvec2(0u)))||!g9UAdd(g9U64(id),g9U32(1u),next)||any(notEqual(next.zw,uvec2(0u))))return g11Fault(29u,bank,10u);g11BankPut(bank,10u,next.x);g11BankPut(bank,11u,next.y);uint sc=g11RoleCounter(bank);W[sc+1u]=next.x;W[sc+2u]=next.y;return true;}
bool g11Sample(uint bank,uint slot,uint c){uint m=g11MarkerAt(bank,slot),samples=uVisV/128u,j=slot%samples,pair=min(j,samples-1u-j),wanted=0u,total=g9Occupied(c);if(total==0u)return true;if(!g11Divide(2u*pair+1u,total,samples,wanted))return g11Fault(29u,c,24u);
 uint kind=0u,rank=0u,row=0u,mass=0u,packet=uK;uvec3 key=uvec3(0u);
 for(uint r=0u;r<g9MapCount(false,c)&&kind==0u;++r){uint a=g9MapAddress(false,c,r),q=C[a+3u];if(wanted<q){kind=1u;rank=wanted;row=r;mass=q;key=g9Key(false,c,r);}else wanted-=q;}
 for(uint r=0u;r<g9MapCount(true,c)&&kind==0u;++r){uint a=g9MapAddress(true,c,r),q=C[a+3u];if(wanted<q){kind=2u;row=r;mass=q;key=g9Key(true,c,r);if(!g11Project(2u*j+1u,2u*samples,mass,rank))return false;}else wanted-=q;}
 for(uint p=0u;p<uK&&kind==0u;++p){uint q=g9Packet(p,0u);if(q==0u||(g9Packet(p,1u)>>20u)!=c)continue;if(wanted<q){kind=3u;packet=p;mass=q;key=uvec3(g9Packet(p,8u),g9Packet(p,9u),g9Packet(p,10u));}else wanted-=q;}
 if(kind==0u)return g11Fault(24u,c,wanted);uint generation=W[m+23u];if(generation==0xffffffffu)return g11Fault(29u,slot,23u);uvec2 id;if(!g11NextId(bank,id))return false;for(uint f=0u;f<32u;++f)W[m+f]=0u;W[m]=kind;W[m+1u]=c;W[m+2u]=row;W[m+9u]=rank;W[m+10u]=mass;W[m+11u]=2u*j+1u;W[m+12u]=2u*samples;W[m+20u]=id.x;W[m+21u]=id.y;W[m+23u]=generation+1u;W[m+24u]=(samples<<16u)|pair;W[m+31u]=3235778817u;g11KeyPut(m,key);
 if(kind==3u){W[m+3u]=packet;W[m+4u]=g9Packet(packet,7u);W[m+5u]=g9Packet(packet,6u);}
 uvec2 point;
 if(kind==1u){uint below=0u;for(uint r=0u;r<row;++r)below+=C[g9MapAddress(false,c,r)+3u];point=uvec2(c*16777216u+(2u*j+1u)*(16777216u/(2u*samples)),(below+rank)*8u+4u);}
 else if(kind==2u){uint x=0u,depth=0u,band=min(g9Column(c,1u),131072u);if(!g11Divide(2u*rank+1u,8388608u,mass,x)||!g11Divide(2u*pair+1u,2u*band-1u,samples,depth))return g11Fault(29u,c,9u);point=uvec2(c*16777216u+x,(2u*g9Material(c)-1u-depth)*4u);W[m+22u]|=8u;}
 else point=uvec2(g9Packet(packet,1u)*16u,g9Packet(packet,2u)*4u);
 W[m+13u]=W[m+15u]=point.x;W[m+14u]=W[m+16u]=point.y;return true;
}
bool g11ColdInitialize(){
 for(uint at=gl_LocalInvocationIndex;at<uVisV*32u;at+=64u){W[uVisMarkerCurrent+at]=0u;W[uVisMarkerActual+at]=0u;W[uVisMarkerReplay+at]=0u;}
 for(uint at=gl_LocalInvocationIndex;at<uVisV*8u;at+=64u)W[uVisGlyph+at]=0u;
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){for(uint bank=0u;bank<3u;++bank){for(uint w=0u;w<128u;++w)g11BankPut(bank,w,0u);g11BankPut(bank,0u,1110520135u);g11BankPut(bank,1u,1u);g11BankPut(bank,2u,W[uVisControl+G11_CFG+2u]);g11BankPut(bank,3u,W[uVisControl+G11_CFG+3u]);g11BankPut(bank,4u,uK);g11BankPut(bank,5u,uVisV);g11BankPut(bank,10u,1u);g11BankPut(bank,16u,uOwner.x);g11BankPut(bank,17u,uOwner.y);g11BankPut(bank,18u,uContext.x);g11BankPut(bank,19u,uContext.y);g11BankPut(bank,20u,uRun.x);g11BankPut(bank,21u,uRun.y);g11BankPut(bank,22u,uSurface.x);g11BankPut(bank,23u,uSurface.y);uint cfg[8];g11Hash(uVisControl+G11_CFG,64u,cfg);for(uint i=0u;i<8u;++i){g11BankPut(bank,24u+i,cfg[i]);g11BankPut(bank,32u+i,W[uConfig+32u+i]);}g11BankPut(bank,40u,1u);g11BankPut(bank,127u,3235778819u);}
 for(uint slot=0u;slot<uVisV;++slot){W[g11MarkerAt(0u,slot)+31u]=3235778817u;if(!g11Sample(0u,slot,slot/(uVisV/128u)))break;}
 }
 memoryBarrierBuffer();barrier();return g9Ctl(0u)==0u;
}
bool visRootCopy(){
 for(uint at=gl_LocalInvocationIndex;at<uVisV*32u;at+=64u){W[uVisMarkerActual+at]=W[uVisMarkerCurrent+at];W[uVisMarkerReplay+at]=W[uVisMarkerCurrent+at];}
 for(uint at=gl_LocalInvocationIndex;at<65536u*8u;at+=64u)W[uVisPlan+at]=0u;
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){for(uint bank=1u;bank<3u;++bank){for(uint w=0u;w<128u;++w)g11BankPut(bank,w,g11BankHeader(0u,w));g11BankPut(bank,40u,1u);uint sc=g11RoleCounter(bank);for(uint w=0u;w<64u;++w)W[sc+w]=0u;W[sc+1u]=g11BankHeader(bank,10u);W[sc+2u]=g11BankHeader(bank,11u);}for(uint w=0u;w<128u;++w)g11HeaderPut(w,0u);}
 memoryBarrierBuffer();barrier();return true;
}
bool visPlanBegin(){
 for(uint at=gl_LocalInvocationIndex;at<65536u*8u;at+=64u)W[uVisPlan+at]=0u;
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u)for(uint r=0u;r<256u;++r){uint bed=uBed+c*256u+r,flow=uFlow+c*256u+r;W[uVisMaps+c*256u+r]=(uRootKind==1u&&uRole==4u)?B[bed]:C[bed];W[uVisMaps+65536u+c*256u+r]=(uRootKind==1u&&uRole==4u)?B[flow]:C[flow];}
 if(gl_LocalInvocationIndex==0u){for(uint w=0u;w<128u;++w)g11HeaderPut(w,0u);uint e=uVisControl+G11_EXPECT,bank=uRole==4u?2u:1u,ordinal=W[g11RoleCounter(bank)];g11HeaderPut(0u,1345401159u);g11HeaderPut(1u,1u);g11HeaderPut(2u,W[e+6u]);g11HeaderPut(3u,W[e+7u]);g11HeaderPut(4u,W[e+66u]);g11HeaderPut(5u,W[e+67u]);g11HeaderPut(6u,uNonce.x);g11HeaderPut(7u,uNonce.y);g11HeaderPut(8u,uBaseCommit.x);g11HeaderPut(9u,uBaseCommit.y);g11HeaderPut(10u,uRole);g11HeaderPut(11u,uMicro);g11HeaderPut(12u,uVisPhase);g11HeaderPut(13u,ordinal);for(uint i=0u;i<8u;++i){g11HeaderPut(16u+i,W[e+128u+i]);g11HeaderPut(24u+i,W[e+120u+i]);}g11HeaderPut(32u,uRun.x);g11HeaderPut(33u,uRun.y);g11HeaderPut(34u,uOwner.x);g11HeaderPut(35u,uOwner.y);g11HeaderPut(36u,uContext.x);g11HeaderPut(37u,uContext.y);g11HeaderPut(38u,uSurface.x);g11HeaderPut(39u,uSurface.y);g11HeaderPut(40u,uK);g11HeaderPut(41u,uVisV);g11HeaderPut(42u,bank);g11HeaderPut(43u,bank);g11HeaderPut(44u,1u);}
 memoryBarrierBuffer();barrier();return true;
}
// Serial refill uses the real exposure/event order and independent bank counters.
bool g11RefillColumn(uint bank,uint c){uint samples=uVisV/128u,have=0u;for(uint slot=0u;slot<uVisV;++slot){uint m=g11MarkerAt(bank,slot);if(W[m]!=0u&&W[m+1u]==c)have++;}if(have>=samples||g9Occupied(c)==0u)return true;
 for(uint slot=0u;slot<uVisV&&have<samples;++slot){uint m=g11MarkerAt(bank,slot);if(W[m]!=0u)continue;if(!g11Sample(bank,slot,c))return false;if(W[m]!=0u){W[m+25u]=uNonce.x;W[m+26u]=uNonce.y;W[m+27u]=g11Header(13u);W[m+28u]=uVisPhase;have++;}}
 if(have<samples){uint sc=g11RoleCounter(bank);if(W[sc+6u]==0xffffffffu)return g11Fault(29u,c,6u);W[sc+6u]++;}return true;
}
bool g11Refill(uint bank){if(uVisPhase==6u){for(uint event=0u;event<g9Ctl(7u);++event){uint e=event*64u;if(E[e+2u]!=G9_EVENT_SOURCE_BIRTH)continue;uint slot=E[e+11u];if(slot>=uK)return g11Fault(24u,event,11u);uint a=g11PlanAt(49152u+slot);if(W[a]==11u&&W[a+6u]==event&&!g11RefillColumn(bank,E[e+9u]))return false;}}
 else if(uVisPhase==1u||uVisPhase==2u){for(uint c=0u;c<128u;++c){bool exposed=false;for(uint r=0u;r<64u;++r)exposed=exposed||W[g11PlanAt(32768u+c*64u+r)]==7u;if(exposed&&!g11RefillColumn(bank,c))return false;}}return true;
}
void g11ExpirePhase(uint m){if((W[m+22u]&1u)==0u)return;uvec4 elapsed;if(!g9USub(g9U64(uVisTicks),g9U64(uvec2(W[m+17u],W[m+18u])),elapsed)){g11Fault(29u,m,17u);return;}if(g9UCmp(elapsed,g9U32(W[m+19u]))>=0){W[m+22u]&=~(1u|16u);W[m+13u]=W[m+15u];W[m+14u]=W[m+16u];W[m+29u]=0u;}}
bool g11MarkerValid(uint m){uint owner=W[m],c=W[m+1u];if(owner==0u)return W[m+31u]==3235778817u&&W[m+30u]==0u;if(owner>3u||c>=128u||W[m+30u]!=0u||W[m+31u]!=3235778817u||(W[m+22u]&~31u)!=0u||all(equal(uvec2(W[m+20u],W[m+21u]),uvec2(0u)))||W[m+23u]==0u)return false;
 uint rank=W[m+2u];if(owner==1u||owner==2u){if(rank>=g9MapCount(owner==2u,c)||any(notEqual(g9Key(owner==2u,c,rank),g11MarkerKey(m)))||W[m+9u]>=C[g9MapAddress(owner==2u,c,rank)+3u]||W[m+10u]!=C[g9MapAddress(owner==2u,c,rank)+3u]||W[m+3u]!=0u||W[m+4u]!=0u||W[m+5u]!=0u)return false;}
 else{uint slot=W[m+3u];if(slot>=uK||g9Packet(slot,0u)==0u||g9Packet(slot,6u)!=W[m+5u]||g9Packet(slot,7u)!=W[m+4u]||any(notEqual(uvec3(g9Packet(slot,8u),g9Packet(slot,9u),g9Packet(slot,10u)),g11MarkerKey(m))))return false;}
 return W[m+15u]<2147483648u&&W[m+16u]<=2147483648u;
}
bool visPlanApply(){
 for(uint c=gl_LocalInvocationIndex;c<128u;c+=64u)for(uint r=0u;r<256u;++r){W[uVisMaps+32768u+c*256u+r]=C[uBed+c*256u+r];W[uVisMaps+98304u+c*256u+r]=C[uFlow+c*256u+r];}
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){if(!g11ResolveAndSeal()||!g11AuditTransfers()){} }
 memoryBarrierBuffer();barrier();if(g9Ctl(0u)!=0u)return false;
 uint bank=uRole==4u?2u:1u;for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u)if(!g11ApplyMarker(bank,slot))break;
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u&&g9Ctl(0u)==0u){if(!g11Refill(bank)){}uint sc=g11RoleCounter(bank),ordinal=W[sc];if(ordinal>=129u)g11Fault(29u,ordinal,0u);else{uint row=uVisControl+10880u+((bank==2u?129u:0u)+ordinal)*16u;W[row]=ordinal;W[row+1u]=uRole;W[row+2u]=uMicro;W[row+3u]=uVisPhase;W[row+4u]=g11Header(14u);W[row+5u]=g9Ctl(14u);W[row+6u]=uVisV;W[row+7u]=0u;uint hash[8];g11Hash(uVisControl+G11_PH,45u,hash);for(uint i=0u;i<8u;++i)W[row+8u+i]=hash[i];W[sc]=ordinal+1u;g11BankPut(bank,40u,g11BankHeader(bank,40u)|2u);}}
 memoryBarrierBuffer();barrier();return g9Ctl(0u)==0u;
}
bool visRootAudit(){
 for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u){uint a=g11MarkerAt(1u,slot),b=g11MarkerAt(2u,slot);g11ExpirePhase(a);g11ExpirePhase(b);if(!g11MarkerValid(a)||!g11MarkerValid(b))g11Fault(24u,slot,0u);for(uint f=0u;f<32u;++f)if(W[a+f]!=W[b+f])g11Fault(25u,slot,f);if(W[a]>3u||W[a+30u]!=0u||W[a+31u]!=3235778817u)g11Fault(26u,slot,31u);}
 memoryBarrierBuffer();barrier();
 if(gl_LocalInvocationIndex==0u){uint batches=uRootKind==1u?1u:g9Ctl(11u)==0u?0u:1u+4u*g9Ctl(11u);if(!g11ConfigValid()||W[g11RoleCounter(1u)]!=batches||W[g11RoleCounter(2u)]!=batches)g11Fault(26u,batches,0u);
 for(uint f=0u;f<128u;++f)if(g11BankHeader(1u,f)!=g11BankHeader(2u,f))g11Fault(25u,1u,f);
 if(g9Ctl(0u)==0u){for(uint bank=1u;bank<3u;++bank)g11BankPut(bank,40u,g11BankHeader(bank,40u)|4u);g9Set(17u,g9Ctl(17u)|262144u);}}
 memoryBarrierBuffer();barrier();return g9Ctl(0u)==0u;
}
bool visGlyphBuild(){
 // Pstyle is rebuilt within the fixed control span. A real unsampled P is opaque.
 for(uint slot=gl_LocalInvocationIndex;slot<512u;slot+=64u){uint at=uVisControl+384u+slot*4u;for(uint f=0u;f<4u;++f)W[at+f]=0u;if(slot<uK&&g9Packet(slot,0u)>0u){W[at]=g9Packet(slot,6u);W[at+1u]=g9Packet(slot,7u);W[at+2u]=65536u;W[at+3u]=G11_NONE;}}
 memoryBarrierBuffer();barrier();
 for(uint slot=gl_LocalInvocationIndex;slot<uVisV;slot+=64u){uint m=g11MarkerAt(1u,slot),g=uVisGlyph+slot*8u,owner=W[m];if(owner==0u){for(uint i=0u;i<8u;++i)W[g+i]=0u;continue;}if(owner==3u){uint p=W[m+3u];if(p>=uK){g11Fault(24u,slot,3u);continue;}atomicMin(W[uVisControl+384u+p*4u+3u],slot);}uint old=(W[m+22u]&1u)!=0u?W[m+29u]:owner,flags=(old&3u)|((owner&3u)<<2u)|(old!=0u?16u:0u)|32u;uint blend=65536u;
 if(W[m+19u]>0u){uvec4 elapsed,product,q,rem;if(!g9USub(g9U64(uVisTicks),g9U64(uvec2(W[m+17u],W[m+18u])),elapsed)){g11Fault(29u,slot,17u);continue;}if(g9UCmp(elapsed,g9U32(W[m+19u]))<0){if(!g9UMul(elapsed,g9U32(65536u),product)||!g9UDiv(product,g9U32(W[m+19u]),q,rem)){g11Fault(29u,slot,19u);continue;}blend=q.x;}}
 if(old==3u&&owner!=3u&&blend<65536u){g11Fault(23u,slot,10u);continue;}
 W[g]=W[m+13u];W[g+1u]=W[m+14u];W[g+2u]=W[m+15u];W[g+3u]=W[m+16u];W[g+4u]=flags;W[g+5u]=blend;W[g+6u]=old==3u?W[m+10u]:0u;W[g+7u]=W[m+24u];if(W[g]>=2147483648u||W[g+2u]>=2147483648u||W[g+1u]>2147483648u||W[g+3u]>2147483648u||blend>65536u)g11Fault(24u,slot,16u);
 }
 memoryBarrierBuffer();barrier();
 for(uint p=gl_LocalInvocationIndex;p<uK;p+=64u){uint at=uVisControl+384u+p*4u;if(g9Packet(p,0u)==0u)continue;uint slot=W[at+3u];if(slot!=G11_NONE){if(slot>=uVisV){g11Fault(24u,p,3u);continue;}uint m=g11MarkerAt(1u,slot);if(W[m]!=3u||W[m+3u]!=p||W[m+4u]!=W[at+1u]||W[m+5u]!=W[at]){g11Fault(24u,p,4u);continue;}if((W[m+22u]&1u)!=0u&&W[m+29u]!=3u)W[at+2u]=W[uVisGlyph+slot*8u+5u];}}
 memoryBarrierBuffer();barrier();if(gl_LocalInvocationIndex==0u&&g9Ctl(0u)==0u&&(g11BankHeader(1u,40u)&4u)!=0u){g11BankPut(1u,40u,g11BankHeader(1u,40u)|8u);g9Set(17u,g9Ctl(17u)|524288u);}memoryBarrierBuffer();barrier();return g9Ctl(0u)==0u;
}
void g11CompletionExtension(){uint e=uVisControl+G11_EXPECT;W[uCompletion]=1127297351u;W[uCompletion+1u]=2u;W[uCompletion+3u]=3235800177u;W[uCompletion+4u]=W[e+6u];W[uCompletion+156u]=1u;W[uCompletion+157u]=W[e+183u];W[uCompletion+158u]=W[e+7u];W[uCompletion+159u]=W[e+6u];
 const uint dst[8]=uint[8](160u,168u,176u,184u,192u,200u,216u,224u);const uint src[8]=uint[8](152u,128u,136u,104u,112u,120u,80u,144u);for(uint n=0u;n<8u;++n)for(uint i=0u;i<8u;++i)W[uCompletion+dst[n]+i]=W[e+src[n]+i];bool same=W[uCompletion+6u]==2u||W[uCompletion+6u]==4u;
 for(uint i=0u;i<2u;++i){W[uCompletion+208u+i]=W[e+62u+i];W[uCompletion+210u+i]=W[e+(same?62u:66u)+i];W[uCompletion+212u+i]=W[e+64u+i];W[uCompletion+214u+i]=W[e+(same?64u:68u)+i];}for(uint i=0u;i<8u;++i)W[uCompletion+232u+i]=W[uVisControl+G11_EXPECT_SHA+i];memoryBarrierBuffer();W[uCompletion+3u]=3235800177u;
}
#endif
