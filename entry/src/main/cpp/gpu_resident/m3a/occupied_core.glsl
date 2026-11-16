#version 310 es
precision highp int;
layout(local_size_x=64) in;
layout(std430,binding=0) readonly buffer Base {uint B[];};
layout(std430,binding=1) buffer Candidate {uint C[];};
layout(std430,binding=2) buffer Workspace {uint W[];};
layout(std430,binding=3) buffer Events {uint E[];};
// T236 GRS_Q20_V1. GLSL ES 3.10 highp uint only, no int64/FP64 extension.
// Integer primitive library, not an M0 step shader or a capability PASS.
uvec2 grsMul32(uint a,uint b){uint high,low;umulExtended(a,b,high,low);return uvec2(low,high);}
bool grsAdd64(uvec2 a,uvec2 b,out uvec2 result){uint carry;uint low=uaddCarry(a.x,b.x,carry);uint carryHigh;uint high=uaddCarry(a.y,b.y,carryHigh);uint carryLast;high=uaddCarry(high,carry,carryLast);if((carryHigh|carryLast)!=0u)return false;result=uvec2(low,high);return true;}
int grsCompare64(uvec2 a,uvec2 b){if(a.y!=b.y)return a.y<b.y?-1:1;return a.x==b.x?0:a.x<b.x?-1:1;}
bool grsSub64(uvec2 a,uvec2 b,out uvec2 result){if(grsCompare64(a,b)<0)return false;uint borrow;uint low=usubBorrow(a.x,b.x,borrow);result=uvec2(low,a.y-b.y-borrow);return true;}
uvec4 grsMul64(uvec2 a,uvec2 b){
 uint aa[4]=uint[4](a.x&65535u,a.x>>16u,a.y&65535u,a.y>>16u),bb[4]=uint[4](b.x&65535u,b.x>>16u,b.y&65535u,b.y>>16u),v[8];for(int i=0;i<8;i++)v[i]=0u;
 for(int i=0;i<4;i++){uint carry=0u;for(int j=0;j<4;j++){uint t=v[i+j]+aa[i]*bb[j]+carry;v[i+j]=t&65535u;carry=t>>16u;}v[i+4]=carry;}
 return uvec4(v[0]|(v[1]<<16u),v[2]|(v[3]<<16u),v[4]|(v[5]<<16u),v[6]|(v[7]<<16u));
}
bool grsDivide128(uvec4 n,uint d,out uvec4 quotient,out uint remainder){
 if(d==0u)return false;uvec4 q=uvec4(0u);uint r=0u;
 for(int i=127;i>=0;i--){bool high=(r>>31u)!=0u;r=(r<<1u)|((n[i/32]>>uint(i%32))&1u);if(high||r>=d){r-=d;q[i/32]|=1u<<uint(i%32);}}
 quotient=q;remainder=r;return true;
}
bool grsMulDivRNE(int a,int b,uint d,out int result){
 if(d==0u)return false;bool negative=(a<0)!=(b<0);uint aa=a<0?0u-uint(a):uint(a),bb=b<0?0u-uint(b):uint(b);uvec2 p=grsMul32(aa,bb);uvec4 q;uint r;
 if(!grsDivide128(uvec4(p,0u,0u),d,q,r)||any(notEqual(q.yzw,uvec3(0u))))return false;
 uint v=q.x;bool rounding=r>d-r||(r==d-r&&(v&1u)!=0u);if(rounding){if(v==0xffffffffu)return false;v++;}
 if(v>(negative?0x80000000u:0x7fffffffu))return false;
 result=negative?(v==0x80000000u?(-2147483647-1):-int(v)):int(v);return true;
}
bool grsFloorBinary64Q20(uvec2 raw,uint maxUnits,out uint result){
 uint exp=(raw.y>>20u)&2047u,frac=raw.y&0xfffffu;if(exp==2047u||((raw.y>>31u)!=0u&&(exp!=0u||frac!=0u||raw.x!=0u)))return false;
 if(exp==0u){result=0u;return true;}uvec2 m=uvec2(raw.x,frac|0x100000u);int shift=int(exp)-1023-32;uint units=0u;
 if(shift>=0){if(shift>=32||m.y!=0u||m.x>(maxUnits>>uint(shift)))return false;units=m.x<<uint(shift);}
 else{int right=-shift;if(right>=64)units=0u;else if(right>=32)units=m.y>>uint(right-32);else{if((m.y>>uint(right))!=0u)return false;units=(m.x>>uint(right))|(m.y<<uint(32-right));}}
 if(units>maxUnits)return false;result=units;return true;
}
uint grsQ20FloatBits(uint units){
 if(units==0u)return 0u;int top=findMSB(units);uint sig;
 if(top<=23)sig=units<<uint(23-top);else{int shift=top-23;sig=units>>uint(shift);uint r=units&((1u<<uint(shift))-1u),halfway=1u<<uint(shift-1);if(r>halfway||(r==halfway&&(sig&1u)!=0u))sig++;if(sig==(1u<<24u)){sig>>=1u;top++;}}
 return (uint(top-20+127)<<23u)|(sig&0x7fffffu);
}
uint grsColumnPreservingFloatBits(uint x){uint bits=grsQ20FloatBits(x),upper=(x>>20u)+1u;if(bits>=grsQ20FloatBits(upper<<20u))bits--;return bits;}

