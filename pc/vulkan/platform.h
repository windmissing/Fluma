#pragma once

#include <Windows.h>
#include <vulkan/vulkan.h>

// The bundled SDK exports Vulkan core but only ships the OHOS platform header.
#if __has_include(<vulkan/vulkan_win32.h>)
#include <vulkan/vulkan_win32.h>
#else
struct VkWin32SurfaceCreateInfoKHR {
  VkStructureType sType;
  const void *pNext;
  VkFlags flags;
  HINSTANCE hinstance;
  HWND hwnd;
};
using PFN_vkCreateWin32SurfaceKHR = VkResult(VKAPI_PTR *)(VkInstance,
    const VkWin32SurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
#endif

namespace grs::pc {
inline uint64_t QpcTicks() {
  LARGE_INTEGER value{};
  return QueryPerformanceCounter(&value) ? uint64_t(value.QuadPart) : 0;
}
inline uint64_t QpcFrequency() {
  LARGE_INTEGER value{};
  return QueryPerformanceFrequency(&value) ? uint64_t(value.QuadPart) : 0;
}
inline uint64_t QpcNs(uint64_t ticks,uint64_t hz) {
  return hz && hz <= UINT64_MAX / 1000000000ull
      ? ticks / hz * 1000000000ull + ticks % hz * 1000000000ull / hz : 0;
}
inline uint64_t TimeNs() { return QpcNs(QpcTicks(),QpcFrequency()); }
}
