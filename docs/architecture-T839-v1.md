# T839 · 目标 11 PC 真实通量与视觉接线工程合同

2026-11-22 · architect · P3 · Windows x64 Vulkan

**验收单位是一帧真实窗口图像及其唯一已接受的 G11 联合 publication。** 由同一接受根把物理侧的最终来源通量、视觉 marker/glyph 和实际 Present 连起来。复用 T639 rev2 的实现语义、T687 的归档工具和 T558 的视觉判据，由 T824/T830 提供 Windows owner、输入、资源与呈现接缝。 [T639](developer-T639-v2.md) · [T687](test-report-T687-v1.md) · [T558](algorithm-G11-surface-lateral-rendering-v1.md) · [T824](architecture-T824-v1.md)

## 实证与接入触发

- T639/T687 已交双父 G11、真实生产源码闭包、包内 shader 和 14 项离线归档夹具。原手机 `STOPPED` 无有效组合帧，T708 已核合并 ABC 含 Worker，独立条目读取失败与实际配置首拒应分别解释。PC 冷档由 native 调用方流式写原件，配置回执、接受状态及归档终态各自保留。 [T687 原件](../test/reports/t687-g11/independent.json) · [T708 分流](architecture-T708-v1.md)
- T830/T835 已证实 UHD 630 上 Win32 surface/swapchain、GPU BOOT 诊断像素和 Present，诊断帧 `acceptedRoot=null`。T830 后续交付**实际窗口上的已接受非空业务根**和冻结的输入/permit/transport/completion、资源代、publication/lease/Present ABI 后，才接 G11 实际产品路径。准备可与 T830/T833 并行。若 T830 首根是 M1，复用 Windows 平台 ABI，G11 自己接真实 G9 R1/DM 父 writer、原完结门与版本化资源。目标 13 纯流场和目标 10 镜像结论均不作为目标 11 的工程前置。 [T830](developer-T830-v1.md) · [T835](test-report-T835-v1.md)
- 首个 PC 组合档选择既有 **R1/K192/V2048、默认 blend256**，新 run 冷初始化，研究入口默认 OFF。DM、K512 或 V8192 按自身类型和实际模块独立取得证据，blend0 只作独立冷配置对照。装载一致性保真实 PE/模块 SHA、backend/adapter/driver/device 代、实际角色及反射、parent/recipe/config/layout/pipeline/seed SHA 与生成依赖。R1/K192/V2048 的原组合 WORK 是 **4,825,728 B**，原额外视觉 payload **3,538,944 B**。实际 Vulkan descriptor range、alignment、storage/vertex/fragment 能力、显存及双冷槽并存预算逐档重测。 [T660 v2 机器表](architecture-T660-g11-manifest-v2.json)

## 接口、所有权和时序

