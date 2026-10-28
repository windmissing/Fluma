# T205：全事务复杂度方案工程规格 V1

日期：2026-10-28。角色：architect。不使用真机，不实现业务，不重跑产品或模型测试。

原卡：`6ae15c23e4b0c3103c068383`，完整标题「【T205·整体复杂度方案工程化】architect：动态索引、数值切点、资源全表与迁移接口」。消费 T197 `f0e3a0c` 的完整方案及 §13、T116 V3、T204 原始已归档 ledger、T199 GPU 报告与现有 T137/T171/T191 接口。本稿是工程交付，实施及产品指标仍为 NOT_IMPLEMENTED / NOT_TESTED。

## 1. 工程裁定与版本

整体路线 `P011_WHOLE_TRANSACTION_V1` 同时覆盖 direct/queued source、λ、seed、preview、authoritative、field、观察与发布；不是单次 find 的补丁。精确结构层可以交 developer 实施；GPU 接口与它同批交付，作为隔离实验分支。新行为单独分版：

| 版本 | 合同与当前状态 |
|---|---|
| `TX_EXACT_STRUCTURE_V1` | 原同步 API、原逻辑切点/首码、F64 运算图、原 source/step 成功边界不变；动态 pair/ID/ticket、私有 view、完整计量可实施 |
| `TX_STAGE_CERT_V1` | 仅完整 read-set 和副作用等价时复用；dirty numeric audit 仍原 slot fold。逐 predicate 接入，不作为删原审核的总开关 |
| `TX_GPU_PAIR_EXPERIMENT_V1` | 冻结同输入的只读 pair backend；CPU F64 merge/账本/预算/事务仍权威。生产默认关闭；旧隔离 Worker 成功不代表生产已接入 |
| `TX_OWNER_ADMISSION_V1` | 异步 ACK、FIFO、work/资源拒绝、有限外部 lease、deadline 为 **DECISION_REQUIRED**，只给候选规格，不启生产行为 |

等价层不等待 owner 新行为裁决，也不等待 T206 写完文档；独立规格后补引用。未经新算法事务设计，不迁移全高度场/粒子推进到 GPU，不暗含 GPU 驻留增量索引或跨 mutation 批量配对。T207 的微负 cache/publication 材质域设计独立消费，T205 不加 EPS/clamp、不改变旧数值审核，也不把目标7的 clear-water ID fold 搬入旧 Hot。

所有后续实现、测试与设备卡必须写：**仅新 XComponent+shader 渲染，含前景与背景；禁止 Canvas 上屏或失败回退。故障为 FAULT 或显式 STALE 的上一有效完整新管线帧。** 调度 GPU 最高优先不豁免数值、守恒、预算、身份与设备证据。

## 2. 输入证据及缺证边界

本稿两个机器可读附件：

- [源分配点与 ledger 指纹](analysis-T205-source-ledger-inventory-v1.json)：54 个本地 import-closure 文件，473 个 `new` typed-array/container/view 静态点，另 1097 个构造/增长/复制/编码风险行；源码及输入 SHA256、原 ledger 前缀。静态点不是分配次数或 RSS，不是语言完备的分配器证明。
- [资源、切点、工作包络 manifest](analysis-T205-tx-resource-work-manifest-v1.json)：20 个切点、118 个新 array 模板、13 类继承所有权模板、每项字节式及全链 work 项。状态 DESIGN_ONLY，未声称已经分配或测得耗时。

T204 原档 `test/artifacts/t156-real-load-20261028/R{1,2,3}-own-run-store.ndjson`：R1/R2 各4条，R3共274条；admissions 文件为空。R1/R2 已提交前缀为 L=0→256→512→512、free=512→256→0→0、nextId=1→257→513→513；第4条是 **256 个 DEGRADE**，不是再新增256粒子。各条 conservation=true、audit.errorCode=null、sourceEpoch=0。最后已提交 N/L512 是满池 benchmark 锚点；失败第5子步没归档，B/M/A、瞬时状态与首个物理错误仍 NOT_CAPTURED。不能补成“失败时确定512并做了若干merge”。

T199 既存隔离 WET_192_64 证据：GPU O0/O2 p50 为1586/1598.5ms，LEGACY 为1442/1458ms，各30条 steady；没有已证的 O2 整事务收益，GPU 该 fixture 仍较慢。UI 竞争 NOT_RUN，GPU timer NOT_MEASURED。旧 owner 耗尽后新 OS 进程恢复有证，不能扩大为同 context reset 可恢复、生产 owner 已接、所有场景 PASS。签名包与原档身份沿用 T199 报告，不以当前脏树借用旧包身份。

## 3. 模块、文件与 hunk 所有权

以下均为**拟新增/修改**，本卡只写文档。模块由一个开发责任人拥有，其他并行任务不得修改其 hunk；共享文件只改列出的函数段，保留并适配在途修改。阶段内先新增核心，再顺序落共享接点，避免两个开发卡同时重写同函数。

