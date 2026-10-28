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
 if(top<=23)sig=units<<uint(23-top);else{int shift=top-23;sig=units>>uint(shift);uint r=units&((1u<<uint(shift))-1u),half=1u<<uint(shift-1);if(r>half||(r==half&&(sig&1u)!=0u))sig++;if(sig==(1u<<24u)){sig>>=1u;top++;}}
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
