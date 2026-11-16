#pragma once
#include "completion_gate.h"
#include "progress_clock_v3.h"
namespace grs::g9spf {
class PlanningControl {
 ProgressClockV3& clock_;RunControl& run_;
 ControlResult Recheck(const Permit& p,uint64_t now)const;
public:
 PlanningControl(ProgressClockV3& clock,RunControl& run):clock_(clock),run_(run){}
 ControlResult BlockPlanningControlLocked(const ValidatedBlockedControlRecord&,uint64_t now);
 ControlResult AcceptQuantumLocked(const ValidatedRecord&,uint64_t now);
};
}
