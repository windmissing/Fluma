# T731 · G13 Vulkan 最小同机对照

2026-11-20，architect，目标20 P5。不使用真机。**第一阶段验证真实设备管线与 BOOT，第二阶段取得同一非零流场的完整首根与有效呈现。** 冻结基于 G13 拆分工作量的 Vulkan 对照。T730 继续原 GLES 对照，两路按实测决定后续实施。 [实施检查清单](architecture-T731-checklist-v1.json)

## 现有入口、SDK 与实际工件

- G13 实际入口为 G13ResearchAbility → GpuResidentG13 → XComponent SURFACE → libgrs_g13。bridge 用 NativeXComponent 的创建、变更、销毁回调，引用 OHNativeWindow 后投递给原 owner，当前创建 EGL window surface。扫描 entry、tools、test 的 native/构建源未发现可直接复用的 Vulkan 渲染实现。可复用研究页、真实窗口、输入、生命周期、场数据及 oracle。
- 工程指定 SDK 26.0.0。实际 OpenHarmony Native 26.0.0.38，Vulkan header revision 309。SDK 声明 VK_USE_PLATFORM_OHOS、VK_OHOS_surface、VkSurfaceCreateInfoOHOS、vkCreateSurfaceOHOS；ARM64 链接库导出 surface、swapchain、compute pipeline 与 present 等入口。临时 ARM64 C 链接探针通过，DT_NEEDED 为 libvulkan.so、libc.so，未运行。SDK 的声明与链接库证据不覆盖实际 Maleoon 935F loader、扩展、队列、精度和 WSI。 [HarmonyOS Vulkan Surface](https://developer.huawei.com/consumer/en/doc/harmonyos-references/vulkan-guidelines/) [Khronos VK_OHOS_surface](https://docs.vulkan.org/refpages/latest/refpages/source/VK_OHOS_surface.html)
- T728/T729 提供同父 GLES 冻结角色。T730 现场另签 MONOLITH 89be7cc5…、SPLIT 707a751b…，离线核包 ELF 分别与 T729 的 BuildId 7c64adb3…、a9363c84… 一致，设备实际加载仍须按 PID/start 和映射核。MONOLITH ticket 177 只有编译 ENTER，报告约 5 分钟后应用退出，实际死因缺系统原件。内存空闲量与无 jscrash 均不足以定位原因。 [T730 原报告](test-report-T730-v2.md) [T729 部署收据](../test/reports/t729/deployment.json)
- T730 v2 的 G13_COMPUTE 原 cache 记录 shader SHA a93aecf8… 精确匹配 **23,834B BOOT**，原 259,025B MONOLITH 为 7563c614…。BOOT 原工厂阶段记冷 compile **49.850s**、link **66.635s**、build 墙时 **116.491s**；落盘收据为 301,961B。后一次同 key、同 binary SHA 的 BINARY_LOAD 区间 **5.645ms**，全 build **10.259ms**，有反射和准备成功。两套 cache 记录 packageSource=UNDECLARED，这只支持 BOOT 的角色身份和已有加载窄证。缺同进程加载和 cache 与 init ticket 的联据，暖新进程、六角色完成及首帧改善均未建立。ticket 209 和后采回执仍显示 INIT_ERROR_27、bootReadyNs=0，尚未有首个非零根或有效 Swap。 [T475 原 cache 记录](../test/reports/t491/t730b/split-cache/856a-warm.jsonl) [原配置故障](../test/reports/t491/t730b/s4.json)

## 实施面、角色及相同计算

developer 后续在隔离 G13 研究构建提供启动后端选项。每进程、每个真实窗口只启用一个 GPU 后端。接缝在 G13 bridge 的窗口代、Configure、Submit/Poll、AcquireScene/ReleaseScene、Present/Retire，沿用实际页面动作和输入回执。现有 controller 直接持有 GL transport/scene，复用其控制、协议和许可代码，按实际调用处加小范围后端接线。第一步做设备能力、真实窗口和 BOOT，第二步接通首个完整时间步，不引入 GLES/Vulkan 跨 API 资源共享或运行中自动切换。

直接消费 T728 SPLIT 生成器的八个角色。两臂保持同 opcode 划分、调度、容量和显示语义。BOOT 覆盖 64/65/66、两轮 FP32 和初始化 fence。正式非零首根须保 OP_00、OP_18、OP_37、OP_48、OP_62 的完整依赖闭包和原 vertex/fragment，保各角色原 66 探针、16 个 SOURCE 与 820 个 QUANTUM pass、PACK/AUDIT。生成的后端源码、SPIR-V 与 pipeline layout 各自带身份，原 algorithm/math/layout/gates、资源与计算定义冻结。 [T728 实际角色](developer-T728-v1.md) [G13 工程合同](architecture-T607-g13-engineering-v1.md)

1. **计算 ABI。** set 0 的 storage binding 0–3 按 Base 只读、Candidate、Work、Control，保原 std430 布局、索引和范围。每次 dispatch 的 uOp/uScope/uJ/uMicro/uBlock/uIndex 以 24B push constants 上传。uL[80] 放 set 0 binding 4 的独立只读 std140 UBO，数组 stride 16，块 1280B。CPU 在配置时复制原 layout 字与确定 padding，GPU 持续驻留推进。核每角色 descriptor/范围、原别名与所有权、block/push offsets、local size 及必要接口；正式产物用独立 SPIR-V 验证与反射。
2. **浮点与图形。** 保原 limb 算法、FP32/RNE、非有限/次正规分类及原 no-contraction 要求。真机核 FloatControls，检查需要的 SPIR-V decorations，跑每角色两轮原 bit 探针。探针、资源资格、已有 F05/F06/F07 原 oracle 分别登记。画面复用 GPU PACK、六个 UINT 实例属性及原色标，显式映射 varying、参数块、采样器和等价的顶点编号。核窗口 transform、Y 方向、viewport、取样、拓扑和表面格式，保原 offscreen 合成与显示语义。 [Khronos Float Controls](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html)
3. **构建与体积。** 临时从签包实际 GLSL 提取六 compute 与两 graphics，按上述接口调整声明、绑定、varying 和顶点索引，glslang -V --target-env vulkan1.1 八份静态通过。未运行 Vulkan 或数值检查。正式交付须再交固定编译器/flags、独立 SPIR-V 校验、反射和可复现生成收据。每单元完整文本仍按 **48 KiB / 49,152B** 过门。探针文本最大 OP_00 **48,855B**，余量 297B；生成的 compute SPIR-V 为 49,556–262,976B，另报体积与实际 pipeline 成本。文本尺寸不能替代设备管线耗时。直接生成 SPIR-V 的路径仍按原规范应用模块上限。 [离线尺寸与散列](architecture-T731-checklist-v1.json)

## 真机最小探针和安全退役

| 步骤 | 必须采集的原件与处置 |
| --- | --- |
| P0 · Loader 与能力 | 实际 libvulkan 装载路径/可获身份，系统/设备/API、vendorID/deviceID/driverVersion、cache UUID、instance/device 扩展和实启 feature，函数解析与原 VkResult。核 VK_KHR_surface、VK_OHOS_surface、VK_KHR_swapchain。首版取同时支持 compute/graphics/该 surface present 的 queue family。缺对应队列保 NO_COMBINED_QUEUE，缺平台扩展保 VULKAN_SURFACE_UNSUPPORTED。核四 SSBO、UBO、push、工作组、shared、存储范围、格式和精度。 |
| P1 · XComponent | 用真实 OHNativeWindow 创建 surface，核该 surface 的 family 支持、capabilities/formats/presentModes 与实际 extent/transform。按支持值建 swapchain，优先 FIFO。0×0 等真实可用窗口。采一次原 acquire/submit/present、窗口代和设备画面，诊断图单独登记。 |
| P2 · BOOT 与非零根 | 同档 REST 验证 64–66、两轮 bit 样本与原四页。再用 T730 冻结的同一非零用例，核 config/seed/input SHA 和真实动作，保 SOURCE 若存在的接受与后续完整 QUANTUM。复用 T721/T631 的速度、压力、黏性及冷档判据，取得同根有效 PRESENT 和画面。 |
| P3 · 撤权与失败 | 取后台、窗口销毁、resize 的回执。覆盖交换链过期或丢失、VK_SUBOPTIMAL_KHR、设备丢失、迟到 fence、旧 token。长调用保 PID/owner、最后 ENTER、观察上限及 CPU、内存和系统退出原件，未知原因保 UNKNOWN。 |

依赖映射要覆盖 compute 写→后续 compute 读/写、pack 写→vertex attribute 读、结果/冷档写→transfer→host、颜色附件写→采样读和 swapchain 的 acquire→绘制→PRESENT layout。以 core 同步路径为 1.1 最小实现，使用 sync2 等扩展前先核实启。按原冷读约定复制有界 tail/档案到 staging，完成 fence 后对 non-coherent memory 按 atom 对齐 invalidate。提交、渲染、readback 的 fence 证据和接受时刻分列，保原许可及 250ms root 期限。 [Khronos 同步示例](https://docs.vulkan.org/guide/latest/synchronization_examples.html)

Acquire 只在可控窗口内等待并保返回。提交等待 acquire 信号，render-finished semaphore 按 swapchain image 保存。重获同 image 并等待 acquire 信号或取得有效 present fence 后才复用。Present 前后均核 run/window/device/surface/swapchain/geometry/session/permission 代及同 root token，保总 VkResult 和适用的 per-swapchain 结果。最小对照只记 VK_SUCCESS 为有效，过期、次优、surface/device lost 具名保留并按真实代次处置。有效 Swap 是真实 vkQueuePresentKHR 成功返回及同已接受 root 和许可，设备实际显示另用画面佐证。 [Khronos Present](https://docs.vulkan.org/refpages/latest/refpages/source/vkQueuePresentKHR.html) [Vulkan WSI](https://docs.vulkan.org/spec/latest/chapters/VK_KHR_surface/wsi.html)

销毁先撤权并停发，再由原 owner 等实际 compute/graphics fence 和独立 WSI 退休证据，按 image/lease、swapchain、surface、device、instance、window 引用的依赖回收。优先核设备实际启用 VK_EXT_swapchain_maintenance1 与 present fence，或原件验证的等价路径。无扩展时，同 image 重新 acquire/等待只证明该 image 可复用；旧链按 Khronos recreation 逻辑延迟整代销毁，直到有证据可退役。submit fence 或 WaitIdle 均不足以证明 WSI 已退休。缺证保整代不可复用，登记 WSI_RETIRE_UNPROVEN；最小对照只在独立进程启动时选择后端，旧进程退出、新 surface 可用后再进另一臂。 [Khronos semaphore 复用](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html) [Khronos swapchain 重建](https://docs.vulkan.org/samples/latest/samples/api/swapchain_recreation/README.html)

## 冷暖配对和实施触发

- **实验条件。** 同一台 935F/同系统，GLES SPLIT 与 Vulkan 使用相同角色源、profile、配置/seed/输入、真实页面尺寸/状态和激活动作。分别记录 GL 和 Vulkan 原驱动信息并关联同一设备。各自冷应用缓存与新进程，设备串行，系统隐式缓存未知显式记录。T729/T730 已匹配的 GLES 样本直接复用，MONOLITH 保为原编译长尾参考。
- **实测阶段。** 用 T727 monotonic ns 和 ENTER/RETURNED/EXIT，记录 Vulkan shader module、compute/graphics pipeline、准备、READY、首 accepted root、真实 PRESENT 与稳态有效帧。glslang 离线耗时计工具链。保存各角色 CPU/墙时、资源峰值，分列 cache 读入、pipeline 创建、首次 dispatch/draw。过程只显示角色/阶段/等待时间，未返回按原有界观察记截尾，保退出、失效与所有尝试分母。
- **暖启。** 取实际 vkGetPipelineCacheData 和成功落盘收据，同后端新进程绑定 vendor/device、pipelineCacheUUID、驱动、SPIR-V/layout/flags，校验 header/长度/SHA。坏条目保原拒并用空 cache 一次重试。仅凭有效的 VkPipelineCreationFeedback 原件登记应用 cache hit；缺反馈记 blob 已提供、实测创建时间及 hit UNKNOWN。T475 GL binary 与 Vulkan blob 独立。 [Khronos cache header](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheHeaderVersionOne.html) [Khronos 创建反馈](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCreationFeedbackFlagBits.html)
- **判定器。** T729 的 test/t729/measure.py 显式只接受 G13 GLES，首 Swap 和 binary 判据绑定 GL。tester 独立补 Vulkan 分支/夹具，复用 T727 schema、原件 SHA、事件排序/scope 与 T721/T631 判据。首帧按原 VkResult、present 状态、已接受 token 与撤权事件核，VK_SUCCESS=0 保原值。现有 GLES 规则维持。部署收据要核实际加载、实际管线输入与逐角色映射。

| T730 结果 | 下一张卡依据 |
| --- | --- |
| SPLIT 冷启、非零首根、有效画面和原数值门成立，首帧达约定预算 | GLES 沿实际产物推进，Vulkan 保留可复用准备。 |
| 当前 T730 v2 · BOOT 已有编译和 binary 载入窄证，配置止于 27 | 优先为 GLES 的 BOOT 初始化尾做独立归因。绑实际加载、role SHA、ticket，留原 INIT_FENCE_POLL、INIT_TAIL_VALID 退出的 cold[255]/cold[7] 和后继 INIT_FP32_RESULT，必要时保 coldAudit 中 sin/cos、subnormal 与 tie 的原 words、双轮 cold[200..215] 及采样身份。现有 27 汇总值无法定位具体谓词。继续既有 GLES 对照；Vulkan 保留可选，若同一资格在 GLES 反复实证失败再做同探针比较。 |
| SPLIT 到 READY，首根/数值/呈现出现明确失败 | 修同 scope 的已证停点。呈现/会话的工程失败独立归因。 |
| SPLIT 仍出现长编译、实际退出或首帧反复超预算，且工件/加载身份成立 | 后续可派 developer 做 Vulkan P0/P1/BOOT，先交阶段，随后接完整首步。只有原 oracle、有效画面、冷启收益及持续有效 30 FPS 通过，才有证据扩大后端实现。 |
| 缺 T730 加载身份、阶段或终态原件 | 复用 T729 和 T730 原出口补实际缺项。T731 离线准备可独立交付，Vulkan 真机支持为 NOT_MEASURED。 |

developer 实施与编译，tester 离线核产物、判定器及签包，effector 串行设备和画面采证，architect 依据原件分流。此卡不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。目标20 P5，GPU 驻留，CPU 停投，前后景仅 XComponent+shader，禁止 Canvas 回退。
