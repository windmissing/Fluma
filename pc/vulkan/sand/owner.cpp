#include "owner.h"
#include "../../../entry/src/main/cpp/gpu_resident/program_cache/sha256.h"
#include <algorithm>
#include <fstream>
#include <new>
#ifdef PC_SAND_G11_R1
#include "../../../entry/src/main/cpp/gpu_resident/g11vis/generated_contract.h"
#endif

namespace grs::pc::sand {
namespace {
constexpr uint32_t warmOpcode=0x7fffff00;
constexpr VkDeviceSize stateBytes=591872, controlsOffset=1166976, tailOffset=1171072;
#ifdef PC_SAND_G11_R1
constexpr VkDeviceSize arenaBytes=11718656, workBytes=4825728, auditBytes=512;
constexpr uint32_t coldOpcode=0x7fffff01, goodSeal=0xc01d0002;
constexpr auto &visual=g11vis::kVisualResources[0];
constexpr VkDeviceSize seedBytes=512+visual.v*128+8192+256;
constexpr VkDeviceSize bootReadbackBytes=auditBytes+seedBytes;
#else
constexpr VkDeviceSize arenaBytes=12767232, workBytes=1311360, auditBytes=8;
constexpr uint32_t coldOpcode=warmOpcode, goodSeal=0x641647;
constexpr VkDeviceSize bootReadbackBytes=auditBytes;
#endif
constexpr uint64_t maxJobNs=250000000;
constexpr uint64_t pollNs=2000000;

bool SliceFits(const Slice &slice,VkDeviceSize bytes) {
  return slice.buffer && slice.generation && bytes<=slice.bytes &&
         slice.offset%4==0 && bytes<=UINT64_MAX-slice.offset;
}

bool SameHash(const std::vector<uint32_t> &words,const char *sha) {
  p011gpu::Sha256 digest;
  digest.update(reinterpret_cast<const uint8_t*>(words.data()),words.size()*4);
  const auto raw=digest.finish();
  constexpr char hex[]="0123456789abcdef";
  for (size_t i=0;i<raw.size();++i)
    if (sha[i*2]!=hex[raw[i]>>4] || sha[i*2+1]!=hex[raw[i]&15]) return false;
  return true;
}

accept647::Digest Digest(const char *hex) {
  accept647::Digest value{};
  const auto digit=[](char c) { return uint32_t(c<='9' ? c-'0' : c-'a'+10); };
  for (size_t i=0;i<32;++i)
    value[i/4]|=(digit(hex[2*i])*16+digit(hex[2*i+1]))<<(8*(i%4));
  return value;
}
}

#define SAND_DEVICE(F) F(CreateShaderModule) F(DestroyShaderModule) F(CreateComputePipelines) F(DestroyPipeline) \
  F(CreateDescriptorSetLayout) F(DestroyDescriptorSetLayout) F(CreatePipelineLayout) F(DestroyPipelineLayout) \
  F(CreateDescriptorPool) F(DestroyDescriptorPool) F(ResetDescriptorPool) F(AllocateDescriptorSets) \
  F(UpdateDescriptorSets) F(CreateCommandPool) F(DestroyCommandPool) F(ResetCommandPool) F(AllocateCommandBuffers) \
  F(BeginCommandBuffer) F(EndCommandBuffer) F(CmdUpdateBuffer) F(CmdCopyBuffer) F(CmdPipelineBarrier) \
  F(CmdBindPipeline) F(CmdBindDescriptorSets) F(CmdPushConstants) F(CmdDispatch) F(CreateFence) \
  F(DestroyFence) F(ResetFences) F(WaitForFences) F(InvalidateMappedMemoryRanges) F(QueueSubmit) F(DeviceWaitIdle)
struct VulkanOwner::Api {
#define MEMBER(name) PFN_vk##name name=nullptr;
  SAND_DEVICE(MEMBER)
#undef MEMBER
};

struct VulkanOwner::Prepared {
  Permit permit;
  accept647::ActiveProgramIdentity active;
  std::optional<accept647::ExpectedContract> expected;
  accept647::ActualEvidence evidence{};
  accept647::Tail tail{};
  StorageSlots storage{};
  std::array<Slice,6> discovery{};
#ifdef PC_SAND_G11_R1
  std::optional<Capture> witness;
#endif
  uint64_t generation=0;
  Prepared(Permit p,accept647::ActiveProgramIdentity a) : permit(std::move(p)),active(std::move(a)) {}
};

VulkanOwner::VulkanOwner()=default;

const char *VulkanOwner::RootModuleSHA(Root root) const {
  const auto op=root==Root::Source ? kSource[std::size(kSource)-1].opcode : kQuantum[std::size(kQuantum)-1].opcode;
  const auto module=std::find_if(std::begin(kModules),std::end(kModules),
                                 [&](const auto &m) { return m.opcode==op; });
  return module!=std::end(kModules) ? module->sha : nullptr;
}

std::optional<VulkanOwner::PreparedWords> VulkanOwner::Prepare(const Permit &permit,accept647::ProgramFacts facts) {
  if (!opened_ || !OnOwner() || !eligible_ || inFlight_ || prepared_) return {};
#ifdef PC_SAND_G11_R1
  const auto *p=std::get_if<g9spf::Permit>(&permit.parent);
  if (!p || permit.compositeCode!=g11vis::kCompositeR1Code ||
      permit.pipelineSha!=Digest(kRoleSetSHA) || permit.visualConfigSha!=facts.visualConfig) return {};
  const uint64_t begin=permit.rootBeginNs;
  facts.v=2048;
  facts.baseVisualGeneration=permit.visualGeneration;
  facts.baseVisualTicks=permit.visualTicks;
  facts.actualVisualSeed=visualSeed_;
  constexpr uint32_t parentKind=1;
#else
  const auto *p=&permit;
  const uint64_t begin=p->rootBeginNs;
  constexpr uint32_t parentKind=2;
#endif
  if ((p->kind!=1 && p->kind!=2) || p->baseBank!=0 || p->candidateBank!=2 || !begin ||
      p->deadline<=begin || p->deadline-begin>maxJobNs) return {};
  const auto root=p->kind==1 ? Root::Source : Root::Quantum;
  const auto *shader=RootModuleSHA(root);
  if (!shader || Digest(shader)!=p->shaderSha) return {};
  const auto op=p->kind==1 ? kSource[std::size(kSource)-1].opcode : kQuantum[std::size(kQuantum)-1].opcode;
  const auto writer=std::find_if(pipelines_.begin(),pipelines_.end(),[&](const auto &r) { return r.opcode==op; });
  if (writer==pipelines_.end()) return {};
  facts.producer=this;
  facts.program=reinterpret_cast<uintptr_t>(writer->object);
  facts.context=reinterpret_cast<uintptr_t>(context_.device);
  facts.owner=thread_;
  facts.k=192;
  facts.stateBytes=stateBytes;
  facts.workBytes=workBytes;
  facts.pathCapacity=49152;
#ifdef PC_SAND_G11_R1
  facts.eventCapacity=33488;
#else
  facts.eventCapacity=37584;
#endif
  facts.actualShader=Digest(shader);
  facts.activePipeline=Digest(kRoleSetSHA);
#ifdef PC_SAND_G11_R1
  prepared_=std::make_unique<Prepared>(permit,accept647::ActiveProgramIdentity::ForComposite(facts,parentKind));
  const auto &frozen=std::get<g9spf::Permit>(prepared_->permit.parent);
#else
  prepared_=std::make_unique<Prepared>(permit,accept647::ActiveProgramIdentity::ForDefaultDm(facts));
  const auto &frozen=prepared_->permit;
#endif
  prepared_->expected=accept647::ExpectedForActivePipeline(prepared_->active,
      accept647::PermitView::Of(frozen,begin),parentKind);
  if (!prepared_->expected) { prepared_.reset(); return {}; }
  PreparedWords result;
  prepared_->expected->Serialize(result.words.data(),result.words.size());
  result.hash=prepared_->expected->Hash();
  return result;
}

std::optional<VulkanOwner::AcceptedProof> VulkanOwner::AcceptCurrent(const std::function<bool()> &current,
    const std::function<uint64_t()> &now) {
  if (!OnOwner() || !prepared_) return {};
  auto job=std::move(prepared_);
  if (!job->expected || !job->generation || job->generation!=generation_ || !job->evidence.mapped ||
      !eligible_ || inFlight_ || !current || !now || !current() || now()>job->expected->U64(28)) return {};
  accept647::NativeMappedObservation observation(job->active,job->tail,job->evidence);
#ifdef PC_SAND_G11_R1
  auto proof=g11vis::CompositeGate::Accepted(job->permit,*job->expected,observation);
  if (proof && proof->Words()[6]!=2 && job->witness) {
    acceptedWitness_=job->witness;
    witnessNonce_=proof->Permission().Nonce();
    witnessGeneration_=job->generation;
  }
  return proof;
#else
  auto proof=g9dm::CompletionGate::Accepted(job->permit,*job->expected,observation);
  if (proof && proof->Words()[6]!=2) {
    acceptedNonce_=proof->Permission().nonce;
    acceptedStorage_=job->storage;
    acceptedDiscovery_=job->discovery;
  }
  return proof;
#endif
}

#ifdef PC_SAND_G11_R1
std::optional<VulkanOwner::WitnessView> VulkanOwner::ReadR1Witness(const AcceptedProof &proof,
    const std::function<bool()> &current) {
  if (!OnOwner() || inFlight_ || !current || !current() || !acceptedWitness_ ||
      witnessGeneration_!=generation_ || proof.Producer()!=this ||
      proof.Permission().Nonce()!=witnessNonce_) return {};
  const auto &capture=*acceptedWitness_;
  const auto &r=capture.staging;
  if (!r.words || MapReady(r,kR1WitnessBytes)!=VK_SUCCESS || !current()) return {};
  const auto *bank=r.words+1024/4;
  const auto *plan=r.words+1536/4;
  const auto &tail=proof.Words();
  if (plan[0]!=1345401159u || plan[44]!=2 || plan[127]!=3235778820u || accept647::Pair(plan,6)!=witnessNonce_ ||
      plan[10]!=capture.selection.role || plan[11]!=capture.selection.micro ||
      plan[12]!=capture.selection.phase || accept647::Pair(plan,4)!=accept647::Pair(tail.data(),210) ||
      !std::equal(plan+16,plan+24,tail.begin()+168) || !std::equal(plan+24,plan+32,tail.begin()+200))
    return {};
  return WitnessView{capture.selection,witnessNonce_,bank,r.words+4096/4,kR1WitnessBytes-4096};
}
#else
bool VulkanOwner::CaptureDmArchive(std::ostream &sink,const AcceptedProof &proof,
    const DmColdSources &sources,bool full,const Readback &readback,const DmColdMetadata &meta,
    const std::function<bool()> &current,uint64_t timeout) {
  if (!OnOwner() || inFlight_ || prepared_ || !current || !current() || !timeout ||
      !acceptedNonce_ || acceptedNonce_!=proof.Permission().nonce || !proof.ProducedBy(this) ||
      (full && proof.Permission().kind!=2) || !readback.words) return false;
  const auto same=[](const Slice &a,const Slice &b) {
    return a.buffer==b.buffer && a.generation==b.generation && a.offset==b.offset && a.bytes==b.bytes;
  };
  if (!same(sources.base,acceptedStorage_[1]) || !same(sources.final,acceptedStorage_[2]) ||
      !same(sources.selected,acceptedStorage_[7]) || !same(sources.authority,acceptedStorage_[6]) ||
      !same(sources.work,acceptedStorage_[5])) return false;
  if (full)
    for (size_t i=0;i<sources.discovery.size();++i)
      if (!same(sources.discovery[i],acceptedDiscovery_[i])) return false;
  const auto plan=PlanDmCold(sources,full);
  if (!plan || !SliceFits(readback.buffer,plan->bytes) || Begin(nullptr,0)!=VK_SUCCESS ||
      !RecordReadback(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,readback.buffer,
                      plan->copies.data(),plan->count) ||
      !MakeHostVisible(api_->CmdPipelineBarrier,command_,readback.buffer) || !current() ||
      Submit()!=VK_SUCCESS || Retire(timeout)!=VK_SUCCESS || !current()) return false;
  DmColdReadback mapped{api_->WaitForFences,api_->InvalidateMappedMemoryRanges,context_.device,
    fence_,readback.memory,readback.allocationBytes,context_.limits.nonCoherentAtomSize,
    readback.mappedOffset,readback.buffer.bytes,readback.coherent,readback.words};
  return WriteDmCold(sink,*plan,mapped,0,proof,this,meta);
}
#endif

VulkanOwner::~VulkanOwner() {
  if (!api_) return;
  if (inFlight_) api_->DeviceWaitIdle(context_.device);
  for (const auto &p:pipelines_) api_->DestroyPipeline(context_.device,p.object,nullptr);
  if (fence_) api_->DestroyFence(context_.device,fence_,nullptr);
  if (pool_) api_->DestroyCommandPool(context_.device,pool_,nullptr);
  if (descriptors_) api_->DestroyDescriptorPool(context_.device,descriptors_,nullptr);
  if (layout_) api_->DestroyPipelineLayout(context_.device,layout_,nullptr);
  if (bindings_) api_->DestroyDescriptorSetLayout(context_.device,bindings_,nullptr);
  delete api_;
}

VkResult VulkanOwner::Open(const Context &context,const std::filesystem::path &directory) {
  if (api_ || !context.device || !context.queue || !context.load ||
      context.limits.maxStorageBufferRange<arenaBytes || context.limits.maxUniformBufferRange<kCommonBytes)
    return VK_ERROR_INITIALIZATION_FAILED;
  context_=context;
  thread_=std::this_thread::get_id();
  api_=new (std::nothrow) Api;
  if (!api_) return VK_ERROR_OUT_OF_HOST_MEMORY;
#define RESOLVE(name) api_->name=reinterpret_cast<PFN_vk##name>(context.load(context.device,"vk" #name)); \
  if (!api_->name) return VK_ERROR_INITIALIZATION_FAILED;
  SAND_DEVICE(RESOLVE)
#undef RESOLVE
  std::array<VkDescriptorSetLayoutBinding,5> set{};
  for (uint32_t i=0;i<set.size();++i)
    set[i]={i,i==4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
  VkDescriptorSetLayoutCreateInfo bindingInfo{};
  bindingInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  bindingInfo.bindingCount=uint32_t(set.size());
  bindingInfo.pBindings=set.data();
  auto r=api_->CreateDescriptorSetLayout(context.device,&bindingInfo,nullptr,&bindings_);
  if (r!=VK_SUCCESS) return r;
#ifdef PC_SAND_G11_R1
  constexpr uint32_t push=36;
#else
  constexpr uint32_t push=20;
#endif
  const VkPushConstantRange pushRange{VK_SHADER_STAGE_COMPUTE_BIT,0,push};
  VkPipelineLayoutCreateInfo pipelineLayout{};
  pipelineLayout.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayout.setLayoutCount=1;
  pipelineLayout.pSetLayouts=&bindings_;
  pipelineLayout.pushConstantRangeCount=1;
  pipelineLayout.pPushConstantRanges=&pushRange;
  r=api_->CreatePipelineLayout(context.device,&pipelineLayout,nullptr,&layout_);
  if (r!=VK_SUCCESS) return r;
  pipelines_.reserve(std::size(kModules));
  for (const auto &entry:kModules) {
    if (!entry.bytes || entry.bytes%4) return VK_ERROR_INITIALIZATION_FAILED;
    std::vector<uint32_t> words(entry.bytes/4);
    std::ifstream stream(directory/std::filesystem::path(entry.name),std::ios::binary);
    if (!stream.read(reinterpret_cast<char*>(words.data()),entry.bytes) ||
        stream.peek()!=std::char_traits<char>::eof() || !SameHash(words,entry.sha) || words[0]!=0x07230203)
      return VK_ERROR_INITIALIZATION_FAILED;
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize=entry.bytes;
    moduleInfo.pCode=words.data();
    VkShaderModule module=VK_NULL_HANDLE;
    r=api_->CreateShaderModule(context.device,&moduleInfo,nullptr,&module);
    if (r!=VK_SUCCESS) return r;
    VkComputePipelineCreateInfo info{};
    info.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module=module;
    info.stage.pName="main";
    info.layout=layout_;
    VkPipeline pipeline=VK_NULL_HANDLE;
    r=api_->CreateComputePipelines(context.device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline);
    api_->DestroyShaderModule(context.device,module,nullptr);
    if (r!=VK_SUCCESS) return r;
    pipelines_.push_back({entry.opcode,pipeline});
  }
  constexpr uint32_t maxBindings=16;
  const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,4*maxBindings},
                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,maxBindings}};
  VkDescriptorPoolCreateInfo descriptors{};
  descriptors.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptors.maxSets=maxBindings;
  descriptors.poolSizeCount=2;
  descriptors.pPoolSizes=sizes;
  r=api_->CreateDescriptorPool(context.device,&descriptors,nullptr,&descriptors_);
  if (r!=VK_SUCCESS) return r;
  VkCommandPoolCreateInfo pool{};
  pool.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool.queueFamilyIndex=context.queueFamily;
  r=api_->CreateCommandPool(context.device,&pool,nullptr,&pool_);
  if (r!=VK_SUCCESS) return r;
  VkCommandBufferAllocateInfo command{};
  command.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command.commandPool=pool_;
  command.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command.commandBufferCount=1;
  r=api_->AllocateCommandBuffers(context.device,&command,&command_);
  if (r!=VK_SUCCESS) return r;
  VkFenceCreateInfo fence{};
  fence.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  r=api_->CreateFence(context.device,&fence,nullptr,&fence_);
  opened_=r==VK_SUCCESS;
  return r;
}
#undef SAND_DEVICE

