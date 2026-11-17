#pragma once
#include "research_config.h"
#include <cmath>
namespace grs::g9spf::candidate548 {
using U128=unsigned __int128;
struct Flick {int64_t dxQ=0,dyQ=0,jx=0;uint64_t jy=0;U128 cap=0;uint32_t flags=4;};
inline uint64_t Rne(U128 n,uint64_t d){const U128 q=n/d,r=n%d;return uint64_t(q+(r*2>d||(r*2==d&&(q&1))));}
inline bool Delta(uint64_t bits,uint64_t maximum,int64_t& out){
 const auto exponent=uint32_t((bits>>52)&2047);const auto mantissa=bits&((1ULL<<52)-1);if(exponent==2047)return false;
 if(!exponent){out=0;return true;}const int shift=int(exponent)-1023-52+20;U128 value=(U128(1)<<52)|mantissa;
 if(shift>=0){if(shift>63)return false;value<<=shift;}else if(-shift>=128)value=0;else value>>=-shift;
 if(value>maximum)return false;out=int64_t(value);if(bits>>63)out=-out;return true;
}
inline bool Recipe(uint64_t dxBits,uint64_t dyBits,uint64_t dt,Flick& out){
 if(dt<8333334||dt>1000000000)return false;
 double dx=0,dy=0;std::memcpy(&dx,&dxBits,8);std::memcpy(&dy,&dyBits,8);
 if(!std::isfinite(dx)||!std::isfinite(dy)||std::abs(dx)>128||std::abs(dy)>256||!Delta(dxBits,128ULL*1048576,out.dxQ)||!Delta(dyBits,256ULL*1048576,out.dyQ))return false;
 const uint64_t ax=out.dxQ<0?uint64_t(-out.dxQ):uint64_t(out.dxQ),ay=out.dyQ<0?uint64_t(-out.dyQ):uint64_t(out.dyQ);
 if(U128(ax)*ax+U128(ay)*ay<U128(262144)*262144)return false;
 const auto rawX=Rne(U128(ax)*1000000000,4*dt),rawY=Rne(U128(out.dyQ>0?out.dyQ:0)*1000000000,4*dt);
 out.flags=4|(rawX>24*1048576?1:0)|(rawY>32*1048576?2:0);
 const uint64_t vx=std::min<uint64_t>(24*1048576,rawX),vy=std::min<uint64_t>(32*1048576,rawY);
 out.jx=int64_t(65536*vx)*(out.dxQ<0?-1:1);out.jy=65536*vy;out.cap=U128(8)*65536*(U128(vx)*vx+U128(vy)*vy)*(1<<24);return out.jx||out.jy;
}
inline bool Map(double px,double py,double left,double top,double width,double height,double& x,double& y){
 if(!std::isfinite(px)||!std::isfinite(py)||!std::isfinite(left)||!std::isfinite(top)||!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0||px<left||py<top||px>=left+width||py>=top+height)return false;
 volatile double tx=px-left;volatile double rx=tx/width;x=rx*128;volatile double bottom=top+height;volatile double ty=bottom-py;volatile double ry=ty/height;y=ry*256;return true;
}
}