int grsCompare128(uvec4 a,uvec4 b){for(int i=3;i>=0;i--)if(a[i]!=b[i])return a[i]<b[i]?-1:1;return 0;}
bool grsDivideSigned(int a,int b,bool ceil,out int result){
 if(b==0)return false;bool negative=(a<0)!=(b<0);uint aa=a<0?0u-uint(a):uint(a),bb=b<0?0u-uint(b):uint(b),q=aa/bb,r=aa%bb;
 if(r!=0u&&((negative&&!ceil)||(!negative&&ceil)))q++;
 if(q>(negative?0x80000000u:0x7fffffffu))return false;result=negative?(q==0x80000000u?(-2147483647-1):-int(q)):int(q);return true;
}
bool grsFloorDiv(int a,int b,out int result){return grsDivideSigned(a,b,false,result);}
bool grsCeilDiv(int a,int b,out int result){return grsDivideSigned(a,b,true,result);}

// Generated from T238 EVENT64 V2; unused words must be zero.
const uvec2 m1Masks[15]=uvec2[15](uvec2(0u),uvec2(0xffffffffu,0x001f0003u),uvec2(0x0030ffffu,0x001f0003u),uvec2(0xffffffffu,0xff1ff000u),uvec2(0xffffffffu,0xff1ff000u),uvec2(0xffffffffu,0xff1fffffu),uvec2(0xffffffffu,0xff1ff000u),uvec2(0xffffffffu,0xff1fffffu),uvec2(0xffffffffu,0xff1f0fffu),uvec2(0x0030ffffu,0x00100fffu),uvec2(0x0030ffffu,0x0010000fu),uvec2(0xffffffffu,0xff1f000fu),uvec2(0xffffffffu,0xff1f0000u),uvec2(0xffffffffu,0xff1f0000u),uvec2(0xffffffffu,0xff1ff000u));

