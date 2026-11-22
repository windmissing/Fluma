# T824 · Windows PC GPU 仿真与呈现验收工程基线

2026-11-22 · architect · P4 · 目标1–6公共工程合同

**Windows 首选原生 Vulkan。** 当前 PC 的 Intel UHD Graphics 630 已在真实 Win32 surface 上通过队列资格，并用原 G13 BOOT SPIR-V 完成硬件执行、fence 与原字回读。Windows 产品窗口、非空业务帧和持续性能分别取证，现阶段尚无这三项的通过结果。 [本机原件](architecture-T824-boot-evidence-v1.json) · [PC 效果与性能口径](design-pc-effects-120fps-v1.md)

## 实际环境和可复用资产

| 域 | 已查到的实际结果或可复用范围 |
| --- | --- |
| 操作系统与适配器 | Windows 11 Pro build 26200。WMI 见 Intel UHD 630（PCI `8086:3e98`，驱动 `31.0.101.2140`）及 GameViewer 虚拟显示适配器。Vulkan 枚举两条 Intel iGPU 记录，同 device/driver UUID，LUID 不同。实际运行收据保选中的 VkPhysicalDevice 属性、LUID/UUID、驱动和队列，不能把枚举条数当独立显卡数。 |
| Windows Vulkan 预检 | 系统 `vulkaninfo` 枚举 GPU API 1.3.215、Win32 surface/swapchain、支持 graphics + compute + present 的 queue family 0。该 Intel 条目报告每阶段 storage buffers 200、单 storage range 1,073,741,820 B、shared 32,768 B、workgroup 1024 lanes、X 1024。制造工具列出的 surface 模式为 FIFO/IMMEDIATE。产品须对自身 HWND 与创建的 device 重新查实际值。 |
| 硬件执行窄证 | 用原 BOOT 的实际 SPIR-V `87980ac5…`、64×64 REST，Windows 薄适配只换 loader、HWND 与进程时钟身份。真实 HWND 为隐藏诊断窗。Vulkan 实际设备选 Intel，family 0。原八个 dispatch、queue submit/fence 返回均 0，CTRL seal `0xc0de1301`、error 0，OP65 +53 得 `0x00000001`，OP66 两轮掩码 `[0,0]`。本次单 pipeline 创建约 13.93 s，未隔离 driver cache，不能当冷暖基线。探针没有创建 swapchain 或调用 Present。 |
| 源码 | 优先复用当前 `gpu_resident/` 的 M1 source/quantum、wire/control/profile、固定 GPU 算法、审计与 scene packet 合同。G13 的 Vulkan BOOT、loader 函数分层、SPIR-V 生成和布局可抽出平台接缝。OHOS XComponent/NAPI、NativeWindow、EGL、平台输入桥与 Linux 时钟/ELF 身份由 PC adapter 接替。 [GPU 源码](../entry/src/main/cpp/gpu_resident/CMakeLists.txt) · [BOOT 实现](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp) |
| Host 工具与原 oracle | MSVC、CMake、系统 loader、仓内 glslang 可用。T496/T693 收据与有界 runner、T825 原 oracle、T826 输入/像素用例及 GPU 冷档 reader 可复用。T281 的 ANGLE/SwiftShader 和 T693 的 Windows host 结果各按原身份留存。硬件 GLES/ANGLE 仅作后端对照，须独立证明实际硬件适配器及相同 oracle，首版不做自动切换。 [T496 设施](test-report-T496-facility-v3.md) · [T693 原件](test-report-T693-v2.md) |

## 可实施的宿主接线