VkResult VulkanOwner::Begin(const Upload *uploads,size_t count) {
  if (!opened_ || !OnOwner() || inFlight_ || (count && !uploads) || generation_==UINT64_MAX)
    return VK_ERROR_INITIALIZATION_FAILED;
  for (size_t i=0;i<count;++i)
    if (!uploads[i].words || !uploads[i].destination.bytes ||
        !SliceFits(uploads[i].destination,uploads[i].destination.bytes) || uploads[i].destination.bytes%4)
      return VK_ERROR_INITIALIZATION_FAILED;
  auto r=api_->ResetDescriptorPool(context_.device,descriptors_,0);
  if (r!=VK_SUCCESS) return r;
  r=api_->ResetCommandPool(context_.device,pool_,0);
  if (r!=VK_SUCCESS) return r;
  r=api_->ResetFences(context_.device,1,&fence_);
  if (r!=VK_SUCCESS) return r;
  VkCommandBufferBeginInfo begin{};
  begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  r=api_->BeginCommandBuffer(command_,&begin);
  if (r!=VK_SUCCESS) return r;
  ++generation_;
  for (size_t i=0;i<count;++i) {
    const auto &upload=uploads[i];
    for (VkDeviceSize off=0;off<upload.destination.bytes;) {
      const auto bytes=std::min<VkDeviceSize>(65536,upload.destination.bytes-off);
      api_->CmdUpdateBuffer(command_,upload.destination.buffer,upload.destination.offset+off,bytes,upload.words+off/4);
      off+=bytes;
    }
  }
  VkMemoryBarrier sync{};
  sync.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  sync.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
  sync.dstAccessMask=VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|
                     VK_ACCESS_TRANSFER_READ_BIT;
  api_->CmdPipelineBarrier(command_,VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,0,
                            1,&sync,0,nullptr,0,nullptr);
  return VK_SUCCESS;
}