| 责任单元 | 拟文件/接点与独占范围 |
|---|---|
| TX storage/core | `entry/src/main/ets/physics/tx/TxTypes.ets`、`TxStorageRegistry.ets`、`TxBranchView.ets`、`TxWordUndo.ets`、`TxOwnerFacade.ets`；handle、bank、address registry、savepoint、同步入口生命周期 |
| TX indexes | 同目录 `TxPairAdjAvl.ets`、`TxIdIndex.ets`、`TxTicketIndex.ets`；纯派生结构、原谓词回调和确定计数，不写物理公式 |
| TX cert/field | `TxCertificates.ets`、`TxFieldViews.ets`；依赖失效和 dirty-column。`P011ParticleHot.ets` 的 begin/collect/request/patch/merge/audit/tail/finish 按第5节接 barrier，不重写原 RN 运算 |
| TX budget/M2 | `P011ParticleHotBudget.ets` 的 ordinal/token lookup、reserve/refund/close；`P011ParticleM2Sink.ets` 的 applyInto 原切点；`P011ParticleM2Field.ets` 的 collect A/B/header/frozen binding；`P011ParticleField.ets` 的受控 mutation/export/import 与 patch banks |
| TX public roots | `WaterSandField.ets` 的 injectGestureSand、queued source、saveBackup/restore、previewChannelPlansAt、stepWithTiltTrace、finishNaturalObservation；`P011TiltPlanningPass.ets`/correction 的调用接点只做 view/dirty/trace 接入 |
| TX trace/capture | `TxWorkTrace.ets`、`TxCapture.ets`；natural/endpoint/peak/store 原触发点的具名 scope、封存/字节预算。不得修改既有 endpoint 协议或另起第二诊断消费名 |
| GPU adapter | `test/p013_compute/ets/tx/TxGpuCompatWork.ets`、`TxGpuMetadata.ets`、`TxPairBackends.ets`、`TxGpuWorker.ets`；隔离构建入口新增 source manifest。复用 wire/native 入口，**不改旧 T171/T191 全局 metadata/selector 语义** |
| 独立工具/用例 | `test/tx/` 下独立 oracle、trace checker、fixture/故障注入；由 tester 依 T206 维护，不能引用 AVL 的实现作 oracle |
| UI/owner候选 | `TxOwnerProtocol.ets`/`TxOwnerWorker.ets` 与页面消息接点留后续版本；生产 `EmptySandDemo.ets`/渲染 controller 本阶段不自动切 worker 或 GPU |

原 capture/material 数值与 renderer 由 T207 后续独占，本表的 trace/copy 接点不得顺手改其公式。所有新增文件编入独立差分 target 后，再纳入目标生产构建；没被构建消费的代码不能记“已接入”。

## 4. Opaque 接口、view 与地址模型

以下签名是开发合同，具体类按 ArkTS 规则显式声明，不用动态结构、任意对象字典或 raw pointer 逃逸。业务对象仍在内部。`TxOwnerHandle/TxViewHandle/TxSavepoint/TxReadLease/TxFrozenPair/TxCaptureLease` 为私有构造的类，内部含 ownerSlot、ownerGeneration、viewSlot、branchGeneration；所有字段外部只读，调用方不能自行拼 handle。

```text
TxOwnerFacade.create(profile: TxProfile, binding: TxLegacyBinding): TxOwnerHandle  // cold
TxOwnerFacade.executeSync(owner, command: TxCommand): TxOriginalReceipt
TxBranchView.enterRoot(owner, rootIdentity: TxRootIdentity): TxViewHandle
TxBranchView.mark(view, cut: TxCutIdentity, rollbackDomain: TxRollbackDomain): TxSavepoint
TxBranchView.fork(parent, role: TxBranchRole, frozenInput: TxReadLease): TxViewHandle
TxBranchView.commitChild(child, parent, point: TxSavepoint): void
TxBranchView.rollback(view, point: TxSavepoint): void
TxBranchView.sealRead(view, purpose: TxReadPurpose): TxReadLease
TxBranchView.releaseRead(lease): void
TxPairIndex.find(view, cut: TxCutIdentity): TxPairResult
TxIdIndex.lookupAtOriginalCut(view, id: number): TxSlotResult
TxTicketIndex.lookupOrdinal(view, ordinal: number): TxTicketRow
TxTicketIndex.lookupToken(view, originalToken: number): TxTicketRow
TxWriteBarrier.beforeKeyWrite(view, slot: number, writeSet: TxWriteSet): TxMutation
TxWriteBarrier.afterOriginalWrite(view, mutation: TxMutation, originalStatus: number): void
TxCertificates.evaluateAtOriginalCut(view, predicate: TxPredicateId, cut: TxCutIdentity): TxOriginalCheck
TxCapture.sealAtOriginalBoundary(view, identity: TxCaptureIdentity): TxCaptureLease
TxGpuPair.prepare(owner, view, cut, frozen: TxReadLease): TxGpuRequest
TxGpuPair.consume(owner, view, cut, request: TxGpuRequest): TxPairResult
```

`TxCommand` 为 closed class 集合：原 direct-source、queued-source、step、reset/configure、capture 请求逐项适配；输入 dt/θ/seed、mode、params/input rev、source sequence/gesture 时间与原 payload 位型原封复制。`TxOriginalReceipt` 逐字段映射旧成功/错误/部分成功回执，不新增同步 `PENDING`。`TxCutIdentity` 含 rootId、parentScopeId、λ/seed/row/candidateRole/substep、cutId、ordinal、算法图版本；不可用当前 frame counter 代替 direct-source 身份。

`TxPairResult` 明确区分 FOUND(aId,bId,aSlot,bSlot,slotGenerations)、NO_PAIR 与原失败，记录backend、view/contentRev。内部 UNCERTIFIED 不是新的物理错误码。GPU ERROR/STALE 是实验 route fault，不作为 NO_PAIR，不触发隐式 CPU 重试。

存储注册必须 cold 完成：每一 unique ArrayBuffer 获 addressId；F64/U32/U8 与对象标量槽注册 width/offset/长度/owner，DataView 仅为 alias。typed-array 长度与 buffer identity 在原绑定检查点验证。标量原字段保持对应同步读写，日志用 raw lo/hi 位恢复，不能用 JSON/数字重算旧值。地址 offset U32 的可表示范围须在 cold profile 检查；legacy API 不适配的极大配置留有名旧 route，不增加一次公开请求的新拒绝。

