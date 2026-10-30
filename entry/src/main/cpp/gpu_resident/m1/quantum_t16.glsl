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
