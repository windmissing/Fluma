#include "integer.h"
#include <limits>
namespace grs {
int Compare64(U64 a,U64 b){if(a.hi!=b.hi)return a.hi<b.hi?-1:1;return a.lo==b.lo?0:a.lo<b.lo?-1:1;}
bool Add64(U64 a,U64 b,U64& out){U64 t; t.lo=a.lo+b.lo;uint32_t carry=t.lo<a.lo;t.hi=a.hi+b.hi;if(t.hi<a.hi)return false;uint32_t h=t.hi+carry;if(h<t.hi)return false;t.hi=h;out=t;return true;}
bool Sub64(U64 a,U64 b,U64& out){if(Compare64(a,b)<0)return false;U64 t;t.lo=a.lo-b.lo;t.hi=a.hi-b.hi-uint32_t(a.lo<b.lo);out=t;return true;}
U64 Mul32(uint32_t a,uint32_t b){
 // 16-bit schoolbook. Each partial fits unsigned 32; carries are explicit.
 uint32_t a0=a&65535,a1=a>>16,b0=b&65535,b1=b>>16;
 uint32_t t=a0*b0,w0=t&65535,carry=t>>16;t=a1*b0+carry;uint32_t w1=t&65535,w2=t>>16;t=a0*b1+w1;
 return U64{(t<<16)+w0,a1*b1+w2+(t>>16)};
}
U128 Mul64(U64 a,U64 b){
 uint32_t aa[4]={a.lo&65535,a.lo>>16,a.hi&65535,a.hi>>16},bb[4]={b.lo&65535,b.lo>>16,b.hi&65535,b.hi>>16},v[8]{};
 for(int i=0;i<4;i++){uint32_t carry=0;for(int j=0;j<4;j++){uint32_t t=v[i+j]+aa[i]*bb[j]+carry;v[i+j]=t&65535;carry=t>>16;}v[i+4]=carry;}
 U128 r;for(int i=0;i<4;i++)r.w[i]=v[i*2]|(v[i*2+1]<<16);return r;
}
int Compare128(U128 a,U128 b){for(int i=3;i>=0;i--)if(a.w[i]!=b.w[i])return a.w[i]<b.w[i]?-1:1;return 0;}
namespace {
bool DivideSigned(int32_t a,int32_t b,bool ceil,int32_t& result){
 if(b==0)return false;bool negative=(a<0)!=(b<0);uint32_t aa=a<0?0u-uint32_t(a):uint32_t(a),bb=b<0?0u-uint32_t(b):uint32_t(b),q=aa/bb,r=aa%bb;
 if(r&&((negative&&!ceil)||(!negative&&ceil)))q++;
 if(q>(negative?0x80000000u:0x7fffffffu))return false;
 result=negative?(q==0x80000000u?(-2147483647-1):-int32_t(q)):int32_t(q);return true;
}
}
bool FloorDiv(int32_t a,int32_t b,int32_t& result){return DivideSigned(a,b,false,result);}
bool CeilDiv(int32_t a,int32_t b,int32_t& result){return DivideSigned(a,b,true,result);}
bool Divide128(U128 n,uint32_t d,U128& q,uint32_t& rem){
 if(!d)return false;U128 out{};uint32_t r=0;
 for(int i=127;i>=0;i--){bool high=(r>>31)!=0;r=(r<<1)|((n.w[i/32]>>(i%32))&1);if(high||r>=d){r-=d;out.w[i/32]|=uint32_t(1)<<(i%32);}}
 q=out;rem=r;return true;
}
bool MulDivRNE(int32_t a,int32_t b,uint32_t d,int32_t& out){
 if(!d)return false;bool negative=(a<0)!=(b<0);uint32_t aa=a<0?uint32_t(0)-uint32_t(a):uint32_t(a),bb=b<0?uint32_t(0)-uint32_t(b):uint32_t(b);
 U64 p=Mul32(aa,bb);U128 n{{p.lo,p.hi,0,0}},q;uint32_t r;if(!Divide128(n,d,q,r)||q.w[1]||q.w[2]||q.w[3])return false;
 uint32_t v=q.w[0];bool round=r>d-r||(r==d-r&&(v&1));if(round){if(v==UINT32_MAX)return false;v++;}
 if(v>(negative?0x80000000u:0x7fffffffu))return false;
 // Conversion of out-of-range uint32 to signed is avoided.
 if(negative)out=v==0x80000000u?(-2147483647-1):-static_cast<int32_t>(v);else out=static_cast<int32_t>(v);return true;
}
bool FloorBinary64Q20(uint32_t lo,uint32_t hi,uint32_t maxUnits,uint32_t& out){
 uint32_t exp=(hi>>20)&2047,frac=hi&0xfffff;
 if(exp==2047||((hi>>31)!=0&&(exp!=0||frac!=0||lo!=0)))return false;
 if(exp==0){out=0;return true;} // Every binary64 subnormal is below a Q20 unit.
 U64 m{lo,frac|0x100000};int shift=int(exp)-1023-32;
 uint32_t units=0;
 if(shift>=0){if(shift>=32||m.hi!=0||m.lo>(maxUnits>>shift))return false;units=m.lo<<shift;}
 else {int right=-shift;if(right>=64)units=0;else if(right>=32)units=m.hi>>(right-32);else {if((m.hi>>right)!=0)return false;units=(m.lo>>right)|(m.hi<<(32-right));}}
 if(units>maxUnits)return false;out=units;return true;
}
uint32_t Q20FloatBits(uint32_t units){
 if(units==0)return 0;int top=31;while(((units>>top)&1)==0)top--;
 uint32_t sig=0;if(top<=23)sig=units<<(23-top);else {int shift=top-23;sig=units>>shift;uint32_t r=units&((uint32_t(1)<<shift)-1),half=uint32_t(1)<<(shift-1);if(r>half||(r==half&&(sig&1)))sig++;if(sig==(1u<<24)){sig>>=1;top++;}}
 return uint32_t(top-20+127)<<23|(sig&0x7fffff);
}
uint32_t ColumnPreservingFloatBits(uint32_t x){
 uint32_t bits=Q20FloatBits(x),upper=(x>>20)+1;
 if(bits>=Q20FloatBits(upper<<20))bits--;return bits;
}
bool CheckedBytes(size_t words,size_t maxBytes,size_t& bytes){if(words>maxBytes/4)return false;bytes=words*4;return true;}
bool AlignUp(size_t bytes,size_t alignment,size_t maxBytes,size_t& out){if(!alignment||bytes>maxBytes)return false;size_t rem=bytes%alignment,add=rem?alignment-rem:0;if(add>maxBytes-bytes)return false;out=bytes+add;return true;}
}