禁止向调用方返回可写 bank；需要旧 raw-array API 时保留兼容路径及原复制/校验工作，相关证书 UNCERTIFIED。历史只读 snapshot 的借用必须实际不可写且持有 lease；TS readonly 声明本身不能证明 alias 没有 writer。参数表等 cold 深复制，不能仍引用调用者对象。

### 4.1 Branch 与 savepoint

冷配置 `viewSlots=8, depthMax=8`，保存点含 logMark、view/branch、原 page/event/lineage 长度、ticket/seen、source/field 标量及首错状态；表中的标量或单独注册槽必须可完全恢复，不把8个F64槽误当全部状态副本。当前嵌套预算按 public→λ/候选→seed→row/sink→patch 的最深调用图登记，frozen input 与父 view 通过 lease 引用；兄弟 preview 顺序复用释放后的 view。开发时生成实际调用图最大深度和同时存活 view 数，外部回调重入不混进受控图。

不得用“8层够用”删除第9层原行为：若原入口可重入或活跃 scope 超此图，该入口走显式旧完整执行并计费，或 cold 配更大的表；新 runtime 拒绝只属于 owner 版本候选。profile 选择及 route 显式记录，不用容量不足伪装原物理 code。

首阶段 `WHOLE_BANK` 保留原需要的 frozen/sourceBefore/stepBefore/preview/patch banks，复用已经证明同 owner 的受控库；复制仍计费。后续 `WORD_UNDO` 的基础实现为**每次写前追加旧位型**：mark 保存日志尾；child rollback 逆序恢复到 mark 并截断；child commit 保留所有记录供 parent rollback；parent commit 后才释放整个根日志。这样同地址多写、内层 commit 后外层失败可正确还原。未证明 first-touch 去重时不用该优化。最坏容量按 T197 保守 `undoRecords≥depthMax*w_upper`，w 为所有域原字写次数，不仅 N 个粒子。

AVL 指针/高度/min/邻接/根、ID/ticket树、revision/dirty/cert.valid、free stack/nextId、原 event/page/journal/预算/source 状态都在日志域。不可撤销的**物理成功前缀**另根提交；只读性能 serial/context/request generation 是控制域，单调递增且 rollback 不回绕。内容恢复原字节也发新 content revision，防迟到 GPU/lease 贴旧号消费；业务 sourceEpoch/原 ticket 值仍按原合同恢复。generation/serial 耗尽不能 wrap；实验停止，生产旧路径的错误顺序不得被新派生错误抢占。

## 5. 原逻辑切点、首码与读写集冻结

20个切点的 ID/原函数/顺序/失败域见 manifest `cutpoints`。开发保留原检查块与 error 常量，只在其入口/出口接 view/barrier/trace；每块保留指纹或对应 diff。下面是不可跨越的次序：

| 原切点组 | 必须保持的读取/顺序 | 写集、失败与成功边界 |
|---|---|---|
| SOURCE_IDENTITY / SOURCE_CAPACITY_ETA | mode/run/input/shape/时间/sequence 原验证；原全局 scale、η、R≤8 顺序 | 每λ原库存/预算/池/source临时态；失败退该λ；成功源 receipt/page/epoch 按原位置提交 |
| BEGIN_HOT | active/backlog/buffer/field scalar/column/token/pool/budget 原顺序 | begin 不在入口先跑 catch-all audit；原 INVALID_BUFFER 等不能被新 code13 抢先 |
| COLLECT_HEADER / COLLECT_AB | 原 frozen 输入；先A后B再比较。headers-only 仍执行旧全 collect 的验证/预测器 finite/计数/overflow | 只省随后无观察且完全覆盖的数据写入；A/B 仍独立 bank，不共享可变 plan |
| REQUEST_VALIDATE / SINK_RESERVE_GUARD | 原 row 身份/seen、capacity、reserve、guard、refund/field stock 顺序 | 原 grant/denominator/F64 debit、ticket receipt 与局部恢复范围不变 |
| PATCH_PREPARE / FIND / PATCH_PUBLISH | 原 binding/grant/snapshot/row/seed/birth/find；每次 find 后立即原 mergePair，再下一次 | nextId/free/质心/phase/source/cache/event/lineage、原 publish 与失败 rollback；不预选多对 |
| AUDIT_SINK / TICKET_CLOSE_EVENTS | 原 slot fold→column fold→budget；先 audit，再 close/event/finishPatch | close 不删除历史 ticket；事件、退款与 sourceExternal 的发生次序不变 |
| MOTION_APPLY / AUDIT_TAIL / AUDIT_FINISH | 原 motion ID查找/推进；tail；finish 先 allClosed 再 audit/token/finish/seal | 原逻辑 audit 全保留，逐次 execute/certHit；motion 使数值依赖失效 |
| PREVIEW_RESTORE / AUTHORITATIVE | 原候选顺序、consistency；queued-source **先于 stepBefore backup** | preview finally 恢复；源成功独立保存，step 失败不能抹掉已成功 source；DIRECT源在frame外另根 |
| FIELD_AUDIT / SOURCE_PUBLISH / OBSERVE_SEAL | field totals/drift/guard 原顺序；原源提交点；原每substep endpoint/peak/natural触发 | 物理commit、源成功、记录/编码成功、UI CURRENT 分开登记；编码失败不能把物理成功改失败 |

错误比较必须包含 `code + stage + ordinal + root/λ/seed/row + partial success + rollback bytes`，不能只比最后 SHA。原请求有重复ID/坏buffer/非finite/alias时，先标派生 UNCERTIFIED，**到原检查点**执行旧验证或精确旧查找；例如 particleSlotHot 原先取最先 physical slot，不能由新 ID 树提前去重报错。未知/不受控 writer 不宣结构证明，走有名 `LEGACY_UNCERTIFIED` 完整执行、完整计费；这属于等价覆盖，不计新树复杂度收益。

