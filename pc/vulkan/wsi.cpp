#include <functional>
#include "wsi.h"
#include "pc_view_spirv.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace grs::pc {
namespace {
struct Stop { const char *stage; VkResult code; };
void Check(VkResult code, const char *stage) {
  if (code != VK_SUCCESS) throw Stop{stage, code};
}

#define PC_INSTANCE(F) F(GetPhysicalDeviceSurfaceCapabilitiesKHR) F(GetPhysicalDeviceSurfaceFormatsKHR)
#define PC_DEVICE(F) \
  F(CreateSwapchainKHR) F(DestroySwapchainKHR) F(GetSwapchainImagesKHR) F(AcquireNextImageKHR) F(QueuePresentKHR) \
  F(CreateImageView) F(DestroyImageView) F(CreateRenderPass) F(DestroyRenderPass) \
  F(CreateFramebuffer) F(DestroyFramebuffer) F(CreateGraphicsPipelines) \
  F(CmdBeginRenderPass) F(CmdEndRenderPass) F(CmdDraw) F(CmdCopyImageToBuffer)

struct Presentation {
  g13vk::VulkanApi &vk;
  VkDevice device;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImageView> views;
  VkRenderPass pass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers;
  VkDescriptorSetLayout descriptors = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkShaderModule vertex = VK_NULL_HANDLE, fragment = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkFence acquired = VK_NULL_HANDLE, submitted = VK_NULL_HANDLE;
  bool inFlight = false;

#define DECLARE(n) PFN_vk##n n = nullptr;
  PC_INSTANCE(DECLARE)
  PC_DEVICE(DECLARE)
#undef DECLARE

