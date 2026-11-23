#include "../../pc/vulkan/m1_d1_drain.h"
#include "../../pc/vulkan/m1_hot_capture.h"
#include "../../entry/src/main/cpp/gpu_resident/m1/source_owner.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace grs;
  using namespace grs::g13vk;
  const SourceScope scope{1,2,3,4,5,6,7,8};
  M1D1Intake intake;
  intake.Start();
  intake.Seal({99,52,2});
  intake.Seal({100,53,3});
  const auto end=intake.Await();
  assert(end.ns==99 && end.pulse==52 && end.offer==2);
  assert(intake.MarkReported() && !intake.MarkReported());

  M1Ingress ingress;
  ingress.EnableTrace();
  ingress.Authorize(scope,7);
  SourceOffer offer{scope,1,10,10,1,1000000000,{7,1,1,64.5,64},2,7,9};
  assert(ingress.OfferSource(offer).code==M1Ingress::Code::Pending);
  assert(ingress.Take().seq==1);
  offer.seq=2;
  assert(ingress.OfferSource(offer).code==M1Ingress::Code::Pending);
  ingress.EndIntake();
  ingress.EndIntake();
  assert(ingress.PendingCount()==1 && ingress.Take().seq==0);
  ingress.Finish(1,{M1Ingress::Code::Accepted,11});
  assert(!ingress.PendingCount());
  offer.seq=3;
  assert(ingress.OfferSource(offer).code==M1Ingress::Code::Revoked);
  std::array<M1Ingress::Trace,8> rows;
  uint64_t lost=0;
  const auto count=ingress.ReadTrace(rows.data(),rows.size(),lost);
  assert(count==5 && !lost);
  assert(rows[2].terminal && rows[2].offer.seq==2 && !std::strcmp(rows[2].reason,"END_INTAKE"));
  assert(rows[3].terminal && rows[3].offer.seq==1 && rows[3].result.root==11);

  M1HotCapture capture;
  capture.Authorize(scope);
  capture.Busy(M1HotCapture::Phase::QuantumFence,11);
  assert(capture.Reserve(scope,1,10)==M1HotCapture::Code::Reserved);
  assert(capture.EndIntake()==1 && !capture.HasLiveRequest());
  assert(capture.Reserve(scope,2,20)==M1HotCapture::Code::Cancelled);

  M1PulseQueue pulses;
  m1::ProgressClockV2 clock;
  assert(clock.BeginSession());
  clock.ready=true;
  const M1PulseScope pulseScope{1,5,clock.session};
  pulses.Begin(pulseScope,1);
  constexpr uint64_t ms=1000000;
  assert(pulses.Receive(5,{0,ms,ms,ms,ms,0,1}).action==1);
  pulses.Receive(5,{0,17*ms,17*ms,17*ms,17*ms,0,2});
  M1Pulse latest,accepted;
  assert(pulses.Consume(pulseScope,clock,latest,accepted));
  m1::PlanGrantV2 grant;
  assert(clock.Claim(18*ms,0,0,false,grant));
  pulses.Receive(5,{0,33*ms,33*ms,33*ms,33*ms,0,3});
  unsigned revokedPulses=0;
  pulses.RevokeQueued(pulseScope,[&](const M1Pulse &pulse,const char *why) {
    assert(pulse.action==3 && !std::strcmp(why,"END_INTAKE_REVOKED"));
    ++revokedPulses;
  });
  const auto quota=clock.activeRemaining;
  const auto carry=clock.carry;
  clock.Revoke("D1_END_INTAKE");
  assert(revokedPulses==1 && quota && carry && clock.Balanced() &&
         clock.revoked==quota && clock.clearedCarry==carry);

  for (const bool pending:{false,true}) {
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
    AcceptedRoot discarded;
    if (pending) {
      assert(owner.Poll(FenceState::Pending,nullptr,0,0,discarded)==ControlCode::QUARANTINED);
      assert(owner.State()==FlightState::Quarantined);
    }
    assert(owner.Poll(FenceState::Signaled,nullptr,0,0,discarded)==ControlCode::STALE);
    assert(owner.State()==FlightState::Idle && owner.AcceptedCount()==0 && !owner.HasLeases());
  }
  std::cout << "T882 B: intake seal, unique offer terminals, capture cancel, quota and owner retirement passed\n";
}
