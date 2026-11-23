# T841 · G13 单 opcode 建管线阻塞复核与工程分流

2026-11-23 · architect · Windows x64 Vulkan · 目标 13

**OP_00 的停滞已缩小到单独的 opcode 7 模块。** 在 Intel UHD 630 上，默认 flags 和 `DISABLE_OPTIMIZATION` 均进入 `vkCreateComputePipelines` 并在 60 s 观察窗内未返回。先分别验证原字段审计与 `drive()` 的两个可达闭包。对 12–17 独立排查，按 `12–14 / 15 / 16 / 17` 隔离。T833 可继续工程验证，目标 13 的稳态业务根仍待资格与完整实测。 [原诊断目录](../test/reports/t833/role-isolation) · [T834 基线](architecture-T834-v1.md) · [本轮机器复核](architecture-T841-evidence-v1.json)

## 原件和结论边界

复算 8 份 JSONL SHA、实际 x64 PE `dbd5bb4f…` 及所执行的嵌入 SPIR-V 字节 SHA。各运行是独立进程，同一 Intel UHD 630 条目、相同 LUID/device UUID/driver UUID、驱动 31.0.101.2140、effective Vulkan 1.2、queue family 0、同一 layout SHA `b9390359…`。每个子进程实际 BOOT OP65/OP66 通过，稳态 `vkCreateShaderModule` 也返回 0。两轮 OP7 的模块均为 `3d8817dc…`，`pipelineCache=null`，未控制驱动内部缓存状态。QPC 频率为 10 MHz，PID、进程创建 tick 和 owner TID 可与各自线程采样核对。 [flags=0 收据](../test/reports/t833/role-isolation/OP_00_7_7.receipt.json) · [flags=1 收据](../test/reports/t833/role-isolation/OP_00_7_7-noopt.receipt.json)

| 稳态对照 | 实际结果 |
| --- | --- |
| `OP_61`、`0–5`、`9–11` | pipeline 分别 **0.641 / 0.800 / 2.132 s**，同角色实际 submit/fence 返回 0，双轮 OP66 `[0,0]`，子进程正常退出。证据覆盖上述模块的建管线及资格探针。 |
| `6–11`、`6–8` | 到达 pipeline ENTER，60 s 观察窗内无 EXIT/VkResult，未提交稳态探针。 |
| `12–17` | 同样到达 ENTER 后未返回。该组不含 opcode 7，须独立解除。 |
| 仅 `7`，flags=0 / 1 | PID/TID 分别 78544/89540、85896/44184。两轮均止于 pipeline ENTER。后段约 50.3/50.4 s 采样间隔中，owner CPU 增 48.72/48.19 s，约 97%/96% 单核。监督端强制收回自己的子进程，无稳态 submit/fence、VkResult 或正常退役原件。 |

60 s 是从 ENTER 开始的诊断预算，120 s 是子进程总墙钟预算。收据最后 CPU 样本的进程龄约 70.5 s，两轮调用实际完成时间未知。证据指向创建路径内的 CPU 工作，缺线程栈和驱动内部阶段，尚不能归因到某个编译器 pass。关闭优化未在该观察窗内解除阻塞。该 Vulkan flag 的语义不能代替实际返回值或运行证明。 [监督器](../pc/vulkan/qualify_role.py) · [Vulkan 标志](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCreateFlagBits.html)

## 已离线验证的隔离模块

复用 T834/T833 的闭包生成及 Vulkan 变换，原 OP7 GLSL 和 SPIR-V 重编后逐字节相同。新切片全部通过 glslang/SPIR-V validator 和反射，保 64 lanes，set0 binding0–3 storage、binding4 UBO stride16，6×U32 push（0–20 B），三项原 FP32 模式、`NoContraction` 以及模块内原私有 OP66。新增最终 GLSL 每个实际变体 ≤49,152 B，另登记实际 SPIR-V 大小。现有静态结果如下，实际 pipeline 和位型资格仍需 PC 取证。 [闭包工具](../tools/architecture/analyze_t750.py) · [构建转换](../pc/vulkan/build_roles.py)

