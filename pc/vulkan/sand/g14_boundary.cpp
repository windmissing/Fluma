#include "g14_boundary.h"
#include "g14_layout.h"
#include "host_device.h"
#include "../../../entry/src/main/cpp/gpu_resident/program_cache/sha256.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace grs::pc::sand {
namespace {
using namespace g14;
namespace fs=std::filesystem;
constexpr std::array<VkDeviceSize,4> sizes{B0_BYTES,B1_BYTES,B2_BYTES,B3_BYTES};
constexpr uint64_t budget=250000000;
constexpr uint32_t seal=0xc0149571,diagnosticSeal=0xd0149571;
constexpr uint32_t maxChildren=4;
static_assert(256+maxChildren*SNAPSHOT_STEP*4==B3_BYTES);

std::string Digest(const void *data,size_t size) {
  p011gpu::Sha256 sha;
  sha.update(static_cast<const uint8_t *>(data),size);
  std::string result;
  constexpr char hex[]="0123456789abcdef";
  for(const auto b:sha.finish()){result+=hex[b>>4];result+=hex[b&15];}
  return result;
}
std::vector<uint32_t> Read(const fs::path &path,size_t minimum,size_t maximum,bool words) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);
  const auto end=in.tellg();
  RequireHost(end>=0 && uint64_t(end)>=minimum && uint64_t(end)<=maximum &&
              (!words || end%4==0),"G14_ASSET_RANGE");
  std::vector<uint32_t> result((size_t(end)+3)/4);
  in.seekg(0);
  RequireHost(bool(in.read(reinterpret_cast<char *>(result.data()),end)),"G14_ASSET_READ");
  return result;
}
void Write(const fs::path &path,const void *data,size_t bytes) {
  std::ofstream out(path,std::ios::binary);
  RequireHost(bool(out.write(static_cast<const char *>(data),bytes)),"G14_RAW_WRITE");
}
uint64_t Wide(const uint32_t *p){return uint64_t(p[1])<<32|p[0];}
void PutWide(uint32_t *p,uint64_t value){p[0]=uint32_t(value);p[1]=uint32_t(value>>32);}

struct GridBoundaryViewV1 {
  const uint32_t *fixture;
  uint64_t nonce,generation,lease;
  Slice input;
};
GridBoundaryViewV1 AcquireBoundaryView(const uint32_t *fixture,uint64_t nonce,uint64_t generation,
                                      uint64_t lease,const Slice &input) {
  RequireHost(fixture && fixture[0]==0x47313449 && fixture[1]==1 && fixture[2]>=1 && fixture[2]<=3 &&
      fixture[3] && fixture[3]<=40 && fixture[4] && fixture[4]<=20 && fixture[5] &&
      fixture[5]<=maxChildren && nonce && generation && lease && input.buffer && input.generation &&
      input.offset%256==0 && input.bytes==B0_BYTES && input.offset<=UINT64_MAX-input.bytes,"G14_VIEW_RANGE");
  return {fixture,nonce,generation,lease,input};
}

class Owner {
 public:
  explicit Owner(HostDevice &device):gpu(device){}
  ~Owner() {
    if(pending) Fn<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(gpu.device);
#define DESTROY(Name,handle) if(handle)Fn<PFN_vkDestroy##Name>("vkDestroy" #Name)(gpu.device,handle,nullptr)
    DESTROY(Fence,fence);
    DESTROY(CommandPool,commands);
    for(auto pipeline:pipelines){DESTROY(Pipeline,pipeline);}
    for(auto shader:shaders){DESTROY(ShaderModule,shader);}
    DESTROY(PipelineLayout,pipelineLayout);
    DESTROY(DescriptorPool,descriptors);
    DESTROY(DescriptorSetLayout,setLayout);
#undef DESTROY
  }
  void Prepare(const std::array<std::vector<uint32_t>,3> &modules);
  bool Run(const fs::path &,const std::vector<uint32_t> &,const Asset &fixture);
 private:
  template<class T> T Fn(const char *name) {
    auto result=reinterpret_cast<T>(gpu.deviceProc(gpu.device,name));
    RequireHost(result!=nullptr,name);
    return result;
  }
  HostDevice &gpu;
  std::array<Slice,4> buffers{};
  HostDevice::Memory staging{};
  VkDescriptorSetLayout setLayout=VK_NULL_HANDLE;
  VkDescriptorPool descriptors=VK_NULL_HANDLE;
  VkDescriptorSet set=VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout=VK_NULL_HANDLE;
  std::array<VkShaderModule,3> shaders{};
  std::array<VkPipeline,3> pipelines{};
  VkCommandPool commands=VK_NULL_HANDLE;
  VkCommandBuffer command=VK_NULL_HANDLE;
  VkFence fence=VK_NULL_HANDLE;
  bool pending=false;
};

