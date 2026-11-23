#pragma once
#include "../platform.h"
#include "owner.h"
#include <vector>

namespace grs::pc::sand {
// A single Windows owner thread owns the queue, window and all Vulkan calls.
class HostDevice {
 public:
  struct Memory {
    Slice slice;
    VkDeviceMemory memory=VK_NULL_HANDLE;
    VkDeviceSize allocation=0;
    const uint32_t *words=nullptr;
    bool coherent=false;
    VulkanOwner::Readback Readback() const { return {slice,memory,allocation,0,words,coherent}; }
  };
  HMODULE loader=nullptr;
  HWND window=nullptr;
  VkInstance instance=VK_NULL_HANDLE;
  VkPhysicalDevice physical=VK_NULL_HANDLE;
  VkDevice device=VK_NULL_HANDLE;
  VkQueue queue=VK_NULL_HANDLE;
  VkSurfaceKHR surface=VK_NULL_HANDLE;
  uint32_t family=0;
  VkPhysicalDeviceProperties properties{};
  PFN_vkGetInstanceProcAddr instanceProc=nullptr;
  PFN_vkGetDeviceProcAddr deviceProc=nullptr;

  HostDevice()=default;
  HostDevice(const HostDevice &)=delete;
  HostDevice &operator=(const HostDevice &)=delete;
  ~HostDevice();
  void Open(bool present);
  Memory Allocate(VkDeviceSize bytes,VkBufferUsageFlags usage,bool host=false);
  bool Current();
  static uint64_t Now();
 private:
  std::vector<Memory> allocations_;
  VkPhysicalDeviceMemoryProperties memory_{};
  uint64_t generation_=0;
  RECT client_{};
  bool closed_=false;
};
void RequireVk(VkResult,const char *);
void RequireHost(bool,const char *);
}
