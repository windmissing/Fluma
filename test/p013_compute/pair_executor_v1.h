#pragma once
#include "pair_prepare_v1.h"
#include <chrono>
#include <string>

namespace p011gpu {
enum class ExecStatus {OK,INPUT_INVALID,CAPACITY_UNSUPPORTED,ENV_UNSUPPORTED,GL_ERROR,TIMEOUT,
 WAIT_FAILED,MAP_FAILED,UNMAP_FAILED,STALE,CANCELLED,PROTOCOL_ERROR};
inline const char* execStatusName(ExecStatus s){switch(s){
#define CASE(X) case ExecStatus::X:return #X;
 CASE(OK) CASE(INPUT_INVALID) CASE(CAPACITY_UNSUPPORTED) CASE(ENV_UNSUPPORTED) CASE(GL_ERROR) CASE(TIMEOUT) CASE(WAIT_FAILED) CASE(MAP_FAILED) CASE(UNMAP_FAILED) CASE(STALE) CASE(CANCELLED) CASE(PROTOCOL_ERROR)
#undef CASE
 }return "UNKNOWN";}
struct RequestDiagnostics {
 std::string stage="NONE",detail="NONE",waitReturn="NOT_RUN";
 uint32_t rawGlError=0,rawEglError=0,rawWaitReturn=0,dispatchCount=0,waitCalls=0;uint64_t contextEpoch=0;
 std::string resetStatus="NOT_RUN";bool resourcesRetired=false;
 double uploadCpuMs=0,dispatchCpuMs=0,fenceWaitCpuMs=0,mapCpuMs=0,requestCpuMs=0;
 bool preserveGlError(uint32_t observed){if(rawGlError==0)rawGlError=observed;return observed!=0;}
 bool gpuTimerMeasured=false,valid=false;
};
struct Deadline {std::chrono::steady_clock::time_point at;};
class ContextSlot {
public:
 ContextSlot();~ContextSlot();ContextSlot(const ContextSlot&)=delete;ContextSlot& operator=(const ContextSlot&)=delete;
 ExecStatus initialize(uint32_t nmax,const std::string& shaderDirectory,RequestDiagnostics&,const std::array<std::string,3>* expectedShaderSha=nullptr);
 ExecStatus executeIntegerPair(const PreparedView&,uint32_t slotIndex,Deadline,Result&,RequestDiagnostics&);
 void cancelOwner(uint64_t ownerGeneration);ExecStatus reset(RequestDiagnostics* diagnostics=nullptr);
 uint32_t slotState(uint32_t slot)const;bool quarantined()const{return quarantined_;}
 uint64_t contextEpoch()const{return epoch_;}
 uint32_t liveBufferCount()const{return liveBuffers_;}
 uint32_t peakBufferCount()const{return peakBuffers_;}
private:
 struct Opaque;Opaque* impl_;uint64_t epoch_=0,owner_=0;uint32_t nmax_=0,K_=0,h_=0;
 uint32_t liveBuffers_=0,peakBuffers_=0;bool initialized_=false,quarantined_=false;
};
}