原 source 一个请求内可能已有 λ 重试，seed 成功 page 先放根私有 capture arena，按旧业务边界封存；不把子 seed 的暂存页当整个 public root 已成功。对确有原 callback/外部副作用的路径，先逐个标实际观察点；不能把已可见旧副作用无声推迟到根末尾。这类路径未完成等价接线前保留旧执行。

## 6. 动态 pair、ID 与 ticket 结构合同

### 6.1 PairAdj AVL

每活跃 slot 一个节点，键完全是 `(phase,sourceKind,sourceId,state,floorX,Y,id,slot)`；group 不能跨越。eligible(v) 只表示同 group 的前驱/后继之一满足**原** `abs(RN64(yA-yB))≤radius`。root.minEligible 给最小 ID 的 a；随后在 a 的**完整合法 Y 区间**排除自身查最小 ID 的 b，绝不能把邻接见证直接当 b。

rank/select 二分用原差值谓词确定区间，不用 rounded `Y±radius`；size+range-min 按 T197 保守 O(log²(N+1))。±0、subnormal、溢出差、UINT32_MAX ID、边界group有独立反例。absence 用 slot=-1/has 标志，不用最大 ID 数值哨兵；树结构只作整数 min/height，不参与 F64 求和。FTZ/F32/不同舍入环境不自动继承证明。

mutation barrier：先根据旧 slot 节点摘除所有将变 key 的节点并保存旧邻接→执行原 F64 writer→由实际新值插回、重算受影响邻接与向根 augmentation。merge 后 a 的加权 x 可能跨 floor；b 退役；new birth/phase/source/state/Y 同理。不能 Work.y 已变而树仍按旧位置查询。一个原 patch 内不开放 query 或外部 callback；writer 原失败按原域恢复，并恢复/失效派生结构，不能先抛树错误覆盖原码。

mass/vx/vy 不变 key 可不 relocate，但必须更新数值/predictor revision。bulk motion 可一次重建，计 `H_bulk`；连续 seeds 必须继承父树，不能每个 beginSeed 冷重建后仍报低 H_bulk。frozen collector 独立 view，不得通过共享树读当前 Work。树受损只能从正确当前 snapshot 有界重建并计费，或报实验 fault；不偷偷换 scan 后仍记新界。

每 view 的16个I32结构数组+4个F64键/revision数组共96N B；完整模板逐名列在 manifest。find 节点访问包络 `6*hR*hA+4*hA+fixedHead`，原子 insert/erase 为 `64*hA+fixedHead`，relocate 算两次；`hA=ceil(2log2(N+1)), hR=ceil(log2(N+1))`。实现若超此宽松界需修订证明/manifest，不能改计数使其“通过”。

### 6.2 ID 与 ticket

ID树按 unsigned ID 全序，lookup 验 live/id/slotGeneration；退役同slot复用不能命中旧ID。collect 如需原ID遍历可迭代树，原 slot-order validation 与原 duplicate 首错仍保留。重复/坏输入树 UNCERTIFIED，到原 cut 执行原 first-physical lookup/检查。

ticket 的 `(ordinal→row)` 和 `(opaqueToken→row)` 两棵树，原 safe-integer token 值与 allocateParticleHotLease 调用次数不变；保留原 epoch/closed/conflict/duplicate 规则。reserve/refund/close 仍原行顺序与原 F64 图；close 只改原 closed，不删除重复身份历史。reset/rollback 恢复树与原 ticket 数组。新增10个I32Q+1个F64Q=48Q B/view，原 token/reserved/refunded/closed/requests 另算。

## 7. Read-set 证书与 field 残余成本

证书包含 owner/view/branch、原 cut/算法图、原结果/首失败位置、完整依赖域、原副作用版本、存储绑定身份；不能凭一个 mutation 号或短 hash 当状态相同。每 view 64 stage、256 conservative dependency domains：8×U32 readMask/stage，当前 domainRev 表，独立 cert.valid。**每个受控写立即按 mask 失效相关 stage**；domainRev 不是每个证书的旧 revision 快照，不能比较共享 current 表后误判历史证书有效。domain 以保守大域覆盖，不给每个slot分配一个超出256的bit；细 slot/column revision 是额外维护，不削弱大域失效。失效扫描的 stage/domain visit 计入 work。

| 证书/派生物 | 最小完整依赖 | 命中与失效 |
|---|---|---|
| pair/ID/free结构 | 13buffer绑定、group keys、slotGeneration/live/free、radiusRev、branch | 受控 barrier 维护；buffer/未知 writer/参数变化失效，bulk rebuild有名 |
| numeric audit | 原slot数组、col cache、field current/before库存与waterTop/suspension、stepExternal、budget实际读取值、原图 | 任一dirty则原 **slot0..N−1，再column0..C−1** fold；不树sum/IDfold/总量+=delta |
| frozen predictor | frozen pool/field、S/waterTop、mode/params/dt/θ/seed/input rev、图与副作用 | 任何参数/位型改变 miss；preview不同角不是同输入 |
| header/plan A/B | 原所有验证依赖、predictor、counts/finite total、buffers/length/identity | header仍做全验证；A/B校验均在原cut，省写而非省失败；复制输出不alias |
| field S/dry envelope | bh/bl/mh/ml；dry还含pool col H/L及原waterTop依赖 | 脏列用原左结合式重算；全场读前按原列序flush，不能 S+=delta |
| ticket/预算 | actual reserved/refunded/local/closed 等按每predicate读集 | close若只改该predicate不读的flag可不使其miss；不能一概认为close无数值依赖 |

外部 alias、同rev异内容或未知 callback 只能完整字节验证或禁止复用；字节比较/冻结成本实计。未证明副作用纯的 predicate 不复用。`AUDIT_LOGICAL=AUDIT_EXECUTE+AUDIT_CERT_HIT`，最坏 A_execute=A_logical，不保证每root一次 audit。原sink/tail/finish的多个logical cut不能合并。

