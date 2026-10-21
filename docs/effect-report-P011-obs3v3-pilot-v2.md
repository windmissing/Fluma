# P-011 观测③ v3 pilot 重跑报告：播放器链路验证通过，metadata 武装缺口阻断（SCHEDULE_METADATA_NOT_ARMED）

- **日期**：2026-10-21
- **执行**：effector
- **任务**：v3 单 run clean pilot 重跑（d4aa997 播放器交付后解锁，任务 6ad82794）
- **被测构建**：d4aa997（`entry-default-s104-signed.hap` SHA256 `79cbf526…7c51`，干净 worktree 重建签名）
- **结论**：**播放器命令链路端到端验证通过（1513 条全部落账、九平台八 leg 齐全、drift 字段工作）；但 schedule 模式缺少 run metadata 武装路径——全部 5939 条 ledger 记录 `runRole=null`/`executedThresholds=null`，状态机未驱动。正式 clean pilot 不能执行（SCHEDULE_METADATA_NOT_ARMED），停机上报 developer/manager**

---

## 1. 重启前置（按任务书）

| 项 | 状态 |
|---|---|
| A_selected=18.7° 复用 | ✅（影子审计证据完整 + 12:5x 复测逐位一致） |
| 出包 d4aa997 | ✅ s104 / `79cbf52602b3f58563da3877e3f2b66209ed3d74f9b6b2555bb199c323b57c51` |
| 冻结 CSV 一致性 | ✅ 播放器回放流与 `schedule-obs3v3-A18.7-rate0.5.csv`（7b59b2e4…bd46）逐条一致（developer 单测 `replay stream matches the frozen CSV row-for-row` + 本次真机 segment 序列核对） |
| 冻结 schedule SHA256 | 7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46（未变） |

## 2. 诊断性 smoke run（正式 pilot 前的执行链验证）

正式 pilot 前发现代码层疑点：schedule 模式的 run metadata 武装路径缺失（见 §3）。为避免整 run 报废，先执行了一次**诊断性 smoke**（非正式 pilot，不占用 run 编号、不产生 fit/holdout 数据、无 CLEAN 期望）：

- want：纯 `p011schedule=18.7:0.5`（无 wstilt），真机 62T0225B10010020，240s 覆盖 196.3s 全时序
- runId：`p011-run-1792565357992-3`（诊断标记，非 pilot）

## 3. Smoke 结果

### 3.1 播放器链路 ✅ 全部通过

| 检查项 | 结果 |
|---|---|
| 命令完整性 | ledger 覆盖 commandSequence 1..1513（1214 个有子步消化，慢帧合并符合 §26.4 风险 3 预期） |
| segment 绑定 | H0..H8 + L1..L8 **全部出现**且与 schedule 一致 |
| drift 字段 | 5939/5939 记录携带 scheduleDriftMs：**P50=57ms / P95=86ms / max=99ms**；>50ms 标记 3722 条 |
| 互斥 | 无单命令混入（纯 schedule want） |

**节拍判定**：drift 稳定在 57~99ms 区间（无追赶、无爆炸增长）——这是 `只记录不追赶` 语义下的正常表现。**注意：P50=57ms 已超 50ms 标记线，>50ms 占比 62.7%**，drift 消费口径（哪些命令失格）归 tester 判定域（§26.4 决策 2），本报告只交数据。

### 3.2 metadata 武装 ❌ 缺口确认

**全部 5939 条记录**：
- `runRole = null`、`thresholdCalibrationId = null`、`thetaStartDeg = null`、`thetaStopDeg = null`
- `executedThresholds = null`（**状态机未武装，全程 LEGACY 路径**）
- legacy 单阈值语义下产生 3238 个转换（H8  alone 2342 个）——这些**不是候选观测**，与影子审计（26/20 机器驱动）的预测分布完全不同

**根因（代码路径分析）**：
- run metadata 唯一设置点在 `SandCanvas.ets:1438` 的 `freshScriptCommand` 路径——该路径要求 **wstilt 单命令被消费**
- 纯 schedule want（无 wstilt）→ metadata 永远为 null → `relaxHysteresisEnabled()=false`
- 若同 want 带 wstilt：§26.4 决策 1 互斥——帧循环先消费 schedule want（启动播放器），同帧 tilt 命令被 `SCHEDULE_ACTIVE_COMMAND_REJECTED` 拒绝；且单命令 held 语义（scriptTargetTilt 持续保持）使后续 schedule want 也被拒——**两条路都到不了 metadata**

**后果**：正式 pilot 若照跑，v3 checker 判 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`（executedThresholds=null vs configured 26/20），run 必 INVALID。**不跑这趟废 run，直接停机**。

## 4. 判定

| 项 | 状态 |
|---|---|
| 播放器命令链路（含 CSV 一致性/drift/segment） | ✅ 真机验证通过 |
| **run metadata 武装** | ❌ **SCHEDULE_METADATA_NOT_ARMED** |
| 正式 clean pilot | **未执行**（避免必然 INVALID 的废 run） |
| thresholdVerdict | NOT_EVALUATED |

## 5. 移交

**developer（小改动，一处）**：schedule want 消费路径（`consumeP011ScheduleWant`→`player.start`）需要同时设置 run metadata——建议 want 增加 metadata 参数（`p011runrole/thresholdcalibrationid/thetastartdeg/thetastopdeg` 独立解析，不再门控于 wstilt），或在 `p011schedule` want 解析成功时从同 want 的这些参数直接武装。可复用既有 `setNaturalObservationRunMetadata` 接口，零 schema 变更。

**tester**：drift 消费口径待定义（P50=57ms 已超 50ms 标记线，62.7% 记录带 marker——若"marker 即失格"则本机制需要收紧漂移而非放宽阈值；建议按 §3.2"既有输入时序闸门"评估实际接受间隔而非绝对 drift）。

**manager**：pilot 重启条件 = developer 补 metadata 武装 + effector 复跑 smoke 确认 executedThresholds 非空 + 正式 pilot 单 run。

*红线自查：smoke run 已明确标记为诊断性（ledger runId 保留，不进 fit/holdout）；未改代码、未改阈值、未放宽闸门；A=18.7 与 schedule 摘要未变。*
