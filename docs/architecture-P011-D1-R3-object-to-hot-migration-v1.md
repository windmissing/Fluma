# P-011 D1-R3 对象式计划与事务迁移设计 v1

日期：2026-10-25  
任务：6adccf16e4b03273ad60b81f（P5；回应 developer 暂停反馈）  
状态：**DESIGN_ONLY / PENDING_MANAGER_ACCEPTANCE / DEVELOPER_IMPLEMENTATION_INPUT / NOT_IMPLEMENTED / NOT_ZERO_ALLOCATION_VERIFIED**  
真机：未使用，本设计不含真机验证。

## 0. 裁定摘要

- **生产 HOT 根必须改走热接口契约 v1 §3/§5/§6**：`beginStepHot → checkpointHot → collectParticlePlanInto(planA) → collectParticlePlanInto(planB) → compareParticlePlansHot → applyParticlePlanInto → finishStepHot`；每个 executor 独占 caller-owned 定容 workspace。旧对象 API 保留为明确不可从 HOT 根到达的 `COLD_DIAGNOSTIC_COMPAT`，不能把 HOT 外壳继续委托给旧 `collectParticlePlan/applyParticlePlan/prepare`。
- **计划生成等价迁移**：固定 `order[N]` 原地 heapsort；请求、运动、结果、事件、lineage、guard、receipt 全部写 SoA 定容缓冲；ordinal 按旧语义排序后稳定赋值；数值 tuple 身份 + `seenEpoch[Q+1]` 去重；双 collect 同一冻结输入写不同 plan A/B，逐字段及 lineage 顺序比较，分母只由正式收集阶段累计一次。
- **事务语义不改**：按契约 §6 顺序预检→reserve→guard/精确退款→patch prepare→field stage→pool/field publish→审计→事件复制→close票据→finish patch。patch 使用已预分配的 `transactionBefore/transactionAfter/transactionScratch` 存储，新增数值 token/state，不在每笔 new pool/patch。
- **R9 接法**：粒子 HOT sink 走 `P011ParticleRefundAdapter` 的定容 numeric sidecar/fixed out，每端按实际扣额退、total 退一次、bottom=0；旧 `refundLocal` 和 `materialGuardRefund` 方法体保持不变，粒子路径不经过旧平分退款语义。
- **探针分母裁定**：当前 probe 静态命中 Field 31 + Prototype 29 = 60；本稿逐项编号 `F01–F31/P01–P29`。**当前分类全部为“仍残留/待迁移”**，因为当前运行代码还未实施迁移。未来每项只有在可达图证据证明冷 API 从 HOT 根不可达，或原生产表达式被热缓冲替代后，才可改标“已迁冷/已替代”；不得删除/忽略 registry 项。旧扫描脚本按方法名粗略认定 HOT，单靠它无法证明调用图隔离，developer 要保留 60 个 ID 并增加 call-graph reachability 字段；原脚本仍显示60不能被包装成零分配失败/通过的唯一结论。

## 1. 唯一输入与适用范围

1. [热接口契约 v1](architecture-P011-M1-hot-buffer-contract-v1.md)：§2 容量与 SoA 行、§3 固定接口、§4 数值令牌、§5 双 collect/数值 request 身份、§6 发布与回滚、§7 冷导出、§8 全调用图分配审计、§9 五组 HOT 验收、§10 生产替代映射。
2. [路线 C 架构 rev3](architecture-P011-dual-field-particle-v1.md)：§13.2 sink/guard/字段发布与 R9、§13.3 collect/apply 分母同源和双跑可核验、§13.6 五组验收；§17 与上述热接口附件协调。
3. [R9 双端退款裁决](decision-P011-dual-end-refund-adapter-v1.md)：逐端实扣退款、total 一次、bottom=0；旧退款方法与旧通道语义不动。
4. 实现读取定位：`P011ParticleField.ets` 的对象计划/patch，`P011ParticlePrototype.ets` 的测试 sink/guard/事务执行，`P011ParticleRefundAdapter.ets` 的 R9 receipt/sidecar；`WaterSandField.ets` 真实场接线仍属 D2，不在本卡移交。
5. 60-site 基线：manager 登记 `probe_p011_particle_allocations.cjs` exit=2、`remainingSiteCount=60`；Field 31 / Prototype 29；object-literal 22 / allocating-helper 20 / array-literal 12 / constructor 4 / closure 2。下表用 probe 规则对源码做静态枚举，不重新执行该测试探针。

