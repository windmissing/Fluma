# T64：T44 失败归因与 T66 最小修复任务书 v1

日期：2026-10-26。角色：designer。原卡：`6adea6ece4b06ee35734245f`。技术路线：`T44_CAPTURE_RENDER_REPAIR_V1`。本次交付为归因与修复设计完成，**不是修复完成或产品验证通过**。不使用真机，不修改产品代码、既有报告、原始证据、冻结协议、质量预算或测试判据。

## 1. 裁决与证据等级

优先修复产品渲染接线、采集结束入口和提交身份装配；目前没有证据要求修改粒子运动、LAND、预算或 endpoint/peak 算法。三类失败不能归成同一个物理故障。

| 失败 | 确定事实／代码缺陷 | 尚缺现场事实 | 分流 |
|---|---|---|---|
| F44-VISIBLE | 粒子发布快照是 schema 4；Canvas 回退 adapter 仅接受 2/3，返回空视图，renderer 不画床也不画真实粒子。这是确定的回退契约断裂 | 主 GPU 路径实际首次失败谓词、native ready/尺寸/epoch、draw/swap 与合成身份 | T66 修复已证实回退及产品入口几何契约；T65 补实际 controller/adapter 夹具。现场渲染身份由后续真机卡补 |
| F44-CLOSE | 页面 hide 仅停 renderer；仅销毁等路径调用 store.close，且 catch 为空。没有可靠的前台结束采集命令；worker 写入进度未被主端消费 | 本次 Back 实际走到哪个回调、close 是否发送、队列是否排空、错误是否被采到 | T66 增幂等结束与收尾观测；T65 还原现有生命周期/worker 真实实现的离线夹具 |
| F44-IDENTITY | 重复 sequence 的记录有不同 attempt/substep/committedOrdinal；getP011Metric 自增序号，而无 hook 的成功 step 不递增。store 元数据 route 写死，与实际账本不同 | 现场模式切换的发起来源；旧导出是否另有未保存尾部 | T66 分开采样／提交／导出身份，修元数据。不能删记录或把所有重复解释成重发 |
| 准入累计差一位 | 原分析用 math.fsum；按生产端原序 binary64 左折叠，与全部 283 条账本对应 sourceEpoch 的累计逐位相同 | CLOSED 后的最终两流计数及尾部 | 属分析口径说明，交 T65 补生产端同序累计；不修物理累计、不改容差 |

“代码确定”不等于“现场已经观测到该分支”。尤其不能把 T52 的 NONUNIFORM_SCALE/INVALID_PARTICLE 页面错误直接移植为 T44 的设备根因。T44 原画面未显示这两码。

## 2. 基线、输入与边界

- 唯一设备基线：隔离工作树 `D:/hmprojects/t44-build`，HEAD `d941e7fd1a94aa1407c757277fcebda4ad673e07`，产品 tree `7836d441b643d4678225ac12e10146e51b8f8eff`；读取时 `entry/src` tracked diff 为空。
- 安装包：`entry-default-s142-signed.hap`，SHA256 `E47FA0DE97573019AA5DCF1A59163E219EEE4B59E4D357059DC44E860C4B18D8`。不把主工作区后续 T49/T51/T62 的状态当作该包状态。
- 输入：`docs/effect-report-P011-T44-dualfield-product-v1.md`、T43 装配回执、T44 执行案；`test/artifacts/t44-dualfield-product-run-20261026/` 内原始两流、actions、full/close 日志。照片／录像的无沙结论来自 effector 报告，本轮没有重新跑设备或重新评分。
- 本轮可复算细节与输入 SHA256：`docs/analysis-P011-T64-raw-evidence-v1.json`。这是 designer 对归档前缀的分析资产，不替代 T65 独立测试报告。
- R0 `p011-run-1792975718417-3`，R1 `p011-run-1792975806102-3`，分开核算；R2/R3 未跑，不补造窗、峰值或终点值。
- R0/R1 前缀 particleRouteActive 分别 954/954、283/283；R1 池峰值 512、质量峰值约 38.8453。初始已有 H3072/L2048 悬浮库存；不能把床增量全算成外源触摸 LAND，也不能凭没有可见沙断言粒子未生成或未 LAND。
- 缺 CLOSED，所有数字保持 PREFIX_ONLY；原始约 5.15e-14 的预算超出与其它残差仍保留，不截零、不重新宣布审计通过。

