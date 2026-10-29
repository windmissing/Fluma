#include "run.h"
#include <cassert>
#include <iostream>
// Control-plane double only. No claim about GPU audit, fences or real device.
static bool flight=false,cancelled=false;static uint32_t polls=0,accepts=0;
namespace grs {
TransportCode SourceGpuTransport::Submit(const std::array<uint32_t,32>&,RootRequest r){pending=r;flight=true;return TransportCode::OK;}
TransportCode SourceGpuTransport::SubmitQuantum(RootRequest r,int){pending=r;flight=true;return TransportCode::OK;}
TransportCode SourceGpuTransport::Poll(AcceptedRoot& out){if(!flight)return TransportCode::NOT_READY;if(++polls%2)return TransportCode::BUSY;flight=false;if(cancelled)return TransportCode::PROTOCOL_REJECTED;out.nonce=pending.nonce;accepts++;return TransportCode::OK;}
ControlCode SourceGpuTransport::Stop(){input.lo++;cancelled=flight;return flight?ControlCode::QUARANTINED:ControlCode::OK;}
}
int main(){using namespace grs;Fixed120Clock c;TickPlan p;assert(c.Plan(0,p)==ControlCode::OK&&p.quanta==0);assert(c.Plan(8000000,p)==ControlCode::OK&&p.quanta==0);assert(c.Plan(8000000,p)==ControlCode::OK&&p.carryAfter==960000000);assert(c.Plan(33333334,p)==ControlCode::INVALID_INPUT);assert(c.Plan(8000000,p)==ControlCode::OK&&p.quanta==0&&p.carryAfter==0);
 SourceGpuTransport t;ResidentRun r(t);std::array<uint32_t,32> command{};
 for(int i=0;i<16;i++)assert(r.Offer(command)==ControlCode::OK);assert(r.Offer(command)==ControlCode::BACKPRESSURE&&r.Queued()==16);
 assert(r.Advance(1,{1,0},{1,0},0,192)==ControlCode::OK);assert(r.Pump()==TransportCode::OK);assert(r.Tick().accepted==0);assert(r.Pump()==TransportCode::BUSY);assert(r.Pump()==TransportCode::OK&&accepts==1&&r.Queued()==15);
 assert(r.Advance(8000000,{2,0},{2,0},0,192)==ControlCode::OK);assert(r.Pump()==TransportCode::OK);auto before=t.InputGeneration();assert(r.Pause()==ControlCode::QUARANTINED);assert(t.InputGeneration().lo==before.lo+1&&r.Queued()==0&&r.Tick().fault);assert(r.Pump()==TransportCode::BUSY);assert(r.Pump()==TransportCode::PROTOCOL_REJECTED&&accepts==1);
 std::cout<<"{\"passed\":14,\"failed\":0,\"scope\":\"REAL_CLOCK_RUN_WITH_TRANSPORT_DOUBLE\",\"acceptedPrefixRetained\":1,\"noWallClockCatchup\":true}\n";
}
