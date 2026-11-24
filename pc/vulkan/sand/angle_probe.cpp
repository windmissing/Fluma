#include "host_device.h"
#include "../../../entry/src/main/cpp/gpu_resident/program_cache/sha256.h"
#include <fstream>
#include <iostream>

namespace grs::pc::sand {
namespace {
std::vector<uint32_t> Read(const wchar_t *path,size_t limit) {
  std::ifstream in(std::filesystem::path(path),std::ios::binary|std::ios::ate);
  const auto size=in.tellg();
  RequireHost(size>0 && uint64_t(size)<=limit && size%4==0,"INPUT_RANGE");
  std::vector<uint32_t> words(size_t(size)/4);
  in.seekg(0);
  RequireHost(bool(in.read(reinterpret_cast<char *>(words.data()),size)),"INPUT_READ");
  return words;
}

std::string Digest(const std::vector<uint32_t> &words) {
  constexpr char hex[]="0123456789abcdef";
  p011gpu::Sha256 hash;
  hash.update(reinterpret_cast<const uint8_t*>(words.data()),words.size()*4);
  std::string text;
  for (auto byte:hash.finish()) { text+=hex[byte>>4]; text+=hex[byte&15]; }
  return text;
}

struct Commands {
  HostDevice &gpu;
  VkDescriptorSetLayout descriptors=VK_NULL_HANDLE;
  VkDescriptorPool sets=VK_NULL_HANDLE;
  VkPipelineLayout layout=VK_NULL_HANDLE;
  VkShaderModule shader=VK_NULL_HANDLE;
  VkPipeline pipeline=VK_NULL_HANDLE;
  VkCommandPool commands=VK_NULL_HANDLE;
  VkFence fence=VK_NULL_HANDLE;
  bool pending=false;
  template<class Fn> Fn Proc(const char *name) {
    auto f=reinterpret_cast<Fn>(gpu.deviceProc(gpu.device,name));
    RequireHost(f!=nullptr,name);
    return f;
  }
  ~Commands() {
#define DESTROY(name,handle) if (handle) Proc<PFN_vkDestroy##name>("vkDestroy" #name)(gpu.device,handle,nullptr)
    if (pending) {
      VkResult result;
      do {
        result=Proc<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(gpu.device);
        if (result!=VK_SUCCESS && result!=VK_ERROR_DEVICE_LOST) Sleep(50);
      } while (result!=VK_SUCCESS && result!=VK_ERROR_DEVICE_LOST);
      const auto confirm=result==VK_SUCCESS ?
          Proc<PFN_vkWaitForFences>("vkWaitForFences")(gpu.device,1,&fence,VK_TRUE,0) : result;
      std::cout << "{\"stage\":\"PROBE_DRAIN\",\"idle\":" << int(result)
                << ",\"fence\":" << int(confirm) << ",\"acceptedRoot\":null}\n";
    }
    DESTROY(Fence,fence); DESTROY(CommandPool,commands); DESTROY(Pipeline,pipeline);
    DESTROY(ShaderModule,shader); DESTROY(PipelineLayout,layout); DESTROY(DescriptorPool,sets);
    DESTROY(DescriptorSetLayout,descriptors);
#undef DESTROY
  }

