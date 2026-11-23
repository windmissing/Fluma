#include "../../pc/vulkan/m1_recovery.h"
#include "../../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h"
#include <cassert>
#include <iostream>

int main() {
  using grs::pc::CanRearmM1;
  assert(CanRearmM1("ROOT_DEADLINE",true,true,true,false,false,true,true));
  assert(CanRearmM1("LATE_FENCE_SIGNAL",true,true,true,false,false,true,true));
  assert(!CanRearmM1("ROOT_DEADLINE",true,true,true,false,false,true,false));
  assert(!CanRearmM1("ROOT_DEADLINE",true,true,true,true,false,true,true));
  assert(!CanRearmM1("ROOT_DEADLINE",true,true,true,false,true,true,true));
  assert(!CanRearmM1("ROOT_DEADLINE",true,false,true,false,false,true,true));
  assert(!CanRearmM1("ROOT_DEADLINE",false,true,true,false,false,true,true));
  assert(!CanRearmM1("DEVICE_LOST",true,true,false,false,false,true,false));
  assert(CanRearmM1("CLOCK_GAP",true,true,true,false,false,false,false));
  grs::m1::ProgressClockV2 clock;
  assert(clock.BeginSession());
  clock.ready=clock.baseline=true;
  clock.Pulse(clock.session,1,1,1);
  clock.Pulse(clock.session,2,33000000,33000000);
  grs::m1::PlanGrantV2 old;
  assert(clock.Claim(33000001,0,11,true,old));
  assert(clock.AcceptQuantum(old.id));
  const auto oldAccepted=clock.accepted;
  const auto oldCredits=clock.activeRemaining+clock.pending;
  const auto oldCarry=clock.carry;
  clock.Revoke("ROOT_DEADLINE");
  assert(clock.revoked==oldCredits && clock.clearedCarry==oldCarry && clock.Balanced());
  assert(clock.BeginSession() && !clock.Identity(old.id,old.session));
  assert(clock.accepted==oldAccepted && clock.activeRemaining==0 && clock.pending==0);
  std::cout << "T894 A: authoritative retirement, scope, pins and quota checks passed\n";
}