#define VK(name) Fn<PFN_vk##name>("vk" #name)
void Owner::Prepare(const std::array<std::vector<uint32_t>,3> &modules) {
  const auto &limit=gpu.properties.limits;
  RequireHost(limit.maxPerStageDescriptorStorageBuffers>=4 && limit.maxDescriptorSetStorageBuffers>=4 &&
      limit.maxStorageBufferRange>=B3_BYTES && limit.maxPushConstantsSize>=32 &&
      limit.maxComputeWorkGroupInvocations>=64 && limit.maxComputeWorkGroupSize[0]>=64 &&
      limit.maxComputeWorkGroupCount[0]>=(B1_BYTES/4+63)/64,"G14_DEVICE_LIMITS");
  std::array<VkDescriptorSetLayoutBinding,4> binding{};
  std::array<VkDescriptorBufferInfo,4> range{};
  for(uint32_t i=0;i<sizes.size();++i) {
    buffers[i]=gpu.Allocate(sizes[i],VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|
        (i==3 ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0)).slice;
    binding[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
    range[i]={buffers[i].buffer,buffers[i].offset,buffers[i].bytes};
  }
  staging=gpu.Allocate(B3_BYTES,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
  VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,nullptr,0,4,binding.data()};
  RequireVk(VK(CreateDescriptorSetLayout)(gpu.device,&layout,nullptr,&setLayout),"G14_SET_LAYOUT");
  const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,4};
  const VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr,0,1,1,&size};
  RequireVk(VK(CreateDescriptorPool)(gpu.device,&pool,nullptr,&descriptors),"G14_POOL");
  const VkDescriptorSetAllocateInfo sets{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,nullptr,descriptors,1,&setLayout};
  RequireVk(VK(AllocateDescriptorSets)(gpu.device,&sets,&set),"G14_SET");
  std::array<VkWriteDescriptorSet,4> writes{};
  for(uint32_t i=0;i<writes.size();++i)
    writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,set,i,0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
               nullptr,&range[i],nullptr};
  VK(UpdateDescriptorSets)(gpu.device,4,writes.data(),0,nullptr);
  const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,32};
  const VkPipelineLayoutCreateInfo recipe{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,nullptr,0,1,&setLayout,1,&push};
  RequireVk(VK(CreatePipelineLayout)(gpu.device,&recipe,nullptr,&pipelineLayout),"G14_PIPELINE_LAYOUT");
  for(size_t i=0;i<modules.size();++i) {
    const auto &m=modules[i];
    const VkShaderModuleCreateInfo spv{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,nullptr,0,m.size()*4,m.data()};
    RequireVk(VK(CreateShaderModule)(gpu.device,&spv,nullptr,&shaders[i]),"G14_MODULE");
    VkComputePipelineCreateInfo pipe{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipe.layout=pipelineLayout;
    pipe.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,
                shaders[i],"main",nullptr};
    RequireVk(VK(CreateComputePipelines)(gpu.device,VK_NULL_HANDLE,1,&pipe,nullptr,&pipelines[i]),"G14_PIPELINE");
  }
  const VkCommandPoolCreateInfo cmd{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,nullptr,0,gpu.family};
  RequireVk(VK(CreateCommandPool)(gpu.device,&cmd,nullptr,&commands),"G14_COMMAND_POOL");
  const VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,nullptr,commands,
                                            VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
  RequireVk(VK(AllocateCommandBuffers)(gpu.device,&allocate,&command),"G14_COMMAND");
  const VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  RequireVk(VK(CreateFence)(gpu.device,&info,nullptr,&fence),"G14_FENCE");
}

