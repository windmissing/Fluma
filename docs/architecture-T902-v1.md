# T902 · PC REPLAY 冷档与 DM2/R1 首根归因

2026-11-24 · architect · P4 · Windows Vulkan · 源码与原件离线复核

**REPLAY 是预览初始化顺序缺陷。** 三次都在空箱初始化后、D1 READY 前把空 `AcceptedRoot` 送到冷档检查，实际未取得根、lease 或冷拷贝 fence。**DM2 和 R1 各自通过 BOOT，均在已提交的首个 SOURCE 离开业务截止窗口。** DM2 的追加同 fence 等待成功。R1 的追加等待仍超时，退出阶段缺最终退役原件。 [逐臂原件复算](architecture-T902-evidence-v1.json)

## A · D1 REPLAY 把空箱当作已接受根

T896 原件固定在 `1c949af2`。本地 T894 B PE 实物 SHA 与 `ba503ee5` 交付一致。`b-replay-off`、`b-replay-off2`、`b-replay-on` 各 30 行，顺序完全相同。 [T896 原报告](../test/reports/t896/report.md) · [T894 B 工件](../test/reports/t894/delivery.md)

| 原行 | 可核阶段 | nonce / backing / 退役 |
| --- | --- | --- |
| 26 | `M1_EMPTY_PREVIEW_SUBMIT` | 预览初始化 checkpoint。原件未到 `M1_SOURCE_WSI`、空箱画面或 D1 READY。 |
| 27–28 | `M1_QUANTUM_COLD_CAPTURE` → `FAILED M1_COLD_ROOT_BACKING` | 无 SOURCE/QUANTUM nonce、Completion 或 backing。按源码已通过的前段为空箱初始化 fence，冷档 guard 更早于 lease Acquire、冷命令录制及 copy submit。 |
| 29–30 | `OWNER_TERMINATED_BEFORE_DRAIN_PROOF` → `M1_D1_CLOSE` | `retired=false`、`ownerCoverage.complete=false`、`producerQueued=0`，没有 `END_INTAKE` 或真实 D1 drain。关闭 QPC 只证明窗口销毁。 |

### 首次失败机制

