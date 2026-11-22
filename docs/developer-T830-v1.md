# T830 Windows Vulkan 窗口阶段交付

Windows x64 PE 已接通真实 HWND、硬件 surface、graphics/compute/present 同队列、swapchain 和 QPC。UI 线程处理 resize、DPI、最小化、撤权与退出，owner 完成处理后 UI 销毁窗口。指针事件记录原像素、DPI、几何代和 receipt，尚未进入业务 SOURCE。

自测在 Intel UHD Graphics 630 上以原 G13 BOOT SPIR-V 执行。片元着色器直接读 BOOT 的 GPU CTRL 封印，swapchain 中心像素经 transfer 和 fence 回读为 0xff00ff00。784×461、两个交换链图像，acquire、graphics submit、fence、Present 和该 image 返回均为 0。当前桌面模式是 60 Hz。单帧诊断先等 graphics fence 再调用 Present，不把 gpu fence 解释为真实扫描输出。 [原始 JSONL](../test/reports/t830/window.jsonl) · [工件和设备收据](../test/reports/t830/receipt.json)

在 MSVC x64 开发者环境中复现。G13_GLSLANG 指向 glslang，VULKAN_HEADERS 指向包含 vulkan 目录的 SDK include 路径。

~~~powershell
cmake -S pc/vulkan -B .codex/local-test/t830-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DG13_GLSLANG=<glslang> -DVULKAN_HEADERS=<include>
cmake --build .codex/local-test/t830-ninja
.codex/local-test/t830-ninja/hello_world_pc.exe --once
~~~

另外复用原 M1 SOURCE/QUANTUM 组装式，将 scalar/uvec2 接口迁至 std140，验证 SPIR-V 地址及四 storage binding 和 UBO，保静态哈希及逐成员 offset。复现先运行 python test/gpu_resident/prepare_t280_commit.py，再运行 python pc/vulkan/check_m1_spirv.py --assembled .codex/local-test/t280 --compiler <glslang> --out .codex/local-test/t830-m1-static。

未完成 M1 Vulkan 事务执行、非空 SOURCE/QUANTUM、同代冷档及 acceptedRoot 到业务呈现。窗口证据类型为 BOOT_DIAGNOSTIC，acceptedRoot 为 null。接续需把原 M1 permit、常量/资源范围、同代 fence 和 GPU completion gate 接入实际 owner，再由 T831 核真实首业务帧。120 Hz 可见验证继续待具备能力的显示模式及业务输出。
