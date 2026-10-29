#pragma once
#include "control.h"
#include <cstring>
#include <cmath>
namespace grs {
inline const char* ValidateBridgePacket(const std::array<uint32_t,32>& w,const Identity& id,U64 input){
 using namespace sourceCommand32;
 if(w[magic]!=wire::magicWords::sourceCommand||w[revision]!=1)return "PACKET_HEADER";
 if(w[kind]!=0||w[flags]!=0||w[subtype]!=0||w[reserved31]!=0)return "PACKET_FEATURE_RESERVED";
 auto pair=[&](size_t at,U64 v){return w[at]==v.lo&&w[at+1]==v.hi;};
 if(!pair(runLo,id.run)||!pair(inputLo,input)||!pair(geometryLo,id.geometry)||!pair(configLo,id.config))return "PACKET_IDENTITY";
 if((w[requestLo]|w[requestHi])==0||(w[gestureLo]|w[gestureHi])==0||w[sourceId]==0||w[requestSeq]==0||(w[clockWindowLo]|w[clockWindowHi])==0||w[timeUnit]>1)return "PACKET_REQUIRED_FIELDS";
 double x,y,q;std::memcpy(&x,w.data()+xRawLo,8);std::memcpy(&y,w.data()+yRawLo,8);std::memcpy(&q,w.data()+volumeRawLo,8);
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(q)||x<0||x>=128||y<0||y>256||q<=0||q>256)return "PACKET_RAW_RANGE";
 return "OK";
}
}
