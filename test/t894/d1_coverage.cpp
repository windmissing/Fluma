#include "../../pc/vulkan/m1_pulse_queue.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <utility>

int main() {
  using namespace grs::g13vk;
  M1PulseQueue q;
  using Terminal=M1PulseQueue::Terminal;
  std::map<std::pair<uint64_t,uint64_t>,Terminal> events;
  uint64_t owner=100;
  q.Observe([&] { return ++owner; },[&](const Terminal &e) {
    assert(e.qpc>=101 && e.scope.run==1 && e.scope.geometry==2);
    assert(events.emplace(std::make_pair(e.scope.session,e.pulse.seq),e).second);
  });
  constexpr uint64_t ms=1000000;
  grs::m1::ProgressClockV2 clock;
  const auto start=[&] {
    assert(clock.BeginSession());
    q.Begin({1,2,clock.session},0);
  };
  const auto put=[&](uint64_t n) {
    return q.ReceiveWithReceipt(2,{0,n*ms,n*ms+1,n*ms,n*ms+1,0,uint32_t(n)});
  };
  M1Pulse latest,accepted;
  const auto poll=[&] { return q.Consume({1,2,clock.session},clock,latest,accepted); };
  start();
  assert(put(1).pulse.seq==1 && poll());
  assert(put(2).pulse.seq==2 && put(3).pulse.seq==3);
  assert(!q.Ledger(1,2).Complete());
  start();
  assert(!std::strcmp(events.at({1,2}).reason,"REVOKED_REANCHOR"));
  assert(!std::strcmp(events.at({1,3}).reason,"REVOKED_REANCHOR"));
  for (size_t i=0;i<q.kCapacity;++i) assert(put(i+1).pulse.seq==i+1);
  const auto failed=put(q.kCapacity+1);
  assert(!failed.pulse.seq && !std::strcmp(failed.state,"PULSE_QUEUE_OVERFLOW"));
  assert(!poll() && !std::strcmp(events.at({2,32}).reason,"REVOKED_OVERFLOW"));
  start();
  assert(put(1).pulse.seq && put(50).pulse.seq && put(51).pulse.seq);
  assert(!poll() && events.at({3,2}).outcome==M1PulseQueue::Outcome::Rejected);
  start();
  assert(put(4).pulse.seq);
  q.Close();
  assert(!std::strcmp(events.at({4,1}).reason,"REVOKED_CLOSED"));
  assert(q.Ledger(1,2).Complete() && events.size()==39);
  std::cout << "T894 B: cross-session owner terminal uniqueness and overflow checks passed\n";
}
