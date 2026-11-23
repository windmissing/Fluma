#include "../../pc/vulkan/m1_pulse_queue.h"
#include "../../entry/src/main/cpp/gpu_resident/m1/source_owner.h"
#include <cassert>
#include <iostream>

int main() {
  using namespace grs;
  using namespace grs::g13vk;
  m1::ProgressClockV2 clock;
  M1PulseQueue queue;
  M1Pulse latest,accepted;
  M1PulseScope scope{1,2,0};
  constexpr uint64_t ms=1000000;
  const auto start=[&](uint64_t after) {
    assert(clock.BeginSession());
    clock.ready=true;
    scope.session=clock.session;
    queue.Begin(scope,after);
  };
  const auto push=[&](uint64_t at,uint64_t receipt) {
    queue.Receive(scope.geometry,{0,at,receipt,at,receipt});
  };
  const auto drain=[&] { return queue.Consume(scope,clock,latest,accepted); };

  start(0);
  push(1*ms,2*ms);
  assert(drain());
  m1::PlanGrantV2 grant;
  assert(clock.Claim(3*ms,0,7,true,grant));
  // The owner is delayed 39 ms. Both real consecutive receipt times survive.
  push(17*ms,18*ms);
  push(33*ms,34*ms);
  assert(drain() && clock.Fresh(41*ms) && clock.Identity(grant.id,grant.session));
  assert(accepted.seq==3 && clock.maxUiGap==16*ms && clock.maxNativeGap==16*ms);
  assert(clock.Balanced());
  // A real receive/callback gap revokes at the first bad pulse.
  push(69*ms,70*ms);
  assert(!drain() && !std::strcmp(clock.fault,"CLOCK_GAP"));
  assert(!clock.Identity(grant.id,grant.session) && clock.Balanced());

  start(100*ms);
  push(99*ms,101*ms);
  queue.Receive(1,{0,102*ms,102*ms,102*ms,102*ms});
  push(103*ms,104*ms);
  assert(drain() && clock.sequence==1 && clock.uiNs==103*ms);
  assert(clock.pending==0 && !clock.Identity(grant.id,grant.session));
  assert(!clock.Fresh(138*ms) && !std::strcmp(clock.fault,"CLOCK_GAP"));

  start(200*ms);
  push(201*ms,202*ms);
  push(217*ms,241*ms);
  assert(!drain() && !std::strcmp(clock.fault,"CLOCK_GAP"));
  assert(clock.maxNativeGap==39*ms); // UI continuity never repairs a native gap.

  start(250*ms);
  push(251*ms,258*ms);
  push(287*ms,288*ms);
  assert(!drain() && !std::strcmp(clock.fault,"CLOCK_GAP"));
  assert(clock.maxUiGap==36*ms); // A shorter receipt interval cannot hide a UI gap.

  start(300*ms);
  for (unsigned i=0;i<=M1PulseQueue::kCapacity;++i) push((301+i)*ms,(301+i)*ms);
  assert(!drain() && !std::strcmp(clock.fault,"PULSE_QUEUE_OVERFLOW") && clock.sequence==0);
  assert(clock.Balanced());

  start(400*ms);
  push(402*ms,403*ms);
  push(401*ms,404*ms);
  assert(!drain() && !std::strcmp(clock.fault,"PULSE_CLOCK_REVERSED"));

  start(500*ms);
  push(503*ms,504*ms);
  queue.Close();
  assert(!drain() && !std::strcmp(clock.fault,"STALE_PULSE_SCOPE"));
  assert(clock.Balanced());

  m1::SourceRootOwner owner;
  Identity id{};
  id.run=id.owner=id.context=id.surface=id.geometry=id.config={1,0};
  assert(owner.Initialize(id,192,128)==ControlCode::OK);
  RootRequest request{};
  request.kind=1; request.control=request.frame=request.nonce={1,0};
  request.publicationSlot=1; request.eventCapacity=193;
  request.dispatchCount=8; request.barrierCount=9;
  assert(owner.Begin(request,{})==ControlCode::OK);
  assert(owner.Stop()==ControlCode::QUARANTINED);
  AcceptedRoot late;
  assert(owner.Poll(FenceState::Pending,nullptr,0,0,late)==ControlCode::QUARANTINED);
  assert(owner.State()==FlightState::Quarantined);
  assert(owner.Begin(request,{})==ControlCode::QUARANTINED);
  assert(owner.Poll(FenceState::Signaled,nullptr,0,0,late)==ControlCode::STALE);
  assert(owner.State()==FlightState::Idle && owner.Bank()==0 && owner.AcceptedCount()==0 && !owner.HasLeases());
  std::cout << "T872: ordered pulses, real gaps, scope, overflow and tombstone checks passed\n";
}
