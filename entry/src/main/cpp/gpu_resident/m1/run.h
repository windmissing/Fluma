#pragma once
#include "full_transport.h"
#include "../run.h"
namespace grs {namespace m1 {
// Native owner controller; clock/FIFO math matches the frozen fixed120 run.
class Run {
public:
 explicit Run(FullM1Transport& t):transport(t){}
 ControlCode Offer(const std::array<uint32_t,32>&);
 ControlCode Advance(uint64_t,U64,U64,int,uint32_t);
 TransportCode Pump();ControlCode Pause();const ResidentTickReceipt& Tick()const{return receipt;}uint32_t Queued()const{return count;}
 // T228 UI boundaries do not call terminal transport.Stop(). A root already
 // submitted drains once; queued/unsubmitted sources and future quanta stop.
 // T324 §2.3: HasFlight covers BOTH unsealed submitting and sealed inflight
 // (Pause drain and pause-window Poll must see either); Submitting exposes
 // the partial-submit phase for bridge telemetry.
 uint32_t DiscardSources();void Suspend();void Resume();bool HasFlight()const{return inflight||submitting;}bool SourceInFlight()const{return (inflight||submitting)&&sourceFlight;}bool Submitting()const{return submitting;}
private:
 void Fail(TransportCode);FullM1Transport& transport;Fixed120Clock clock;QuantumParameters params;
 std::array<std::array<uint32_t,32>,16> queue{};uint32_t head=0,count=0,K=0,nextQuantum=1;
 U64 nonce{},frame{},control{},lastFrame{};int angle=0;bool sourcePending=false,inflight=false,sourceFlight=false,quarantined=false,suspended=false;bool submitting=false;
 uint64_t offered=0,backpressured=0;ResidentTickReceipt receipt;
};
}}
