#include "m1/run.h"
#include "bridge_state.h"
#include "m1/ui_state.h"
#include <cassert>
#include <iostream>
using namespace grs;
static RootRequest submitted;static bool flight=false;static uint32_t sourceCalls=0,quantumCalls=0,polls=0;
namespace grs {namespace m1 {
TransportCode FullM1Transport::SubmitSource(const std::array<uint32_t,32>&,RootRequest r){assert(r.kind==1&&r.quantumOrdinal==0);submitted=r;sourceCalls++;flight=true;return TransportCode::OK;}
TransportCode FullM1Transport::SubmitQuantum(const QuantumParameters& p,RootRequest r){assert(r.kind==2&&r.quantumOrdinal>=1&&r.quantumOrdinal<=4&&p.requestedAngle==0);submitted=r;quantumCalls++;flight=true;return TransportCode::OK;}
TransportCode FullM1Transport::Poll(AcceptedRoot& out){if(!flight)return TransportCode::NOT_READY;if(++polls%2)return TransportCode::BUSY;flight=false;out.nonce=submitted.nonce;return TransportCode::OK;}
ControlCode FullM1Transport::Stop(){assert(false);return ControlCode::OK;} // ordinary UI never invokes terminal Stop
}}
int main(){int passed=0;m1::UiState ui;assert(ui.SetMode(true)&&!ui.Fresh(1));passed++;
 assert(ui.Sample(60,.5,1,100)&&ui.target==7680&&ui.rawAngle==60&&ui.Fresh(250000100)&&!ui.Fresh(250000101));passed++;
 assert(!ui.Sample(0,0,1,101)&&!ui.Sample(NAN,0,2,101)&&!ui.Sample(0,2,2,101));passed++;
 std::array<uint32_t,32> packet{};ui.SetSource(1);ui.applied=ui.boundary;packet[2]=1;packet[10]=uint32_t(ui.boundary);assert(std::string(ui.Offer(packet,101))=="OK");ui.SetSource(-1);assert(std::string(ui.Offer(packet,101))=="SOURCE_CLOSED_OR_STALE");passed++;
 m1::SceneOptics cold;assert(m1::ValidOptics(cold));cold.values[7]=-0.f;assert(!m1::ValidOptics(cold));cold.values[7]=1;assert(!m1::ValidOptics(cold));cold={};cold.values[0]=-1;assert(!m1::ValidOptics(cold));passed++;
 Words256 c{};c[140]=3;c[144]=4;c[75]=7;c[107]=uint32_t(-256);c[27]=1;c[61]=9;c[63]=1;ui.Completion(c);assert(ui.settled.lo==3&&ui.flight.lo==4&&ui.water.lo==7&&ui.acceptedAngle==-256&&ui.admitted==9);ui.Memory(100,20);ui.Memory(50,10);assert(ui.knownGpuPeak==120);ui.NewRun();assert(ui.water.lo==0&&ui.frameBytes==0&&ui.knownGpuPeak==120);passed++;
 // Stop before submit drops a queued birth but still executes physics.
 {m1::FullM1Transport t;m1::Run run(t);auto before=sourceCalls;run.Offer(packet);run.Advance(1,{1,0},{1,0},0,192);assert(run.DiscardSources()==1);while(run.Tick().active)run.Pump();assert(sourceCalls==before&&run.Queued()==0);passed++;}
 // A submitted source drains exactly once. The other pending birth is dropped.
 {m1::FullM1Transport t;m1::Run run(t);auto before=sourceCalls;run.Offer(packet);run.Offer(packet);run.Advance(1,{1,0},{1,0},0,192);run.Pump();assert(run.DiscardSources()==1&&run.Queued()==1);while(run.Tick().active)run.Pump();assert(sourceCalls==before+1&&run.Queued()==0);passed++;}
 {m1::FullM1Transport t;m1::Run run(t);run.Advance(1,{1,0},{1,0},0,192);run.Advance(16666667,{2,0},{2,0},0,192);auto before=quantumCalls;run.Pump();run.Suspend();assert(run.Offer(packet)==ControlCode::BUSY);while(run.HasFlight())run.Pump();assert(quantumCalls==before+1&&!run.Tick().active);passed++;
  run.Resume();assert(run.Advance(1000,{3,0},{3,0},0,192)==ControlCode::OK&&run.Tick().time.quanta==0);assert(run.Advance(8333334,{4,0},{4,0},0,192)==ControlCode::OK&&run.Tick().time.quanta==1);while(run.Tick().active)run.Pump();assert(quantumCalls==before+2);passed++;}
 std::cout<<"{\"revision\":\"T228_UI_NATIVE_CONTROL_V1\",\"passed\":"<<passed<<",\"failed\":0,\"transport\":\"EXPLICIT_DOUBLE_REAL_RUN_CLOCK\",\"device\":\"NOT_RUN\"}\n";
}
