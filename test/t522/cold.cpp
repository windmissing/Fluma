#include <cstdint>
#include "m3a/cold_contract.h"
#include "m3a/cold_trace.h"
#include "cold_trace.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace grs::m3a;
 for(const auto& profile:kProfiles){
  cold::Layout layout;assert(cold::Layout::Make(profile,256,layout));
  assert(layout.state==256&&layout.quota==256+profile.stateBytes/4&&layout.control==layout.quota+256&&layout.input==layout.control+2048&&layout.events==layout.input+544);
  std::array<char,1024> bytes{};cold::Writer w(bytes.data(),bytes.size());assert(cold::Capabilities(w,profile,256)&&w.Good());
  cold::Layout full;assert(cold::Layout::Make(profile,256,full,true));
  assert(full.branchMeta==layout.events&&full.branchControl==full.branchMeta+32&&full.branchEvents==full.branchControl+8192&&full.events==full.branchEvents+20*size_t(profile.scopeRows)*64);
  assert(full.residentPayload==layout.residentPayload+20*size_t(profile.scopeRows)*256+32768);
  cold::Writer shortWriter(bytes.data(),8);assert(!cold::Capabilities(shortWriter,profile,256));
 }
 cold::ReadScope scope;scope.identity={1,2,3,4,5,6,7,8,9};cold::ReadGate gate;
 assert(gate.Begin(scope,scope,16383)==ControlResult::WOULD_BLOCK);
 assert(gate.Begin(scope,scope,16384)==ControlResult::OK);
 assert(gate.Begin(scope,scope,16384)==ControlResult::WOULD_BLOCK);
 auto changed=scope;++changed.identity.context;assert(gate.Page(changed,1024)==ControlResult::STALE_REJECTED);
 assert(gate.Page(scope,16385)==ControlResult::WOULD_BLOCK);
 assert(gate.Page(scope,16384)==ControlResult::OK&&gate.Page(scope,1)==ControlResult::WOULD_BLOCK);
 gate.End();assert(gate.Page(scope,1)==ControlResult::STALE_REJECTED);
 {
  coldtrace::TraceManager traces;grs::cold503::Scope id;std::strcpy(id.route.data(),"m3a");id.control.run=1;id.control.context=2;id.control.surface=3;
  constexpr size_t physical=6400,state=physical+2848,events=64,words=256+state+events;
  uint64_t request=0,trace=0;assert(std::strcmp(traces.Begin(id,id,1,0,words,request,trace,state),"REQUESTED")==0);
  int slot=traces.Start();assert(slot>=0);traces.OldFreed(slot);traces.Adopt(slot,std::make_unique<uint32_t[]>(words));
  auto* w=traces.slots[slot].block.get();w[0]=1381192263;w[1]=4;w[2]=256;w[3]=3235823838u;w[4]=1;w[6]=1;w[10]=2;w[12]=3;w[27]=2;w[49]=15;w[53]=1;w[173]=1;w[174]=7;w[188]=1;
  auto* quota=w+256+physical;quota[0]=860963661;quota[1]=3;quota[2]=256;quota[3]=0xc0de0003;
  grs::cold503::TraceManager::Slot legacy;legacy.capacity=legacy.requestWords=256;legacy.block=std::make_unique<uint32_t[]>(256);std::memcpy(legacy.block.get(),w,1024);
  assert(!grs::cold503::TraceManager::Layout(legacy,0,0));
  traces.Finish(slot,id,2,"OK",state,events);assert(traces.published==slot);
  coldtrace::TraceManager::Read read;assert(std::strcmp(traces.Acquire(trace,1,3,id,read),"OK")==0);
  assert(std::strcmp(traces.End(trace,4,id),"ENDED")==0&&traces.slots[slot].state==coldtrace::TraceManager::RETIRING);
  assert(std::strcmp(traces.Release(read,5,id),"ENDED")==0&&traces.slots[slot].state==coldtrace::TraceManager::FREE);
  std::strcpy(id.route.data(),"m2");assert(std::strcmp(traces.Begin(id,id,6,0,words,request,trace,state),"UNSUPPORTED_ROUTE")==0);
 }
 {
  const auto& p=kProfiles[0];cold::Layout layout;assert(cold::Layout::Make(p,256,layout,true));
  coldtrace::TraceManager traces;grs::cold503::Scope id;std::strcpy(id.route.data(),"m3a");id.control.run=1;id.control.context=2;id.control.surface=3;
  uint64_t req=0,trace=0;const size_t state=layout.events-256;
  assert(!std::strcmp(traces.Begin(id,id,1,0,layout.words,req,trace,state,p.scopeRows),"REQUESTED"));
  const int slot=traces.Start();traces.OldFreed(slot);traces.Adopt(slot,std::make_unique<uint32_t[]>(layout.words));
  auto* w=traces.slots[slot].block.get();w[0]=1381192263;w[1]=4;w[2]=256;w[3]=3235823838u;w[4]=w[6]=1;w[10]=2;w[12]=3;w[27]=2;w[49]=15;w[53]=1;w[173]=1;w[174]=7;w[188]=1;
  auto* quota=w+layout.quota;quota[0]=860963661;quota[1]=3;quota[2]=256;quota[3]=0xc0de0003;
  auto* m=w+layout.branchMeta;m[0]=0x3442464d;m[1]=1;m[2]=20;m[3]=p.scopeRows;m[4]=1;m[6]=31;m[7]=1;
  assert(coldtrace::TraceManager::Layout(traces.slots[slot],state,64));m[6]=1u<<20;assert(!coldtrace::TraceManager::Layout(traces.slots[slot],state,64));m[6]=31;
  traces.Finish(slot,id,2,"OK",state,64);coldtrace::TraceManager::Read read;assert(!std::strcmp(traces.Acquire(trace,0,3,id,read),"OK"));
  std::array<char,16384> b{};cold::Writer writer(b.data(),b.size());coldtrace::TracePage(writer,read,0);assert(writer.Good());
  assert(std::strstr(writer.Data(),"GRS_M3_FULL_BRANCH_V1")&&std::strstr(writer.Data(),"branchEventsWordOffset"));
  traces.Release(read,4,id);std::cout<<"FULL_BRANCH separate credit/layout/schema/mask validation PASS\n";
 }
 std::cout<<"BQ20 existing V3 cold reader rejects V4 completion before interpretation PASS\n";
 std::cout<<"T522 V4 cold payload/fixed quota/control layout, capacity and stale-page gates PASS\n";
}
