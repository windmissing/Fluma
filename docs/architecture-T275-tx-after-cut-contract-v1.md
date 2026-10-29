# T275 原事务 AFTER 切点合同 · 工程裁定 v1

2026-10-29，architect；不使用真机。消费 T250 原卡最新回执、`developer-T250-source-cut-v8.md`、`test/reports/t250/cut-inventory.json`、T256/T245 工程及当前原函数。此稿修订 T256 对“20 cut 逐点注入”的歧义；BEFORE 18 个待开发照原 T250 继续，不把它们归为合同缺口。T250 已交 cut1/cut6 BEFORE 的原返回码、撤销对照和原字节回滚保持。

## 1. AFTER 的可执行语义与 firstCode 归属

`AFTER` 是某个**原检查/写入已成功返回**的观测边界。若边界后存在同一 root 内的原消费方及原拒绝谓词，可在**调用该消费方之前**仅损坏它尚未冻结的隔离输入，让消费方按原代码返回原 code/receipt；firstCode 属于实际拒绝的**消费者 stage**，而不是把前一已成功 cut 的 `txCode` 改写成失败。注入要带 `(root,parent,cutId,edge,ordinal,originalConsumer,originalCode)`，记录变更前后字节和 finally 撤销；已成功的子步/源前缀不回滚。若没有可损坏输入、没有同 root 后续拒绝、或已越过权威提交边界，则 `AFTER_OBSERVE`/`AFTER_NA`，只核只读状态、回执、事件/lineage、rollback readset，不人为 `throw`、调用新拒绝接口或改已返回码。后续消费者自身的 BEFORE 失败不能重复记为前驱 cut 的失败覆盖率；可在关联字段记 predecessorCut，但唯一 firstCode 仍归消费者。

生产不得增加 fault hook；隔离生成器不得改物理语义/阈值。原异常若业务本来抛出，保留原异常类型/消息并独立记录 `firstStage`，不能 `txCode=-1` 后宣称产品定义了 -1 业务错误。`source` 原 precheck 对无效/零请求返回 `null`、`txCode=0`；`NO_WATER`/`CAPACITY_BLOCKED` 是原 receipt reason，不可替换成新 code。每条冷 fixture 有真实 original consumer 和可再现输入才宣 `AFTER_INJECTED`，否则只宣观测或 N/A。

## 2. 20 个切点逐项裁定

下表“下一消费者”用于确定可追踪的后续原返回；它**不是**给前一 cut 另造一个返回码。文件行号按 `cut-inventory.json` 当前快照，后续源码变动用函数/上下文及 manifest SHA 重新绑定。`OBS`=成功后只读观测；`NEXT`=可用原下一消费方真实拒绝，但 firstCode 归该消费方；`N/A`=无同 root AFTER 故障语义。

