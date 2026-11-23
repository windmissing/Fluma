# T889 · PC 流场预算与干沙观测缺口工程分流

2026-11-23 · architect · P3 · Windows Vulkan · 原件与源码离线复核

**流场先补失败根的 GPU 查询，再决定减同步还是减派发。干沙可分别实现空箱预览、合法边界上的可控重力和同根区域像素取证。** 两组工作只共用 Windows 平台证据口径，实施各走现有 owner，与 T886 的三态／真通量接入并行。 [复算收据](architecture-T889-evidence-v1.json)

## A · 目标 13 的实际时间预算

T879 六臂按 `a56e4a3c` 重读，业务实现按 `555cfe69`。实物 T877 Release SHA 匹配。两种原 `fixtureConfigSHA` 和整份 Config128 文件 SHA 分开登记，前者取 word 76–83，State 原字 SHA 与 T868 一致。同进程以 nonce、fence、queue、device 联 QUANTUM 等待、GPU 查询和设备排空。频率均为 10 MHz，全部 submit 返回 0，首退出均 `GRANT_DEADLINE`，业务最后 fence 和同根退役 fence 均为 `VK_TIMEOUT=2`，设备排空为 0。两条 F05 SOURCE 已有自己的同根 fence、接受和 V2 原档。 [T879 原件](../test/reports/t879/report.md) · [T877 诊断合同](../test/reports/t877/handoff.md)

| 工件臂 | 录制 ms | 业务等待 ms（submit 返回→首退出 / 短等待次数） | 退役等待 ms | 设备排空墙时 ms | submit 返回→可确认仍未退役／已排空 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| F02 full | 5.867 | 245.267 / 16 | 253.157 | 1207.174 | 498.424 / 1710.826 |
| F02 hot | 0.668 | 249.966 / 16 | 252.579 | 1181.535 | 502.546 / 1684.343 |
| F03 full | 0.879 | 252.413 / 16 | 253.208 | 1016.475 | 505.621 / 1522.337 |
| F03 hot | 0.633 | 254.130 / 16 | 264.000 | 1298.347 | 518.130 / 1816.753 |
| F05 full | 0.647 | 249.308 / 17 | 251.002 | 1589.650 | 500.310 / 2112.209 |
| F05 hot | 0.596 | 255.197 / 17 | 265.220 | 1589.097 | 520.417 / 2135.460 |

