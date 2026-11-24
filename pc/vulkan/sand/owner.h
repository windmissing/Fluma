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
    uint32_t timestampBits=0;
    bool sourceTimestamps=false;
    bool sourceDetail=false;
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
  struct CommandPreparation {
    struct Call { uint64_t enter=0,exit=0; VkResult result=VK_NOT_READY; };
    std::array<Call,3> calls{};
    uint64_t priorGeneration=0,useGeneration=0;
    uintptr_t pool=0,fence=0;
    Call retireProof{};
    bool outsideGrant=false;
  };
  struct RootReceipt {
    VkResult result=VK_NOT_READY;
    VkFence fence=VK_NULL_HANDLE;
    uint64_t generation=0;
    uint32_t dispatches=0;
    std::array<uint32_t,256> tail{};
    bool read=false;
    uint32_t captured=0;
    struct Timeline {
      struct Wait { uint64_t enter=0,exit=0,timeout=0; VkResult result=VK_NOT_READY; };
      std::array<Wait,128> waits{};
      Wait lastWait{};
      size_t calls=0;
      uint64_t recordEnter=0,recordExit=0,submitEnter=0,submitExit=0,stopNs=0;
      uint64_t readEnter=0,readExit=0;
      uint64_t descriptorNs=0,commandResetNs=0,uploadRecordNs=0,dispatchRecordNs=0,copyRecordNs=0,endCommandNs=0;
      bool descriptorPrewarmed=false;
      CommandPreparation commands{};
      VkResult submit=VK_NOT_READY,readFence=VK_NOT_READY,invalidate=VK_NOT_READY;
      const char *stop="PRECONDITION";
    } timing;
  };
  struct RootIdentity { uint64_t run,nonce,plan; };
  struct AcceptTiming {
    uint64_t gateEnter=0,gateExit=0;
    const char *result="TAIL_NOT_READY";
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
  VkResult QualifyG15FlightPath(const Context &,const std::filesystem::path &module,
                                const std::filesystem::path &prefix);
  static bool CheckRolePackage(const std::filesystem::path &modules);
  // Caller holds the same resource generation through qualification and roots.
  // Uploads contain the original frozen seed, WORK/config and Common, retained through recording.
  BootReceipt Qualify(const StorageSlots &, const Slice &common,const Readback &,
                      const Upload *,size_t,const std::function<bool()> &current,uint64_t timeoutNs);
  RootReceipt Run(Root,const StorageSlots &,const Slice &common,const Readback &,const Upload *,size_t,
                  const std::function<bool()> &current,uint64_t absoluteDeadlineNs,
                  const std::function<uint64_t()> &nowNs,const Capture *capture=nullptr,
                  const RootIdentity *diagnostic=nullptr);
  VkResult PrewarmSource(const StorageSlots &,const Slice &common);
  VkResult PrepareRetiredSource(CommandPreparation &);
  void FinishSourceDiagnostic(const RootReceipt &,bool accepted,uint64_t retireTimeoutNs);
  VkResult ReadRawEnvelope(const RootReceipt &,const StorageSlots &,const Readback &,uint64_t timeoutNs);
  VkResult Retire(uint64_t timeoutNs);
  bool HasPendingWork() const { return inFlight_; }
  bool Eligible() const { return eligible_; }
  const char *RoleSetSHA() const { return kRoleSetSHA; }
  const char *RootModuleSHA(Root) const;
  std::optional<PreparedWords> Prepare(const Permit &,accept647::ProgramFacts);
  std::optional<AcceptedProof> AcceptCurrent(const std::function<bool()> &current,
                                             const std::function<uint64_t()> &nowNs,AcceptTiming *timing=nullptr);
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
  VkResult OpenPipelines(const Context &,const std::vector<uint32_t> &opcodes,
                         const std::vector<std::vector<uint32_t>> &modules,bool qualify);
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
  VkQueryPool sourceQueries_=VK_NULL_HANDLE;
  VkResult queryCreate_=VK_NOT_READY;
  std::optional<RootIdentity> submittedSource_;
  StorageSlots sourceWarmStorage_{};
  Slice sourceWarmCommon_{};
  std::array<VkDescriptorSet,std::size(kSource)> sourceWarmSets_{};
  bool sourceWarm_=false;
  std::optional<CommandPreparation> sourcePreparedCommands_;
  StorageSlots rawSubmittedStorage_{};
  uint64_t rawSubmittedGeneration_=0;
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
  VkResult Begin(const Upload *,size_t,bool timestamp=false,bool prewarmed=false,RootReceipt::Timeline *timing=nullptr,
                 CommandPreparation *commands=nullptr,bool usePreparedCommands=false);
  VkResult ResetForRecording(CommandPreparation *);
  VkResult Submit(RootReceipt::Timeline *timing=nullptr);
  VkResult MapReady(const Readback &, VkDeviceSize, RootReceipt::Timeline *timing=nullptr);
  void ReportSourceGpu(const char *,VkResult,bool accepted);
  void DrainPending();
  bool Dispatch(const Pass &,const Command &,const StorageSlots &,const Slice &common,
                VkDescriptorSet);
};
}
