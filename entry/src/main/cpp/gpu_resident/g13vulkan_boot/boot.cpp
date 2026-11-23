#include "boot.h"
#include "boot_contract.h"
#include "vk_boot_spirv.h"
#include "vulkan_api.h"
#ifdef _WIN32
#include "wsi.h"
#include "root_plan.h"
#include "../angles_generated.h"
#include "../m1/progress_clock_v2.h"
#include "../m1/quantum_passes.h"
#include "../m1/source_owner.h"
#include <filesystem>
#else
#include "../program_cache/mapped_build_id.h"
#include <dlfcn.h>
#include <hilog/log.h>
#include <unistd.h>
#endif

#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

namespace grs::g13vk {
namespace {
using namespace grs::g13;
constexpr uint32_t kSeal = 0xc0de1301u;
constexpr size_t kTailWords = 256, kInputs65 = 744, kInputs66 = 18648;
constexpr size_t kMathWords = 60, kMathProbeOffset = 512;
constexpr size_t kStagingWords = kTailWords + 4 + 5 + kMathWords;
constexpr size_t kLayoutBytes = 80 * 16, kPushBytes = 6 * 4;
constexpr size_t kWork = 5, kControl = 6, kLayout = 7, kStaging = 8;
#ifdef _WIN32
constexpr size_t kM1Base = 9, kM1Candidate = 10, kM1Work = 11, kM1Events = 12, kM1Pub = 13,
                 kM1Tail = 14, kM1Uniform = 15, kM1QScratch = 16, kM1QWork = 17, kM1QEvents = 18,
                 kM1QUniform = 19, kM1QPub = 20, kM1ColdStaging = 21, kM1QPubAlt = 22, kBufferCount = 23;
#else
constexpr size_t kBufferCount = 9;
#endif
constexpr auto kCompute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
constexpr auto kTransfer = VK_PIPELINE_STAGE_TRANSFER_BIT;
constexpr auto kShaderRW = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
constexpr const char *kTag = "G13VkBoot";

struct Failure {
  std::string reason;
  VkResult result = VK_SUCCESS;
  bool hasResult = false;
};
void Require(bool value, const char *reason) {
  if (!value)
    throw Failure{reason};
}
void Check(VkResult result, const char *call) {
  if (result != VK_SUCCESS)
    throw Failure{call, result, true};
}
template <typename Range> std::string Words(const Range &words) {
  std::ostringstream json;
  json << '[';
  bool comma = false;
  for (auto word : words) {
    if (comma)
      json << ',';
    json << +word;
    comma = true;
  }
  json << ']';
  return json.str();
}
template <typename T, typename Enumerate> std::vector<T> Enumerated(Enumerate enumerate, const char *name) {
  uint32_t count = 0;
  Check(enumerate(&count, nullptr), name);
  Require(count <= 4096, "ENUMERATION_COUNT_LIMIT");
  std::vector<T> items(count);
  if (count != 0) {
    Check(enumerate(&count, items.data()), name);
    items.resize(count);
  }
  return items;
}
bool Has(const std::vector<VkExtensionProperties> &items, const char *name) {
  return std::any_of(items.begin(), items.end(), [name](const auto &item) {
    return std::strcmp(item.extensionName, name) == 0;
  });
}
std::string ProcessStart() {
#ifdef _WIN32
  FILETIME start{}, exit{}, kernel{}, user{};
  if (!GetProcessTimes(GetCurrentProcess(), &start, &exit, &kernel, &user)) return {};
  return std::to_string((uint64_t(start.dwHighDateTime) << 32) | start.dwLowDateTime);
#else
  std::ifstream stat("/proc/self/stat");
  std::string line, value;
  std::getline(stat, line);
  auto end = line.rfind(')');
  if (end == std::string::npos)
    return "";
  std::istringstream fields(line.substr(end + 1));
  for (int field = 3; field <= 22; ++field)
    if (!(fields >> value))
      return "";
  return value;
#endif
}

struct Buffer {
  VkBuffer handle = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDeviceSize bytes = 0;
  VkMemoryPropertyFlags flags = 0;
};
struct Candidate {
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties{};
  uint32_t family = UINT32_MAX;
  VkQueueFlags queueFlags = 0;
  bool present = false;
  bool useExtension = false;
};

class Boot {
public:
  Boot(const Attempt &a, const Publish &p) : attempt(a), publish(p) {}
  ~Boot() {
    while (inFlight) {
      const VkResult drained = vk.DeviceWaitIdle(device);
      if (drained == VK_SUCCESS || drained == VK_ERROR_DEVICE_LOST)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (fence)
      vk.DestroyFence(device, fence, nullptr);
    if (commandPool)
      vk.DestroyCommandPool(device, commandPool, nullptr);
    if (pipeline)
      vk.DestroyPipeline(device, pipeline, nullptr);
    if (shader)
      vk.DestroyShaderModule(device, shader, nullptr);
#ifdef _WIN32
    for (auto p : m1QuantumPipelines)
      if (p) vk.DestroyPipeline(device, p, nullptr);
    if (m1QuantumDescriptorPool) vk.DestroyDescriptorPool(device, m1QuantumDescriptorPool, nullptr);
#endif
    if (pipelineLayout)
      vk.DestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (descriptorPool)
      vk.DestroyDescriptorPool(device, descriptorPool, nullptr);
    if (descriptorLayout)
      vk.DestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
    for (const auto &buffer : buffers) {
      if (buffer.handle)
        vk.DestroyBuffer(device, buffer.handle, nullptr);
      if (buffer.memory)
        vk.FreeMemory(device, buffer.memory, nullptr);
    }
    if (device && vk.DestroyDevice)
      vk.DestroyDevice(device, nullptr);
    if (surface)
      vk.DestroySurfaceKHR(instance, surface, nullptr);
    if (instance && vk.DestroyInstance)
      vk.DestroyInstance(instance, nullptr);
    if (loader) {
#ifdef _WIN32
      FreeLibrary(loader);
#else
      dlclose(loader);
#endif
    }
  }
  std::string Run();
  std::string Failed(const Failure &error) {
    if (error.hasResult)
      fields << ",\"vkResult\":" << int(error.result);
    if (inFlight)
      publish(Receipt("RETIRING_FAILED_SUBMISSION", error.reason));
    return Receipt("FAILED", error.reason);
  }

private:
  const Attempt &attempt;
  const Publish &publish;
  VulkanApi vk{};
#ifdef _WIN32
  HMODULE loader = nullptr;
  std::array<VkPipeline,2> m1QuantumPipelines{};
  VkDescriptorPool m1QuantumDescriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m1QuantumSets;
  std::vector<m1::QuantumPassOp> m1QuantumPasses;
#else
  void *loader = nullptr;
#endif
  uint32_t instanceApi = 0;
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties memory{};
  std::array<Buffer, kBufferCount> buffers{};
  VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::array<VkDescriptorSet, 5> sets{};
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkShaderModule shader = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkCommandBuffer command = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  bool inFlight = false;
  const char *stage = "HOST_IDENTITY";
  std::ostringstream fields;

  void Checkpoint(const char *next) {
    stage = next;
    Require(publish(Receipt("RUNNING", "")), "ATTEMPT_REVOKED");
#ifndef _WIN32
    OH_LOG_Print(LOG_APP, LOG_INFO, 0x0D00143, kTag, "ticket=%{public}llu stage=%{public}s",
                 static_cast<unsigned long long>(attempt.ticket), stage);
#endif
  }
  std::string Receipt(const char *state, const std::string &detail) {
    std::ostringstream json;
    json << "{\"schema\":\"G13VulkanBootV1\",\"role\":\"BOOT\",\"state\":" << Quote(state)
         << ",\"stage\":" << Quote(stage) << ",\"detail\":" << Quote(detail)
         << ",\"ticket\":\"" << attempt.ticket << "\",\"surface\":\"" << attempt.surface
         << "\",\"geometry\":\"" << attempt.geometry << "\"" << fields.str() << '}';
    return json.str();
  }
  void Open();
  Candidate Select(const Profile &profile);
  void OpenDevice(const Candidate &candidate);
  void Allocate(size_t slot, VkDeviceSize bytes, VkBufferUsageFlags usage, bool host = false);
  void CreatePipeline();
  void CreateDescriptors();
  void Record(const Profile &, const Config &, const std::array<uint32_t, 18688> &);
  void Barrier(VkPipelineStageFlags from, VkAccessFlags accesses,
               VkPipelineStageFlags to, VkAccessFlags next) {
    VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    memoryBarrier.srcAccessMask = accesses;
    memoryBarrier.dstAccessMask = next;
    vk.CmdPipelineBarrier(command, from, to, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  }
  void Dispatch(size_t set, uint32_t op, uint32_t scope);
  std::string Read(const std::array<uint32_t, 18688> &control);
#ifdef _WIN32
  void QualifySteady(const std::string &roleName);
  void ObserveT849Reduction(const char *,PFN_vkResetCommandPool);
  void ObserveT849Writes();
  std::string ProbeM1Source(const Candidate &);
  VkPipeline M1Pipeline(const std::wstring &, const char *);
  void PrepareM1Quantum(const Candidate &);
  bool RunM1Quantum(const Candidate &, grs::m1::SourceRootOwner &, grs::m1::ProgressClockV2 &,
                   grs::m1::PlanGrantV2 &, const std::function<bool()> &, grs::AcceptedRoot &,
                   uint64_t &, uint32_t &);
  void CaptureM1Quantum(grs::m1::SourceRootOwner &, const grs::AcceptedRoot &, const grs::Words256 &,
                        uint32_t, const std::array<uint32_t,32> &);
#endif
};

void Boot::Open() {
  Checkpoint("VULKAN_LOADER");
#ifdef _WIN32
  loader = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  Require(loader != nullptr, "LOAD_SYSTEM_VULKAN_FAILED");
  auto proc = GetProcAddress(loader, "vkGetInstanceProcAddr");
  Require(proc != nullptr, "MISSING_vkGetInstanceProcAddr");
#else
  loader = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
  if (!loader) {
    const char *error = dlerror();
    fields << ",\"loaderError\":" << Quote(error ? error : "");
    throw Failure{"DLOPEN_LIBVULKAN"};
  }
  auto *proc = dlsym(loader, "vkGetInstanceProcAddr");
  Require(proc != nullptr, "MISSING_vkGetInstanceProcAddr");
  Dl_info where{};
  if (dladdr(proc, &where) && where.dli_fname) {
    fields << ",\"loaderPath\":" << Quote(where.dli_fname);
    char loaderId[129]{};
    if (grs::MappedBuildId(where.dli_fbase, loaderId))
      fields << ",\"loaderBuildId\":" << Quote(loaderId);
  }
#endif
  vk.GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc);
  vk.EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
      vk.GetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
  uint32_t loaderApi = VK_API_VERSION_1_0;
  if (vk.EnumerateInstanceVersion)
    Check(vk.EnumerateInstanceVersion(&loaderApi), "vkEnumerateInstanceVersion");
  fields << ",\"loaderApi\":" << loaderApi;
  Require(loaderApi >= VK_API_VERSION_1_1, "VULKAN_1_1_UNAVAILABLE");
  instanceApi = std::min(loaderApi, VK_API_VERSION_1_2);
#define T742_GLOBAL(name)                                                        \
  vk.name = reinterpret_cast<PFN_vk##name>(vk.GetInstanceProcAddr(nullptr, "vk" #name)); \
  Require(vk.name != nullptr, "MISSING_vk" #name);
  T742_GLOBAL(EnumerateInstanceExtensionProperties)
  T742_GLOBAL(CreateInstance)
#undef T742_GLOBAL
  auto extensions = Enumerated<VkExtensionProperties>(
      [&](uint32_t *n, VkExtensionProperties *p) {
        return vk.EnumerateInstanceExtensionProperties(nullptr, n, p);
      }, "vkEnumerateInstanceExtensionProperties");
#ifdef _WIN32
  const std::array<const char *, 2> names{VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface"};
#else
  const std::array<const char *, 2> names{VK_KHR_SURFACE_EXTENSION_NAME, VK_OHOS_SURFACE_EXTENSION_NAME};
#endif
  Require(Has(extensions, names[0]), "MISSING_VK_KHR_surface");
  Require(Has(extensions, names[1]), "MISSING_PLATFORM_SURFACE");
  Checkpoint("INSTANCE_CREATE");
  VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app.pApplicationName = "G13 Vulkan BOOT";
  app.apiVersion = instanceApi;
  VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  info.pApplicationInfo = &app;
  info.enabledExtensionCount = names.size();
  info.ppEnabledExtensionNames = names.data();
  Check(vk.CreateInstance(&info, nullptr, &instance), "vkCreateInstance");
  fields << ",\"instanceApi\":" << instanceApi
         << ",\"instanceExtensions\":[" << Quote(names[0]) << ',' << Quote(names[1]) << ']';
#define T742_LOAD(name)                                                           \
  vk.name = reinterpret_cast<PFN_vk##name>(vk.GetInstanceProcAddr(instance, "vk" #name)); \
  Require(vk.name != nullptr, "MISSING_vk" #name);
  T742_INSTANCE_API(T742_LOAD)
#undef T742_LOAD
#ifdef _WIN32
  Checkpoint("WIN32_SURFACE");
  VkWin32SurfaceCreateInfoKHR win32{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                   GetModuleHandleW(nullptr), attempt.window};
  Check(vk.CreateWin32SurfaceKHR(instance, &win32, nullptr, &surface), "vkCreateWin32SurfaceKHR");
#else
  Checkpoint("XCOMPONENT_SURFACE");
  VkSurfaceCreateInfoOHOS infoSurface{VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS};
  infoSurface.window = attempt.window;
  Check(vk.CreateSurfaceOHOS(instance, &infoSurface, nullptr, &surface), "vkCreateSurfaceOHOS");
#endif
  fields << ",\"surfaceCreated\":true";
}

Candidate Boot::Select(const Profile &profile) {
  Checkpoint("PHYSICAL_DEVICE_QUALIFICATION");
  const auto devices = Enumerated<VkPhysicalDevice>(
      [&](uint32_t *n, VkPhysicalDevice *p) { return vk.EnumeratePhysicalDevices(instance, n, p); },
      "vkEnumeratePhysicalDevices");
  Require(!devices.empty(), "NO_PHYSICAL_DEVICE");
  Candidate choice{};
  size_t chosenIndex = 0;
  std::ostringstream diagnostics;
  diagnostics << '[';
  for (size_t index = 0; index < devices.size(); ++index) {
    Candidate candidate{};
    candidate.physical = devices[index];
    auto &properties = candidate.properties;
    vk.GetPhysicalDeviceProperties(candidate.physical, &properties);
#ifdef _WIN32
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
        properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      if (index) diagnostics << ',';
      diagnostics << "{\"qualification\":\"NON_HARDWARE_DEVICE\",\"deviceName\":" << Quote(properties.deviceName) << '}';
      continue;
    }
#endif
    const uint32_t api = std::min(properties.apiVersion, instanceApi);
    candidate.useExtension = api < VK_API_VERSION_1_2;
    bool extension = false;
    if (api >= VK_API_VERSION_1_1 && candidate.useExtension) {
      const auto names = Enumerated<VkExtensionProperties>(
          [&](uint32_t *n, VkExtensionProperties *p) {
            return vk.EnumerateDeviceExtensionProperties(candidate.physical, nullptr, n, p);
          }, "vkEnumerateDeviceExtensionProperties");
      extension = Has(names, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    }
    if (index)
      diagnostics << ',';
    diagnostics << "{\"vendorId\":" << properties.vendorID << ",\"deviceId\":" << properties.deviceID
                << ",\"driverVersion\":" << properties.driverVersion << ",\"deviceName\":"
                << Quote(properties.deviceName) << ",\"deviceApi\":" << properties.apiVersion
                << ",\"cacheUUIDBytes\":" << Words(properties.pipelineCacheUUID)
                << ",\"effectiveApi\":" << api;
    if (candidate.useExtension)
      diagnostics << ",\"extensionAvailable\":" << (extension ? "true" : "false");
    if (api < VK_API_VERSION_1_1 || (candidate.useExtension && !extension)) {
      diagnostics << ",\"qualification\":\""
                  << (api < VK_API_VERSION_1_1 ? "VULKAN_1_1_UNAVAILABLE" : "MISSING_VK_KHR_shader_float_controls")
                  << "\"}";
      continue;
    }
    VkPhysicalDeviceFloatControlsProperties floats{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
#ifdef _WIN32
    VkPhysicalDeviceIDProperties ids{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    floats.pNext = &ids;
#endif
    VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &floats};
    vk.GetPhysicalDeviceProperties2(candidate.physical, &properties2);
#ifdef _WIN32
    diagnostics << ",\"deviceUUIDBytes\":" << Words(ids.deviceUUID)
                << ",\"driverUUIDBytes\":" << Words(ids.driverUUID)
                << ",\"deviceLUIDValid\":" << (ids.deviceLUIDValid ? "true" : "false")
                << ",\"deviceLUIDBytes\":" << Words(ids.deviceLUID);
#endif
    diagnostics << ",\"shaderDenormPreserveFloat32\":" << floats.shaderDenormPreserveFloat32
                << ",\"shaderRoundingModeRTEFloat32\":" << floats.shaderRoundingModeRTEFloat32
                << ",\"shaderSignedZeroInfNanPreserveFloat32\":" << floats.shaderSignedZeroInfNanPreserveFloat32
                << ",\"denormIndependence\":" << int(floats.denormBehaviorIndependence)
                << ",\"roundingIndependence\":" << int(floats.roundingModeIndependence);
    if (floats.shaderDenormPreserveFloat32 != VK_TRUE || floats.shaderRoundingModeRTEFloat32 != VK_TRUE ||
        floats.shaderSignedZeroInfNanPreserveFloat32 != VK_TRUE) {
      diagnostics << ",\"qualification\":\"FP32_PROPERTIES_MISSING\"}";
      continue;
    }
    const auto &limits = properties.limits;
#ifdef _WIN32
    if (attempt.steadyQualification && limits.maxStorageBufferRange < 2331392u) {
      diagnostics << ",\"qualification\":\"FIVE_PROFILE_STORAGE_RANGE_UNSUPPORTED\"}";
      continue;
    }
#endif
    if (limits.maxComputeWorkGroupInvocations < 64 || limits.maxComputeWorkGroupSize[0] < 64 ||
        limits.maxComputeSharedMemorySize < 4096 || limits.maxPerStageDescriptorStorageBuffers < 4 ||
        limits.maxDescriptorSetStorageBuffers < 4 || limits.maxUniformBufferRange < kLayoutBytes ||
        limits.maxPushConstantsSize < kPushBytes ||
        limits.maxStorageBufferRange < 4 * std::max({profile.sw, profile.pw, profile.ww, 18688u})) {
      diagnostics << ",\"qualification\":\"COMPUTE_LIMITS_MISSING\",\"limits\":"
                  << Words(std::array<uint32_t, 8>{limits.maxComputeWorkGroupInvocations,
                       limits.maxComputeWorkGroupSize[0], limits.maxComputeSharedMemorySize,
                       limits.maxPerStageDescriptorStorageBuffers, limits.maxDescriptorSetStorageBuffers,
                       limits.maxUniformBufferRange, limits.maxPushConstantsSize,
                       limits.maxStorageBufferRange}) << '}';
      continue;
    }
    uint32_t count = 0;
    vk.GetPhysicalDeviceQueueFamilyProperties(candidate.physical, &count, nullptr);
    Require(count <= 4096, "QUEUE_FAMILY_COUNT_LIMIT");
    std::vector<VkQueueFamilyProperties> families(count);
    if (count)
      vk.GetPhysicalDeviceQueueFamilyProperties(candidate.physical, &count, families.data());
    diagnostics << ",\"computeQueues\":[";
    bool comma = false;
    bool surfaceQueryFailed = false;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &family = families[i];
      if (!family.queueCount || !(family.queueFlags & VK_QUEUE_COMPUTE_BIT))
        continue;
      VkBool32 present = VK_FALSE;
      const VkResult queried = vk.GetPhysicalDeviceSurfaceSupportKHR(candidate.physical, i, surface, &present);
      if (comma)
        diagnostics << ',';
      comma = true;
      diagnostics << "{\"index\":" << i << ",\"flags\":" << family.queueFlags
                  << ",\"surfaceSupportVkResult\":" << int(queried) << ",\"present\":" << present << '}';
      if (queried != VK_SUCCESS) {
        surfaceQueryFailed = true;
        continue;
      }
      if (present && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
        candidate.family = i;
        candidate.queueFlags = family.queueFlags;
        candidate.present = true;
        break;
      }
      if (candidate.family == UINT32_MAX) {
        candidate.family = i;
        candidate.queueFlags = family.queueFlags;
        candidate.present = present == VK_TRUE;
      }
    }
    diagnostics << "],\"qualification\":\""
                << (candidate.family != UINT32_MAX ? "ELIGIBLE" :
                    surfaceQueryFailed ? "SURFACE_SUPPORT_QUERY_FAILED" : "NO_COMPUTE_QUEUE") << "\"}";
    bool candidateCombined = candidate.present && (candidate.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    bool choiceCombined = choice.present && (choice.queueFlags & VK_QUEUE_GRAPHICS_BIT);
#ifdef _WIN32
    if (!candidateCombined) continue;
#endif
    if (candidate.family != UINT32_MAX && (!choice.physical || (candidateCombined && !choiceCombined))) {
      choice = candidate;
      chosenIndex = index;
    }
  }
  fields << ",\"devices\":" << diagnostics.str() << ']';
  Require(choice.physical != VK_NULL_HANDLE, "NO_ELIGIBLE_PHYSICAL_DEVICE");
  fields << ",\"selectedDevice\":" << chosenIndex << ",\"queueFamily\":" << choice.family
         << ",\"queueFlags\":" << choice.queueFlags << ",\"queueSurfaceSupport\":"
         << (choice.present ? "true" : "false") << ",\"floatControlsRoute\":"
         << Quote(choice.useExtension ? "VK_KHR_shader_float_controls" : "Vulkan 1.2 core");
  return choice;
}

void Boot::OpenDevice(const Candidate &candidate) {
  Checkpoint("DEVICE_CREATE");
  const float priority = 1.f;
  VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueInfo.queueFamilyIndex = candidate.family;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;
  std::array<const char *, 2> names{};
  uint32_t nameCount = 0;
  if (candidate.useExtension) names[nameCount++] = VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
#ifdef _WIN32
  if (attempt.presentBoot) {
    const auto extensions = Enumerated<VkExtensionProperties>(
        [&](uint32_t *n, VkExtensionProperties *p) {
          return vk.EnumerateDeviceExtensionProperties(candidate.physical, nullptr, n, p);
        }, "vkEnumerateDeviceExtensionProperties");
    Require(Has(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME), "MISSING_VK_KHR_swapchain");
    names[nameCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  }
#endif
  VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = &queueInfo;
  info.enabledExtensionCount = nameCount;
  info.ppEnabledExtensionNames = names.data();
  Check(vk.CreateDevice(candidate.physical, &info, nullptr, &device), "vkCreateDevice");
  fields << ",\"deviceExtensions\":[";
  for (uint32_t i = 0; i < nameCount; ++i) {
    if (i) fields << ',';
    fields << Quote(names[i]);
  }
  fields << ']';
#define T742_LOAD(name)                                                           \
  vk.name = reinterpret_cast<PFN_vk##name>(vk.GetDeviceProcAddr(device, "vk" #name)); \
  Require(vk.name != nullptr, "MISSING_vk" #name);
  T742_DEVICE_API(T742_LOAD)
#undef T742_LOAD
  vk.GetDeviceQueue(device, candidate.family, 0, &queue);
  Require(queue != VK_NULL_HANDLE, "NULL_COMPUTE_QUEUE");
  vk.GetPhysicalDeviceMemoryProperties(candidate.physical, &memory);
}

void Boot::Allocate(size_t slot, VkDeviceSize bytes, VkBufferUsageFlags usage, bool host) {
  auto &buffer = buffers[slot];
  buffer.bytes = bytes;
  VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  info.size = bytes;
  info.usage = usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  Check(vk.CreateBuffer(device, &info, nullptr, &buffer.handle), "vkCreateBuffer");
  VkMemoryRequirements requirements{};
  vk.GetBufferMemoryRequirements(device, buffer.handle, &requirements);
  VkMemoryPropertyFlags required = host ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  uint32_t chosen = UINT32_MAX;
  for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
    const auto flags = memory.memoryTypes[i].propertyFlags;
    if (!(requirements.memoryTypeBits & (1u << i)) || (flags & required) != required)
      continue;
    if (chosen == UINT32_MAX)
      chosen = i;
    if (host && (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      chosen = i;
      break;
    }
  }
  Require(chosen != UINT32_MAX, host ? "NO_HOST_VISIBLE_MEMORY" : "NO_DEVICE_LOCAL_MEMORY");
  buffer.flags = memory.memoryTypes[chosen].propertyFlags;
  VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocation.allocationSize = requirements.size;
  allocation.memoryTypeIndex = chosen;
  Check(vk.AllocateMemory(device, &allocation, nullptr, &buffer.memory), "vkAllocateMemory");
  Check(vk.BindBufferMemory(device, buffer.handle, buffer.memory, 0), "vkBindBufferMemory");
}

void Boot::CreatePipeline() {
  Checkpoint("PIPELINE_LAYOUT");
  std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
  for (uint32_t i = 0; i < bindings.size(); ++i)
    bindings[i] = {i, i == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                   1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
  VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.bindingCount = bindings.size();
  info.pBindings = bindings.data();
  Check(vk.CreateDescriptorSetLayout(device, &info, nullptr, &descriptorLayout), "vkCreateDescriptorSetLayout");
  VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, kPushBytes};
  VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout.setLayoutCount = 1;
  layout.pSetLayouts = &descriptorLayout;
  layout.pushConstantRangeCount = 1;
  layout.pPushConstantRanges = &push;
  Check(vk.CreatePipelineLayout(device, &layout, nullptr, &pipelineLayout), "vkCreatePipelineLayout");
  fields << ",\"layout\":{\"set\":0,\"storageBindings\":[0,1,2,3],\"uniformBinding\":4,"
            "\"layoutBytes\":1280,\"layoutStride\":16,\"pushOffsets\":[0,4,8,12,16,20]}";
  Checkpoint("BOOT_SHADER_MODULE");
  Require(Hex(Hash(kBootSpirv, sizeof(kBootSpirv)).data()) == kModuleSHA, "MODULE_SHA_MISMATCH");
  VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  shaderInfo.codeSize = sizeof(kBootSpirv);
  shaderInfo.pCode = kBootSpirv;
  auto started = Now();
  const VkResult created = vk.CreateShaderModule(device, &shaderInfo, nullptr, &shader);
  fields << ",\"shaderModuleVkResult\":" << int(created) << ",\"shaderModuleNs\":\"" << Now() - started << '"';
  Check(created, "vkCreateShaderModule");
  Checkpoint("BOOT_PIPELINE");
  VkComputePipelineCreateInfo infoCompute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  infoCompute.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  infoCompute.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  infoCompute.stage.module = shader;
  infoCompute.stage.pName = "main";
  infoCompute.layout = pipelineLayout;
  started = Now();
  const VkResult built = vk.CreateComputePipelines(device, VK_NULL_HANDLE, 1, &infoCompute, nullptr, &pipeline);
  fields << ",\"pipelineVkResult\":" << int(built) << ",\"pipelineNs\":\"" << Now() - started << '"';
  Check(built, "vkCreateComputePipelines");
  fields << ",\"pipelineCount\":1";
}

void Boot::CreateDescriptors() {
  std::array<VkDescriptorPoolSize, 2> sizes{{
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5}}};
  VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool.maxSets = sets.size();
  pool.poolSizeCount = sizes.size();
  pool.pPoolSizes = sizes.data();
  Check(vk.CreateDescriptorPool(device, &pool, nullptr, &descriptorPool), "vkCreateDescriptorPool");
  std::array<VkDescriptorSetLayout, 5> layouts{};
  layouts.fill(descriptorLayout);
  VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocation.descriptorPool = descriptorPool;
  allocation.descriptorSetCount = sets.size();
  allocation.pSetLayouts = layouts.data();
  Check(vk.AllocateDescriptorSets(device, &allocation, sets.data()), "vkAllocateDescriptorSets");
  for (size_t set = 0; set < sets.size(); ++set) {
    const std::array<size_t, 5> slots{0, set, kWork, kControl, kLayout};
    std::array<VkDescriptorBufferInfo, 5> infos{};
    std::array<VkWriteDescriptorSet, 5> writes{};
    for (uint32_t i = 0; i < slots.size(); ++i) {
      const auto &buffer = buffers[slots[i]];
      infos[i] = {buffer.handle, 0, buffer.bytes};
      auto &write = writes[i];
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = sets[set];
      write.dstBinding = i;
      write.descriptorCount = 1;
      write.descriptorType = i == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      write.pBufferInfo = &infos[i];
    }
    vk.UpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
  }
}

void Boot::Dispatch(size_t set, uint32_t op, uint32_t scope) {
  std::array<uint32_t, 6> push{op, scope, 0, 0, 0, 0};
  vk.CmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &sets[set], 0, nullptr);
  vk.CmdPushConstants(command, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), push.data());
  vk.CmdDispatch(command, 1, 1, 1);
}

void Boot::Record(const Profile &profile, const Config &config,
                  const std::array<uint32_t, 18688> &control) {
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command, &begin), "vkBeginCommandBuffer");
  const auto rawLayout = BootLayout(profile);
  std::array<uint32_t, kLayoutBytes / 4> layout{};
  for (size_t i = 0; i < rawLayout.size(); ++i)
    layout[i * 4] = rawLayout[i];
  constexpr size_t kUpdateLimit = 65536;
  static_assert(sizeof(control) > kUpdateLimit && sizeof(control) <= 2 * kUpdateLimit);
  vk.CmdUpdateBuffer(command, buffers[kControl].handle, 0, kUpdateLimit, control.data());
  vk.CmdUpdateBuffer(command, buffers[kControl].handle, kUpdateLimit, sizeof(control) - kUpdateLimit,
                     control.data() + kUpdateLimit / 4);
  vk.CmdUpdateBuffer(command, buffers[kLayout].handle, 0, sizeof(layout), layout.data());
  Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, kCompute, kShaderRW | VK_ACCESS_UNIFORM_READ_BIT);
  vk.CmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  constexpr std::array<uint32_t, 5> scopes{0, 1, 1, 2, 2};
  for (size_t i = 0; i < scopes.size(); ++i) {
    Dispatch(i, 64, scopes[i]);
    Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
  }
  const auto header = BootHeader(profile, config, g13::Identity{attempt.ticket, attempt.ticket,
      attempt.ticket, attempt.surface, attempt.geometry});
  Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT);
  vk.CmdUpdateBuffer(command, buffers[0].handle, 0, sizeof(header), header.data());
  Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, kCompute, kShaderRW);
  Dispatch(0, 65, scopes.back());
  Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
  Dispatch(0, 66, 0);
  Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
  Dispatch(0, 66, 1);
  Barrier(kCompute | kTransfer, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
          kTransfer, VK_ACCESS_TRANSFER_READ_BIT);
  const std::array<VkBufferCopy, 3> controlCopies{{
      {kColdTailWordOffset * 4, 0, kTailWords * 4},
      {kInputs65 * 4, kTailWords * 4, 16},
      {kInputs66 * 4, (kTailWords + 4) * 4, 20}}};
  const VkBufferCopy math{(profile.work[19].offset + kMathProbeOffset) * 4,
                          (kTailWords + 9) * 4, kMathWords * 4};
  vk.CmdCopyBuffer(command, buffers[kControl].handle, buffers[kStaging].handle,
                   controlCopies.size(), controlCopies.data());
  vk.CmdCopyBuffer(command, buffers[kWork].handle, buffers[kStaging].handle, 1, &math);
  Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command), "vkEndCommandBuffer");
  fields << ",\"dispatches\":[[64,0,0],[64,1,1],[64,1,2],[64,2,3],[64,2,4],"
            "[65,2,0],[66,0,0],[66,1,0]],\"dispatchColumns\":[\"op\",\"scope\",\"candidateSlot\"],"
            "\"ctrlTailByteOffset\":" << controlCopies[0].srcOffset
         << ",\"work65ByteOffset\":" << math.srcOffset;
}