本设计仅给 D1-R3 可实施迁移路径。不改 300/s、128列、守恒/库存/容量/有限值、26/20、tan30、estimand、§3 API、R9语义、§9/§13.6验收组；不做 D2 场集成、不授权实现、不宣称零分配完成。

## 2. 对象计划到定容数值计划的逐段映射

| 旧段/现状 | 新生产 HOT 形态 | 必需定容字段/不变量 | 等价性与可验证判据 |
|---|---|---|---|
| `beginStep(identity, params, view)`：identity JSON key、克隆 params、冻结 view（F01–02；Prototype view P02/P13–17） | `beginStepHot(work, input)` 校验caller预创建 input 与 FieldBuffer，复制有限 scalar 到 workspace 的 step identity / params scalar；`frozenField` 在 workspace 固定 arrays 内复制。旧 `beginStep/view` 只留冷 wrapper | `runToken,substepId,attemptToken,stepEpoch,dt,effectiveTiltRad,seed,paramsRevision,inputRevision`；冻结字段列与源epoch；owner/capacity；禁止new FrozenView | 同输入下 identity/params/view逐scalar一致；调用后改写caller输入不影响本step；ArrayBuffer引用/长度不变；不借用可变对象跨step |
| 活粒枚举 `order=[]; push(i); sort(callback)`（F21–22） | 遍历冻结池，把活槽号写进预分配 `Int32Array order[N]` 的有效前缀；显式原地迭代 heapsort，不闭包；比较 `pId`，pId相同时以slot固定破平局，并仍校验pId唯一性语义 | `orderCount`、固定长度N、排序工作标量；compare按pid/slot；无临时scratch array | 活粒相同则顺序、候选集合和旧代码排序相同；重复pid/非法状态给同错误码；边界0/1/N及全活池固定容量 |
| deposits/crossings/motions 三数组及请求/motion对象（F23–30） | 按契约 `RequestBuffer(Q)`/`MotionBuffer(N)` 写有效前缀；kind数字枚举；contact/target与预测位置速度逐列写入；无目标用 `targetColumn=-1` | Request：kind/phase/sourceKind/column/targetColumn/particleId/sourceId/substepId/ordinal/requested/weight/predictedXYV；mergeStart/count；Motion：particleId/column/x/y/vx/vy；`requestCount/motionCount` | 同场景逐行比旧输出字段；数组顺序不变；Q/M满返回 `PLAN_OVERFLOW/MOTION_OVERFLOW` 数值码，不能部分提交或扩容 |
| request tuple 经 `JSON.stringify`；`mergedIds=[]`（F27–30、F26） | tuple `(runToken,substepId,kind,particleId或sourceId,column,targetColumn,ordinal)` 分列数值存储；merge写 lineage arena 区间，非merge `mergeCount=0`；requestKey只在 §7 冷导出恢复 | run token安全整数；ordinal 1..Q；source/particle id；column/target；固定 `seenEpoch[Q+1]` 与身份列，epoch不wrap | 双跑比同ordinal全字段/lineage；重复ordinal=`DUPLICATE_REQUEST`；同ordinal不同身份或内容=`IDENTITY_MISMATCH`；±0按契约归一、NaN拒绝 |
| `record(request)` push、`finish()` slice/plan object/JSON hash（F17–20） | 热collector直接写caller PlanBuffer；finish primitive status/count；`compareParticlePlansHot(A,B)`逐值比较有效前缀，不生产hash字符串。旧 collector接口cold-only | A/B为不同持久PlanBuffer，各自owner/epoch/count/totalRequested/valid；写前容量预检；request identity列 | 对相同冻结输入 planA/planB count、所有字段、lineage、motion逐值相同；任一字段注入差异返回 mismatch；总请求和收集分母只增一次 |
| `applyParticlePlan` stringify身份/hash、requests数组、结果数组、motions apply（F03–07、F31） | `applyParticlePlanInto` 接planA/结果固定out；检查caller保存的expected epoch与A身份；循环按序号 apply 固定行；motions按旧确定性时序应用；只返回status码 | 预分配ResultBuffer(Q)，每行status/grant/remainder/clip/guard/event区间；`resultCount`；没有返回对象/数组 | 同输入逐请求结果与旧路径grant/各拒绝分桶、运动、事件一致；失败status在首个失败行固定记录，随后整步rollback；不在HOT new Error |
| `checkpoint()` 对象/`snapshot()`/`Array.from(Set)` 与 rollback new Set（F08–10；P03–04） | `checkpointHot/rollbackStepHot` 对workspace既有SoA、field、budget、source、consumed ordinal、journal尾和scalar计数做固定区复制；token/stepEpoch控制counter不回滚 | 固定checkpoint各SoA；applied request状态改 seenEpoch；分配固定错误attempt槽 | 整步失败后canonical/field/budget/源账/journal/out状态逐值回原；token/epoch仍消耗且旧句柄拒绝；rollback不new Set/snapshot |
| `prepare` new patch对象，scratch编辑/副作用临时物（F11；F12–16；P18–19） | `preparePatchHot` 写现有 `transactionBefore/transactionAfter/transactionScratch` 数据区；patch只由workspace scalar `activePatchToken/activePatchRequestOrdinal/patchState` 表示；token按§4单调发放 | 复用已有before/after/scratch粒子列；固定action/merge/birth arena；activePatchCapacity=1；错误stage数值码 | 逐粒质量/位置/速度/state/id/freeSlots/列质量缓存与旧patch完全一致；prepared期间canonical不变；publish/restore/discard生命周期与token反例满足§4 |
| `mergeRecords/birthRecords/representationRecords` 对象行（F12–16） | 固定action arena + merge/birth列 + lineage arena；用start/count/index表达每个patch动作，不返回record对象数组 | E/lineage capacity先预留；actionKind数字enum；质量/ID/phase/source/坐标等定列 | 同动作顺序、幸存/吸收ID、位置一阶矩、出生次序、相位与source；arena溢出在canonical/field/预算变化前拒绝 |
| `appliedKeys Set<string>`/`requestKey` | numeric tuple + `seenEpoch[Q+1]` 固定表；rollback只还原本step消费位；新step/attempt lease重新绑定 | Q+1长度、安全epoch、身份各分列；无string map/set | 同一次不可重复apply；新attempt合法重试但旧token不可复用；用户冷快照仍可导出原requestKey |

