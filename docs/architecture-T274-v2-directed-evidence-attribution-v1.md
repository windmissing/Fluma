# T274 V2 落床续证阻断首次归因 · 工程修订 v1

2026-10-29，architect；不使用真机。消费 T267 报告、原卡评论、`test/reports/t267/t267_tail.txt` 300 行、T266-v9 生成源码与 T243 v2。T267 是执行 BLOCKED，DD12 产品仍未通过。以下区分已归档设备事实、报告中无原件的多 run 摘要和源码推断；不改物理 dt、阈值或 shader。

## 1. 首生计时“冻结”不成立

T266-v9 `T266Clock.source()` 首次正量出生时只冻结 `birthClock=acceptedPhysicsSeconds`；`stepEvidence()` 每成功子步累加 `acceptedPhysicsSeconds`；`secondsSinceFirstBirth` 与 `summary().acceptedPhysicsSecondsSinceFirstBirth` 均实时计算差值。离线自测覆盖首次出生后的差值增长。`P051DerivedDemoV2.tick()` 每帧调用 `stepEvidence()`，但 `T266_SOURCE` 只在**源请求**时输出；源停后不输出新 `T266_SOURCE`，不能把最后一条源日志的 since 值当后续 step 时钟。无需改为另一种累加法，也不能用跨 run 总秒替代。

原 300 行全属 pid62459、run `p011-run-1793266737729-3`、epoch2 的末尾窗口。18 条 `T266_SOURCE` 中同一 run 的 since 从 0、0.03、…递增到约 0.57 秒；末条总物理秒约 2.6667，首生基线约 2.1 秒、`firstBirthCommit=129`、准入量 50.3577、拒绝数1。末条 `T248_STEP` 提交 commit193，旧观察累计约 2.9 秒；按同 run 已见 step 与首生基线，末尾至多支持约 0.8 秒首生后成功模拟，**未到 8 秒**。此窗口并未显示冻结。报告声称另三 run 总秒 28.57/15.7/9.93 和 firstBirthCommit 387/1669/929，但归档只有上述 run 的 tail，缺各 run 首生基线、同 run 最终 clock、源/step 序列，不能从总秒反推出首生后秒数或核多 run 四个固定值。报告“≥8 总秒达标”是累计总秒口径，非 DD12 首生后锚点。

71 条 `T266_FRAME` 日志每条在 4165 字符处截断，末尾 `runClock/boundedTrace/nativeStages` 不在可解析内容内；日志中 `acceptedPhysicsSecondsSinceFirstBirth` 仅出现在 18 条 `T266_SOURCE`。生成器把巨型 authority/publication/receipt 放在 runClock 前，超出设备 hilog 单条上限。正常/重日志两臂本应有界，实际采集格式使后半截关键证据系统性丢失，属于**诊断导出工程问题**。`t266AnchorFrames` 在模型首次非零床或 since≥8 时设4并由页面逐帧递减的源码逻辑存在；是否在报告提及的其他 run 触发，当前原件无法验证。

## 2. 床量与运动可证范围

归档 tail 内的 `T266_FRAME` 前段包含同 run authority 观测：初期空池；末帧约 commit193、512 个活动粒、`minY≈149.39`，比前几帧约150.42下降，显示本窗口活动粒仍运动且未触底。观测的 `bh/bl/mh/ml` 为零、`landEvents=NOT_CAPTURED_USE_WRITER_HOOK`、`pixel=NOT_CAPTURED_USE_NATIVE_SAME_TOKEN_HOOK`。这些只证明**此窗口被采样的权威床量为零**；最近 `T248_LAND events=[]` 不是全 run LAND 否定，LAND writer/pixel 根本未采。其他 run 的“床全0”、真实触底速度或数值唯一瓶颈缺完整原件，不从同现象推算法缺陷。0 床量时没有床像素可验，也不能派床层 shader 返修。

