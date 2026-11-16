#pragma once
#include "completion_gate.h"
#include "progress_clock_v3.h"
namespace grs::m3b {
class PlanningControl {
    ProgressClockV3& clock_;
    RunControl& run_;
    ControlResult Recheck(const RootPermitV5&,uint64_t now)const;
public:
    PlanningControl(ProgressClockV3& clock,RunControl& run):clock_(clock),run_(run){}
    ControlResult BlockPlanningControlLocked(const ValidatedBlockedTailV5&,uint64_t nativeNow);
    ControlResult AcceptQuantumLocked(const ValidatedSuccessTailV5&,uint64_t nativeNow);
};
}
