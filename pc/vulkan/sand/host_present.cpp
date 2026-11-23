#include "host_device.h"
#include "../../../entry/src/main/cpp/gpu_resident/program_cache/sha256.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>

namespace grs::pc::sand {
namespace {
#define DRAW_API(F) F(CreateSwapchainKHR) F(DestroySwapchainKHR) F(GetSwapchainImagesKHR) F(AcquireNextImageKHR) \
  F(QueuePresentKHR) F(CreateImageView) F(DestroyImageView) F(CreateRenderPass) F(DestroyRenderPass) \
  F(CreateFramebuffer) F(DestroyFramebuffer) F(CreateDescriptorSetLayout) F(DestroyDescriptorSetLayout) \
  F(CreateDescriptorPool) F(DestroyDescriptorPool) F(AllocateDescriptorSets) F(UpdateDescriptorSets) \
  F(CreatePipelineLayout) F(DestroyPipelineLayout) F(CreateShaderModule) F(DestroyShaderModule) \
  F(CreateGraphicsPipelines) F(DestroyPipeline) F(CreateCommandPool) F(DestroyCommandPool) F(AllocateCommandBuffers) \
  F(BeginCommandBuffer) F(EndCommandBuffer) F(CmdPipelineBarrier) F(CmdBeginRenderPass) F(CmdEndRenderPass) \
  F(CmdBindPipeline) F(CmdBindDescriptorSets) F(CmdBindVertexBuffers) F(CmdPushConstants) F(CmdDraw) \
  F(CreateFence) F(DestroyFence) F(WaitForFences) F(QueueSubmit) F(DeviceWaitIdle)

struct DrawOwner {
  HostDevice &host;
#define F(name) PFN_vk##name name=nullptr;
  DRAW_API(F)
#undef F
  VkSwapchainKHR swapchain=VK_NULL_HANDLE;
  VkDescriptorSetLayout binding=VK_NULL_HANDLE;
  VkDescriptorPool descriptors=VK_NULL_HANDLE;
  VkDescriptorSet set=VK_NULL_HANDLE;
  VkRenderPass render=VK_NULL_HANDLE;
  VkPipelineLayout layout=VK_NULL_HANDLE;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
  std::vector<VkFramebuffer> framebuffers;
  std::vector<VkShaderModule> shaders;
  std::array<VkPipeline,3> pipelines{};
  VkCommandPool pool=VK_NULL_HANDLE;
  VkFence acquired=VK_NULL_HANDLE,drawn=VK_NULL_HANDLE;
  VkCommandBuffer command=VK_NULL_HANDLE;
  VkExtent2D extent{};
  VkFormat format=VK_FORMAT_UNDEFINED;
  bool submitted=false,acquireIssued=false;

  explicit DrawOwner(HostDevice &g) : host(g) {}
  ~DrawOwner() {
    const auto d=host.device;
    if (submitted || acquireIssued) DeviceWaitIdle(d);
    for (auto p:pipelines) if (p) DestroyPipeline(d,p,nullptr);
    for (auto m:shaders) DestroyShaderModule(d,m,nullptr);
    if (pool) DestroyCommandPool(d,pool,nullptr);
    if (acquired) DestroyFence(d,acquired,nullptr);
    if (drawn) DestroyFence(d,drawn,nullptr);
    if (descriptors) DestroyDescriptorPool(d,descriptors,nullptr);
    if (layout) DestroyPipelineLayout(d,layout,nullptr);
    if (binding) DestroyDescriptorSetLayout(d,binding,nullptr);
    for (auto f:framebuffers) DestroyFramebuffer(d,f,nullptr);
    for (auto v:views) DestroyImageView(d,v,nullptr);
    if (render) DestroyRenderPass(d,render,nullptr);
    if (swapchain) DestroySwapchainKHR(d,swapchain,nullptr);
  }

