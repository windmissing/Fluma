#pragma once
#include "root_binding.h"
#include "pc_steady_spirv.h"

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
}