#define M3_SCOPE_KERNEL
#define M3_ROLE_DEFINED
uniform uint uM3Role;
uniform uint uK,uKw,uPad,uWater,uColumn,uHeader,uWords,uPass,uColor,uSortK,uSortJ;
uniform uint uSandIndex,uWaterIndex,uArrivalIndex,uEdges,uFrozen,uRows,uAngles,uSourceEvents,uEventStart,uEventCapacity;
uniform uint uGravity,uSettle,uKAir,uKFlight,uKWet,uKappa,uSoft,uFlux,uQuantumOrdinal;
int m3ScopeAngle;
uniform uvec2 uNonce,uFrame,uControl;
const uint Q=1048576u,CQ=268435456u,TICKS=65536u;
#ifndef M3_EVENT_WORD_DEFINED
#define M3_EVENT_WORD_DEFINED
uniform uint uM3EventWord;
#endif
uniform uint uM3RootPass;
// W0 first error, W1 event cursor, W2 first failing stage, W3 row ordinal.
void qfail(uint reason){if(atomicCompSwap(W[0],0u,reason)==0u){W[2]=uM3RootPass+1u;W[3]=W[1]+1u;}}
uint col(uint f,uint c){return C[uColumn+f*128u+c];}void pc(uint f,uint c,uint v){C[uColumn+f*128u+c]=v;}
uint baseAt(uint ns){return ns==0u?0u:uWater;}uint slots(uint ns){return ns==0u?uK:uKw;}
uint at(uint ns,uint f,uint slot){return baseAt(ns)+f*slots(ns)+slot;}
uint tag(uint ns,uint slot){return C[at(ns,ns==0u?6u:10u,slot)];}
uint medium(uint ns,uint slot){return ns==0u?C[at(0u,14u,slot)]:0u;}
uint eta(uint c){return col(10u,c)+col(8u,c)+col(6u,c);}uint occupied(uint c){return eta(c)+col(7u,c);}
void derived(uint c){pc(11u,c,col(10u,c)+col(8u,c));pc(12u,c,eta(c));pc(13u,c,occupied(c));}
uint classify(uint ns,uint c,uint y,int vy){return ns==0u&&W[uFrozen+128u+c]!=0u&&y>col(10u,c)&&(y<W[uFrozen+c]||(y==W[uFrozen+c]&&vy<=0))?1u:0u;}
void state(uint ns,uint slot,uint value,uint wait,uint m){C[at(ns,ns==0u?6u:10u,slot)]=value|(wait<<12u);if(ns==0u){C[at(0u,11u,slot)]=wait;C[at(0u,14u,slot)]=m;C[at(0u,15u,slot)]=C[uHeader+118u]+1u;}}
void before(uint ns,uint slot,out uint data[16]){for(uint i=0u;i<16u;i++)data[i]=C[at(ns,i,slot)];}
void plusHeader(uint word,uint q){uvec2 n;if(!grsAdd64(uvec2(C[uHeader+word],C[uHeader+word+1u]),uvec2(q,0u),n)){qfail(13u);return;}C[uHeader+word]=n.x;C[uHeader+word+1u]=n.y;}
// GRS_M1_SEGMENT_T16_V1. Signed magnitude intermediates, exact integer RNE.
struct WideSigned {uvec2 mag;bool negative;};
WideSigned wp(int a,uint b){WideSigned z;z.negative=a<0;z.mag=grsMul32(a<0?0u-uint(a):uint(a),b);return z;}
WideSigned wadd(WideSigned a,WideSigned b){WideSigned z;z.negative=a.negative;if(a.negative==b.negative){if(!grsAdd64(a.mag,b.mag,z.mag))qfail(13u);}else if(grsCompare64(a.mag,b.mag)>=0)grsSub64(a.mag,b.mag,z.mag);else{grsSub64(b.mag,a.mag,z.mag);z.negative=b.negative;}return z;}
int wrne(WideSigned a,uint den){uvec4 q;uint rem;if(!grsDivide128(uvec4(a.mag,0u,0u),den,q,rem)||any(notEqual(q.yzw,uvec3(0u)))){qfail(13u);return 0;}uint v=q.x;if(rem>den-rem||(rem==den-rem&&(v&1u)!=0u)){if(v==0xffffffffu){qfail(13u);return 0;}v++;}if(v>(a.negative?0x80000000u:0x7fffffffu)){qfail(13u);return 0;}return a.negative?(v==0x80000000u?(-2147483647-1):-int(v)):int(v);}
int velocity(int v,uint r,uint k,int acceleration,int target,bool wet){int next=wet?wrne(wadd(wp(v,7864320u),wp(target,k*r)),7864320u+k*r):wrne(wp(v,7864320u),7864320u+k*r)+wrne(wp(acceleration,r),7864320u);if(next< -1073741824||next>1073741824)qfail(13u);return next;}
int displacement(int a,int b,uint ticks){return wrne(wadd(wp(a,ticks),wp(b,ticks)),15728640u);}
int isign(int a){return a<0?-1:a>0?1:0;}
uint turnTime(int v,uint r,uint k,int a,int target,bool wet){if(v==0||isign(velocity(v,r,k,a,target,wet))==isign(v))return r;uint lo=1u,hi=r;for(uint i=0u;i<16u&&lo<hi;i++){uint mid=(lo+hi)/2u;if(isign(velocity(v,mid,k,a,target,wet))==isign(v))lo=mid+1u;else hi=mid;}return lo;}
bool ratioLess(uint an,uint ad,uint bn,uint bd){return grsCompare64(grsMul32(an,bd),grsMul32(bn,ad))<0;}
bool ratioEqual(uint an,uint ad,uint bn,uint bd){return grsCompare64(grsMul32(an,bd),grsMul32(bn,ad))==0;}
void plane(int num,uint den,uint kind,inout uint n,inout uint d,inout uint hit){if(num<0||den==0u||uint(num)>den)return;if(ratioLess(uint(num),den,n,d)||(ratioEqual(uint(num),den,n,d)&&kind<hit)){n=uint(num);d=den;hit=kind;}}
uint tickCeil(uint ticks,uint n,uint d){uvec4 q;uint rem;if(!grsDivide128(uvec4(grsMul32(ticks,n),0u,0u),d,q,rem)||any(notEqual(q.yzw,uvec3(0u)))){qfail(13u);return 0u;}return q.x+(rem>0u?1u:0u);}
int point(int start,int delta,uint n,uint d){return start+wrne(wp(delta,n),d);}

