# T378 · CPU 跨列受阻纵向推进工程合同 v1

2026-11-06，architect。不使用真机。按 T377 设计、T353 §7 与 T369 原档，交 T381 实现。P3 CPU 参考，GPU 依当日主线排期。T377 的 Python 模型证明夹具算术，实际 Hot/M2 提交、回滚和产品验证由后续卡执行。

## 1. 启用、身份和文件归属

新行为只在冷建/同版本恢复的 `CPU_ORIGINAL_F64 + P011_DRY_CACHE_EXACT_V2 + DRY_AIR_V1 + P011_CPU_DRY_CROSS_VERTICAL_V1` 开启。`P051DerivedFacadeV2.ets::DerivedConfigV2` 新增可选 `motionRevision`，省略解析为旧行为，未知值冷拒。该属性跟随 owner/field/work，不做热 toggle。首版只支持显式隔离 V2 dry run、既有 `T216_DRY_WRITERS_V2` writer 图和正常 `STEP` 通道。若同自然观测/其它 writer 组合无法按本稿证明闭包，应在启用入口明确拒绝该组合。旧路与新路不跨版本恢复。

`P011DerivedCheckpointV2.ets` 为启用态登记新 checkpoint discriminator（`P011_DERIVED_CHECKPOINT_CROSS_VERTICAL_V1`）及 motionRevision，clone/export/restore 同版保留，拒绝旧↔新互读。旧实例、缺字段的旧档沿原 `P011_DERIVED_CHECKPOINT_V2`；新档缺字段或版本不符在写入前失败。`P011PublishedRenderState.ets`、`P011DerivedCommitV2.ets` 的 fill/封签/consumer copy 以可选运动版本 provenance 明确新 run，封签绑定该值，保留 `numericPolicy/cacheRevision/envelopeRevision/publicationRevision` 原语义及 `commit/elapsed/particles` 接口。端点采样协议保持现版，新增诊断只走独立 companion。新行为的按钮或 launch 参数接线限定隔离入口，避免默认生产无意启用。

| 文件 | 改动 |
|---|---|
| `P011ParticleHot.ets` | plan field/身份、零获准 T4/T3 余量写者、行后记录、定容 storage 注册。 |
| `P011ParticleM2Sink.ets` 与 `P011ParticleHotBudget.ets` | 合法零原因来源、原票据整包 reserve/refund/close、行提交后落尾候选、票据 replay 身份。 |
| `P011ParticleM2Field.ets`、`P011ParticleM2Projection.ets`、`P011ParticleHotEvidence.ets` | 同版本计划入口、链头投影与 proof key、候选/权威尾项转交、事件和独立收据。 |
| 新 `physics/derived/P011DryCrossVerticalV1.ets` | 私有尾缓冲、final bed 独立只读复算、canonical 尾写入口、按外层物理子步的提交/丢弃。 |
| `WaterSandField.ets` 与既有 derived owner/backup/publication | 外层尾钩子、真实 writer 约束、失败冻结和回滚完整性、成功 token 后暴露伴随证据。 |

## 2. 计划、行权限和状态转换

`P011ParticleHotPlanBuffer` 启用时定容 F64 `blockedYFree`。在 `collectParticlePlanIntoBody` 生成 T4 时记录目标床处理之前的 `yFree`，继续使用现有 `predictedVy`。源冻结床命中仍优先 T3。所有非 T4，包括 dry batch、lift、seed、degrade、测试或 preview 合成请求都覆盖成规范 `+0`。T4 检 finite 和 `yFree<=C`，允许负中间候选，最终池必须处于合法域。其余输入沿原错误顺序。

逐条扩展 `storageValidHot`/binding/byte range/alias 及已注册 derived/TxD1 raw 源，`planValuesValidHot`、`compareParticlePlansHot`、`sameRequestHot`、`requestIdentityHot`、`copyRequestRowHot` 与所有 bulk plan copy。覆盖 A/B、seenRequests、patchOriginal/patchRequest、budget ticket 内的 requests 以及其它构造者。新路径比较用 F64 位型（含±0），保旧路径的原先比较口径。计数/头部另校验 motionRevision，手工重放不能以同 ordinal 覆盖别的候选。原 `predictedY` 保持目标列成功路径。