### 2.1 双 collect 与分母唯一来源

每个子步先冻结一次场与粒子池。collect A 和 B 必须各写自己的 `PlanBuffer`，用同一冻结输入、同一数值 identity 和参数revision；两者都不得 reserve、扣预算、调用真实sink或改变canonical、ID/freeSlots、seed/shake状态。由 `compareParticlePlansHot(A,B)` 完整对账后，仅正式apply A。请求分母 `totalRequested` 只由固定排序后第一次语义收集写入；第二次collect生成独立副本，不得再把分母累加到共享统计。

移动计划身份需保留既定物理语义：同一 `runToken/substepId/kind/particle-or-source/column/target/ordinal` 表示同一语义请求；`attemptToken/stepEpoch` 表示执行租约，重试可复用语义tuple但必须用新lease。ordinal先完成“deposit→crossing、各自粒子ID升序、再其它已登记事务顺序”后再编号，不能按新缓冲自然写入顺序改换预算与事件次序。

## 3. 事务语义分步迁移（契约 §3–§6；路线C §13.2–§13.3）

| 序号 | 固定事务阶段 | owner/现状替换 | 失败行为与缓冲要求 |
|---|---|---|---|
| 0 | HOT入口校验与checkpoint | M1 workspace验证config、owner、expectedStepEpoch、Plan/Result/Field/Budget/Event buffer引用/长度/alias；保存固定step checkpoint。Prototype sink拥有测试field/budget数组；pool只拥有粒子SoA | 校验失败零副作用；返回status，失败attempt写固定scalar槽。禁止逐step snapshot对象 |
| 1 | 容量预检与reserve | CommitSink持有预算写权限；`reserveInto`写预分配ticket sidecar及receipt固定out，T2/T3/T4扣量不变 | 票据容量满在扣款前报错；保存每端实际reserved、total一次、bottom固定0；apply与collect共用same denominator来源 |
| 2 | guard、容量/库存裁剪、R9退款 | guard专用HOT适配拥有数值edge列和GuardBuffer；由现闭式规则同步生成同样受影响边，不改旧guard函数。退款经R9 adapter按sidecar逐端退 | 非有限/越界先拒绝；部分guard拒绝退local两端各实际扣额及total一次；不调用旧平分refundLocal/materialGuardRefund；退款失败前后ledger必须原子不变 |
| 3 | pool prepare | M1 workspace独占粒子池写权限；用transactionBefore/After/Scratch固定存储实现`preparePatchHot`，field未写，返回number token | 容量/状态失败返回数值码与stage；不new粒子池/patch对象；需多phase时全部预留后才发token；失败丢弃scratch有效计数，不动canonical |
| 4 | field stage | Prototype测试sink或D2真实sink独占床/悬沙/水面/预算/guard/event写权限；固定列before-image和stage列写入床/悬沙/派生S等 | 按§13.2：deposit只增mh/ml，lift按splitTopV2同序扣，T5只增对应susp，crossing不改S；现有cap/库存/guard不变。stage失败恢复field/预算临时列 |
| 5 | 原子发布与审计 | 同步栈内 `publishPatchHot` 与field写入；无observer可见半状态；固定审计读字段、粒子池、质量桶/容量/有限值与原子链接 | publish前事件不可见；发布后任何审计失败先恢复已发布pool/field/budget并回滚step全部状态。不会只refund预算而留床/粒子改动 |
| 6 | event复制、close、finish | 由M1/commit sink把成功事务事件写EventBuffer有效尾；adapter close核对净reserved==committed；再`finishPatchHot`/`finishStepHot`结束lease | Event/lineage满在发布前预留容量并拒绝整步；票据close失败触发全rollback。无新事件对象、无外部回调、失败原因numeric code；commit后才进入冷导出页 |

