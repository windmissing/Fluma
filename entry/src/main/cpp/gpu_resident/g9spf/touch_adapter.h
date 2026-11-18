#pragma once
#include "flick_numeric.h"
#include "clock_producer.h"
namespace grs::g9spf::candidate548 {
struct Mapping {ControlIdentity scope{};uint64_t generation=0,commit=0;double left=0,top=0,width=0,height=0;std::array<uint32_t,8> parameter{},seed{};bool current=false;};
struct TouchPoint {uint64_t sample=0,receipt=0;int32_t pointer=0;double px=0,py=0;};
struct Stroke {
 Mapping mapping{};TouchPoint down{},up{};double downX=0,downY=0,upX=0,upY=0;uint64_t sequence=0;uint32_t moves=0;Flick impulse{};
};
inline bool SameMapping(const Mapping& a,const Mapping& b){return a.current&&b.current&&a.scope==b.scope&&a.generation==b.generation&&a.left==b.left&&a.top==b.top&&a.width==b.width&&a.height==b.height&&a.parameter==b.parameter&&a.seed==b.seed;}
class TouchAdapter {
 Stroke stroke{};bool active=false,proof=false;Mapping proved{};uint64_t serial=0,last=0;
 bool Point(const TouchPoint& p,const Mapping& m,double& x,double& y){return p.sample&&p.sample<=p.receipt&&p.sample>=last&&Map(p.px,p.py,m.left,m.top,m.width,m.height,x,y);}
public:
 void Revoke(){active=proof=false;last=0;}
 // The caller must validate platform source/clock proof before granting this
 // scope. No timestamp supplied by JS can grant proof by itself.
 bool AuthorizeClock(const Mapping& scope,const ClockProducer& producer){Revoke();if(!producer.Valid(scope.scope))return false;proved=scope;proof=scope.current;return proof;}
 void Cancel(){active=false;last=0;}
 const char* Down(const TouchPoint& p,const Mapping& m,uint32_t changed,uint32_t touches){
  if(active){Cancel();return "REPEATED_DOWN";}if(!proof||!SameMapping(proved,m))return "INPUT_CLOCK_UNVERIFIED";
  if(changed!=1||touches!=1)return "MULTITOUCH_REJECTED";stroke={};last=0;
  if(!Point(p,m,stroke.downX,stroke.downY))return "ADAPTER_POINT_REJECTED";stroke.mapping=m;stroke.down=p;active=true;last=p.sample;return "DOWN_ACTIVE";
 }
 const char* Move(const TouchPoint& p,const Mapping& m,uint32_t changed,uint32_t touches){double x=0,y=0;
  if(!active)return "IDLE";if(!SameMapping(stroke.mapping,m)||changed!=1||touches!=1||p.pointer!=stroke.down.pointer||++stroke.moves>256||!Point(p,m,x,y)){Cancel();return "STROKE_CANCELLED";}last=p.sample;return "MOVE_ZERO_FORCE";
 }
 const char* Up(const TouchPoint& p,const Mapping& m,uint32_t changed,uint32_t touches,Stroke& out){
  if(!active)return "IDLE";active=false;
  if(!proof||!SameMapping(stroke.mapping,m)||changed!=1||touches>1||p.pointer!=stroke.down.pointer||!Point(p,m,stroke.upX,stroke.upY)||p.receipt-p.sample>250000000)return "STROKE_CANCELLED";
  if(p.sample<stroke.down.sample)return "CLOCK_REVERSED";const auto dt=p.sample-stroke.down.sample;
  volatile double dx=stroke.upX-stroke.downX;volatile double dy=stroke.upY-stroke.downY;const double x=dx,y=dy;uint64_t xb=0,yb=0;std::memcpy(&xb,&x,8);std::memcpy(&yb,&y,8);
  if(!Recipe(xb,yb,dt,stroke.impulse))return "NO_IMPULSE_OR_DURATION_REJECTED";
  const double grid=stroke.upX*1048576;const auto whole=uint64_t(grid);const double remainder=grid-double(whole);if(whole+(remainder>0.5||(remainder==0.5&&(whole&1)))>=128*1048576)return "ADAPTER_TOUCH_GRID_OUT_OF_RANGE";
  if(serial==UINT64_MAX)return "INPUT_SEQUENCE_EXHAUSTED";stroke.up=p;stroke.sequence=++serial;out=stroke;last=0;return "UP_READY_NOT_CONSUMED";
 }
};

// T572 (problem T543 §7, T552 R2): cold-V2 prefix records. Two
// TOUCH_EVIDENCE256 slots at prefix words 3072..3583 (CURRENT_FROZEN_TOUCH /
// BASE_LAST_CONSUMED_TOUCH; absence is all-zero with an explicit kind word),
// Config64 at 3584..3647, SEED_META64 at 3648..3711.
struct TouchEvidence256 {
 uint32_t kind=0; // 0=NONE,1=CURRENT_FROZEN_TOUCH,2=BASE_LAST_CONSUMED_TOUCH
 std::array<uint32_t,256> words{};
};
struct SeedMeta64 {
 std::array<uint32_t,8> templateSha{},actualSha{},parameterSha{};
 uint64_t uploadedBytes=0;uint32_t m0=0,origin=0,sourceId=0,version=2;
};
inline void WriteColdV2Records(uint32_t* prefix,
                               const TouchEvidence256& current,
                               const TouchEvidence256& base,
                               const std::array<uint32_t,64>& config64,
                               const SeedMeta64& seed){
 if(!prefix)return;
 std::memset(prefix+3072,0,2048);
 if(current.kind)std::memcpy(prefix+3072,current.words.data(),1024);
 if(base.kind)std::memcpy(prefix+3328,base.words.data(),1024);
 // Record magic/identity words are never overwritten by presence metadata.
 std::memcpy(prefix+3584,config64.data(),256);
 std::memset(prefix+3648,0,256);
 std::memcpy(prefix+3648,seed.templateSha.data(),32);
 std::memcpy(prefix+3656,seed.actualSha.data(),32);
 std::memcpy(prefix+3664,seed.parameterSha.data(),32);
 Put(prefix+3672,0,seed.uploadedBytes);
 prefix[3674]=seed.m0;prefix[3675]=seed.origin;prefix[3676]=seed.sourceId;prefix[3677]=seed.version;
}
}