std::string Boot::Read(const std::array<uint32_t, 18688> &control) {
  auto &staging = buffers[kStaging];
  void *mapped = nullptr;
  Check(vk.MapMemory(device, staging.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory");
  const bool coherent = staging.flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!coherent) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    range.memory = staging.memory;
    range.size = VK_WHOLE_SIZE; // Whole allocation with offset zero satisfies atom alignment.
    const VkResult result = vk.InvalidateMappedMemoryRanges(device, 1, &range);
    if (result != VK_SUCCESS) {
      vk.UnmapMemory(device, staging.memory);
      Check(result, "vkInvalidateMappedMemoryRanges");
    }
  }
  std::array<uint32_t, kStagingWords> raw{};
  std::memcpy(raw.data(), mapped, sizeof(raw));
  vk.UnmapMemory(device, staging.memory);
  const size_t probe = kProbeWordOffset - kColdTailWordOffset;
  std::array<uint32_t, 2> presence{}, masks{};
  for (size_t repeat = 0; repeat < masks.size(); ++repeat) {
    const auto *words = raw.data() + probe + repeat * kExpectedFp32.size();
    presence[repeat] = words[7] == kExpectedFp32.back();
    for (size_t i = 0; i < kExpectedFp32.size(); ++i)
      if (words[i] != kExpectedFp32[i])
        masks[repeat] |= 1u << i;
  }
  std::array<uint32_t, 16> output{};
  std::copy_n(raw.data() + probe, output.size(), output.begin());
  std::array<uint32_t, 4> inputs65{};
  std::array<uint32_t, 5> inputs66{}, math{};
  std::copy_n(raw.data() + kTailWords, inputs65.size(), inputs65.begin());
  std::copy_n(raw.data() + kTailWords + inputs65.size(), inputs66.size(), inputs66.begin());
  for (size_t i = 0; i < math.size(); ++i)
    math[i] = raw[kTailWords + 9 + 35 + 6 * i];
  const bool inputsOK = std::equal(inputs65.begin(), inputs65.end(), control.begin() + kInputs65) &&
                        std::equal(inputs66.begin(), inputs66.end(), control.begin() + kInputs66);
  const bool sealed = raw[255] == kSeal;
  const bool op65 = sealed && raw[7] == 0;
  const bool op66 = QualifiedProbe(output.data());
  const bool mathOK = math[0] == 0 && math[1] == 0x3f800000u &&
      std::isfinite(F32(math[2])) && std::abs(F32(math[2]) - 1.f) <= std::ldexp(1.f, -21) &&
      math[3] == 1u && math[4] == 0x3f800000u;
  fields << ",\"stagingCoherent\":" << (coherent ? "true" : "false")
         << ",\"stagingInvalidated\":" << (coherent ? "false" : "true")
         << ",\"op65\":{\"state\":" << Quote(!sealed ? "UNSEALED" : op65 ? "QUALIFIED" : "REJECTED")
         << ",\"tailSeal\":" << raw[255] << ",\"tailError\":" << raw[7]
         << ",\"input744To747\":" << Words(inputs65) << ",\"work35_41_47_53_59\":" << Words(math)
         << "},\"op66\":{\"state\":" << Quote(!presence[0] || !presence[1] ? "MISSING_PROBE_SEAL" :
                                                    op66 ? "QUALIFIED" : "REJECTED")
         << ",\"input18648To18652\":" << Words(inputs66) << ",\"ctrl200To215\":" << Words(output)
         << ",\"presence\":" << Words(presence) << ",\"mismatchMasks\":" << Words(masks) << '}';
  const char *first = !op65 ? "OP65" : !op66 ? "OP66" : "NONE";
  const unsigned originalError = !op65 ? (raw[7] ? raw[7] : 28) : !op66 ? 27 : 0;
  fields << ",\"originalFirstReject\":" << Quote(first) << ",\"originalError\":" << originalError;
  if (!inputsOK)
    throw Failure{"INPUT_READBACK_MISMATCH"};
  if (!op65 || !op66)
    return Receipt("REJECTED", first);
  Require(mathOK, "WORK_MATH_READBACK_CONTRADICTS_CTRL");
  return Receipt("BOOT_NUMERIC_QUALIFIED", "");
}