field 优化仅做受控 dirty-column 和有证明的纯稳定排序/输入复用。必须保留：

- water `getTotals` 中的 pool/column工作、40次迭代与实际 donor×receiver 的 C² 项；不能只预算 40C。
- source η 的原有限轮次、端点与 RN 列累加，最高64C项；原λ全调用集仍在。
- 每通道的 `p_a*C²` 或每patch全S重算残余，在 dirty接线前逐项保留；`particleMassAt(c)` 是cache O(1)，不误加N扫描。
- correction `byRatio/rest` 原内扫一次后break，不凭嵌套语法误报第二个C²；其 patch 若仍全S更新，则按真实调用计 C。
- field sort 只在原比较器纯且等价时换固定scratch stable mergesort，NaN/±0 tie及原输入稳定序保留；有副作用则留原sort并计费用。
- requested/clamped/old preview、consistency、authoritative独立branch；lift collect只产rise request，不多算seed；lift apply按phase/column才seed，不额外套source λ8。

endpoint/peak/natural、源去重记录、物化与归档均在完整调用树内。不得把 `particleResidueDiagnostic` 改成第二个 `endpointDiagnosis` 消费名，不能改commit/elapsed/particles协议。T207将来变数值域须新rev后分别差分，不能由本稿默认通过。

## 8. 全数组、对象、native及生命周期预算

manifest 中 118 个新 array 模板逐项给 type/length/byte式/instanceScope/owner/lifetime，另有13个继承模板。基准 C128、N192或512，Q2048/E_hot4096/Lg_hot8192/H=N/R≤8/V≤3/S≤2。E_hot 只是单Hot容量，**不是整个public root/frame事件上限**。

### 8.1 继承资源必须累计

| 家族 | 有效payload式与同时实例规则 |
|---|---|
| pool13 | `55N+16C` B/份；原canonical、patchBefore/After/Scratch、frozenPool至少5份distinct。bindHotTransactionStorage返回的引用不再乘3；source/step/preview/export另份另计 |
| Hot plan/result/event | plan=`91Q+40H+4Lg`；result=`75Q`；event=`270E+4Lg`。planA/B/sealed、seenRequests(Q,1,2Q)、patchRequest/Original(1,1,2)、budget.requests均按各自容量；results/sealedResults、patchActions/events两份不可遗漏 |
| Hot aux/旧索引 | aux=`144C+5N+20(Q+1)+33E`；旧T123索引=`12N+8K`。替换完成前与新AVL重叠；storage引用数组及对象头另算 |
| budget/M2 | budget=`32C+25Q+plan(Q,1,2Q)`；M2 host before两列16C，projection scratch+8slots=`9*8*(5+16Q+11N)`；双refund/guard receipts按原容量另算 |
| field/backup/planner | inventory中每个constructor/lazy branch的实际typed长度；sourceBefore、stepBefore、motion/planner/preview shadow、自然观察滑动标志、edge表、dry/source暂存与入场参数深副本 |
| observation/store/source | endpoint/peak/natural/journal/结果页/事件lineage；dedup Map/Set及payload留存；JSON字符串、数组容量、cold export/caller持有快照不可当零 |
| renderer | scenePrep F32[2048]、motion F64[10]、4×F64[128]；每DualFieldSlot F32[512]+F32[2cap]+U32[2cap]；native material/staging、实际纹理format/stride/height、FBO/program、CURRENT/STALE及context恢复重叠 |
| IO/跨线程 | 完整schema最坏UTF8/转义字节、ring/持有页、message发送与接收重叠、write队列；实际transfer完成才可免sender副本，不能仅因API名含transfer减内存 |

473个静态点的状态在开发build ledger中必须逐个写 RETAINED/REPLACED/UNREACHABLE_WITH_ROUTE_PROOF，lazy零长数组与后续替换分别记最大同时活跃值。1097个风险行区分typed.set（不一定分配）、Map增长、slice复制、JSON编码、构造；对象/数组字面量、字符串、错误对象、ArkTS/runtime隐藏容量由模块显式表补足，不能宣“枚举new即已零分配”。继承Hot的storage数组是buffer引用容器，其backing不得双计。

新增 storage registry 的 cold allocation ledger 每行字段为：`bufferId/ownerId/viewId/branchGen/instancePath/sitePathLine/type/elementLength/byteLength/aliasOf/lifetimeBeginEnd/coldOrHot/maxConcurrent/copyOrTransferFence/nativeCapacityBytes/profileRev`。热路径配置冻结后不能扩容新array/日志；原路径保留的临时分配仍具名计费，不伪装新层零分配。对象注册表和只读metadata cold预建；未知opaque hook产生的内存必须归其明确scope，否则只能报告 PARTIAL_BOUND。

### 8.2 新结构、日志与封存

pair/ID 96N/view，ticket48Q/view，基础cert32N+64C+64stage/view；另外依赖mask4stage×8、domain8×256、dirty2C、valid stage字节，不塞入基础上界。8views时 pair N192/512为147456/393216B，ticket均786432B，cert+dirty+依赖为182784/264704B；这些是**结构分项**，不是总内存。

undo24×records+64×depth；registered scalar/address table、view32×F64 headers、field stable-sort双I32C、trace256×32F64+128counters逐项另计。savepoint header只放mark/身份，实际旧状态全部在registered undo/bank内。

capture先定根内 `EpubMax/LgpubMax/JpubMax/BpubMax`，事件按原schema的实际字段宽度计；提案48×F64 evidence arena只是**版本化证据封存**，不是替换旧endpoint事件wire。双buffer/lineage/header、公有sealed对象的实际副本均入表。schema变长字符串逐字段给最大UTF8转义式（不能用平均JSON长度），无该上界的字段不能声称固定Zio。原同步API允许调用者无限保留cold快照，其外部内存项是 `UNBOUNDED_EXTERNAL`；有限lease/新拒绝须owner版本裁决，不能为了报总MB改变旧API。