uint reserveRow(uint ns,uint slot){uint row=W[1];if(row>=uEventCapacity){qfail(12u);return row;}if(ns<2u){uint p=uRows+(ns==0u?slot:uK+slot);uint count=W[p],limit=ns==0u?48u:16u;if(count>=limit){qfail(12u);return row;}W[p]=count+1u;}W[1]=row+1u;return row;}
void emit(uint kind,uint ns,uint slot,uint from,uint to,uint old[16],uint a0,uint a1,uint b0,uint b1,uint total0,uint la0,uint lb0,uint num,uint den,uint tau0,uint tau1,uint segment,uint planeTag,uint turnMask){
 uint row=ns==2u?W[uEdges+16u*min(from,to)+10u]:reserveRow(ns,slot);if(W[0]!=0u)return;uint e[64];for(uint i=0u;i<64u;i++)e[i]=0u;
 e[0]=1163088455u;e[1]=4u;e[2]=uNonce.x;e[3]=uNonce.y;e[4]=2u;e[5]=kind==9u?3u:kind==13u||kind==11u?4u:2u;e[6]=kind;e[7]=row+1u;e[9]=uFrame.x;e[10]=uFrame.y;e[11]=uQuantumOrdinal;e[12]=B[uHeader+14u];e[13]=B[uHeader+15u];uvec2 commit;if(!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(1u,0u),commit)){qfail(13u);return;}e[14]=commit.x;e[15]=commit.y;e[20]=from;e[21]=to;
 if(ns<2u){uint gen=ns==0u?10u:11u,src=ns==0u?7u:12u,origin=ns==0u?8u:6u,tf=ns==0u?6u:10u;
  e[16]=slot;e[17]=old[gen];e[18]=old[5];e[19]=old[src];e[22]=old[0];e[23]=C[at(ns,0u,slot)];e[24]=old[1];e[25]=old[2];e[26]=C[at(ns,1u,slot)];e[27]=C[at(ns,2u,slot)];e[28]=old[3];e[29]=old[4];e[30]=C[at(ns,3u,slot)];e[31]=C[at(ns,4u,slot)];e[48]=old[origin];e[49]=old[origin+1u];
  if(old[origin]==0u||old[origin+1u]!=0u||old[origin]>B[uHeader+34u]){qfail(15u);return;}uint srcAt=uSourceEvents+16u*(old[origin]-1u);e[50]=W[srcAt];e[51]=W[srcAt+1u];uint oldM=ns==0u?old[14]:0u,newM=medium(ns,slot),next=tag(ns,slot);
  e[52]=(old[tf]&15u)|((next&15u)<<4u)|(oldM<<8u)|(newM<<9u)|(((old[tf]>>12u)&15u)<<10u)|(((next>>12u)&15u)<<14u)|(ns<<18u)|(turnMask<<20u);e[56]=tau0;e[57]=tau1;e[58]=oldM;e[59]=newM;e[60]=W[uFrozen+from];e[61]=W[uFrozen+to];e[62]=planeTag;e[63]=segment;
 }else e[52]=2u<<18u;
 e[52]|=uM3Role<<22u;e[32]=a0;e[33]=a1;e[34]=b0;e[35]=b1;e[36]=total0;e[38]=C[uHeader+54u];e[40]=la0;e[41]=col(15u,from);e[42]=kind==5u||kind==9u?lb0:0u;e[43]=kind==5u||kind==9u?col(15u,to):0u;if(kind==9u){uint q=a0-a1;e[38]=total0-q;e[41]=la0-q;e[43]=lb0-q;}e[44]=num;e[46]=den;
 uvec2 mask=m1Masks[kind];for(uint i=0u;i<64u;i++)E[uM3EventWord+row*64u+i]=(((i<32u?mask.x:mask.y)>>(i%32u))&1u)!=0u?e[i]:0u;
 if(ns==1u)W[8]++;if(kind==6u)W[9]++;if(kind==11u)W[10]++;if(kind==10u)atomicAdd(W[11],1u);
}
void retire(uint ns,uint slot){uint gen=ns==0u?10u:11u;for(uint f=0u;f<16u;f++)if(f!=gen)C[at(ns,f,slot)]=0u;}
uint guard(uint requested,uint donor,uint receiver,bool land){uint q=requested;int tangent=int(W[5]);uint lo=land?donor:min(donor,receiver),hi=land?donor:max(donor,receiver);for(int e=int(lo)-1;e<=int(hi);e++){if(e<0||e>=127)continue;uint a=uint(e),b=a+1u;int delta=int(col(10u,b))-int(col(10u,a))-tangent,k=(b==receiver?1:0)-(a==receiver?1:0);if(!land)k-=(b==donor?1:0)-(a==donor?1:0);if(abs(delta)>605394){qfail(14u);return 0u;}if(k>0)q=min(q,uint(605394-delta)/uint(k));if(k<0)q=min(q,uint(605394+delta)/uint(-k));}return q;}
void charge(uint q,uint a,uint b,bool pair){if(q>C[uHeader+54u]||q>col(15u,a)||(pair&&q>col(15u,b))){qfail(15u);return;}C[uHeader+54u]-=q;pc(15u,a,col(15u,a)-q);pc(14u,a,col(14u,a)+q);if(pair){pc(15u,b,col(15u,b)-q);pc(14u,b,col(14u,b)+q);}}
void land(uint slot,uint c){uint q=min(C[slot],min(C[uHeader+54u],col(15u,c)));q=guard(q,c,c,true);if(q==0u||W[0]!=0u)return;uint old[16];before(0u,slot,old);uint a=col(2u,c),b=col(8u,c),t=C[uHeader+54u],l=col(15u,c);charge(q,c,c,false);pc(2u,c,a+q);pc(10u,c,a+q);pc(8u,c,b-q);if(q==old[0])retire(0u,slot);else{C[slot]-=q;C[2u*uK+slot]=col(10u,c);C[4u*uK+slot]=0u;state(0u,slot,2u,1u,0u);}derived(c);emit(8u,0u,slot,c,c,old,a,a+q,b,b-q,t,l,0u,0u,0u,TICKS,TICKS,0u,256u+c,0u);}