- **已定位到提交后的期限闭合。** 250 ms 从 grant 起算，首拒检查在原 deadline 后 0.001–5.832 ms。每次短等待请求至多 2 ms，真实调用墙时受实现与调度影响。日志中的最后拍、callback/receipt 间隔及 firstStop 未指向脉冲撤销。业务早拒保留，退役证据不可回写接受。 [T877 等待参数](../pc/vulkan/g13_candidate_timing.h) · [Vulkan fence 语义](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForFences.html)
- **排空墙时只能度量宿主观察。** 1016–1590 ms 从 `vkDeviceWaitIdle` 的进入算起，前面已经有业务和退役等待。最后一次同 fence 超时证明在退役等待结束时仍未取得完成，设备排空成功说明同设备工作最终结束。两端给宿主观察上的完成区间，未分出实际 queue start、GPU compute、驱动调度与 fence 通知。六臂 QUANTUM 的 17 个查询点都被 `reportGpu(false)` 输出为 unavailable，原代码在 drain 后销毁 query pool，缺最终信号后的真实读数。T879 所称 4–6 倍 GPU 执行耗时不能由这些原件计算。 [Vulkan 设备排空](https://docs.vulkan.org/refpages/latest/refpages/source/vkDeviceWaitIdle.html) · [Vulkan 查询可用位](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetQueryPoolResults.html)
- **full/hot 的结论限于这六臂。** 850 dispatch、850 条 compute barrier、槽内 28 次 copy 及 28 个 compute↔transfer 边在两臂相同。hot 从业务 fence 内移出七段用作冷档的 copy，业务 copy 命令从 36 降至 29，保 3,376 B 热前缀。三对都没跨过期限，单对设备排空墙时差为 −25.639、+281.872、−0.553 ms，样本不足以判断零效应或指定算子占比。F05 SOURCE 同端点 `UPLOAD_DONE→BUSINESS_COPIES_DONE` 为 51.715 / 57.473 ms；58.138 ms 算到了 hot 的 `COLD_COPIES_DONE`，是不同区间。以上均为标记间隔，且 SOURCE 不能外推 QUANTUM。 [候选录制与查询](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc)

### A1 · 先交付失败根的真实查询

1. 在原 `T877_ROOT_WAIT/T877_GPU_TIMESTAMPS` 补 `submit` 前的 root 开始标记和最后 business copy 完成标记，保当代 `{run, nonce, queue, device, fence generation, queryPool, writtenMask}`。用同队列有效位掩码计算模差和设备 `timestampPeriod`，两个有效点才能给 span。分报 ROOT、ACTUAL、REPLAY、COMPARE/PACK、热 copy、独立冷 copy 与 QPC `grant→record→submit→stop→retire→device drain`。压力、黏性现有首末 span 内含交错工作，对定位有用，不能累加为算子独占时间。需要独占分因时在同输入另做按原 logical ordinal 的抽样，并留采样开关对照，以实际有效点为准。 [Vulkan timestamp 规则](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html)
2. 已提交超时时，把 query pool 的当代标识和 writtenMask 同 tombstone 保到真正退役。在 `Boot` 最终排空成功且同一未复用 fence 确认后、销毁资源前，以 `WITH_AVAILABILITY` 无 WAIT 读取，另发 `LATE_RETIRED_DIAGNOSTIC`。保原 firstStop、business reject、acceptedRoot 为 null，不读成业务物理场。设备失效、未取得真实 fence 或缺任一点时保 `UNAVAILABLE`。control 中明确区分资格提交、当前根和同句柄后续提交，尤其 F05 的 SOURCE 与 QUANTUM。 [退役接缝](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp#L157)
3. 预注册同机一对基线／加查询，等价实现后再 A/B。固定实际 PE/SPIR-V、布局、配置、seed、driver、电源/温度窗口和原 250 ms/33.333333 ms。允许记录真实晚完成供定位。作为时间改善，必须在相同测量边沿和输入下可复现；作为业务通过，原 fence、owner 检查及提交要在 grant 窗内闭合且数值通过。绝对预算仍为 `录制+提交+排队/执行+尾映射及接受≤250ms`。现有最快一臂同根退役等待到 submit 后 498.424 ms 仍超时，单凭可省的少量调用无法外推能过门。

### A2 · 等价缩减候选及独显触发

| 顺序 | 候选及最小修改 | 证据与放行 |
| --- | --- | --- |
| 1 | **槽内 transfer 改为按槽 GPU 保存。** 14 槽各保 `(32+10)×4=168 B` 的 latch 和真实 first-error 原字，给现有 WORK 私有区增加 2,352 B 及现有对齐所需范围。每槽在原 OP17 ledger 完成位置冻结，整段尾读。优先在私有 ledger 能证明写完可见时输出，若需单独 snapshot dispatch 就独立计入最多 14 次，并与原版比较。 | 保每槽 `(scope,J,micro,ordinal)`、首错锁存、后续槽覆盖前的值、NOOP 和保留字段。对照全部 588 个 U32 与原 copy 序列。在无需新派发的实现可移除 28 次槽 copy 与对应 28 个阶段转换，新增一次尾 copy。若只能移到后继根才读，该项不可替代本根 latch/接受门。 |
| 2 | **按已证读写集缩减 compute 屏障。** 先在 host 按 `opcode/role × base/candidate/replay/work/control/pack 范围` 出实际表，对可证没有 RAW/WAR/WAW 的相邻段消除多余依赖，或按冲突范围缩窄。先用一个字段与其读写消耗可列全的小段。 | 未知范围、顺序竞争、首次错误原子位、ACTUAL/REPLAY 交界、OP17 私有交接及尾 map 的依赖继续保留。仅更换范围与边数时，原派发、相位、首错/接受全字应保持。至少实际取 F02 私有、F03 业务首根和 F05，逐步加独立数值及撤权对照。 |
| 3 | **条件性减少派发。** 如果 A1 量到 OP17 切段开销，先按 T854 试 836（仅将各 OP17 的 MMS 与 ledger 合并在经验证的模块）。850→836 仅减少 14 个物理派发，约 1.65%，不能直接换算成墙时。若实际热点是原 224 个 `PRESSURE_V` /112 个 `VISC_BLOCK` 逻辑槽，先核真实 active/NOOP、收敛轨迹和输出精度，再决定独立等价内核或交算法评估。 | 保 820 逻辑槽、J 选择/trace、状态、RNE/Soft128、原 ACTUAL/REPLAY 和首错。原固定计划禁止按 host 猜测绕过 NOOP 槽及其计数。836 先过新增 shader 单元/变体 ≤49,152 B、建管线、每角色双轮 OP66、T857 写集/独立参考、再过业务根。820 单派发整段只留历史可选对照。 [T854 拆分范围](architecture-T854-v1.md) · [T861 真槽门](architecture-T861-v1.md) |

独显同工件一旦有合格 Windows Vulkan 工位便可跑，沿 T878 能力、实际 device/driver UUID、输入和 J=1/2/4 试验定义，先原 T877 PE 的 F02/F03/F05 `full/hot`，再比诊断或优化工件。每次保存完整 argv、有效时标、实际 ROOT/FULL 和撤权/迟到原件。同工件能过只证明该硬件与配置的实际可达域。F02 私有投影保 `acceptedRoot=null`；F03 的 30 个实际接受根、F05 独立速度/压力/黏性和 T828/T631/T866 原数值门仍照原要求。目标 14 只从可信实际 `GridView` 接续。 [T878 工位触发](../test/reports/t878/report.md) · [T828 原验收](architecture-T828-v1.md) · [独立数值检查](../test/t866/check.py)

## B · 目标 4/5 可独立合入的 Win32 接口

### 原件复算与范围

按 `c24df15b` 比对四份 T884 原件和 T872 实物。空箱臂一条 `M1_POINTER_READY`，0 个 SOURCE/业务画面，关闭后 `ATTEMPT_REVOKED`。长窗 721 个 queued 根，首末 `presentExitQpc` 相距 28.749 s，以 720 个间隔计算约 25.04 个新增队列帧/秒，真实显示速率仍缺 T850 的独立证据。`dual-release` 的 153 帧按 Present 退出在 Up 后计，按 root QPC 则 152 个新根，一帧跨 Up。活沙包在第 442→446 行已由 1→0，新根 QPC 约早于 Up 2.165 s。Up 后继续持有 128,878 Q20，实证为停源及留床。要验证释放后才落床，必须把 Up 放在活 P 尚在的时段并采之后的 `LAND` 原字。此轮 raw `settledAfter[140:142]` 与 `flightAfter[144:146]` 均为零，无法用作 S/P 质量证明。仅有 `sandStockQ/activeSand` 和底边单像素时，还缺同帧非零 S/P 与像素空间配对。 [T884 原报告](../test/reports/t884/report.md) · [T885 平台边界](architecture-T885-v1.md) · [T850 证据](../test/reports/t850/handoff.md)

### B1 · 空箱窗口先出图

- 在 `m1_source.inc` 的 `awaitSource` 前增加交互空箱入口，将首个 SOURCE 中现有冷 BASE 上载和可见性准备提取为同代一次初始化，先以实际初始化 fence 证明冷零库存。原非预览调用仍可在 SOURCE 内完成初始化。用同一 `PresentBoot` swapchain/graphics shader，首次提交写 `frameKind=EMPTY_READY`、`acceptedRoot=null`、`sourceCommit=0`、无物理时间。`pc/vulkan/wsi` 与现有 fragment 增加显式空画面输入，沿原视口、边框、色彩转换出图。绑定有效零初始化资源，空画面只画边框和底色。收据关联真实冷 BASE 初始化 fence，零量、空 scene、同代 HWND/surface/geometry、GPU 提交和呈现原码分别可查。固定诊断关键色路径保原模式。
- 复用同一次 WSI 循环的后续 publication 交接。真实首张空窗后开启 Ready，Win32 UI 始终能接消息，owner 进入原 `awaitSource`。真 Down、收到合法真实拍、首个 SOURCE 接受后再送已接受业务根，空箱无 permit、无虚构的零量 SOURCE、无物理时钟债务。resize/撤权后先退旧空/业务资源再开新代、重取真实 Down。首版交互入口采用独立开关，保持现行 D1、单根冷档和无窗口 probe 优先级及其证据格式。 [现有 SOURCE 接缝](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [WSI 续帧](../pc/vulkan/wsi.cpp)
- 验收用例先空窗、静置不发源、再按下和松手，同一实际窗口在首源前有真实空画面。空态无业务根，首粒出现后用原同根令牌。补首次空窗前后 resize、关闭、失焦和旧代输入反例，留真实等待/失效收据。无有效旧画面时明确 FAULT，有同代旧 shader 帧时可按 STALE 合同处理。

### B2 · PC 可控重力沿原 M1 角度域

- 在 `Window` 的短锁输入侧增加 `(geometry, sequence, actual QPC, signedAngle256)` 的最新有效记录。最小人工入口沿当前 1/2 选料之外加 `左/右/回中`，复用原 `manualAngle` 的每度 256 个 index、±7680 约束。测试驱动同入口，越界、非递增、旧代留具名拒绝，失焦撤销旧输入。复用原 M1 角表与 shader 计算。 [原 M1 手动角度](../entry/src/main/ets/physics/GpuM1DemoController.ets#L112) · [原角度与约束](../entry/src/main/cpp/gpu_resident/m1/quantum_stages.glsl)
- owner 在合法帧边界生成 grant 时一次采角，所有 `clock.Claim` 入口同步使用，包括首源、恢复和 T882 后续预约。`RunM1Quantum` 在已有 grant 时读 `permit.angle`，纯 QUANTUM 申领时同一快照进新 grant。用一个冻结值贯通 `BeginQuantum` 与 Vulkan push/UBO，并保实际 `angleAcceptedIndex` 的带符号原字。GPU 现行床约束可能把请求角裁到可接受值，记录 `requested/granted/accepted`、输入 QPC/seq、相关 `frameSerial/ordinal/root` 与是否约束。SOURCE 仍零物理时长；同 grant 的所有 QUANTUM 共享请求，变更不能覆盖在途 grant。重力操作只改方向，原 120 Hz、freshness、来源、许可和 shader 同代渲染不变。
- T836 原 fixture `[1,0]` 代表 90° 横向重力，超出本 M1 原域。把该动作保为 `ANGLE_OUT_OF_RANGE` 负例，并用新的 M1 参数档 `0/+15°/−15°/0` 与原 `resize/旧代` 回放做正控。沿角表复算真实受理位和向量，核相同输入下质心/位移方向及同根图像，另用受限床例确认角度裁限真实存在并带原字。超过 ±30° 的旋转按 T829/T843。 [T836 输入与判定](../test/t836/README.md) · [T829 旋转前置](architecture-T829-v1.md)

### B3 · 同一接受根的床沙与粒子像素

- 第一段沿 `--m1-root-on-busy` 或 T882 的同代冷读，按指定动作区间预约有界根。冻结真实 bank/原 `sand16/column16` publication、`Completion256` 和事件范围，以独立 reader 从 GPU 原字核同一根的各列 S、P 量皆非零，活 P 的坐标/半径、床轮廓及 `S+P` 守恒。缺命中的预约给实际 terminal。保原 nonce/slot/generation/geometry、pin、同步、copy fence/invalidate，正常热路无需整池读回。 [T869 原后继根](../test/reports/t869/report.md)
- 第二段在 `PresentBoot` 新加测试专用 `ProbeRequest/ProbeReceipt`，保普通单点合同。`ProbeRequest` 在录制前给床内、床轮廓之外的已知活粒投影和背景三处，首版每处至多 16×16，四字节格式总 payload 至多 3,072 B，Vulkan 对齐另算。按实际 extent/视口及图像布局确认，可以用固定场景约定 ROI，或先对同 lease 做分段冷读再按该根布点。采用四字节像素原格式，在着色之后、Present 之前的 transfer-src 区间 `vkCmdCopyImageToBuffer`，复用真实 graphics fence。匹配 `root+lease+geometry+imageIndex+graphics fence`，保坐标、格式、行向、sRGB 路径、原像素 bytes/SHA 与 Present 原结果。同色要靠互斥空间定位，背景负控与原 GPU 原字一起验证。 [当前 shader](../pc/vulkan/m1_dry_scene.frag) · [现有像素 copy](../pc/vulkan/wsi.cpp)
- 验收一根同时有非零床量和活 P，并在同一实际图像的两处看到分别匹配的着色原字，读背景作负控。再测无床、无活 P、错 nonce/旧代、裁剪出界、未命中和 copy 未退役时禁止使用；停源的 `LAND` 再取槽级事件、前后库存及在途坐标。各段对现有 draw 只增加有界的按需采样，产品见效仍由实际 Windows GPU shader 帧判定。

三段可以分别交付，空箱和重力不等像素工具，当前零角业务也可先验证 ROI。采用原运行的正确性合同；新增测试接口不改变 D1 四臂、T882 源预约和退役、原数值和原产品接受。继续 GPU 驻留、shader 渲染，禁止 Canvas 回退。T877 C++ 时序／绑定自测、T836 与 T850 离线检查均已复跑；本卡没有新 GPU 执行。
