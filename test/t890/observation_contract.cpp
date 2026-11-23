#include "../../pc/vulkan/g13_candidate_timing.h"
#include "../../pc/vulkan/m1_observation.h"
#include "../../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace grs::pc;
  assert(Fits({48,32,16,16},64,48));
  assert(!Fits({49,32,16,16},64,48));
  assert(!Fits({0,0,17,16},64,48));
  assert(!Fits({UINT32_MAX,0,16,16},64,48));
  assert(kM1RoiBytes==3072);
  assert(TimestampDelta(250,3,8)==9);
  assert(TimestampDelta(UINT64_MAX-1,1,64)==3);
  assert(TimestampDelta(0,1,0)==0);
  assert(ValidM1Angle(7680) && ValidM1Angle(-7680) && !ValidM1Angle(90*256));

  grs::m1::ProgressClockV2 clock;
  assert(clock.BeginSession());
  clock.ready=true;
  assert(!std::strcmp(clock.Pulse(clock.session,1,100,100),"PAGE_PULSE_ACCEPTED"));
  assert(!std::strcmp(clock.Pulse(clock.session,2,16000100,16000100),"PAGE_PULSE_ACCEPTED"));
  M1Gravity requested{12,1,200,kM1AngleStep};
  grs::m1::PlanGrantV2 grant;
  assert(clock.Claim(16000200,requested.angle,7,true,grant));
  requested={12,2,201,-kM1AngleStep};
  grs::m1::PlanGrantV2 next;
  assert(!clock.Claim(16000300,requested.angle,0,false,next));
  assert(grant.angle==kM1AngleStep && clock.active.angle==kM1AngleStep);
  assert(clock.AcceptQuantum(grant.id));
  assert(!std::strcmp(clock.Pulse(clock.session,3,32000100,32000100),"PAGE_PULSE_ACCEPTED"));
  assert(clock.Claim(32000200,requested.angle,0,false,next));
  assert(next.angle==-kM1AngleStep && clock.Balanced());
  std::cout << "T890: ROI bounds, timestamp wrap, and grant angle freeze passed\n";
}