## 3. F44-VISIBLE：两条渲染路径分别处理

### 3.1 确定的 Canvas 回退断裂

隔离版 `SandCanvas.ets` 约 2194–2237：粒子 route 且 wssnapshot=false 时，`copyPublishedRenderStateInto` 同步复制 field/pool；其 field 替换 waterSandSnapshot。GPU receipt 非空、code=0 且 swapSucceeded 才置 dualDrawn=true；否则进入 P012 全量 Canvas 渲染。

隔离版 `WaterSandField.ets` 约 3930 明确要求发布槽 field.schemaVersion=4。`P012WaterSandAdapter.ets:195` 的 validateSnapshotFinite 却只接受 2/3；拒绝后 update 返回 curr=null，reason=NOT_FINITE。`P012WaterSandRenderer.ets` 约 405–413 看到 curr=null 就返回，而页面背景已清。故该回退无法画沙；错误还把“不支持 schema”混同“非有限”。这是源代码即可确认的缺陷，无需推断坏粒子。

T66 必须让 Canvas 明确消费 schema 4 同一次发布的 field、真实粒子和上下文；验证 run/stateEpoch/physicsCommitId/route、cols/capacity/nmax/liveCount、数组长度、有限值及源域。保留 2/3 老入口；不可把 4 伪装成 3。Canvas 用权威 Float64 源域校验，不应继承 GPU Float32 包转换造成的拒绝。

回退画床与真实池，池激活时 B tracer 仍互斥；失败后隐藏旧 GLES 沙层并清前景残留，成功后只由 GPU 画沙，防止双画。返回实际 `renderer=GPU_DUAL/CANVAS_DUAL/NONE`、fallbackReason 和 token。Canvas 成功只证明回退可见，不能证明新 GPU 管线通过。

P012 原配对质量只含 D+U，不含 P；LAND 与外源使 D+U 变化时，不可强制 pair.valid=true 或放宽其守恒容差。最小修复允许合法当前几何静态显示，历史配对失败仍如实报告；要启用跨帧流动推断，需另立同源 D+U+P／sourceEpoch 配对契约，不夹带在“接收 schema 4”中。

### 3.2 T44 自身的冷启动几何反例，而非借用 T52 结论

T44 原 `P011_RASTER_LAYOUT` 给出的逻辑尺寸是 365.7142857142857 × 742.2857142857143。隔离版 controller 的 submitDualField **先 pack，再检查尺寸，最后 native draw**。若 native getState 尚无真实 surface 尺寸，它用 round 得到 366×742：

`sx=1.00078125; sy=0.9996150885296381; abs(sx-sy)=0.0011661614703618595 > 1e-6`。

这会在 native draw 前触发 NONUNIFORM_SCALE。隔离版 native Init 仅绑定 owner/启用；GetState→QuerySize 在 EGL_NO_SURFACE 时不创建 surface；OnWindowCreated 只记录 window/hint。实际创建位于 draw 路径 EnsureCurrent。因此冷启动存在确定的循环依赖。**条件计算成立；本次设备没有保存 state/receipt，不能宣称已捕到 T44 首拒码**。pack 还在尺寸检查之前，不能排除更早的源域或表示拒绝。

T62 已增加独立 prepare、真实 surface px 入口及显式 Float32 表示策略。当前主区 SandCanvas 仍未调用 submitDualFieldSurfacePx。T66 应比对并复用 T62 已完成的 controller/native hunk，把 SandCanvas 的容器测量、原点、radius、epoch 与完整容器布局接到同一像素契约；不能只在 controller 新增接口后声称产品迁移完成。真实非均匀变换、过期 epoch、非法源仍拒绝。不得重做或覆盖 T62，也不预设 INVALID_PARTICLE 在 T44 已被设备证实。

### 3.3 必须补的渲染观测

