#include "../../pc/vulkan/g13_candidate_timing.h"
#include "../../pc/vulkan/root_plan.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  using namespace grs;
  using namespace grs::pc;
  constexpr uint64_t ms=1000000;
  assert(CandidatePollNs(249*ms,250*ms)==ms);
  assert(CandidatePollNs(200*ms,250*ms)==2*ms);
  assert(CandidatePollNs(250*ms,250*ms)==0);
  assert(CandidatePollNs(251*ms,250*ms)==0);

  CandidateTiming wait;
  wait.AddPoll({12,13,2000000,2});
  wait.Stop("WINDOW_REVOKED",14);
  wait.retire={15,16,kCandidateJobNs,0};
  wait.Stop("FENCE_RESULT",16);
  assert(wait.count==1 && wait.last.result==2 && wait.retire.result==0);
  assert(!std::strcmp(wait.stop,"WINDOW_REVOKED") && wait.stopQpc==14);
  for (uint64_t i=0;i<200;++i) wait.AddPoll({20+i,21+i,1,2});
  assert(wait.count==201 && wait.last.exitQpc==220 && wait.polls.back().exitQpc==147);

  const CandidatePulse a{18,11,12,1*ms,2*ms}, b{19,21,22,17*ms,18*ms}, c{20,31,32,33*ms,34*ms};
  assert(!CandidatePulseOrder(a,b) && !CandidatePulseOrder(b,c));
  assert(!CandidatePulseFreshness(c,39*ms));
  assert(!std::strcmp(CandidatePulseFreshness(c,68*ms),"PULSE_STALE"));
  assert(!std::strcmp(CandidatePulseOrder(a,c),"PULSE_SEQUENCE_GAP"));
  assert(!std::strcmp(CandidatePulseOrder(b,{20,31,32,53*ms,54*ms}),"PULSE_GAP"));
  assert(!std::strcmp(CandidatePulseOrder(b,{20,31,32,16*ms,18*ms}),"PULSE_REVERSED"));

  std::string_view role;
  unsigned set=~0u,dispatches=0,roleBinds=0,setBinds=0,op17=0;
  for (size_t i=0;i<std::size(g13::k_quantum);++i) {
    CandidatePass step{};
    assert(PlanSplitCandidate(false,i,g13::k_quantum[i],step));
    if (set!=CandidateSet(step.binding)) { set=CandidateSet(step.binding); ++setBinds; }
    for (size_t j=0;j<step.count;++j) {
      if (role!=step.roles[j]) { role=step.roles[j]; ++roleBinds; }
      ++dispatches;
    }
    op17+=step.binding.push[0]==17;
  }
  assert(dispatches==850 && roleBinds==160 && setBinds==6 && op17==14);
  std::cout << "T877 timing, pulse and binding-plan checks passed\n";
}