uint m3JClearance(uint c){
 uint result=CQ-col(7u,c);
 for(uint i=0u;i<uKw;++i)if(C[uWater+i]!=0u&&(C[uWater+uKw+i]>>20u)==c){uint y=C[uWater+2u*uKw+i];result=min(result,y==0u?0u:y-1u);}
 return result>eta(c)?result-eta(c):0u;
}
void resolvePacket(uint ns,uint s){
 uint c=C[at(ns,1u,s)]>>20u,tau=0u,lastPlane=0xffffffffu;uint old[16];before(ns,s,old);uint y=old[2],m=classify(ns,c,y,int(old[4]));
 if((tag(ns,s)&15u)!=1u&&((tag(ns,s)&15u)!=2u||y>(ns==1u?W[uFrozen+c]:col(10u,c)))){state(ns,s,1u,0u,m);emit(12u,ns,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u);}
 else if(m!=medium(ns,s)){state(ns,s,1u,0u,m);emit(6u,ns,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,1024u+c,0u);}
 uint angle=uAngles+uint(int(W[4])+7680)*4u;int ax=wrne(wp(int(W[angle+1u]),uGravity),Q),ay=-wrne(wp(int(W[angle+2u]),uGravity),Q),tx=wrne(wp(int(W[angle+1u]),uSettle),Q),ty=-wrne(wp(int(W[angle+2u]),uSettle),Q);
 for(uint iteration=0u;iteration<49u&&tau<TICKS;iteration++){
  if(W[0]!=0u)return;before(ns,s,old);int x=int(old[1]);y=old[2];int vx=int(old[3]),vy=int(old[4]);m=medium(ns,s);bool wet=m==1u;uint k=ns==1u?uKFlight:wet?uKWet:uKAir,r=TICKS-tau;
  uint turnX=turnTime(vx,r,k,ax,tx,wet),turnY=turnTime(vy,r,k,ay,ty,wet),seg=min(turnX,turnY);int ex=velocity(vx,seg,k,ax,tx,wet),ey=velocity(vy,seg,k,ay,ty,wet),dx=displacement(vx,ex,seg),dy=displacement(vy,ey,seg);
  uint num=1u,den=1u,hit=seg<r?5u:6u,turnMask=(turnX==seg?1u:0u)|(turnY==seg?2u:0u);int boundary=dx<0?int(c*Q):int((c+1u)*Q);
  if(y<=col(10u,c))plane(0,1u,0u,num,den,hit);else if(dy<0)plane(int(y-col(10u,c)),uint(-dy),0u,num,den,hit);
  if(dy>0)plane(int(CQ-y),uint(dy),1u,num,den,hit);
  if(dx!=0){bool wall=(dx<0&&c==0u)||(dx>0&&c==127u);plane(dx<0?x-boundary:boundary-x,uint(abs(dx)),wall?1u:2u,num,den,hit);}
  if(ns==1u){if(y<=W[uFrozen+c])plane(0,1u,3u,num,den,hit);else if(dy<0)plane(int(y-W[uFrozen+c]),uint(-dy),3u,num,den,hit);}
  else if(W[uFrozen+128u+c]!=0u){if(!wet&&dy<0&&y>=W[uFrozen+c])plane(int(y-W[uFrozen+c]),uint(-dy),3u,num,den,hit);if(wet&&dy>0&&y<=W[uFrozen+c])plane(int(W[uFrozen+c]-y),uint(dy),3u,num,den,hit);}
  uint ticks=tickCeil(seg,num,den),nextTau=tau+ticks,planeTag=(hit+1u)*256u+(hit==2u||hit==1u?uint(boundary)/Q:c);
  if(ticks==0u&&lastPlane==planeTag){qfail(17u);return;}lastPlane=ticks==0u?planeTag:0xffffffffu;
  int nx=point(x,dx,num,den),ny=point(int(y),dy,num,den),nvx=velocity(vx,ticks,k,ax,tx,wet),nvy=velocity(vy,ticks,k,ay,ty,wet);uint kind=ns==0u?3u:4u,to=c,tf=C[uHeader+54u],lf=col(15u,c),lt=lf;uint a0=0u,a1=0u,b0=0u,b1=0u;
  bool wait=false,landNow=false;
  if(hit==0u){ny=int(col(10u,c));nvy=0;wait=true;landNow=ns==0u;state(ns,s,2u,1u,0u);kind=7u;}
  else if(hit==1u){bool ceiling=dy>0&&ratioEqual(CQ-y,uint(dy),num,den);if(ceiling){planeTag=512u+129u;ny=int(CQ);nvy=0;}else{nx=dx<0?0:int(128u*Q-1u);nvx=0;}wait=true;kind=7u;state(ns,s,3u,5u,m);}
  else if(hit==2u){to=dx<0?c-1u:c+1u;lt=col(15u,to);uint mass=old[0];bool high=ny<=int(col(10u,to)),capacity=mass>CQ-occupied(to)||(ns==0u&&mass>m3JClearance(to)),budget=ns==0u&&(mass>tf||mass>lf||mass>lt);
   if(high||capacity||budget){if(high)planeTag=256u+to;to=c;nx=dx<0?boundary:boundary-1;nvx=0;wait=true;kind=7u;state(ns,s,3u,high?3u:capacity?2u:4u,m);}
   else{uint f=ns==0u?8u:7u;a0=col(f,c);b0=col(f,to);if(a0<mass){qfail(15u);return;}pc(f,c,a0-mass);pc(f,to,b0+mass);a1=a0-mass;b1=b0+mass;if(ns==0u)charge(mass,c,to,true);derived(c);derived(to);nx=dx<0?boundary-1:boundary;kind=5u;state(ns,s,1u,0u,classify(ns,to,uint(ny),nvy));}
  }else if(hit==3u){ny=ns==1u?min(int(y),int(W[uFrozen+c])):int(W[uFrozen+c]);if(ns==1u){wait=true;nvy=0;kind=7u;state(ns,s,2u,6u,0u);}else{if(wet)ny++;state(ns,s,1u,0u,wet?0u:1u);kind=6u;}}
  else if(hit==5u){kind=14u;state(ns,s,1u,0u,m);}
  else state(ns,s,1u,0u,m);
  if(nx<0||nx>=int(128u*Q)||ny<0||ny>int(CQ)){qfail(15u);return;}C[at(ns,1u,s)]=uint(nx);C[at(ns,2u,s)]=uint(ny);C[at(ns,3u,s)]=uint(nvx);C[at(ns,4u,s)]=uint(nvy);
  if(ns==0u)C[at(0u,14u,s)]=classify(0u,to,uint(ny),nvy);
  emit(kind,ns,s,c,to,old,a0,a1,b0,b1,tf,lf,lt,num,den,tau,wait?TICKS:nextTau,seg,planeTag,hit==5u?turnMask:0u);c=to;tau=wait?TICKS:nextTau;if(landNow)land(s,c);if(C[at(ns,0u,s)]==0u)return;
 }
 if(tau<TICKS)qfail(12u);
}
void resolvePool(uint ns){uint count=ns==0u?uPad:uKw,offset=ns==0u?uSandIndex:uWaterIndex;for(uint i=0u;i<count;i++){uint s=W[offset+i];if(s>=slots(ns)||C[at(ns,0u,s)]==0u)continue;resolvePacket(ns,s);if(W[0]!=0u)return;}}