T894 B 在 `Window::Owner` 默认使 D1 开启 preview，除非明确走 `--m1-d1-cold-reanchor`。`RunM1Source` 成功等待空箱初始化 fence 后构造默认 `AcceptedRoot`，其 nonce、publication generation 和 Completion 均为零。preview 分支将 `startBusiness()` 推迟到 `PresentBoot` 的首次后续回调。此时外层仍在 `PresentBoot` 前无条件走 `!m1RootCapture → captureAccepted()`，REPLAY prefix 使其执行 `CaptureM1Quantum()`。其中 `stored[27]==2 || (replay && sourceRoot)` 要求实际接受的 QUANTUM 或 SOURCE，而 `stored[27]==0`，因此在 `M1_COLD_ROOT_BACKING` 被拒。就算逐项打印其他复合谓词，也没有合法根可读。此路径尚未走到 `root.Acquire` 或 cold fence 创建，不支持“第一个接受根 backing 有问题”的推断。 [预览与接入](../pc/vulkan/main.cpp) · [归档调用顺序](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [冷档实际 guard](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc#L295)

异常被 `RunBoot` 封为 FAILED，Win32 owner 随后用退出兜底回执关闭。空箱初始化的同 fence 信号是源码路径所需，原件没有对应句柄和等待时间可独立联。三条 CLOSE 没有提供冷拷贝退役或正常 D1 业务退役的证据。当前 52 动作在 READY 前根本没有开始，不能补合成终态。历史 T887 的 15 份已接受根 `M1PROOT1` 保留其独立同根结论。 [T891 既有同根档](architecture-T891-v1.md)

### Developer 最小修订和独立验收

1. **以真实接受边沿触发归档。** 非 preview 的原首根入口继续使用 `startBusiness()` 后的采集点。preview 入口跳过 `PresentBoot` 前的 `captureAccepted()`，在它的首次后续回调内由 `startBusiness()` 成功返回后，首次 QUANTUM 的实际 `Acquire` 显示 lease 之前补采该根。原 SOURCE 在 `runSource` 接受后已有独立 replay 拷贝，保原调用点。后续根仍在旧帧 lease 释放及新根接受后采。以 `(run, geometry, rootKind, nonce, publicationGeneration)` 排重，首两笔分别为真实 SOURCE 与真实 QUANTUM。空箱单独保 `acceptedRoot=null` 和初始化 fence，保持原冷档判据、动作 READY 启动方式和首根语义。
2. **在录制 copy 前冻结可审计的 backing。** 保存接受尾中实际 nonce、root kind、BASE/FINAL bank、publication slot/generation、真实 event arena、start/end/capacity、各 `VkBuffer+allocation generation/range` 与原业务 fence 的成功回执。校验 `root.Bank`、Completion 与 lease 一致后再提交独立 copy。正常提交后的 lease 保到同 copy fence 信号或实际设备失效才释放，独立记 `COLD_COPY_SUBMITTED`、实际等待／信号、invalidate 与 SHA，最后才允许 writer 出完整档。提交前错误释放尚未使用的 pin，晚 fence 保隔离，不读取尾、不迁就命名。不把 writer 落盘或窗口 CLOSE 写成原 GPU 退役。
3. **先用离线接缝验证，再复采。** 负控 preview null root 不进入采集，错 nonce、slot、generation 和事件范围仍明确拒绝。正控覆盖 preview→READY→SOURCE→QUANTUM、原 `--m1-d1-cold-reanchor`、重复触发排重、`END_INTAKE` 正逢 copy pending、真实 writer 失败和设备丢失。PC 按两个 REPLAY 模式各跑一轮，分别取首笔真实 SOURCE 与首个真实 QUANTUM 的档及 backing/lease/fence 原行，同时核 52 动作、覆盖和真实 drain。程序故障保 `OWNER_TERMINATED_BEFORE_DRAIN_PROOF` 和实际缺项，正常拒绝根沿原安全退役。 [独立归档 reader](../test/t496/check_pc_d1.py)
4. **REPLAY 恢复可执行后，四臂仍先过输入门。** 本轮重跑 T897，T896 OFF×2 已有实际 owner 覆盖及同代退役，首差仍在 action 1。相对 READY 为 24,108,700 / 23,463,100 ns，SOURCE 7 实际 Δ 为 14,964,500 / 15,141,800 ns，实际接受量 125,531 / 127,018 Q20，SOURCE 29 终态分别撤销与过期。“都能执行”与“时域与原字可比”分别出证据。按 T891 冻结同输入与真实 owner 时标，再交 T496 比对；暂保 `CLOCK_OR_INPUT_DIFFERENCE`。 [T897 入口](../test/t897/check.py) · [严格跨臂门](architecture-T891-v1.md)

## B · DM2 / R1 已提交 SOURCE 的宿主截止与退役

两个 host 实物、同一研究输入前像 `3580ccb1…` 和本地生成的角色表、SPIR-V 字节核对齐全。DM2 45 个 compute 模块，R1 49 个 compute 和 6 个 graphics 模块。graphics 哈希核对只证明工件，原件未进入 R1 窗口呈现。两份日志都记录实际 Intel `8086:3e98` 和 `driverVersion=1656924`，各自的 run、fence、role-set 与实际 frozen `inputSHA` 不同。原 `inputSHA` 是实际冻结输入，和共享研究文件 SHA 分开绑定。 [T894 样机入口](../test/reports/t894/delivery.md) · [原件复算](architecture-T902-evidence-v1.json)

| 家族 | BOOT 宿主 QPC 间隔／原资格 | SOURCE grant → 截止（ns） | 业务和显式退役原值 |
| --- | --- | --- | --- |
| DM2 | 141.9224 ms，`vk=0`，seal 6559303，eligible | `1599666172773900 → 1599666422773900` | 12 个录制派发，业务 `2`，无热尾。追加同 fence `RETIRE=0`。 |
| R1 | 395.2477 ms，`vk=0`，seal 3223126018，eligible | `1599688708107200 → 1599688958107200` | 19 个录制派发，业务 `2`，无热尾。追加同 fence `RETIRE=2`，最终 signal 缺证。 |

上表 grant 由原 `deadlineNs−250,000,000` 和 `host.cpp` 的 `pacing.grant.grantedNs` 赋值复算，分别在 BOOT 结束后 9.8516 / 11.5490 ms。它是业务许可起点，包含冻结 input、构建 expected、录制、提交、等待及尾接受的墙钟预算。BOOT 使用独立 2 s 资格等待，表中为整个宿主资格 span；该数字不能外推到 SOURCE。 [时标与许可赋值](../pc/vulkan/sand/host.cpp)

- **准确解读 `ROOT.vkResult=2`。** `VulkanOwner::Run` 录制全部派发，提交成功后进入 wait loop。每次实参为 `min(2ms, remaining)`，原 `VK_TIMEOUT` 会重试。当宿主采样已到 `deadline` 时，函数自行返回 `VK_TIMEOUT`，当只有 `current()` 失效时返回 `VK_NOT_READY`。两份原日志都另有 `RETIRE`，它只有 `HasPendingWork()` 才输出，因此可以钉到实际提交且在途后的宿主截止。原 `ROOT` 的 `clockFault=0` 仅是枚举样本；`Fresh()` 还检查实际时效，且与 deadline 同时失效时返回 2。两份都无当时的 native pulse 年龄、每次等 fence 的 VkResult／实际 QPC、抢先撤权顺序或 query ticks。无法把 `ROOT.vkResult=2` 解释为最后一次原始 fence 返回，更无法计算纯 GPU 时间。`tailRead=false` 时 outcome/reason 零是初始化字段。GPU 物理拒因尚未到达。 [提交与业务循环](../pc/vulkan/sand/owner.cpp#L623) · [原 Fresh 谓词](../entry/src/main/cpp/gpu_resident/g9dm/progress_clock_v3.cpp)
- **退役是独立实测。** DM2 的后续 `Retire(Clock::RootNs)` 返回 0，结合原逻辑同一个 fence、generation=2，能证明拒根后续真实完成。R1 在这个独立 250 ms 请求结束仍为 2，`inFlight_` 保留。析构当前调用 `vkDeviceWaitIdle` 但未记录结果、实测时间或最终同 fence，不能把进程退出认作已成功退役。原窗口期已经失败，之后任何晚完成只能诊断和清理。 [owner 退役与析构](../pc/vulkan/sand/owner.cpp) · [设备清理](../pc/vulkan/sand/host_device.cpp)

### 最小测量、对照和继续条件

1. **保原算法、派发表和 250 ms。** 首个 SOURCE 用一个业务 submit、同一原尾 fence 和 1,024 B 热尾。`ROOT_TIMELINE` 记录 family、实际 `device/queue/fence + fenceGeneration`、role-set 与实际 buffer ranges、nonce/plan、grant/deadline、输入冻结及 expected 完成、record begin/end、submit enter/exit 与真实 VkResult。短 wait 留调用实参、enter/exit、返回、第一次实际退出原因、当时真实 pulse/callback/receipt 及 Fresh age。尾 map/invalidate、typed gate 和晋升各自记录原结果。用 `GRANT_DEADLINE`、`STALE_PERMIT`、`FENCE_RESULT`、`LATE_FENCE_SIGNAL`、`TAIL_REJECT` 等区分宿主结果，保首次原因。
2. **最小 GPU 分段用有界 timestamp，保持原运行一份命令。** 记 upload/BASE copy、准备、writer、审计和 seal/尾拷贝边沿。**DM2** 在 0–4（输入准备）、5–7（出生/来源/账）、8–11（审计、比较、pack/seal）分段。**R1** 用 0–5（物理和视觉准备）、6–10（ACTUAL 视觉计划与父出生/来源账）、11–13（REPLAY 视觉计划、父审核及应用）、14–18（比较、pack、视觉审核及 seal）。以冻结 SOURCE 表的 ordinal、opcode、角色和绑定定位。各组跨度可能包括同步与 copy。记录真实 queue `timestampValidBits/period`、raw tick/availability 和同窗口无查询控制。需要验证 `--witness` 的额外 copy 时另采，现有 R1 原档没有 `VISUAL_SEED` 阶段。DM2 的 FULL discovery 不进这份 SOURCE 对照。测段维持同一次业务提交及原有工作。
3. **先闭合失败根的资源生命期。** 业务拒绝立即隔离且禁止 accept/发布。额外同 fence wait 独立记 `enter/exit/timeout/return` 和真实对象代，仍 pending 时保 command/pool/descriptor、bank、query handles 至安全退役。最终 `DeviceWaitIdle` 记录真实墙时和返回，只有成功后再取同代 fence 检查与可用 query 原字，失去设备按明确失效分支，未证时发 `RETIRE_PENDING/DEVICE_LOST`。当前 owner 与 HostDevice 都做清理，工程上宜由单一拥有者记最后有效退役，另一个消费该结果。退役故障不读 staging 当 accepted、不得让 RAII 无声复用资源。G13 T889/T895 的时标做法可复用，当前两族不借 G13 数值或耗时结论。 [已验证的分时接缝](architecture-T889-v1.md) · [T897 既有实测](../test/reports/t897/handoff.md)
4. **每族独立对照。** 固定对应 T894 PE、生成角色 manifest/SPIR-V、同一 `t894-model-source.u32le`、配置与 seed、固定 SOURCE-only 且无 witness/Present 的显式 argv，先 `--modules DIR --source INPUT` 建两边同口径最小复采。T896 没附完整 argv，两台机均做新冻结复采。目标独显需要实际选中的 Windows Vulkan 硬件、该族所有模块能建管线、storage/UBO 和 queue 足额。如存在多个适配器，旧 HostDevice 取首个可用非 CPU，复采必须核实际 `deviceType/UUID/LUID/vendor/device/driver/queue`，需要时加按明确 UID 选择能力并保同构建两臂，不能以系统有独显代替确实选中。开发集显继续验证功能及取分段；目标独显上的持续性能由用户在实际场景测试，交对应工件、输入及计量口径。 [设备选择](../pc/vulkan/sand/host_device.cpp) · [T878 设备边界](../test/reports/t878/report.md)
5. **按真实接受分支推进。** DM2 由同一真实 owner 完成 `SOURCE→QUANTUM` 后取本族 ROOT / FULL、独立对照库存/事件/动员，FULL 以原 discovery 和已接受根实际封存。R1 单独拿实际 R1 父 SOURCE，再按原选中微步取非零真通量、WORK witness、联合 lease 和实际 shader Present。两族各保原 T647 / T839 / T845 typed gate、oracle 和接受额度。 [DM2 合同](architecture-T845-v1.md) · [R1 通量合同](architecture-T839-v1.md)

本卡已复跑 T894 的跨会话覆盖与 T872 脉冲退役离线自测，以及 T897 的实际 OFF×2 和 T496 历史比较。三份 REPLAY 和两份 SOURCE 原失败均保留。此次交付为架构归因与修订，未启动 GPU 实测。全程 GPU 驻留、shader 渲染，禁止 Canvas 回退。