`M_peak = Σ所有同时活跃unique backing bytes + JS对象/容器实际capacity + native vectors/strings/NAPI + driver资源及恢复重叠 + caller-held copies`。别名只计一次，跨scope相同引用不相乘。完整symbolic上界交付，但 aggregate配置未冻结、driver/runtime实测前 **NOT_TOTALLED**，不填虚假总MB。profile不能满足等价fixture时在cold构建使用足额配置或显式旧route，不因新配置不足缩小原请求、驱逐dedup/归档或热扩容。

## 9. Work envelope、计数与 trace 接口

以**原public root/原call-set**为分母，统一 `P011_TX_WORK_TRACE_V1`。每scope固定字段：ENTER/LEAVE/FAIL/CANCEL、scopeId/parentId、origin(DIRECT/QUEUE/SHAKE)、run/root/attempt/λ/seed/row/candidateRole/substep、cutId、view/branch/contentRev前后、原code/stage/ordinal、offered/accepted/committed体积、开关/图/包身份、clockDomain与时间。低扰动counter始终有固定槽；详细trace可关，但标DISABLED，不能填0当测得无工作。超ring/安全整数溢出标 INCOMPLETE。

```text
W_particle ≤ H_bulk*(N+L log(L+1)) + U log(N+1) + M_find log²(N+1)
           + ID_ops log(N+1) + ticket_ops log(Q+1)
           + Σ实际collect/header验证工作 + A_execute*(N+C)
           + copyPool/Field/Plan/Event bytes + logged/restored word工作
           + B_enter + Q + H + E + Lg + cert验证/失效工作
W_field    ≤ Σ原field调用的(N+64C+(p_a+1)C²+k_f*C)，以实际通道展开
W_root     = 原source/λ/seed/preview/authoritative/consistency独占工作和
           + observation/publication/encode/message/IO/render/GPU独占工作
```

必须统计：B_REQ/ENTER/COMMIT、M_FIND(found/noPair/error/active/cancel)/MERGE_COMMIT、J_SEED/R_LAMBDA、PAIR_BUILD/insert/erase/relocate/rotate/neighbor/rank/range、ID/ticket visits、COLLECT_A/B/HEADER/PLAN_BYTES、AUDIT_LOGICAL/EXECUTE/CERT_HIT/FOLD_SLOTS/COLUMNS、copy/undo/field visits/eta/water/donorReceiver/dirtyFlush、事件/lineage/page、publish/encode/write/fsync、GPU各阶段与queue时延。字段字典沿 T197 §12；T206的独立schema对接后追加字段映射，不能让生产与checker使用同段求和代码自证。

scope必须满足闭合恒等式：entered=completed+failed+cancelled+active；B_commit≤B_enter≤B_req；birthFind≤B_enter（显式MERGE另类）；M_find=found+noPair+error+active+cancel；auditLogical=execute+certHit；parent copy/work为exclusive child和。J_source≤2CR，lift不再乘R；V≤3/S≤2按原实际call-set，已经累计到frame的计数不再乘这些数。FIRST_LONG是未闭合进度，不把active计成noPair、不把缺失尾部当0。

Wpub/undo/Epub/Zio 的 preflight必须来自整个root保守上界及所有输出，checked safe integer；无源码证明常数不填经验数。当前 `WpubMax/clock/service/deadline=PENDING_DEVICE_CALIBRATION`。结构界与node包络可离线核对，设备校准常数另交tester/effector。工作量上界不能保证driver/wall-clock响应；fence/IO/调度不响应单列真实故障。

性能报告三种口径分开：同 offered/accepted root的总算力/服务时间；输入到物理commit/UI显示/归档的时延；UI线程响应。异步搬线程不等于减少工作，拒绝多数输入不等于旧算法变快。保留失败/拒绝/timeout/预热/outlier以及全分母。GPU计CPU fence等待不称GPU timer，跨clock不直接相减。

## 10. GPU只读pair的同批隔离接入

### 10.1 三backend与权威边界

隔离 `TxPairBackends` 冷选择 `LEGACY_SCAN / TX_ADJ_AVL / GPU_PAIR_V1`，每个完整public fixture克隆**同一**入口字节、params/dt/seed/源序列；逐find、逐原cut、最终账本/页全部差分。T123可作第四个参考而不是替代legacy oracle。三backend共用同一原 F64 merge writer、budget/field/事件合同；GPU只返回候选pair并由CPU消费验证。backend选择绑定configId/source+build manifest，不能某request GPU错后自动scan，不能中途改arm。

纯结构层先在现同步owner运行；GPU实验在**一个独立Worker**内运行整个fixture owner，使prepare与CPU账本在同一owner线程串行，native EGL资源也归其唯一ContextSlot。并发两个owner必须两个metadata/selector/原生state，不共享全局t159Arm。UI只收sealed evidence；这不是生产页面异步化授权。

### 10.2 可复用与不兼容接口

