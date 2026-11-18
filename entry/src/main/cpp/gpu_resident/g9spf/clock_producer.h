#pragma once
#include "research_config.h"
#include <time.h>
#if __has_include("g9_clock_source.h")
#include "g9_clock_source.h"
#endif
namespace grs::g9spf::candidate548 {
struct ProducerProof {
 ControlIdentity scope{};std::array<uint32_t,8> osBuild{},producerSource{},conversionSource{},nativeClockSource{};
 uint64_t generation=0;uint32_t profile=0;bool deploymentChainVerified=false;
};
// Optional native platform integration. A provider must verify the deployed
// vendor build and full conversion chain; absence is an explicit UNKNOWN.
// No JS/NAPI setter, timestamp fitting, or successful-swap authorization.
#if defined(__clang__) || defined(__GNUC__)
bool LoadQualifiedPlatformClockProof(const ControlIdentity&,ProducerProof&) __attribute__((weak));
#endif
inline bool ReadQualifiedPlatformClockProof(const ControlIdentity& scope,ProducerProof& out){
#if (defined(__clang__) || defined(__GNUC__)) && defined(GRS_G9_NATIVE_CLOCK_SOURCE_SHA)
 if(!LoadQualifiedPlatformClockProof||!LoadQualifiedPlatformClockProof(scope,out)||out.scope!=scope)return false;
 const char* hex=GRS_G9_NATIVE_CLOCK_SOURCE_SHA;std::array<uint32_t,8> expected{};auto nibble=[](char c){return uint32_t(c<='9'?c-'0':c-'a'+10);};for(size_t i=0;i<32;++i)expected[i/4]|=((nibble(hex[2*i])<<4)|nibble(hex[2*i+1]))<<(8*(i%4));
 return expected==out.nativeClockSource;
#else
 (void)scope;(void)out;return false;
#endif
}
// Native-only qualification. This type is not a NAPI input or JS boolean.
class ClockProducer {
 ProducerProof proof_{};uint64_t serial_=0;bool verified_=false;
 static bool Named(const std::array<uint32_t,8>& sha){for(auto w:sha)if(w)return true;return false;}
public:
 static uint64_t Now(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)!=0||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;if(uint64_t(t.tv_sec)>(UINT64_MAX-uint64_t(t.tv_nsec))/1000000000)return 0;return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);}
 bool InstallVerifiedProducerProof(const ProducerProof& p){
  Revoke();if(!p.deploymentChainVerified||p.profile!=1||!p.scope.run||!p.scope.owner||!p.scope.context||!p.scope.inputEpoch||!p.scope.pageSession||!p.scope.permission||!p.scope.window||!p.scope.config||!p.scope.surface||!Named(p.osBuild)||!Named(p.producerSource)||!Named(p.conversionSource)||!Named(p.nativeClockSource)||!Now()||serial_==UINT64_MAX)return false;
  proof_=p;proof_.generation=++serial_;verified_=true;return true;
 }
 void Revoke(){verified_=false;proof_={};}
 bool Valid(const ControlIdentity& scope)const{return verified_&&scope==proof_.scope;}
 uint64_t Generation()const{return verified_?proof_.generation:0;}
 const ProducerProof& Proof()const{return proof_;}
};
}