VkResult VulkanOwner::Submit() {
  const auto ended=api_->EndCommandBuffer(command_);
  if (ended!=VK_SUCCESS) return ended;
  VkSubmitInfo submit{};
  submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&command_;
  const auto result=api_->QueueSubmit(context_.queue,1,&submit,fence_);
  inFlight_=result==VK_SUCCESS;
  if (result==VK_ERROR_DEVICE_LOST) eligible_=false;
  return result;
}

VkResult VulkanOwner::Retire(uint64_t timeoutNs) {
  if (!opened_ || !OnOwner()) return VK_ERROR_INITIALIZATION_FAILED;
  if (!inFlight_) return VK_NOT_READY;
  const auto result=api_->WaitForFences(context_.device,1,&fence_,VK_TRUE,timeoutNs);
  if (result==VK_SUCCESS || result==VK_ERROR_DEVICE_LOST) inFlight_=false;
  if (result==VK_ERROR_DEVICE_LOST) eligible_=false;
  return result;
}

VkResult VulkanOwner::MapReady(const Readback &memory,VkDeviceSize bytes) {
  return AwaitReadback(api_->WaitForFences,api_->InvalidateMappedMemoryRanges,context_.device,
      fence_,0,memory.coherent,memory.memory,memory.allocationBytes,context_.limits.nonCoherentAtomSize,
      memory.mappedOffset,bytes);
}