bool Owner::Run(const fs::path &out,const std::vector<uint32_t> &words,const Asset &fixture) {
  std::ofstream log(out/"roots.jsonl");
  RequireHost(bool(log),"G14_ROOT_LOG");
  const auto barrier=[&](VkPipelineStageFlags from,VkAccessFlags access){
    const VkMemoryBarrier edge{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,access,
                               VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT};
    VK(CmdPipelineBarrier)(command,from,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&edge,0,nullptr,0,nullptr);
  };
  const auto run=HostDevice::Now();
  for(size_t offset=0;offset<words.size();offset+=B0_BYTES/4) {
    std::array<uint32_t,B0_BYTES/4> input{};
    std::memcpy(input.data(),words.data()+offset,sizeof(input));
    const uint64_t generation=offset/input.size()+1;
    RequireHost(!pending && gpu.Current() && input[7]==generation,"G14_ROOT_SCOPE");
    const auto opened=HostDevice::Now();
    const auto view=AcquireBoundaryView(input.data(),opened,generation,generation,buffers[0]);
    const auto until=opened+budget;
    PutWide(input.data()+8,input[2]==2 && (input[6]&8) ? generation+1 : generation);
    PutWide(input.data()+10,view.nonce);
    const uint32_t children=input[5],faces=(input[3]+1)*input[4]+input[3]*(input[4]+1);
    const auto file=std::to_string(generation);
    RequireVk(VK(ResetFences)(gpu.device,1,&fence),"G14_RESET_FENCE");
    RequireVk(VK(ResetCommandPool)(gpu.device,commands,0),"G14_RESET_COMMAND");
    const VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,
                                        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,nullptr};
    RequireVk(VK(BeginCommandBuffer)(command,&begin),"G14_BEGIN");
    VK(CmdUpdateBuffer)(command,view.input.buffer,view.input.offset,sizeof(input),input.data());
    for(size_t b=1;b<buffers.size();++b)VK(CmdFillBuffer)(command,buffers[b].buffer,0,buffers[b].bytes,0);
    barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);
    VK(CmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&set,0,nullptr);
    uint32_t dispatches=0;
    const auto dispatch=[&](uint32_t pipe,uint32_t op,uint32_t child,uint32_t n){
      const uint32_t push[]{op,child,children,uint32_t(view.generation),uint32_t(view.generation>>32),
                            64+child*SNAPSHOT_STEP,0,0};
      VK(CmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelines[pipe]);
      VK(CmdPushConstants)(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push);
      VK(CmdDispatch)(command,(n+63)/64,1,1);
      ++dispatches;
      barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT);
    };
    dispatch(2,0,0,1);
    for(uint32_t child=0;child<children;++child) {
      dispatch(0,1,child,input[3]*input[4]);
      dispatch(0,2,child,faces);
      dispatch(1,3,child,faces);
      dispatch(1,4,child,input[3]*input[4]);
      dispatch(2,5,child,1);
      dispatch(2,6,child,B1_BYTES/4);
    }
    dispatch(2,7,0,1);
    auto slice=buffers[3];
    slice.bytes=256+children*SNAPSHOT_STEP*4;
    const CopySpan copy{slice,0};
    RequireHost(dispatches<=26 && RecordReadback(VK(CmdPipelineBarrier),VK(CmdCopyBuffer),command,
        staging.slice,&copy,1) && MakeHostVisible(VK(CmdPipelineBarrier),command,staging.slice),"G14_COPY");
    RequireVk(VK(EndCommandBuffer)(command),"G14_END");
    const auto beforeSubmit=HostDevice::Now();
    const VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,nullptr,0,nullptr,nullptr,1,&command,0,nullptr};
    const auto queued=beforeSubmit<until && gpu.Current()?VK(QueueSubmit)(gpu.queue,1,&submit,fence):VK_NOT_READY;
    VkResult waited=VK_NOT_READY,retired=VK_NOT_READY,read=VK_NOT_READY;
    uint64_t businessFenceNs=0,retiredNs=0;
    if(queued==VK_SUCCESS) {
      pending=true;
      const auto now=HostDevice::Now();
      waited=VK(WaitForFences)(gpu.device,1,&fence,VK_TRUE,now<until?until-now:0);
      businessFenceNs=HostDevice::Now();
      retired=waited==VK_TIMEOUT?VK(WaitForFences)(gpu.device,1,&fence,VK_TRUE,5000000000ull):waited;
      retiredNs=HostDevice::Now();
      pending=retired!=VK_SUCCESS && retired!=VK_ERROR_DEVICE_LOST;
      if(retired==VK_SUCCESS) {
        read=AwaitReadback(VK(WaitForFences),VK(InvalidateMappedMemoryRanges),gpu.device,fence,0,
          staging.coherent,staging.memory,staging.allocation,gpu.properties.limits.nonCoherentAtomSize,
          0,slice.bytes);
      }
    }
    const auto *r=staging.words;
    const bool identity=read==VK_SUCCESS && r[0]==0x47313452 && r[1]==1 && r[2]==input[2] &&
        r[3]==generation && Wide(r+4)==generation && r[6]==children && r[7]==input[3] &&
        r[8]==input[4] && Wide(r+10)==view.nonce && Wide(r+12)==Wide(input.data()+8);
    const auto checked=HostDevice::Now();
    const bool timely=waited==VK_SUCCESS && checked<=until && gpu.Current();
    bool siblings=identity;
    for(uint32_t i=0;i<children && siblings;++i)siblings=r[32+i]==seal;
    const bool sealed=timely && siblings && r[16]==0 && r[14]==dispatches && r[63]==seal &&
        r[15]==(input[2]==2?2u:1u) && r[26]==0x3f800000 && r[27]==0x30800000;
    const char *state=sealed?(input[2]==2?"MANUFACTURED_SEALED":"ANALYTIC_SEALED"):
        queued!=VK_SUCCESS?"SUBMIT_REJECTED":retired!=VK_SUCCESS?"ORIGINAL_FENCE_UNRETIRED":
        !timely?"LATE_DIAGNOSTIC":!identity?"RECEIPT_IDENTITY":"GPU_REJECTED";
    log<<"{\"schema\":\"GridBoundaryViewV1\",\"state\":\""<<state<<"\",\"acceptedRoot\":null,\"typedToken\":null"
       <<",\"fixtureSHA\":\""<<fixture.sha<<"\",\"layoutSHA\":\""<<kLayoutSHA<<"\",\"moduleManifestSHA\":\""
       <<kManifestSHA<<"\",\"run\":\""<<run<<"\",\"root\":"<<generation<<",\"nonce\":\""<<view.nonce
       <<"\",\"kind\":\""<<(input[2]==2?"MANUFACTURED":"ANALYTIC")
       <<"\",\"fieldGeneration\":\""<<view.generation<<"\",\"geometryGeneration\":\""<<view.generation
       <<"\",\"commit\":null,\"stateBank\":null,\"bedRevision\":1,\"poseRevision\":1,\"coordinateRevision\":1"
       <<",\"fieldProvider\":\"AnalyticProvider\",\"coordinateUnits\":\"BOX_SI_X_RIGHT_Y_UP\",\"indexOrder\":\"X_FAST\""
       <<",\"producerFence\":{\"handle\":\""<<reinterpret_cast<uintptr_t>(fence)<<"\",\"generation\":\""<<generation
       <<"\",\"device\":\""<<reinterpret_cast<uintptr_t>(gpu.device)<<"\",\"queue\":\""
       <<reinterpret_cast<uintptr_t>(gpu.queue)<<"\",\"VkResult\":"<<int(waited)<<"},\"retiredVkResult\":"<<int(retired)
       <<",\"submitVkResult\":"<<int(queued)<<",\"readVkResult\":"<<int(read)<<",\"submitNs\":\""<<beforeSubmit
       <<"\",\"deadlineNs\":\""<<until<<"\",\"fenceNs\":\""<<businessFenceNs<<"\",\"retiredNs\":\""<<retiredNs
       <<"\",\"checkedNs\":\""<<checked<<"\",\"readLeaseSerial\":\""<<view.lease
       <<"\",\"buffer\":{\"resourceSize\":"<<view.input.bytes<<",\"allocationGeneration\":\""
       <<view.input.generation<<"\",\"offset\":"<<view.input.offset<<",\"length\":"<<view.input.bytes
       <<",\"stride\":4},\"dispatches\":"<<dispatches<<",\"identity\":"<<(identity?"true":"false");
    if(identity) {
      log<<",\"reason\":"<<r[16]<<",\"firstStage\":"<<r[17]<<",\"firstChild\":"<<r[18]
         <<",\"entityKind\":"<<r[19]<<",\"firstKey\":"<<r[20]
         <<",\"measuredWords\":["<<r[21]<<','<<r[22]<<"],\"expectedOrBoundWords\":["<<r[23]<<','<<r[24]<<']'
         <<",\"receiptSHA\":\""<<Digest(r,256)
         <<"\",\"children\":[";
      bool separator=false;
      for(uint32_t i=0;i<children;++i)if(r[32+i]==seal || r[32+i]==diagnosticSeal) {
        const auto *snapshot=r+64+i*SNAPSHOT_STEP;
        log<<(separator?",":"")<<"{\"id\":"<<i<<",\"sealed\":"<<(r[32+i]==seal?"true":"false")
           <<",\"sha\":\""<<Digest(snapshot,4*SNAPSHOT_STEP)<<"\"}";
        separator=true;
      }
      log<<']';
      Write(out/(std::string(timely?"root-":"late-")+file+".u32le"),r,slice.bytes);
      Write(out/("input-"+file+".u32le"),input.data(),sizeof(input));
    }
    log<<"}\n"<<std::flush;
    RequireHost(bool(log),"G14_LOG_WRITE");
    if(!sealed)return false;
  }
  return true;
}
#undef VK
}

