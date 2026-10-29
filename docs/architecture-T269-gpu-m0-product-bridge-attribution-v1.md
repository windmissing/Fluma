# T269 GPU M0 产品桥接失败首次归因 · 工程修订 v1

2026-10-29，architect，原卡 `6ae30629e4b0c3103c250489`；不使用真机。消费 `test-report-T264-gpu-m0-product-device-v1.md`、`test/reports/t264/`、`developer-T236-app-route-v5.md`、T230/T233/T259 和当前 `gpu_resident`/页面源码。此稿补充 `architecture-T259-gpu-m0-id-window-handoff-v1.md` §2 与 `architecture-T233-gpu-resident-M0-v1.md` §4，仅修实验产品桥接/诊断，不变更 `GRS_FIXED120_TIME_V1`、sourceCommand32/completion256 wire、120Hz、F≤4 或守恒/事务/审计门。K192 产品最小闭环先行；T263 独立与 T240 M1 在途 hunk 不回退。

## 1. 设备事实、源码事实与未证假设

T264 包 SHA256 `18e4895bd46de6974242df588b36789cb9cc2cb2d5ad7483a37111755248b1cc` 的 `pages/GpuResidentM0`/`libgrsm0.so` 在真机加载。`t264a–h.json` 可复查两组 `STATE_LOST→FAULT/ADVANCE` UI 状态，`t264_grs.txt` **仅归档一条** pid12348 于15:54:00 的 `GRS_SOURCE INVALID_COMMAND`；报告所称 pid16078 第二条在这份归档里未找到。原始 JSON 是采样状态，不含每次 N-API 入口返回、`ControlCode`、`deltaNs`、owner epoch/队列深度或 sourceCommand32 原 words。`t264_suspend.txt` 的 `Doze has special:ERR_IN_STATE_FOREGROUND ... DOZE_BY_STOP_LIFECYCLE_DETECT_FOR_APP_FOREGROUND` 不能单独推出该会话某次 `Advance` 之前的 delta 超限。当前没有 completion/lease/swap/收益数据；产品 M0 仍 FAIL/NOT_RUN。

`GpuResidentM0.source()` 确实构造 `ArrayBuffer(128)`+`Uint32Array(32)`，用 `DataView` 小端写 x/y/volume，`offer(w)`；仅说明调用方意图，不证明真机 N-API 所见对象类别/元素数。`app_bridge.cpp::offer()` 把四类失败合并成字符串 `INVALID_COMMAND`：argc≠1、`napi_get_typedarray_info` 非 ok、type≠`napi_uint32_array`、length≠32。归档无该分支及原始 `napi_status/type/length/byteOffset/backingBytes`，故不能认定“ArkTS Uint32Array 的 enum 值异常”为唯一根因，也不能放宽长度、猜 enum 数值或截断 packet。

`app_bridge.cpp::loop()` 在 `!run->Tick().active` 时用 `steady_clock now-last` 调 `Advance`；`last` 只在调用前更新，GPU root 在飞期间未更新，故下一次 delta 包含整个等待间隔。它把**任何**非 OK `ControlCode` 无差别设 `failed=true`，状态仅写 `FAULT/ADVANCE`，其后同 epoch 不再 pump/恢复。`ResidentRun::Advance` 的可能非 OK 包括 BUSY（inflight/quarantine）、帧/控制/角/容量输入非法，或 `Fixed120Clock::Plan` 超 33,333,333ns；`Plan` 超窗会 `Pause()` 清 carry/first，随后 `Advance` 清 run 内队列并 `transport.Stop()`。本次实际首码、delta/carry/队列状态未归档，故“Doze→delta 超限”仍为源码可行路径，非已证复现原因。**无差别永久熔断、缺错误字段和非活动间隔混入下一 delta 是已证工程接线偏离**；不需等设备先修。

`GpuResidentM0` 仅在 status JSON 含 `run/epoch` 时更新页面缓存；`STATE_LOST` 不清旧缓存。页面 sourceCommand32 的 `inputLo=1`、`runHi/inputHi/geometryHi=0` 写死，`SourceGpuTransport::Stop()` 却递增输入代；在同 run 暂停或旧代场景可发送陈旧命令。当前小 run 值未证明它导致 T264 的 `INVALID_COMMAND`，但完整 32-word 身份接线必须修；不得把 UI 缓存的浮点 JSON 单个 run 数当完整 U64 身份。`app_bridge` 第二层 FIFO16 调 `ResidentRun::Offer` 失败时保留条目却不对页面给原因；N-API 成功字符串 `ENQUEUED_NOT_ACK` 本来也不是 GPU 准入/源终态。