### 3.1 `preparePatchHot` 如何复用现有存储

现有 `P011ParticleField` 已有 `transactionBefore`、`transactionAfter`、`transactionScratch` 和 `warmUpTransactions()` 路径。热实现应在冷 `configureHot/warmUpHot` 阶段一次创建或绑定这三份固定存储：每笔先检查它们各自底层buffer不alias，复制canonical→before及before→scratch，在scratch上依旧算法更新，再复制scratch→after；active patch只记录数值token、epoch、requestOrdinal和状态。`publishPatchHot`把after复制到同一个canonical池字段；`restorePatchHot`把before复制回来；`discard/finish`只关闭状态/令牌。**不得为before/after另建第二个完整粒子池实例，也不得每request返回 `P011ParticlePreparedPatch`。**旧 `prepare/publish/restore/discard`对象API留cold wrapper，不能由热实现内部调用。

merge/birth/representation动作当前靠三个对象record数组暴露给sink；改为预分配action/lineage arena，`patchActionCount`与每行index映射对应类型化字段。sink只借用当前patch lease内的数值区域，finish/rollback后视图失效，不保存借用视图。池内物理写入与field写入保持一个同步事务顺序，不改质量/分相/合并一阶矩算法。

### 3.2 R9 fixed out 接口边界

- HOT sidecar在workspace `BudgetBuffer(Q,C)` 或适配器内固定capacity中保存 `ticketToken, requestOrdinal, source, target, requested, sourceReserved, targetReserved, refunded, closed` 等数字列；每条票据的退款receipt使用契约固定二端out，不是每次新建 receipt/object。
- `reserveInto/refundInto/closeInto`按热接口附件 §3.1 的签名和原R9逐端实扣语义实现；实际双端退款分别核对localUsed/localRemaining，total仅一次，bottom始终0。拒绝必须写out前置校验后零副作用。
- 现有 `P011ParticleRefundAdapter` 的对象接口/字符串key/API兼容物料可保留在cold路径；新HOT入口可共用同一个ledger逻辑但必须通过预分配numeric sidecar/fixed out，不可从生产HOT调 `registerTicket(ticketObject)`、string keys或旧 `refundLocal/materialGuardRefund`。旧方法体不得改。
- `close`验证`reserved-refunded==committed`（沿用既有1e-9审计）；跨步、重复、超额退款拒绝；整step rollback恢复budget与R9 sidecar快照，不复用已消耗ticket/lease token。

