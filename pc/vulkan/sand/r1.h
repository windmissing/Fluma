#pragma once
#include "pass.h"
#include "../../../entry/src/main/cpp/gpu_resident/g11vis/composite_gate.h"
#include "../../../entry/src/main/cpp/gpu_resident/g11vis/generated_contract.h"

namespace grs::pc::sand {
inline constexpr char kR1Publication[]="G11_JOINT_PUBLICATION_V2";
inline constexpr char kR1Witness[]="G11_PHASE_WITNESS_V2";
inline constexpr size_t kR1WitnessBytes=3149824;
inline constexpr VkVertexInputBindingDescription kR1GlyphBinding{0,32,VK_VERTEX_INPUT_RATE_INSTANCE};
inline constexpr std::array<VkVertexInputAttributeDescription,2> kR1GlyphAttributes{{
  {0,0,VK_FORMAT_R32G32B32A32_UINT,0}, {1,0,VK_FORMAT_R32G32B32A32_UINT,16}
}};
struct R1ViewPush {
  float resolution[2], padding[2], box[4];
  int32_t columns;
  uint32_t foreground;
};
static_assert(offsetof(R1ViewPush,box)==16 && offsetof(R1ViewPush,columns)==32 && sizeof(R1ViewPush)==40);
struct WitnessSelection { uint32_t micro, phase, role; };
struct WitnessPlan {
  size_t beginOrdinal, applyOrdinal;
  CopySpan begin;
  std::array<CopySpan,5> apply;
};
// bank is the actual WORK marker bank mapped by the owner for this role.
std::optional<WitnessPlan> PlanR1Witness(Root, WitnessSelection, const Slice &work, uint32_t bank);

struct WindowKey {
  uint64_t run, owner, context, window, surface, geometry, permission, swapchain;
  bool operator==(const WindowKey &x) const {
    return run==x.run && owner==x.owner && context==x.context && window==x.window &&
           surface==x.surface && geometry==x.geometry && permission==x.permission && swapchain==x.swapchain;
  }
};
struct R1Scene {
  WindowKey key;
  const void *producer;
  uint64_t nonce, commit, acceptedQuanta, visualGeneration, visualTicks, leaseSerial;
  Slice state, markers, glyphs, styles;
};
struct PresentReceipt {
  VkFence fenceObject;
  VkResult acquire, submit, fence, present, imagePresent;
  uint32_t imageIndex;
  uint64_t enterQpc, exitQpc;
};
struct PresentedMapping {
  uint64_t nonce, commit, visualGeneration, leaseSerial;
  uint32_t imageIndex;
  uint64_t enterQpc, exitQpc;
};
class R1SceneGate {
 public:
  bool Acquire(const g11vis::ValidatedCompositeAccepted &, const R1Scene &, const WindowKey &);
  bool CanPresent(const WindowKey &) const;
  bool BindGlyphs(const WindowKey &, PFN_vkCmdBindVertexBuffers, VkCommandBuffer) const;
  bool Submitted(const WindowKey &, VkFence);
  std::optional<PresentedMapping> Presented(const WindowKey &,const PresentReceipt &);
  bool CancelUnsubmitted();
  // Signal/device-loss closes this graphics pin; timeout leaves it pinned.
  VkResult Retire(PFN_vkWaitForFences, VkDevice, uint64_t timeoutNs);
 private:
  std::optional<R1Scene> scene_;
  VkFence fence_=VK_NULL_HANDLE;
  uint64_t previousSerial_=0;
  bool presented_=false;
};
}