| 诊断模块 | 原业务闭包 | GLSL B / SPIR-V B | SPIR-V 指令 |
| --- | --- | ---: | ---: |
| 原 OP7 | `fieldAudit → audit(1) → audit(2) → drive` | 22,828 / 108,832 | 7,129 |
| OP7 字段审计 | 原 7 入口、计数、`fieldAudit` 和两次 `audit` | 11,459 / 44,256 | 2,888 |
| OP7 驱动计算 | 原 `drive`、`acceptedTime`、Soft128 和 `softExp`，仅初始化 lane | 18,142 / 81,776 | 5,280 |
| OP6 | 原 scope compare | 7,725 / 21,968 | 1,339 |
| OP8 | 原 time prepare | 7,860 / 21,472 | 1,270 |
| OP12–14 | 原 trial gate/reset/init | 8,598 / 27,448 | 1,705 |
| OP15 | 原 entry field audit | 11,570 / 44,704 | 2,918 |
| OP16 | 原 entry partial | 9,765 / 34,836 | 2,168 |
| OP17 | 原 `drive + entryLedger` | 21,332 / 99,768 | 6,475 |

复现以冻结 `role_OP_00.comp` 使用 `split_group`。字段臂在单 7 的 `main` 保留原 prologue 和计数，最后执行原三个调用，删原 `drive` 入口后重取函数闭包。驱动臂只保原 66 arm，原 7 arm 只执行 `lane=gl_LocalInvocationID.x; drive(); return;` 后重取闭包。其余臂只按原 opcode 取闭包。沿 `build_roles.py` 原 glslang `-V --target-env vulkan1.1 --spirv-val --spirv-dis`、`precise` 变换和反射规则生成。 [实际字段路径](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp) · [实际驱动路径](../entry/src/main/cpp/gpu_resident/g13flow/drive.glsl)

## OP7 的双派发等价候选

首轮只把两个臂登记到独立诊断注册表。均取得实际创建 `VK_SUCCESS`、原两轮 OP66 `[0,0]`，并通过私有原字差分后，T833 可把双派发作为显式版本化的 Vulkan 调度候选。原 SOURCE 16 槽与 QUANTUM 820 **逻辑槽**保留。完整 QUANTUM 的原第 815 槽（从 0 起）拆成两个实际 `vkCmdDispatch`，因此该候选为 **821 次物理派发**，在平台收据逐项登记。两项都完成录制并受同一成功 GPU 提交后才计一个逻辑槽，接受仍等原尾及 fence。原 820 物理派发路径保冻结身份。若实测需维持原物理数，先留两臂因果证据，再从同一数值路径找可验证的单派发等价候选。 [T607 操作表](architecture-T607-g13-dispatch-v1.json) · [T753 绑定](architecture-T753-v1.md)