限频结构化回执至少包含：runId、发布 token、ledgerCommitKey、geometryEpoch、renderer、packResult、首个拒绝字段、native state/sizeSource/widthPx/heightPx/surfaceEpoch、prepare/draw code、swapSucceeded、可见层选择、Canvas schema/reason/绘制计数。错误首例与状态转换必记，其余限频；不输出长数组。

T44 full/close 中没有足够的 JSAPP/native 分支日志。日志采集 tag/级别可能过滤 console；因此“没找到 p013_dual_frame”不能单独证明没有发生 draw。新真机卡需在同 PID/run/token 下取渲染回执和画面，不能借同窗口其它 G0 swap 或屏幕尺寸代替。

## 4. F44-IDENTITY：不是导出重发，也不是已证实的坏账本

### 4.1 原始反例

R1 最终文件 283 条有效 JSON；sequence267 出现在第 267–273 行，共 7 条，多出 6 条。对应 substepId 和 endpointCommittedOrdinal 分别为 267–273，attemptId 为该 run 的 a267–a273，frameId 为 134/134/135/135/136/136/137。原行 SHA256 全不同，物理状态／事件有变化。283 个 attemptId、完整关联键和 committedOrdinal 各自唯一。**不能按 sequence 去重，否则删掉真实成功提交。**

第266行 modeChangeCount=0；第267–272行为1；第273行变2；第274行 sequence 恢复增长为268。隔离版：

- getP011Metric（约2202）自身执行 p011SampleSequence++；额外读取也会消耗这个编号。
- beginNaturalObservation 与 recordEndpointCommit 都用 p011SampleSequence+1。
- 成功 step 的尾部只有提供 p011Hook 才调用该 getter；SandCanvas 只在 testMode=true 提供 hook。
- setEndpointCaptureMode 每帧记录 TEST/EXPERIENCE 转换，所以无 hook 区间与重复序号区间吻合。
- sampler 的现有判定允许 sequence 相等、只拒绝倒退；它还检查其它身份。因此不能擅自把现有数据按“sequence 单列不唯一”直接改判 INVALID_LEDGER，完整键仍应保留并分析。

结论是提交关联编号错误依赖观察者／模式，破坏稳定的单调序号用途；不是已证实文件重发或物理提交被执行两次。LongPress 400ms 可调用 applyInputMode，Pan 也在页面；600px/s 滑动不能自行换算成模式切换证据。原日志没有对应 P011_MODE boundary，发起来源保留未知，交 T65／后续现场日志补证。

### 4.2 推荐唯一实现口径

采用 `P011_COMMIT_IDENTITY_V2` 的显式兼容说明（只修装配身份，不修改 endpoint/peak 冻结公式、阶段、时钟或门槛）：

1. 每次实际物理 attempt 进入时分配一次 run 内单调安全整数 ledgerSequence；与 attemptId 同生命周期。失败／blocked 也保留原记录，可有成功提交序号间隙。所有 context 由同一 attempt 身份对象复制，不再现场 `getterCounter+1`。
2. 成功物理提交后只推进一次 committedOrdinal；将该 attempt 的 sequence/frame/substep/attemptId 与 ordinal 组装成一个不可变 commitKey，同源提供给账本、endpoint/peak stamp 与 metricCommitKey。没有合法 clock/phase 时仍记录真实成功次数，但 stamp 保持缺失并报错，不伪造合格样本。
3. getP011Metric 不分配 ledgerSequence/commitKey。诊断读取、无 hook、TEST/EXPERIENCE、额外 UI 读取均不改变提交编号。需要标识每条 metric 发布时，另增 telemetryOrdinal；noop 遥测不能伪造成功 commit 或增加 integratedPhysicsSeconds。
4. attempt 与提交计数不因 rollback 回退；物理 snapshot/restore、reset/new run 的身份生命周期明确测试。安全整数溢出在物理执行前拒绝并报告，不能执行后才发现无法编号。现有 frame/substep 保留其语义，不要求跨失败 attempt 单独唯一。
5. Store 每条 ledger/admission 的 exportOrdinal 各自单调，和物理 commitKey 分离。重试若存在，使用相同 export 身份和明确重试状态；本次没有证据需要加重发功能。
6. 渲染 physicsCommitId 还覆盖 EXTERNAL_ADMISSION/RESET 等发布，不能强行等于 endpointCommittedOrdinal。发布 token 与 ledgerKey 关联而非混为同一计数器。
7. 对新身份 revision 的“登记一个新成功提交”检查严格增长；对“给已登记提交打 stamp”检查完整 key 等于已登记的那个 key，不再次计数。保留旧 revision 历史解析，不用新断言改判 T44 历史文件。新增身份描述须同时写入 manifest/报告，原字段名不静默换含义。

