# T845 · 目标 9 PC 三态 writer、ROOT/FULL 与发布接入

2026-11-23 · architect · Windows x64 Vulkan · P3

**目标 9 的验收链从本族真实 GPU writer 出发。** 同一已接受根绑定 S/F/P 状态、逐来源及面通量账本、ROOT/FULL 冷档和真实窗口 publication。T830 的固定干沙 SOURCE 可作为 Windows 平台接线正控。目标 9 需要单独的 G9 角色、协议及原件。 [T824](architecture-T824-v1.md) · [T827](test-report-T827-v1.md) · [T838](../test/t838/README.md)

## 已有证据与接入条件

2026-11-24 · T945 续接。T923/T924 已有真实 DM2 SOURCE 和同次进程 QUANTUM 超期证据。先用同一 CURRENT 取 Q 分段、真实 250 ms 内的连续接受，再开 ROOT/FULL。 [T945 集显工程合同](architecture-T945-v1.md)

| 来源 | 复用范围 |
| --- | --- |
| T830/T840/T842 固定输入 | T830 的 Intel UHD 630 GPU 准入 131,072 Q20，permit 后约 74.596 ms 取得 fence。原 Completion256 被 M1 owner 接受，publication generation 1 同根进入 784×461 窗口，GPU/Present 原结果全 0。T840 v2 用同 SHA 工件独立复现固定 SOURCE 及同根呈现，11 项通过。T842 已核错 geometry、nonce、接受结果、frame token、退役五项负控。 [T830 实测](../test/reports/t830/m1-source-window.md) · [T840 v2](test-report-T840-v1.md) · [T842](test-report-T842-v1.md) |
| T830 交互版 | 已重新核 PE 及 `full_source / quantum_candidate / quantum_commit` 三个 SPIR-V SHA。Win32 指针、SOURCE→QUANTUM/REPLAY、同代呈现及 `M1PCOLD1` 已有编译交付，运行收据标 `PENDING_INDEPENDENT_VERIFICATION`。`M1PCOLD1` 八段只解释 M1，T845 复用它的冷请求、映射、封存和身份关联方式。 [交互交付](../test/reports/t830/m1-interactive-handoff.md) · [构建收据](../test/reports/t830/m1-interactive-build.json) |
| T779/T796/T838 | T779 交独立三态规定快照、DM 量化、RX 位移及坏冷档负控。T796（`9c18ea26`）证 RX 冻结动态设计的 13 项构建负控和 4 种合法字节对照。T838 自测的镜像、来源和像素是合成用例，真实 eligible pair 未观测。 [T779](test-report-T779-v1.md) · [T838 checker](../test/t838/check.py) |
| T639 rev2 | 复用 typed 父证明、同锁发布、GPU scene lease/retire 和冷档 caller-span。G11 组合是独立家族，视觉 bank/glyph 与 19/2543/2923 派发属于它的组合合同。目标 9 先交父 P/F/S 内容，供 T839 后续接真实通量视觉。 [T639](developer-T639-v2.md) · [T839](architecture-T839-v1.md) |

离线映射、reader 适配和角色构建检查可与 T830/T833 并行。**实际接入触发**为 T830 在真实窗口取得同代 FullM1 的非空 SOURCE、随后实际 GPU 接受 QUANTUM 与同根呈现，并发布可复用的输入、permit、transport、fence、lease/Present 和冷读范围接口。G9 需同时交付自己的真实硬件角色清单、该身份的 BOOT 数值及资源资格、目标根所需的全部角色可执行证据。只具备本族 BOOT+SOURCE 时可出独立 SOURCE 资格回执，QUANTUM 与业务发布继续等资格。跨根 lease 轮转和持续/反向场景按实际 WSI 能力开放。目标 13、目标 10 镜像结果和目标 11 视觉扩展均无本卡技术前置。 [T736 本族资格](architecture-T736-v1.md) · [T774 证据边界](architecture-T774-v1.md)

## 家族与实际 GPU 角色 ABI