## 4. 60 处探针站点逐项归类

**状态说明：**下表“当前分类”均为**仍残留（未迁移）**，共60/60；列出的后续目标为设计状态，不是实现状态。`已迁冷`表示旧节点仍存在但由静态调用图证明HOT不可达；`已替代`表示新HOT表达已替换生产调用而旧节点只在cold compatibility保留。正式复核必须同时报原始registry总数60、已迁冷数、已替代数、仍HOT可达数；registry任何条目不能删掉或从分母漏计。

### 4.1 `P011ParticleField.ets`（31）

| ID | 行 | probe命中形态 | 当前分类 | 迁移归属 / 目标判据 |
|---|---:|---|---|---|
| F01 | 383 | `JSON.stringify(identity)` | 仍残留 | `beginStepHot` scalar tuple；旧字符串identity留cold。 |
| F02 | 385 | params对象字面量克隆 | 仍残留 | config冷校验一次；step只拷固定scalar列；旧clone限cold。 |
| F03 | 401 | `JSON.stringify(plan.identity)` | 仍残留 | numeric identity逐scalar compare；无JSON进入HOT图。 |
| F04 | 402 | `JSON.stringify(plan.requests)` | 仍残留 | A/B RequestBuffer有效前缀逐列比较。 |
| F05 | 403 | `JSON.stringify(plan.motions)` | 仍残留 | MotionBuffer有效前缀逐列比较。 |
| F06 | 412 | `[]` amounts缓冲 | 仍残留 | Result行固定scalar验证；不建numbers临时数组。 |
| F07 | 416 | amounts数值数组字面量 | 仍残留 | 固定字段逐项inline finite/nonnegative检查；不降字段检查覆盖。 |
| F08 | 441 | checkpoint对象字面量 | 仍残留 | checkpointHot写预分配workspace checkpoint区；对象快照只cold。 |
| F09 | 441 | `this.snapshot()` | 仍残留 | SoA原位复制到checkpoint；不得每步创建snapshot。 |
| F10 | 445 | `new Set(checkpoint.appliedKeys)` | 仍残留 | seenEpoch/consumedOrdinal定容区；冷兼容旧Set可留。 |
| F11 | 481 | `new P011ParticlePreparedPatch(...)` | 仍残留 | number patch token + patchState scalar；不得构造对象句柄。 |
| F12 | 540 | merge action对象 | 仍残留 | 固定 actionKind/index列；已预留action arena。 |
| F13 | 541 | merge record对象 | 仍残留 | Merge SoA定容行 + lineage区；字段逐值保持。 |
| F14 | 590 | birth/seed对象字面量 | 仍残留 | Birth/action固定列；slot、相位、质量、位置、速度均入列。 |
| F15 | 596 | birth action对象 | 仍残留 | 数值action行指向birth arena。 |
| F16 | 597 | birth record对象 | 仍残留 | Birth SoA行，不分配返回对象。 |
| F17 | 740 | `mergedIds.slice()` | 仍残留 | lineage arena按start/count写；仅cold export物化number[]。 |
| F18 | 747 | `requests.slice(0,count)` | 仍残留 | PlanBuffer有效前缀count，不复制数组。 |
| F19 | 748 | ParticlePlan对象字面量 | 仍残留 | caller-owned plan A/B写入固定列；finish仅返回status。 |
| F20 | 750 | `JSON.stringify(active)` request hash | 仍残留 | tuple/seenEpoch + 双跑逐字段比较；hash不能代替compare。 |
| F21 | 765 | `order=[]` | 仍残留 | 预分配Int32 `order[N]`；只更新orderCount。 |
| F22 | 767 | `sort`比较闭包 | 仍残留 | 手写原地heapsort；pId再slot固定tie-break，无callback。 |
| F23 | 770 | deposits数组字面量 | 仍残留 | RequestBuffer按kind分区/计数；ordinal规则不变。 |
| F24 | 771 | crossings数组字面量 | 仍残留 | RequestBuffer定容行；目标列/迁移方向入列。 |
| F25 | 772 | motions数组字面量 | 仍残留 | MotionBuffer(N)；每活粒至多一行。 |
| F26 | 792 | motion对象字面量 | 仍残留 | MotionBuffer对应粒子列逐字段写入。 |
| F27 | 797 | request对象字面量 | 仍残留 | RequestBuffer typed columns；kind/phase/source/位置/速度语义保持。 |
| F28 | 798 | `JSON.stringify(tuple)` | 仍残留 | request tuple分列数值；字符串只在冷导出用manifest恢复。 |
| F29 | 798 | tuple数组字面量 | 仍残留 | 数值tuple各分量直接写身份列，不建临时tuple数组。 |
| F30 | 809 | 非merge `mergedIds=[]` | 仍残留 | `mergeCount=0` 且无lineage写入。 |
| F31 | 817 | `JSON.stringify(motions)` | 仍残留 | MotionBuffer双跑compare；无HOT hash序列化。 |