模式切换即便不再造成编号重复，仍会影响冻结协议 eligibility；modeChangeCount 必须保留，不能为合格而清零。触摸与长按冲突若被补证，T66 可在采集模式显式禁用隐式模式切换手势并保留独立模式命令，记录被拒事件；不在缺来源证据时改一般交互语义。

### 4.3 准入累计、route 元数据

27 条 admissionTransactionId 与 cumulativeCount 在原文件顺序都是1..27。按 `acc=acc+admittedH/L` 的 binary64 左折叠：

| 方法 | H、L 各自结果 | binary64 hex |
|---|---|---|
| 生产端同序累加／账本最终源累计 | 19.731137432098386 | 0x1.3bb2bd29fbe76p+4 |
| 原分析 math.fsum | 19.73113743209839 | 0x1.3bb2bd29fbe77p+4 |

全部283条账本的 sourceEpoch 前缀均逐位匹配，失配数0。约3.55e-15由求和算法解释，不据此派物理 source 返修。T65 补独立复算与两种算法标签；不得排序、去重、覆写原始源值或放宽逐位要求。缺 CLOSED，仍不声称整个 run 准入流完整。

SandCanvas 约1149的 P011_OBS_STORE 把 observationRouteId 写死为 P011_NATURAL_OBSERVATION_ROUTE_ID；实际账本为 dual-field-particle-v1。修为采集启动时实际选中并冻结的 route，并带 run/storeGeneration/identityRevision。若想记录探针实现版本，另用 implementationRevision，不能冒用物理 route 字段。之后 route 变更保留事件并按既有协议使 run 不合格，不掩盖变化。

## 5. F44-CLOSE：明确结束点，区分请求、写完与进程终止

隔离版 SandCanvas onPageHide（约756）只清手势、p013.setActive(false) 和可见标志；aboutToDisappear（约767–795）才解绑 probe/admission sink、调用 observationStore.close，异步错误被吞。EntryAbility 的 background/destroy/windowStageDestroy 没有采集结束桥接。不能假定 Back/Home 一定触发页面销毁。

FileStore 确有真实 worker。append 的 sentRecords/sentAdmissions 先增后发送；worker 对每行 JSON 完整 UTF8 短写循环并 fsync，写完发 written；close 在队列前序消息之后 fsync/close 两文件，再发 closed，主端严格比对两计数才记 COMPLETE 并终止 worker。主端当前忽略 written。故“没有 CLOSED”既可能没有发 close，也可能未排空、异常或日志缺失。≥4秒不能证明充分排空；force-stop 也不能替代正常关闭，不能凭可解析文件宣布全部写完。

### 5.1 推荐收尾状态机

`OPEN → DRAINING → CLOSED`，任一步错误进入 `FAILED`；被强杀而缺最终回执为外部判定 `PREFIX_ONLY`。

