#include "../../pc/vulkan/m1_ingress.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace grs::g13vk;
  using Code=M1Ingress::Code;
  constexpr uint64_t ms=1000000;
  const SourceScope scope{1,2,3,4,5,6,7,8};
  M1Ingress ingress;
  ingress.EnableTrace();
  ingress.Authorize(scope,7);
  const auto pulse=[=](uint64_t i) -> M1Pulse { return {i,i*16*ms,i*16*ms,i*16*ms,i*16*ms,2}; };
  const auto offer=[&](uint64_t i) -> SourceOffer {
    const auto a=pulse(i-1),b=pulse(i);
    return {scope,i,b.uiNs,b.callbackTicks,1,1000000000,{7,1,1,64.5,64},1,0,b.uiNs-a.uiNs,a,b};
  };
  for (uint64_t i=2;i<=12;++i) assert(ingress.OfferLatest(offer(i)).code==Code::Pending);
  assert(ingress.PendingCount()==1);
  const auto first=ingress.TakeLatest(195*ms,1,2);
  assert(first.seq==12 && first.deltaNs==16*ms && first.previous.seq==11 && first.current.seq==12);
  assert(ingress.OfferLatest(offer(13)).code==Code::Pending);
  assert(!ingress.TakeLatest(210*ms,1,2).seq && ingress.PendingCount()==2);
  assert(ingress.OfferLatest(offer(12)).code==Code::Consuming);
  ingress.Finish(first.seq,{Code::Accepted,19});
  assert(ingress.TakeLatest(245*ms,1,2).seq==0); // Endpoint 208 ms, too old.
  assert(ingress.OfferLatest(offer(14)).code==Code::Pending);
  assert(ingress.TakeLatest(225*ms,225*ms,3).seq==0); // New clock baseline.
  auto invalid=offer(15);
  invalid.current.receiptNs+=34*ms;
  assert(ingress.OfferLatest(invalid).code==Code::Invalid);
  assert(ingress.OfferLatest(offer(15)).code==Code::Pending);
  ingress.Authorize({},0);
  assert(!ingress.PendingCount());
  std::array<M1Ingress::Trace,64> rows;
  uint64_t dropped=0;
  const auto used=ingress.ReadTrace(rows.data(),rows.size(),dropped);
  unsigned terminals=0,superseded=0,expired=0;
  for (size_t i=0;i<used;++i) if (rows[i].terminal) {
    ++terminals;
    superseded+=rows[i].result.code==Code::Superseded;
    expired+=rows[i].result.code==Code::Expired;
    for (size_t j=0;j<i;++j) assert(!rows[j].terminal || rows[j].offer.seq!=rows[i].offer.seq);
  }
  assert(!dropped && terminals==14 && superseded==10 && expired==2);
  std::cout << "T882 A: latest interval, immutable claim, freshness, and unique terminals passed\n";
}
