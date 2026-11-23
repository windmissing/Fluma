#pragma once
#include <functional>
#include "platform.h"
#include "vulkan_api.h"
#include "m1_hot_capture.h"
#include "m1_observation.h"

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
  bool empty=false;
  uint64_t serial=0;
  uintptr_t graphicsFence=0;
  struct Roi {
    std::array<M1Rect,3> regions{};
    std::array<uint8_t,kM1RoiBytes> bytes{};
    size_t size=0;
    const char *state=nullptr;
  } roi;
  const char *stage = "WSI_SETUP";
};

using PresentBusy = std::function<void(g13vk::M1HotCapture::Phase,bool,bool)>;
struct PresentObservations {
  bool emptyFirst=false;
  VkDeviceSize roiStride=0;
  std::function<const std::array<M1Rect,3>*()> requestedRoi;
};
Frame PresentBoot(g13vk::VulkanApi &, VkInstance, VkPhysicalDevice, VkDevice, VkQueue,
                  uint32_t family, VkSurfaceKHR, VkBuffer control, VkBuffer staging,
                  VkDeviceMemory stagingMemory, VkMemoryPropertyFlags,
                  HWND window, const std::function<bool()> &current,
                  const std::function<void(const Frame &)> &published,
                  VkDeviceSize m1PublicationBytes = 0, bool requireDiagnosticPixel = true,
                  const std::function<VkBuffer()> &nextPublication = {}, bool fullM1 = false,
                  const PresentBusy &captureBusy = {}, const PresentObservations &observation = {});
}