#ifdef _WIN32
#include "m1_source.inc"
#include "m1_quantum.inc"

void Boot::QualifySteady(const std::string &roleName) {
  const auto reset = reinterpret_cast<PFN_vkResetCommandPool>(
      vk.GetDeviceProcAddr(device, "vkResetCommandPool"));
  Require(reset != nullptr, "MISSING_vkResetCommandPool");
  // The original steady-role gate runs opcode 66 twice against the same
  // program and consumes only its independently committed 16 probe words.
  size_t qualifiedCount = 0;
  for (const auto &role : pc::kSteadyModules) {
    if (roleName.empty() ? qualifiedCount >= 5 : roleName != role.name) continue;
    Require(Hex(Hash(role.words, role.bytes).data()) == role.sha, "STEADY_MODULE_SHA_MISMATCH");
    if (shader) {
      vk.DestroyShaderModule(device, shader, nullptr);
      shader = VK_NULL_HANDLE;
    }
    if (pipeline) {
      vk.DestroyPipeline(device, pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
    stage = "STEADY_PIPELINE";
    Require(publish(Receipt("RUNNING", role.name)), "ATTEMPT_REVOKED");
    VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module.codeSize = role.bytes;
    module.pCode = role.words;
    VkComputePipelineCreateInfo compute{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute.flags = attempt.disableOptimization ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0;
    compute.layout = pipelineLayout;
    auto edge = [&](const char *call, const char *phase, const char *result) {
      std::ostringstream row;
      row << "{\"schema\":\"PC_G13_PIPELINE_PROGRESS_V1\",\"call\":" << Quote(call)
          << ",\"edge\":" << Quote(phase) << ",\"role\":" << Quote(role.name) << ",\"moduleSHA\":" << Quote(role.sha)
          << ",\"layoutSHA\":" << Quote(kHash_layoutSHA) << ",\"flags\":" << compute.flags << ",\"pipelineCache\":null"
          << ",\"pid\":" << GetCurrentProcessId() << ",\"ownerTid\":" << GetCurrentThreadId()
          << ",\"processStartTick\":" << Quote(ProcessStart()) << ",\"qpcFrequency\":\"" << pc::QpcFrequency() << "\""
          << ",\"qpc\":\"" << pc::QpcTicks() << "\",\"vkResult\":" << result << '}';
      return publish(row.str());
    };
    Require(edge("vkCreateShaderModule", "ENTER", "null"), "ATTEMPT_REVOKED");
    const auto loaded = vk.CreateShaderModule(device, &module, nullptr, &shader);
    edge("vkCreateShaderModule", "EXIT", std::to_string(int(loaded)).c_str());
    Check(loaded, "vkCreateShaderModule.STEADY");
    compute.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                     VK_SHADER_STAGE_COMPUTE_BIT, shader, "main", nullptr};
    Require(edge("vkCreateComputePipelines", "ENTER", "null"), "ATTEMPT_REVOKED");
    const auto buildStart = Now();
    const auto built = vk.CreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute, nullptr, &pipeline);
    const auto buildNs = Now() - buildStart;
    edge("vkCreateComputePipelines", "EXIT", std::to_string(int(built)).c_str());
    Check(built, "vkCreateComputePipelines.STEADY");
    Require(publish(Receipt("ROLE_PIPELINE_CREATED", role.name)), "ATTEMPT_REVOKED");

    vk.DestroyFence(device, fence, nullptr);
    fence = VK_NULL_HANDLE;
    Check(reset(device, commandPool, 0), "vkResetCommandPool");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vk.BeginCommandBuffer(command, &begin), "vkBeginCommandBuffer.STEADY");
    constexpr std::array<uint32_t, 16> clear{};
    vk.CmdUpdateBuffer(command, buffers[kControl].handle, kProbeWordOffset * sizeof(uint32_t),
                       sizeof(clear), clear.data());
    Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, kCompute, kShaderRW);
    vk.CmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    Dispatch(0, 66, 0);
    Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kCompute, kShaderRW);
    Dispatch(0, 66, 1);
    Barrier(kCompute, VK_ACCESS_SHADER_WRITE_BIT, kTransfer, VK_ACCESS_TRANSFER_READ_BIT);
    const VkBufferCopy output{kProbeWordOffset * sizeof(uint32_t), 0, sizeof(clear)};
    vk.CmdCopyBuffer(command, buffers[kControl].handle, buffers[kStaging].handle, 1, &output);
    Barrier(kTransfer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);
    Check(vk.EndCommandBuffer(command), "vkEndCommandBuffer.STEADY");
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    Check(vk.CreateFence(device, &fenceInfo, nullptr, &fence), "vkCreateFence.STEADY");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    stage = "STEADY_OP66";
    Require(publish(Receipt("RUNNING", role.name)), "ATTEMPT_REVOKED");
    const auto queued = vk.QueueSubmit(queue, 1, &submit, fence);
    edge("vkQueueSubmit", "EXIT", std::to_string(int(queued)).c_str());
    Check(queued, "vkQueueSubmit.STEADY");
    inFlight = true;
    const auto waited = vk.WaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    if (waited == VK_SUCCESS || waited == VK_ERROR_DEVICE_LOST) inFlight = false;
    edge("vkWaitForFences", "EXIT", std::to_string(int(waited)).c_str());
    Check(waited, "vkWaitForFences.STEADY");

    std::array<uint32_t, 16> words{};
    const auto &staging = buffers[kStaging];
    void *mapped = nullptr;
    Check(vk.MapMemory(device, staging.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory.STEADY");
    VkResult invalidation = VK_SUCCESS;
    if (!(staging.flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, staging.memory, 0, VK_WHOLE_SIZE};
      invalidation = vk.InvalidateMappedMemoryRanges(device, 1, &range);
    }
    if (invalidation == VK_SUCCESS) std::memcpy(words.data(), mapped, sizeof(words));
    vk.UnmapMemory(device, staging.memory);
    Check(invalidation, "vkInvalidateMappedMemoryRanges.STEADY");

    std::array<uint32_t, 2> mask{};
    for (size_t i = 0; i < words.size(); ++i)
      if (words[i] != kExpectedFp32[i % 8]) mask[i / 8] |= 1u << (i % 8);
    std::ostringstream sample;
    sample << "{\"schema\":\"PC_G13_STEADY_OP66_V1\",\"role\":" << Quote(role.name)
           << ",\"moduleSHA\":" << Quote(role.sha) << ",\"pid\":" << GetCurrentProcessId()
           << ",\"processStartTick\":" << Quote(ProcessStart()) << ",\"ownerTid\":" << GetCurrentThreadId()
           << ",\"pipelineNs\":\"" << buildNs << "\",\"queueSubmit\":" << int(queued)
           << ",\"wait\":" << int(waited) << ",\"probeWords\":" << Words(words)
           << ",\"mismatchMasks\":" << Words(mask) << ",\"acceptedRoot\":null}";
    Require(publish(sample.str()), "ATTEMPT_REVOKED");
    Require(QualifiedProbe(words.data()), "STEADY_FP32_REJECTED");
    if (roleName == "OP7_REDUCE_ONLY") ObserveT849Reduction(role.sha,reset);
    ++qualifiedCount;
  }
  Require(qualifiedCount != 0, "UNKNOWN_STEADY_ROLE");
  if (attempt.t849WriteFile.empty())
    fields << ",\"steadyRoleCount\":" << qualifiedCount
           << ",\"sourceRoot\":\"NOT_RUN\",\"quantumRoot\":\"NOT_RUN\"";
}