- 增加前台显式 finishCapture(runId,requestId,storeGeneration)；输入命令为单用途 envelope，可复用既有 shakeOnly 防 reset 模式，但不能复用会冷启／重置／切模式的通用 Want 更新分支。
- UI 单线程在 attempt 间边界封住本 capture 的生产者，冻结 final ledger/admission sentCount、last commitKey/sourceEpoch；同一请求幂等，旧 generation 请求不能关闭新 run。随后拆 probe/admission sink 并发 close；两流的已接受消息必须全部排在 close 之前。
- 本轮选择结束采集时同时停止该 capture 的 schedule/自动输入与新的触摸准入，冻结采集模型最后状态供显示；不自动重开 run，不将采集结束算作 endpoint 停摇事件。普通 Home/Show 仅按既定暂停／恢复语义处理，不能偷偷 close 后继续追加。reset/销毁走同一幂等 finalizer 做尽力收尾，前台显式 finish 是正式导出的必要操作。
- 使用真实 written 回执记录进度，主端不可把 sent 当 written。CLOSE_REQUESTED/PROGRESS/CLOSED/ERROR 均带 run/storeGeneration/requestId、两流计数与末端身份、路径；written 按两流分别维护，不能把一种消息未携带的计数重置为0。
- 只有两流最终计数与冻结 sent fence 严格相等、fsync/close 成功才 COMPLETE。错误不得空 catch；超时报告 DRAINING/失败并保留文件和计数，不提前 kill worker 或输出假 CLOSED。进程死亡前没有回执的 run 继续 PREFIX_ONLY。
- 完成后导出 manifest：两文件字节数/SHA256、raw 行数、run/route/identityRevision、最后键、最终计数与 close 结果。文件哈希可以在受控拉取后离线生成，不要求在 UI 线程同步遍历大文件。

此修复每次身份／计数更新 O(1)，空间 O(1)；原持久化仍为 O(记录字节数)。不顺手修改 fsync 频率、数据结构或截取 raw 快照来降成本；吞吐优化须另案确保可靠性。

## 6. T65 精确补证需求与缺失处理

T65 原卡不使用真机；本轮读取时尚无其交付报告。以下是事实／反例需求，整体根因仍由本文件负责：

| 项 | T65 离线应交 | 原始设备不可补出的部分 |
|---|---|---|
| 身份 | 原行号／哈希、完整键、sequence267七行、modeChangeCount边界；真实 WaterSandField 有hook／无hook／额外getter／模式切换对照，保留失败attempt/noop | P011_MODE 的 source、真实手势触发顺序；不存在就写 MISSING |
| 渲染 | 实际 P012 adapter 的schema4反例、当前合法／非法源；隔离版 controller 小数尺寸＋native未ready反例，说明 stub边界，不合成实际设备 receipt | 首次pack/prepare/draw/swap/合成token；tag过滤范围与PID。缺字段不能拿别的run补 |
| 生命周期 | 实际页面 hide/disappear 与 EntryAbility 事件调用链；真实 Store/Worker 在可控worker/fs边界跑 ready、append、short-write、delay、close、count mismatch、error；明确 ArkUI/worker/fs 哪些是替身 | Back是否destroy、close是否发出、worker积压量／真实fs时延，均未知 |
| 两流 | 27条同序左折叠每个sourceEpoch逐位核对，math.fsum仅独立诊断；meta/ledger route来源；原文件一条不删 | CLOSED最终records/admissions与尾部；不得以force-stop后可解析替代 |

T66 所需的最小离线反例是上述真实实现的 schema、序号、关闭计数／生命周期测试资产。T65 缺设备字段本身不要求再造一次 T44 历史现场，也不能让已经明确的缺陷无限等待。T66 整卡调度由 manager 按已到证据处理；T64 不修改它的 blocked 标签或任务状态。

## 7. T66 唯一最小修复任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容；仅按最新有限例外改自己行状态。不改滴答同名清单。多人共享代码，不撤回其他人的修改；本卡不承担真机端到端通过。

