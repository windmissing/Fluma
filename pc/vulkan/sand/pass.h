#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>

namespace grs::pc::sand {
enum class Root { Source, Quantum };
struct Pass {
  uint32_t opcode, role, j;
  int32_t micro;
  uint32_t visual, phase;
  bool hasModule;
  uint32_t bindingOpcode=UINT32_MAX;
};
struct ModuleFile { uint32_t opcode, bytes; const char *name, *sha; };
struct GraphicsFile { const char *group, *stage; uint32_t bytes; const char *name, *sha; };
}
#include "sand_passes.h"

namespace grs::pc::sand {
inline constexpr VkDeviceSize kReplayIndirectOffset=1311360;
inline constexpr VkDeviceSize kReplayIndirectBytes=388;
inline constexpr size_t kReplayIndirectProducer=2007, kReplayIndirectFirst=2013, kReplayIndirectEnd=2397;
static_assert(sizeof(VkDispatchIndirectCommand)==12);

struct Command {
  std::array<uint8_t,4> storageSlots;
  std::array<uint32_t,9> push;
  bool freezeSourceBase, saveSelection;
};
using Common = std::array<uint8_t,kCommonBytes>;
void PutCommonWord(Common &, size_t byteOffset, uint32_t);
std::optional<Command> Plan(Root, size_t ordinal);

// A set allocated and frozen by the Vulkan owner for the exact pass bindings.
struct BoundPass {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  VkDescriptorSet descriptors;
  uint32_t opcode;
  std::array<uint8_t,4> storageSlots;
};
struct DispatchApi {
  PFN_vkCmdBindPipeline bind;
  PFN_vkCmdBindDescriptorSets descriptors;
  PFN_vkCmdPushConstants push;
  PFN_vkCmdDispatch dispatch;
  PFN_vkCmdPipelineBarrier barrier;
  PFN_vkCmdDispatchIndirect dispatchIndirect=nullptr;
};
bool Record(const DispatchApi &, VkCommandBuffer, const Pass &, const Command &, const BoundPass &,
            VkBuffer indirect=VK_NULL_HANDLE,VkDeviceSize indirectOffset=0);

struct Slice {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceSize offset = 0, bytes = 0;
  uint64_t generation = 0;
};
using StorageSlots = std::array<Slice,10>;
bool WriteDescriptors(PFN_vkUpdateDescriptorSets, VkDevice, VkDescriptorSet, const Command &,
                      const StorageSlots &, const Slice &common, const VkPhysicalDeviceLimits &,
                      bool initialAudit=false);
bool RecordParentCopies(PFN_vkCmdPipelineBarrier, PFN_vkCmdCopyBuffer, VkCommandBuffer,
                        const Command &, const StorageSlots &, VkDeviceSize stateBytes, VkDeviceSize arenaBytes);
struct CopySpan {
  Slice source;
  VkDeviceSize destination;
};
bool RecordReadback(PFN_vkCmdPipelineBarrier, PFN_vkCmdCopyBuffer, VkCommandBuffer,
                    const Slice &staging, const CopySpan *, size_t count);
bool MakeHostVisible(PFN_vkCmdPipelineBarrier, VkCommandBuffer, const Slice &staging);
VkResult AwaitReadback(PFN_vkWaitForFences, PFN_vkInvalidateMappedMemoryRanges, VkDevice,
                       VkFence, uint64_t timeoutNs, bool coherent, VkDeviceMemory,
                       VkDeviceSize allocationBytes, VkDeviceSize atom,
                       VkDeviceSize begin, VkDeviceSize bytes,
                       VkResult *fenceResult=nullptr,VkResult *invalidateResult=nullptr);
}