#include "t849_writes.inc"

void Boot::ObserveT849Reduction(const char *moduleSHA, PFN_vkResetCommandPool reset) {
  Checkpoint("T849_PRIVATE_REDUCE");
  constexpr uint32_t thetaBits=0x3fc00000;
  constexpr std::array<uint32_t,5> empty{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
  constexpr auto firstWord=624u;
  static_assert(sizeof(empty)==20);
  constexpr std::array<uint32_t,6> push{7,0,0,0,0,815};
  Check(reset(device, commandPool, 0), "vkResetCommandPool.T849_REDUCE");
  vk.DestroyFence(device,fence,nullptr);
  fence=VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device,&fenceInfo,nullptr,&fence),"vkCreateFence.T849_REDUCE");
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  Check(vk.BeginCommandBuffer(command,&begin),"vkBeginCommandBuffer.T849_REDUCE");
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT);
  vk.CmdUpdateBuffer(command,buffers[1].handle,26*sizeof(uint32_t),sizeof(thetaBits),&thetaBits);
  vk.CmdUpdateBuffer(command,buffers[kControl].handle,firstWord*sizeof(uint32_t),sizeof(empty),empty.data());
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,kCompute,kShaderRW);
  vk.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
  vk.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&sets[1],0,nullptr);
  vk.CmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push.data());
  vk.CmdDispatch(command,1,1,1);
  Barrier(kCompute,VK_ACCESS_SHADER_WRITE_BIT,kTransfer,VK_ACCESS_TRANSFER_READ_BIT);
  const VkBufferCopy copy{firstWord*sizeof(uint32_t),0,sizeof(empty)};
  const auto &staging=buffers[kStaging];
  vk.CmdCopyBuffer(command,buffers[kControl].handle,staging.handle,1,&copy);
  Barrier(kTransfer,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_HOST_READ_BIT);
  Check(vk.EndCommandBuffer(command),"vkEndCommandBuffer.T849_REDUCE");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount=1;
  submit.pCommandBuffers=&command;
  const auto submitted=vk.QueueSubmit(queue,1,&submit,fence);
  Check(submitted,"vkQueueSubmit.T849_REDUCE");
  inFlight=true;
  const auto waited=vk.WaitForFences(device,1,&fence,VK_TRUE,250000000u);
  if (waited==VK_SUCCESS || waited==VK_ERROR_DEVICE_LOST) inFlight=false;
  Check(waited,"vkWaitForFences.T849_REDUCE");
  void *mapped=nullptr;
  Check(vk.MapMemory(device,staging.memory,0,VK_WHOLE_SIZE,0,&mapped),"vkMapMemory.T849_REDUCE");
  VkResult invalidation=VK_SUCCESS;
  if (!(staging.flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,nullptr,staging.memory,0,VK_WHOLE_SIZE};
    invalidation=vk.InvalidateMappedMemoryRanges(device,1,&range);
  }
  std::array<uint32_t,5> output{};
  if (invalidation==VK_SUCCESS) std::memcpy(output.data(),mapped,sizeof(output));
  vk.UnmapMemory(device,staging.memory);
  Check(invalidation,"vkInvalidateMappedMemoryRanges.T849_REDUCE");
  Require(output[0]<=3 && output!=empty,"T849_PRIVATE_REDUCE_UNWRITTEN");
  std::ostringstream row;
  row << "{\"schema\":\"PC_G13_T849_REDUCE_V1\",\"role\":\"OP7_REDUCE_ONLY\",\"moduleSHA\":"
      << Quote(moduleSHA) << ",\"inputF32Word\":" << thetaBits << ",\"push\":" << Words(push)
      << ",\"ctrlStartWord\":" << firstWord << ",\"words\":" << Words(output)
      << ",\"queueSubmit\":" << int(submitted) << ",\"wait\":" << int(waited)
      << ",\"acceptedRoot\":null}";
  Require(publish(row.str()),"ATTEMPT_REVOKED");
}