### 4.2 `P011ParticlePrototype.ets`（29）

| ID | 行 | probe命中形态 | 当前分类 | 迁移归属 / 目标判据 |
|---|---:|---|---|---|
| P01 | 58 | budget ticket对象字面量 | 仍残留 | Numeric BudgetBuffer receipt + ticketToken；legacy ticket只cold。 |
| P02 | 164 | `new P011ParticlePrototypeView` | 仍残留 | 预分配FieldBuffer由sink绑定；无逐step view对象。 |
| P03 | 178 | `this.snapshot()` | 仍残留 | checkpointHot复制入固定sink/field/budget区。 |
| P04 | 178 | `pool.checkpoint()` | 仍残留 | pool checkpoint与step checkpoint共用预分配SoA，不建checkpoint对象。 |
| P05 | 186 | 成功执行结果对象 | 仍残留 | ResultBuffer/status scalar；仅冷wrapper导出对象。 |
| P06 | 191 | 失败结果对象 | 仍残留 | 数值status/errorStageCode + 预分配attempt槽。 |
| P07 | 191 | `results: []` | 仍残留 | ResultBuffer `resultCount=0`；冷物化才建空数组。 |
| P08 | 195 | `new Map<number,number>()` | 仍残留 | 固定GuardBuffer edge列；delta只有固定受影响边，不建Map。 |
| P09 | 197 | edge计算闭包 | 仍残留 | 专用guard HOT loop内联/固定纯函数，不捕获环境/不传callback。 |
| P10 | 201 | guard decision对象 | 仍残留 | GuardBuffer scalar/count/limitingEdge列。 |
| P11 | 241 | `this.snapshot()` before | 仍残留 | sink checkpoint写固定before区，包含field、budget、event尾。 |
| P12 | 251 | `this.pool.snapshot()` | 仍残留 | 粒子SoA复制到workspace固定pool before区。 |
| P13 | 253 | `this.view()` | 仍残留 | 使用已冻结FieldBuffer引用；不每笔create view。 |
| P14 | 254 | `this.view().freeSpaceAt(...)` | 仍残留 | 直接读冻结的water/sand/susp/particleMass列。 |
| P15 | 254 | 内层`this.view()` | 仍残留 | 与P14同站点表达，字段数组直接读，无getter对象。 |
| P16 | 256 | `this.view().freeSpaceAt(...)` | 仍残留 | 同P14；跨列target库存/capacity沿原公式读取。 |
| P17 | 256 | 内层`this.view()` | 仍残留 | 与P16同站点表达，零新view/闭包。 |
| P18 | 292 | staged request对象字面量 | 仍残留 | 固定request scratch row，阶段接触高度照旧公式写入。 |
| P19 | 299 | grant对象字面量 | 仍残留 | ResultBuffer/patch scalar grant字段，disposition enum数值化。 |
| P20 | 322 | `this.pool.snapshot()`审计后 | 仍残留 | 审计直接读canonical列与after固定区；不生成快照。 |
| P21 | 340 | merge event对象字面量 | 仍残留 | EventBuffer数值行 + lineage范围。 |
| P22 | 344 | `[survivor, absorbed]` | 仍残留 | lineage arena写两个ID，event行保存start/count。 |
| P23 | 350 | SPAWN event对象字面量 | 仍残留 | EventBuffer固定字段；冷导出映射字符串 kind。 |
| P24 | 354 | SPAWN `mergedIds: []` | 仍残留 | lineageCount=0；导出再给空数组。 |
| P25 | 359 | 主事件对象字面量 | 仍残留 | EventBuffer(kind/request/质量/预算/guard摘要)固定行。 |
| P26 | 369 | DEGRADE event对象字面量 | 仍残留 | EventBuffer固定行；保持source与质量关联语义。 |
| P27 | 372 | DEGRADE `mergedIds: []` | 仍残留 | lineageCount=0，不创建空列表。 |
| P28 | 375 | committed ApplyResult对象 | 仍残留 | ResultBuffer有效行与eventBegin/count数值列。 |
| P29 | 382 | failure ApplyResult对象/string error code | 仍残留 | numeric status/errorStageCode；冷导出映射既有errorCode字符串。 |