| 接口 | 裁定 |
|---|---|
| `pair_wire_v1.h` | 原V1 header64U32=256B/result32U32=128B、magic/done/status/echo/布局完全复用；leaf16B/query32B/candidate32B不改 |
| prepare/identity/native owner | 复用13buffer冻结、原 CPU RN64 range/canonical LE/SHA256、pair predicate与所有消费检查；`T138_CANONICAL_IDENTITY_LE_V2`原字节顺序保持 |
| `pair_binding_v1.cpp` | 原binding读 Hot workspace形状的ownerId/runToken/stepEpoch/attemptToken/substepId、params/input/seed，不能直接传新opaque handle。新增cold `TxGpuCompatWork` 显式镜像这些字段并在adapter验证其与真实view一致 |
| `T171Metadata.ets` | 全局Map64、role/ordinal/serial和旧Snapshot对象键不适合新多owner/branch。新增每owner固定表，不直接借全局Map，也不重写旧generator |
| `T171Selector`/Native包装 | 全局arm及每request对象/字符串/计时数组不自动变成新owner API；隔离adapter单独管理，现分配成本完整计入；不冒“native已有所以零分配” |
| T191 O0/O2构建 | 复用隔离可复现编译/签名流程，各自source/header/shader/ABI/SHA清单；改bridge后必须新manifest/新包，不将旧签名证据移贴 |
| 新CPU动态索引 | 只在CPU树backend增量维护。旧GPU协议仍每mutation freeze/sort/range/hash/upload，不因CPU树存在声称GPU驻留/增量更新 |

### 10.3 身份映射与迟到拒绝

native现要求 run=`configuredRunId+"|runToken:"+runToken`，state=`owner:<owner>|step:<step>|attempt:<attempt>|substep:<substep>`；ctx与work镜像必须完全相符。adapter维护 `real root/view/branch → compat safeInteger step/attempt` 一一映射，branch不同分配新compat identity，不能改旧字符串规范或把真实业务step号悄悄重编号。映射在sealed trace里保留真实/compat两套键；compat仅实验桥，不回写原业务账本。

ownerGeneration/contextEpoch、request/payload serial、poolMutationSerial、attempt/substep、candidateRole1..5及其正ordinal、slot0/1、params/input/seed raw bits、source/build/shader SHA共同绑定请求；role表在cold config逐一映射原scope并写configId，不能把所有preview和authoritative都填1。新branch、rollback恢复相同字节、同slot复用均须新控制content revision；request/payload严格单调，role ordinal按native现验证不倒退，绝不在rollback/reset后复用旧请求身份。

请求绑定冻结13buffer的身份/长度/值和所有标量；发送后不能被CPU merge/另branch修改。native消费已核 sameIdentity+sameSnapshotBytes，adapter再核真实owner/view/branch/cut/contentRev仍是原请求，才运行CPU原merge。迟到、reset、取消、context退役、foreignowner、同rev异内容一律STALE/FAULT，不入池、不写budget、不中途换backend。需回滚按原当前root域，既有成功source前缀保留。

### 10.4 完整scope、资源与错误

一次GPU find scope分项：freeze13+结构/字节验证→sort/idOrder/range→canonical pack→snapshot/payload/identity SHA各自实际块数→NAPI/context字符串与镜像→upload→build/query/reduce dispatch+barrier→单尾fence等待/map→header/result/done/status/identity/原predicate/可选oracle消费→CPU原F64merge/audit→外层封存/归档。SHA块数逐digest为 `ceil((messageBytes+9)/64)`；不可跨mutation复用payload SHA，cold不可变config前缀缓存另名计费。

每CPU slot原frozen `55N+16C`、order/idOrder8N、freeSeenN、leaf16K、query32K、header256/result128、canonical identity容量**12288B**，两slot；原run/state分别1024UTF8、config8192，不可只预8192总identity。native vectors/strings capacity与NAPI wrapper对象另算，不把ETS镜像和native副本当alias。GpuSlot六SSBO为 `{256,16K(h+1),32K,32K,32K,128}`，两slot总 `2*(256+16K(h+1)+96K+128)`；实际upload/request=`256+16K+32K+128`。N192/K256/h8的11dispatch、N512/K512/h9的12dispatch沿现 reduction图逐stage计，实际实现不同需新manifest，不套固定数遮漏。

ContextSlot归唯一native owner线程，独立EGL pbuffer16×16/program3，不假定能借XComponent渲染context；compute与render共享的是目标设备资源，争用/资源峰值仍待真机。renderer只读完整published capture，compute fault不能Canvas回退。

状态机 `COLD→CONFIGURED→SLOT_FROZEN→SUBMITTED→FENCED→CONSUMED/STALE`，GL错误保留首个码/阶段；timeout/contextlost进入QUARANTINED。未确认driver完成的slot/buffer/context不能立刻重用；实际owner线程销毁fence/SSBO/program/context后才retire，析构/等待失败保留占用并显式报告。configured owner额度耗尽不能假reset“恢复”；沿已有契约需要新OS进程时实做新进程。迟到完成只能清理资源，不得复活已取消物理root。

wall/fence watchdog是实验可观察故障行为，绑定experiment config；不在纯等价生产层凭空加timeout。完整生产GPU迁移必须另有算法/事务/资源与UI收益证据。

## 11. Owner/admission候选：交manager的具体API差异

以下是已明确的新行为选择，不是本卡等待manager验收；等价核心和GPU隔离可以推进。

| 差异 | 候选合同/待裁决点 |
|---|---|
| 同步返回变异步 | `submit(command): PendingReceipt(requestId)`，其ACK只代表接收；`awaitCompletion(requestId): OriginalReceipt`才有源成功/原失败。必须决定公开API版本与调用方适配，不能沿旧同步名返回假成功 |
| 输入队列/顺序 | Kq=16，每条256B header+2C F64 payload，C128为36864B。FIFO保留offered输入/dt/θ/sequence，stop/reset/capturefinish均为有序control命令，不合并/丢旧source、不改dt、不抢占到半提交物理root |
| 准入/背压 | `WOULD_BLOCK`/`REJECT_WORK_ENVELOPE`明确新状态；整个root的W/undo/event/lineage/lease/IO容量先预留，保留offered分母。重试幂等身份/接收截止与UI提示须裁决，不用PARTICLE原错误码冒充 |
| 运行超界/取消 | 已开始发现bound violation按当前root整根rollback，失败回执保留原成功source前缀；不可partial grant后报告整体accepted。控制取消是否仅在root边界生效与deadline时钟语义需定版 |
| publication/IO | 建议2publication/2IO banks，有限lease/credit；外部持有/慢IO背压是新行为。物理commit后write失败保持物理成功、capture INCOMPLETE；不能改写成功源账本 |
| UI帧与故障 | 只消费完整sealed新管线frame；新帧失败FAULT或明确STALE上个有效frame，不能把旧frame序号当当前。允许“只显示最新”与丢档完全两事，不能默认允许后者 |
| 内存与时限 | Bpub/Epub/Lgpub/W/undo/Zio/外部lease上限与设备service/deadline常数待具体profile；未选择时不启新生产拒绝，不宣硬实时 |

