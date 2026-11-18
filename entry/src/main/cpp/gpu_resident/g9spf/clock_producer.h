#pragma once
#include "research_config.h"
#include <time.h>
namespace grs::g9spf::candidate548 {
struct ProducerProof {
 ControlIdentity scope{};std::array<uint32_t,8> osBuild{},producerSource{},conversionSource{},nativeClockSource{};
 uint64_t generation=0;uint32_t profile=0;bool deploymentChainVerified=false;
};
// Native-only qualification. This type is not a NAPI input or JS boolean.
class ClockProducer {
 ProducerProof proof_{};uint64_t serial_=0;bool verified_=false;
 static bool Named(const std::array<uint32_t,8>& sha){for(auto w:sha)if(w)return true;return false;}
public:
 static uint64_t Now(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)!=0||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;if(uint64_t(t.tv_sec)>(UINT64_MAX-uint64_t(t.tv_nsec))/1000000000)return 0;return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);}
 bool InstallVerifiedProducerProof(const ProducerProof& p){
  Revoke();if(!p.deploymentChainVerified||p.profile!=1||!p.scope.run||!p.scope.context||!p.scope.inputEpoch||!Named(p.osBuild)||!Named(p.producerSource)||!Named(p.conversionSource)||!Named(p.nativeClockSource)||!Now()||serial_==UINT64_MAX)return false;
  proof_=p;proof_.generation=++serial_;verified_=true;return true;
 }
 void Revoke(){verified_=false;proof_={};}
 bool Valid(const ControlIdentity& scope)const{return verified_&&scope==proof_.scope;}
 uint64_t Generation()const{return verified_?proof_.generation:0;}
 const ProducerProof& Proof()const{return proof_;}
};
}