**基线分类汇总：**Field 31/31、Prototype 29/29 当前均“仍残留”；当前已迁冷=0、已替代=0、仍残留=60。未来 manager 验收证据可逐ID调整状态，但迁移前不得预先标为“已迁冷/已替代”。扫描器原始 60-site registry要保持全量历史身份；新增的HOT可达性列解释其是否仍阻断严格零分配。

### 4.3 不在原60内但仍必须纳入完整HOT图

- `P011ParticleRefundAdapter.ets` numeric reserve/refund/close、receipt out和票据sidecar：契约 §8明确审计；原probe的60分母未覆盖它，不能把“不在60里”写作免责。旧对象adapter可冷存，HOT numeric入口必须零显式分配。
- guard专用热适配模块、rollback失败分支、begin/finish状态机、冷导出队列封页与调用关系：按§8/§9追加完整可达图与节点清单；新增站点另列`HOT-NEW-*`，并和原60并报，不覆盖/缩小原registry。
- `WaterSandField.ets`真实场绑定属于D2；本卡只设计Prototype/Node D1 sink接口，不把真实生产接线并入R3。

## 5. 3–5个可实施切片与串并行关系

| 切片 | 范围/依赖 | 本片判据（developer可执行） | 不做的反向阻塞 | 并行关系 |
|---|---|---|---|---|
| S0 定容workspace与旧路径隔离 | 首片；只新增/绑定§2缓冲、热状态/错误码和configure/warm-up；列出所有生产根与冷wrapper边界，不改变物理调用 | 固定capacity/typed-array引用及长度在多step不变；坏owner/alias/size输入零副作用；旧33+11既有用例保持原结果；AST registry仍完整含60条并新增Reachability列 | 没有workspace所有权与固定容量，计划与事务都只能继续制造临时物件 | 可与S0的文档/探针registry格式工作并行；代码骨架被其它切片依赖 |
| S1 双collect计划生成 | 依赖S0；收口F01–07、F17–31的HOT替代、固定排序/request/motion/identity、plan A/B compare | 同冻结输入两次collect逐字段+lineage+motion相等；每个旧候选/ordinal/totalRequested与对象参考同输入逐值一致；deposit/crossing优先序与分母只一次；plan Q/M边界数值错误无副作用 | S1未完成不能有可信plan，S2不得以HOT计划apply；不能宣称collect生产路径已迁移 | S1完成前不可与S2生产apply并行；测试夹具/静态站点登记可并行准备 |
| S2 数值patch与Pool发布 | 依赖S1计划身份及S0固定patch区；收口F08–16与prepare/edit/publish/restore/finish;复用transactionBefore/After/Scratch | 逐粒状态/质量缓存/ID/free stack与旧patch逐值一致；token discard/publish/restore/rollback/ABA/busy/MAX边界逐条拒绝/通过；无new patch/pool；部分事务回滚全池一致 | 未有token patch与after image，sink无法安全stage/发布，事件和退款错误会把半状态写入canonical | 可和S3的Prototype field-buffer静态搬迁并行设计，但不能合入同一热调用前各自接口冻齐 |
| S3 Prototype sink + Guard/Budget/R9固定事务 | 依赖S1/S2；收口P01–29，fixed guard/receipt/event/result, R9 refund,事务全序、field/pool/budget/event原子回滚 | R9每端实扣逐值退款/total一次/bottom0；旧 refundLocal/materialGuardRefund回归不变；guard边/决策与旧冷参考逐值等价；六事务结果/event顺序与旧33+11/新固定Buffer同输入逐值；失败发布回滚所有field/pool/budget/event | 不完成S3，D1 Node完成体不满足契约sink/事务语义，HOT Five groups不可测 | 一部分编码可与S2并行，但需要S2 patch token接口先冻结，集成和验收必须串行 |
| S4 全图门槛与原60结项 | 依赖S1–S3；Adapter/guard/error/rollback/export及调用图静态归档，probe report扩展保全原ID | 原60逐项新状态+文件/行/Reachability证据；HOT可达显式分配=0才允许静态项PASS；动态600s/36,000子步全事务双跑及句柄、容量、错误、GC要求全部按热契约§9和主稿§13.6，不能删旧测试/放宽证据 | 没有完整静态和动态证据，D1 R3不能通过，D2不解锁 | 必须最后串行做，HOT根和调用图稳定后才有结论 |