同步 `executeSync` 版本不受这些新状态影响；隔离实验可显式使用候选profile测试背压，但产物标experiment/versioned behavior。

## 12. 实施接力、差分入口与产品证据

这是后续可执行工作范围，不是本卡创建远端任务或新增manager批准门。manager据交付登记派工；developer只负责开发、编译和不涉真机自测，端到端由独立tester/effector接力。

| 阶段/责任 | 具体输入与交付 | 实际前置 |
|---|---|---|
| D1 developer核心 | storage/opaque view+whole-bank、PairAdj/ID/ticket、所有原cut trace、独立构建三backend选择入口；先保留全部数值fold/原验证，cold allocation ledger与实际深度输出 | 本稿+T197；不等待owner新行为裁决 |
| D1G developer GPU优先并行 | 隔离TxGpuCompatWork/Metadata/Worker及新manifest；同public输入跑LEGACY_SCAN/TX_ADJ_AVL/GPU_PAIR；wire原字节/原消费验证，不碰生产开关 | GPU桥可先接legacy whole-bank；树backend待D1联调，避免空评估卡挡整体 |
| D2 developer受控优化 | wordundo逐域/证书逐predicate/dirty-column/header省写/稳定sort；原路径与有名unsupported写入覆盖表，allocation/work envelope更新 | D1；每项有独立read/write-set与rollback证明，未接保留旧成本 |
| I tester独立 | T206 TX01–TX12 oracle+trace checker；逐pair/原cut/firstcode/rollback byte、full root/frame计数与资源峰值；未实现预测试RED登记NOT_IMPLEMENTED | 可先产规格，执行等对应实现；不能引用T197模型当oracle |
| P tester设备 | 新签名包/原档、N192/512+C/B/Q/E独立变化与accepted满池锚点；先correctness，再同输入三backend+O0/O2服务/prepare/归档全成本；owner/context/迟到与资源释放 | 对应实现/可复现包身份；设备串行，打真机标签 |
| E effector设备 | 同root负载的真实触摸/停止/源准入/床变化/完整shader场景，UI竞争、响应、持续稳定性；失败frame身份与STALE明确 | 对应无阻断实现与设备窗口；保留T207材质故障独立真实前置 |
| O owner版本实施 | 第11节已裁决的API/资源/FIFO/cancel/IO合同，真实预留与完整root运行，再独立/设备验证 | 仅此新行为层需行为裁决，不反向阻塞D1/D1G/D2 |

差分fixture入口拟 `test/tx/TxFixtureDriver.ets`，接受一个不可变 `TxFixture{entryStateBits, commandSequence, paramsBits, algorithmRev, profileRev, backend, faultPlan}`；每backend从独立克隆开始，输出原每cut状态/receipt/events/lineage、trace、allocation ledger。不能仅录最后poolSHA。原失败未归档的第5子步不凭最后accepted状态重造“真实输入”；可构造满池synthetic fixture并标 synthetic、把缺证实际入口留后续精确取证。

独立矩阵沿T197/T206：TX01规模/满池；TX02非相邻最小b/浮点边界/slot复用；TX03每原cut/预算/部分源成功；TX04各read-set翻位/同rev异内容；TX05 slot-vs-ID/tree fold反例；TX06 source/λ/lift/V/S/DIRECT全乘数；TX07field残余；TX08nested undo/event/ticket溢出；TX09准入/分母；TX10FIFO/control/慢IO/迟到与STALE；TX11GPU完整链与包身份；TX12峰值/work/真实UI与归档。TX09/10中新行为仅对candidate版本判定，不拿未实施RED派产品返修。

故障注入点为各原切点前后、key删除与物理写中间、nested child成功后parent失败、log/output预留、fence迟到/contextlost、封存后encode/write失败。注入必须绑定faultPlan/config并隔离产物，不进入生产默认；observer/callback是否可见原状态须逐点断言。

T206当前原卡评论已开始，声明trace工具与TX执行器尚NOT_IMPLEMENTED；本稿未消费其未交付正式规格，不造成互相等待。若它发现算法矛盾交designer、原用例争议交manager，实际测试失败先回architect归因；工程缺陷先改本稿再developer，证据不足提出精确补证，不笼统要求重跑所有卡。

## 13. 本卡完成范围与自查

交付：原数值/错误/成功边界、opaque接线与hunk所有权、动态结构/rollback/cert、完整symbolic资源与全链工作表、GPU同输入隔离接口/生命周期、owner具体待裁决差异及分阶段派工。没有业务/endpoint/wire修改，没有设备或产品性能结论。W/资源profile校准、真实driver/runtime容量、T206独立执行与后续实现属于已明确后续工作，不伪造为本卡产品PASS。

只读自查范围：原ledger解析及输入/源码SHA；manifest字段/模板/算式与本稿一致性；T197原图约束；GPU现ABI/raw identity/资源生命周期；文档diff。未运行算法模型、产品用例、构建或设备。本轮只更新自己T205行状态并回原卡，清单其它行/队列/滴答同名清单由manager维护。