在 M2 sink 容量预检和预算后产生内部 `crossRejectReason`，仅合法 `CAPACITY` 或 `BUDGET` 能开启新写者。CAPACITY 无票据，BUDGET 沿原 zero/partial reserve 与同票 refund，然后 `close(0)`。`editPoolHot` 可在原 stage/publish 工作区内提前做候选位置写入，以便原行审计看真实候选。最终确立原因/登记尾项须在票据 close、row audit、事件与 `finishPatchHot` 都成功之后。中间任一步失败仍从原 Hot/外层边界回滚。新写者受相邻目标、grant=0/disposition6、源 slot/ID/phase/source/mass、合法 F64 候选和 refund 状态共同约束。未知零原因、非法引用或票据失败不得被吸收为接触等待。

使用 T377 运算序 `(bh[c]+bl[c])+mh[c]+ml[c]` 的 `Srow`，核 [0,C]。零 grant T4 的 x 夹源侧且 floor 仍为 c，vx=+0，未触床 y=yFree/vy=vyFree/state1，`min(y0,yFree)<=Srow` 则 state2、`y=min(C,Srow+contactEps)`、vy=+0。保留粒子 ID、相、来源、质量、槽和两列缓存。成功 T4 整包/原目标床修正及正常同列 motion 不变。新版本的 T3 正余量在行内同样做顶盖锚定，留原 vx，纳入尾列表；全额 T3 按原顺序退役且无尾项。新触床的 T4 只记约束结果，下一成功子步正常 collect 以 state2 发原列 T3 (`target=-1`)。该 T3 进入新分母、新票据、local/total 与 guard 流程，无 bottom 扣款。保留 partial/rejected remainder 与 exact70 审计，避免旧 T4 退款影响下一子步 LAND。

## 3. 尾所有权、writer 闭包和 preview

`WaterSandField.applyMotionChannels` 的现行顺序是 `particleBinding.apply/finish → 链头 projection/verifyPreviewCommitConsistency → advection → settle → percolate → creep/deep → relax/correction → liftByShake → updateSandTop → audit(true)`，外层再 `advectionEndOfSubstep` 和 `recordEndpointCommit`。插点固定为 **最后 `liftByShake`、`updateSandTop` 之后、`APPLY_AUDIT_BEFORE` 与 `audit(true)` 之前**。`advectionEndOfSubstep` 在该支持域须逐版本核只收尾账，不写池或床。外层发布准备前再确认尾项已结算且无后续写者越权。禁止在 render copy、`prepareInactive` 或 `M2 finish` 时无父事务地改池。

链头到尾部的已知写者中，advection/settle/percolate/creep/deep/relax/correction 的床变化允许进入 `Sfinal`。dry `liftByShake` 是真实池 writer，走 `beginDryBatch(..., sourceKind=1)`，会复用 `planA/B/results/patchAfter` 和 Hot attempt。dry 无空槽直接返回5，普通 dry birth/lift 取空槽，本支不调用湿场的 merge。于是存活的尾目标应保持 slot/ID 和行后位型，但不能从 `patchAfter` 或最新 `work.attemptToken` 重新寻找父行。权威头部 `apply` 成功后、后续场通道前，将尾项及父身份复制到 **WaterSandField 外层当前子步独占缓冲**。校验 dry lift 写的是空槽且有独立事务身份，后段任何合法移除/替换目标的组合超出首版支持面。发现身份/源列/质量/行后位型失配时以实际后续 writer 和失败 cut 记录，回滚整个子步。这样保留正常 dry lift，无法证明的新 writer 不获启用资格。