1. **原第 815 槽的字段臂。** 冻结 `uOp=7, uScope=0, uJ=uMicro=uBlock=0, uIndex=815`，同一组 64 lanes，绑定 0/1 为冻结 BASE/CANDIDATE，WORK/CTRL 不更换。保 prologue 的 `c[752]`、`c[676]` 初始化及 `c[522]` 一次逻辑计数。保持字段审计的范围、组归约、float 原字/压力 Q32 检查、所有 barrier 和 `c[755]` 的 audit(1)/audit(2)。`failDetail` 和原 first-error 原子竞争协议保留。
2. **相同逻辑槽的驱动臂。** 同一 descriptor 和 push，保持 `uOp=7` 以得到 `acceptedTime = t + 1/120`（连同原 SOURCE 分支判定）。只初始化 lane 后调用原 `drive()`，无再次计数/清 `c[676]`/重置首错，保原数学私有初值、全 113-bit 有效精度和 RNE、角包络、力开关、conditioned `c[642/643]` 写及 `failMath`。原字段审计先做，即使字段有错，驱动臂仍照原顺序执行，已有 `c[514]` 时沿原原子首错保留字段首错。撤权/超期由原根许可废弃整个候选，不保半个 7 的 publication。故障时保实际已记录的两个物理边沿。
3. **API 可见性与提交。** 维持原 `6 → 7字段 → 7驱动 → 8`，两臂中间与前后用同一 queue 的 compute write→compute read/write 依赖覆盖实际 CTRL/State/WORK 范围。原子失败字段、完成掩码和 seal 不能借 host 补写。保同一次实际根终结、原单尾 fence、1024 B 回读及冷档版本。`c[753]` 和 Completion256 `[39..41]` 保逻辑 820 及原执行/跳过计数，独立收据记录实际派发、额外 barrier、实际提交与窗口结果，T837 的逻辑判据沿用原值。超时或第二臂缺失须拒绝，完整原件区分已录制和已实际提交。 [Vulkan 同步](https://docs.vulkan.org/guide/latest/synchronization_examples.html) · [T837 判定器](../test/t837/README.md)

## 下一轮取证与恢复条件

- **先跑两个 OP7 臂和 OP15/OP17，再依结果补轻臂。** 每臂同设备、同布局、默认 flags、独立诊断子进程，沿 T834 的 60 s/120 s 上限。每臂保 PE/SPIR-V/layout SHA、创建 ENTER/EXIT、submit/fence、两轮 OP66 原字及 sample。两臂单独均返回时，先取私有连续组合写集，再走整根。若 `drive` 臂及 OP17 同时不返，优先为实际 Soft128/驱动闭包补诊断，而不假设共同首因。若只有 OP17 不返，保 `entryLedger` 路径并沿真实入口 `uJ/uMicro` 隔离。OP15 使用 trial selected/forced guard，按其原独立资格判断。OP6/8、12–14/16 和其他稳态角色也要以各自发布的实际模块通过。实际 56/59 等 SOURCE 审计和其它 `drive` 调用点均独立取证，OP7 解锁不授予 SOURCE 通过。
- **若仍不返，补精确原件。** 在同次 ENTER 后两处采同 owner TID 的用户态栈和地址所属 ICD/loader 模块、实际文件版本/偏移，另记权限或符号缺失。记录原 QPC 和最终 supervisor QPC。同一 SPIR-V/layout 的替代硬件驱动结果可作编译器路径对照，仍按实际 adapter 归属。flags=0 和 no-opt 已各做一次限时试验，原单 7 无需重复长等。内部拆 Soft128 或按运动/力模式切换，须另交分支覆盖与精确位等价，保原力窗、`mathDomain/first-error` 与私有 Soft 状态，不能先将运行模式锁成 REST 来宣告生产通过。
- **差分与完整根。** 用相同冻结 BASE/CANDIDATE/WORK/CTRL、实际绑定和 push，取两臂终点原字。核 `c[676]`、`c[522/523]`、`c[755]`、事故首错/细节/原 seal、`c[624..643]` 与 `c[820..827]`、未写范围及下一槽 8 的状态/时间原字。覆盖 REST、固定/匀转、RAMP 的短相位及结束边界、MMS 力窗与域错、字段首错加驱动域错、错误 descriptor、第二臂撤权、无新增 publication 负控。能够运行原 OP7 的合格后端优先做原调用与候选同输入差分，原模块缺真实输出时保证据缺项，并按冻结算法、原 GPU 证书和 T837/T828 独立物理原值核验。只有所有实际角色通过原 OP66 和数学/资源门，才测完整非零 SOURCE/QUANTUM、原 250 ms/33,333,333 ns、ROOT/FULL、同代 `GridView`、实际速度/压力/黏性及窗口。目标14依旧等可信场实际结果，GPU 驻留和 shader 渲染，禁止 Canvas。 [原数学证据](test-report-T721-v1.md) · [PC 定量合同](architecture-T828-v1.md)
