#pragma once
#include "pass.h"
#include <filesystem>
#include <functional>
#include <thread>
#include <vector>
#include <memory>
#ifdef PC_SAND_G11_R1
#include "r1.h"
#else
#include "dm2.h"
#endif

namespace grs::pc::sand {
class VulkanOwner {
 public:
  struct Context {
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamily;
    PFN_vkGetDeviceProcAddr load;
    VkPhysicalDeviceLimits limits;
  };
  struct Readback {
    Slice buffer;
    VkDeviceMemory memory;
    VkDeviceSize allocationBytes, mappedOffset;
    const uint32_t *words;
    bool coherent;
  };
  struct Upload {
    Slice destination;
    const uint32_t *words;
  };
  struct BootReceipt {
    VkResult result=VK_NOT_READY;
    uint32_t error=0, seal=0;
    bool eligible=false;
    std::array<uint8_t,32> visualSeedSHA{};
  };
  struct Capture {
#ifdef PC_SAND_G11_R1
    Readback staging;
    WitnessSelection selection{};
#else
    const Slice *discovery=nullptr; // Six DM2 arenas reserved before the root.
#endif
  };
  struct RootReceipt {
    VkResult result=VK_NOT_READY;
    VkFence fence=VK_NULL_HANDLE;
    uint64_t generation=0;
    uint32_t dispatches=0;
    std::array<uint32_t,256> tail{};
    bool read=false;
    uint32_t captured=0;
  };
#ifdef PC_SAND_G11_R1
  using Permit=g11vis::CompositePermit;
  using AcceptedProof=g11vis::ValidatedCompositeAccepted;
#else
  using Permit=g9dm::Permit;
  using AcceptedProof=g9dm::ValidatedRecord;
#endif
  struct PreparedWords {
    std::array<uint32_t,224> words{};
    accept647::Digest hash{};
  };
  VulkanOwner();
  VulkanOwner(const VulkanOwner &)=delete;
  VulkanOwner &operator=(const VulkanOwner &)=delete;
  ~VulkanOwner();
  VkResult Open(const Context &,const std::filesystem::path &modules);
  // Caller holds the same resource generation through qualification and roots.
  // Uploads contain the original frozen seed, WORK/config and Common, retained through recording.
  BootReceipt Qualify(const StorageSlots &, const Slice &common,const Readback &,
                      const Upload *,size_t,const std::function<bool()> &current,uint64_t timeoutNs);
  RootReceipt Run(Root,const StorageSlots &,const Slice &common,const Readback &,const Upload *,size_t,
                  const std::function<bool()> &current,uint64_t absoluteDeadlineNs,
                  const std::function<uint64_t()> &nowNs,const Capture *capture=nullptr);
  VkResult Retire(uint64_t timeoutNs);
  bool HasPendingWork() const { return inFlight_; }
  bool Eligible() const { return eligible_; }
  const char *RoleSetSHA() const { return kRoleSetSHA; }
  const char *RootModuleSHA(Root) const;
  std::optional<PreparedWords> Prepare(const Permit &,accept647::ProgramFacts);
  std::optional<AcceptedProof> AcceptCurrent(const std::function<bool()> &current,
                                             const std::function<uint64_t()> &nowNs);
#ifdef PC_SAND_G11_R1
  struct WitnessView {
    WitnessSelection selection;
    uint64_t nonce;
    const uint32_t *bankAndPlan; // 256 words from the GPU; excludes the native archive prefix.
    const uint32_t *payload;
    size_t payloadBytes;
  };
  std::optional<WitnessView> ReadR1Witness(const AcceptedProof &,const std::function<bool()> &current);
#else
  bool CaptureDmArchive(std::ostream &,const AcceptedProof &,const DmColdSources &,bool full,
                        const Readback &,const DmColdMetadata &,const std::function<bool()> &current,
                        uint64_t timeoutNs);
#endif
 private:
  struct Api;
  struct Prepared;
  struct Pipeline { uint32_t opcode; VkPipeline object; };
  Api *api_=nullptr;
  Context context_{};
  VkDescriptorSetLayout bindings_=VK_NULL_HANDLE;
  VkPipelineLayout layout_=VK_NULL_HANDLE;
  VkDescriptorPool descriptors_=VK_NULL_HANDLE;
  VkCommandPool pool_=VK_NULL_HANDLE;
  VkCommandBuffer command_=VK_NULL_HANDLE;
  VkFence fence_=VK_NULL_HANDLE;
  std::vector<Pipeline> pipelines_;
  std::unique_ptr<Prepared> prepared_;
  std::thread::id thread_;
  uint64_t generation_=0, qualifiedWork_=0;
  accept647::Digest visualSeed_{};
#ifdef PC_SAND_G11_R1
  std::optional<Capture> acceptedWitness_;
  uint64_t witnessNonce_=0,witnessGeneration_=0;
#else
  uint64_t acceptedNonce_=0;
  StorageSlots acceptedStorage_{};
  std::array<Slice,6> acceptedDiscovery_{};
#endif
  VkBuffer qualifiedBuffer_=VK_NULL_HANDLE;
  bool inFlight_=false,eligible_=false,opened_=false;
  bool OnOwner() const { return thread_==std::this_thread::get_id(); }
  VkResult Begin(const Upload *,size_t);
  VkResult Submit();
  VkResult MapReady(const Readback &, VkDeviceSize);
  bool Dispatch(const Pass &,const Command &,const StorageSlots &,const Slice &common,
                VkDescriptorSet);
};
}