1. **独立 Windows x64 CMake/PE 目标。** 在 PC 构建入口拆 `Win32Window`（消息、尺寸、DPI、输入）、`PcGpuOwner`（同代提交、等待、呈现、退役）、`VulkanTransport`（缓冲、映射和同步）、共用 protocol/lease 与 GPU scene 数据。直接从 canonical shader/布局生成 SPIR-V，并记完整源 SHA/bytes、编译/反射、SPIR-V、PE SHA 和实际模块路径。首版固定一个 Vulkan 硬件 adapter 和一个支持 graphics/compute/present 的 family，用同一 queue 定义执行顺序。显式过滤 CPU/软件设备及仅虚拟显示设备。能力不符返回具名拒绝并保结果。
2. **XComponent 生命周期映射。** `OnSurfaceCreated` → 建 HWND 并发布 `WindowAvailable(windowEpoch, clientExtentPx, dpi)`，owner 对实际 HWND 建 `VkSurfaceKHR` 和 swapchain，达到 renderer Ready 后才允许实际业务帧。`OnSurfaceChanged` → `WM_SIZE/WM_DPICHANGED` 更新 geometry，在新表面合格前撤旧发布许可。最小化或零 extent 暂停呈现，复原重核实际 extent 和 format。`OnSurfaceDestroyed` → `WM_CLOSE` 先撤权、停止新提交和输入、由 owner 完成旧代 GPU 及 WSI 退役，再由 UI 线程 `DestroyWindow`。UI 始终 pump 消息，异步通知 owner，禁止双方同步等锁。换屏/DPI/resize 实际 geometry 参与 root/Present 身份，迟到结果只能退役。 [Win32 surface 和消息线程](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateWin32SurfaceKHR.html)
3. **保原提交和可见性。** PC 仅替后端 API。Controller 保原输入、permit/nonce、base/candidate、SOURCE/QUANTUM、容量、守恒、replay、完成尾、拒绝和原子提交。按实际绑定和 range 映射全部依赖：compute 写→后续 compute，PACK→真实 vertex/fragment 读，尾写→transfer staging→host，颜色附件→采样，acquire→写 swapchain image→present。每根冻结 descriptor 与资源代并核布局/反射及原 shader 范围。热路只读原有界 Completion，场/事件按明确冷请求同代 fence 后以原布局导出，非 coherent 映射按对齐 invalidate。CPU 只做平台控制、输入和证据，不计算物理场或生成替代帧。 [原绑定与可见性](architecture-T753-v1.md) · [Vulkan 同步示例](https://docs.vulkan.org/guide/latest/synchronization_examples.html)
4. **GPU 和 WSI 退役分开。** 按 swapchain image 持有呈现等待信号量。在重新 acquire 并等上对应信号，或实际启用并取得 present fence 等等价证据后复用。resize 使用原 swapchain 关联并延迟释放旧代，按 Khronos 的重建路径确认呈现资源已不再使用。当前预检未枚举 swapchain maintenance1/present wait，不能假定 present fence 可用。只有 queue submit fence 的代保留，禁止直接复用；出错保 `VK_*_KHR`、旧图标 STALE 或明确 FAULT。 [呈现信号量复用](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html) · [交换链重建](https://docs.vulkan.org/samples/latest/samples/api/swapchain_recreation/README.html)
5. **Windows 指针与重力输入。** 用客户区物理像素及最近真实已呈现的 viewport/容器内轮廓换算到原几何坐标，记 DPI、原事件、序号和 QPC receipt。Down/Move/Up/Cancel、捕获丢失、焦点丢失与越界分别保语义。按钮、拖杆或确定性输入脚本给等效重力方向，记录原请求与 GPU 实际接受值。脚本物理旋转另提供 `thetaUnwrapped/omega/alpha` 和轴/参考点，禁止以指针时序推断惯性或将镜头旋转报成仿真输入。QPC 原 ticks/frequency 与同进程 ns 同存，多个时钟先校域。移动端 proof 保历史，Windows 输入使用独立来源标记并沿原消费/撤权。 [Windows QPC](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps)

平台事件最小固定字段为 `kind, windowEpoch, geometryEpoch, widthPx, heightPx, dpi, sequence, receiptTicks`，输入另含 pointerId、相位和双坐标或物理运动区间。UI 只发布事件。owner 输出 `window/device/surface/swapchain/geometryEpoch + rootNonce + acceptedCommit + sceneGeneration + VkResult`，随帧绑定实际对象与时间。旧代输入和帧响应均按原交易边界拒绝。

## 分阶段验收与交接

| 阶段 | 最小可用和必存原件 |
| --- | --- |
| PC GPU 基础 | 新 PE、实际 adapter、GLSL/SPIR-V SHA、真实 HWND/queue、真实 compute→graphics、buffer readback 与可追踪提交。BOOT 或诊断图单列。 |
| 目标1／3／6 | 原 GPU SOURCE 和 QUANTUM 在代表性非空指针脚本与物理状态上提交；留原始输入、同根完成尾/事件、落床量、出生/来源账、被拒根不污染及复验。软件 GPU 或 host mock 不能给实际硬件结论。 |
| 目标2／4／5 | shader 生成完整背景和前景。真实窗口 capture 与原 scene token/geometry 共同证明堆积和粒子同帧、空箱和指针等效可控重力。记录 `window/surface/swapchain/config/geometry/generation`、presentId（若支持）、acquire/submit/present 原结果、FBO/像素及 acceptedRoot token。队列接受和实际显示事件/画面证据独立。同一旧状态的重复或插值、STALE、空诊断帧均另计。 |
| 持续性能 | 固定 PC、实际 120 Hz 以上输出能力、分辨率/画质/负载、电源与持续窗口。分别记录真正新呈现帧率、acceptedSimTime/wallTime、P95/P99/最坏帧、超期与丢帧、GPU 计算/渲染与 owner wall、内存峰值。当前枚举的 Intel 桌面模式为 **1920×1080 / 60 Hz**，可先采应用提交与吞吐，持续 120 FPS 的可见呈现保待测。120 FPS 是最新桌面设计目标，现有根期限、接受语义与场景阈值继续各自验证。 |

Developer 先交同窗口真实 shader 和一条非空 GPU 根，以小切片逐步接完整目标1–6。Windows 实现尚缺产品 PE、Win32 生命周期 owner、完整非空 transport 和 scene/swapchain 接线。T825/T826 可先复用原资产准备比对，实际 GPU/呈现用新工件原件。G13 的独立定量接力沿 T828。所有业务帧保 shader 渲染、GPU 驻留和原验收，禁止 Canvas 回退。 [T828 定量迁移](architecture-T828-v1.md)