#endif

std::string Boot::Run() {
#ifdef _WIN32
  Checkpoint("HOST_IDENTITY");
  const std::string start = ProcessStart();
  Require(!start.empty(), "PROCESS_START_TICK_UNAVAILABLE");
  fields << ",\"platform\":\"WIN32\",\"pid\":" << GetCurrentProcessId()
         << ",\"processStartTick\":" << Quote(start) << ",\"ownerTid\":" << GetCurrentThreadId()
         << ",\"qpcFrequency\":\"" << pc::QpcFrequency() << "\",\"moduleSHA\":" << Quote(kModuleSHA);
#else
  char buildId[129]{};
  Dl_info module{};
  Checkpoint("HOST_IDENTITY");
  Require(grs::BuildIdForAddress(kBootSpirv, buildId) &&
          dladdr(kBootSpirv, &module) && module.dli_fname, "MAPPED_ELF_IDENTITY_UNAVAILABLE");
  const std::string start = ProcessStart();
  Require(!start.empty(), "PROCESS_START_TICK_UNAVAILABLE");
  fields << ",\"pid\":" << getpid() << ",\"processStartTick\":" << Quote(start)
         << ",\"ownerTid\":" << gettid() << ",\"mappedBuildId\":" << Quote(buildId)
         << ",\"mappedElf\":" << Quote(module.dli_fname) << ",\"moduleSHA\":" << Quote(kModuleSHA);
#endif
  fields << ",\"sourceSHA\":" << Quote(kSourceSHA) << ",\"parentSourceSHA\":" << Quote(kParentSHA)
         << ",\"configSHA\":" << Quote(kConfigSHA) << ",\"seedSHA\":" << Quote(kSeedReceiptSHA)
         << ",\"layoutSHA\":" << Quote(kHash_layoutSHA) << ",\"mathSHA\":" << Quote(kHash_mathSHA);
  Config config{};
  static_assert(std::size(kRestConfig) == std::tuple_size<Config>::value);
  std::copy(std::begin(kRestConfig), std::end(kRestConfig), config.begin());
  Require(ValidateConfig(config, false) == 0 && Hex(Hash(config.data(), sizeof(config)).data()) == kConfigSHA &&
          Hex(config.data() + 60) == kSeedReceiptSHA, "REST_CONFIG_128_INVALID");
  const auto *profile = FindProfile(config[4], config[5]);
  Require(profile && profile->work[19].words >= kMathProbeOffset + kMathWords &&
          (profile->work[19].offset + kMathProbeOffset) * 4 == 583296, "REST_WORK_LAYOUT_MISMATCH");
  Open();
  const auto candidate = Select(*profile);
  OpenDevice(candidate);
  CreatePipeline();
  Checkpoint("BOOT_BUFFERS");
  const auto storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
#ifdef _WIN32
  const auto stateTransferRead=attempt.t849WriteFile.empty() ? 0 : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
#else
  constexpr auto stateTransferRead=0;
#endif
  for (size_t i = 0; i < 3; ++i)
    Allocate(i, profile->sw * 4, storage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | stateTransferRead);
  for (size_t i = 3; i < 5; ++i)
    Allocate(i, profile->pw * 4, storage);
  Allocate(kWork, profile->ww * 4, storage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  Allocate(kControl, 18688 * 4, storage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  Allocate(kLayout, kLayoutBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  Allocate(kStaging, kStagingWords * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
  CreateDescriptors();
  VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  pool.queueFamilyIndex = candidate.family;
  Check(vk.CreateCommandPool(device, &pool, nullptr, &commandPool), "vkCreateCommandPool");
  VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocate.commandPool = commandPool;
  allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate.commandBufferCount = 1;
  Check(vk.AllocateCommandBuffers(device, &allocate, &command), "vkAllocateCommandBuffers");
  const auto &p = candidate.properties;
  const std::string identity = std::to_string(p.vendorID) + ":" + std::to_string(p.deviceID) +
      ":" + std::to_string(p.driverVersion) + ":" + std::to_string(candidate.family);
  const std::string environment = Hex(Hash(identity.data(), identity.size()).data());
  fields << ",\"environmentSHA\":" << Quote(environment) << ",\"ctrlObject\":6,\"workObject\":5";
  const auto control = BootControl(config, kSourceSHA, environment);
  Checkpoint("RECORD_BOOT");
  Record(*profile, config, control);
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  Check(vk.CreateFence(device, &fenceInfo, nullptr, &fence), "vkCreateFence");
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command;
  Checkpoint("QUEUE_SUBMIT");
  Check(vk.QueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit");
  inFlight = true;
  fields << ",\"queueSubmitVkResult\":0";
  stage = "BOOT_FENCE";
  publish(Receipt("RUNNING", ""));
  uint64_t timeouts = 0;
  VkResult wait;
  do {
    wait = vk.WaitForFences(device, 1, &fence, VK_TRUE, 50000000);
    if (wait == VK_TIMEOUT)
      ++timeouts;
  } while (wait == VK_TIMEOUT);
  if (wait == VK_SUCCESS || wait == VK_ERROR_DEVICE_LOST)
    inFlight = false;
  fields << ",\"fenceWaitVkResult\":" << int(wait) << ",\"fencePollTimeouts\":" << timeouts;
  Check(wait, "vkWaitForFences");
  Checkpoint("FENCED_READBACK");
  const auto numeric = Read(control);
#ifdef _WIN32
  if (attempt.steadyQualification) {
    if (numeric.find("\"state\":\"BOOT_NUMERIC_QUALIFIED\"")==std::string::npos)
      return numeric;
    if (!attempt.t849WriteFile.empty()) {
      ObserveT849Writes();
      return Receipt("T849_PRIVATE_WRITES_CAPTURED","");
    }
    QualifySteady(attempt.steadyRole);
    return Receipt(attempt.steadyRole.empty() ? "STEADY_ROLES_QUALIFIED" : "STEADY_ROLE_QUALIFIED", "");
  }
  if (!attempt.m1SourceModule.empty()) return ProbeM1Source(candidate);
  if (attempt.presentBoot) {
    Checkpoint("BOOT_DIAGNOSTIC_WSI");
    const auto writeFrame = [&](const pc::Frame &frame) {
      fields << ",\"windowFrame\":{\"kind\":\"BOOT_DIAGNOSTIC\",\"acceptedRoot\":null,\"stage\":" << Quote(frame.stage)
             << ",\"width\":" << frame.width << ",\"height\":" << frame.height << ",\"format\":" << frame.format
             << ",\"images\":" << frame.images << ",\"imageIndex\":" << frame.imageIndex
             << ",\"acquire\":" << int(frame.acquire) << ",\"submit\":" << int(frame.submit)
             << ",\"fence\":" << int(frame.fence) << ",\"present\":" << int(frame.present)
             << ",\"imagePresent\":" << int(frame.imagePresent) << ",\"error\":" << int(frame.error)
             << ",\"stale\":" << (frame.stale ? "true" : "false")
             << ",\"presentEnterQpc\":\"" << frame.presentEnter << "\",\"presentExitQpc\":\"" << frame.presentExit
             << "\",\"pixel\":" << (frame.pixelRead ? std::to_string(frame.pixel) : "null") << '}';
    };
    const auto frame = pc::PresentBoot(vk, instance, candidate.physical, device, queue, candidate.family,
        surface, buffers[kControl].handle, buffers[kStaging].handle, buffers[kStaging].memory,
        buffers[kStaging].flags, attempt.window, attempt.current, [&](const pc::Frame &visible) {
          writeFrame(visible);
          publish(Receipt("BOOT_DIAGNOSTIC_PRESENT_QUEUED", ""));
        });
    if (frame.error != VK_SUCCESS) {
      writeFrame(frame);
      return Receipt(frame.stale ? "WINDOW_STALE" : "WSI_FAILED", frame.stage);
    }
    return Receipt("BOOT_DIAGNOSTIC_RETIRED", "");
  }
#endif
  return numeric;
}
} // namespace

std::string RunBoot(const Attempt &attempt, const Publish &publish) {
  Boot boot(attempt, publish);
  try {
    return boot.Run();
  } catch (const Failure &error) {
    return boot.Failed(error);
  } catch (const std::exception &error) {
    return boot.Failed(Failure{error.what()});
  }
}
} // namespace grs::g13vk