**依赖图：**`S0 → S1 → S2 → S3 → S4`。S2与S3允许在接口冻结后分支开发，S3集成必须以S2 token patch接口为准；S4只在最终集成调用图冻结后运行。tester可并行准备等价性与边界用例，但不得改已有33+11/§9五组内容或estimand。所有实现仍等待本稿manager验收和原D1 R3授权状态处理。

## 6. 与契约条款逐项对应

| 设计点 | 权威条款 | 本稿落实 |
|---|---|---|
| capacities/SoA/order/request/motion/result/event/lineage/budget | 热契约 §2、§2.1 | §2、§4和§5，固定capacity、有效前缀、overflow整步失败 |
| 热接口名字/参数/所有权 | 热契约 §3、§3.1 | S0–S3只实现既有签名，不引入另一套API |
| token/ABA/step lease/out alias | 热契约 §4、§4.1–§4.2 | §2 checkpoint与patch、§3事务、§5 S2判据 |
| identity tuple/ordinal/seenEpoch/double collect/denominator | 热契约 §5；主稿§13.3 | §2、§2.1，逐字段Compare，正式请求分母单次累计 |
| transaction order/failure rollback/event publish | 热契约 §6；主稿§13.2 | §3步骤0–6，发布前不可见、发布后失败整步回滚 |
| cold export/compatibility/call graph reachability | 热契约 §7–§8 | §4原60注册表+新站点，兼容对象接口只冷可达 |
| five HOT groups / long run / full-call graph | 热契约 §9、主稿§13.6 | §5 S4要求完全保留，不缩scope，不声称本稿已通过 |
| production substitution and D1/D2 boundary | 热契约 §10 | §0/§3/§4.3，返回对象计划必须替换，真实WaterSandField接线仍属D2 |
| numeric ticket / R9 fixed out | 热契约 §3.1、§6；主稿§13.2 R9；refund decision §2 | §3.2，按双端实际扣额退款，旧方法体和负对照保留 |

## 7. Manager验收点与红线自检

- 四项任务齐备：计划逐段映射、完整事务顺序/owner/回滚、60/60逐项归类、五个开发切片含判据与反向依赖/串并行。
- 60处当前状态诚实登记为仍残留；没有把静态命中说成60个运行时分配/每帧次数，也没有把Cold wrapper存在误写成已冷迁。future proof要求完整HOT call graph。
- 沿用热接口契约 §3签名，不改R9，不触碰旧退款方法；patch复用既有固定状态区，不新建particle pool。
- 保留§9/§13.6验收五组和旧33+11用例；300/s、128列、库存、容量、有限值、26/20、tan30与预注册estimand不变。
- D1只交Node/isolated prototype迁移设计；真实WaterSandField集成仍是D2；不使用真机，不改`entry/`或`test/`，不宣称实现或零分配验证通过。

**完成判断：**设计可供manager逐项验收，再按S0→S4转developer实施；当前旧路径与60处仍未迁移，D1 R3仍未解阻/未授权，不据本稿关闭D1或解锁D2。
