#include "host_device.h"
#include "g16_multi.h"
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
  struct Pass {
    VkShaderModule shader=VK_NULL_HANDLE;
    VkPipeline pipeline=VK_NULL_HANDLE;
    VkDescriptorSet set=VK_NULL_HANDLE;
  };

  HostDevice &gpu;
  VkDescriptorSetLayout descriptors=VK_NULL_HANDLE;
  VkDescriptorPool sets=VK_NULL_HANDLE;
  VkPipelineLayout layout=VK_NULL_HANDLE;
  std::array<Pass,2> passes{};
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
    DESTROY(Fence,fence); DESTROY(CommandPool,commands);
    for (const auto &pass:passes) {
      DESTROY(Pipeline,pass.pipeline); DESTROY(ShaderModule,pass.shader);
    }
    DESTROY(PipelineLayout,layout); DESTROY(DescriptorPool,sets);
    DESTROY(DescriptorSetLayout,descriptors);
#undef DESTROY
  }

  void Prepare(Pass &pass,const std::vector<uint32_t> &module,const Slice *slices,uint32_t count) {
    const auto d=gpu.device;
#define F(name) Proc<PFN_vk##name>("vk" #name)
    const VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,nullptr,sets,1,&descriptors};
    RequireVk(F(AllocateDescriptorSets)(d,&setInfo,&pass.set),"PROBE_SET");
    std::array<VkDescriptorBufferInfo,4> ranges{};
    std::array<VkWriteDescriptorSet,4> writes{};
    for (uint32_t i=0;i<count;++i) {
      ranges[i]={slices[i].buffer,0,slices[i].bytes};
      writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,pass.set,i,0,1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 nullptr,&ranges[i],nullptr};
    }
    F(UpdateDescriptorSets)(d,count,writes.data(),0,nullptr);
    const VkShaderModuleCreateInfo program{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,nullptr,0,module.size()*4,module.data()};
    RequireVk(F(CreateShaderModule)(d,&program,nullptr,&pass.shader),"PROBE_MODULE");
    VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,pass.shader,"main",nullptr};
    compute.layout=layout;
    RequireVk(F(CreateComputePipelines)(d,VK_NULL_HANDLE,1,&compute,nullptr,&pass.pipeline),"PROBE_PIPELINE");