## 3. 给 developer 的有界工程修订

在隔离 `test/t266/generate.cjs` 与生成的 `P051DerivedV2Demo.ets`/`pages/P051DerivedV2Demo.ets`、`T266Clock.ets` 修诊断出口；生产物理/默认路由不改。每个 run 的首生及每个 60 帧/8秒锚点先输出一条**小于设备单条上限**的标量 `T266_CLOCK_V3`：runId、stateEpoch、frameSerial、physicsCommit、firstBirthCommit、firstBirthAcceptedSeconds、acceptedPhysicsSeconds、sinceFirstBirth、committedSubstepCount、sourceRequestedSeconds、admittedMass、rejectedCount、step/source dropped、clock revision、anchor reason。第一出生前 since 用 null，不填0。源事件另报请求 sec、admitted/rejected，不能把它作为模拟秒。每条日志携带有界 `partIndex/partCount/byteCount` 与内容摘要；长 authority/profile/receipt/trace 拆成带同 token 的冷档/分片且显式检测截断，单个 JSON > 上限不发送。日志开销计入 previous log cost，正常臂禁逐帧大 JSON。

锚点导出须冻结同一 run/epoch/commit 的 clock、权威床四量/粒状态和 y/vy 极值、publication/packed token、完整新管线 receipt；LAND writer hook 只在实际已提交事件时写定容 `(run,epoch,substep,commit,particle,fromState,toState,bedDelta,firstCode)`，环满报 dropped，不能从最近页 `[]` 推全历史。首次床量非零时及其后3帧，另给显式真机同 token 像素采集接点；无设备采集仍标 NOT_CAPTURED。若 `firstBirthCommit` 与源成功 commit 不是同一身份，拒锚点并记录首个不一致，不能跨 run 拼。复制/导出失败 `INCOMPLETE` 或 `NOT_CAPTURED`，不将旧新管线帧标 CURRENT；渲染背景/前景仍只 XComponent+shader，失败 FAULT 或完整旧 token 显式 STALE，禁 Canvas。

非机回归：同 run 首生前后 0→正 since，源停止后继续成功 step 仍增长；两个 run/epoch 重置；部分成功子步前缀；长日志分片可重组/缺片被发现；诊断开关下 authority/receipt/commit/首码一致。编译签包和非机结果是 developer 交付，不是产品落床 PASS。既有 GPU 新效果优先，本卡只修既有 V2 取证。

## 4. 后续定向补证与分流

tester 仅在上述修订签包及 SHA 到件后，以**一个新 run**收齐首次出生基线和后续同 run 正成功物理秒≥8 的 `T266_CLOCK_V3`，每条与 commit/stepRows 对上；源停后仍取 clock。若设备休眠使锚点不可达，原样归档最后成功秒和原因，记 BLOCKED。达标后取 writer LAND、床四量、publication→packed→同 token 后3个 CURRENT 与像素；若权威床仍0，提交连续 y/vy/state/首码与阶段成本给 architect 判断运动/设计/工程；若权威床正而下游缺失，才分 publication/packing/shader。多 run 不能合并秒；缺设备原件需精确补证，不重跑 T218 全矩阵。T267 报告的“计时冻结需修法”应勘误为**源日志停止及帧日志截断导致观测不足**，并保留原报告而非改写历史。

## 5. T277 回传后工程修订 v2（2026-10-29）

T277 真机包 `T276-v5-signed.hap` SHA `e9c0b37c66b6013849f12292d2bc3b2b128fe98e4f522d79447e9d6361885380`。`test/reports/t277/reassembled{,2}/index.json` 均为 `INCOMPLETE`、`invalidPhysicalLines=0`、`clocks=[]`；其中若干 `T248_LAND/T248_STEP/PACK/T266_FRAME` 单项 `COMPLETE` 有独立 SHA，带源主档缺片。完整帧36/37 `firstBirthCommit=null`，不可转述为带源锚点；PACK 床四量为0仅限这些无源期帧。电量3%、USB慢充与进程停顿支持设备窗口不稳定，**尚不足证明低电 Doze 是唯一根因**；本轮没有同 run 首生后≥8成功物理秒，也未触发 LAND→床→同 token 像素链。

