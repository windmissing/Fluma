# T660 · G11 实际视觉种子与 P ghost 持久合同

2026-11-19。架构交付，**ENGINEERING_READY_NOT_IMPLEMENTED_NOT_PRODUCT_PASS**；不使用真机，不运行产品、shader、测试或构建。消费 T639 `b0143d23`、两条原问题 `6afeba20e4b0e5384a544ca3` / `6afebd90e4b0dc337eac0d2e`，以及 T648 `2a670f2` 的真实私有 proof 工厂。后者已解决原父接缝，不能再把旧 ede5891 缺口当当前前置。

本修订覆盖 T563/T634/T647 对这两个缺口的旧含糊描述。完整新机器表 `architecture-T660-g11-manifest-v2.json`、配方 `architecture-T660-visual-recipe-v2.json`、增量合同 `architecture-T660-contract-v1.json` 是原 T639 接续依据；旧 v1 文件/生成物保留为历史，不混用。不是新增整套开发卡，不以设备测试/manager 批准作为续作前置。

## 1. actualVisualSeedRawSHA 的唯一前像

令 r 为新机器表选定的八档资源行，WORK 为该真实组合 GPU 对象。只在**新 run 冷初始化成功、初始物理 CURRENT 已冻结、任何 SOURCE/Q root claim 之前**读取如下四段，按表顺序直接拼接。每个 word 严格 LE U32；不得序列化 JSON、补对齐、加摘要、重排、归一化角色或省略零字节。

| 顺序 | 原件 | WORK 内定位 | 字节数 |
|---|---|---|---:|
| 0 | CURRENT bankHeader128 | r.control.byteStart + 0 | 512 |
| 1 | CURRENT 全部 V 个 marker32，含 INACTIVE、word30 与 seal | r.markerCURRENT.byteStart | 128×V |
| 2 | 全部 512×Pstyle4，K 外空槽也含 | r.control.byteStart + 1536 | 8192 |
| 3 | VisualConfig64V2 全原件，含保留零 | r.control.byteStart + 60544 | 256 |

`actualVisualSeedRawSHA = SHA256(Hcurrent || Mcurr || Pstyle || VisualConfig)`，长度 **128V+8960 B**：V2048 为 **271104 B**，V8192 为 **1057536 B**。每段的 GPU 源 offset、拼接 offset/长度与单段 SHA 由 reader manifest 逐项登记；最终 SHA 是上述原件的 SHA，不能 hash 四个段 SHA 代替。摘要标准32字节按每4字节 LE 放入8word，沿 T647 的 HashWords 约定。

不包括 ACTUAL/REPLAY bank/header、glyph、plan、四 map、运行 counters、FrozenExpected224/hash、尾256、文件 manifest、物理原件或配方文本。CURRENT header 已含 parent/composite/K/V、owner/context/run/surface、visualConfigSHA、**实际组合 pipelineSHA**；初始 generation/tick/baseCommit/rootNonce=0。header rev=2，字典保留零照验，nextVisualId/activeCount 等是 GPU 初始化后的真实值。物理 seedSHA、inputEpoch、page session、初始物理 bank/generation、layoutSHA、BuildId 作为同一 bootstrap receipt 的外部绑定，不能从 header 猜这些值。最终同一个私有 owner 把该 receipt 与物理初始 token 绑定，任何 scope/物理 bank/config 变化使 bootstrap 作废并冷重建。

GPU 冷初始化在既有专用冷初始化 dispatch 内完成 CURRENT marker/header 和 Pstyle：live packet 的 id/gen 来自初始物理 state，opacity=65536，representative 为相符 CURRENT P marker 的最小 slot，无样本用 NONE；非 live/K 外行全零。初始 phase 不活动，ghost 字段零。ACTUAL/REPLAY 不是本 seed 的授权原件，首 root 仍独立从 CURRENT copy。不调用会依赖 Expected/accepted plan 的正常 glyph-build 来制造 bootstrap。现有候选冷初始化尚未完整产生这些字段，由 T639 完成。

## 2. 先 raw/hash，再首个 claim；复用 witness 信用

接口为 native 内部类型，不能由 JS 构造已验证身份：

```cpp
BootstrapResult BeginVisualBootstrap(const TypedParentInitialToken&,
    const VisualConfig64V2&, WitnessBroker&, BootstrapTicket*);
BootstrapResult PollVisualBootstrap(const BootstrapTicket&); // owner GL thread
ReadResult ReadVisualBootstrap(const BootstrapTicket&, uint32_t segment,
    uint32_t byteOffset, Span<uint8_t> caller, SeedPageMeta*);
BootstrapResult SealVisualBootstrap(const BootstrapTicket&, VisualSeedReceipt*);
```

