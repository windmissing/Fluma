#include "g16_multi.h"
#include "host_device.h"
#include "../wsi.h"
#include "../../../entry/src/main/cpp/gpu_resident/program_cache/sha256.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace grs::pc::sand {
namespace {
constexpr std::array<VkDeviceSize,4> storageBytes{196608,1179648,608256,65536};
constexpr VkDeviceSize configOffset=346112,receiptOffset=350208,witnessOffset=559104;
constexpr VkDeviceSize stateStride=65536,sceneStride=32768,receiptBytes=1024,witnessBytes=256;
constexpr VkDeviceSize journalOffset=509952,journalRowBytes=160,journalCapacity=300;
constexpr uint64_t budget=250000000;
using DigestBytes=std::array<uint8_t,32>;
using Completion=std::array<uint32_t,256>;
using Config=std::array<uint32_t,256>;

DigestBytes Hash(const void *data,size_t bytes) {
  p011gpu::Sha256 hash;
  hash.update(static_cast<const uint8_t *>(data),bytes);
  return hash.finish();
}
constexpr DigestBytes ParseDigest(const char (&hex)[65]) {
  const auto digit=[](char c){return c<='9'?c-'0':10+c-'a';};
  DigestBytes digest{};
  for(size_t i=0;i<digest.size();++i)digest[i]=uint8_t(16*digit(hex[2*i])+digit(hex[2*i+1]));
  return digest;
}
std::string Hex(const DigestBytes &digest) {
  std::string out;
  constexpr char alphabet[]="0123456789abcdef";
  for(auto octet:digest) {out+=alphabet[octet>>4];out+=alphabet[octet&15u];}
  return out;
}
std::vector<uint32_t> ReadWords(const wchar_t *name,size_t maxBytes) {
  std::ifstream input(std::filesystem::path(name),std::ios::binary|std::ios::ate);
  const auto count=input.tellg();
  RequireHost(count>0 && uint64_t(count)<=maxBytes && count%4==0,"G16_INPUT_RANGE");
  std::vector<uint32_t> words(size_t(count)/4);
  input.seekg(0);
  RequireHost(bool(input.read(reinterpret_cast<char *>(words.data()),count)),"G16_INPUT_READ");
  return words;
}
template<class Words> uint64_t Wide(const Words &words,size_t at) {
  return uint64_t(words[at+1])<<32|words[at];
}
void PutWide(Config &config,size_t at,uint64_t value) {
  config[at]=uint32_t(value);
  config[at+1]=uint32_t(value>>32);
}
float AsFloat(uint32_t bits) {
  float value;
  std::memcpy(&value,&bits,4);
  return value;
}
uint32_t AsWord(float value) {
  uint32_t word;
  std::memcpy(&word,&value,4);
  return word;
}
void Write(const std::filesystem::path &path,const void *data,size_t bytes) {
  std::ofstream stream(path,std::ios::binary);
  RequireHost(bool(stream.write(static_cast<const char *>(data),bytes)),"G16_OUTPUT_WRITE");
}

enum class Motion { Static,Fast,Slow,Hold };
#ifdef PC_G16_FREEZE_FIXTURE
struct FreezeFault {
  uint32_t disk,word,value;
  VkDeviceSize Offset(uint32_t bank) const {return bank*stateStride+(disk-1)*64u+word*4u;}
};
#endif
struct Program {
  std::vector<uint32_t> module,seed;
  std::filesystem::path out;
  Config initial{};
  uint32_t j=128,roots=0,disks=0;
  bool mirror=false,present=false;
  Motion motion=Motion::Static;
#ifdef PC_G16_FREEZE_FIXTURE
  std::optional<FreezeFault> fault;
#endif

  void BuildPose() {
    const auto add=[&](uint32_t ms,int left,int right) {
      const uint32_t k=initial[6]++;
      const uint32_t a=128+k*16;
      const uint64_t start=k ? Wide(initial,a-16)+Wide(initial,a-14) : 0;
      PutWide(initial,a,start);
      PutWide(initial,a+2,uint64_t(ms)*1000000);
      initial[a+4]=uint32_t((mirror?-left:left)*1000);
      initial[a+5]=uint32_t((mirror?-right:right)*1000);
      initial[a+6]=1;
    };
    if(motion==Motion::Static) add(10000,0,0);
    else if(motion==Motion::Hold) {
      add(1000,0,80);add(500,80,90);add(500,90,90);add(500,90,100);add(1000,100,180);add(1000,180,180);
    } else {
      const uint32_t duration=motion==Motion::Slow?2000:1000;
      add(duration,0,80);add(duration,80,100);add(duration,100,180);add(1000,180,180);
    }
  }

