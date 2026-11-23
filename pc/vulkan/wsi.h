#pragma once
#include <functional>
#include "platform.h"
#include "vulkan_api.h"

namespace grs::pc {
struct Frame {
  VkResult error = VK_SUCCESS;
  VkResult acquire = VK_NOT_READY;
  VkResult submit = VK_NOT_READY;
  VkResult fence = VK_NOT_READY;
  VkResult present = VK_NOT_READY;
  VkResult imagePresent = VK_NOT_READY;
  VkResult retire = VK_NOT_READY;
  uint32_t width = 0, height = 0, format = 0, images = 0, imageIndex = UINT32_MAX;
  uint64_t presentEnter = 0, presentExit = 0;
  uint32_t pixel = 0;
  bool pixelRead = false, stale = false;
  const char *stage = "WSI_SETUP";
};

Frame PresentBoot(g13vk::VulkanApi &, VkInstance, VkPhysicalDevice, VkDevice, VkQueue,
                  uint32_t family, VkSurfaceKHR, VkBuffer control, VkBuffer staging,
                  VkDeviceMemory stagingMemory, VkMemoryPropertyFlags,
                  HWND window, const std::function<bool()> &current,
                  const std::function<void(const Frame &)> &published,
                  VkDeviceSize m1PublicationBytes = 0, bool requireDiagnosticPixel = true);
}