时序：固定父/config/组合程序 → 占用既有 singlePhase witness 的**一个 GPU 保留和一个 native 块** → 完成冷初始化 → 同 GL owner 对四段原件 copy 到该 GPU witness 的 packed raw `[4096,4096+N)`（前4096B是既有witness header；hash前像自身从0编号，不含该header）→ 冷初始化 fence 实际 signal 后，按既有 bounded caller-span 路径搬到同一个 native 块 → 流式核各段/整前像并封存只读 receipt → 所有 GL read/map/copy 真正退休、reader End/TTL 实际释放后退回 witness 信用 → 私有 owner 使用 receipt 设置 ActiveProgramIdentity.actualVisualSeed → ExpectedForActivePipeline → 首 claim。

初始化器在封存前只消费冷配置/真实程序身份，不读尚不存在的 Expected.actualVisualSeed，故无自 hash 环。`Expected224[144..151]`、GPU 尾 `[224..231]`、冷 manifest 的实际 seedSHA 均取同一封存值；recipeSHA、physicalSeedSHA、未知零禁止代填。整个 flight 内保持 ProgramFacts/permit/Expected 引用有效且不可变，按 T648 私有 friend VisualTransport 产生 ProgramFacts/NativeMappedObservation，经真实 R1/DM 父 proof 再组合门；JS/raw-tail 不能授权。

bootstrap 与 phase witness 互斥，不占额外 FULL/ROOT 冷槽、不新建历史缓存。native 块不能在 credit 归还后继续给 reader 借址；reader 一次一个，caller≤4096B、JSON≤16KiB，TTL60s不续。TTL/撤权/Stop/换 context 只置失效；GL fence 未退休或 reader 未释放时仍不可复用，报告 WOULD_BLOCK/RETIRED_NOT_FREED。不得在 GL 热路径写文件；tester 可用 caller-span 流式归档。保存的固定 seed receipt 不保存全前像；后续完整冷档继续记录 seedSHA 与初始 receipt locator，不能拿**演化后的** CURRENT 重算初始 seed。

容量：既有 witness GPU/native 各 **3149824 / 4722688 B**，四段 packed N 分别271104/1057536 B，小于各自上限。header 元数据只使用既有 witness 4096B header；不另分配 N 字节镜像、不占正常 root 的1024B读回。冷创建实际 copy/read/fence 次数和字节单独报告，不冒零成本；本修订不要求额外正常 root dispatch/barrier/fence。两 DM FULL388308992 B、最大已知 witness 并存包络641920128 B不增；固定 owner helper 的 ABI另计，见第5节。

## 3. 正式登记 markerRevision2：旧 P 质量和身份

不借用 massSnapshot 作歧义 union。**marker32 word30** 从历史 reserved0 正式登记为 `phaseOldPacketMassQ20`，word31 seal 保持 `0xC0DE1101`；版本由 VisualConfig64[17]=2 和 bankHeader[1]=2 强制选择，readerRev2，configRev2。未通过 rev2 身份校验的 v1 仍要求 word30=0，禁止静默升级/热续接。

质量取真实 CAPTURE event64[15]（该批 plan.amount 同值），在清 slot/覆盖 plan 前按 sealed event 的 slot/id/generation、origin、nonce 等逐项核对并写入各自 ACTUAL/REPLAY marker。必须为合法父包的非零 U32 Q20 原质量；不把最终 F 总质量、glyph6、重用后的 slot 或后批 event 当旧包质量。word10 始终是当前 owner 的 massSnapshot，F 为当前对应最终 cohort 质量，不更改父物理/转移算法。

同时登记既有 word3/4/5 的**互斥 union**：owner=P 时为当前 packetSlot/generation/id；owner≠P 且 phaseActive、phaseOldKind=P 时为旧 ghost packetSlot/generation/id；其余情形全零。P→F 后保留的是旧身份 locator，不再授权读取物理 slot。用于代表分组的 key 为同 run/context 下 `{oldSlot, oldGeneration, oldId, phaseStartTick}`，与该 marker 的来源/旧锚点/质量一致；真实 slot 下一代即使已存活也不能更新 ghost。

| 状态/事件 | word30 与 word3..5 |
|---|---|
| INACTIVE/初始化/无活动 old-P phase | mass=0；非 P 的3..5=0；当前 P 的3..5仍真实身份 |
| 真实 P→F 且 blend256 | 对两个角色分别从各自合法 event/plan latch 旧 q+旧身份，phaseStart=真实转换 tick，span=256 |
| SOURCE零时间，F同 owner 混合/移列，跨 root/plan覆盖 | 保持旧 q/身份/anchor/start/span，不因新 F mass 或 slot 复用而变 |
| 又发生 owner 转换/rebase | 沿既有单 phase rebase：旧 phase 终止，按本次真实 old owner 建新 phase；old owner 非 P 时清旧 mass/非 P locator；不得叠加第二个 ghost |
| phase 到期、样本失效/退役 | 清 mass、oldKind、代表标志；非 P locator清零，当前 P locator保持真实 |
| blend0 对照 | 转换立即完成，不保留旧 ghost，mass=0；不能拿它替代默认 blend256 |

