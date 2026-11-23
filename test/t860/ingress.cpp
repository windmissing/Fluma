#include "../../pc/vulkan/m1_hot_capture.h"
#include <cassert>
#include <iostream>
#include <thread>

int main() {
  using namespace grs::g13vk;
  using SourceCode=M1Ingress::Code;
  M1Ingress ingress;
  ingress.EnableTrace();
  const SourceScope scope{1,2,3,4,5,6,7,8};
  PointerSample sample{7,8,9,10,11};
  ingress.Authorize(scope,7);
  SourceOffer first{scope,1,12,13,11,10,sample,1};
  auto next=first;
  std::thread producer([&] {
    for (uint64_t seq=1;seq<=4;++seq) {
      next.seq=seq;
      assert(ingress.OfferSource(next).code==SourceCode::Pending);
    }
    next.seq=5;
    assert(ingress.OfferSource(next).code==SourceCode::Full);
  });
  producer.join();
  assert(ingress.Take().seq==1);
  assert(ingress.OfferSource(first).code==SourceCode::Consuming);
  ingress.Finish(1,{SourceCode::Accepted,99});
  assert(ingress.OfferSource(first).root==99);
  auto conflict=first;
  conflict.sample.x=9;
  assert(ingress.OfferSource(conflict).code==SourceCode::Conflict);
  assert(ingress.OfferSource(next).code==SourceCode::Pending);
  ingress.Authorize(scope,0);
  assert(!ingress.Take().seq);
  assert(ingress.OfferSource(next).code==SourceCode::Revoked);
  ingress.Authorize(scope,7);
  assert(ingress.OfferSource(first).code==SourceCode::Expired);

  auto nextScope=scope;
  nextScope.permission=20;
  ingress.Authorize(scope,0);
  ingress.Authorize(nextScope,20);
  auto water=next;
  water.scope=nextScope;
  water.sample.hold=20;
  water.sample.material=1;
  water.seq=6;
  auto invalid=water;
  invalid.sample.material=2;
  assert(ingress.OfferSource(invalid).code==SourceCode::Invalid);
  invalid.sample=water.sample;
  invalid.sample.x=128;
  assert(ingress.OfferSource(invalid).code==SourceCode::Invalid);
  assert(ingress.OfferSource(water).code==SourceCode::Pending);
  const auto acquired=ingress.Take();
  assert(acquired.scope.run==scope.run && acquired.sample.material==1 && acquired.sample.hold==20);
  std::array<M1Ingress::Trace,32> trace;
  uint64_t dropped=1;
  const auto records=ingress.ReadTrace(trace.data(),trace.size(),dropped);
  assert(records>10 && !dropped);
  bool acceptedRoot=false, backedOff=false, revoked=false;
  for (size_t i=0;i<records;++i) {
    assert(trace[i].serial==i+1);
    acceptedRoot|=trace[i].terminal && trace[i].result.root==99;
    backedOff|=!trace[i].terminal && trace[i].result.code==SourceCode::Full;
    revoked|=trace[i].terminal && trace[i].result.code==SourceCode::Revoked;
  }
  assert(acceptedRoot && backedOff && revoked);

  M1HotCapture capture;
  using Code=M1HotCapture::Code;
  using Phase=M1HotCapture::Phase;
  capture.Authorize(scope);
  assert(capture.Reserve(scope,1,10)==Code::NotBusy);
  capture.Busy(Phase::QuantumFence,99);
  assert(capture.Reserve(scope,1,10)==Code::Reserved);
  assert(capture.Reserve(scope,2,10)==Code::Conflict);
  assert(!capture.Read(scope,1,11));
  std::array<uint32_t,256> completion{10};
  capture.Safe(99,&completion,12);
  completion[0]=0;
  const auto page=capture.Read(scope,1,13);
  assert(page && page->busyRoot==99 && page->completion[0]==10 && page->safePoint==1);
  assert(!capture.Read(scope,1,13));
  capture.End(scope,1);
  capture.Busy(Phase::SourceFence,101);
  assert(capture.Reserve(scope,2,15)==Code::Reserved);
  capture.Safe(101,&completion,1000000015);
  assert(capture.Reserve(scope,2,1000000015)==Code::TimedOut);
  capture.Busy(Phase::SourceFence,102);
  assert(capture.Reserve(scope,3,1000000016)==Code::Reserved);
  capture.Authorize({});
  capture.Safe(102,&completion,1000000017);
  assert(!capture.Read(scope,3,1000000018));
  capture.Authorize(scope);
  capture.Busy(Phase::GraphicsFence,103);
  assert(capture.Reserve(scope,4,1000000019)==Code::Reserved);
  capture.StopBusy();
  capture.Safe(103,&completion,1000000020);
  const auto graphics=capture.Read(scope,4,1000000021);
  assert(graphics && graphics->blocker==Phase::GraphicsFence);
  capture.End(scope,4);
  capture.Busy(Phase::Present,103);
  capture.StopBusy();
  assert(capture.Reserve(scope,5,1000000022)==Code::NotBusy);
  capture.Busy(Phase::QuantumFence,104);
  assert(capture.Reserve(scope,5,1000000023)==Code::Reserved);
  capture.Safe(104,&completion,1000000024);
  const auto ticket=capture.ClaimCold(104,1000000025);
  assert(ticket && !capture.ClaimCold(104,1000000025));
  std::vector<uint8_t> payload{1,2,3,4,5};
  assert(capture.SealCold(*ticket,payload,std::string(64,'a'),1000000026));
  std::array<uint8_t,3> buffer{};
  assert(!capture.ReadCold(scope,5,1,buffer.data(),buffer.size(),1000000027));
  const auto firstPage=capture.ReadCold(scope,5,0,buffer.data(),buffer.size(),1000000027);
  assert(firstPage && firstPage->offset==0 && firstPage->count==3 && firstPage->total==5);
  assert((buffer==std::array<uint8_t,3>{1,2,3}));
  assert(!capture.ReadCold(scope,5,0,buffer.data(),buffer.size(),1000000028));
  const auto lastPage=capture.ReadCold(scope,5,3,buffer.data(),buffer.size(),1000000028);
  assert(lastPage && lastPage->count==2 && buffer[0]==4 && buffer[1]==5);
  capture.Authorize({});
  assert(!capture.ReadCold(scope,5,3,buffer.data(),buffer.size(),1000000029));
  std::cout << "T860 ingress and immutable hot page checks passed\n";
}
