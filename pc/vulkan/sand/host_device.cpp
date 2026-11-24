#include "host_device.h"
#include <stdexcept>
#include <string>

namespace grs::pc::sand {
void RequireVk(VkResult result,const char *where) {
  if (result!=VK_SUCCESS) throw std::runtime_error(std::string(where)+" VkResult="+std::to_string(int(result)));
}
void RequireHost(bool ok,const char *where) {
  if (!ok) throw std::runtime_error(where);
}

uint64_t HostDevice::Now() {
  LARGE_INTEGER ticks,hz;
  QueryPerformanceCounter(&ticks);
  QueryPerformanceFrequency(&hz);
  const auto t=uint64_t(ticks.QuadPart), f=uint64_t(hz.QuadPart);
  return (t/f)*1000000000+(t%f)*1000000000/f;
}

bool HostDevice::Current() {
  if (!window) return true;
  MSG msg;
  while (PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
    if (msg.message==WM_QUIT) closed_=true;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  RECT bounds{};
  return !closed_ && GetClientRect(window,&bounds) && !IsIconic(window) &&
         bounds.right==client_.right && bounds.bottom==client_.bottom;
}

void HostDevice::Open(bool present,const wchar_t *title,bool requireFloat64) {
  if (present) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW type{};
    type.lpfnWndProc=DefWindowProcW;
    type.hInstance=GetModuleHandleW(nullptr);
    type.lpszClassName=L"PcSandR1Host";
    RequireHost(RegisterClassW(&type)!=0,"WIN32_REGISTER");
    window=CreateWindowExW(0,type.lpszClassName,title,WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,CW_USEDEFAULT,800,500,nullptr,nullptr,type.hInstance,nullptr);
    RequireHost(window!=nullptr,"WIN32_WINDOW");
    ShowWindow(window,SW_SHOWNORMAL);
    RequireHost(GetClientRect(window,&client_) && client_.right>0 && client_.bottom>0,"WIN32_EXTENT");
  }
  loader=LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
  RequireHost(loader!=nullptr,"VULKAN_LOADER");
  instanceProc=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader,"vkGetInstanceProcAddr"));
  RequireHost(instanceProc!=nullptr,"VK_INSTANCE_PROC");
  const auto create=reinterpret_cast<PFN_vkCreateInstance>(instanceProc(nullptr,"vkCreateInstance"));
  RequireHost(create!=nullptr,"VK_CREATE_INSTANCE_PROC");
  const VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO,nullptr,"PC Sand",1,nullptr,0,VK_API_VERSION_1_1};
  const char *instanceNames[]={VK_KHR_SURFACE_EXTENSION_NAME,"VK_KHR_win32_surface"};
  VkInstanceCreateInfo createInfo{};
  createInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo=&app;
  createInfo.enabledExtensionCount=present ? 2u : 0u;
  createInfo.ppEnabledExtensionNames=present ? instanceNames : nullptr;
  RequireVk(create(&createInfo,nullptr,&instance),"vkCreateInstance");
#define I(name) reinterpret_cast<PFN_vk##name>(instanceProc(instance,"vk" #name))
  if (present) {
    const auto make=I(CreateWin32SurfaceKHR);
    RequireHost(make!=nullptr,"VK_WIN32_SURFACE_PROC");
    const VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,nullptr,0,
                                           GetModuleHandleW(nullptr),window};
    RequireVk(make(instance,&info,nullptr,&surface),"vkCreateWin32SurfaceKHR");
  }
  const auto enumerate=I(EnumeratePhysicalDevices);
  const auto queueProps=I(GetPhysicalDeviceQueueFamilyProperties);
  const auto support=I(GetPhysicalDeviceSurfaceSupportKHR);
  const auto props=I(GetPhysicalDeviceProperties);
  const auto mem=I(GetPhysicalDeviceMemoryProperties);
  const auto features=I(GetPhysicalDeviceFeatures);
  const auto makeDevice=I(CreateDevice);
  RequireHost(enumerate && queueProps && props && mem && features && makeDevice && (!present || support),
              "VK_INSTANCE_FUNCTIONS");
  uint32_t count=0;
  RequireVk(enumerate(instance,&count,nullptr),"vkEnumeratePhysicalDevices.count");
  std::vector<VkPhysicalDevice> devices(count);
  RequireVk(enumerate(instance,&count,devices.data()),"vkEnumeratePhysicalDevices");
  for (const auto d:devices) {
    VkPhysicalDeviceProperties candidate{};
    props(d,&candidate);
    if (candidate.apiVersion<VK_API_VERSION_1_1 || candidate.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU) continue;
    uint32_t countFamilies=0;
    queueProps(d,&countFamilies,nullptr);
    std::vector<VkQueueFamilyProperties> families(countFamilies);
    queueProps(d,&countFamilies,families.data());
    for (uint32_t i=0;i<countFamilies;++i) {
      const VkQueueFlags flags=VK_QUEUE_COMPUTE_BIT | (present ? VK_QUEUE_GRAPHICS_BIT : 0);
      if (!families[i].queueCount || (families[i].queueFlags&flags)!=flags) continue;
      VkBool32 supported=VK_TRUE;
      if (present && support(d,i,surface,&supported)!=VK_SUCCESS) continue;
      if (!supported) continue;
      physical=d;
      family=i;
      properties=candidate;
      timestampBits=families[i].timestampValidBits;
      break;
    }
    if (physical) break;
  }
  RequireHost(physical!=VK_NULL_HANDLE,"NO_COMPATIBLE_GPU_QUEUE");
  if (const auto props2=I(GetPhysicalDeviceProperties2)) {
    VkPhysicalDeviceProperties2 identity{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,&deviceId};
    props2(physical,&identity);
  }
  mem(physical,&memory_);
  VkPhysicalDeviceFeatures supported{};
  features(physical,&supported);
  VkPhysicalDeviceFeatures enabled{};
  enabled.shaderInt64=supported.shaderInt64;
  RequireHost(!requireFloat64 || supported.shaderFloat64,"G14_FLOAT64_UNAVAILABLE");
  enabled.shaderFloat64=requireFloat64 ? VK_TRUE : VK_FALSE;
  const float priority=1.f;
  const VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,nullptr,0,family,1,&priority};
  const char *extensions[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.queueCreateInfoCount=1;
  deviceInfo.pQueueCreateInfos=&queueInfo;
  deviceInfo.pEnabledFeatures=&enabled;
  deviceInfo.enabledExtensionCount=present ? 1u : 0u;
  deviceInfo.ppEnabledExtensionNames=present ? extensions : nullptr;
  deviceProc=I(GetDeviceProcAddr);
  RequireHost(deviceProc!=nullptr,"VK_DEVICE_PROC");
  RequireVk(makeDevice(physical,&deviceInfo,nullptr,&device),"vkCreateDevice");
  const auto getQueue=reinterpret_cast<PFN_vkGetDeviceQueue>(deviceProc(device,"vkGetDeviceQueue"));
  RequireHost(getQueue!=nullptr,"VK_QUEUE_PROC");
  getQueue(device,family,0,&queue);
  RequireHost(queue!=VK_NULL_HANDLE,"VK_QUEUE");
#undef I
}

