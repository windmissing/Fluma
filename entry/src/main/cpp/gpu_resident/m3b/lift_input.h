#pragma once
#include "protocol_generated.h"
#include <array>
#include <cmath>
#include <cstring>
namespace grs::m3b {
enum class LiftMode:uint32_t {OFF=0,SENSOR_RESEARCH=1,SYNTHETIC_TEST=2};
enum class InputResult {OK,CALIBRATION_CONFIG_REQUIRED,BAD_ENVELOPE,SENSOR_CLOCK_UNVERIFIED,STALE_INPUT,TIMESTAMP_PRECISION_UNSUPPORTED};
struct LiftParameters {
 LiftMode mode=LiftMode::OFF;
 uint32_t revision=0,liftRateQ16=0,bandQ=0,rampQ16=0,decayQ16=0,liftSpeedQ=0,liftCapQ=0;
 std::array<uint32_t,8> sha{};
 bool Valid()const{
  bool named=false;for(auto v:sha)named|=v!=0;
  return uint32_t(mode)<=2&&revision&&named&&liftRateQ16<=62914560&&bandQ&&bandQ<=1048576&&
         rampQ16&&rampQ16<=7864320&&decayQ16&&decayQ16<=62914560&&liftSpeedQ&&liftSpeedQ<=67108864&&liftCapQ<=314572800;
 }
};
using LiftInput64=std::array<uint32_t,64>;
inline uint64_t InputU64(const LiftInput64& w,size_t i){return uint64_t(w[i])|(uint64_t(w[i+1])<<32);}
inline void InputPut64(LiftInput64& w,size_t i,uint64_t v){w[i]=uint32_t(v);w[i+1]=uint32_t(v>>32);}
inline double InputDouble(const LiftInput64& w,size_t i){const auto bits=InputU64(w,i);double value;std::memcpy(&value,&bits,sizeof(value));return value;}
struct LiftInputIdentity {uint64_t inputEpoch=0,pageSession=0,owner=0,run=0,context=0;};
// This native gate never supplies a sensor mapping on the basis of a bracket
// measurement alone. A device/SDK clock certificate is an external authority.
class LiftInputGate {
 LiftInputIdentity identity_{};LiftParameters parameters_{};LiftInput64 pending_{},frozen_{};
 uint32_t sequence_=0;uint64_t frozenPlan_=0;bool ready_=false,clockVerified_=false,syntheticOwner_=false;
 InputResult Reject(InputResult reason){if(parameters_.mode!=LiftMode::OFF){ready_=false;frozenPlan_=0;}return reason;}
 static bool Age(uint64_t capture,uint64_t now){return capture<=now&&now-capture<=250000000ULL;}
public:
 InputResult Configure(const LiftInputIdentity& identity,const LiftParameters& p,bool syntheticOwner){
  if(!p.Valid())return InputResult::CALIBRATION_CONFIG_REQUIRED;
  if(!identity.inputEpoch||!identity.pageSession||!identity.owner||!identity.run||!identity.context)return InputResult::BAD_ENVELOPE;
  identity_=identity;parameters_=p;sequence_=0;frozenPlan_=0;ready_=clockVerified_=false;syntheticOwner_=syntheticOwner;
  if(p.mode==LiftMode::OFF){
   pending_={};pending_[0]=893994572;pending_[1]=5;pending_[15]=p.revision;
   InputPut64(pending_,4,identity.inputEpoch);InputPut64(pending_,40,identity.pageSession);InputPut64(pending_,42,identity.owner);InputPut64(pending_,44,identity.run);InputPut64(pending_,46,identity.context);
   for(size_t i=0;i<8;++i)pending_[48+i]=p.sha[i];ready_=true;
  }return InputResult::OK;
 }
 // Only the native clock verifier may call this after a formal certificate.
 void InvalidateClock(){clockVerified_=false;ready_=parameters_.mode==LiftMode::OFF;frozenPlan_=0;}
 InputResult Offer(const LiftInput64& raw,uint64_t nativeReceipt){
  if(!parameters_.Valid())return Reject(InputResult::CALIBRATION_CONFIG_REQUIRED);
  if(parameters_.mode==LiftMode::OFF)return Reject(InputResult::BAD_ENVELOPE);
  if(raw[0]!=893994572||raw[1]!=5||raw[3]&~3u||!raw[6]||raw[6]<=sequence_||sequence_==UINT32_MAX||raw[7]==0||
     InputU64(raw,4)!=identity_.inputEpoch||raw[15]!=parameters_.revision||InputU64(raw,40)!=identity_.pageSession||
     InputU64(raw,42)!=identity_.owner||InputU64(raw,44)!=identity_.run||InputU64(raw,46)!=identity_.context||InputU64(raw,10)||InputU64(raw,12))return Reject(InputResult::BAD_ENVELOPE);
  for(size_t i=48;i<56;++i)if(raw[i]!=parameters_.sha[i-48])return Reject(InputResult::BAD_ENVELOPE);
  for(size_t i=56;i<64;++i)if(raw[i])return Reject(InputResult::BAD_ENVELOPE);
  for(size_t i=16;i<40;i+=2)if(!std::isfinite(InputDouble(raw,i)))return Reject(InputResult::BAD_ENVELOPE);
  if(InputDouble(raw,18)<0||InputDouble(raw,18)>1)return Reject(InputResult::BAD_ENVELOPE);
  if(parameters_.mode==LiftMode::SENSOR_RESEARCH){
   if(raw[2]!=1||raw[14]!=1||raw[3]!=3||!clockVerified_)return Reject(InputResult::SENSOR_CLOCK_UNVERIFIED);
  }else if(raw[2]!=2||raw[14]!=2||raw[3]!=2||!syntheticOwner_)return Reject(InputResult::BAD_ENVELOPE);
  if(!Age(InputU64(raw,8),nativeReceipt))return Reject(InputResult::STALE_INPUT);
  pending_=raw;InputPut64(pending_,10,nativeReceipt);sequence_=raw[6];ready_=true;return InputResult::OK;
 }
 InputResult Freeze(uint64_t plan,uint64_t rootNow,const LiftInput64*& out){
  out=nullptr;if(!plan||!ready_)return InputResult::STALE_INPUT;
  if(frozenPlan_!=plan){frozen_=pending_;if(parameters_.mode!=LiftMode::OFF)InputPut64(frozen_,12,rootNow);frozenPlan_=plan;}
  // Revalidate at each root even when one plan intentionally shares a sample.
  if(parameters_.mode!=LiftMode::OFF&&(!Age(InputU64(frozen_,8),rootNow)||rootNow<InputU64(frozen_,10)))return InputResult::STALE_INPUT;
  out=&frozen_;return InputResult::OK;
 }
};
}
