# T834 · 目标 13 Windows Vulkan OP_00 建管线停滞归因

2026-11-22 · architect · P3 · Windows PC 工程合同

**首个已定位停点是 OP_00 的 `vkCreateComputePipelines` 在观察窗口内未返回。** 同设备 BOOT 已过原 OP65 和双轮 OP66。稳态 OP_00 尚未执行任何 GPU 命令。优先按角色单独量测，首轮保留原模块作对照，随后验证从同一份 GLSL 依赖闭包切出的三个小角色。T833 可据此续做工程隔离，业务根仍须等待实际装载角色分别过门。 [T833 交付](developer-T833-v1.md) · [静态复核及原件身份](architecture-T834-evidence-v1.json)

## 已证实的事实与因果边界

| 原件 | 核对结果 |
| --- | --- |
| 首轮 `steady-initial.jsonl` | Windows x64 PE SHA `c3ec8563…`，PID 20752 / owner TID 78476。最后一行是 `STEADY_PIPELINE/OP_00`，该版本无调用边沿。进程龄 271.565→305.899 s 的 34.334 s 中，同线程 CPU 增加 32.594 s，约占单逻辑核的 95%。该组样本只给进程观察时间和该线程活动量。进程被外部终止，无 owner 退役回执。 |
| 复跑 `steady-progress.jsonl` | Windows x64 PE SHA `cd82962b…`，PID 37668 / owner TID 7672，10 MHz QPC，`ENTER=15162861922343`，对应相同 OP_00 SPIR-V `ce59f3ea…`。原代码在模块创建成功且核嵌入 SHA 后写 ENTER，然后实际调用创建，再写 EXIT。原件截至 ENTER，`vkResult=null`，外部 supervisor 强制结束。未采到同域结束边沿，无法给调用耗时上界或返回码。 |
| 硬件及模块 | 复算两件 PE 与两份 JSONL 的完整 SHA。两件 PE 均唯一包含同一 BOOT 和全部五个稳态 SPIR-V 原字节。均选 Intel UHD 630 `8086:3E98` 的条目 0、相同 LUID、device/driver UUID 及 pipeline cache UUID，驱动 31.0.101.2140（raw `1656924`），effective Vulkan 1.2、queue family 0。BOOT 建管线分别 13.642/13.384 s，OP65 QUALIFIED、两轮 OP66 `[0,0]`。T835 另证相同硬件的诊断 Present，诊断帧 `acceptedRoot=null`。 |

首轮高线程 CPU 与复跑的 API 边沿支持优先隔离驱动端创建工作。缺 CPU 栈、驱动内部阶段和返回码，优化器复杂度是待验假设，长编译、驱动内部循环或同步问题仍需分流。源码已在 glslang 生成并静态校验，停点在 Vulkan 创建阶段，早于稳态 OP66 dispatch、queue submit、fence、业务 SOURCE/QUANTUM 和 swapchain 绘制。305.899 s 是进程龄，第二轮无实测 OP_00 耗时。现行调用的 `flags=0`、无 specialization、`pipelineCache=VK_NULL_HANDLE`，驱动内部缓存状态未知。 [两轮原件与监督记录](../test/reports/t833/receipt.json) · [实际调用](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp) · [T835](test-report-T835-v1.md) · [Vulkan 创建接口](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateComputePipelines.html)

## 最小隔离与等价切片

继续用 `pc/vulkan/build_roles.py` 的同一 Vulkan 转换及验证，接 `tools/g13/split_shader.py` 的依赖闭包。复核用 T750 `split_group` 对冻结 `role_OP_00.comp` 拆开再合回，重建的完整 PC GLSL 和 SPIR-V 与 T833 按字节相等。离线实际编译以下各臂，保原 opcode arm 和传递函数、原私有 OP66、set0 bindings 0–4、layout 数组 stride 16、24 B push、64 lanes、三项 FP32 执行模式和必要 `NoContraction`。源码逐实际单元 ≤49,152 B，SPIR-V 体积另记。 [原生成器](../tools/g13/split_shader.py) · [原角色拆分工具](../tools/architecture/analyze_t750.py) · [Windows 编译转换](../pc/vulkan/build_roles.py) · [T731](architecture-T731-v1.md)

| 臂 | 生产 opcode | GLSL B | SPIR-V B | SPIR-V 指令 / 函数调用指令 |
| --- | --- | ---: | ---: | ---: |
| 原 `OP_00` | 0–17 | 49,013 | 262,996 | 17,049 / 498 |
| 第一切片 | 0–5 | 13,001 | 56,552 | 3,598 / 47 |
| 第二切片 | 6–11 | 35,684 | 183,172 | 11,846 / 413 |
| 第三切片 | 12–17 | 30,560 | 154,368 | 10,101 / 355 |
| 第二片内的复查臂 | 6–8 / 9–11 | 26,023 / 18,941 | 126,476 / 88,944 | 8,246 / 5,670 |
| 第二片单算子定位 | 7，`ROOT_FINAL_FIELD_AUDIT` | 22,828 | 108,832 | 7,129 / 307 |