## 2. 版本化工程修订：唯一 owner、命令与可观测性

**唯一 GL/ResidentRun owner 线程**维持现有 `App::loop`；XComponent callback/UI 仅交有界消息，不同步进 GL 或等待 fence。只改实验 `entry/src/main/cpp/gpu_resident/app_bridge.cpp`、`entry/src/main/ets/pages/GpuResidentM0.ets`、相邻 `libgrsm0` 类型声明与必要非机 bridge 测试；`control.cpp` 的 `Fixed120Clock::Plan` 判线和 `run.cpp` 事务根不改。与 T240/T263 对共享 `gpu_resident` hunk 单 owner 串行协调；不拿独立 probe 当产品通过。

N-API `offer` 首先逐项查 `napi_get_cb_info`、argc、对象类型和实际长度，记录**首次**失败项、原 `napi_status`、typedarray enum/length/offset、backing buffer 长度及是否 detached（若 API 可得）。成功路径只接受**精确 128 字节**的 Uint32Array(32) 全视图，或页面明确传入的 ArrayBuffer(128)；后一分支用 `napi_get_arraybuffer_info` 完整读 128 字节，原 32×U32 位型不变，不将任意 JS array/Float64/view 子片/多余字节当合法命令。页面可优先 `offer(b)` 走 ArrayBuffer 明确路径，是否可在本机 ABI 与真机成立由测试证明；不能凭推测删除旧类型检查。复制至 app FIFO 前核 byte length、buffer/offset/非 null、当前状态/epoch，并以生成的 sourceCommand32 offset/manifest 校验 magic/revision/reserved、完整 run/request/input/geometry/config U64、有限 raw F64 x/y/volume 位型及其余必需槽；非法返回分阶段拒绝，绝不部分入队或替换字段。`ENQUEUED_NOT_ACK` 只表示 app FIFO 接受，`ResidentRun::Offer` 成功也仍非 GPU receipt；最终 sourceSuccess 必须来自产品 completion256。诊断冷导出可存完整32 word，热日志只存定长 reason/首错和身份，不每帧 dump 大 JSON。

状态 JSON/窄 trace 增 `runLo/Hi, owner/context/surface/geometryLo/Hi, inputGenerationLo/Hi, epoch, appQueueDepth, runQueueDepth, lastOfferCode, advanceCode, clockDeltaNs, carryBefore/After, plannedQuanta, clockPhase, firstFailureStage, rootNonce, acceptedCount, source/physicsCommit` 等**已知值**；U64 以两 U32 传 ArkTS，页面按生成布局逐 word 写入，不用 JS Number 承载超过安全整数的完整 ID。每个返回码带 stage/enum 名和原整数码；`INVALID_COMMAND` 一定标记分支。trace 环固定容量、满时报 dropped/incomplete，冷归档签包 SHA 和 owner/epoch；不改 completion256、reserved 或源 row。页面仅在当前 epoch `READY` 且收到完整身份快照后启用发源，`STATE_LOST/PAUSED/FAULT` 立即清缓存，重新 READY 后用新 inputGeneration 和新 gesture/request 序列；旧请求保持原字节用于迟到/重放判定，不能在桥内悄悄改 inputGeneration。

## 3. 时间与生命周期修订：不钳 delta，不丢 carry

`Fixed120Clock` 的 >33,333,333ns 拒绝、F≤4、`a=carry+120×deltaNs`、dt0严格无写、first positive tick 仅建立锚仍保持。**禁止** `min(delta,33333333)`、拆大墙钟为多帧补跑、忽略一段 delta 却保留旧 carry、改变 h=1/120 或把 INVALID_INPUT 当成功 quantum。bridge 对 onPageHide/Ability background/window change/destroy/context loss 先递交带 epoch 的控制消息；owner 将旧输入与旧窗口代隔离，已接受 bank 保持真实历史，inflight candidate tombstone/quarantine，释放 fence/lease 后按 T259 冷建新空 run（不能从摘要恢复旧池）。恢复首个正 tick 只建立时钟锚，设备不以后台墙钟追赶。