bool VulkanOwner::Dispatch(const Pass &p,const Command &c,const StorageSlots &storage,
                           const Slice &common,VkDescriptorSet set) {
  const auto pipeline=std::find_if(pipelines_.begin(),pipelines_.end(),[&](const auto &r) {
    return r.opcode==p.opcode;
  });
  if (pipeline==pipelines_.end()) return false;
  if (!set) {
    VkDescriptorSetAllocateInfo alloc{};
    alloc.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool=descriptors_;
    alloc.descriptorSetCount=1;
    alloc.pSetLayouts=&bindings_;
    if (api_->AllocateDescriptorSets(context_.device,&alloc,&set)!=VK_SUCCESS ||
        !WriteDescriptors(api_->UpdateDescriptorSets,context_.device,set,c,storage,common,context_.limits,true))
      return false;
  }
  return Record({api_->CmdBindPipeline,api_->CmdBindDescriptorSets,api_->CmdPushConstants,api_->CmdDispatch,
                 api_->CmdPipelineBarrier},command_,p,c,{pipeline->object,layout_,set,p.opcode,c.storageSlots});
}

VulkanOwner::BootReceipt VulkanOwner::Qualify(const StorageSlots &storage,const Slice &common,
    const Readback &readback,const Upload *uploads,size_t count,const std::function<bool()> &current,
    uint64_t timeoutNs) {
  BootReceipt result;
  if (!opened_ || !OnOwner() || inFlight_ || prepared_) return result;
  eligible_=false;
  visualSeed_={};
  if (!current || !current() || !timeoutNs || !SliceFits(storage[5],workBytes) ||
      !SliceFits(readback.buffer,bootReadbackBytes) || !readback.words) return result;
  result.result=Begin(uploads,count);
  if (result.result!=VK_SUCCESS) return result;
#ifndef PC_SAND_G11_R1
  acceptedNonce_=0;
#endif
  auto rootInit=Plan(Root::Quantum,0);
#ifdef PC_SAND_G11_R1
  if (rootInit) rootInit->storageSlots={0,0,5,6};
#endif
  if (!rootInit || !Dispatch(kQuantum[0],*rootInit,storage,common,VK_NULL_HANDLE)) {
    result.result=VK_ERROR_INITIALIZATION_FAILED;
    return result;
  }
  Pass cold{coldOpcode,0,0,-1,0,0,true};
  auto setup=*rootInit;
  setup.push[0]=coldOpcode;
  if (!Dispatch(cold,setup,storage,common,VK_NULL_HANDLE)) {
    result.result=VK_ERROR_INITIALIZATION_FAILED;
    return result;
  }
  const auto spans=[&] {
    const auto span=[&](VkDeviceSize offset,VkDeviceSize bytes,VkDeviceSize destination) {
      auto from=storage[5]; from.offset+=offset; from.bytes=bytes;
      return CopySpan{from,destination};
    };
    return std::array{
      span(controlsOffset,auditBytes,0)
#ifdef PC_SAND_G11_R1
      ,span(visual.visual[g11vis::kSegControl],512,auditBytes)
      ,span(visual.visual[g11vis::kSegMarkerCurrent],visual.v*128,auditBytes+512)
      ,span(visual.visual[g11vis::kSegControl]+g11vis::kControlPacketStyle.byteStart,8192,auditBytes+512+visual.v*128)
      ,span(visual.visual[g11vis::kSegControl]+g11vis::kControlVisualConfig.byteStart,256,bootReadbackBytes-256)
#endif
    };
  }();
  if (!RecordReadback(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,readback.buffer,spans.data(),spans.size()) ||
      !MakeHostVisible(api_->CmdPipelineBarrier,command_,readback.buffer) || !current()) {
    result.result=VK_ERROR_INITIALIZATION_FAILED;
    return result;
  }
  if ((result.result=Submit())!=VK_SUCCESS ||
      (result.result=Retire(timeoutNs))!=VK_SUCCESS || !current()) return result;
  result.result=MapReady(readback,bootReadbackBytes);
  if (result.result!=VK_SUCCESS) return result;
  result.error=readback.words[0];
  result.seal=readback.words[auditBytes/4-1];
  eligible_=result.eligible=result.error==0 && result.seal==goodSeal;
#ifdef PC_SAND_G11_R1
  const auto *header=readback.words+auditBytes/4;
  eligible_=result.eligible=eligible_ && header[0]==1110520135u && header[1]==2 &&
      header[2]==g11vis::kCompositeR1Code && header[4]==192 && header[5]==2048 &&
      header[40]==1 && !header[42] && header[127]==3235778819u;
  if (eligible_) {
    p011gpu::Sha256 digest;
    digest.update(reinterpret_cast<const uint8_t*>(header),seedBytes);
    result.visualSeedSHA=digest.finish();
    for (size_t i=0;i<result.visualSeedSHA.size();++i)
      visualSeed_[i/4]|=uint32_t(result.visualSeedSHA[i])<<(8*(i%4));
  }
#endif
  if (eligible_) {
    qualifiedWork_=storage[5].generation;
    qualifiedBuffer_=storage[5].buffer;
  }
  return result;
}

