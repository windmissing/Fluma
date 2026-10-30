#pragma once
#include "scene.h"
#include <algorithm>
#include <cmath>
namespace grs {namespace m1 {
// GRS_M1_DEMO_UI_V4 control epoch is separate from GPU inputGeneration.
// Stop closes admission immediately; a submitted root drains before PAUSED.
struct UiState {
 bool enabled=false,sensorMode=true,sampleValid=false;int source=-1;
 uint64_t boundary=1,applied=0,sampleNs=0;uint32_t sampleSeq=0;
 double rawAngle=0,shake=0;int target=0;
 uint64_t bufferBytes=0,frameBytes=0,knownGpuPeak=0;
 uint32_t receipt=0,admitted=0,rejected=0,request=0;int acceptedAngle=0;
 U64 settled{},flight{},sand{},water{};
 void NewRun(){SetSource(-1);applied=0;receipt=admitted=rejected=request=0;acceptedAngle=0;settled=flight=sand=water={};bufferBytes=frameBytes=0;}
 bool SetSource(int kind){if(kind< -1||kind>1)return false;if(boundary==UINT64_MAX){source=-1;applied=0;return false;}source=kind;++boundary;return true;}
 bool SetMode(bool sensor){if(!SetSource(-1))return false;enabled=true;sensorMode=sensor;sampleValid=false;sampleSeq=0;return true;}
 bool Sample(double raw,double magnitude,uint32_t seq,uint64_t ns){if(!std::isfinite(raw)||std::abs(raw)>89||!std::isfinite(magnitude)||magnitude<0||magnitude>1||seq<=sampleSeq)return false;rawAngle=raw;shake=magnitude;sampleSeq=seq;sampleNs=ns;target=int(std::round(std::max(-30.,std::min(30.,raw))*256));sampleValid=true;return true;}
 bool Fresh(uint64_t ns)const{return !enabled||!sensorMode||(sampleValid&&ns>=sampleNs&&ns-sampleNs<=250000000u);}
 bool SourceReady(uint64_t ns)const{return source>=0&&applied==boundary&&Fresh(ns);}
 const char* Offer(const std::array<uint32_t,32>& w,uint64_t ns)const{if(enabled&&(!SourceReady(ns)||w[2]!=uint32_t(source)||w[10]!=uint32_t(boundary)||w[11]!=uint32_t(boundary>>32)))return "SOURCE_CLOSED_OR_STALE";return "OK";}
 void Memory(uint64_t buffers,uint64_t frames){bufferBytes=buffers;frameBytes=frames;knownGpuPeak=std::max(knownGpuPeak,buffers+frames);}
 void Completion(const Words256& c){sand={c[completion256::sandAfterLo],c[completion256::sandAfterHi]};water={c[completion256::waterAfterLo],c[completion256::waterAfterHi]};settled={c[completion256::settledAfterLo],c[completion256::settledAfterHi]};flight={c[completion256::flightAfterLo],c[completion256::flightAfterHi]};acceptedAngle=int32_t(c[completion256::angleAcceptedIndex]);if(c[completion256::rootKind]==1){receipt=c[completion256::receiptStatus];admitted=c[completion256::admittedUnits];rejected=c[completion256::rejectedUnits];request=c[completion256::requestSeq];}}
};
inline bool ValidOptics(const SceneOptics& optics){const auto& o=optics.values;for(float v:o)if(!std::isfinite(v))return false;if(o[0]<0||o[5]!=1||o[6]!=1||(o[7]!=0||std::signbit(o[7]))||o[8]<=0||o[8]>8||o[13]<1||o[13]>64||std::abs(std::hypot(o[14],o[15])-1)>1e-6)return false;for(int i:{1,2,3,4,9,10,11,12})if(o[i]<0||o[i]>1)return false;return true;}
}}