同 epoch 运行中若调度/GPU 在飞使下次 `now-last` 超窗，bridge 在调用 `Advance` **之前**标记 `CLOCK_GAP`、原 delta/carry/旧 active/root 身份，执行已定 `Pause` 边界：清 app 与 run 尚未准入的输入、transport inputGeneration 增代、carry 归零；已接受 root 不回滚，未接受 root 等非阻塞 quarantine 退休。安全退休后可在**同一有效 context、同一 accepted bank** 建立 `PAUSED_CLOCK→READY` 新输入代/新时钟锚；若 owner/lease/context 已失效或安全退休无法证明，则显式 `FAULT_CLOCK_GAP/NEW_RUN_REQUIRED`，经用户现有 New empty run 冷启，不暗中丢掉已接受库存。页面拿到新代后才允许新 DOWN/命令；丢弃/取消的 app FIFO 项计数并回报，不伪造 GPU terminal。`last` 在 epoch/pause/rebase 时重新锚定，不把后台或失活等待算作 active delta。新主动帧若其他条件返回 BUSY/BACKPRESSURE，保持队列和已接受状态、下一 loop 有界重试；真正非法/状态丢失保留首码并 FAULT，不以统一 `ADVANCE` 永久糊住错误。若实现上述同 run 安全恢复接点与现有 `ResidentRun::Pause/Pump` 状态冲突，应先精确指出无法保持的不变量交 architect/manager，再决定是否需 designer 修时间语义；developer 不自行放宽。

`New empty run` 明确请求新 epoch，先标旧 epoch `STATE_LOST`、撤销页面缓存、退休旧 GL owner 后冷建新的 run/clock/FIFO/输入代；setActive(true) 在同生命周期被重复调用也须可辨重复、不能无提示覆盖在途根。READY、PAUSED_CLOCK、FAULT、STATE_LOST、CURRENT/STALE 的含义与源 ACK/physics commit 分列，不能用 UI READY 或 source enqueue 冒产品帧成功。渲染仍为新 XComponent+shader，含背景/前景；失败只 FAULT 或完整旧 token 的显式 STALE，绝无 Canvas 上屏/回退。

## 4. 开发与定点测试接力

developer 离线对真实 `libgrsm0` N-API 函数做 128B ArrayBuffer 与 Uint32Array 正例、错误 argc/type/31或33 word/偏移/detach/非法 header及全 32 word 逐位复制；用相同生成 manifest 构造真实 source packet，证实桥层入 FIFO 后仍等 GPU completion 才 ACK。注入正常 8ms、>33,333,333ns、GPU inflight 后间隔、pause/new run/epoch 变更/两层 FIFO 背压/late fence；核首码、carry、inputGeneration、拒绝/已接受计数与不补跑。真实产品代码编译/非机测试与可部署签包 SHA 为开发交付；不可把 host stub 当真机或完成 GR01–09。若必须改 `control.cpp/run.cpp` 公共状态机，先在本稿增版本与不变量核对，再串行协调 T240/T263。

tester 在修复签包上定点真机：先取每次 `offer` 的 N-API 分支/完整 packet 身份和 app→run FIFO 回执，再取 `Advance` 原码/delta/carry/phase 与 pause/rebase/epoch，**同一 K192 新 run**完成 source terminal→至少一个成功 quantum/completion256→lease→同 token shader/swap/像素。分别注入稳定前台与一次真实暂停/恢复，先保留诊断原序列；能复现 T264 时按首码分流，不按 Doze 日志猜 delta。随后完整故障矩阵、K512/末 ID/收益沿既有真实前置另测，绝不重跑旧独立 probe 充产品门。若设备仍仅给 `INVALID_COMMAND`/`FAULT/ADVANCE` 而无首码/参数，则是精确仪表缺口，先回 architect，不盲改时钟或换数字类型。

## 5. T273 真机回传后工程修订 v2（2026-10-29）

T273 用 `T272-signed.hap` SHA `7a5644a311168f423aba1caf4b039cef89e7606e24229167987a5d0f481e7ed6`。`t273d.json` 的 128B ArrayBuffer、`napiStage=OK`、`lastOfferCode=ENQUEUED_NOT_ACK`、`appQueueDepth=1` 和 hilog 对应，说明旧 N-API 入口拒绝已越过；`t273e/g/j.json` 的原始 `clockDeltaNs` 分别约 192/167/210ms，`advanceCode=6`、`carryAfter=0`、`NEW_RUN_REQUIRED`，说明超窗转显式 pause 的设备行为。`acceptedCount=0`，故本次**没有**验证已接受库存的保留语义。source terminal、quantum/completion256、publication lease 和 CURRENT 均未执行，产品 M0 仍未通过。`runQueueDepth` 在 pause 后可残留 1，是最后一次采样值，不能当 pause 后真实队列仍有待提交 root。