  void Freeze(uint64_t run) {
    initial[0]=0x49363147;initial[1]=1;initial[2]=128;initial[3]=1;
    initial[4]=disks;initial[5]=j;initial[44]=disks==13?10:20;initial[45]=2*initial[44];
    initial[46]=disks;initial[47]=1;initial[48]=AsWord(.05f);initial[49]=AsWord(.1f);
    initial[51]=AsWord(-9.81f);initial[52]=AsWord(.00005f);initial[53]=AsWord(.00025f);
    initial[61]=AsWord(disks==13?.01f:.005f);
    PutWide(initial,8,run);PutWide(initial,10,1);PutWide(initial,12,1);PutWide(initial,14,1);
    BuildPose();
    const auto named=[](std::string_view name) {return Hash(name.data(),name.size());};
    const auto pose=Hash(initial.data()+128,initial[6]*64);
    constexpr auto layout=ParseDigest("87112e0fab8f06a6edc57b9e26a71017a33411c59fbff62dc88aa3571255ee69");
    const auto digests=std::array{Hash(seed.data(),seed.size()*4),pose,named("G16_DRY_FRICTIONLESS_REFERENCE_V1"),
        layout,Hash(module.data(),module.size()*4)};
    for(size_t i=0;i<digests.size();++i)std::memcpy(initial.data()+64+i*8,digests[i].data(),32);
    PutWide(initial,20,(uint64_t(initial[73])<<32|initial[72])|1);
  }
};

struct Head {
  uint32_t bank=0,scene=0;
  uint64_t commit=0,stateGeneration=1,sceneGeneration=0;
};

bool CheckIdentity(const Config &c,const Completion &r,uint32_t outputOutcome) {
  const auto same=[&](size_t dest,size_t src,size_t words) {
    return std::equal(r.begin()+dest,r.begin()+dest+words,c.begin()+src);
  };
  return r[0]==0x36314743 && r[1]==1 && r[2]==256 && r[3]==0xC016CAFE &&
      same(4,8,2) && same(6,16,4) && same(10,10,6) && same(16,20,4) &&
      same(22,24,2) && same(26,26,14) && same(48,40,4) && same(52,4,1) &&
      same(57,5,1) && same(88,64,40) && r[40]==outputOutcome &&
      r[53]==c[44]*c[45] && r[54]<=8192 && r[55]<=16 && r[56]<=16384 &&
      r[58]<=2*c[5] && r[59]<=16*r[58] && r[60]<=22560 && r[61]==c[5]+2 &&
      r[64]<=r[54] && r[64]<=c[4]*(c[4]+7)/2 &&
      (r[63]&0x801)==0x801 && (r[63]&~0xfffu)==0 && (r[65]&~3u)==0 &&
      std::all_of(r.begin()+128,r.end(),[](uint32_t x){return x==0;});
}
bool CanCommit(const Config &c,const Completion &r) {
  if(!CheckIdentity(c,r,1) || r[41]!=0 || r[42]!=0 || r[62]!=c[4] ||
     Wide(r,20)!=Wide(c,22)+1 || Wide(r,24)!=Wide(c,24)+1 ||
     Wide(r,66)!=130000000 || Wide(r,68)!=Wide(r,66) ||
     !std::all_of(r.begin()+43,r.begin()+48,[](uint32_t x){return x==UINT32_MAX;}))return false;
  for(size_t w=70;w<=85;++w)if(!std::isfinite(AsFloat(r[w])))return false;
  const float momentum=.001f*.13f*std::sqrt(9.81f*.2f);
  return AsFloat(r[71])>=0 && AsFloat(r[71])<=.00025f && AsFloat(r[72])>=0 && AsFloat(r[72])<=.001f &&
      std::hypot(AsFloat(r[75]),AsFloat(r[76]))<=momentum &&
      std::abs(AsFloat(r[77]))<=.2f*momentum && std::abs(AsFloat(r[78]))<=.001f*.13f*9.81f*.2f &&
      AsFloat(r[80])>=0 && (r[63]&0xe1f)==0xe1f;
}

class Owner {
 public:
  explicit Owner(HostDevice &gpu): gpu_(gpu) {}
  Owner(const Owner &)=delete;
  ~Owner() {
    if(pending_) {
      VkResult idle;
      do {
        idle=Proc<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(gpu_.device);
        if(idle!=VK_SUCCESS && idle!=VK_ERROR_DEVICE_LOST)Sleep(50);
      } while(idle!=VK_SUCCESS && idle!=VK_ERROR_DEVICE_LOST);
      const VkResult retired=idle==VK_SUCCESS?
          Proc<PFN_vkWaitForFences>("vkWaitForFences")(gpu_.device,1,&fence_,VK_TRUE,0):idle;
      std::cout<<"{\"stage\":\"G16_PRIVATE_DRAIN\",\"fenceGeneration\":\""<<fenceGeneration_
               <<"\",\"fenceVkResult\":"<<int(retired)<<",\"acceptedRoot\":null}\n";
    }
#define DESTROY(name,handle) if(handle)Proc<PFN_vkDestroy##name>("vkDestroy" #name)(gpu_.device,handle,nullptr)
    DESTROY(Fence,fence_);DESTROY(CommandPool,commands_);DESTROY(Pipeline,pipeline_);DESTROY(ShaderModule,shader_);
    DESTROY(PipelineLayout,layout_);DESTROY(DescriptorPool,sets_);DESTROY(DescriptorSetLayout,bindings_);
#undef DESTROY
  }

  template<class Fn> Fn Proc(const char *name) {
    const auto f=reinterpret_cast<Fn>(gpu_.deviceProc(gpu_.device,name));
    RequireHost(f!=nullptr,name);
    return f;
  }

  void Prepare(const Program &p);
  bool Run(const Program &p);

