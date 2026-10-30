#include "m1/run.h"
#include "bridge_state.h"
#include <cassert>
#include <iostream>
using namespace grs;
static RootRequest submitted;static bool flight=false;static uint32_t sourceCalls=0,quantumCalls=0,polls=0;
namespace grs {namespace m1 {
TransportCode FullM1Transport::SubmitSource(const std::array<uint32_t,32>&,RootRequest r){assert(r.kind==1&&r.quantumOrdinal==0);submitted=r;sourceCalls++;flight=true;return TransportCode::OK;}
TransportCode FullM1Transport::SubmitQuantum(const QuantumParameters& p,RootRequest r){assert(r.kind==2&&r.quantumOrdinal>=1&&r.quantumOrdinal<=4&&p.requestedAngle==0);submitted=r;quantumCalls++;flight=true;return TransportCode::OK;}
TransportCode FullM1Transport::Poll(AcceptedRoot& out){if(!flight)return TransportCode::NOT_READY;if(++polls%2)return TransportCode::BUSY;flight=false;out.nonce=submitted.nonce;return TransportCode::OK;}
ControlCode FullM1Transport::Stop(){flight=false;return ControlCode::OK;}
}}
int main(){int passed=0;BridgeState state;state.window=reinterpret_cast<void*>(1);Identity id;id.run={1,2};id.geometry={1,3};id.config={1,4};state.CommitReadyLocked(1,state.window,id,{1,0});std::array<uint32_t,32> w{};w[0]=1129534023;w[1]=2;w[2]=1;w[4]=1;w[5]=2;w[6]=w[8]=w[10]=w[14]=w[15]=w[24]=w[27]=1;w[12]=1;w[13]=3;w[28]=1;w[29]=4;double x=64.5,y=100,m=.125;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&m,8);assert(std::string(state.OfferLocked(w))=="PACKET_HEADER");passed++;
 state.commandRevision=2;state.waterAllowed=true;assert(std::string(state.OfferLocked(w))=="ENQUEUED_NOT_ACK");std::array<uint32_t,32> original;assert(state.FrontLocked(1,{1,0},original)&&original==w);passed++;auto bad=w;bad[2]=2;assert(std::string(state.OfferLocked(bad))=="PACKET_HEADER_FEATURE");bad=w;bad[1]=1;assert(std::string(state.OfferLocked(bad))=="PACKET_HEADER_FEATURE");passed+=2;
 m1::FullM1Transport transport;m1::Run run(transport);assert(run.Offer(w)==ControlCode::OK);assert(run.Advance(1,{1,0},{1,0},0,192)==ControlCode::OK);assert(run.Pump()==TransportCode::OK&&sourceCalls==1);assert(run.Pump()==TransportCode::BUSY);assert(run.Pump()==TransportCode::OK);passed++;
 assert(run.Advance(16666667,{2,0},{2,0},0,192)==ControlCode::OK);while(run.Tick().active)run.Pump();assert(quantumCalls==2&&run.Tick().successfulQuanta==2);passed++;
 assert(run.Advance(33333334,{3,0},{3,0},0,192)==ControlCode::INVALID_INPUT);assert(run.Queued()==0);passed++;
 std::cout<<"{\"revision\":\"T281_REAL_RUN_PACKET_HOST\",\"passed\":"<<passed<<",\"failed\":0,\"transport\":\"CONTROL_DOUBLE\",\"device\":\"NOT_RUN\"}\n";
}
