#pragma once
#include "pass.h"
#include "../../../entry/src/main/cpp/gpu_resident/g9dm/completion_gate.h"
#include <iosfwd>

namespace grs::pc::sand {
inline constexpr size_t kDmColdRootBytes=26750976, kDmColdFullBytes=103354368;
struct DmColdSources {
  Slice base, final, selected, authority, work;
  std::array<Slice,6> discovery;
};
struct DmColdPlan {
  // GPU summary + controls, then the original archive's four or ten segments.
  std::array<CopySpan,12> copies{};
  size_t count=0, bytes=0;
};
std::optional<DmColdPlan> PlanDmCold(const DmColdSources &, bool full);

struct DmColdMetadata {
  uint64_t request, trace, createdNs;
  g9dm::Input128 input;
  std::array<uint32_t,128> config;
  std::array<uint32_t,256> quota;
};
struct DmColdReadback {
  PFN_vkWaitForFences wait;
  PFN_vkInvalidateMappedMemoryRanges invalidate;
  VkDevice device;
  VkFence copyFence;
  VkDeviceMemory memory;
  VkDeviceSize allocationBytes, atom, offset, mappedBytes;
  bool coherent;
  const uint32_t *words; // mapped pointer at offset, pinned until the stream is complete
};
// Call only after recording the accepted root's copy plan and host barrier.
// On a failed wait or invalidate, leaves the sink untouched.
bool WriteDmCold(std::ostream &, const DmColdPlan &, const DmColdReadback &, uint64_t timeoutNs,
                 const g9dm::ValidatedRecord &, const void *owner, const DmColdMetadata &);
}