首个完整三态对照选择既有 **默认 DM2 / K192 / 原同族 S40 冷种子**。先取 DM20 的实际动员，再扩 DM22/29/31 与原 P→F→S/反向起动用例。保原实际输入和 Config64，以 GPU 审核后的 seed、实际上传 SHA 和原合法场域为依据。R1 可用来对照共用工程，RX 在实际位移拨动、GeometryKey/FrameWitness 接通后另取样本。每个 profile 独立开冷 run。 [T554 DM2](architecture-T554-g9-dynamic-mobilization-v2.md) · [T670 用例](test-report-T670-offline-handoff-v1.md) · [T641 RX](architecture-T641-v1.md)

| 家族 | SOURCE / QUANTUM 实际角色表与掩码 | 冷档与 reader |
| --- | --- | --- |
| `G9SPF R1` | 12 / 2024，均 `0xffff`，T524 原 10 段微步及 R1 typed gate | `G9_SPF_COLD_V1`，沿 R1 schema，T670 的 SPF 库存对照另记覆盖。 |
| `GDM2` | 12 / 2404，`0xffff / 0x3ffff`，T554 原 12 段微步与 T647 新证书 | `G9_SPF_DM_COLD_V2`，DM `.u32le` 结构 reader 和 T670 DM event/库存 oracle。当前 reader 对 GLES 源臂或冻结单程序 SHA 的校验必须按实际 PC 角色 manifest 显式扩展。 |
| `GNR1 RX` | 12 / 2404，`0xffff / 0x3ffff`，父 DM 运算及 RX typed 输入/许可身份 | `G9_DM_RX_COLD_V1`，原 `G9_DM_RX_SEGMENT_PAGE_V1` JSONL，RX 原 reader 校 CURRENT/BASE 输入 record 和实际 receipt。PC PE/角色身份用显式 PC 版封套。 |

1. **实际角色。** Windows 复用 T824 单 owner 与 Vulkan queue。set0 binding0–3 依实际 `Transport::Bind` 选 B/C/WORK/EVENT_PATH，各次分支/family 通过生成表逐 ordinal 核对象、范围和不准别名处。binding4 用生成的 std140 UBO 冻结原每根布局、控制、配置与 permit 数据。20 B push 对应原 `uOp/uRole/uJslot/uMicro/uPass`，按 U32 offset 0/4/8/12/16 反射。实际 dispatch 保 1 组 × 64 lanes，保原 `uMicro=-1` 的 U32 编码与生成表 guard。每个新增 shader 单元/变体完整输入 ≤49,152 B，另报 SPIR-V 大小。复用历史 G9 BOOT/SOURCE 闭包和实际 pass 映射，QUANTUM 的每个生产角色按可达依赖离线切分并逐个取实际创建、反射及原数值证据。登记 PE/加载模块、实际 role/SPIR-V SHA、pipeline/layout、参数/seed/input SHA、对象/arena generation 和原 pass/copy/barrier 计数。提交必须验证实际绑定的角色，不由 manifest 自证 GPU 已执行。 [T736 原绑定](architecture-T736-v1.md) · [DM 传输](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp) · [DM 操作表](architecture-T554-g9-dm-dispatch-v2.json)
2. **真实 writer。** SOURCE 先核输入、去重、量化、容量，再通过原 `SOURCE_PRIVATE_BIRTH` 真实写 P/来源，核 replay/COMPARE/SEAL，SOURCE 不推进物理时间。QUANTUM 以同一 BASE 依次保 DISCOVERY、已选路径、AUTHORITATIVE、独立拒绝前缀与 REPLAY，GPU 决定实际 `J∈{1,2,4,8,16,32}`。DM 保 `DM_REQUEST→DM_APPLY` 原 q/q+1、top-first 来源前缀和 WAIT，`FLIGHT_PATH/PARTICLE_FOLD` 写真实 P 路径，`PREDICTOR_FACE→MIDPOINT_BUILD→CORRECTOR_FACE→FLOW_ORIGIN_RESERVE→FLOW_PAIR_APPLY` 从原 entryP 到真实封存 q。保 `FRICTION_REST_DEPOSIT` 的 F→S 与 `ORDERED_CAPTURE` 的 P→F，真实入射/发射由原 TOUCH/LAUNCH 条件触发，沿原动作核 S/F/P 写集。稳定 S、贴床 F 和离床 P 分立，禁止由 M1 LAND、纹理或最终净变化重建虚假 phase。DM 实际 `EVENT64` kind15/16、审核 bit16/17 和 `PATH16` 同 scope、trial、micro、ordinal、slot/id/generation 联证，SOURCE 正常无 DM 动员/预测事件。 [三态事件字典](design-T515-three-state-manifest-v1.json) · [T648 原实现](developer-T648-v1.md)
3. **原子接受。** 循原 q、路径、seal 和有效前缀检查，独立比较 SELECTED/AUTHORITATIVE/REPLAY 的 state、有效 event/path 及拒绝前缀证明。语义比较只用各族批准的 role 位和 seal 规范化。每根一真实执行尾 fence 和 1,024 B Completion 热读。按实际 `kind/magic/revision/profile` 查私有 `Expected/NativeMappedObservation`，以原 `[80..87]` SHA 字段核实际写尾角色，并另核当前根的完整角色集合及逐 ordinal 身份。由 owner 同一短锁重验冻结 permit、期限、scope、FIFO 和对象代后原子晋升 `state + arena + accepted clock + publication`。所有正常 QUANTUM 成功才推进 A 一次，合法新 SOURCE/q0 metadata 可按原 outcome 规则消费一次且不增 A，重复不生成新内容，BLOCKED/FAULT/撤权/迟到保前缀。保 `250 ms` 根截止、`33,333,333 ns` freshness、物理 `120 Hz` 与原配额。PC 以同源 QPC 记录原 ticks/frequency 和真实采样点。 [T647 typed gate](architecture-T647-v1.md) · [T524 事务](architecture-T524-g9-spf-engineering-v1.md)

