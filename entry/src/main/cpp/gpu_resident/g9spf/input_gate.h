#pragma once
#include "research_config.h"
#include <cmath>
namespace grs::g9spf {
inline bool SameCommand(const Input128& a,const Input128& b){
 for(size_t i=40;i<128;++i)if(a[i]!=b[i])return false;
 const size_t also[]={4,5,12,13,26,27,28,29,30,31,32,33};for(auto i:also)if(a[i]!=b[i])return false;return true;
}
class InputGate {
 ControlIdentity identity_{};Config config_;uint64_t sample_=0;
 Input128 pose_{};bool ready_=false;
 static bool Age(uint64_t stamp,uint64_t now){return stamp<=now&&now-stamp<=250000000;}
public:
 bool RealEnvelope(const Input128& body,Input128& out)const{
  if(!config_.realTouch||!ready_||sample_==UINT64_MAX)return false;
  out=body;for(size_t i=0;i<40;++i)out[i]=pose_[i];out[1]=2;Put(out.data(),14,sample_+1);Put(out.data(),18,0);Put(out.data(),20,0);return true;
 }
 bool ModelEnvelope(const Input128& body,int32_t angle,uint64_t now,Input128& out)const{
  if(!config_.synthetic||sample_==UINT64_MAX)return false;out=body;out[0]=kInputMagic;out[1]=1;out[2]=kProfileCode;Put(out.data(),4,identity_.run);Put(out.data(),6,identity_.owner);Put(out.data(),8,identity_.context);Put(out.data(),12,identity_.inputEpoch);Put(out.data(),14,sample_+1);Put(out.data(),16,now);out[22]=out[23]=2;out[24]=uint32_t(angle);out[25]=config_.revision;for(size_t i=0;i<8;++i)out[26+i]=config_.parameterSha[i];if(out[40])Put(out.data(),43,now);return true;
 }
 void Configure(const Config& c,const ControlIdentity& id){config_=c;identity_=id;sample_=0;pose_={};ready_=false;}
 Result Offer(Input128& raw,uint64_t now){
  if(!config_.research)return Result::CALIBRATION_CONFIG_REQUIRED;
  if(raw[0]!=kInputMagic||raw[1]!=(config_.realTouch?2u:1u)||raw[2]!=kProfileCode||Pair(raw.data(),4)!=identity_.run||Pair(raw.data(),6)!=identity_.owner||Pair(raw.data(),8)!=identity_.context||Pair(raw.data(),12)!=identity_.inputEpoch||raw[25]!=config_.revision||raw[38]||raw[39]||raw[40]>2||Pair(raw.data(),10)||Pair(raw.data(),18)||Pair(raw.data(),20))return Result::INVALID;
  for(size_t i=0;i<8;++i)if(raw[26+i]!=config_.parameterSha[i])return Result::INVALID;
  // This initial implementation admits explicitly identified synthetic model
  // inputs only. No pixel gain or sensor-clock proof is inferred here.
  if((!config_.synthetic&&!config_.realTouch)||raw[22]!=2||raw[23]!=2)return Result::INPUT_CLOCK_UNVERIFIED;
  if(config_.realTouch&&(raw[40]!=2u||raw[77]!=53401u||(raw[45]&4u)==0u||(raw[45]&~7u)))return Result::INVALID;
  if(!Pair(raw.data(),14)||Pair(raw.data(),14)<=sample_||sample_==UINT64_MAX||!Age(Pair(raw.data(),16),now))return Result::STALE;
  if(raw[40]&&(!Pair(raw.data(),41)||!Age(Pair(raw.data(),43),now)))return Result::STALE;
  if(int32_t(raw[24])< -7680||int32_t(raw[24])>7680)return Result::INVALID;
  Put(raw.data(),18,now);sample_=Pair(raw.data(),14);pose_=raw;pose_[40]=0;for(size_t i=41;i<128;++i)pose_[i]=0;ready_=true;return Result::OK;
 }
 // Native/manual research input is an explicit control event. Its pose clock
 // is native-owned and never described as a sensor capture timestamp.
 void Pose(int32_t angle,uint64_t now){
  pose_={};pose_[0]=kInputMagic;pose_[1]=config_.realTouch?2u:1u;pose_[2]=kProfileCode;Put(pose_.data(),4,identity_.run);Put(pose_.data(),6,identity_.owner);Put(pose_.data(),8,identity_.context);Put(pose_.data(),12,identity_.inputEpoch);if(sample_!=UINT64_MAX)++sample_;Put(pose_.data(),14,sample_);Put(pose_.data(),16,now);Put(pose_.data(),18,now);pose_[22]=2;pose_[23]=2;pose_[24]=uint32_t(angle);pose_[25]=config_.revision;for(size_t i=0;i<8;++i)pose_[26+i]=config_.parameterSha[i];ready_=true;
 }
 Result Freeze(const Input128* command,uint64_t nonce,uint64_t now,Input128& out)const{
  if(!ready_||!Age(Pair(pose_.data(),16),now))return Result::STALE;
  out=command?*command:pose_;
  if(command&&!Age(Pair(command->data(),43),now))return Result::STALE;
  // Share the frozen command body but retain current authorized pose envelope.
  if(command)for(size_t i=0;i<40;++i)out[i]=pose_[i];
  Put(out.data(),10,nonce);Put(out.data(),20,now);return Result::OK;
 }
};
}