原报告“READY 从未观察到”需勘误：`t273b/c/d.json` 的状态串为 `STATE_LOST`，但同一快照 `ready=true`、`clockPhase=READY`；`t273d` 还成功入 app FIFO。**这不是自洽的 READY 证据，也不是从未 ready**。源码 `app_bridge.cpp::loop` 在 EGL/transport 创建后单独锁内写 `identity/input/ready=true/phase=READY`，再经另一次 `publish(READY)` 写状态串；期间 `invalidate()` 可增 epoch 并写 `STATE_LOST/ready=false`，而创建线程未在 ready 写入处重新核 `seen==epoch`。于是旧 epoch 的创建完成可写回 `ready=true`，`publish()` 因 epoch 不同拒绝旧 READY 字符串，留下本次快照的矛盾组合；在下一次 owner loop 回收前，`offer()` 只核 `ready` 与 packet 身份，存在旧 run/旧 epoch 入队窗口。原件与源码证明**状态发布/epoch 门存在工程缺口**；该交错是否造成 T273 的每一次 CLOCK_GAP，现有采样不能断言。

`ps` 相隔 5s CPU TIME 不变，结合多次 138–215ms 超窗和资源调度 Doze 日志，支持“进程运行/调度停顿且时钟窗口失守”；`Doze has special:ERR_IN_STATE_FOREGROUND` 本身不证明每个超窗都由 cgroup freezer 唯一造成，更不能排除 UI/GL/诊断或电量因素。保持 `GRS_FIXED120_TIME_V1` 的 33,333,333ns 上限、120Hz、F≤4、无补跑/钳值；设备持续停顿时显式 `NEW_RUN_REQUIRED` 是合理失败，不应伪恢复或改变物理合同。

### 5.1 给 developer 的有界修订

在实验 bridge 而非物理核修原子发布：每次异步 EGL/transport/owner 初始化后、写任何共享 `identity/input/ready/status` 前，于同一锁内重新核 `seen==epoch && active && held window 仍属当前 surface`。不匹配则只退休本次局部资源，不写当前 epoch 状态；匹配时一次写完整身份、`readyEpoch`、`phase=READY` 和 `status=READY`，消除两锁间的观察窗口。`invalidate()` 在同一锁内撤销 ready/readyEpoch、旧消息队列和状态串；`offer()` 只允许 `status=READY && readyEpoch==epoch`，packet 与队列项绑定同 epoch，worker 消费前再核 epoch/输入代，不让旧初始化/旧 FIFO 穿透。`status()` 从同一状态快照序列化，不拼旧 `state` 与新 `ready`；保留有限首错/旧代拒绝/取消计数。对 `runQueueDepth` 定义采样时点或在 Pause 后刷新，避免旧值作现况。

非机竞争用例应阻断在 EGL/transport 建立完成与 READY 发布之间插入 `invalidate/newRun/surfaceChange`，核绝无 `STATE_LOST+ready=true`、旧包 `ENQUEUED_NOT_ACK`、旧 epoch 恢复 CURRENT；正常入口和 CLOCK_GAP 原测试继续。真实构建与非机自测即可交 developer 卡；需要新签包和设备证据的部分另由 tester 执行。公共 owner/scene hunk 与 T279–T281 串行协调，始终仅 XComponent+shader，失败 FAULT 或完整旧 token 显式 STALE。

### 5.2 给 tester 的条件与判线

工程修订包后，在稳定供电、亮屏且应用实际保持前台的窗口，归档显示/电量/进程 CPU TIME 与每次 loop wake、active delta、offer/owner 队列/epoch 原码；先核 READY 快照内部一致，再同一 K192 run 收 source terminal→成功 quantum/completion256→lease→同 token CURRENT/像素。若仍 CLOCK_GAP，保留 firstFailure/active 时间线和是否发生诊断输出，不把 `ENQUEUED_NOT_ACK` 当 GPU ACK；调度停顿无法消除时只报环境阻断，并提出可复现设备条件。T273 原卡执行已关闭，后续定点续证需另卡。完整矩阵、收益和 K512 仍未通过。