 private:
  VkResult Submit(uint64_t until,const HostDevice::Memory &destination,VkDeviceSize bytes,
                  VkResult &submit,VkResult &retire);
  void Export(const Program &,const Config &,const Completion &,bool accepted);
  bool Present(const Program &,const Config &,const uint32_t *pose);
  HostDevice &gpu_;
  std::array<HostDevice::Memory,4> resources_{};
  HostDevice::Memory readback_{};
  VkDescriptorSetLayout bindings_=VK_NULL_HANDLE;
  VkDescriptorPool sets_=VK_NULL_HANDLE;
  VkDescriptorSet set_=VK_NULL_HANDLE;
  VkPipelineLayout layout_=VK_NULL_HANDLE;
  VkShaderModule shader_=VK_NULL_HANDLE;
  VkPipeline pipeline_=VK_NULL_HANDLE;
  VkCommandPool commands_=VK_NULL_HANDLE;
  VkCommandBuffer command_=VK_NULL_HANDLE;
  VkFence fence_=VK_NULL_HANDLE;
  bool pending_=false;
  uint64_t fenceGeneration_=0,previousNonce_=0;
  Head head_{};
  bool scenePinned_=false;
#ifdef PC_G16_FREEZE_FIXTURE
  HostDevice::Memory fixtureState_{};
  bool fixtureBound_=false;
  void RecordFreezeFault(const FreezeFault &,uint32_t bank);
#endif
};

#define VKFN(name) Proc<PFN_vk##name>("vk" #name)
void Owner::Prepare(const Program &p) {
  const auto &limits=gpu_.properties.limits;
  RequireHost(limits.maxPerStageDescriptorStorageBuffers>=4 && limits.maxDescriptorSetStorageBuffers>=4 &&
      limits.maxStorageBufferRange>=storageBytes[1] && limits.maxPushConstantsSize>=12 &&
      limits.minStorageBufferOffsetAlignment!=0,"G16_DEVICE_LIMIT");
  std::array<VkDescriptorSetLayoutBinding,4> layoutBindings{};
  std::array<VkDescriptorBufferInfo,4> ranges{};
  std::array<VkWriteDescriptorSet,4> writes{};
  const std::array<VkBufferUsageFlags,4> usage{
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,0,VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
  for(uint32_t i=0;i<4;++i) {
    resources_[i]=gpu_.Allocate(storageBytes[i],usage[i]|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    layoutBindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
    const auto &allocation=resources_[i];
    ranges[i]={allocation.slice.buffer,0,storageBytes[i]};
    std::cout<<"{\"stage\":\"G16_BIND\",\"binding\":"<<i<<",\"buffer\":\""
        <<reinterpret_cast<uintptr_t>(allocation.slice.buffer)<<"\",\"offset\":0,\"range\":"<<storageBytes[i]
        <<",\"generation\":\""<<allocation.slice.generation<<"\",\"memorySize\":"<<allocation.allocation
        <<",\"memoryAlignment\":"<<allocation.alignment<<",\"memoryType\":"<<allocation.memoryType
        <<",\"flags\":"<<layoutBindings[i].stageFlags<<"}\n";
  }
  std::cout<<"{\"stage\":\"G16_DEVICE\",\"vendor\":"<<gpu_.properties.vendorID<<",\"device\":"<<gpu_.properties.deviceID
    <<",\"driver\":"<<gpu_.properties.driverVersion<<",\"family\":"<<gpu_.family
    <<",\"maxStorageRange\":"<<limits.maxStorageBufferRange<<",\"maxStoragePerStage\":"<<limits.maxPerStageDescriptorStorageBuffers
    <<",\"maxStoragePerSet\":"<<limits.maxDescriptorSetStorageBuffers<<",\"storageOffsetAlign\":"
    <<limits.minStorageBufferOffsetAlignment<<"}\n";
  readback_=gpu_.Allocate(receiptBytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto device=gpu_.device;
  const VkDescriptorSetLayoutCreateInfo desc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,nullptr,0,4,layoutBindings.data()};
  RequireVk(VKFN(CreateDescriptorSetLayout)(device,&desc,nullptr,&bindings_),"G16_LAYOUT");
  const VkDescriptorPoolSize capacity{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,4};
  const VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr,0,1,1,&capacity};
  RequireVk(VKFN(CreateDescriptorPool)(device,&pool,nullptr,&sets_),"G16_DESCRIPTOR_POOL");
  const VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,nullptr,sets_,1,&bindings_};
  RequireVk(VKFN(AllocateDescriptorSets)(device,&alloc,&set_),"G16_DESCRIPTOR_SET");
  for(uint32_t i=0;i<4;++i)
    writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,set_,i,0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,nullptr,&ranges[i],nullptr};
  VKFN(UpdateDescriptorSets)(device,4,writes.data(),0,nullptr);
  const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,12};
  const VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,nullptr,0,1,&bindings_,1,&push};
  RequireVk(VKFN(CreatePipelineLayout)(device,&pl,nullptr,&layout_),"G16_PIPELINE_LAYOUT");
  const VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,nullptr,0,p.module.size()*4,p.module.data()};
  RequireVk(VKFN(CreateShaderModule)(device,&module,nullptr,&shader_),"G16_SHADER_MODULE");
  VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  compute.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader_,"main",nullptr};
  compute.layout=layout_;
  RequireVk(VKFN(CreateComputePipelines)(device,VK_NULL_HANDLE,1,&compute,nullptr,&pipeline_),"G16_PIPELINE");
  const VkCommandPoolCreateInfo cmdPool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,nullptr,0,gpu_.family};
  RequireVk(VKFN(CreateCommandPool)(device,&cmdPool,nullptr,&commands_),"G16_COMMAND_POOL");
  const VkCommandBufferAllocateInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,nullptr,commands_,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
  RequireVk(VKFN(AllocateCommandBuffers)(device,&cmdInfo,&command_),"G16_COMMAND");
  const VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  RequireVk(VKFN(CreateFence)(device,&fenceInfo,nullptr,&fence_),"G16_FENCE");
#ifdef PC_G16_FREEZE_FIXTURE
  if(p.fault)
    fixtureState_=gpu_.Allocate(storageBytes[0],VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT);