不能以源码当前对 blend>0 的23拒绝作为正式可用策略。默认256、独立0对照两个 profile 的参数 SHA 必须不同；视觉 tick仍每接受 quantum=256，SOURCE=0，不随墙钟/Draw/FPS推进，不把旧文32物理tick误作视觉单位。

ACTUAL(role2)/REPLAY(role4)从共同 CURRENT 单次 copy 后，各自消费真实同源合法 plan、更新/过期/重基；bit19 前比较**包括word30与union3..5的全部32word**，不能从 ACTUAL 复制到 REPLAY补相等。各自先按上述 key 独立确定最小 marker slot，校验同组旧质量/旧锚一致、仅一代表；单个 glyph8 不充当第二角色历史存储。glyph6 对活动 old-P ghost 取 marker30；当前 P 非 ghost 沿实际 packet mass，其他0。共享 glyph 只在独立比较后由 ACTUAL 生成，再逐字段校验/计算代表；无任何 CPU 补 bit18/19。

Pstyle4仍只服务当前真实 P 的 id/gen/opacity/representative，不增加质量或借给 ghost。ghost 代表可在既有 marker flags.bit4登记并接受独立角色比较，min归约使用已预留 markerIndex/columnIndex scratch；不得占剩余control保留字。GPU dispatch不增，用既有VIS审核/归约步骤实现并报告实际复杂度。当前P body每slot仅一次，旧ghost按代表一次、其它marker保新F点。未完整实现应 FAULT/保留明确STALE新管线帧，不能双画或改淡化以冒闭环。

## 4. 字典、生成链与身份切换

完整 v2 manifest 保留八档 WORK offsets/budget，正式叠加 T647 已有 `[61312,62208)` Expected224、`[62208,62240)` ExpectedSHA8；真正保留零为 `[62240,65536)`。没有把这928B当新内存。本次新登记word30/union不增加marker/glyph/plan/control长度，物理State/Config/input/数学/时间不改。

VisualConfig64 的[1]=2、[16]=1 descriptor、[17]=2 marker、[18]=2 glyph语义、[19]=2 reader，其它字段沿原正式参数。bankHeader rev2；配方marker/scene rev2，sampling/物理配方不变。冷路由 `G11_VIS_COLD_V2` / `G11_VIS_SEGMENT_PAGE_V2`、bootstrap `G11_VIS_SEED_RAW_V1`；旧reader拒新字典，新reader只有显式历史只读模式才接v1，不能给v2运行背书。T647 Expected224/尾256/十行family/rootKind的 ABI和静态门不改；组合layout/config/recipe/actualshader/pipeline/cache/reader身份全部重算，program二进制ABI salt含T660新合同SHA，旧冷缓存/程序不热续接。

原 T639 developer 负责同步 `tools/g11/generate_contract.cjs`、`test/t566_generate_composite.cjs`、`tools/g11/prepare_compile.cjs`、g11vis generated_contract/native/GLSL/reader、kernels.cmake/cache dependencies：统一读v2 manifest，输入缺失/旧SHA/stale生成物立即fatal；复制构建输入时必须含T660合同/新配方。生成 native/GLSL/reader 的word30/union/版本/seed段字典，不手写另一份offset。**本卡没有运行旧生成器覆盖历史，也没有实现业务代码；新合同已可实施，现有候选代码仍rev1，不冒已切换。**

## 5. ABI、接续与角色边界

T648实测AArch64 ABI只读复用：Expected224=896B；ProgramFacts408B、ActiveProgramIdentity416B、ExpectedContract56B、NativeMappedObservation80B、双父CompositeProof各1064B。这些是实际类型尺寸，非编译器栈峰值。T639新增常驻 bootstrap helper 采用**平坦128word / 512B POD预算**（机器表给出字段范围），只保存身份/摘要/段描述/状态，不存 raw N/Expected896B，不再另嵌ProgramFacts。其边界是新增明确host bookkeeping512B，原GPU/cold包络不增；T639若另常驻ActiveProgramIdentity/两个proof或allocator元数据，须逐项按实际sizeof和生命周期报账，不能声称都被512B覆盖。witness退信用后仅512B receipt可保留。

原T639继续 W01–W12：typed双父owner/真实Expected/冷初始化和首claim时序、材料/marker/全部审核、联合提交/SceneLease/完整XComponent帧、真实ROOT/FULL/witness/reader。保留b0143d23局部成果；不改默认g9dm/共享proof/g9rx，不再等待T648设备PASS。developer完成开发与编译即可；tester/effector负责签名部署、真机证据及独立双角色/负控/ghost/source跨根与slot复用验证。

执行prompt：不使用真机；不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。XComponent+shader覆盖所有背景/前景；失败FAULT或明确STALE，无Canvas/软件GL产品回退；CPU停止、目标10暂停、水13→14→12。

静态核对见 `analysis-T660-evidence-v1.json`：八档offset、原件拼接/容量、版本/union合法域、T647928B无重叠、Expected/尾ABI与真实工厂接口。本合同修复的是持久字段/身份工程缺口，未更改算法，无需转designer猜新的物理策略；不宣产品/视觉PASS。