HostDevice::Memory HostDevice::Allocate(VkDeviceSize bytes,VkBufferUsageFlags usage,bool mapped) {
#define D(name) reinterpret_cast<PFN_vk##name>(deviceProc(device,"vk" #name))
  const auto make=D(CreateBuffer);
  const auto req=D(GetBufferMemoryRequirements);
  const auto alloc=D(AllocateMemory);
  const auto bind=D(BindBufferMemory);
  const auto map=D(MapMemory);
  RequireHost(make && req && alloc && bind && (!mapped || map),"VK_BUFFER_FUNCTIONS");
  allocations_.emplace_back();
  auto &resource=allocations_.back();
  auto &slice=resource.slice;
  slice.bytes=bytes;
  slice.generation=++generation_;
  const VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,nullptr,0,bytes,usage,
                                VK_SHARING_MODE_EXCLUSIVE,0,nullptr};
  RequireVk(make(device,&info,nullptr,&slice.buffer),"vkCreateBuffer");
  VkMemoryRequirements r{};
  req(device,slice.buffer,&r);
  uint32_t type=UINT32_MAX;
  for (uint32_t i=0;i<memory_.memoryTypeCount;++i) {
    if (!(r.memoryTypeBits&(1u<<i))) continue;
    const auto flags=memory_.memoryTypes[i].propertyFlags;
    if (mapped && !(flags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) continue;
    if (type==UINT32_MAX) type=i;
    if (flags&(mapped ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      type=i;
      break;
    }
  }
  RequireHost(type!=UINT32_MAX,"VK_MEMORY_TYPE");
  resource.allocation=r.size;
  resource.alignment=r.alignment;
  resource.memoryType=type;
  resource.coherent=(memory_.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;
  const VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,nullptr,r.size,type};
  RequireVk(alloc(device,&allocation,nullptr,&resource.memory),"vkAllocateMemory");
  RequireVk(bind(device,slice.buffer,resource.memory,0),"vkBindBufferMemory");
  if (mapped) {
    void *p=nullptr;
    RequireVk(map(device,resource.memory,0,VK_WHOLE_SIZE,0,&p),"vkMapMemory");
    resource.words=static_cast<const uint32_t*>(p);
  }
  return resource;
#undef D
}

HostDevice::~HostDevice() {
  if (device) {
#define D(name) reinterpret_cast<PFN_vk##name>(deviceProc(device,"vk" #name))
    D(DeviceWaitIdle)(device);
    for (const auto &r:allocations_) {
      if (r.words) D(UnmapMemory)(device,r.memory);
      if (r.slice.buffer) D(DestroyBuffer)(device,r.slice.buffer,nullptr);
      if (r.memory) D(FreeMemory)(device,r.memory,nullptr);
    }
    D(DestroyDevice)(device,nullptr);
#undef D
  }
  if (surface) reinterpret_cast<PFN_vkDestroySurfaceKHR>(instanceProc(instance,"vkDestroySurfaceKHR"))(instance,surface,nullptr);
  if (instance) reinterpret_cast<PFN_vkDestroyInstance>(instanceProc(instance,"vkDestroyInstance"))(instance,nullptr);
  if (window && IsWindow(window)) DestroyWindow(window);
  if (loader) FreeLibrary(loader);
}
}