  void Load(VkInstance instance) {
#define LOAD(n) n = reinterpret_cast<PFN_vk##n>(vk.GetInstanceProcAddr(instance, "vk" #n)); \
    if (!n) throw Stop{"MISSING_vk" #n, VK_ERROR_EXTENSION_NOT_PRESENT};
    PC_INSTANCE(LOAD)
#undef LOAD
#define LOAD(n) n = reinterpret_cast<PFN_vk##n>(vk.GetDeviceProcAddr(device, "vk" #n)); \
    if (!n) throw Stop{"MISSING_vk" #n, VK_ERROR_EXTENSION_NOT_PRESENT};
    PC_DEVICE(LOAD)
#undef LOAD
  }

  ~Presentation() {
    if (inFlight) vk.DeviceWaitIdle(device);
    if (submitted) vk.DestroyFence(device, submitted, nullptr);
    if (acquired) vk.DestroyFence(device, acquired, nullptr);
    if (commandPool) vk.DestroyCommandPool(device, commandPool, nullptr);
    if (pipeline) vk.DestroyPipeline(device, pipeline, nullptr);
    if (fragment) vk.DestroyShaderModule(device, fragment, nullptr);
    if (vertex) vk.DestroyShaderModule(device, vertex, nullptr);
    if (layout) vk.DestroyPipelineLayout(device, layout, nullptr);
    if (descriptorPool) vk.DestroyDescriptorPool(device, descriptorPool, nullptr);
    if (descriptors) vk.DestroyDescriptorSetLayout(device, descriptors, nullptr);
    for (auto framebuffer : framebuffers)
      if (framebuffer) DestroyFramebuffer(device, framebuffer, nullptr);
    if (pass) DestroyRenderPass(device, pass, nullptr);
    for (auto view : views)
      if (view) DestroyImageView(device, view, nullptr);
    if (swapchain) DestroySwapchainKHR(device, swapchain, nullptr);
  }
};
}

Frame PresentBoot(g13vk::VulkanApi &vk, VkInstance instance, VkPhysicalDevice physical, VkDevice device,
                  VkQueue queue, uint32_t family, VkSurfaceKHR surface, VkBuffer control, VkBuffer staging,
                  VkDeviceMemory stagingMemory, VkMemoryPropertyFlags memoryFlags,
                  HWND window, const std::function<bool()> &current,
                  const std::function<void(const Frame &)> &published,
                  VkDeviceSize m1PublicationBytes, bool requireDiagnosticPixel,
                  const std::function<VkBuffer()> &nextPublication, bool fullM1,
                  const PresentBusy &captureBusy) {
  using BusyPhase=g13vk::M1HotCapture::Phase;
  Frame frame;
  const bool dryScene=m1PublicationBytes && !requireDiagnosticPixel;
  Presentation wsi{vk, device};
  try {
    wsi.Load(instance);
    VkSurfaceCapabilitiesKHR caps{};
    Check(wsi.GetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    constexpr auto usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if ((caps.supportedUsageFlags & usage) != usage)
      throw Stop{"SURFACE_COLOR_READBACK_UNSUPPORTED", VK_ERROR_FORMAT_NOT_SUPPORTED};
    uint32_t count = 0;
    Check(wsi.GetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
    if (!count || count > 4096) throw Stop{"SURFACE_NO_FORMATS", VK_ERROR_FORMAT_NOT_SUPPORTED};
    std::vector<VkSurfaceFormatKHR> formats(count);
    Check(wsi.GetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
    formats.resize(count);
    const auto found = std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR &f) {
      return f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
             (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM ||
              f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB);
    });
    if (found == formats.end()) throw Stop{"SURFACE_RGBA8_UNAVAILABLE", VK_ERROR_FORMAT_NOT_SUPPORTED};
    VkExtent2D extent = caps.currentExtent;
    RECT rect{};
    if (!GetClientRect(window, &rect) || rect.right <= rect.left || rect.bottom <= rect.top || !current()) {
      frame.stale = true;
      throw Stop{"WINDOW_CHANGED_BEFORE_CREATE", VK_ERROR_OUT_OF_DATE_KHR};
    }
    if (extent.width == UINT32_MAX) {
      extent.width = std::clamp(uint32_t(rect.right - rect.left), caps.minImageExtent.width, caps.maxImageExtent.width);
      extent.height = std::clamp(uint32_t(rect.bottom - rect.top), caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if (!extent.width || !extent.height) throw Stop{"SURFACE_ZERO_EXTENT", VK_ERROR_OUT_OF_DATE_KHR};
    frame.width = extent.width;
    frame.height = extent.height;
    frame.format = found->format;
    VkSwapchainCreateInfoKHR chain{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    chain.surface = surface;
    chain.minImageCount = caps.minImageCount;
    chain.imageFormat = found->format;
    chain.imageColorSpace = found->colorSpace;
    chain.imageExtent = extent;
    chain.imageArrayLayers = 1;
    chain.imageUsage = usage;
    chain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    chain.preTransform = caps.currentTransform;
    const uint32_t alpha = caps.supportedCompositeAlpha;
    if (!alpha) throw Stop{"SURFACE_NO_COMPOSITE_ALPHA", VK_ERROR_INITIALIZATION_FAILED};
    chain.compositeAlpha = VkCompositeAlphaFlagBitsKHR(alpha & (0u - alpha));
    chain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    chain.clipped = VK_TRUE;
    Check(wsi.CreateSwapchainKHR(device, &chain, nullptr, &wsi.swapchain), "vkCreateSwapchainKHR");
    Check(wsi.GetSwapchainImagesKHR(device, wsi.swapchain, &count, nullptr), "vkGetSwapchainImagesKHR");
    if (!count || count > 4096) throw Stop{"SWAPCHAIN_NO_IMAGES", VK_ERROR_INITIALIZATION_FAILED};
    std::vector<VkImage> images(count);
    Check(wsi.GetSwapchainImagesKHR(device, wsi.swapchain, &count, images.data()), "vkGetSwapchainImagesKHR");
    frame.images = count;

    VkAttachmentDescription color{};
    color.format = found->format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 1;
    pass.pAttachments = &color;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    Check(wsi.CreateRenderPass(device, &pass, nullptr, &wsi.pass), "vkCreateRenderPass");

    VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorSetLayoutCreateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ds.bindingCount = 1;
    ds.pBindings = &binding;
    Check(vk.CreateDescriptorSetLayout(device, &ds, nullptr, &wsi.descriptors), "vkCreateDescriptorSetLayout.FRAGMENT");
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
    VkDescriptorPoolCreateInfo descPool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descPool.maxSets = 1;
    descPool.poolSizeCount = 1;
    descPool.pPoolSizes = &poolSize;
    Check(vk.CreateDescriptorPool(device, &descPool, nullptr, &wsi.descriptorPool), "vkCreateDescriptorPool.FRAGMENT");
    VkDescriptorSet set{};
    VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    setInfo.descriptorPool = wsi.descriptorPool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &wsi.descriptors;
    Check(vk.AllocateDescriptorSets(device, &setInfo, &set), "vkAllocateDescriptorSets.FRAGMENT");
    VkDescriptorBufferInfo range{control, 0, m1PublicationBytes ? m1PublicationBytes : 18688 * sizeof(uint32_t)};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &range;
    vk.UpdateDescriptorSets(device, 1, &write, 0, nullptr);
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &wsi.descriptors;
    const VkPushConstantRange extentPush{VK_SHADER_STAGE_FRAGMENT_BIT,0,12};
    if (dryScene) {
      layout.pushConstantRangeCount = 1;
      layout.pPushConstantRanges = &extentPush;
    }
    Check(vk.CreatePipelineLayout(device, &layout, nullptr, &wsi.layout), "vkCreatePipelineLayout.GRAPHICS");

    auto shader = [&](const auto &words, VkShaderModule &module) {
      VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
      info.codeSize = sizeof(words);
      info.pCode = words;
      Check(vk.CreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule.GRAPHICS");
    };
    shader(kBootViewVertex, wsi.vertex);
    if (dryScene) {
      if (fullM1) shader(kM1FullSceneFragment, wsi.fragment);
      else shader(kM1DrySceneFragment, wsi.fragment);
    } else if (m1PublicationBytes)
      shader(kM1SourceViewFragment, wsi.fragment);
    else
      shader(kBootViewFragment, wsi.fragment);
    VkPipelineShaderStageCreateInfo stages[]{
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, wsi.vertex, "main"},
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, wsi.fragment, "main"}};
    VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0, 0, float(extent.width), float(extent.height), 0, 1};
    VkRect2D scissor{{0,0}, extent};
    VkPipelineViewportStateCreateInfo viewportInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState channels{};
    channels.colorWriteMask = 15;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &channels;
    VkGraphicsPipelineCreateInfo graphics{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    graphics.stageCount = 2;
    graphics.pStages = stages;
    graphics.pVertexInputState = &vertex;
    graphics.pInputAssemblyState = &assembly;
    graphics.pViewportState = &viewportInfo;
    graphics.pRasterizationState = &raster;
    graphics.pMultisampleState = &samples;
    graphics.pColorBlendState = &blend;
    graphics.layout = wsi.layout;
    graphics.renderPass = wsi.pass;
    Check(wsi.CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics, nullptr, &wsi.pipeline), "vkCreateGraphicsPipelines");

    const VkImageSubresourceRange subresource{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    wsi.views.resize(images.size());
    wsi.framebuffers.resize(images.size());
    for (size_t i=0; i<images.size(); ++i) {
      VkImageViewCreateInfo imageView{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      imageView.image = images[i];
      imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageView.format = found->format;
      imageView.subresourceRange = subresource;
      Check(wsi.CreateImageView(device,&imageView,nullptr,&wsi.views[i]),"vkCreateImageView");
      VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      fb.renderPass = wsi.pass;
      fb.attachmentCount = 1;
      fb.pAttachments = &wsi.views[i];
      fb.width = extent.width;
      fb.height = extent.height;
      fb.layers = 1;
      Check(wsi.CreateFramebuffer(device,&fb,nullptr,&wsi.framebuffers[i]),"vkCreateFramebuffer");
    }
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    Check(vk.CreateFence(device,&fence,nullptr,&wsi.acquired),"vkCreateFence.ACQUIRE");
    Check(vk.CreateFence(device,&fence,nullptr,&wsi.submitted),"vkCreateFence.GRAPHICS");
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.queueFamilyIndex = family;
    Check(vk.CreateCommandPool(device, &pool, nullptr, &wsi.commandPool), "vkCreateCommandPool.GRAPHICS");
    VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocation.commandPool = wsi.commandPool;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    VkCommandBuffer command{};
    Check(vk.AllocateCommandBuffers(device, &allocation, &command), "vkAllocateCommandBuffers.GRAPHICS");
    auto resetPool = reinterpret_cast<PFN_vkResetCommandPool>(vk.GetDeviceProcAddr(device,"vkResetCommandPool"));
    auto resetFences = reinterpret_cast<PFN_vkResetFences>(vk.GetDeviceProcAddr(device,"vkResetFences"));
    if (nextPublication && (!resetPool || !resetFences))
      throw Stop{"MISSING_WSI_RESET_FUNCTIONS",VK_ERROR_INITIALIZATION_FAILED};
    for (;;) {
      if (!current()) {
        frame.stale = true;
        throw Stop{"WINDOW_CHANGED_BEFORE_ACQUIRE",VK_ERROR_OUT_OF_DATE_KHR};
      }
      frame.acquire = wsi.AcquireNextImageKHR(device,wsi.swapchain,1000000000,VK_NULL_HANDLE,
                                             wsi.acquired,&frame.imageIndex);
      if (frame.acquire!=VK_SUBOPTIMAL_KHR) Check(frame.acquire,"vkAcquireNextImageKHR");
      Check(vk.WaitForFences(device,1,&wsi.acquired,VK_TRUE,UINT64_MAX),"vkWaitForFences.ACQUIRE");
      const VkImage image=images.at(frame.imageIndex);
      VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      Check(vk.BeginCommandBuffer(command, &begin), "vkBeginCommandBuffer.GRAPHICS");
      VkMemoryBarrier compute{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                              VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT};
      vk.CmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 1, &compute, 0, nullptr, 0, nullptr);
      VkRenderPassBeginInfo draw{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      draw.renderPass = wsi.pass;
      draw.framebuffer = wsi.framebuffers[frame.imageIndex];
      draw.renderArea = scissor;
      wsi.CmdBeginRenderPass(command, &draw, VK_SUBPASS_CONTENTS_INLINE);
      vk.CmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, wsi.pipeline);
      vk.CmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, wsi.layout, 0, 1, &set, 0, nullptr);
      if (dryScene) {
        const bool unorm=found->format==VK_FORMAT_B8G8R8A8_UNORM || found->format==VK_FORMAT_R8G8B8A8_UNORM;
        struct FramePush { float width,height; uint32_t encode; };
        const FramePush values{float(extent.width),float(extent.height),uint32_t(unorm)};
        static_assert(sizeof(values)==12);
        vk.CmdPushConstants(command,wsi.layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(values),&values);
      }
      wsi.CmdDraw(command, 3, 1, 0, 0);
      wsi.CmdEndRenderPass(command);

      VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
      imageBarrier.srcQueueFamilyIndex = imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageBarrier.image = image;
      imageBarrier.subresourceRange = subresource;
      imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      vk.CmdPipelineBarrier(command, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
      VkBufferImageCopy pixel{};
      pixel.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      pixel.imageOffset = {int32_t(m1PublicationBytes ? uint64_t(extent.width)*129/256 : extent.width/2),
                           int32_t(m1PublicationBytes ? extent.height-1 : extent.height/2), 0};
      pixel.imageExtent = {1,1,1};
      wsi.CmdCopyImageToBuffer(command, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1, &pixel);
      imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imageBarrier.dstAccessMask = 0;
      VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT};
      vk.CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            0, 1, &host, 0, nullptr, 1, &imageBarrier);
      Check(vk.EndCommandBuffer(command), "vkEndCommandBuffer.GRAPHICS");
      if (!current()) {
        frame.stale = true;
        throw Stop{"WINDOW_CHANGED_BEFORE_SUBMIT", VK_ERROR_OUT_OF_DATE_KHR};
      }
      VkSubmitInfo submission{VK_STRUCTURE_TYPE_SUBMIT_INFO};
      submission.commandBufferCount = 1;
      submission.pCommandBuffers = &command;
      frame.submit = vk.QueueSubmit(queue, 1, &submission, wsi.submitted);
      Check(frame.submit, "vkQueueSubmit.GRAPHICS");
      wsi.inFlight = true;
      if (captureBusy) captureBusy(BusyPhase::GraphicsFence,true,false);
      frame.fence = vk.WaitForFences(device, 1, &wsi.submitted, VK_TRUE, UINT64_MAX);
      if (captureBusy) captureBusy(BusyPhase::GraphicsFence,false,frame.fence==VK_SUCCESS && current());
      Check(frame.fence, "vkWaitForFences.GRAPHICS");
      void *mapped = nullptr;
      Check(vk.MapMemory(device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory.PIXEL");
      VkResult invalidation = VK_SUCCESS;
      if (!(memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, stagingMemory, 0, VK_WHOLE_SIZE};
        invalidation = vk.InvalidateMappedMemoryRanges(device, 1, &range);
      }
      if (invalidation == VK_SUCCESS) std::memcpy(&frame.pixel, mapped, sizeof(frame.pixel));
      vk.UnmapMemory(device, stagingMemory);
      Check(invalidation, "vkInvalidateMappedMemoryRanges.PIXEL");
      frame.pixelRead = true;
      if (requireDiagnosticPixel && frame.pixel != 0xff00ff00u)
        throw Stop{m1PublicationBytes ? "M1_SOURCE_PIXEL_MISMATCH" : "BOOT_FRAGMENT_PIXEL_MISMATCH", VK_ERROR_UNKNOWN};
      if (!current()) {
        frame.stale = true;
        throw Stop{"WINDOW_CHANGED_BEFORE_PRESENT", VK_ERROR_OUT_OF_DATE_KHR};
      }
      // This diagnostic submits once per generation. The host waits for the
      // graphics fence before present, so the WSI owns no application semaphore.
      VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
      present.swapchainCount = 1;
      present.pSwapchains = &wsi.swapchain;
      present.pImageIndices = &frame.imageIndex;
      present.pResults = &frame.imagePresent;
      if (captureBusy) captureBusy(BusyPhase::Present,true,false);
      frame.presentEnter = QpcTicks();
      frame.present = wsi.QueuePresentKHR(queue, &present);
      frame.presentExit = QpcTicks();
      if (captureBusy) {
        const auto usable=[](VkResult result) { return result==VK_SUCCESS || result==VK_SUBOPTIMAL_KHR; };
        captureBusy(BusyPhase::Present,false,usable(frame.present) && usable(frame.imagePresent) && current());
      }
      wsi.inFlight = wsi.inFlight || frame.present == VK_SUCCESS || frame.present == VK_SUBOPTIMAL_KHR;
      if (frame.present != VK_SUBOPTIMAL_KHR) Check(frame.present, "vkQueuePresentKHR");
      if (frame.imagePresent != VK_SUBOPTIMAL_KHR) Check(frame.imagePresent, "PRESENT_IMAGE_RESULT");
      frame.stage = nextPublication ? "M1_PRESENT_QUEUED" : "DIAGNOSTIC_PRESENT_QUEUED";
      published(frame);
      if (!nextPublication) {
        while (current()) Sleep(16);
        break;
      }
      if (!current()) break;
      const auto next=nextPublication();
      if (!next || !current()) break;
      Check(resetPool(device,wsi.commandPool,0),"vkResetCommandPool.GRAPHICS");
      const VkFence fences[]={wsi.acquired,wsi.submitted};
      Check(resetFences(device,2,fences),"vkResetFences.GRAPHICS");
      range.buffer=next;
      vk.UpdateDescriptorSets(device,1,&write,0,nullptr);
    }
    frame.retire = vk.DeviceWaitIdle(device);
    if (frame.retire == VK_SUCCESS || frame.retire == VK_ERROR_DEVICE_LOST) wsi.inFlight = false;
    Check(frame.retire, "vkDeviceWaitIdle.PRESENT_RETIRE");
  } catch (const Stop &failure) {
    frame.stage = failure.stage;
    frame.error = failure.code;
    if (failure.code == VK_ERROR_OUT_OF_DATE_KHR) frame.stale = true;
  }
  return frame;
}
}
