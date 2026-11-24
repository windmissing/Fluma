#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "probe_shaders.inc"

namespace {

void check(VkResult result, const char* call) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(call) + ":" + std::to_string(result));
}

void line(const char* name, const std::string& value) {
    std::cout << name << '=' << value << std::endl;
}

#define VULKAN_FUNCTIONS(F) \
    F(CreateInstance) F(DestroyInstance) F(EnumeratePhysicalDevices) F(GetPhysicalDeviceProperties2) \
    F(GetPhysicalDeviceQueueFamilyProperties) F(GetPhysicalDeviceMemoryProperties) \
    F(CreateDevice) F(DestroyDevice) F(GetDeviceQueue) F(CreateBuffer) F(DestroyBuffer) \
    F(GetBufferMemoryRequirements) F(AllocateMemory) F(FreeMemory) F(BindBufferMemory) F(MapMemory) F(UnmapMemory) \
    F(CreateDescriptorSetLayout) F(DestroyDescriptorSetLayout) F(CreatePipelineLayout) F(DestroyPipelineLayout) \
    F(CreateShaderModule) F(DestroyShaderModule) F(CreateComputePipelines) F(DestroyPipeline) \
    F(CreateDescriptorPool) F(DestroyDescriptorPool) F(AllocateDescriptorSets) F(UpdateDescriptorSets) \
    F(CreateCommandPool) F(DestroyCommandPool) F(AllocateCommandBuffers) F(BeginCommandBuffer) \
    F(CmdBindPipeline) F(CmdBindDescriptorSets) F(CmdDispatch) F(CmdPipelineBarrier) F(EndCommandBuffer) \
    F(CreateFence) F(DestroyFence) F(QueueSubmit) F(WaitForFences) F(DeviceWaitIdle)

struct Api {
    HMODULE library = nullptr;
#define DECLARE(name) PFN_vk##name name = nullptr;
    VULKAN_FUNCTIONS(DECLARE)
#undef DECLARE
    Api() {
        library = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!library) throw std::runtime_error("VULKAN_LOADER_MISSING");
        try {
#define LOAD(name) \
            name = reinterpret_cast<PFN_vk##name>(GetProcAddress(library, "vk" #name)); \
            if (!name) throw std::runtime_error("VULKAN_LOADER_MISSING_" #name);
            VULKAN_FUNCTIONS(LOAD)
#undef LOAD
        } catch (...) {
            FreeLibrary(library);
            throw;
        }
    }
    ~Api() { FreeLibrary(library); }
    Api(const Api&) = delete;
    Api& operator=(const Api&) = delete;
};

struct Adapter {
    VkPhysicalDevice id = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFloatControlsProperties fp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
    uint32_t family = 0;
};

struct Context {
    Api vk;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    Adapter adapter{};