#endif
}

#ifdef PC_G16_FREEZE_FIXTURE
void Owner::RecordFreezeFault(const FreezeFault &fault,uint32_t bank) {
  const auto &base=resources_[0].slice;
  const auto &fixture=fixtureState_.slice;
  // BASE comes from the first transfer upload or an already retired compute root.
  const VkMemoryBarrier input{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,
    VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT};
  VKFN(CmdPipelineBarrier)(command_,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&input,0,nullptr,0,nullptr);
  const VkBufferCopy copy{0,0,base.bytes};
  VKFN(CmdCopyBuffer)(command_,base.buffer,fixture.buffer,1,&copy);
  const VkMemoryBarrier copied{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,
    VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
  VKFN(CmdPipelineBarrier)(command_,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,1,&copied,0,nullptr,0,nullptr);
  VKFN(CmdUpdateBuffer)(command_,fixture.buffer,fault.Offset(bank),sizeof(fault.value),&fault.value);
  const VkDescriptorBufferInfo range{fixture.buffer,0,fixture.bytes};
  const VkWriteDescriptorSet bind{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,set_,0,0,1,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,nullptr,&range,nullptr};
  VKFN(UpdateDescriptorSets)(gpu_.device,1,&bind,0,nullptr);
  fixtureBound_=true;
  std::cout<<"{\"stage\":\"G16_FREEZE_FIXTURE_INPUT\",\"bank\":"<<bank<<",\"disk\":"<<fault.disk
    <<",\"word\":"<<fault.word<<",\"value\":"<<fault.value<<",\"offset\":"<<fault.Offset(bank)
    <<",\"trustedBuffer\":\""<<reinterpret_cast<uintptr_t>(base.buffer)<<"\",\"fixtureBuffer\":\""
    <<reinterpret_cast<uintptr_t>(fixture.buffer)<<"\",\"bindingOffset\":0,\"bindingRange\":"<<fixture.bytes
    <<",\"bindingGeneration\":\""<<fixture.generation<<"\",\"acceptedRoot\":null}\n";
}
#endif

VkResult Owner::Submit(uint64_t until,const HostDevice::Memory &destination,VkDeviceSize bytes,
                       VkResult &submit,VkResult &retire) {
  const auto now=HostDevice::Now();
  if(now>=until) return VK_TIMEOUT;
  const auto device=gpu_.device;
  VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  info.commandBufferCount=1;info.pCommandBuffers=&command_;
  submit=VKFN(QueueSubmit)(gpu_.queue,1,&info,fence_);
  if(submit!=VK_SUCCESS)return submit;
  pending_=true;
  const auto entered=HostDevice::Now();
  retire=VKFN(WaitForFences)(device,1,&fence_,VK_TRUE,entered<until?until-entered:0);
  pending_=retire!=VK_SUCCESS && retire!=VK_ERROR_DEVICE_LOST;
  if(retire!=VK_SUCCESS)return retire;
  if(HostDevice::Now()>until)return VK_TIMEOUT;
  const auto invalidated=AwaitReadback(VKFN(WaitForFences),VKFN(InvalidateMappedMemoryRanges),device,fence_,0,
    destination.coherent,destination.memory,destination.allocation,gpu_.properties.limits.nonCoherentAtomSize,0,bytes);
  return invalidated==VK_SUCCESS && HostDevice::Now()>until?VK_TIMEOUT:invalidated;
}

void Owner::Export(const Program &p,const Config &config,const Completion &r,bool accepted) {
  // A separate, read-only acquisition after the exact business fence retired.
  // Copy only the valid edge prefix; all three fixed planes keep their offsets.
  constexpr VkDeviceSize maxEdgePlane=393216;
  const VkDeviceSize stateBytes=p.disks*64u,sceneBytes=p.disks*32u,edgeBytes=r[64]*48u;
  const VkDeviceSize noteBytes=witnessBytes+r[64]*8u,headBytes=p.disks*8u,adjBytes=r[56]*16u;
  const VkDeviceSize arrays=2*stateBytes+sceneBytes+noteBytes;
  const VkDeviceSize journalAt=arrays+3*edgeBytes+headBytes+adjBytes;
  const VkDeviceSize ordinaryBytes=journalAt+journalRowBytes*journalCapacity;
  VkDeviceSize bytes=ordinaryBytes;
#ifdef PC_G16_FREEZE_FIXTURE
  if(fixtureBound_)bytes+=3*stateStride;
#endif
  const auto capture=gpu_.Allocate(bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  const auto device=gpu_.device;
  RequireHost(!pending_,"G16_COLD_IN_FLIGHT");
  RequireVk(VKFN(ResetCommandPool)(device,commands_,0),"G16_COLD_RESET");
  RequireVk(VKFN(ResetFences)(device,1,&fence_),"G16_COLD_FENCE_RESET");
  ++fenceGeneration_;
  const VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  RequireVk(VKFN(BeginCommandBuffer)(command_,&begin),"G16_COLD_BEGIN");
  const auto span=[&](uint32_t binding,VkDeviceSize source,VkDeviceSize size,VkDeviceSize target) {
    auto slice=resources_[binding].slice;
    RequireHost(source<=slice.bytes && size<=slice.bytes-source,"G16_COLD_RANGE");
    slice.offset=source;slice.bytes=size;
    return CopySpan{slice,target};
  };
  auto base=span(0,config[40]*stateStride,stateBytes,0);
  auto candidate=span(0,config[41]*stateStride,stateBytes,stateBytes);
#ifdef PC_G16_FREEZE_FIXTURE
  if(fixtureBound_) {
    base.source.buffer=candidate.source.buffer=fixtureState_.slice.buffer;
    base.source.generation=candidate.source.generation=fixtureState_.slice.generation;
  }
#endif
  std::vector<CopySpan> ranges{
    base,candidate,
    span(2,witnessOffset,noteBytes,stateBytes*2+sceneBytes),
    span(2,354304,headBytes,arrays+3*edgeBytes),
    span(2,journalOffset,journalRowBytes*journalCapacity,journalAt)};
  if(accepted)ranges.push_back(span(3,config[43]*sceneStride,sceneBytes,stateBytes*2));
  for(uint32_t i=0;i<3 && edgeBytes;++i)
    ranges.push_back(span(1,i*maxEdgePlane,edgeBytes,arrays+i*edgeBytes));
  if(adjBytes)ranges.push_back(span(2,0,adjBytes,arrays+3*edgeBytes+headBytes));
#ifdef PC_G16_FREEZE_FIXTURE
  if(fixtureBound_) {
    ranges.push_back(span(0,config[40]*stateStride,stateStride,ordinaryBytes));
    auto faulty=base.source;
    auto privateCandidate=candidate.source;
    faulty.bytes=privateCandidate.bytes=stateStride;
    ranges.push_back({faulty,ordinaryBytes+stateStride});
    ranges.push_back({privateCandidate,ordinaryBytes+2*stateStride});
  }
#endif
  RequireHost(RecordReadback(VKFN(CmdPipelineBarrier),VKFN(CmdCopyBuffer),command_,capture.slice,ranges.data(),ranges.size()),
    "G16_COLD_RECORD");
  RequireHost(MakeHostVisible(VKFN(CmdPipelineBarrier),command_,capture.slice),"G16_COLD_HOST_BARRIER");
  RequireVk(VKFN(EndCommandBuffer)(command_),"G16_COLD_END");
  VkResult submit=VK_NOT_READY,retire=VK_NOT_READY;
  const auto result=Submit(HostDevice::Now()+budget,capture,bytes,submit,retire);
  std::cout<<"{\"stage\":\"G16_COLD\",\"nonce\":\""<<Wide(config,16)<<"\",\"fence\":\""
    <<reinterpret_cast<uintptr_t>(fence_)<<"\",\"fenceGeneration\":\""<<fenceGeneration_
    <<"\",\"submit\":"<<int(submit)<<",\"retire\":"<<int(retire)<<",\"vkResult\":"<<int(result)
    <<",\"acceptedRoot\":null}\n";
  RequireVk(result,"G16_COLD_RESULT");
  const auto *words=capture.words;
  Write(p.out/"base.disk64",words,stateBytes);
  Write(p.out/"candidate.disk64",words+stateBytes/4,stateBytes);
#ifdef PC_G16_FREEZE_FIXTURE
  if(fixtureBound_) {
    const auto *trusted=words+ordinaryBytes/4;
    const auto *faulty=trusted+stateStride/4;
    const auto *privateCandidate=faulty+stateStride/4;
    const auto changed=((p.fault->disk-1)*16u)+p.fault->word;
    RequireHost(trusted[changed]!=p.fault->value && faulty[changed]==p.fault->value &&
        std::equal(trusted,trusted+changed,faulty) &&
        std::equal(trusted+changed+1,trusted+stateStride/4,faulty+changed+1),
        "G16_FREEZE_FIXTURE_DELTA");
    Write(p.out/"trusted-base-bank.disk64",trusted,stateStride);
    Write(p.out/"fixture-base-bank.disk64",faulty,stateStride);
    Write(p.out/"fixture-candidate-bank.disk64",privateCandidate,stateStride);
    std::cout<<"{\"stage\":\"G16_FREEZE_FIXTURE_BYTES\",\"nonce\":\""<<Wide(config,16)
      <<"\",\"trustedBaseSHA\":\""<<Hex(Hash(trusted,stateStride))
      <<"\",\"faultyBaseSHA\":\""<<Hex(Hash(faulty,stateStride))
      <<"\",\"privateCandidateSHA\":\""<<Hex(Hash(privateCandidate,stateStride))<<"\",\"acceptedRoot\":null}\n";
  }
#endif
  if(accepted)Write(p.out/"scene.scene32",words+stateBytes*2/4,sceneBytes);
  const auto *notes=words+(stateBytes*2+sceneBytes)/4;
  Write(p.out/"witness.u32le",notes,noteBytes);
  RequireHost(notes[26]==journalCapacity && notes[24]<=journalCapacity && notes[25]>=notes[24] &&
      (notes[25]>notes[24])==bool(r[65]&1u),"G16_COLD_JOURNAL");
  if(notes[24])
    Write(p.out/"contact-journal.bin",words+journalAt/4,notes[24]*journalRowBytes);
  if(edgeBytes) {
    const auto *edges=words+arrays/4;
    Write(p.out/"key-brackets.bin",edges,edgeBytes);
    Write(p.out/"solver-residuals.bin",edges+edgeBytes/4,edgeBytes);
    Write(p.out/"geometry-work.bin",edges+2*edgeBytes/4,edgeBytes);
  }
  if(notes[1] && notes[1]<=r[56]) {
    const auto *heads=words+(arrays+3*edgeBytes)/4;
    Write(p.out/"csr-heads.u32le",heads,headBytes);
    Write(p.out/"adjacency.bin",heads+headBytes/4,notes[1]*16u);
  }
  Write(p.out/"final-config.u32le",config.data(),sizeof(config));
  if(p.present && accepted)RequireHost(Present(p,config,notes+16),"G16_PRESENT_FAILED");
}

bool Owner::Present(const Program &p,const Config &config,const uint32_t *pose) {
  const uint64_t inputId=uint64_t(pose[7])<<32|pose[6];
  const auto offset=head_.scene*sceneStride;
  const auto &scene=resources_[3].slice;
  if (scenePinned_ || inputId!=Wide(config,20) || head_.scene!=config[43] ||
      head_.sceneGeneration!=Wide(config,32) || !std::isfinite(AsFloat(pose[0])) ||
      !std::isfinite(AsFloat(pose[1])) || offset+p.disks*32u>scene.bytes) return false;
  const auto pixels=gpu_.Allocate(4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  g13vk::VulkanApi api{};
  api.GetInstanceProcAddr=gpu_.instanceProc;
#define LOAD_I(name) api.name=reinterpret_cast<PFN_vk##name>(gpu_.instanceProc(gpu_.instance,"vk" #name));
  T742_INSTANCE_API(LOAD_I)
#undef LOAD_I
#define LOAD_D(name) api.name=VKFN(name);
  T742_DEVICE_API(LOAD_D)
#undef LOAD_D
  const pc::G16SceneView view{offset,p.disks,{AsFloat(pose[0]),AsFloat(pose[1])}};
  pc::PresentObservations observation;
  observation.g16=&view;observation.timing=true;observation.timestampBits=gpu_.timestampBits;
  scenePinned_=true;
  const auto frame=pc::PresentBoot(api,gpu_.instance,gpu_.physical,gpu_.device,gpu_.queue,gpu_.family,gpu_.surface,
    scene.buffer,pixels.slice.buffer,pixels.memory,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|(pixels.coherent?VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:0),gpu_.window,
    [&] {return scenePinned_ && head_.scene==config[43] && head_.sceneGeneration==Wide(config,32) && gpu_.Current();},
    [&](const pc::Frame &visible) {
      std::cout<<"{\"stage\":\"G16_PRIVATE_PRESENT\",\"nonce\":\""<<Wide(config,16)<<"\",\"commit\":\""<<head_.commit
        <<"\",\"inputId\":\""<<inputId<<"\",\"geometry\":\""<<Wide(config,14)
        <<"\",\"hwnd\":\""<<reinterpret_cast<uintptr_t>(gpu_.window)
        <<"\",\"stateBufferGeneration\":\""<<resources_[0].slice.generation
        <<"\",\"stateGeneration\":\""<<head_.stateGeneration
        <<"\",\"sceneBufferGeneration\":\""<<scene.generation
        <<"\",\"sceneGeneration\":\""<<head_.sceneGeneration<<"\",\"sceneSlot\":"<<head_.scene
        <<",\"sceneOffset\":"<<offset<<",\"sceneRange\":"<<p.disks*32u<<",\"leaseSerial\":\""<<head_.sceneGeneration
        <<"\",\"instances\":"<<p.disks
        <<",\"graphicsFence\":\""<<visible.graphicsFence<<"\",\"graphicsExitQpc\":\""<<visible.graphicsFenceExit
        <<"\",\"presentExitQpc\":\""<<visible.presentExit<<"\",\"imageIndex\":"<<visible.imageIndex
        <<",\"imagePresentVkResult\":"<<int(visible.imagePresent)<<",\"acceptedRoot\":null}\n";
    },0,false,{},false,{},observation);
  scenePinned_=false;
  std::cout<<"{\"stage\":\"G16_PRIVATE_PRESENT_RETIRE\",\"nonce\":\""<<Wide(config,16)
    <<"\",\"vkResult\":"<<int(frame.error)<<",\"retireVkResult\":"<<int(frame.retire)<<",\"acceptedRoot\":null}\n";
  return frame.error==VK_SUCCESS && frame.retire==VK_SUCCESS;
}

bool Owner::Run(const Program &p) {
  std::ofstream stream(p.out/"completions.u32le",std::ios::binary);
  RequireHost(bool(stream),"G16_COMPLETION_OUTPUT");
  const auto device=gpu_.device;
  const auto writeBarrier=[&](VkPipelineStageFlags from,VkAccessFlags access) {
    const VkMemoryBarrier memory{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,access,
        VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT};
    VKFN(CmdPipelineBarrier)(command_,from,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memory,0,nullptr,0,nullptr);
  };
  for(uint32_t i=0;i<p.roots;++i) {
    RequireHost(!pending_ && !scenePinned_ && gpu_.Current(),"G16_ROOT_REVOKED");
    RequireVk(VKFN(ResetCommandPool)(device,commands_,0),"G16_COMMAND_RESET");
    RequireVk(VKFN(ResetFences)(device,1,&fence_),"G16_FENCE_RESET");
    Config config=p.initial;
    const auto begin=HostDevice::Now();
    PutWide(config,16,std::max(begin,previousNonce_+1));previousNonce_=Wide(config,16);
    PutWide(config,18,i+1);PutWide(config,22,head_.commit);PutWide(config,24,head_.commit);
    PutWide(config,26,head_.stateGeneration);PutWide(config,28,head_.stateGeneration+1);
    PutWide(config,30,head_.sceneGeneration);PutWide(config,32,head_.sceneGeneration+1);
    PutWide(config,34,++fenceGeneration_);
    PutWide(config,36,begin);PutWide(config,38,begin+budget);
    config[40]=head_.bank;config[41]=1-head_.bank;config[42]=head_.scene;config[43]=1-head_.scene;
    const VkCommandBufferBeginInfo start{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    RequireVk(VKFN(BeginCommandBuffer)(command_,&start),"G16_ROOT_BEGIN");
    if(i==0) {
      const auto &state=resources_[0].slice;
      VKFN(CmdFillBuffer)(command_,state.buffer,0,state.bytes,0);
      const VkMemoryBarrier clear{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
      VKFN(CmdPipelineBarrier)(command_,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&clear,0,nullptr,0,nullptr);
      VKFN(CmdUpdateBuffer)(command_,state.buffer,0,p.seed.size()*4,p.seed.data());
    }
    bool faultArmed=false;
#ifdef PC_G16_FREEZE_FIXTURE
    if(p.fault && i+1==p.roots) {
      RecordFreezeFault(*p.fault,config[40]);
      faultArmed=true;
    }
#endif
    VKFN(CmdUpdateBuffer)(command_,resources_[2].slice.buffer,configOffset,sizeof(config),config.data());
    writeBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_WRITE_BIT);
    VKFN(CmdBindPipeline)(command_,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_);
    VKFN(CmdBindDescriptorSets)(command_,VK_PIPELINE_BIND_POINT_COMPUTE,layout_,0,1,&set_,0,nullptr);
    const auto dispatch=[&](uint32_t stage,uint32_t micro) {
      const uint32_t push[]{stage,micro,p.j+2};
      VKFN(CmdPushConstants)(command_,layout_,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push);
      VKFN(CmdDispatch)(command_,1,1,1);
    };
    dispatch(0,0);
    for(uint32_t micro=0;micro<p.j;++micro) {
      writeBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);
      dispatch(1,micro);
    }
    writeBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);
    dispatch(2,0);
    auto hot=resources_[2].slice;
    hot.offset=receiptOffset;hot.bytes=receiptBytes;
    const CopySpan copy{hot,0};
    RequireHost(RecordReadback(VKFN(CmdPipelineBarrier),VKFN(CmdCopyBuffer),command_,readback_.slice,&copy,1) &&
        MakeHostVisible(VKFN(CmdPipelineBarrier),command_,readback_.slice),"G16_HOT_RECORD");
    RequireVk(VKFN(EndCommandBuffer)(command_),"G16_ROOT_END");
    VkResult submit=VK_NOT_READY,retire=VK_NOT_READY;
    const auto result=Submit(begin+budget,readback_,receiptBytes,submit,retire);
    Completion receipt{};
    if(result==VK_SUCCESS)std::copy_n(readback_.words,receipt.size(),receipt.begin());
    const bool valid=result==VK_SUCCESS && CheckIdentity(config,receipt,receipt[40]) &&
        (receipt[40]==1 || (receipt[40]==2 && receipt[41]>=1 && receipt[41]<=17));
    const bool current=gpu_.Current();
    const bool fresh=HostDevice::Now()<=begin+budget;
    const bool accepted=!faultArmed && valid && current && fresh && CanCommit(config,receipt);
    if(accepted)
      head_={config[41],config[43],Wide(receipt,20),Wide(receipt,28),Wide(receipt,32)};
    std::cout<<"{\"stage\":\"G16_PRIVATE_ROOT\",\"run\":\""<<Wide(config,8)<<"\",\"nonce\":\""<<Wide(config,16)
      <<"\",\"inputId\":\""<<Wide(config,20)<<"\",\"fence\":\""<<reinterpret_cast<uintptr_t>(fence_)
      <<"\",\"fenceGeneration\":\""<<fenceGeneration_<<"\",\"beginNs\":\""<<begin
      <<"\",\"exitNs\":\""<<HostDevice::Now()<<"\",\"submit\":"<<int(submit)<<",\"retire\":"<<int(retire)
      <<",\"vkResult\":"<<int(result)<<",\"current\":"<<(current?"true":"false")<<",\"fresh\":"<<(fresh?"true":"false")
      <<",\"baseCommit\":\""<<Wide(config,22)<<"\",\"privateCommit\":\""<<head_.commit
      <<"\",\"identityOK\":"<<(valid?"true":"false")<<",\"firstReason\":"<<receipt[41]
      <<",\"firstStage\":"<<receipt[42]<<",\"B\":"<<receipt[53]<<",\"N\":"<<p.disks
      <<",\"C\":"<<receipt[54]<<",\"Ic\":"<<receipt[59]<<",\"dispatches\":"<<p.j+2
      <<",\"computeBarriers\":"<<p.j+1<<",\"acceptedRoot\":null}\n";
    if(valid)
      RequireHost(bool(stream.write(reinterpret_cast<const char *>(receipt.data()),sizeof(receipt))),"G16_COMPLETION_WRITE");
    if(!accepted || i+1==p.roots) {
      if(valid)Export(p,config,receipt,accepted);
#ifdef PC_G16_FREEZE_FIXTURE
      if(faultArmed) {
        const bool passed=valid && current && fresh && receipt[40]==2 && receipt[41]==2 && receipt[42]==1 &&
            receipt[45]==p.fault->disk && receipt[63]==0x801 && receipt[62]==0 &&
            receipt[54]==0 && receipt[56]==0 && receipt[58]==0 && receipt[59]==0 &&
            Wide(receipt,20)==0 && Wide(receipt,24)==0 &&
            head_.bank==config[40] && head_.scene==config[42] && head_.commit==Wide(config,22) &&
            head_.stateGeneration==Wide(config,26) && head_.sceneGeneration==Wide(config,30);
        std::cout<<"{\"stage\":\"G16_FREEZE_FIXTURE_RESULT\",\"passed\":"<<(passed?"true":"false")
          <<",\"baseBank\":"<<head_.bank<<",\"privateCommit\":\""<<head_.commit
          <<"\",\"stateGeneration\":\""<<head_.stateGeneration<<"\",\"sceneGeneration\":\""<<head_.sceneGeneration
          <<"\",\"firstDisk\":"<<receipt[45]<<",\"acceptedRoot\":null}\n";
        return passed;
      }
#endif
      return accepted;
    }
  }
  return false;
}
#undef VKFN
}

int RunG16Multi(int argc,wchar_t **argv) {
  Program p;
  bool faultMode=false;
#ifdef PC_G16_FREEZE_FIXTURE
  faultMode=argc==12 && std::wstring_view(argv[8])==L"--freeze-fault";
#endif
  RequireHost(faultMode || (argc>=8 && argc<=10),
      "USE: g16-multi MODULE SEED (static|fast|slow|hold) (128|256) ROOTS OUT [mirror] [present]");
  for(int i=8;!faultMode && i<argc;++i) {
    const std::wstring_view flag(argv[i]);
    if(flag==L"mirror" && !p.mirror)p.mirror=true;
    else if(flag==L"present" && !p.present)p.present=true;
    else RequireHost(false,"G16_FLAG");
  }
  p.module=ReadWords(argv[2],256*1024);
  const auto moduleSHA=Hex(Hash(p.module.data(),p.module.size()*4));
  RequireHost(moduleSHA=="792f5986328b796aa4bea47b601899cce55aef83e5f0e76ed3d29afe7b718cda" ||
              moduleSHA=="786bb1db9b8d3a679042bdc36d014226d68647913770d503192c297e0c10e361","G16_MODULE_SHA");
  p.seed=ReadWords(argv[3],52*64);
  const auto seedSHA=Hex(Hash(p.seed.data(),p.seed.size()*4));
  if(seedSHA=="d74e416cebfbc9199c85a5c25f63ed52f315873ec2b5908a7f7ce94bbe1d3168")p.disks=13;
  else if(seedSHA=="eca4c474bc8143d358e0ed5f3a67f93c7f9d0a91f9c86fbe1a405b909ffb0884")p.disks=52;
  else RequireHost(false,"G16_SEED_SHA");
  const std::wstring_view motion(argv[4]),steps(argv[5]);
  RequireHost(motion==L"static" || motion==L"fast" || motion==L"slow" || motion==L"hold","G16_POSE_PROFILE");
  p.motion=motion==L"static"?Motion::Static:motion==L"fast"?Motion::Fast:motion==L"slow"?Motion::Slow:Motion::Hold;
  RequireHost(steps==L"128" || steps==L"256","G16_J");
  p.j=steps==L"128"?128:256;
  const std::wstring_view rootCount(argv[6]);
  RequireHost(!rootCount.empty() && rootCount.size()<=4 &&
      std::all_of(rootCount.begin(),rootCount.end(),[](wchar_t c){return c>=L'0' && c<=L'9';}),"G16_ROOT_COUNT");
  p.roots=uint32_t(std::stoul(argv[6]));
  RequireHost(p.roots && p.roots<=1200,"G16_ROOT_COUNT");
#ifdef PC_G16_FREEZE_FIXTURE
  if(faultMode) {
    const auto decimal=[](const wchar_t *raw) {
      const std::wstring_view digits(raw);
      RequireHost(!digits.empty() && digits.size()<=2 &&
          std::all_of(digits.begin(),digits.end(),[](wchar_t c){return c>=L'0' && c<=L'9';}),"G16_FIXTURE_NUMBER");
      return uint32_t(std::stoul(raw));
    };
    const auto disk=decimal(argv[9]),value=decimal(argv[11]);
    const std::wstring_view field(argv[10]);
    const auto word=field==L"flags"?13u:field==L"reserved0"?14u:field==L"reserved1"?15u:
      field==L"generation"?1u:field==L"origin"?2u:field==L"material"?3u:UINT32_MAX;
    RequireHost((p.roots==1 || p.roots==2) && disk>=1 && disk<=p.disks && value<=3 && word!=UINT32_MAX &&
        value!=(word>=14?0u:word==2?disk:1u),"G16_FIXTURE_ARGUMENTS");
    p.fault=FreezeFault{disk,word,value};
  }
#endif
  p.out=argv[7];
  if(p.mirror)for(uint32_t i=0;i<p.disks;++i)
    for(uint32_t w: {4u,6u,8u,9u}) p.seed[i*16u+w]^=0x80000000u;
  p.Freeze(HostDevice::Now());
  RequireHost(std::filesystem::create_directory(p.out),"G16_OUTPUT_EXISTS");
  Write(p.out/"initial-config.u32le",p.initial.data(),sizeof(p.initial));
  Write(p.out/"seed.disk64",p.seed.data(),p.seed.size()*4);
  std::cout<<"{\"stage\":\"G16_INPUT\",\"seedSHA\":\""<<seedSHA<<"\",\"actualSeedSHA\":\""
    <<Hex(Hash(p.seed.data(),p.seed.size()*4))<<"\",\"moduleSHA\":\""<<moduleSHA<<"\",\"J\":"<<p.j<<"}\n";
  HostDevice gpu;
  gpu.Open(p.present,L"G16 Vulkan 13/52 disk diagnostic");
  Owner owner(gpu);
  owner.Prepare(p);
  return owner.Run(p)?0:3;
}
}
