#pragma once
#ifdef _WIN32
#include "platform.h"
#define GRS_CREATE_SURFACE(F) F(CreateWin32SurfaceKHR)
#else
#include <vulkan/vulkan.h>
#define GRS_CREATE_SURFACE(F) F(CreateSurfaceOHOS)
#endif

// Resolved from the actual device loader so that missing entry points are
// reported by the isolated probe instead of preventing the N-API module load.
#define T742_INSTANCE_API(F)                                                    \
  F(DestroyInstance) GRS_CREATE_SURFACE(F) F(DestroySurfaceKHR)                    \
  F(EnumeratePhysicalDevices) F(GetPhysicalDeviceProperties)                     \
  F(GetPhysicalDeviceProperties2) F(EnumerateDeviceExtensionProperties)           \
  F(GetPhysicalDeviceQueueFamilyProperties) F(GetPhysicalDeviceSurfaceSupportKHR) \
  F(GetPhysicalDeviceMemoryProperties) F(CreateDevice) F(GetDeviceProcAddr)

#define T742_DEVICE_API(F)                                                       \
  F(DestroyDevice) F(GetDeviceQueue) F(CreateBuffer) F(DestroyBuffer)              \
  F(GetBufferMemoryRequirements) F(AllocateMemory) F(FreeMemory)                  \
  F(BindBufferMemory) F(CreateDescriptorSetLayout) F(DestroyDescriptorSetLayout)   \
  F(CreateDescriptorPool) F(DestroyDescriptorPool) F(AllocateDescriptorSets)        \
  F(UpdateDescriptorSets) F(CreatePipelineLayout) F(DestroyPipelineLayout)         \
  F(CreateShaderModule) F(DestroyShaderModule) F(CreateComputePipelines)           \
  F(DestroyPipeline) F(CreateCommandPool) F(DestroyCommandPool)                   \
  F(AllocateCommandBuffers) F(BeginCommandBuffer) F(EndCommandBuffer)             \
  F(CmdBindPipeline) F(CmdBindDescriptorSets) F(CmdPushConstants)                  \
  F(CmdDispatch) F(CmdPipelineBarrier) F(CmdUpdateBuffer) F(CmdCopyBuffer)         \
  F(CreateFence) F(DestroyFence) F(QueueSubmit) F(WaitForFences) F(DeviceWaitIdle)  \
  F(MapMemory) F(UnmapMemory) F(InvalidateMappedMemoryRanges)

namespace grs::g13vk {
struct VulkanApi {
  PFN_vkGetInstanceProcAddr GetInstanceProcAddr = nullptr;
  PFN_vkEnumerateInstanceVersion EnumerateInstanceVersion = nullptr;
  PFN_vkEnumerateInstanceExtensionProperties EnumerateInstanceExtensionProperties = nullptr;
  PFN_vkCreateInstance CreateInstance = nullptr;
#define T742_DECLARE(name) PFN_vk##name name = nullptr;
  T742_INSTANCE_API(T742_DECLARE)
  T742_DEVICE_API(T742_DECLARE)
#undef T742_DECLARE
};
} // namespace grs::g13vk
