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