bool inventory(){
 if(C[uHeader]!=1213420103u||C[uHeader+1u]!=4u||C[uHeader+2u]!=256u||C[uHeader+31u]!=15u||C[uHeader+32u]!=0u||C[uHeader+34u]>4096u){qfail(1u);return false;}
 uvec2 sand=uvec2(0u),water=uvec2(0u);
 for(uint c=0u;c<128u;c++){if(col(0u,c)!=0u){qfail(22u);return false;}if(col(1u,c)!=0u||col(3u,c)!=0u||col(4u,c)!=0u||col(5u,c)!=0u||col(9u,c)!=0u||col(10u,c)!=col(2u,c)){qfail(15u);return false;}
  uint s=col(10u,c),w=col(6u,c),j=0u,p=0u;if(s>CQ||w>CQ-s){qfail(15u);return false;}
  for(uint ns=0u;ns<2u;ns++)for(uint i=0u;i<slots(ns);i++){uint mass=C[at(ns,0u,i)];if(mass==0u)continue;uint x=C[at(ns,1u,i)],y=C[at(ns,2u,i)];int vx=int(C[at(ns,3u,i)]),vy=int(C[at(ns,4u,i)]);if(mass>CQ||x>=128u*Q||y>CQ||vx< -1073741824||vx>1073741824||vy< -1073741824||vy>1073741824||(ns==1u&&vy>0)||C[at(ns,5u,i)]==0u||(tag(ns,i)&15u)<1u||(tag(ns,i)&15u)>3u){qfail(15u);return false;}
   if((x>>20u)==c){if(ns==0u){if(p>CQ-mass){qfail(15u);return false;}p+=mass;}else{if(j>CQ-mass){qfail(15u);return false;}j+=mass;}}
  }
  if(p>CQ-s-w||j>CQ-s-w-p||p!=col(8u,c)||j!=col(7u,c)||col(11u,c)!=s+p||col(12u,c)!=s+p+w||col(13u,c)!=s+w+p+j){qfail(15u);return false;}
  if(!grsAdd64(sand,uvec2(s+p,0u),sand)||!grsAdd64(water,uvec2(w+j,0u),water)){qfail(13u);return false;}
 }
 for(uint ns=0u;ns<2u;ns++)for(uint i=0u;i<slots(ns);i++)if(C[at(ns,0u,i)]!=0u){uint origin=ns==0u?8u:6u,gen=ns==0u?10u:11u,src=ns==0u?7u:12u;uint o=C[at(ns,origin,i)];if(o==0u||C[at(ns,origin+1u,i)]!=0u||o>C[uHeader+34u]||C[at(ns,gen,i)]==0u||C[at(ns,src,i)]==0u){qfail(15u);return false;}uint se=uSourceEvents+(o-1u)*16u;if((W[se+4u]==1u?1u:0u)!=ns||W[se+9u]!=C[at(ns,src,i)]||C[at(ns,5u,i)]<W[se+7u]||C[at(ns,5u,i)]-W[se+7u]>=W[se+8u]){qfail(15u);return false;}for(uint k=i+1u;k<slots(ns);k++)if(C[at(ns,0u,k)]!=0u&&C[at(ns,5u,k)]==C[at(ns,5u,i)]){qfail(15u);return false;}}
 uvec2 es,ew;if(!grsAdd64(uvec2(C[uHeader+64u],C[uHeader+65u]),uvec2(C[uHeader+60u],C[uHeader+61u]),es)||!grsAdd64(uvec2(C[uHeader+66u],C[uHeader+67u]),uvec2(C[uHeader+62u],C[uHeader+63u]),ew)||grsCompare64(es,sand)!=0||grsCompare64(ew,water)!=0||grsCompare64(sand,uvec2(C[uHeader+56u],C[uHeader+57u]))!=0||grsCompare64(water,uvec2(C[uHeader+58u],C[uHeader+59u]))!=0){qfail(15u);return false;}return true;
}
void prepare(){W[1]=uEventStart;if((uK!=192u&&uK!=512u)||(uKw!=64u&&uKw!=128u)||uQuantumOrdinal==0u||uQuantumOrdinal>4u||uGravity<120u||uGravity>1073741824u||uSettle==0u||uSettle>1073741824u||uKAir>960u||uKFlight>960u||uKWet<1u||uKWet>960u||uKappa>960u||uSoft>=605394u||m3ScopeAngle< -7680||m3ScopeAngle>7680||uEventStart>uEventCapacity){qfail(20u);return;}if(!inventory())return;
 int accepted=m3ScopeAngle;for(uint c=0u;c<127u;++c)if(abs(int(col(10u,c+1u))-int(col(10u,c))-int(W[uAngles+uint(accepted+7680)*4u]))>605394){qfail(14u);return;}
 W[4]=uint(accepted);W[5]=W[uAngles+uint(accepted+7680)*4u];uvec2 serial;if(!grsAdd64(uvec2(B[uHeader+118u],B[uHeader+119u]),uvec2(1u,0u),serial)){qfail(13u);return;}W[6]=serial.x;W[7]=serial.y;C[uHeader+45u]=uint(accepted);C[uHeader+46u]=uint(m3ScopeAngle);C[uHeader+52u]=2621440u;C[uHeader+53u]=0u;C[uHeader+54u]=2621440u;C[uHeader+55u]=0u;for(uint c=0u;c<128u;c++){pc(14u,c,0u);pc(15u,c,244667u);}
}
bool lessSlot(uint a,uint b,uint ns,bool arrival){uint n=slots(ns);bool av=a<n&&C[at(ns,0u,a)]!=0u,bv=b<n&&C[at(ns,0u,b)]!=0u;if(av!=bv)return av;if(!av)return a<b;
 if(arrival){uint ca=C[at(ns,1u,a)]>>20u,cb=C[at(ns,1u,b)]>>20u;if(ca!=cb)return ca<cb;uint ya=C[at(ns,2u,a)],yb=C[at(ns,2u,b)];if(ya!=yb)return ya<yb;}
 uint ia=C[at(ns,5u,a)],ib=C[at(ns,5u,b)];return ia==ib?a<b:ia<ib;
}
void sortIndex(uint i,uint offset,uint count,uint ns,bool arrival){if(i>=count)return;uint j=i^uSortJ;if(j<=i||j>=count)return;uint a=W[offset+i],b=W[offset+j];bool up=(i&uSortK)==0u;if(up?lessSlot(b,a,ns,arrival):lessSlot(a,b,ns,arrival)){W[offset+i]=b;W[offset+j]=a;}}
void waterPropose(uint edge){uint color=(W[6]&1u)^uColor;if(edge%2u!=color)return;uint a=uEdges+16u*edge;for(uint i=0u;i<16u;i++)W[a+i]=0u;uint c=edge,r=edge+1u;int ha=2*int(eta(c))-(2*int(c)+1)*int(W[5]),hb=2*int(eta(r))-(2*int(r)+1)*int(W[5]);uint d=ha>=hb?c:r; r=ha>=hb?r:c;int hd=max(ha,hb),bedD=2*int(col(10u,d))-(2*int(d)+1)*int(W[5]),bedR=2*int(col(10u,r))-(2*int(r)+1)*int(W[5]);uint delta=uint(abs(ha-hb));uvec4 quotient;uint rem;if(!grsDivide128(uvec4(grsMul32(uKappa,delta),0u,0u),240u,quotient,rem)||any(notEqual(quotient.yzw,uvec3(0u)))){qfail(13u);return;}
 uint q=min(quotient.x,min(delta/4u,min(col(6u,d),min(uint(max(0,hd-max(bedD,bedR)))/2u,CQ-occupied(r)))));W[a]=d;W[a+1u]=r;W[a+2u]=q;W[a+4u]=col(6u,d);W[a+5u]=col(6u,r);
}
void waterPrefix(){uint color=(W[6]&1u)^uColor;for(uint e=color;e<127u;e+=2u){uint a=uEdges+16u*e,q=W[a+2u];if(q==0u)continue;W[a+3u]=q;W[a+10u]=reserveRow(2u,0u);if(W[0]!=0u)return;}}
void bedPropose(uint edge){if(edge%3u!=uColor)return;uint a=uEdges+16u*edge;for(uint i=0u;i<16u;i++)W[a+i]=0u;int delta=int(col(10u,edge+1u))-int(col(10u,edge))-int(W[5]);if(abs(delta)<=int(uSoft))return;uint d=delta>0?edge+1u:edge,r=delta>0?edge:edge+1u,q=uint(abs(delta)-int(uSoft))/2u;q=min(q,min(col(2u,d),min(uFlux/120u,min(min(CQ-occupied(r),m3JClearance(r)),min(col(15u,d),col(15u,r))))));W[a]=d;W[a+1u]=r;W[a+2u]=q;W[a+4u]=col(2u,d);W[a+5u]=col(2u,r);}
void bedPrefix(){for(uint e=uColor;e<127u;e+=3u){uint a=uEdges+16u*e,d=W[a],r=W[a+1u],q=W[a+2u];if(q==0u)continue;q=guard(min(q,C[uHeader+54u]),d,r,false);if(W[0]!=0u)return;if(q==0u)continue;W[a+3u]=q;W[a+6u]=col(15u,d);W[a+7u]=col(15u,r);W[a+8u]=C[uHeader+54u];W[a+10u]=reserveRow(2u,0u);if(W[0]!=0u)return;charge(q,d,r,true);}}
void edgeApply(uint edge,bool bed){uint color=bed?uColor:((W[6]&1u)^uColor);if(edge%(bed?3u:2u)!=color)return;uint a=uEdges+16u*edge,q=W[a+3u];if(q==0u)return;uint d=W[a],r=W[a+1u],field=bed?2u:6u,da=W[a+4u],rb=W[a+5u];if(col(field,d)!=da||col(field,r)!=rb||da<q||rb>CQ-q){qfail(15u);return;}pc(field,d,da-q);pc(field,r,rb+q);if(bed){pc(10u,d,da-q);pc(10u,r,rb+q);}derived(d);derived(r);uint empty[16];for(uint i=0u;i<16u;i++)empty[i]=0u;emit(bed?9u:10u,2u,0u,d,r,empty,da,da-q,rb,rb+q,bed?W[a+8u]:0u,bed?W[a+6u]:0u,bed?W[a+7u]:0u,0u,0u,0u,0u,0u,0u,0u);}
void finalSand(){for(uint i=0u;i<uPad;i++){uint s=W[uSandIndex+i];if(s>=uK||C[s]==0u)continue;uint c=C[uK+s]>>20u,y=C[2u*uK+s],old[16];before(0u,s,old);if(y<col(10u,c)){C[2u*uK+s]=col(10u,c);C[4u*uK+s]=0u;state(0u,s,2u,1u,0u);emit(13u,0u,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,TICKS,TICKS,0u,256u+c,0u);}else if((tag(0u,s)&15u)==2u&&y>col(10u,c)){state(0u,s,1u,0u,classify(0u,c,y,int(C[4u*uK+s])));emit(12u,0u,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,TICKS,TICKS,0u,256u+c,0u);}if(W[0]!=0u)return;}}
void arrivals(){for(uint i=0u;i<uKw;i++){uint s=W[uArrivalIndex+i];if(C[at(1u,0u,s)]==0u)continue;uint c=C[at(1u,1u,s)]>>20u,y=C[at(1u,2u,s)],old[16];before(1u,s,old);if(y<=eta(c)){uint m=old[0],a=col(6u,c),b=col(7u,c);if(b<m||a>CQ-m){qfail(15u);return;}pc(6u,c,a+m);pc(7u,c,b-m);retire(1u,s);derived(c);plusHeader(116u,1u);emit(11u,1u,s,c,c,old,a,a+m,b,b-m,0u,0u,0u,0u,0u,TICKS,TICKS,0u,1024u+c,0u);}else if((tag(1u,s)&15u)==2u){state(1u,s,1u,0u,0u);emit(12u,1u,s,c,c,old,0u,0u,0u,0u,0u,0u,0u,0u,0u,TICKS,TICKS,0u,1024u+c,0u);}if(W[0]!=0u)return;}}
void finishCandidate(){if(!inventory())return;uvec2 settled=uvec2(0u),flight=uvec2(0u);for(uint c=0u;c<128u;c++){int d=c<127u?int(col(10u,c+1u))-int(col(10u,c))-int(W[5]):0;if(abs(d)>605394||col(14u,c)+col(15u,c)!=244667u){qfail(14u);return;}grsAdd64(settled,uvec2(col(6u,c),0u),settled);grsAdd64(flight,uvec2(col(7u,c),0u),flight);derived(c);}uint liveS=0u,liveW=0u;for(uint s=0u;s<uK;s++)if(C[s]!=0u)liveS++;for(uint s=0u;s<uKw;s++)if(C[at(1u,0u,s)]!=0u){liveW++;if(C[at(1u,2u,s)]<=eta(C[at(1u,1u,s)]>>20u)){qfail(15u);return;}}C[uHeader+41u]=liveS;C[uHeader+43u]=uK-liveS;C[uHeader+42u]=liveW;C[uHeader+44u]=uKw-liveW;
 C[uHeader+112u]=settled.x;C[uHeader+113u]=settled.y;C[uHeader+114u]=flight.x;C[uHeader+115u]=flight.y;plusHeader(16u,1u);C[uHeader+118u]=W[6];C[uHeader+119u]=W[7];C[uHeader+120u]=W[6];C[uHeader+121u]=W[7];C[uHeader+128u]=W[8];C[uHeader+129u]=W[9];C[uHeader+130u]=W[10];C[uHeader+131u]=W[11];C[uHeader+38u]=W[1];C[uHeader+51u]=uQuantumOrdinal;C[uHeader+12u]=uNonce.x;C[uHeader+13u]=uNonce.y;C[uHeader+49u]=uFrame.x;C[uHeader+50u]=uFrame.y;C[uHeader+18u]=uControl.x;C[uHeader+19u]=uControl.y;C[uHeader+79u]=2u;C[uHeader+80u]=1u;W[12]=1u;
}
