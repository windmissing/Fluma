#include "../../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h"
#include "../../pc/vulkan/m1_recovery.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using grs::m1::ProgressClockV2;
  using grs::m1::PlanGrantV2;
  constexpr auto ms=1000000ull;
  ProgressClockV2 clock;
  assert(clock.BeginSession());
  clock.ready=true;
  const auto pulse=[&](uint64_t seq,uint64_t now) {
    assert(!std::strcmp(clock.Pulse(clock.session,seq,now,now),"PAGE_PULSE_ACCEPTED"));
  };
  pulse(1,1*ms);
  pulse(2,33*ms);
  pulse(3,50*ms);
  assert(clock.pending==4 && clock.Balanced());
  auto legacy=clock;
  PlanGrantV2 grant;
  assert(legacy.Claim(51*ms,0,7,true,grant) && grant.quanta==4 && legacy.pending==0);
  assert(clock.Claim(51*ms,0,7,true,grant,1) && grant.quanta==1 && clock.pending==3);
  assert(clock.AcceptQuantum(grant.id));
  const auto old=grant;
  assert(clock.Claim(52*ms,0,0,false,grant,1) && clock.pending==2 && grant.grantedNs==52*ms);
  assert(grs::pc::CanRearmM1("ROOT_DEADLINE",true,true,true,false,false,true,true));
  assert(!grs::pc::CanRearmM1("ROOT_DEADLINE",true,true,true,true,false,true,true));
  clock.Revoke("ROOT_DEADLINE");
  assert(clock.revoked==3 && clock.accepted==1 && clock.Balanced());
  assert(clock.BeginSession() && !clock.Identity(old.id,old.session));
  pulse(1,400*ms); // Establishes real baseline without granting physics debt.
  clock.ready=clock.baseline=true;
  assert(!clock.Claim(401*ms,0,0,false,grant,1));
  pulse(2,416*ms);
  assert(clock.Claim(417*ms,0,0,false,grant,1) && grant.sourceBoundary==0 && grant.quanta==1);
  assert(clock.AcceptQuantum(grant.id) && clock.accepted==2 && clock.Balanced());
  assert(ProgressClockV2::GapNs==33333333 && ProgressClockV2::JobNs==250000000);
  std::cout << "T899 A: capped grant, legacy grant, retired balance, and no-SOURCE session passed\n";
}