| 接缝 | PC 实施合同 |
| --- | --- |
| 窗口与输入 | Win32 UI 线程投递 T824 的实际窗口、DPI/尺寸、指针/可控重力事件和 QPC 原始时标。物理 owner 使用对应父 `Config/InputGate/ProgressClock/Permit` typed 路径。真实窗口指针、测试输入及等效角度各留来源。事件入队和父 SOURCE 真正接受分别记录，观察反向以物理 `q` 过零为准。同输入比较以真实接受时段对齐。 |
| 冷建与根许可 | 复用 T660 `VisualConfig64V2`、原物理 seed 及 `actualVisualSeedRawSHA` 原前像。先 pin 冷建的真实 CURRENT header512、marker128V、全 512 行 Pstyle8192 和 config256，实 fence 后按原顺序核 4 段原字。封存 seed receipt、真实归还相同 witness 信用后再开 session、许可与首 claim。沿 T648 由实际 pipeline set 的私有 owner 生成 `ActiveProgramIdentity → Expected224 (+ SHA8) → NativeMappedObservation → 父 proof → CompositeGate`。真实 tail/expected 比较保同 scope/nonce、父/组合身份、首错、完成掩码与数量。 |
| Vulkan compute ABI | 首版同一 set 0 的 storage binding 0–3 沿实际 `VisualTransport::Bind` 映射 `B/C/WORK/EVENT`，binding 4 用生成的 std140 UBO 装冻于根内的 `Common` layout/permit/原常量。按原 `Dispatch` 参数序冻结 36 B push，U32 offset 0/4/8/12/16/20/24/28/32 对应 `uOp/uRole/uJslot/uMicro/uPass/uVisPhase/uVisOldBank/uVisNewBank/uVisOrdinal`。保 U64 原 low/high、符号角和字偏移，特别比对 SOURCE replay 旧 B、实际/重放与 FULL 分支的 state/arena。用生成的 native 编码、SPIR-V reflection 与实际 descriptor/range ledger 同核。descriptor 与 UBO 段须冻结到实际消费者退休。 |
| 运行及联合接受 | 原冻结 dispatch 每根 SOURCE 19，R1 QUANTUM 2543，DM QUANTUM 2923，四 storage binding 和一尾 fence/1024 B 热尾读回。保持 250 ms 根绝对期限、33,333,333 ns freshness 与 120 Hz 物理钟。GPU bit18 证明计划映射，bit19 证明独立视觉重放与 glyph，原 mask 为 R1 SOURCE/Q 和 DM SOURCE `0xcffff`、DM Q `0xfffff`。只有父 proof 与视觉审核都成立，owner 才按原同一短锁一次提交物理 bank、视觉 bank/generation、工件与时钟/FIFO。窗口/许可在途撤销、丢设备、坏尾或旧 fence 保旧接受前缀。 |
| `AcquireScene` / `CanPresent` / `ReleaseScene` | 在现有不可伪造的 `SceneLease` 语义上拿同一个 `G11_JOINT_PUBLICATION_V2`，联合 pin 物理 state、视觉 marker bank、单份 glyph WORK、Pstyle 和相应 pack/裁剪读范围。PC 外层附已接受尾中冻结的 `rootNonce/rootKind`、commit、`acceptedQuanta/visualTicks/visualGeneration`、实际 bank generation、lease serial 和 `window/surface/swapchain/geometry/permission`。从实际已接受尾取 nonce，当前队列或 owner 的最新 nonce 不参与拼接。接受根是物理身份，图像使用的根与实际 present receipt 分别保存后以 lease 对齐。尚无实际 GPU submit 时先取消已录制命令，再退 pin。draw 已提交须带真实图形 fence，缺 fence 隔离该份资源。 |
| `Present` 与冷档 | 在 record、提交/Present 前及登记结果时核同代 lease/许可。提交候选整帧后，保 acquire、submit、fence、present/image 原结果及 QPC，记录 image index 和真实支持时的 presentId。`PresentedMapping` 只由实际成功且同代的 Present 推进。应用返回与可见显示分别留原件。冷 `Begin/Page/End` 只读同 token 已 seal 或明确故障诊断资料，复用原字典和私有 credit。 |

真实 QUANTUM 才推进 `acceptedQuanta` 及 `visualTicks=acceptedQuanta×256`。SOURCE 为零物理时长，非重复 SOURCE 可按原父合同更新库存和 generation，duplicate 不新增提交或淡化时间。重复呈现、暂停和 STALE 沿最后已接受相位。保旧的完整有效帧时明确标 STALE，否则报告具名 FAULT。 [T647/T648 接受接口](architecture-T647-v1.md) · [现有 Controller](../entry/src/main/cpp/gpu_resident/g11vis/controller.h) · [T660 seed](architecture-T660-v1.md)

## 真实通量与单批 witness