    void open() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "Fluma G13 FP32 diagnostic";
        app.apiVersion = VK_API_VERSION_1_2;
        VkInstanceCreateInfo make{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        make.pApplicationInfo = &app;
        check(vk.CreateInstance(&make, nullptr, &instance), "vkCreateInstance");
        uint32_t count = 0;
        check(vk.EnumeratePhysicalDevices(instance, &count, nullptr), "vkEnumeratePhysicalDevices");
        std::vector<VkPhysicalDevice> ids(count);
        check(vk.EnumeratePhysicalDevices(instance, &count, ids.data()), "vkEnumeratePhysicalDevices");
        for (auto id : ids) {
            Adapter current{};
            current.id = id;
            VkPhysicalDeviceProperties2 info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            info.pNext = &current.fp;
            vk.GetPhysicalDeviceProperties2(id, &info);
            current.properties = info.properties;
            const auto type = current.properties.deviceType;
            if (type != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && type != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                continue;
            uint32_t n = 0;
            vk.GetPhysicalDeviceQueueFamilyProperties(id, &n, nullptr);
            std::vector<VkQueueFamilyProperties> families(n);
            vk.GetPhysicalDeviceQueueFamilyProperties(id, &n, families.data());
            bool compute = false;
            for (uint32_t i = 0; i < n; ++i) {
                if (families[i].queueCount && (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    current.family = i;
                    compute = true;
                    break;
                }
            }
            if (compute && (!adapter.id || (type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    adapter.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)))
                adapter = current;
        }
        if (!adapter.id) throw std::runtime_error("NO_HARDWARE_COMPUTE_ADAPTER");
        line("GPU", adapter.properties.deviceName);
        line("DRIVER", std::to_string(adapter.properties.driverVersion));
        line("TYPE", adapter.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated");
        if (adapter.properties.apiVersion < VK_API_VERSION_1_2) throw std::runtime_error("VULKAN_1_2_REQUIRED");
        if (!adapter.fp.shaderRoundingModeRTEFloat32) throw std::runtime_error("RTE32_UNSUPPORTED");
        if (!adapter.fp.shaderSignedZeroInfNanPreserveFloat32) throw std::runtime_error("SIGNED_ZERO_INF_NAN32_UNSUPPORTED");

        float priority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = adapter.family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        check(vk.CreateDevice(adapter.id, &deviceInfo, nullptr, &device), "vkCreateDevice");
        vk.GetDeviceQueue(device, adapter.family, 0, &queue);
    }
    ~Context() {
        if (device) vk.DestroyDevice(device, nullptr);
        if (instance) vk.DestroyInstance(instance, nullptr);
    }
};

struct Pipeline {
    Context& c;
    VkDescriptorSetLayout descriptors = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkShaderModule module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkResult create(const uint32_t* words, size_t bytes, bool op48) {
        std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
        for (uint32_t i = 0; i < bindings.size(); ++i)
            bindings[i] = {i, i == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        ds.bindingCount = op48 ? 5 : 1;
        ds.pBindings = bindings.data();
        check(c.vk.CreateDescriptorSetLayout(c.device, &ds, nullptr, &descriptors), "vkCreateDescriptorSetLayout");
        const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, 24};
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &descriptors;
        pl.pushConstantRangeCount = op48 ? 1 : 0;
        pl.pPushConstantRanges = op48 ? &push : nullptr;
        check(c.vk.CreatePipelineLayout(c.device, &pl, nullptr, &layout), "vkCreatePipelineLayout");
        VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        sm.codeSize = bytes;
        sm.pCode = words;
        check(c.vk.CreateShaderModule(c.device, &sm, nullptr, &module), "vkCreateShaderModule");
        VkComputePipelineCreateInfo cp{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        cp.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                    VK_SHADER_STAGE_COMPUTE_BIT, module, "main", nullptr};
        cp.layout = layout;
        cp.basePipelineIndex = -1;
        if (op48) line("STAGE", "OP48_CREATE");
        const auto begin = std::chrono::steady_clock::now();
        const VkResult result = c.vk.CreateComputePipelines(c.device, VK_NULL_HANDLE, 1, &cp, nullptr, &pipeline);
        if (op48) {
            const auto elapsed = std::chrono::steady_clock::now() - begin;
            line("OP48", result == VK_SUCCESS ? "OK" : "VK_ERROR(" + std::to_string(result) + ")");
            line("MS", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
        }
        return result;
    }
    ~Pipeline() {
        if (pipeline) c.vk.DestroyPipeline(c.device, pipeline, nullptr);
        if (module) c.vk.DestroyShaderModule(c.device, module, nullptr);
        if (layout) c.vk.DestroyPipelineLayout(c.device, layout, nullptr);
        if (descriptors) c.vk.DestroyDescriptorSetLayout(c.device, descriptors, nullptr);
    }
};

struct Probe {
    Context& c;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDescriptorPool descriptors = VK_NULL_HANDLE;
    VkCommandPool commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool submitted = false;

    void run(Pipeline& pipeline) {
        constexpr std::array<uint32_t, 16> inputs{
            0u, 0x80000000u, 1u, 0x807fffffu, 0x00800000u, 0x00ffffffu, 0x01000000u, 0x3f800000u,
            0x81000000u, 0x7f800000u, 0x7fc00000u, 0x80800000u, 0x3f000000u, 0x3f7fffffu, 0x33800000u, 0x7e800000u};
        constexpr std::array<uint32_t, 21> expected{
            0u, 0u, 0u, 0u, 0u, 0u, 0x01000000u, 0x3f800000u, 0x81000000u, 0x7f800000u,
            0x7fc00000u, 0u, 0u, 0u, 0u, 0u, 0x3f000000u, 0x3f800000u, 0u, 3u, 0x916f0032u};
        constexpr VkDeviceSize bytes = (inputs.size() + expected.size()) * sizeof(uint32_t);
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size = bytes;
        bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        check(c.vk.CreateBuffer(c.device, &bi, nullptr, &buffer), "vkCreateBuffer");
        VkMemoryRequirements req{};
        c.vk.GetBufferMemoryRequirements(c.device, buffer, &req);
        VkPhysicalDeviceMemoryProperties props{};
        c.vk.GetPhysicalDeviceMemoryProperties(c.adapter.id, &props);
        constexpr auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        uint32_t type = 0;
        while (type < props.memoryTypeCount && (!(req.memoryTypeBits & (1u << type)) ||
                (props.memoryTypes[type].propertyFlags & flags) != flags))
            ++type;
        if (type == props.memoryTypeCount) throw std::runtime_error("NO_COHERENT_PROBE_BUFFER");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = req.size;
        allocation.memoryTypeIndex = type;
        check(c.vk.AllocateMemory(c.device, &allocation, nullptr, &memory), "vkAllocateMemory");
        check(c.vk.BindBufferMemory(c.device, buffer, memory, 0), "vkBindBufferMemory");
        check(c.vk.MapMemory(c.device, memory, 0, bytes, 0, &mapped), "vkMapMemory");
        std::memset(mapped, 0xff, static_cast<size_t>(bytes));
        std::memcpy(mapped, inputs.data(), sizeof(inputs));

        const VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dp.maxSets = 1;
        dp.poolSizeCount = 1;
        dp.pPoolSizes = &poolSize;
        check(c.vk.CreateDescriptorPool(c.device, &dp, nullptr, &descriptors), "vkCreateDescriptorPool");
        VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        da.descriptorPool = descriptors;
        da.descriptorSetCount = 1;
        da.pSetLayouts = &pipeline.descriptors;
        VkDescriptorSet set;
        check(c.vk.AllocateDescriptorSets(c.device, &da, &set), "vkAllocateDescriptorSets");
        const VkDescriptorBufferInfo binding{buffer, 0, bytes};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &binding;
        c.vk.UpdateDescriptorSets(c.device, 1, &write, 0, nullptr);

        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.queueFamilyIndex = c.adapter.family;
        check(c.vk.CreateCommandPool(c.device, &pool, nullptr, &commands), "vkCreateCommandPool");
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool = commands;
        ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ca.commandBufferCount = 1;
        VkCommandBuffer cmd;
        check(c.vk.AllocateCommandBuffers(c.device, &ca, &cmd), "vkAllocateCommandBuffers");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(c.vk.BeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");
        c.vk.CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
        c.vk.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1, &set, 0, nullptr);
        c.vk.CmdDispatch(cmd, 1, 1, 1);
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        c.vk.CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                               0, 1, &barrier, 0, nullptr, 0, nullptr);
        check(c.vk.EndCommandBuffer(cmd), "vkEndCommandBuffer");
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        check(c.vk.CreateFence(c.device, &fi, nullptr, &fence), "vkCreateFence");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        check(c.vk.QueueSubmit(c.queue, 1, &submit, fence), "vkQueueSubmit");
        submitted = true;
        check(c.vk.WaitForFences(c.device, 1, &fence, VK_TRUE, 5000000000ull), "vkWaitForFences");
        submitted = false;
        const auto* actual = static_cast<const uint32_t*>(mapped) + inputs.size();
        for (size_t i = 0; i < expected.size(); ++i) {
            if (actual[i] != expected[i]) {
                std::cerr << std::hex << "word=" << std::dec << i << std::hex
                          << " actual=0x" << actual[i] << " expected=0x" << expected[i] << std::endl;
                throw std::runtime_error("PROBE_WORD_" + std::to_string(i));
            }
        }
    }
    ~Probe() {
        if (submitted) c.vk.DeviceWaitIdle(c.device);
        if (fence) c.vk.DestroyFence(c.device, fence, nullptr);
        if (commands) c.vk.DestroyCommandPool(c.device, commands, nullptr);
        if (descriptors) c.vk.DestroyDescriptorPool(c.device, descriptors, nullptr);
        if (mapped) c.vk.UnmapMemory(c.device, memory);
        if (buffer) c.vk.DestroyBuffer(c.device, buffer, nullptr);
        if (memory) c.vk.FreeMemory(c.device, memory, nullptr);
    }
};

} // namespace

int main(int argc, char** argv) {
    const bool probeOnly = argc == 2 && std::string(argv[1]) == "--probe-only";
    if (argc != 1 && !probeOnly) return 2;
    try {
        line("STAGE", "ADAPTER");
        Context context;
        context.open();
        line("STAGE", "PROBE");
        {
            Pipeline pipeline{context};
            check(pipeline.create(kProbe, sizeof(kProbe), false), "probe_pipeline");
            Probe test{context};
            test.run(pipeline);
        }
        line("PROBE", "PASS");
        if (probeOnly) return 0;
        line("STAGE", "OP48_SETUP");
        Pipeline op48{context};
        const VkResult result = op48.create(kOp48, sizeof(kOp48), true);
        return result == VK_SUCCESS ? 0 : 1;
    } catch (const std::exception& error) {
        line("ERROR", error.what());
        return 1;
    }
}