`allocatePreviewPlans` 提供链头预演和后续通道计划，当前链头 `P011ParticleM2Projection`/`DerivedProjectionV2` 早于最终床。保留既有对拍，并增加 blockedYFree、原因、实际 reserve/refund/grant 与行后位型。projection 的候选 key 含 owner/run、外层 substep/物理 attempt、accepted dt/角/输入修订、同一 before bank/预算初态及候选序号。预览与权威的 Hot attempt 数值各自合法，不要求等值。不能只靠 `theta` 复用同角旧候选或让失败候选覆盖匹配。preview 的尾列表隔离在本次 Hot 缓冲，回到 `restoreBackup` 时失效，最多留现有8个定容头投影及各自身份。preview 不消费公开尾记录。

由于现有 shadow 的计划非经核实的 final bed，采用 T377 指定的**预发布独立只读复算**。在固定 accepted 链头行后状态和最终实际床的条件下，先检查所有 tail entries，再由独立纯算式生成期望状态。取已 flush 的 `sandTopScratch[c]` 为 Sfinal，并逐触及列核它与 T377 加法序的位型。若行已 state2，或 `min(y0,yFree)<=Sfinal`，按最终床重新锚定 state2、vy=+0，T4 vx=+0，T3沿原 vx；否则保留 T4 行后积分。命中后床回落仍随床锚定。仅尾命中记 `CONTACT_AT_TAIL`，不伪造连续碰撞时间或额外 LAND。独立计算先生成 typed expected rows，实际 writer 再逐位对拍 state/x/y/vx/vy、列及质量/来源不变。后续字段审计和 public exact70 仍必须通过。final bed 读数配合已有严格审计，不能作为新传感值或用渲染投影值代替。

尾写只由 outer owner permit 持有当前 run/epoch/物理 substep，使用现有 V2 `begin(canonical,slot,-1,'CROSS_VERTICAL_TAIL') → write → finish` 和 TxD1 before/after 钩子。复用相同 membership 的派生规则，位置改变仍推进必要的内容 serial 并让封签在正确身份重新认证。禁止直接给权威数组写 y 后冒用旧证书。无额外整池读或重复全量校验，原 K16/末审计与公共边界逐桶精确校验照旧。

## 4. 定容、证据和事务失败

每个列表 T=min(N,requestCapacity)，唯一活动 slot 且固定顺序，按入列前的 slot generation/ID 保证最多 T 项，异常重复/溢出在写尾之前拒绝。每项 `Uint32[8]`（slot、ID、sourceId、源列、目标列+1、ordinal、packed phase/source/state/reason/contact、行事件索引）和 `Float64[20]`（mass；before x/y/vx/vy；yFree/vyFree/predictedY；行后 x/y/vx/vy；Srow/Sfinal；ticket token/reserved/refunded；grant/remainder/requested），共192B/项。父 owner/run/stateEpoch/substep/attempt/role 与绑定代按缓冲头固定，安全整数票据保精度。三块定容列表（Hot 当前、外层 staged、上一成功 companion），期望缓冲 4个F64+1个U8/项和 `Uint32[N]` slot seen，T=N=512 时 payload `3*192*512+33*512+4*512=313856B`，另计头、注册元数据及冷拷贝。预览不保留8份尾列表。尾检查/写入 O(T_used)，复用 slot 直接索引，无 N×R 扫池。以代清逻辑失效，避免帧内清整个请求容量。

plan 新增数组按 `8*Σ各实例requestCapacity` 计。现行 A/B/seen/budget.requests 各2048，加 patch 两个单行实例，基数65,552B，若额外构造备份或 transport 需按实加账。已有8个 projection 候选+scratch 的追加字段按 `9*8*requestCapacity*k` 计，k为新增每行F64词数，至少包含 blockedYFree、原因、票据结论，投影内复用已有 grant、预测与池行后字段。实际 k 和总字节在实现交付注明。不得按 T 低估 requestCapacity 的计划/投影成本。无需创建新的全池快照来做每行尾处理。

