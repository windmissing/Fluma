#pragma once
#include "pair_prepare_v1.h"
#include <chrono>
#include <string>

namespace p011gpu {
enum class ExecStatus {OK,INPUT_INVALID,CAPACITY_UNSUPPORTED,ENV_UNSUPPORTED,GL_ERROR,TIMEOUT,
 WAIT_FAILED,MAP_FAILED,UNMAP_FAILED,STALE,CANCELLED,PROTOCOL_ERROR};
struct RequestDiagnostics {
 std::string stage="NONE",detail="NONE",waitReturn="NOT_RUN";
 uint32_t rawGlError=0,dispatchCount=0,waitCalls=0,contextEpoch=0;
 double uploadCpuMs=0,dispatchCpuMs=0,fenceWaitCpuMs=0,mapCpuMs=0,requestCpuMs=0;
 bool gpuTimerMeasured=false,valid=false;
};
struct Deadline {std::chrono::steady_clock::time_point at;};
class ContextSlot {
public:
 ContextSlot();~ContextSlot();ContextSlot(const ContextSlot&)=delete;ContextSlot& operator=(const ContextSlot&)=delete;
 ExecStatus initialize(uint32_t nmax,const std::string& shaderDirectory,RequestDiagnostics&);
 ExecStatus executeIntegerPair(const PreparedView&,uint32_t slotIndex,Deadline,Result&,RequestDiagnostics&);
 void cancelOwner(uint64_t ownerGeneration);void reset();
 uint64_t contextEpoch()const{return epoch_;}
 uint32_t liveBufferCount()const{return liveBuffers_;}
 uint32_t peakBufferCount()const{return peakBuffers_;}
private:
 struct Opaque;Opaque* impl_;uint64_t epoch_=0,owner_=0;uint32_t nmax_=0,K_=0,h_=0;
 uint32_t liveBuffers_=0,peakBuffers_=0;bool initialized_=false,quarantined_=false;
};
}