VulkanOwner::RootReceipt VulkanOwner::Run(Root root,const StorageSlots &storage,const Slice &common,
    const Readback &readback,const Upload *uploads,size_t count,const std::function<bool()> &current,
    uint64_t deadline,const std::function<uint64_t()> &now,const Capture *capture) {
  RootReceipt result;
  if (!opened_ || !OnOwner() || !eligible_ || !current || !now || !current() || now()>=deadline ||
      storage[5].buffer!=qualifiedBuffer_ || storage[5].generation!=qualifiedWork_ ||
      !SliceFits(storage[5],workBytes) || !SliceFits(readback.buffer,1024) || !readback.words) return result;
  if (prepared_ && (prepared_->generation || !prepared_->expected ||
      prepared_->expected->Word(3)!=(root==Root::Source ? 1u : 2u) ||
      prepared_->expected->U64(28)!=deadline)) return result;
#ifdef PC_SAND_G11_R1
  std::optional<WitnessPlan> witness;
  if (capture) {
    if (!uploads || !count) return result;
    const auto *uniform=std::find_if(uploads,uploads+count,[&](const auto &u) {
      return u.destination.buffer==common.buffer && u.destination.generation==common.generation &&
             u.destination.offset==common.offset && u.destination.bytes==kCommonBytes && u.words;
    });
    if (uniform==uploads+count) return result;
    const auto bankOffset=capture->selection.role==4 ? k_uVisHeaderReplay : k_uVisHeaderActual;
    witness=PlanR1Witness(root,capture->selection,storage[5],uniform->words[bankOffset/4]);
    if (!witness || !SliceFits(capture->staging.buffer,kR1WitnessBytes) || !capture->staging.words) return result;
  }
#else
  if (capture) {
    if (root!=Root::Quantum || !capture->discovery) return result;
    for (size_t i=0;i<6;++i)
      if (!SliceFits(capture->discovery[i],arenaBytes)) return result;
  }
#endif
  result.result=Begin(uploads,count);
  if (result.result!=VK_SUCCESS) return result;
  result.generation=generation_;
  result.fence=fence_;
  if (prepared_) prepared_->generation=generation_;
#ifndef PC_SAND_G11_R1
  acceptedNonce_=0;
#endif
  const bool source=root==Root::Source;
  const auto rows=source ? kSource : kQuantum;
  const auto n=source ? std::size(kSource) : std::size(kQuantum);
  std::array<std::pair<std::array<uint8_t,4>,VkDescriptorSet>,16> cache{};
  size_t used=0;
  for (size_t i=0;i<n;++i) {
    const auto planned=Plan(root,i);
    if (!planned || !RecordParentCopies(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,*planned,
                                        storage,stateBytes,arenaBytes)) {
      result.result=VK_ERROR_INITIALIZATION_FAILED;
      return result;
    }
    auto found=std::find_if(cache.begin(),cache.begin()+used,[&](const auto &entry) {
      return entry.first==planned->storageSlots;
    });
    if (found==cache.begin()+used) {
      if (used==cache.size()) { result.result=VK_ERROR_INITIALIZATION_FAILED; return result; }
      VkDescriptorSetAllocateInfo alloc{};
      alloc.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      alloc.descriptorPool=descriptors_;
      alloc.descriptorSetCount=1;
      alloc.pSetLayouts=&bindings_;
      auto &entry=cache[used++];
      entry.first=planned->storageSlots;
      result.result=api_->AllocateDescriptorSets(context_.device,&alloc,&entry.second);
      if (result.result!=VK_SUCCESS ||
          !WriteDescriptors(api_->UpdateDescriptorSets,context_.device,entry.second,*planned,storage,common,context_.limits)) {
        if (result.result==VK_SUCCESS) result.result=VK_ERROR_INITIALIZATION_FAILED;
        return result;
      }
    }
    if (!Dispatch(rows[i],*planned,storage,common,found->second)) {
      result.result=VK_ERROR_INITIALIZATION_FAILED;
      return result;
    }
    ++result.dispatches;
    if (capture) {
#ifdef PC_SAND_G11_R1
      const bool begin=i==witness->beginOrdinal,apply=i==witness->applyOrdinal;
      if (begin || apply) {
        const auto *spans=begin ? &witness->begin : witness->apply.data();
        const size_t countSpans=begin ? 1 : witness->apply.size();
        if (!RecordReadback(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,capture->staging.buffer,spans,countSpans)) {
          result.result=VK_ERROR_INITIALIZATION_FAILED;
          return result;
        }
        result.captured+=uint32_t(countSpans);
      }
#else
      const auto &p=rows[i];
      if (p.role==1 && p.opcode==35) {
        size_t slot=0;
        for (auto j=p.j;j>1;j>>=1) ++slot;
        if (slot>=6 || !SliceFits(storage[6],arenaBytes)) {
          result.result=VK_ERROR_INITIALIZATION_FAILED;
          return result;
        }
        auto sourceSpan=storage[6];
        sourceSpan.bytes=arenaBytes;
        const CopySpan snapshot{sourceSpan,0};
        if (!RecordReadback(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,capture->discovery[slot],&snapshot,1)) {
          result.result=VK_ERROR_INITIALIZATION_FAILED;
          return result;
        }
        ++result.captured;
      }
#endif
      VkMemoryBarrier copyRetired{};
      copyRetired.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
      copyRetired.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
      copyRetired.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
      if (result.captured && (
#ifdef PC_SAND_G11_R1
          begin || apply
#else
          p.role==1 && p.opcode==35
#endif
          ))
        api_->CmdPipelineBarrier(command_,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  0,1,&copyRetired,0,nullptr,0,nullptr);
    }
  }
  auto tail=storage[5];
  tail.offset+=tailOffset;
  tail.bytes=1024;
  const CopySpan copy{tail,0};
  if (!RecordReadback(api_->CmdPipelineBarrier,api_->CmdCopyBuffer,command_,readback.buffer,&copy,1) ||
      !MakeHostVisible(api_->CmdPipelineBarrier,command_,readback.buffer)) {
    result.result=VK_ERROR_INITIALIZATION_FAILED;
    return result;
  }
#ifdef PC_SAND_G11_R1
  if (capture && (result.captured!=6 ||
                  !MakeHostVisible(api_->CmdPipelineBarrier,command_,capture->staging.buffer))) {
    result.result=VK_ERROR_INITIALIZATION_FAILED;
    return result;
  }
#endif
  if (!current() || now()>=deadline) { result.result=VK_NOT_READY; return result; }
  result.result=Submit();
  if (result.result!=VK_SUCCESS) return result;
  uint64_t fenceNs=0;
  for (;;) {
    const auto at=now();
    if (!current() || at>=deadline) {
      result.result=at>=deadline ? VK_TIMEOUT : VK_NOT_READY;
      return result;
    }
    result.result=Retire(std::min(deadline-at,pollNs));
    if (result.result!=VK_TIMEOUT) { fenceNs=now(); break; }
  }
  if (result.result!=VK_SUCCESS || !current() || fenceNs>=deadline) return result;
  result.result=MapReady(readback,1024);
  if (result.result==VK_SUCCESS && current() && now()<deadline) {
    std::copy_n(readback.words,result.tail.size(),result.tail.begin());
    result.read=true;
    if (prepared_) {
      auto &job=*prepared_;
      job.tail=result.tail;
      job.storage=storage;
#ifndef PC_SAND_G11_R1
      if (capture && capture->discovery) std::copy_n(capture->discovery,job.discovery.size(),job.discovery.begin());
#else
      if (capture) job.witness=*capture;
#endif
      auto &e=job.evidence;
      e.dispatches=result.dispatches;
      e.barriers=result.dispatches;
      e.fenceNs=fenceNs;
      e.observedNs=now();
      e.currentContext=reinterpret_cast<uintptr_t>(context_.device);
      const auto seal=std::find_if(pipelines_.begin(),pipelines_.end(),
                                   [&](const auto &r) { return r.opcode==rows[n-1].opcode; });
      e.actualProgram=reinterpret_cast<uintptr_t>(seal->object);
      e.owner=thread_;
      e.fenceSignaled=e.mapped=e.current=e.liveGuard=true;
    }
  }
  return result;
}
}