1. GPU 材料 writer 逐来源记录原 Hamilton 最终左/右份额与实际转换区间，按 `OLD_BED → NEW_BED → OLD_FLOW → NEW_FLOW` 保四 map 的既有二进制排列。FLOW 在 `VIS_PLAN_BEGIN → FLOW_ORIGIN_RESERVE → FLOW_PAIR_APPLY → VIS_PLAN_APPLY` 连续交接，SOURCE replay 的旧图取真实 BASE。全程保原 phase、role、ordinal、来源键和相同的父物理写集。ACTUAL 角色 2 和 REPLAY 角色 4 各从同一视觉 CURRENT 独立消费计划并比较全部 32 个 marker words，再构建唯一 glyph。发现试探或被拒微步无可发布视觉写入。T563 两个完整表均为 SOURCE 2 对、QUANTUM 258 对计划 BEGIN/APPLY 槽，实际执行数量由父选择及 GPU 守卫给出。 [R1 表](architecture-T563-g11-r1-dispatch-v1.json) · [DM 表](architecture-T563-g11-dm-dispatch-v1.json)
2. 按实际 `(run, rootNonce, micro, phase, role)` 预预约**一个** `G11_PHASE_WITNESS_V2`。SOURCE 的 `micro=-1` 在 wire 为 `UINT32_MAX`，FLOW 是 phase 3。生成表可将四个选择映射到唯一 BEGIN/APPLY ordinal。R1 `micro=0` 的 FLOW，ACTUAL 为 `699→702`，REPLAY 为 `1965→1968`。实际进入时，在 BEGIN 后取该角色旧 marker，在 APPLY 后取该角色新 marker、完整 plan/fourMaps 与当时 bank/plan header，使用同一 root 尾 fence，保真实 batch scope、提交终态和 locator，另记 copy/barrier/bytes。表内命中而执行守卫未过、批次 header/ordinal/seal 或接受尾对不上时记 `NOT_CAPTURED` / `EVIDENCE_MISMATCH` 并保首错。预预约和主证先用 ACTUAL，独立角色原件用同条件另一次确定性回放或原全量档对照。 [现有捕获位置](../entry/src/main/cpp/gpu_resident/g11vis/phase_witness.cpp) · [计划与字段](architecture-T563-g11-accepted-flux-visual-v1.md)
3. 保 `G11_VIS_COLD_V2` ROOT/FULL 11/17 段、32768 B header、七段原 VIS 原件、LE U32 和段/整体 SHA。ROOT/FULL 的 plan/maps 是最后实际批次，横移持续性须用连续已接受时段和指定批次 witness 或原父 EVENT/PATH+冻结 BASE 独立重建。`test/t687/read_archive.py` 先核结构、字节数与 SHA，再由判定器同 accepted tail 核 witness header、plan 角色/micro/ordinal、map 方向和旧/新 marker 数据。V2048/V8192 各保 **3,149,824 / 4,722,688 B** 一份 GPU 加一份 native 同量信用，和 bootstrap 互斥，ROOT/FULL 两 slot、单 builder/reader、60 s 不续，页 ≤1024 U32/16 KiB。退休后分段读；热路禁止整池下载、CPU 回填通量或纹理滚动。 [T687 归档 reader](../test/t687/read_archive.py) · [W01–W12](../test/t687/CASES.md)

## 同代 shader 画面与 Vulkan 资源

