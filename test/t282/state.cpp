#include "bridge_state.h"
#include <condition_variable>
#include <thread>
#include <iostream>
#include <cassert>
using namespace grs;
std::array<uint32_t,32> packet(const Identity& id,U64 generation){std::array<uint32_t,32> w{};w[0]=1129534023;w[1]=1;w[4]=id.run.lo;w[5]=id.run.hi;w[6]=w[10]=w[14]=w[15]=w[24]=w[27]=1;w[8]=generation.lo;w[9]=generation.hi;w[12]=id.geometry.lo;w[13]=id.geometry.hi;w[28]=id.config.lo;w[29]=id.config.hi;double x=64.5,y=100,q=.125;std::memcpy(w.data()+16,&x,8);std::memcpy(w.data()+18,&y,8);std::memcpy(w.data()+20,&q,8);return w;}
int main(){int passed=0;Identity id;id.run={1,77};id.geometry={3,23};id.config={4,66};U64 input{2,99};void* first=reinterpret_cast<void*>(1);void* second=reinterpret_cast<void*>(2);
 // Pause at the exact post-create/pre-CommitReadyLocked boundary. A separate
 // thread performs each lifecycle interleave before the real commit gate.
 for(int mode=0;mode<4;mode++){BridgeState b;b.window=first;uint64_t seen=b.epoch;std::mutex m;std::condition_variable cv;bool created=false,resume=false,result=true;std::thread init([&]{{std::unique_lock<std::mutex> l(m);created=true;cv.notify_one();cv.wait(l,[&]{return resume;});}std::lock_guard<std::mutex> l(b.mutex);result=b.CommitReadyLocked(seen,first,id,input);});{std::unique_lock<std::mutex> l(m);cv.wait(l,[&]{return created;});}{std::lock_guard<std::mutex> l(b.mutex);if(mode==0)b.InvalidateLocked();if(mode==1){b.active=false;b.InvalidateLocked();}if(mode==2){b.window=second;b.InvalidateLocked();}if(mode==3)b.window=second;}{std::lock_guard<std::mutex> l(m);resume=true;cv.notify_one();}init.join();assert(!result&&!b.AllowedLocked()&&!b.ready&&b.readyEpoch==0);assert(std::strcmp(b.OfferLocked(packet(id,input)),"NOT_READY")==0);passed+=2;}
 BridgeState b;b.window=first;assert(b.CommitReadyLocked(1,first,id,input));assert(b.readyEpoch==1&&b.AllowedLocked()&&b.status=="{\"state\":\"READY\"}");passed++;
 auto command=packet(id,input);assert(std::strcmp(b.OfferLocked(command),"ENQUEUED_NOT_ACK")==0);std::array<uint32_t,32> out{};assert(b.FrontLocked(1,input,out)&&out==command);b.PopLocked();assert(b.count==0);passed++;
 b.OfferLocked(command);assert(!b.FrontLocked(2,input,out)&&b.count==1);passed++;
 assert(!b.FrontLocked(1,{3,99},out)&&b.count==0&&b.cancelled==1&&b.staleRejected==1);passed++;
 b.OfferLocked(command);b.queue[b.head].epoch=0;assert(!b.FrontLocked(1,input,out)&&b.count==0);passed++;
 b.OfferLocked(command);b.runQueue=3;b.InvalidateLocked();assert(!b.ready&&b.readyEpoch==0&&b.count==0&&b.runQueue==0&&b.status.find("STATE_LOST")!=std::string::npos);passed++;
 assert(!b.CommitReadyLocked(1,first,id,input));id.run.lo=2;assert(b.CommitReadyLocked(2,first,id,{3,99}));assert(std::strcmp(b.OfferLocked(command),"PACKET_IDENTITY")==0);passed++;
 for(int i=0;i<16;i++)assert(std::strcmp(b.OfferLocked(packet(id,{3,99})),"ENQUEUED_NOT_ACK")==0);assert(std::strcmp(b.OfferLocked(packet(id,{3,99})),"BACKPRESSURE")==0);passed++;
 b.RevokeLocked("PAUSED_CLOCK");b.phase="CLOCK_GAP";assert(!b.AllowedLocked()&&!b.ready&&b.readyEpoch==0&&b.count==0);assert(std::strcmp(b.OfferLocked(packet(id,{3,99})),"NOT_READY")==0);passed++;
 // A snapshot observes either complete READY or complete revoke, never a mix.
 std::atomic<bool> done{false};std::thread reader([&]{while(!done){std::lock_guard<std::mutex> l(b.mutex);if(b.AllowedLocked()){assert(b.readyEpoch==b.epoch&&b.status.find("STATE_LOST")==std::string::npos);assert(b.phase==std::string("READY"));}if(b.status.find("STATE_LOST")!=std::string::npos)assert(!b.ready&&b.readyEpoch==0);}});for(int i=0;i<2000;i++){std::lock_guard<std::mutex> l(b.mutex);b.InvalidateLocked();assert(b.CommitReadyLocked(b.epoch,first,id,input));}done=true;reader.join();passed++;
 std::cout<<"{\"revision\":\"T282_READY_ATOMIC_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}