#undef F
  }

  VkResult Run(const std::vector<uint32_t> &module,const std::vector<std::vector<uint32_t>> &input,
               const VulkanOwner::Readback &output,uint32_t groups,uint32_t faces,
               const std::vector<uint32_t> &originModule) {
    const auto d=gpu.device;
    const bool origin=!originModule.empty();
    const uint32_t passCount=origin ? 2 : 1;
    const uint32_t count=origin ? 4 : uint32_t(input.size()+1);
    std::array<VkDescriptorSetLayoutBinding,4> bindings{};
    for (uint32_t i=0;i<count;++i) bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
    std::array<Slice,3> inputs{};
    for (size_t i=0;i<input.size();++i)
      inputs[i]=gpu.Allocate(input[i].size()*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT).slice;
#define F(name) Proc<PFN_vk##name>("vk" #name)
    const VkDescriptorSetLayoutCreateInfo schema{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,nullptr,0,count,bindings.data()};
    RequireVk(F(CreateDescriptorSetLayout)(d,&schema,nullptr,&descriptors),"PROBE_LAYOUT");
    const VkDescriptorPoolSize slots{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,count*passCount};
    const VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr,0,passCount,1,&slots};
    RequireVk(F(CreateDescriptorPool)(d,&pool,nullptr,&sets),"PROBE_SET_POOL");
    const VkPipelineLayoutCreateInfo pipelineSchema{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,nullptr,0,1,&descriptors};
    RequireVk(F(CreatePipelineLayout)(d,&pipelineSchema,nullptr,&layout),"PROBE_PIPELINE_LAYOUT");
    if (origin) {
      const auto flux=gpu.Allocate(faces*64,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT).slice;
      const auto scratch=gpu.Allocate((faces-1)*64*8,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT).slice;
      const std::array fluxSlices{inputs[0],inputs[1],flux};
      const std::array originSlices{inputs[2],flux,output.buffer,scratch};
      Prepare(passes[0],module,fluxSlices.data(),uint32_t(fluxSlices.size()));
      Prepare(passes[1],originModule,originSlices.data(),uint32_t(originSlices.size()));
    } else {
      inputs[input.size()]=output.buffer;
      Prepare(passes[0],module,inputs.data(),count);
    }
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
      update(command,inputs[i].buffer,at*4,words*4,input[i].data()+at);
    }
    const auto barrier=[&](VkPipelineStageFlags from,VkAccessFlags written,VkPipelineStageFlags to,VkAccessFlags reading) {
      const VkMemoryBarrier m{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,written,reading};
      F(CmdPipelineBarrier)(command,from,to,0,1,&m,0,nullptr,0,nullptr);
    };
    barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT);
    const auto dispatch=[&](const Pass &pass,uint32_t groupCount) {
      F(CmdBindPipeline)(command,VK_PIPELINE_BIND_POINT_COMPUTE,pass.pipeline);
      F(CmdBindDescriptorSets)(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&pass.set,0,nullptr);
      F(CmdDispatch)(command,groupCount,1,1);
    };
    dispatch(passes[0],groups);
    if (origin) {
      barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT,
              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT);
      dispatch(passes[1],1);
    }
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
              << '"';
    if (origin) std::cout << ",\"originModuleSHA\":\"" << Digest(originModule) << '"';
    std::cout << ",\"fenceVkResult\":" << int(wait) << ",\"elapsedNs\":" << elapsed << ",\"acceptedRoot\":null}\n";
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
    const auto mode=argc>=2 ? std::wstring_view(argv[1]) : std::wstring_view{};
    if(mode==L"g16-multi")return RunG16Multi(argc,argv);
    const bool corrector=mode==L"g15-corrector" || mode==L"g15-step";
    const bool origin=mode==L"g15-origin" || mode==L"g15-step";
    const bool pose=mode==L"g15-pose",flux=origin || corrector || mode==L"g15-flux";
    const bool g15=pose || flux || mode==L"g15";
    const bool g16v2=mode==L"g16-v2",g16=mode==L"g16" || g16v2;
    RequireHost((origin && argc==8)||(g15 && !origin && argc==6)||(g16 && argc==5),
                "USE: (g15|g15-pose|g15-flux|g15-corrector MODULE INPUT ANGLES OUTPUT) | (g16|g16-v2 MODULE INPUT OUTPUT) | "
                "(g15-origin|g15-step FLUX_OR_CORRECTOR INPUT ANGLES ORIGIN_MODULE ORIGIN_INPUT OUTPUT)");
    const auto module=Read(argv[2],corrector ? 80*1024 : 64*1024);
    RequireHost(Digest(module)==(corrector ? "7f0af4e5aeea143dc1fa1d066a575f8f811512c9241e642272835d3cb25248b9" :
                                flux ? "41d52e58b47e70afca62491a8c76fadb398419275852dd585939ab86615d1826" :
                                pose ? "21452cbf2441d19b0feb386488548bc7d3833d6587e871b631c3df6954ee5086" :
                                g15 ? "0744d45f428c56064089cff52dafa08e189f80bed0eaca817e13fbb4ad069fd0" :
                                g16v2 ? "7e1dae62f752c263223eae9af2c3b4ce56cd36d6611b5ca9dad15ceefd7ce4b1" :
                                    "cc1c655f6ac151fb499c9b3e2894233a4d713482633f4986e08601b03117dc8b"),"PROBE_MODULE_SHA");
    std::vector<std::vector<uint32_t>> input{Read(argv[3],flux ? (8+129*8)*4 : (6+127)*4)};
    const auto &body=input.front();
    const size_t header=flux ? 8 : pose ? 6 : 4;
    const uint32_t faces=g15 && body.size()>=header ? body[flux ? 5 : header-1] : 0;
    RequireHost(g15 ? faces && faces<=(flux ? 129u : 127u) && (!flux || faces>=2u) &&
                      body.size()==header+faces*(flux ? 8u : 1u) : body.size()==13,"PROBE_INPUT_ABI");
    if (origin || corrector) for (uint32_t f=1;f<faces;++f) {
      const auto left=header+(f-1)*8,right=header+f*8;
      RequireHost(std::equal(body.begin()+left+2,body.begin()+left+4,body.begin()+right) &&
                  std::equal(body.begin()+left+6,body.begin()+left+8,body.begin()+right+4),
                  "PROBE_MIDPOINT_COLUMN_MISMATCH");
    }
    constexpr size_t columnBytes=(4+64*4)*4;
    const size_t bytes=origin ? 16+(faces-1)*columnBytes : g15 ? size_t(faces)*(flux ? 64 : pose ? 32 : 24) : g16v2 ? 104 : 36;
    if (g15) {
      input.push_back(Read(argv[4],184328));
      RequireHost(Digest(input.back())=="6278b4a06d704df8439b3291b1c3abff608a42049c4837720e6745a4b272ad61","ANGLE_TABLE_SHA");
    }
    std::vector<uint32_t> originModule;
    if (origin) {
      originModule=Read(argv[5],64*1024);
      RequireHost(Digest(originModule)=="e129edcbaa2380fc5b0a9c69f0cc696eedf6ce6be0ed0c47ca1defb71d0b6516",
                  "ORIGIN_MODULE_SHA");
      const auto originBytes=(faces-1)*columnBytes;
      input.push_back(Read(argv[6],originBytes));
      RequireHost(input.back().size()*4==originBytes,"ORIGIN_INPUT_ABI");
    }
    const std::filesystem::path destination(argv[argc-1]);
    RequireHost(!std::filesystem::exists(destination),"PROBE_OUTPUT_EXISTS");
    HostDevice gpu;
    gpu.Open(false);
    const auto output=gpu.Allocate(bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,true).Readback();
    Commands commands{gpu};
    RequireVk(commands.Run(module,input,output,g15 ? (faces+63)/64 : 1,faces,originModule),"PROBE_RESULT");
    std::ofstream file(destination,std::ios::binary);
    RequireHost(bool(file.write(reinterpret_cast<const char *>(output.words),bytes)),"PROBE_OUTPUT_WRITE");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
