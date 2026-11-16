// U32-limb U128/S128, low-first, two's complement. No int64 extension or
// floating-point intermediary. All faulting operations leave out=0.
bool g9Zero(uvec4 a){return all(equal(a,uvec4(0u)));}
bool g9Negative(uvec4 a){return (a.w&0x80000000u)!=0u;}
uvec4 g9U32(uint a){return uvec4(a,0u,0u,0u);}
uvec4 g9S32(int a){uint sign=a<0?0xffffffffu:0u;return uvec4(uint(a),sign,sign,sign);}
uvec4 g9U64(uvec2 a){return uvec4(a,0u,0u);}
uvec4 g9S64(uvec2 a){uint sign=(a.y&0x80000000u)!=0u?0xffffffffu:0u;return uvec4(a,sign,sign);}
int g9UCmp(uvec4 a,uvec4 b){for(int i=3;i>=0;--i)if(a[i]!=b[i])return a[i]<b[i]?-1:1;return 0;}
int g9SCmp(uvec4 a,uvec4 b){if(g9Negative(a)!=g9Negative(b))return g9Negative(a)?-1:1;return g9UCmp(a,b);}
uvec4 g9AddBits(uvec4 a,uvec4 b,out uint carry){
 uvec4 result;carry=0u;for(int i=0;i<4;++i){uint c0,c1;uint sum=uaddCarry(a[i],b[i],c0);result[i]=uaddCarry(sum,carry,c1);carry=c0|c1;}return result;
}
uvec4 g9NegBits(uvec4 a){uint carry;return g9AddBits(~a,g9U32(1u),carry);}
bool g9UAdd(uvec4 a,uvec4 b,out uvec4 result){uint carry;result=g9AddBits(a,b,carry);if(carry!=0u){result=uvec4(0u);return false;}return true;}
bool g9USub(uvec4 a,uvec4 b,out uvec4 result){result=uvec4(0u);if(g9UCmp(a,b)<0)return false;uint carry;result=g9AddBits(a,g9NegBits(b),carry);return true;}
bool g9SAdd(uvec4 a,uvec4 b,out uvec4 result){uint carry;result=g9AddBits(a,b,carry);if(g9Negative(a)==g9Negative(b)&&g9Negative(result)!=g9Negative(a)){result=uvec4(0u);return false;}return true;}
bool g9SNeg(uvec4 a,out uvec4 result){result=uvec4(0u);if(all(equal(a,uvec4(0u,0u,0u,0x80000000u))))return false;result=g9NegBits(a);return true;}
bool g9SSub(uvec4 a,uvec4 b,out uvec4 result){
 // MIN is a valid subtrahend when the signed difference remains in range.
 uint carry;result=g9AddBits(a,g9NegBits(b),carry);if(g9Negative(a)!=g9Negative(b)&&g9Negative(result)!=g9Negative(a)){result=uvec4(0u);return false;}return true;
}
uvec4 g9Magnitude(uvec4 a){return g9Negative(a)?g9NegBits(a):a;}
bool g9FromMagnitude(uvec4 magnitude,bool negative,out uvec4 result){
 result=uvec4(0u);const uvec4 minMagnitude=uvec4(0u,0u,0u,0x80000000u);int cmp=g9UCmp(magnitude,minMagnitude);
 if(cmp>0||!negative&&cmp==0)return false;result=negative&&!g9Zero(magnitude)?g9NegBits(magnitude):magnitude;return true;
}
bool g9UMul(uvec4 a,uvec4 b,out uvec4 result){
 uint aa[8],bb[8],digits[16];for(int i=0;i<8;++i){aa[i]=(a[i/2]>>uint(16*(i%2)))&65535u;bb[i]=(b[i/2]>>uint(16*(i%2)))&65535u;}
 for(int i=0;i<16;++i)digits[i]=0u;
 for(int i=0;i<8;++i){uint carry=0u;for(int j=0;j<8;++j){uint n=digits[i+j]+aa[i]*bb[j]+carry;digits[i+j]=n&65535u;carry=n>>16u;}digits[i+8]=carry;}
 result=uvec4(0u);for(int i=8;i<16;++i)if(digits[i]!=0u)return false;
 for(int i=0;i<4;++i)result[i]=digits[2*i]|(digits[2*i+1]<<16u);return true;
}
bool g9SMul(uvec4 a,uvec4 b,out uvec4 result){uvec4 magnitude;if(!g9UMul(g9Magnitude(a),g9Magnitude(b),magnitude)){result=uvec4(0u);return false;}return g9FromMagnitude(magnitude,g9Negative(a)!=g9Negative(b),result);}
bool g9UShift(uvec4 a,uint n,out uvec4 result){
 result=uvec4(0u);if(n>=128u)return g9Zero(a);
 for(int bit=127;bit>=0;--bit)if(((a[bit/32]>>uint(bit%32))&1u)!=0u){uint target=uint(bit)+n;if(target>=128u){result=uvec4(0u);return false;}result[int(target/32u)]|=1u<<(target%32u);}return true;
}
bool g9SShift(uvec4 a,uint n,out uvec4 result){uvec4 magnitude;if(!g9UShift(g9Magnitude(a),n,magnitude)){result=uvec4(0u);return false;}return g9FromMagnitude(magnitude,g9Negative(a),result);}
bool g9UDiv(uvec4 numerator,uvec4 denominator,out uvec4 quotient,out uvec4 remainder){
 quotient=remainder=uvec4(0u);if(g9Zero(denominator))return false;
 for(int bit=127;bit>=0;--bit){uint high=remainder.w>>31u;remainder=uvec4(remainder.x<<1u,(remainder.y<<1u)|(remainder.x>>31u),(remainder.z<<1u)|(remainder.y>>31u),(remainder.w<<1u)|(remainder.z>>31u));remainder.x|=(numerator[bit/32]>>uint(bit%32))&1u;
  if(high!=0u||g9UCmp(remainder,denominator)>=0){uint carry;remainder=g9AddBits(remainder,g9NegBits(denominator),carry);quotient[bit/32]|=1u<<uint(bit%32);}
 }return true;
}
// rounding: 0 TZ, 1 nearest-even, 2 floor, 3 ceil.
bool g9SDiv(uvec4 numerator,uvec4 denominator,uint rounding,out uvec4 result){
 result=uvec4(0u);uvec4 q,r;uvec4 d=g9Magnitude(denominator);bool negative=g9Negative(numerator)!=g9Negative(denominator);
 if(!g9UDiv(g9Magnitude(numerator),d,q,r))return false;
 bool increase=false;if(!g9Zero(r)){
  if(rounding==1u){uvec4 complement;if(!g9USub(d,r,complement))return false;int cmp=g9UCmp(r,complement);increase=cmp>0||cmp==0&&(q.x&1u)!=0u;}
  else increase=rounding==2u&&negative||rounding==3u&&!negative;
 }
 if(increase&&!g9UAdd(q,g9U32(1u),q))return false;return g9FromMagnitude(q,negative,result);
}
bool g9I32(uvec4 a,out int result){uint sign=(a.x&0x80000000u)!=0u?0xffffffffu:0u;result=0;if(a.y!=sign||a.z!=sign||a.w!=sign)return false;result=int(a.x);return true;}
bool g9I64(uvec4 a,out uvec2 result){uint sign=(a.y&0x80000000u)!=0u?0xffffffffu:0u;result=uvec2(0u);if(a.z!=sign||a.w!=sign)return false;result=a.xy;return true;}
bool g9Uint(uvec4 a,out uint result){result=0u;if(any(notEqual(a.yzw,uvec3(0u))))return false;result=a.x;return true;}
bool g9CeilSqrt64(uvec2 raw,out uint result){
 uvec4 n=g9U64(raw);uint low=0u,high=0xffffffffu;
 for(uint iteration=0u;iteration<32u;++iteration){uint middle=low+(high-low)/2u;uvec4 square;if(!g9UMul(g9U32(middle),g9U32(middle),square))return false;if(g9UCmp(square,n)<0)low=middle+1u;else high=middle;}
 uvec4 square;if(!g9UMul(g9U32(low),g9U32(low),square)||g9UCmp(square,n)<0)return false;result=low;return true;
}
struct G9EnergyInterval {uvec4 low;uvec4 high;};
bool g9FlowKinetic(uvec2 momentum,uint mass,out G9EnergyInterval result){
 result.low=result.high=uvec4(0u);uvec4 p=g9Magnitude(g9S64(momentum));if(mass==0u)return g9Zero(p);
 uvec4 square,q,r,fraction,rest;if(!g9UMul(p,p,square)||!g9UDiv(square,g9U32(mass),q,r)||!g9UShift(q,24u,q)||!g9UShift(r,24u,r)||!g9UDiv(r,g9U32(mass),fraction,rest)||!g9UAdd(q,fraction,result.low))return false;
 result.high=result.low;if(!g9Zero(rest)&&!g9UAdd(result.high,g9U32(1u),result.high))return false;return !g9Negative(result.high);
}