  VkResult Run(const std::vector<uint32_t> &module,const std::vector<std::vector<uint32_t>> &input,
               const VulkanOwner::Readback &output,uint32_t groups) {
    const auto d=gpu.device;
    const uint32_t count=uint32_t(input.size()+1);
    std::array<VkDescriptorSetLayoutBinding,3> bindings{};
    std::array<Slice,3> slices{};
    for (uint32_t i=0;i<count;++i) {
      bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
      slices[i]=i+1==count ? output.buffer : gpu.Allocate(input[i].size()*4,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT).slice;
    }
#define F(name) Proc<PFN_vk##name>("vk" #name)
    const VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,nullptr,0,count,bindings.data()};
    RequireVk(F(CreateDescriptorSetLayout)(d,&schema,nullptr,&descriptors),"PROBE_LAYOUT");
    const VkDescriptorPoolSize slots{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,count};
    const VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr,0,1,1,&slots};
    RequireVk(F(CreateDescriptorPool)(d,&pool,nullptr,&sets),"PROBE_SET_POOL");
    const VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,nullptr,sets,1,&descriptors};
    VkDescriptorSet set=VK_NULL_HANDLE;
    RequireVk(F(AllocateDescriptorSets)(d,&setInfo,&set),"PROBE_SET");
    std::array<VkDescriptorBufferInfo,3> ranges{};
    std::array<VkWriteDescriptorSet,3> writes{};
    for (uint32_t i=0;i<count;++i) {
      ranges[i]={slices[i].buffer,0,slices[i].bytes};
      writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,set,i,0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 nullptr,&ranges[i],nullptr};
    }
    F(UpdateDescriptorSets)(d,count,writes.data(),0,nullptr);
    const VkPipelineLayoutCreateInfo pipelineSchema{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,nullptr,0,1,&descriptors};
    RequireVk(F(CreatePipelineLayout)(d,&pipelineSchema,nullptr,&layout),"PROBE_PIPELINE_LAYOUT");
    const VkShaderModuleCreateInfo program{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,nullptr,0,module.size()*4,module.data()};
    RequireVk(F(CreateShaderModule)(d,&program,nullptr,&shader),"PROBE_MODULE");
    VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};
    compute.layout=layout;
    RequireVk(F(CreateComputePipelines)(d,VK_NULL_HANDLE,1,&compute,nullptr,&pipeline),"PROBE_PIPELINE");
    const VkCommandPoolCreateInfo commandPool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,nullptr,0,gpu.family};
    RequireVk(F(CreateCommandPool)(d,&commandPool,nullptr,&commands),"PROBE_COMMAND_POOL");
    const VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,nullptr,
                                                 commands,VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
    VkCommandBuffer command=VK_NULL_HANDLE;
    RequireVk(F(AllocateCommandBuffers)(d,&commandInfo,&command),"PROBE_COMMAND");
    const VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    RequireVk(F(CreateFence)(d,&fenceInfo,nullptr,&fence),"PROBE_FENCE");
    const VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    RequireVk(F(BeginCommandBuffer)(command,&begin),"PROBE_BEGIN");
    const auto update=F(CmdUpdateBuffer);
    for (size_t i=0;i<input.size();++i) for (size_t at=0;at<input[i].size();at+=16384) {
      const size_t words=std::min<size_t>(16384,input[i].size()-at);
      update(command,slices[i].buffer,at*4,words*4,input[i].data()+at);
    }
    const auto barrier=[&](VkPipelineStageFlags from,VkAccessFlags written,VkPipelineStageFlags to,VkAccessFlags reading) {
      const VkMemoryBarrier m{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,written,reading};
      F(CmdPipelineBarrier)(command,from,to,0,1,&m,0,nullptr,0,nullptr);
    };
    barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT);
    F(CmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
    F(CmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,nullptr);
    F(CmdDispatch)(command,groups,1,1);
    barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
    RequireVk(F(EndCommandBuffer)(command),"PROBE_END");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount=1; submit.pCommandBuffers=&command;
    const auto start=HostDevice::Now();
    RequireVk(F(QueueSubmit)(gpu.queue,1,&submit,fence),"PROBE_SUBMIT");
    pending=true;
    constexpr uint64_t budget=250000000;
    const auto now=HostDevice::Now();
    const auto wait=F(WaitForFences)(d,1,&fence,VK_TRUE,now-start<budget ? budget-(now-start) : 0);
    const auto elapsed=HostDevice::Now()-start;
    pending=wait!=VK_SUCCESS && wait!=VK_ERROR_DEVICE_LOST;
    std::cout << "{\"stage\":\"PRIVATE_ANGLE_PROBE\",\"vendor\":" << gpu.properties.vendorID
              << ",\"device\":" << gpu.properties.deviceID << ",\"driver\":" << gpu.properties.driverVersion
              << ",\"queueFamily\":" << gpu.family << ",\"moduleSHA\":\"" << Digest(module)
              << "\",\"fenceVkResult\":" << int(wait) << ",\"elapsedNs\":" << elapsed << ",\"acceptedRoot\":null}\n";
    if (wait!=VK_SUCCESS || elapsed>budget) return wait==VK_SUCCESS ? VK_TIMEOUT : wait;
    return AwaitReadback(F(WaitForFences),F(InvalidateMappedMemoryRanges),d,fence,0,output.coherent,
                         output.memory,output.allocationBytes,gpu.properties.limits.nonCoherentAtomSize,
                         output.mappedOffset,output.buffer.bytes);
#undef F
  }
};
}
}

int wmain(int argc,wchar_t **argv) {
  using namespace grs::pc::sand;
  try {
    const bool pose=argc>=2 && std::wstring_view(argv[1])==L"g15-pose";
    const bool g15=pose || (argc>=2 && std::wstring_view(argv[1])==L"g15");
    const bool g16=argc>=2 && std::wstring_view(argv[1])==L"g16";
    RequireHost((g15 && argc==6)||(g16 && argc==5),"USE: (g15|g15-pose MODULE INPUT ANGLES OUTPUT) | (g16 MODULE INPUT OUTPUT)");
    const auto module=Read(argv[2],64*1024);
    RequireHost(Digest(module)==(pose ? "21452cbf2441d19b0feb386488548bc7d3833d6587e871b631c3df6954ee5086" :
                                g15 ? "0744d45f428c56064089cff52dafa08e189f80bed0eaca817e13fbb4ad069fd0" :
                                    "cc1c655f6ac151fb499c9b3e2894233a4d713482633f4986e08601b03117dc8b"),"PROBE_MODULE_SHA");
    std::vector<std::vector<uint32_t>> input{Read(argv[3],(6+127)*4)};
    const auto &body=input.front();
    const size_t header=pose ? 6 : 4;
    const uint32_t faces=g15 && body.size()>=header ? body[header-1] : 0;
    RequireHost(g15 ? faces && faces<=127 && body.size()==header+faces : body.size()==13,"PROBE_INPUT_ABI");
    const size_t bytes=g15 ? size_t(faces)*(pose ? 32 : 24) : 36;
    if (g15) {
      input.push_back(Read(argv[4],184328));
      RequireHost(Digest(input.back())=="6278b4a06d704df8439b3291b1c3abff608a42049c4837720e6745a4b272ad61","ANGLE_TABLE_SHA");
    }
    const std::filesystem::path destination(argv[argc-1]);
    RequireHost(!std::filesystem::exists(destination),"PROBE_OUTPUT_EXISTS");
    HostDevice gpu;
    gpu.Open(false);
    const auto output=gpu.Allocate(bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,true).Readback();
    Commands commands{gpu};
    RequireVk(commands.Run(module,input,output,g15 ? (faces+63)/64 : 1),"PROBE_RESULT");
    std::ofstream file(destination,std::ios::binary);
    RequireHost(bool(file.write(reinterpret_cast<const char *>(output.words),bytes)),"PROBE_OUTPUT_WRITE");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
