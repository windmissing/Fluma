// Low-first U128/S128, and U256 product. No float intermediates in pressure.
struct U256 { uvec4 lo; uvec4 hi; };
bool wzero(uvec4 a){return all(equal(a,uvec4(0u)));}
bool wlt(uvec4 a,uvec4 b){for(int i=3;i>=0;--i){if(a[i]!=b[i])return a[i]<b[i];}return false;}
uvec4 wadd(uvec4 a,uvec4 b){uint c=0u;for(int i=0;i<4;++i){uint t=a[i]+b[i],c0=uint(t<a[i]);uint z=t+c;c=c0|uint(z<t);a[i]=z;}return a;}
uvec4 wnot(uvec4 a){return ~a;}
uvec4 wneg(uvec4 a){return wadd(~a,uvec4(1u,0u,0u,0u));}
uvec4 wsub(uvec4 a,uvec4 b){return wadd(a,wneg(b));}
bool wbit(uvec4 a,uint b){return b<128u&&((a[b/32u]>>(b%32u))&1u)!=0u;}
uvec4 wshl(uvec4 a,uint s){if(s>=128u)return uvec4(0u);uvec4 r=uvec4(0u);for(uint i=0u;i<4u;++i){if(i>=s/32u){r[i]=a[i-s/32u]<<(s%32u);if(s%32u!=0u&&i>s/32u)r[i]|=a[i-s/32u-1u]>>(32u-s%32u);}}return r;}
uvec4 wshr(uvec4 a,uint s){if(s>=128u)return uvec4(0u);uvec4 r=uvec4(0u);for(uint i=0u;i<4u;++i){if(i+s/32u<4u){r[i]=a[i+s/32u]>>(s%32u);if(s%32u!=0u&&i+s/32u+1u<4u)r[i]|=a[i+s/32u+1u]<<(32u-s%32u);}}return r;}
int wtop(uvec4 a){for(int i=3;i>=0;--i)if(a[i]!=0u)return i*32+findMSB(a[i]);return -1;}
bool wlow(uvec4 a,uint bits){if(bits>=128u)return !wzero(a);return !wzero(wsub(a,wshl(wshr(a,bits),bits)));}
uvec4 wrne(uvec4 a,uint bits){if(bits==0u)return a;uvec4 q=wshr(a,bits);bool guard=wbit(a,bits-1u);if(guard&&(wlow(a,bits-1u)||(q.x&1u)!=0u))q=wadd(q,uvec4(1u,0u,0u,0u));return q;}
U256 wmul(uvec4 a,uvec4 b){uint r[8];for(int i=0;i<8;++i)r[i]=0u;for(int i=0;i<4;++i)for(int j=0;j<4;++j){uint hi,lo;umulExtended(a[i],b[j],hi,lo);int k=i+j;uint old=r[k];r[k]+=lo;uint carry=uint(r[k]<old);++k;old=r[k];r[k]+=hi;uint c2=uint(r[k]<old);old=r[k];r[k]+=carry;carry=c2|uint(r[k]<old);for(int t=k+1;t<8;++t){old=r[t];r[t]+=carry;carry=uint(r[t]<old);}}return U256(uvec4(r[0],r[1],r[2],r[3]),uvec4(r[4],r[5],r[6],r[7]));}
uvec4 wdiv(uvec4 a,uvec4 b,out uvec4 rem){uvec4 q=uvec4(0u);rem=uvec4(0u);for(int i=127;i>=0;--i){bool over=(rem.w&0x80000000u)!=0u;rem=wshl(rem,1u);rem.x|=uint(wbit(a,uint(i)));if(over||!wlt(rem,b)){rem=wsub(rem,b);q[uint(i)/32u]|=1u<<(uint(i)%32u);}}return q;}
uvec4 wisqrt(uvec4 x){uvec4 root=uvec4(0u);for(int i=63;i>=0;--i){uvec4 q=root;q[uint(i)/32u]|=1u<<(uint(i)%32u);U256 p=wmul(q,q);if(wzero(p.hi)&&!wlt(x,p.lo))root=q;}return root;}
bool sneg(uvec4 a){return (a.w&0x80000000u)!=0u;}
uvec4 sabs(uvec4 a){return sneg(a)?wneg(a):a;}
uvec4 s64(uvec2 a){return uvec4(a,(a.y&0x80000000u)!=0u?uvec2(0xffffffffu):uvec2(0u));}
uvec4 si(int x){return uvec4(uint(x),x<0?uvec3(0xffffffffu):uvec3(0u));}
uvec4 smulsmall(uvec4 a,uint b){bool n=sneg(a);U256 p=wmul(sabs(a),uvec4(b,0u,0u,0u));return n?wneg(p.lo):p.lo;}
uvec4 sdivrne(uvec4 a,uint den){bool n=sneg(a);uvec4 rem;uvec4 q=wdiv(sabs(a),uvec4(den,0u,0u,0u),rem);uint twice=rem.x*2u;if(twice>den||(twice==den&&(q.x&1u)!=0u))q=wadd(q,uvec4(1u,0u,0u,0u));return n?wneg(q):q;}
uvec4 sdivtz(uvec4 a,uint den){uvec4 rem;uvec4 q=wdiv(sabs(a),uvec4(den,0u,0u,0u),rem);return sneg(a)?wneg(q):q;}
bool sfits64(uvec4 a){uint fill=(a.y&0x80000000u)!=0u?0xffffffffu:0u;return a.z==fill&&a.w==fill;}
float sfloat(uvec4 a,int binaryScale){uvec4 mag=sabs(a);int top=wtop(mag);if(top<0)return 0.;int exponent=top+binaryScale;if(exponent>127){fail(17u);return 0.;}uvec4 significand=top>23?wrne(mag,uint(top-23)):wshl(mag,uint(23-top));if(significand.x>=0x1000000u){significand=wshr(significand,1u);++exponent;}if(exponent< -126){int shift= -149-binaryScale;significand=shift>0?wrne(mag,uint(shift)):wshl(mag,uint(-shift));return uintBitsToFloat(significand.x|(sneg(a)?0x80000000u:0u));}uint bits=(uint(exponent+127)<<23u)|(significand.x&0x7fffffu);return uintBitsToFloat(bits|(sneg(a)?0x80000000u:0u));}

uvec4 floatFixed(float value,int fractionBits){uint bits=floatBitsToUint(value);uint raw=(bits>>23u)&255u;if(raw==0u&&((bits&0x7fffffu)==0u))return uvec4(0u);uint sig=(bits&0x7fffffu)|(raw==0u?0u:0x800000u);int shift=(raw==0u?-126:int(raw)-127)-23+fractionBits;uvec4 v=uvec4(sig,0u,0u,0u);v=shift>=0?wshl(v,uint(shift)):wrne(v,uint(-shift));return (bits&0x80000000u)!=0u?wneg(v):v;}