  void Setup(const Slice &state) {
    const auto device=host.device;
#define F(name) name=reinterpret_cast<PFN_vk##name>(host.deviceProc(device,"vk" #name)); \
                RequireHost(name!=nullptr,"VK_DRAW_PROC_" #name);
    DRAW_API(F)
#undef F
#define I(name) reinterpret_cast<PFN_vk##name>(host.instanceProc(host.instance,"vk" #name))
    const auto capabilities=I(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    const auto formats=I(GetPhysicalDeviceSurfaceFormatsKHR);
#undef I
    RequireHost(capabilities && formats,"VK_SURFACE_FUNCTIONS");
    VkSurfaceCapabilitiesKHR caps{};
    RequireVk(capabilities(host.physical,host.surface,&caps),"SURFACE_CAPS");
    uint32_t count=0;
    RequireVk(formats(host.physical,host.surface,&count,nullptr),"SURFACE_FORMAT_COUNT");
    std::vector<VkSurfaceFormatKHR> choices(count);
    RequireVk(formats(host.physical,host.surface,&count,choices.data()),"SURFACE_FORMATS");
    const auto chosen=std::find_if(choices.begin(),choices.end(),[](const auto &f) {
      return (f.format==VK_FORMAT_R8G8B8A8_UNORM || f.format==VK_FORMAT_B8G8R8A8_UNORM ||
              f.format==VK_FORMAT_R8G8B8A8_SRGB || f.format==VK_FORMAT_B8G8R8A8_SRGB) &&
             f.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    RequireHost(chosen!=choices.end() && (caps.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
                "SURFACE_RGBA8_COLOR");
    format=chosen->format;
    RECT client;
    RequireHost(GetClientRect(host.window,&client) && client.right>0 && client.bottom>0 && host.Current(),
                "CURRENT_WINDOW_EXTENT");
    extent=caps.currentExtent;
    if (extent.width==UINT32_MAX)
      extent={std::clamp(uint32_t(client.right),caps.minImageExtent.width,caps.maxImageExtent.width),
              std::clamp(uint32_t(client.bottom),caps.minImageExtent.height,caps.maxImageExtent.height)};
    const uint32_t modeMask=caps.supportedCompositeAlpha;
    const auto opacity=static_cast<VkCompositeAlphaFlagBitsKHR>(modeMask&(~modeMask+1));
    VkSwapchainCreateInfoKHR swap{};
    swap.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap.surface=host.surface;
    swap.minImageCount=std::max(caps.minImageCount,2u);
    if (caps.maxImageCount) swap.minImageCount=std::min(swap.minImageCount,caps.maxImageCount);
    swap.imageFormat=format;
    swap.imageColorSpace=chosen->colorSpace;
    swap.imageExtent=extent;
    swap.imageArrayLayers=1;
    swap.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    swap.preTransform=caps.currentTransform;
    swap.compositeAlpha=opacity;
    swap.presentMode=VK_PRESENT_MODE_FIFO_KHR;
    swap.clipped=VK_TRUE;
    RequireVk(CreateSwapchainKHR(device,&swap,nullptr,&swapchain),"CREATE_R1_SWAPCHAIN");
    RequireVk(GetSwapchainImagesKHR(device,swapchain,&count,nullptr),"R1_IMAGE_COUNT");
    images.resize(count);
    RequireVk(GetSwapchainImagesKHR(device,swapchain,&count,images.data()),"R1_IMAGES");
    VkAttachmentDescription attachment{};
    attachment.format=format;
    attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount=1;
    sub.pColorAttachments=&color;
    VkRenderPassCreateInfo renderInfo{};
    renderInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderInfo.attachmentCount=renderInfo.subpassCount=1;
    renderInfo.pAttachments=&attachment;
    renderInfo.pSubpasses=&sub;
    RequireVk(CreateRenderPass(device,&renderInfo,nullptr,&render),"CREATE_R1_RENDER_PASS");
    views.reserve(count);
    framebuffers.reserve(count);
    for (auto image:images) {
      VkImageViewCreateInfo view{};
      view.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view.image=image;
      view.viewType=VK_IMAGE_VIEW_TYPE_2D;
      view.format=format;
      view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
      VkImageView made=VK_NULL_HANDLE;
      RequireVk(CreateImageView(device,&view,nullptr,&made),"CREATE_R1_IMAGE_VIEW");
      views.push_back(made);
      VkFramebufferCreateInfo framebuffer{};
      framebuffer.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer.renderPass=render;
      framebuffer.attachmentCount=1;
      framebuffer.pAttachments=&views.back();
      framebuffer.width=extent.width;
      framebuffer.height=extent.height;
      framebuffer.layers=1;
      VkFramebuffer target=VK_NULL_HANDLE;
      RequireVk(CreateFramebuffer(device,&framebuffer,nullptr,&target),"CREATE_R1_FRAMEBUFFER");
      framebuffers.push_back(target);
    }
    const VkDescriptorSetLayoutBinding slot{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo bindingInfo{};
    bindingInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    bindingInfo.bindingCount=1;
    bindingInfo.pBindings=&slot;
    RequireVk(CreateDescriptorSetLayout(device,&bindingInfo,nullptr,&binding),"CREATE_R1_SCENE_BINDING");
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets=poolInfo.poolSizeCount=1;
    poolInfo.pPoolSizes=&size;
    RequireVk(CreateDescriptorPool(device,&poolInfo,nullptr,&descriptors),"CREATE_R1_SCENE_DESCRIPTORS");
    const VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,nullptr,descriptors,1,&binding};
    RequireVk(AllocateDescriptorSets(device,&allocate,&set),"ALLOCATE_R1_SCENE_BINDING");
    const VkDescriptorBufferInfo range{state.buffer,state.offset,state.bytes};
    VkWriteDescriptorSet write{};
    write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet=set;
    write.descriptorCount=1;
    write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo=&range;
    UpdateDescriptorSets(device,1,&write,0,nullptr);
    const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(R1ViewPush)};
    const VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,nullptr,0,1,&binding,1,&push};
    RequireVk(CreatePipelineLayout(device,&layoutInfo,nullptr,&layout),"CREATE_R1_GRAPHICS_LAYOUT");
    VkCommandPoolCreateInfo cmdPool{};
    cmdPool.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPool.queueFamilyIndex=host.family;
    RequireVk(CreateCommandPool(device,&cmdPool,nullptr,&pool),"CREATE_R1_DRAW_POOL");
    const VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,nullptr,pool,
                                                   VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
    RequireVk(AllocateCommandBuffers(device,&commandInfo,&command),"ALLOC_R1_DRAW");
    const VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,nullptr,0};
    RequireVk(CreateFence(device,&fenceInfo,nullptr,&acquired),"CREATE_R1_ACQUIRE_FENCE");
    RequireVk(CreateFence(device,&fenceInfo,nullptr,&drawn),"CREATE_R1_GRAPHICS_FENCE");
  }

  VkShaderModule Load(const std::filesystem::path &dir,std::string_view group,std::string_view stage) {
    const auto row=std::find_if(std::begin(kGraphicsModules),std::end(kGraphicsModules),[&](const auto &m) {
      return group==m.group && stage==m.stage;
    });
    RequireHost(row!=std::end(kGraphicsModules) && row->bytes%4==0,"R1_DRAW_MODULE_MISSING");
    std::vector<uint32_t> words(row->bytes/4);
    std::ifstream raw(dir/row->name,std::ios::binary);
    RequireHost(bool(raw.read(reinterpret_cast<char*>(words.data()),row->bytes)) &&
                raw.peek()==std::char_traits<char>::eof(),"R1_DRAW_MODULE_LENGTH");
    const auto sha=accept647::HashWords(words.data(),words.size());
    RequireHost(sha==g11vis::HexDigest(row->sha),"R1_DRAW_MODULE_SHA");
    VkShaderModule result=VK_NULL_HANDLE;
    const VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,nullptr,0,row->bytes,words.data()};
    RequireVk(CreateShaderModule(host.device,&info,nullptr,&result),"CREATE_R1_SCENE_SHADER");
    shaders.push_back(result);
    return result;
  }

  void Pipelines(const std::filesystem::path &directory) {
    const VkViewport viewport{0.f,float(extent.height),float(extent.width),-float(extent.height),0.f,1.f};
    const VkRect2D screen{{0,0},extent};
    VkPipelineViewportStateCreateInfo view{};
    view.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    view.viewportCount=view.scissorCount=1;
    view.pViewports=&viewport;
    view.pScissors=&screen;
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode=VK_POLYGON_MODE_FILL;
    raster.cullMode=VK_CULL_MODE_NONE;
    raster.lineWidth=1.f;
    VkPipelineMultisampleStateCreateInfo samples{};
    samples.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    const VkVertexInputBindingDescription packets[]={{0,256,VK_VERTEX_INPUT_RATE_INSTANCE},
                                                       {1,16,VK_VERTEX_INPUT_RATE_INSTANCE}};
    const VkVertexInputAttributeDescription attrs[]={{0,0,VK_FORMAT_R32_UINT,0},{1,0,VK_FORMAT_R32_UINT,4},
      {2,0,VK_FORMAT_R32_UINT,8},{3,1,VK_FORMAT_R32G32B32A32_UINT,0},{4,0,VK_FORMAT_R32_UINT,24},
      {5,0,VK_FORMAT_R32_UINT,28}};
    const std::array<const char*,3> names{"base","glyph","packet"};
    for (size_t i=0;i<names.size();++i) {
      VkPipelineShaderStageCreateInfo stages[2]{};
      for (size_t s=0;s<2;++s) {
        stages[s].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[s].stage=s ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
        stages[s].module=Load(directory,names[i],s ? "frag" : "vert");
        stages[s].pName="main";
      }
      VkPipelineVertexInputStateCreateInfo vertex{};
      vertex.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      if (i==1) {
        vertex.vertexBindingDescriptionCount=1;
        vertex.pVertexBindingDescriptions=&kR1GlyphBinding;
        vertex.vertexAttributeDescriptionCount=uint32_t(kR1GlyphAttributes.size());
        vertex.pVertexAttributeDescriptions=kR1GlyphAttributes.data();
      } else if (i==2) {
        vertex.vertexBindingDescriptionCount=2;
        vertex.pVertexBindingDescriptions=packets;
        vertex.vertexAttributeDescriptionCount=6;
        vertex.pVertexAttributeDescriptions=attrs;
      }
      VkPipelineColorBlendAttachmentState alpha{};
      alpha.blendEnable=VkBool32(i!=0);
      alpha.srcColorBlendFactor=alpha.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
      alpha.dstColorBlendFactor=alpha.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      alpha.colorBlendOp=alpha.alphaBlendOp=VK_BLEND_OP_ADD;
      alpha.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
      VkPipelineColorBlendStateCreateInfo blend{};
      blend.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      blend.attachmentCount=1;
      blend.pAttachments=&alpha;
      VkGraphicsPipelineCreateInfo create{};
      create.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      create.stageCount=2;
      create.pStages=stages;
      create.pVertexInputState=&vertex;
      create.pInputAssemblyState=&assembly;
      create.pViewportState=&view;
      create.pRasterizationState=&raster;
      create.pMultisampleState=&samples;
      create.pColorBlendState=&blend;
      create.layout=layout;
      create.renderPass=render;
      RequireVk(CreateGraphicsPipelines(host.device,VK_NULL_HANDLE,1,&create,nullptr,&pipelines[i]),
                "CREATE_R1_GRAPHICS_PIPELINE");
    }
  }
};
#undef DRAW_API
}

PresentedMapping PresentR1(HostDevice &gpu,const std::filesystem::path &directory,
                           const VulkanOwner::AcceptedProof &proof,const R1Scene &scene) {
  DrawOwner draw(gpu);
  draw.Setup(scene.state);
  draw.Pipelines(directory);
  R1SceneGate lease;
  RequireHost(gpu.Current() && lease.Acquire(proof,scene,scene.key),"R1_JOINT_LEASE");
  PresentReceipt receipt{};
  receipt.imageIndex=UINT32_MAX;
  receipt.acquire=draw.AcquireNextImageKHR(gpu.device,draw.swapchain,250000000,VK_NULL_HANDLE,draw.acquired,
                                         &receipt.imageIndex);
  RequireHost((receipt.acquire==VK_SUCCESS || receipt.acquire==VK_SUBOPTIMAL_KHR) &&
              receipt.imageIndex<draw.framebuffers.size(),"R1_ACQUIRE");
  draw.acquireIssued=true;
  RequireVk(draw.WaitForFences(gpu.device,1,&draw.acquired,VK_TRUE,250000000),"R1_WAIT_ACQUIRE");
  const VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,
                                        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,nullptr};
  RequireVk(draw.BeginCommandBuffer(draw.command,&begin),"R1_BEGIN_DRAW");
  VkMemoryBarrier visible{};
  visible.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  visible.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
  visible.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  draw.CmdPipelineBarrier(draw.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,
                          1,&visible,0,nullptr,0,nullptr);
  const VkClearValue clear{{{0.f,0.f,0.f,1.f}}};
  const VkRenderPassBeginInfo start{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,nullptr,draw.render,
    draw.framebuffers[receipt.imageIndex],{{0,0},draw.extent},1,&clear};
  draw.CmdBeginRenderPass(draw.command,&start,VK_SUBPASS_CONTENTS_INLINE);
  draw.CmdBindDescriptorSets(draw.command,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.layout,0,1,&draw.set,0,nullptr);
  const auto width=float(draw.extent.width),height=float(draw.extent.height);
  const auto w=std::min(width,height*.5f),h=w*2.f;
  R1ViewPush push{{width,height},{0,0},{(width-w)*.5f,(height-h)*.5f,w,h},128,0};
  const auto layer=[&](unsigned i,uint32_t vertices,uint32_t instances) {
    draw.CmdBindPipeline(draw.command,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.pipelines[i]);
    draw.CmdPushConstants(draw.command,draw.layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),&push);
    draw.CmdDraw(draw.command,vertices,instances,0,0);
  };
  layer(0,3,1);
  RequireHost(lease.BindGlyphs(scene.key,draw.CmdBindVertexBuffers,draw.command),"R1_GLYPH_BIND");
  layer(1,6,2048);
  const VkBuffer packet[]={scene.state.buffer,scene.styles.buffer};
  const VkDeviceSize offsets[]={scene.state.offset+g9spf::kProfiles[0].state[6].begin,scene.styles.offset};
  draw.CmdBindVertexBuffers(draw.command,0,2,packet,offsets);
  layer(2,6,192);
  push.foreground=1;
  layer(0,3,1);
  draw.CmdEndRenderPass(draw.command);
  RequireVk(draw.EndCommandBuffer(draw.command),"R1_END_DRAW");
  RequireHost(gpu.Current() && lease.CanPresent(scene.key),"R1_STALE_BEFORE_DRAW");
  VkSubmitInfo submit{};
  submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&draw.command;
  receipt.submit=draw.QueueSubmit(gpu.queue,1,&submit,draw.drawn);
  RequireVk(receipt.submit,"R1_QUEUE_DRAW");
  draw.submitted=true;
  RequireHost(lease.Submitted(scene.key,draw.drawn),"R1_DRAW_LEASE");
  receipt.fenceObject=draw.drawn;
  receipt.fence=draw.WaitForFences(gpu.device,1,&draw.drawn,VK_TRUE,250000000);
  RequireVk(receipt.fence,"R1_WAIT_GRAPHICS_FENCE");
  RequireHost(gpu.Current() && lease.CanPresent(scene.key),"R1_STALE_BEFORE_PRESENT");
  const VkPresentInfoKHR presentation{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,nullptr,0,nullptr,1,&draw.swapchain,
                                       &receipt.imageIndex,&receipt.imagePresent};
  receipt.enterQpc=grs::pc::QpcTicks();
  receipt.present=draw.QueuePresentKHR(gpu.queue,&presentation);
  receipt.exitQpc=grs::pc::QpcTicks();
  std::cout << "{\"stage\":\"R1_WSI\",\"nonce\":\"" << scene.nonce << "\",\"geometry\":\"" << scene.key.geometry
            << "\",\"fence\":\"" << reinterpret_cast<uintptr_t>(draw.drawn)
            << "\",\"acquire\":" << int(receipt.acquire) << ",\"submit\":" << int(receipt.submit)
            << ",\"fenceResult\":" << int(receipt.fence) << ",\"present\":" << int(receipt.present)
            << ",\"image\":" << int(receipt.imagePresent) << ",\"width\":" << draw.extent.width
            << ",\"height\":" << draw.extent.height << ",\"format\":" << int(draw.format) << "}\n";
  RequireHost(gpu.Current(),"R1_STALE_AFTER_PRESENT");
  const auto mapping=lease.Presented(scene.key,receipt);
  RequireHost(mapping.has_value(),"R1_PRESENT_MAPPING");
  RequireVk(lease.Retire(draw.WaitForFences,gpu.device,0),"R1_GRAPHICS_LEASE_RETIRE");
  const auto visibleUntil=HostDevice::Now()+1000000000;
  while (gpu.Current() && HostDevice::Now()<visibleUntil) Sleep(16);
  return *mapping;
}
}
