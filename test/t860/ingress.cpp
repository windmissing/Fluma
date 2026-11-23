#include "../../pc/vulkan/m1_hot_capture.h"
#include <cassert>
#include <iostream>
#include <thread>

int main() {
  using namespace grs::g13vk;
  using SourceCode=M1Ingress::Code;
  M1Ingress ingress;
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
  std::cout << "T860 ingress and immutable hot page checks passed\n";
}
