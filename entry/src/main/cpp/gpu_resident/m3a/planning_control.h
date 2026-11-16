#pragma once
#include "completion_gate.h"
#include "progress_clock_v3.h"
namespace grs::m3a {
class PlanningControl {
    ProgressClockV3& clock_;
    RunControl& run_;
    ControlResult Recheck(const RootPermitV4&,uint64_t now)const;
public:
    PlanningControl(ProgressClockV3& clock,RunControl& run):clock_(clock),run_(run){}
    ControlResult BlockPlanningControlLocked(const ValidatedBlockedTailV4&,uint64_t nativeNow);
    ControlResult AcceptQuantumLocked(const ValidatedSuccessTailV4&,uint64_t nativeNow);
};
}
