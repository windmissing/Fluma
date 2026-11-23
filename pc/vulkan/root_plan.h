#pragma once
#include "root_binding.h"
#include "pc_steady_spirv.h"
#include <string_view>

namespace grs::pc {
struct RootPlanPass {
  unsigned role;
  unsigned baseSlot;
  unsigned targetSlot;
  std::array<uint32_t, 6> push;
};

inline constexpr bool PlanRootPass(bool source, size_t ordinal, const g13::Pass &pass,
                                   RootPlanPass &out) {
  g13::RootBinding binding{};
  if (!g13::ResolveRootBinding(source, ordinal, pass, binding) ||
      pass.op >= std::size(kOpcodeToRole) || !kOpcodeToRole[pass.op])
    return false;
  out = {kOpcodeToRole[pass.op], binding.candidateBase ? 1u : 0u,
         binding.target == g13::RootTarget::Pack ? 4u :
         binding.target == g13::RootTarget::Replay ? 2u : 1u,
         {pass.op, binding.shaderScope, pass.J, pass.micro, pass.block, uint32_t(ordinal)}};
  return out.baseSlot != out.targetSlot;
}

template<size_t N>
inline constexpr bool ValidRootPlan(bool source, const g13::Pass (&passes)[N]) {
  for (size_t ordinal = 0; ordinal < N; ++ordinal) {
    RootPlanPass pass{};
    if (!PlanRootPass(source, ordinal, passes[ordinal], pass)) return false;
  }
  return true;
}

static_assert(std::size(g13::k_source) == 16 && ValidRootPlan(true, g13::k_source));
static_assert(std::size(g13::k_quantum) == 820 && ValidRootPlan(false, g13::k_quantum));

struct CandidatePass {
  RootPlanPass binding{};
  std::array<std::string_view,3> roles{};
  uint32_t count=0;
};
inline constexpr unsigned CandidateSet(const RootPlanPass &binding) {
  return binding.targetSlot==4 ? 3u : binding.baseSlot==1 ? 2u : binding.targetSlot==2 ? 1u : 0u;
}

// An explicit candidate table. The canonical role and logical plans stay intact.
inline constexpr bool PlanSplitCandidate(bool source,size_t ordinal,const g13::Pass &pass,
                                         CandidatePass &out) {
  out={};
  if (!PlanRootPass(source,ordinal,pass,out.binding)) return false;
  out.count=1;
  const auto op=pass.op;
  if (op<=5) out.roles[0]="OP_00_0_5";
  else if (op==6) out.roles[0]="OP6";
  else if (op==7) {
    out.roles={"OP7_FIELD_ONLY","OP7_MOTION_ONLY","OP7_MMS_FORCE_ONLY"};
    out.count=3;
  } else if (op==8) out.roles[0]="OP8";
  else if (op<=11) out.roles[0]="OP_00_9_11";
  else if (op<=14) out.roles[0]="OP12_14";
  else if (op==15) out.roles[0]="OP15";
  else if (op==16) out.roles[0]="OP16";
  else if (op==17) {
    out.roles={"OP17_PRIVATE_MOTION","OP17_PRIVATE_MMS","OP17_PRIVATE_LEDGER"};
    out.count=3;
  } else {
    if (out.binding.role>5) return false;
    out.roles[0]=kSteadyModules[out.binding.role-1].name;
  }
  return true;
}

template<size_t N>
constexpr bool ValidSplitCandidate(bool source,const g13::Pass (&passes)[N],
                                   uint32_t actual,uint32_t op17Count) {
  uint32_t encoded=0, slots=0;
  for (size_t i=0;i<N;++i) {
    CandidatePass pass;
    if (!PlanSplitCandidate(source,i,passes[i],pass)) return false;
    encoded+=pass.count;
    slots+=passes[i].op==17;
  }
  return encoded==actual && slots==op17Count;
}
static_assert(ValidSplitCandidate(true,g13::k_source,16,0));
static_assert(ValidSplitCandidate(false,g13::k_quantum,850,14));
}