## 接受键、冷档与同帧呈现

**冻结接受关联键。** Completion/BASE/实际 permit 联核 `family/profile, kind, run, owner/context, surface/geometry, permission/config/inputEpoch/session, plan, nonce, baseCommit→commit, A_before→A_after, selectedJ, physical bank/arena generation`，外层绑定本进程、真实 device/window、实际 pipeline-set 身份、fence/result/QPC、publication generation 和 `leaseSerial`。字段宽值保 LE 原字或规范十进制字符串。ROOT/FULL、像素和原始输入用同一接受键关联，显示模式/lease 身份从实际资源记录取值。不能由统计时间、最新 nonce、恰好相同的 GPU 句柄或外部 JSON 的 `accepted=true` 授权。 [T842 关联负控](test-report-T842-v1.md)

- **冷接口**沿各族 `ColdBegin`、`ColdState`、`ColdPage` 和 `ColdEnd`。PC 调用方提交冻结接受键、mode 和 replace 选择，owner 只对同代键发 traceId。续读按原 trace/page ABI。先根据原 ROOT/FULL 模式预约信用，锁住该根的真实 BASE/FINAL/SELECTED/AUTHORITY 及对应 arena。FULL 的六个 discovery arena 须在根执行前预留并取实际执行的原件，未执行标 `NOT_EXECUTED`。不能在已发布 ROOT 后从摘要补成 FULL。复制使用 actual `VkBuffer+storageGeneration/range`，依 shader-write→transfer-read、transfer-write→host-read、真 fence 和必要 `nonCoherentAtomSize` 对齐 invalidate 后冷读。中间 state/arena 的生命周期延到真实 copy 或消费退休。流式输出同族 32 KiB prefix、原四段或十段、真实 valid count/stride/段 hash 和全档 SHA，不改 LE 原字或 padding 保留区。两 slot、单 builder/reader、60 s 不续、页 ≤1024 U32/16 KiB，撤销仅先失效，实际释放后才归信用。DM/K192 的原单 slot ROOT/FULL 是 **26,750,976 / 103,354,368 B**，两 FULL **206,708,736 B**，Vulkan 对齐、staging、显存与窗口附件并存另记。 [T554 原资源表](architecture-T554-g9-dm-manifest-v2.json) · [DM 冷入口](../entry/src/main/cpp/gpu_resident/g9dm/controller_cold.cpp) · [Vulkan 同步](https://docs.vulkan.org/guide/latest/synchronization_examples.html) · [映射失效规则](https://docs.vulkan.org/refpages/latest/refpages/source/vkInvalidateMappedMemoryRanges.html)
- **场景接口**沿 `AcquireScene → Present → ReleaseScene`。lease 同代 pin accepted state、相应 arena/pack 和本人图形消费范围。按原场景在同一完整 shader candidate 填 S 的 `0..S`、F 的 `S..S+F`（FLOW/REST 原料和实际 p/F）、按真实 packet x/y2、mass、id 和 generation 绘 P，再合前景。不借 G11 marker 推进物理。compute→实际 vertex/fragment 可见性、图形消费 fence、WSI 图像重用分别验证。只在仍同代且成功 Present 时登记 `PresentedMapping`，实际可见证据单存，返回码不会推算屏幕已显示。换尺寸/DPI、焦点或 surface 撤权按实际事件/许可域阻断旧接受。故障只报告 FAULT 或标 STALE 上个完整 shader 帧。保 GPU 驻留，禁止 Canvas 和 CPU 仿真回退。 [T824 窗口合同](architecture-T824-v1.md) · [DM 场景](../entry/src/main/cpp/gpu_resident/g9dm/scene.cpp)

## 独立判定与交接

- **先验冷档，再从原字导出指标。** T838 当前硬接 `tools/g9rx/read_cold.py`，并在 RX JSONL 首行读取 Completion，不能直接给 DM/R1 归档。tester 按实际 `family+schema+backend` 选择 reader，输出共同 `AcceptedInterval`，从本族 prefix/STATE/事件有效段解 `nonce/run/geometry/commit/A`，与外部 GPU/窗口链交叉核。保测试端可复算的区段/SHA、源行范围、有效数量及原始符号单位。S/F/P 从已接受 state 与真实活包列重算，来源量从 bed cohort、F origin map、活 P 和实际 SOURCE/source rows 对账。面通量从所选权威、封存的 `EDGE_FLUX` 方向及 q 取值，排除私有预测/拒绝前缀/重复重放。逐来源搬运若未直接留 writer plan，须沿原 top-first/Hamilton 从同 BASE 和有效事件独立重建，缺件时该指标为 `null`。同 token/tag 的手填数组只能作夹具。实际图像另验 Vulkan format/sRGB/行向及 RGBA8 原字 SHA、实际尺寸、lease/geometry 和呈现来源。 [T838 实现](../test/t838/check.py) · [DM reader](../tools/g9dm/read_cold.py) · [RX reader](../tools/g9rx/read_cold.py) · [T670 差分](../test/t670/check.py)
- **单次真实事务与三态连续链。** 先验证 SOURCE+首个 QUANTUM 的原流、cold、同一 scene；再按 T779/T670 验有/无外源的总量、逐来源量、动量/能量和 stable S，以及实际 `S→F→P`、`P→F→S` 的触发、库存、packet 时序与冷档。无 SOURCE 的 QUANTUM 逐来源总量守原账，净变为零也按实际通量验局部运动。冷档负控覆盖错族/角色/SHA、未 seal 的 valid prefix、离散来源偷换、假相转化、撤权或晚 fence/旧 lease、错 geometry 与把 M1/单 tail 当 G9 archive。资料不足按缺件记录，独立 reader 结果不代 native proof。
- **同条件镜像与观感。** 复用 T838/T814 的 `x'=128Q−1−x`、面方向翻转、同 profile/参数/layout/时窗及实际 J。两侧独立 run/原始档，分别核总量、逐来源、面通量及像素，保不同量的分母和实际窗口帧。A05 既有整数 tie 与 A06 对照照实登记，无新增容差。当前 60 Hz 可测功能和计算吞吐，持续 120 FPS 可见验收待实际刷新能力及业务内容。 [PC 帧率设计](design-pc-effects-120fps-v1.md)

Developer 先交实际 FullM1 平台 ABI 对照及 G9 角色/绑定 manifest，再接 DM2 工件。Tester 可先准备按族读档和精确负控，effector 拿实际三态窗口原件后做同根对账和观感。T845 核验范围是 T830 原始 JSONL 与交互构建的 PE/SPIR-V 身份、原角色/冷档表，以及 T779、T838 和 T670 离线回归。实际 G9 PC 角色、三态演化和原始冷档尚待取证。