上述计数包括 SPIR-V 中的声明和调试指令，作为同链静态比较。三个主切片共同覆盖原 0–17 一次，其他 `OP_18/37/48/61` 保留原内容，生产映射仍由实际 opcode 唯一选角色，私有 OP66 仍按角色直绑，正常 BOOT 的 64/65/66 映射保持。沿 T753 以完全相同的 16/820 行对照 descriptor tuple、六 push、ordinal、scope、candidate/replay/pack ranges 与依赖。按单条连续命令流计算，SOURCE 的 pipeline 使用段为 8→8，QUANTUM 为 99→101，实际切换和耗时留 PC 量测。OP_18/37/48 的静态规模也在约 17k 条指令，OP_00 解除后逐个确认，禁止推定其余角色可用。 [T607 原操作表](architecture-T607-g13-dispatch-v1.json) · [T753 绑定和可见性](architecture-T753-v1.md)

## Developer 执行顺序

1. **先交一个可独立执行的角色诊断。** 为 T833 的诊断入口增加 `--qualify-steady-role <role>` 和显式标志选择。复用同 device/layout、BOOT 及前后边沿，单次创建 1 个 pipeline，同设备一次只跑一个臂。先测现有轻角色 OP_61 作为稳态路径正控，再在 `flags=0` 测三个主切片。已有两轮原 `OP_00` 截尾作基准。如需统一计时，原字节只追加一轮限时对照。第二片未返回时先试 `6–8` 与 `9–11`，再据结果定位 7。若要区分优化路径，同一 SPIR-V/layout 另跑 `VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT` 单变量对照，分别记录创建、实际 OP66 和 GPU 时间。该标志可能降低创建时间，其返回及运行性能均以实测为准。不同驱动或 adapter 只给因果对照。 [Vulkan 创建标志](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCreateFlagBits.html)
2. **给每次调用可观测终态。** 诊断 supervisor 持有自己的子进程句柄，用 PID + process start + attemptId 定位。打印并逐行刷新 `shaderModule ENTER/EXIT → pipeline ENTER/EXIT → queue submit → fence → OP66 → retire`，每次给 QPC/frequency、模块及布局 SHA、flags、cache 状态、真实结果、线程 CPU 与所选 GPU 身份。首轮诊断预算设为 pipeline ENTER 起 60 s 观察、子进程启动起 120 s 墙钟上限。约每 5 s 采 CPU/线程状态，具备环境时附同线程 Win32 栈，另存可用性。超出观察窗仅记 `CALL_UNRETURNED` 和最后 QPC/样本，先撤资格再由 supervisor 收自己的诊断子进程，登记实际 exit/是否强制。禁止跨线程销毁仍在调用中的 Vulkan 对象，禁止把进程退出当 owner 退役。API 返回错误和监督终止分别记账。T830 继续走其窗口推进路径。 [原阶段接线](../pc/vulkan/main.cpp) · [T731 退役合同](architecture-T731-v1.md)
3. **把建管线与业务期限分开。** 配置冷阶段逐角色建管线并完成资格，窗口消息线程保持可响应。尚未完成的配套显示为 `CONFIG_PENDING` 或实际故障，许可计时从原业务条件真正满足后开始。正式 owner 缓存已成功创建并过资格的活句柄与这次实际映射，保同一个 Vulkan device 和生命周期。诊断子进程的成功仅转交收据或兼容缓存字节。若实测使用 Vulkan 持久缓存，原子落盘并核 header、UUID、驱动、layout、模块及 flags，分别记录写入、独立新进程实际读取、应用命中反馈及创建结果。在 Vulkan 1.3 核心路径或已启用对应 EXT 的路径上，只有 feature 已启用才用 `FAIL_ON_PIPELINE_COMPILE_REQUIRED` 探测。`VK_PIPELINE_COMPILE_REQUIRED` 是本次需要编译的原结果。反馈也仅在实际支持且 `VALID` 时消费。 [Vulkan 缓存头](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheHeaderVersionOne.html) · [缓存控制特性](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevicePipelineCreationCacheControlFeatures.html) · [创建反馈](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCreationFeedback.html)

## T833 恢复条件

- **阶段一，继续接线。** Developer 可立即沿最小隔离实现与原静态 root plan 并行准备。首个小角色取得 `VK_SUCCESS`、实际 submit/fence、原 OP66 两轮原字和 `mismatchMasks=[0,0]` 时，登记该角色在本构建和设备上的证据。切片等价再逐项核原组布局、对应 opcode 实际写集、错误优先级及跨角色同步。原模块与切片的同输入差分若缺原模块硬件结果，留明范围并依原冻结定义、独立 oracle 及原始 GPU 业务写集继续验证。单角色/BOOT 成功仅覆盖所得范围。
- **阶段二，开放非零业务根。** 实际映射中的所有角色都能建管线、各自通过原 OP66 双轮精确位型和对应数学/资源资格之后，在同一 PC owner 核真实 pulse/permit、输入和完整非零 SOURCE/QUANTUM。保原 250 ms、33,333,333 ns freshness、T753 的 ACTUAL/REPLAY 隔离、原 16/820 派发、单根 fence/1024 B `Completion256`、PACK/AUDIT 与接受门。只从实际被接受根导出 ROOT/FULL、`GridView` 和流场窗口证据，按 T828/T721 核速度、压力、黏性原门及独立参考。GPU 驻留，保 shader 和实际窗口，禁止 Canvas 与 CPU 物理回退。目标 14 等该可信场的实际交付。 [T828 数值及恢复边界](architecture-T828-v1.md) · [T721](test-report-T721-v1.md)

本轮核原件及静态编译切片。三片的驱动创建时间、GPU 资格和非空根仍待实际 PC 测量。