- 完整候选按底材/背景 → glyph → 真实 P 与单代表旧 P ghost → 前景 → 合成到窗口。`glyph8` 的 loc0/1 是两个 U32x4（偏移 0/16、stride32、instance），正确读取旧/新锚及 kind/blend 字段。P loc0–5 沿实际父 attrs、Pstyle、id/gen 的原反射。固定锚点按已接受 tick 淡化，保 rev2 旧 P 的 `{slot, generation, id, phaseStartTick, 原 CAPTURE 质量}`，旧 ghost 无权解引用复用后的物理 slot。取同代 S/F/B 和前景遮罩，F 可见带 `min(F,Q/8)`，有厚度才画原层，亚像素或无可见层按原 `NO_VISIBLE_LAYER`。保原 alpha、坐标及颜色语义。 [T660 ghost](architecture-T660-v1.md) · [实际场景](../entry/src/main/cpp/gpu_resident/g11vis/scene.cpp)
- 同设备同 queue 优先。原 dispatch 间 shader write→compute/transfer/实际 fragment SSBO 或 vertex attributes，冷 witness 加实际 transfer→host 与 noncoherent invalidate，copy 完成前防同区 next writer。整帧附件到合成/Present 的同步与已接受 publication 分开。图形读结束的 fence 释放物理/视觉/glyph pin，单份 glyph 被读期间下一 writer 背压。swapchain image 的 WSI 资源按 T824 重获该图像并等其信号，或实际已启用的 present-fence 路径退役。尺寸/DPI、surface/adapter 换代撤权，不将旧图标 CURRENT。禁止 Canvas，前后背景均走 GPU shader。 [Vulkan 同步](https://docs.vulkan.org/guide/latest/synchronization_examples.html) · [Present 信号量复用](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)
- 核 T687 实际计算源码 R1 **168,369 B**、DM **185,386 B**，两者沿其原 SHA 作冻结对照。新增 PC GLSL 每个 compute/graphics 编译单元和实际变体完整输入 **≤49,152 B**，另报 SPIR-V bytes，直接生成 SPIR-V 的单元按模块大小过同一门。建议按现有 dispatch 的父根/审计及 seal、材料 writer（闭包保真正 q/事件源）、VIS copy/plan/独立 audit/glyph 等可达闭包先做离线分组，单一角色超限按原阶段边切开并回 architect 核读写/屏障。先证小角色能建管线，再按原表覆盖全角色，逐个保反射、时序、实际组与资源范围、父资格及持久化身份。静态逐行核原调度、原尾、GPU 审核及每个 opcode 唯一匹配。PC 私有 `ProgramFacts` 绑定实际已创建且在本根逐项核对的 pipeline set，使用包含 parent、角色/opcode 映射、最终模块 SHA、pipeline layout 与生成版本的冻结集合身份并沿 `Expected224` 和尾校验，历史单程序 cache/reader 保原身份。根状态/位/字段宽度沿既有 rev2。 [生成/编译收据](../test/reports/t687-g11/independent.json) · [原尺寸规范](architecture-T727-v1.md) · [T834 PC 隔离经验](architecture-T834-v1.md)

## PC 验收与角色接力

| 阶段 | 要取得的原始证据 |
| --- | --- |
| 首个业务正控 | R1 小档真实砂源、实际非零通量 FLOW、同 scope 两侧终态/父证书和角色独立视觉对照。一个窗口帧用接受尾、witness 与 `PresentedMapping` 关联起来。W01–W12 用真实配置、seed、19/2543、绑定和事件、lease 及冷档逐项判定。 |
| 真实位移与反证 | V01 恒向真实 q、V02 弱流跨根累计、V03 无相转化且 q0 的精确位置冻结、V04 真实 q 变号后反向。按 per-origin 入出份额独立复算 rank 和 `x=c+(r+1/2)/m`，均匀流 `Δx=q/m`，净库存相同仍可有运动。单有 p/jp、输入角改变、相机缩放、时间 shader 或帧末高度差均没有来源通量授权。 |
| 同帧着色、相变与容错 | V05–V14 核真实裁剪、静床/遮挡后原 visualId、P↔F↔S、ghost/slot 复用、旧 lease 拒绝、原点和来源 tie、两角色逐 word 校验、SOURCE 零时长、重复/STALE。按原实验做同输入视觉 ON/OFF 物理 hash、实际通量和 ID/账本等价。比较独立镜像时分开物理 Hamilton tie 和视觉量化误差。若 `VIS_DENSITY_LIMITED`，照实报告样本密度与物理账。 |
| 窗口观感及成本 | 同一已接受仿真时段比较底材和组合着色，附真实 q、相位、各层像素/遮罩、实际窗口时间序列。记录起动、弱流、真实反向、停流及纹理闪动。分别报冷 pipeline、暖加载、GPU/owner wall、显存、acceptedSimTime/wallTime、真实新帧和重复帧。当前 60 Hz 可验功能、采吞吐，持续 120 FPS 可见验收留有实际输出能力时执行。原 T558 无独立最终观感数值阈值，不补设阈值。 |

developer 取得 T830 非空窗口根与 ABI 后对照上述接口，先交原模板 R1 工件及模块/角色、lease 和 witness 收据。tester 可先沿 W01–W12、V01–V14 隔离归档完整性与物理/视觉语义，客观记录实际覆盖和负控。effector 在实际窗口复验图像与观感，保根和画面原始身份。T839 核对两份 dispatch 表共 5,504 行、T687 冻结 shader 哈希，复跑 14 项归档夹具和 T558 配套的 6,664 个给定计划、58,660 个 rank 映射。本轮为离线核对，G11 的 PC 业务根、通量和真实观感待实施验证。 [T558 夹具](design-T557-T558-fixtures-v1.json) · [PC 帧率设计](design-pc-effects-120fps-v1.md)
