#pragma once
#include "source_transport.h"
namespace grs {
// Called exclusively by the native GL owner. Pump submits/polls at most one root;
// it never sleeps, maps a pool, calls CPU physics, or draws an unaccepted bank.
struct ResidentTickReceipt {
 TickPlan time;uint32_t submitted=0,accepted=0,successfulQuanta=0,discardedQuanta=0;
 bool active=false,complete=false,fault=false;TransportCode code=TransportCode::OK;
 AcceptedRoot latest;
};
class ResidentRun {
 public:
 ResidentRun(const ResidentRun&)=delete;ResidentRun& operator=(const ResidentRun&)=delete;
 explicit ResidentRun(SourceGpuTransport& t,U64 initialNonce={}):transport(t),nonce(initialNonce){}
 ControlCode Offer(const std::array<uint32_t,32>&);
 ControlCode Advance(uint64_t deltaNs,U64 frame,U64 control,int requestedAngle,uint32_t capacity);
 TransportCode Pump();
 ControlCode Pause();
 const ResidentTickReceipt& Tick()const{return receipt;}
 uint32_t Queued()const{return count;}
 uint64_t Offered()const{return offered;}uint64_t Backpressured()const{return backpressured;}
 private:
 void Fail(TransportCode);SourceGpuTransport& transport;Fixed120Clock clock;
 std::array<std::array<uint32_t,32>,16> queue{};uint32_t head=0,count=0,K=0,nextQuantum=1;
 U64 nonce{},frame{},control{},lastFrame{};int angle=0;bool sourcePending=false,inflight=false,sourceFlight=false,quarantined=false;
 uint64_t offered=0,backpressured=0;ResidentTickReceipt receipt;
};
}