T276 的 `T266Clock.ets` 已用 `acceptedPhysicsSeconds-birthClock` 差值并在 `bind()` 重置；这与 v1 核定语义一致，原“T267 since 冻结”缺陷不能继续当已证物理/计时 bug。T277 未取得出生后钟值，因此真机未验证长时增长，结论为**源码合同已修/早期运行有证，DD12 长时行为未证**。

另有确定的**取证工程问题**：`test/t266/generate.cjs` 把 `t266Heavy=true` 直接并入每帧 `t266Anchor`；T277 正以 `--ps t266heavy true` 运行。`test/t276/generate.cjs` 虽将单条日志缩到≤1800字符，却对每个重日志帧同步导出完整 `T266_FRAME`；已完整的单帧档约 1.87MB、7314 分片，带源帧缺片。逐条有界不等于**每帧总量/主线程耗时有界**，会与16.7/33.3ms窗口竞争并放大低电停顿。不能仅以充电重跑同一重日志流程作为充分接力，也不能据此宣它是所有进程冻结的唯一原因。

### 5.1 给 developer 的诊断出口修订

仅改隔离 `test/t266/generate.cjs`、`test/t276/generate.cjs`/`T276Export.ets` 及生成的诊断页面，生产物理和默认路由不动。`t266Heavy` 只开放**冷采能力**，不再使 `t266Anchor` 每帧为真；第一笔正量出生时先输出单条 `T266_CLOCK_V3(FIRST_BIRTH)`，并指定一个后续 frame 的有界锚点，首床及 `sinceFirstBirth≥8` 再各指定最多数帧。允许明确 `captureFrame` 一次性选择同 run/frame/token；普通帧只写定长 clock、commit/首码/dropped 与成本，不自动导出巨型 authority。每 run/锚点设可核的总字节、分片数、主线程时间和队列容量预算，超限标 `INCOMPLETE/BUDGET`，不得一帧同步打印几千片后称正常臂；冷快照与重组可在空闲/停止后分批完成，保留冻结时同 token 的身份/哈希，不从后续 canonical 伪造旧帧。失败首码和物理成功前缀不可因诊断 IO 反写。给同一锚点的 scalar clock→LAND writer→床/粒状态→publication/packed→native pixel 保序索引，缺片如实 `MISSING_PART`。

非机回归至少覆盖重日志开关下非锚点0个大导出、首生/首床/8秒只触发约定次数、手选帧一次、总预算溢出显式不完整、导出推迟后仍为原 token/commit、诊断开关前后物理位型与提交相同，并编译签包。此修订先交 developer 后再给 tester；不改 dt/阈值/水沙物理，不引入 Canvas。若现有同步架构无法冷封快照并延后导出，developer 须回报具体生命周期/所有权限制，交 architect 版本化裁定，不能以热逐帧大日志凑证据。

### 5.2 设备续证判线

待稳定供电与修订包到件，tester 先核两 index 的完整性与新有界预算，再在一个新 run 取 FIRST_BIRTH、连续成功 step 至 `sinceFirstBirth≥8` 的小钟行，源停后继续 clock；重组仅指定锚点的完整档，分别核 LAND writer/床四量、publication/packed、同 token 三 CURRENT/像素。若仍床0，交连续 y/vy/state/首码与成本回 architect；若床正而下游丢，才派具体工程层。真机电量/亮屏/调度停止时间线随证据归档，不能用充电、跨run总秒或无源期 COMPLETE 档替代产品结果。设备窗口等待不列实质阻塞，T277 执行报告已结束，续证需 manager 另卡接力。