| 文件／责任 hunk | 最小改动 | 边界 |
|---|---|---|
| pages/SandCanvas.ets：dual渲染分支、XComponent测量/合成 | 接 T62 真实surface px入口；schema4 Canvas fallback、实际renderer/token/首拒日志；GPU/Canvas层互斥 | 只改产品双场入口；不改T59干场、EmptySandDemo或颜色方案 |
| render/P012WaterSandAdapter.ets、P012WaterSandRenderer.ets（必要时共用类型） | schema4原始源验证、明示拒因、同发布field/pool绑定；合法curr静态回退、tracer互斥 | 不放宽pair容差，不用GPU量化规则拒Canvas合法源 |
| render/P013RenderController.ets及types/native桥 | 先核对T62已完成能力；仅补产品接线实际缺失与结构化回执 | 禁止复制旧controller覆盖T62；只有已证实缺口才改native |
| physics/WaterSandField.ets：attempt/commit/metric身份 | §4.2，身份生成离开getter与hook；保留原物理计算/预算/失败记录 | 与T59共享文件按函数hunk协调，不动动力学与干场准入 |
| physics/P011EndpointMetrics.ets、P011EndpointCaptureSession.ets及必要观测类型 | 新身份revision兼容说明、单次登记与stamp同键；分开提交/诊断/导出序号 | 不改冻结endpoint/peak窗口、公式和eligibility；不把旧数据套新断言 |
| physics/P011NaturalObservationStore.ets、workers/P011NaturalObservationWorker.ets | fence/generation、written进度、严格计数/错误、幂等close和结果身份 | 不丢尾部、不提前terminate、不改fsync频率来“修好” |
| SandCanvas采集装配、EntryAbility/输入命令类型的必要hunk | 真实route metadata、前台单用途finish、销毁同一finalizer；Home语义明示 | 不借finish重置run/切mode/伪造停摇；普通输入兼容 |
| 对应开发离线自测及交付文档 | 消费T65反例，证明最小修复；编译/离线自测/commit身份 | T65维护其原始证据和分析脚本；不覆写effector报告、raw或冻结checker |

必须量化验证：

1. 同源合法 schema4：GPU成功时仅GPU画沙；GPU失败时Canvas床／真实池可见计数有据，B tracer=0；非法源仍拒绝并给具体原因，2/3旧入口不退化。
2. T44小数尺寸和native冷启动：显式prepare获得真实尺寸后才能提交；实际非均匀、失效epoch仍拒绝。合法F64与T62量化策略分支区分；不能统一关闭边界校验。
3. 相同输入物理提交序列，在有／无hook及任意额外getter读取下，ledger/endpoint/peak/metric的同提交键一致；新revision下attempt sequence严格增长、成功ordinal连续。失败attempt保留、noop不增加成功计数；模式切换仍真实影响资格。
4. finish 重复、旧generation、append-after-close、两流长积压/短写/错误场景均有确定结果；仅真实written与最终close计数相等才COMPLETE。计数或hash不一致不可变PASS。
5. 使用T44同27条原序数据逐位复算，不新增容差；与无改物理基线的同输入state/预算/事件比较，修复身份不能改变质量事务。

开发卡完成条件沿用户职责边界：开发完成、编译及适当离线自测通过、报告明确commit/文件/结果即可交付；设备复测另卡。编译／host桩成功不表示真实GPU或文件系统已通过。

## 8. 修复后真机复测与后续路由

由 manager 单独安排有“真机”标签的串行 tester/effector 卡；本轮不占设备、不建卡、不改他人状态。

- 先 R0 无外源、R1 五次实际600px/s滑动，保留原始输入时序；验证route激活、实际renderer与token、同commit画面、床与池、tracer互斥、源／LAND事务，不能只看“有沙”。
- 每臂用显式finish等待严格两流CLOSED，保存原始流/manifest/计数/hash；再独立测试Home/Show、Back/销毁。故意force-stop只作PREFIX_ONLY负例，不作完整run退出。
- R0/R1新装配证据齐后再执行R2同run停摇endpoint、R3冻结时序peak双报；保留旧／新口径、真实时钟、身份、外源和模式资格。不得用修复前缀拼新完整run。
- 如果实际draw/swap成功但画面仍无沙，转合成层/布局渲染返修；若同源渲染正常且特定粒子长期CONTACT_WAIT或LAND不闭合，再派逐粒事件补证和designer物理归因。当前不预开“大角度／湿沙动力学”修改来替代装配修复。
- 本任务交付并不改变T44的可见FAIL／采集合规BLOCKED，也不把T62离线通过当T44产品通过。

## 9. 交付登记

本轮完成独立失败分流、最小修复任务书、T65补证清单、原始前缀复算及独立复盘。已证实问题可实施；现场首拒因与实际收尾链仍是明确证据缺口。本轮只更新T64本行交付状态并回原卡，manager负责完成登记与后续调度，不增加验收前置。
