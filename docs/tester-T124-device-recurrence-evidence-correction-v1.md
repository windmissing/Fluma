# T124 设备复发证据纠正报告 v1

日期：2026-10-26。责任：tester。任务卡 `6adf424be4b03273ad863bfe`。不使用真机。
方法：只重组既有 T94 原始证据（`test/artifacts/t94-dualfield-stability-20261026/`，commit 52a0700 采集），不重跑设备、不整体归因。按 T116§1/§2/§5 逐项核对，对 T94 报告（effect-report-P011-T94-dualfield-stability-v1.md）作版本化勘误。

## 1. R1/R2 复发成立（PID 隔离核对通过）

| 臂 | 进程 PID | 冻结记录归属 | 栈 |
|---|---|---|---|
| R1 | 48057 | R1-appfreeze-192032.log `Pid:48057`；R1-freeze.txt 全部记录 48057（freeze_20261026192030_48057） | mergeOldestHot (P011ParticleHot.ets:929:69) ← editPoolHot:955 ← preparePatchHot:1030 ← applyInto (M2Sink:230) ← seed ← liftByShake |
| R2 | 48742 | R2R3-appfreeze-192151.log `Pid:48742`；R2-freeze.txt 全部记录 48742（freeze_20261026192148_48742） | 同上函数链 |

两臂 THREAD_BLOCK_3S 样本均落 T96 版 mergeOldestHot→seed→liftByShake→applyMotionChannels，与 T78 同函数链（源码行已变，非逐字节同栈）。**T96 等价短路未消除设备硬故障的结论在 R1/R2 成立**。

## 2. R3 证据纠正（勘误核心）

### 2.1 "R3 FREEZE 19:22:51"归属错误

R3 实际进程 PID **49203**（19:22:22.556 spawn）。核对：

- R3-freeze.txt 19:22:51.215 FreezeDetector 处理的事件是 `[AAFWK, THREAD_BLOCK_3S, 48742]`（**R2 进程**），merge 的是 freeze_20261026192148_48742.txt；全文件 `49203` 出现 **0 次**。
- R3-full.txt 中 49203 共 2138 行，**零条** THREAD_BLOCK / FreezeDetector / Faultlogger 记录。
- 执行器 Check-Alive 全局 grep 未过滤 PID/run，把上一臂 48742 的冻结记录当成 R3 判冻结。console "R3 FREEZE detected 19:23:14"同源。

**勘误 E1**：T94 §1 表格"R3 FREEZE（19:22:51）——零摇晃也卡死"及 §2"R3 shake=0 仍同栈卡死"**证据不成立**。R3 没有独立冻结首因证据；"三臂全复发"应改为"两臂复发（R1/R2），R3 未判"。

### 2.2 提前 CLOSED 已精确定位到事件级（比 T116 判断更进一步）

19:22:28.989 `ac: down`（ITK Id:628325，Button+LongPress+Pan 识别器）→ 19:22:29.065 `CLK RACC, T: Button`（点击命中）→ 19:22:29.066 `P011_OBS_STORE_CLOSE_REQUESTED {requestId:"ui:1", state:"DRAINING", sentRecords:264}` → 19:22:29.068 `P011_OBS_STORE_FENCE {requestId:"ui:1", state:"CLOSED", sent/written=264/0}` → 同毫秒 worker destroy（`host post nullptr to worker`）。

即：CLOSED 由**页面收尾按钮的一次真实点击**触发，时间 19:22:29.068，早于 R3 脚本 pan 序列（首个 pan_update 19:22:36.535）**约 7.5 秒**。R4 脚本 19:23:23 检索到的是这条历史 CLOSED，不是"进程死后新发"。

**勘误 E2**：T94 §1 B1"CLOSED 出现但进程已死"表述应改为"CLOSED 由 19:22:29.068 的 ui:1 按钮点击正常触发，早于 pan 序列；进程当时存活，后续未再冻结"。

### 2.3 R3 零源机制：收尾先于手势，物理已被停

- WS_FRAME 最后一条 19:22:29.062（sequence 264 commit），FENCE 后 0 帧：captureFinished 生效，物理停机。
- 之后 pan_update 497 条（19:22:36.535–44.645），其中 accepted=true 424 条——但收尾后 stageGestureSandAndFeed 跳过 inject，ledger sourceEpoch 全 0、particles.count 全 0、capacityUsed 全 0、264 行全 committed。
- pan 期间无任何 WS_FRAME/P011_METRIC 输出，pan_end 19:22:44.645 后进程除 GC/一条 vsync 外静默。

**勘误 E3**："R3 零源与提前收尾一致"确认为**候选原因成立且事件级时序闭合**（收尾点击→FENCE→物理停→后续 pan 全部跳过注入）。**"手势注入事务的 seed 路径进 merge 全扫"在 R3 无证据**——R3 的 pan 发生在物理停止之后，从未进入 seed/merge。触发面收窄的表述（T94 §2）仅对 R1/R2 的 shake-lift 路径有栈证据，gesture 注入路径未证实。

## 3. 真实负载缺项清单（按 T116§2）

| 缺项 | 现状 |
|---|---|
| 冻结时刻实际 N / capacityUsed | R3 ledger 明记 **capacityLimit=512**（非报告假定的 192），capacityUsed=0（窗口全零）；R1/R2 冻结时刻无任何 ledger（FENCE 只在 R3 有且提前关闭） |
| pool snapshot（冻结池状态） | 无。R3 ledger 仅 264 行已提交窗口，不含满池/merge 调用时刻 |
| birth / seed / B / λ 尝试计数 | 无。POOL_PROGRESS 未开启/未归档 |
| mergeCandidateChecks 实际值 | 无。T96 报告 N512=2,097,152 是 8 次 λ 重试累计，非单次 |
| 阶段 wall time（copy/collect/apply/merge） | 无 |
| "设备慢 10 倍"系数 | 系 T97 host 2.1s vs 设备 3s 阈值对比的推算，**非实测**，不得当事实引用 |

**禁止**：用模拟数据补旧 run、把 host 系数当设备事实、以此构造"精确重放"。

## 4. 交回 T116 的后续设备计数需求

1. 首超长 attempt 只导出**固定预算计数**（实际 N/active/B/seed/λ 序号、各阶段耗时、入口/出口进度），诊断不使 UI 多打无界数组（T116§5 已定方向，本卡确认缺口真实存在）。
2. 采集时序须覆盖 R3 型条件：**先 pan 后收尾**，或显式禁用收尾按钮，避免 FENCE 提前停物理污染样本。
3. R1/R2 型复发臂须逐臂独立抓取带 PID 的 freeze 记录（Check-Alive 需按 PID/run 过滤）。
4. capacity 语义须澄清：ledger 记 512、T94 报告假 192，设备补采前不得用任一值代入复杂度公式出结论。

## 5. 判定

| 项 | 结果 |
|---|---|
| R1/R2 复发（PID 隔离后） | 成立，栈同 T78 函数链 |
| R3 独立冻结 | **不成立**（证据全属 48742） |
| 提前 CLOSED 机制 | 事件级闭合：ui:1 按钮 click → DRAINING → FENCE，19:22:29.068，早于 pan 7.5s |
| R3 零源 | 与提前收尾一致，已闭合；非"seed 吞注入" |
| 真实负载数据 | 全部缺失，复杂度/唯一首因仍不可判 |

本卡为证据重组与勘误，不做归因结论、不改判据、不重跑设备。原始产物路径不变（`test/artifacts/t94-dualfield-stability-20261026/`），旧 T94 报告保留，以本报告 v1 勘误为准。