| cut | 原位置（当前目录 `entry/src/main/ets/physics/`） | AFTER 裁定与下一原消费者 | 可控输入、原码及回滚边界 |
| --- | --- | --- | --- |
| 1 SOURCE_IDENTITY | `WaterSandField.ets::injectGestureSand` precheck，约1803 | **NEXT**：后续 route `anyWet` 检查 | 合法正请求置于无湿列，原 `NO_WATER` receipt、`txCode=0`；precheck 本身成功，backup 后恢复。若预检无效输入则属 cut1 BEFORE、原 `null`，不得移来 AFTER |
| 2 SOURCE_CAPACITY_ETA | `WaterSandField.ets::injectGestureSand` λ循环入口，约1860 | **NEXT**：原全局湿容量/η solve/`gestureInjectionAudit`、≤8 λ重试 | 冷 fixture 用接近容量但合法的源量/库存；原循环返回 `CAPACITY_BLOCKED` receipt 或原正准入，失败按 `restoreBackup` 回滚，本 cut 无新 firstCode；不能在成功 solve 后改已接受源量 |
| 3 BEGIN_HOT | `P011ParticleHot.ets::beginStepHot`，约505 | **OBS**：原 begin 已成功；后续 collect 以自身 status 归属 | 只读 work.active/epoch/backup/池字节；后续 collect 故障记 cut4/5 或实际函数，不改 begin 返回 |
| 4 COLLECT_HEADER | `P011ParticleM2Field.ets::collectBirthHeaders`，约135 | **NEXT**：同函数 `collectParticlePlanInto` 的原 status 是实际消费者 | 用冷 fixture 的合法/非法 birth header 触发原 collect status；firstCode 归 collect，失败保留此前成功阶段；不能在已返回 header 后伪改 status |
| 5 COLLECT_AB | `P011ParticleM2Field.ets` A/B collect，约270 | **OBS**：A、B 各自 `collectParticlePlanInto` 返回后无第二个同 cut 谓词 | 记录两次原 status/plan/序列；B 原失败归 B collect，不把 A 成功写失败；后续 sink 的拒绝归 cut6/7 |
| 6 REQUEST_VALIDATE | `P011ParticleHot.ets::validateParticleHotRequest`，约910 | **NEXT**：`P011ParticleM2Sink.ets::applyInto` 的原 `copyPoolDiagnosticHot`、target/free、budget/guard | 以合法请求进入 sink，再用冷 fixture 的真实资源不足/guard 不可准入；原 `applyInto` code/`failHot` 归 sink cut7，cut6 保留成功；cut6 BEFORE 已交的原 validator 拒绝不重复 |
| 7 SINK_RESERVE_GUARD | `P011ParticleM2Sink.ets::applyInto`，约174 | **NEXT**：同函数的 `preparePatchHot` 与事件/lineage 容量核 | 仅用原 budget/guard/事件容量 fixture；原 `preparePatchHot` status 或 `failHot(9/10)`，回滚由 sink 原路径；不越过已有 ticket/guard 预留造外部写 |
| 8 PATCH_PREPARE | `P011ParticleHot.ets::preparePatchHot`，约1071 | **NEXT**：`P011ParticleM2Sink.ets::applyInto` 的事件/lineage capacity、`publishPatchHot` | 原容量 `failHot(9/10)` 或 publish 原码，归事件/发布消费者；保留 prepare 成功 token，失败按原 sink rollback，不手改 token 返回值 |
| 9 FIND | `P011ParticleHot.ets` merge index `find`，约961 | **OBS**：find 的 found/noPair 是原搜索结果，不自动是失败 | 冷 fixture 分别覆盖 found/noPair/error 的原结果；下一 prepare 或 sink 错误归其实际入口，不能把 noPair 冒 `firstCode` |
| 10 PATCH_PUBLISH | `P011ParticleHot.ets::publishPatchHot`，约1110 | **NEXT**：`P011ParticleM2Sink.ets::applyInto` 的 `auditParticleHot('SINK_ROW')`、现有 `failAfterPublish` | 原 `audit` 非零或已有 `failAfterPublish`→原 `failHot(13)`，firstCode 归 sink 原分支；核已发布候选回滚与已成功前缀，不改 publish 成功码 |
| 11 AUDIT_SINK | `P011ParticleM2Sink.ets` 的上述 `auditParticleHot` | **N/A（字典缺入口）**：inventory `originalSites=[]`，无独立 cut11 checkpoint | 先绑定精确原调用/ordinal；其后是 `failAfterPublish`/ticket close。未绑定前不能宣 cut11 AFTER 已执行；已有 audit 失败按 cut10 后消费者或 sink 函数首码 |
| 12 TICKET_CLOSE_EVENTS | `P011ParticleM2Sink.ets::applyInto` `budget.closeInto`，约256 | **OBS**：close 原码本身属 cut12，后续事件写封存 | 记录 ticketToken/closed/refund/events/lineage 原位型；close 失败应在该原调用 BEFORE/执行时触发，成功后不再改 close code，后续审计按自身 stage |
| 13 MOTION_APPLY | `P011ParticleHot.ets` motion apply，约1202 | **OBS**：motion 原执行结果已定 | 后续 `audit/finish` 的失败归各自实际消费者；记录 patch/actions/预算与失败时原 rollback，不以 AFTER 篡改 motion 结果 |
| 14 AUDIT_TAIL | 尾审计候选，inventory `originalSites=[]` | **N/A（字典缺入口）**：尚无可复核原 checkpoint | 必须先在原函数找到确切调用与 firstCode 才谈 AFTER；不能把任意尾阶段用同名 label 充入20 cut |
| 15 AUDIT_FINISH | `P011ParticleHot.ets::finishStepHot`，约611 | **OBS**：finish 结果为该作用域终态 | 记录原 status/event/lineage/sealed 与 rollback readset；之后父 root 失败按父 stage，不回写 finish code |
| 16 PREVIEW_RESTORE | `WaterSandField.ets::previewChannelPlansAt`，约5302 | **OBS**：preview 可返回 plan 或 `null`，该返回即原判定 | 父 planner 的下一候选/恢复决定归父；保留 `null`、原 snapshot/角/λ，不把成功 preview 改成拒绝 |
| 17 AUTHORITATIVE | `WaterSandField.ets::stepWithTiltTrace`，约2331 | **OBS**：root 的子步成功前缀、尾失败均由实际返回确定 | 记录每子步 commit/firstCode、authority before/after；失败仅回滚失败候选，不能撤此前成功子步或在 root 返回后造错误 |
| 18 FIELD_AUDIT | `WaterSandField.ets::audit`，约7327 | **OBS**：原 `WaterSandErrorCode|null` 即判定 | 审计失败应由该函数原谓词触发，返回 `null` 后不再改；只读守恒/容量/库存与首错 |
| 19 SOURCE_PUBLISH | `WaterSandField.ets::injectGestureSand` `sourceEpoch++`，约1953 | **N/A（权威源已提交）**：后续 `publishRenderState`/通知/导出是发布和观察 | 源成功账与粒子/水 authority 已成功，不得事后 rollback 或改 admitted receipt；下游 IO/渲染故障单列，不伪造源拒绝 |
| 20 OBSERVE_SEAL | `WaterSandField.ets::finishNaturalObservation`，约4037 | **N/A（终观察边界）**：无同 root 下一原拒绝消费者已绑定 | 记录封存记录、原 errorCode 和缺采状态；不能 throw 成新物理首码；若未来有新 consumer，须升字典版本并给精确原谓词 |