失败传播统一由外层成功子步事务控制。`saveBackup`/`restoreBackup` 已覆盖床、pool raw、derived field mark 和 `endpointParticleEvents` 长度。**它们的现行代码不足以自行证明 M2 finish 后预算/票据和 prepared publication 的完整恢复**。T381 补冷预分配的外层 before，按原子步位置在初始化预算后、任何 apply/preview 写之前固定实际局部/总/底预算与相关统计、票据 adapter/sink 状态、事件 cursor/lineage、tail generation 和提交时钟；为 preview 自身恢复保留正确的历史阶段，分开不同 restore 用途。可复用已有 budget 保存恢复接口，验证 `finishStepHot` 后也可恢复。保必要快照或逆序日记，禁止以浮点减后加代替字节恢复。补正式的 abortPrepared/invalidate 入口确保 inactive bank seal 或未发布 companion 不会后来变 ACTIVE；不触 active bank 及已成功前缀。若 derived fault 已触发，冻结原始 first-fault 后只执行一次父级恢复，避免 `recordEndpointCommit` 与 facade abort 双回滚污染证据。

定义原错误域中的明确 cut（`CROSS_VERTICAL_TAIL_IDENTITY`→`CERT_STALE`，非法数据→`INPUT_INVALID`，原容量/审计失败沿原码）。任何后失败都保最早 cut、originalHotCode/FieldCode、父 attempt 和 rollback 结果。对已有失败路径优先级逐位比较，新增尾错误只在已到该切点时可观察。`P011_CPU_DRY_CROSS_VERTICAL_RECEIPT_V1` 定容 companion 在 public commit 成功后才能从外层 staged 切可读，绑原 event ordinal/commit。记录 T4 实际尝试目标与提交源列、接触阶段、票据和实际 F64 位型；新 T3 仍产生原 LAND 类事件。旧 T367 72词格式保留，可新增带版本的 sidecar或新宽度及 fields，不重用旧 decoder 解释新行。失败档独立标 failed/rolledBack，不伪成功。扩展 hot 诊断的 event/payload 数量按实际预算登记。

## 5. T381 非机回归与交付

- Z01 读 T369 的两档权威位型，按当前 libm 复算，y=31.399036625257057、vy=-44.27410539280644、dt=1/60、30°，目标床为0时同一子步新值对应约30.6587773551/-44.4155562088。旧行为对照保留原位型。40步到床仅是 T377 固定夹具估算。
- Z02–Z03 区分源 local/目标 local/total/比例/部分 reserve/接收容量和完全获准。核双端 local、total 一次、退款关闭、源列/目标列缓存、左右两向、目标高床不抬升、原事件/回执。无钱/无空间仍有原列约束。
- Z04–Z05 覆盖冻结床先触、行前已有 T3 抬床、尾通道抬床、命中后床回落、顶 eps、向上起点已触及、反向脱离、T3 零/部分/全额及唯一退役。dry lift 在链头后仍可执行，并证明只写空槽，另注入身份重用反例，要求整子步失败。记录尾状态只在下一子步生成 T3，质量/守恒、phase/source/原列床及同 token 验证。
- Z06 预览候选拒绝/多次尝试、同角不同身份、两个 ID 竞争、原序退款、请求重复与篡改 `blockedYFree`、detach/alias、字段的±0、resume/reset及旧↔新 restore。未启用、wet、外源和旧 V2 签包基线按原状态/事件口径逐位比较，GPU另线。
- Z07 reserve 后、refund 后、行写后、事件满、M2 finish 后、尾写后、publication prepare 失败逐点注错。核实际 pool/field/derived/票据/事件/elapsed/commit 和 LastGood 内容位型，成功前缀保留，失败后原 token 不可复用。失败诊断首错和回滚结果分列。运行现有 CPU hot/M2/T216/preview 相关套件、ArkTS/native 编译和签包。

开发交付含源码 diff、实际 typed bytes、writer 闭包证明、位型和签包回执。独立 tester 后续同包沿 T367 cohort 观测 UP→零获准→原列下降/触床→T3 全额或明确受限余量，设备不作为开发关闭前置。前后景只走 XComponent+shader，显示失败用 FAULT 或完整旧 token STALE。T378 当前工程合同完成，可交 T381 按 P3 排期实施。