int RunG14Boundary(int argc,wchar_t **argv) {
  RequireHost(argc==5 && std::wstring_view(argv[1])==L"g14-boundary",
              "USE: g14-boundary MODULE_MANIFEST FIXTURE OUT");
  const fs::path manifest=argv[2], fixturePath=argv[3], out=argv[4];
  std::ifstream stream(manifest,std::ios::binary|std::ios::ate);
  const auto length=stream.tellg();
  RequireHost(length>0 && length<=65536,"G14_MANIFEST_RANGE");
  std::string raw(size_t(length),'\0');
  stream.seekg(0);
  RequireHost(bool(stream.read(raw.data(),length)) && Digest(raw.data(),raw.size())==kManifestSHA,"G14_MANIFEST_SHA");
  std::array<std::vector<uint32_t>,3> modules{};
  for(size_t i=0;i<modules.size();++i) {
    const auto &m=g14::kModules[i];
    modules[i]=Read(manifest.parent_path()/m.name,m.bytes,m.bytes,true);
    RequireHost(modules[i][0]==0x07230203 && Digest(modules[i].data(),m.bytes)==m.sha,"G14_MODULE_SHA");
  }
  const auto words=Read(fixturePath,4096,50*4096,true);
  const auto fixtureSHA=Digest(words.data(),words.size()*4);
  const Asset *fixture=nullptr;
  for(const auto &f:kFixtures)if(fixtureSHA==f.sha && words.size()*4==f.bytes)fixture=&f;
  RequireHost(fixture!=nullptr,"G14_FIXTURE_SHA");
  RequireHost(fs::create_directory(out),"G14_OUTPUT_EXISTS");
  HostDevice gpu;
  gpu.Open(false,L"G14 Analytic Boundary",true);
  std::cout<<"{\"stage\":\"G14_DEVICE\",\"vendor\":"<<gpu.properties.vendorID<<",\"device\":"
           <<gpu.properties.deviceID<<",\"driver\":"<<gpu.properties.driverVersion
           <<",\"shaderFloat64\":true,\"acceptedRoot\":null}\n";
  Owner owner(gpu);
  owner.Prepare(modules);
  return owner.Run(out,words,*fixture)?0:1;
}
}

int wmain(int argc,wchar_t **argv) {
  try{return grs::pc::sand::RunG14Boundary(argc,argv);}
  catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 2;}
}