其中 cut4/7/8/10 的 `NEXT` 仍要求 developer 用源码与构建 SHA 证明**实际**下一原函数被调用及原码可复现，未做时标 `AFTER_NEXT_NOT_IMPLEMENTED`；不能把此表当已通过注入。cut1/2 的 return 是 source 原 receipt/null，`txCode=0` 与 receipt reason 并列。cut11/14 缺原 site 是**独立字典绑定工程缺口**，须补原调用位置；若正式测试坚持每 cut 都有 AFTER 业务 FAIL，cut19/20 等 N/A 与既有事务提交/终观察语义冲突，交 manager 裁决测试用例，不由 developer 删除或伪造。

## 3. 给 T250 的字典与离线回归边界

将 `cut-inventory.json` 升至 `T250_CUT_SITES_V2`，每行增加 `afterDisposition(OBS|NEXT|NA)`、`consumerFile/function/predicate`、`fixtureInput`、`expectedOriginalReturn`、`firstCodeOwner`、`rollbackBoundary`、`reason`，并保留 v1 原始记录供追溯。只有原 consumer 的真实执行、原始 return/receipt 和逐字节 authority/receipt/events/lineage 对照齐，才把 NEXT 标完成。OBS 需采已成功后的只读状态及后续结果，NA需明确无失败语义；不可把它们计入“故障注入 PASS”分母。所有 case 均记录相同包/SHA、freeze arm、root/parent/cut/ordinal、offered/admitted/accepted、冷克隆输入、撤销注入再跑对照；日志缺位 `INCOMPLETE`，已定义未实现 `NOT_IMPLEMENTED`。18 BEFORE 按 T256 原计划独立继续；不能因 AFTER 合同停整卡，也不能将已交 2/40 扩写成产品 PASS。

离线编译与自测属 T250 developer；T251 真机仅在完整工程/签包后按实际可执行字典续证。新增渲染仍仅 XComponent+shader 含前景背景，失败 FAULT 或完整旧 token 显式 STALE，禁 Canvas 上屏/回退；不重跑已过 64root 矩阵，不改物理 dt/阈值/算法。
