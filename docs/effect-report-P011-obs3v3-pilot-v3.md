# P-011 观测③ v3 pilot 重跑报告（第二次停机）：metadata 武装成功，执行身份字段缺口 SCHEDULE_EXEC_IDENTITY_NOT_CARRIED

- **日期**：2026-10-21
- **执行**：effector
- **任务**：v3 pilot smoke 复跑 + 正式 pilot（任务 6ad82794，69345cc 解锁）
- **被测构建**：69345cc（`entry-default-s105-signed.hap` SHA256 `393f996e…de30`，干净 worktree 重建签名）
- **结论**：**metadata 武装真机验证通过（smoke2 全部 6830 记录 runRole=calibration / executedThresholds 非空 / θ=26/20 正确、双阈值状态机真机驱动 90 个转换）；但 schedule want 不携带执行身份三字段（sourceCommit/sourceFileSha256/buildSha256 落账为空串），v3 checker 判 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`——正式 pilot 再次停跑（SCHEDULE_EXEC_IDENTITY_NOT_CARRIED），上报 developer**

---

## 1. 重启前置

| 项 | 状态 |
|---|---|
| A_selected=18.7° / schedule CSV 7b59b2e4…bd46 | ✅ 复用（前轮已核对） |
| 出包 69345cc | ✅ s105 / `393f996e692063511fa778e57b659e92c332ab544861cddf415a090d9424de30` |
| §26.5 metadata 武装代码路径 | ✅ 代码审查确认（schedule want 解析 `p011runrole/thresholdcalibrationid/thetastartdeg/thetastopdeg` 成组校验，arm before player.start） |

## 2. smoke2（武装验证 run，非正式 pilot）

- want：`p011schedule=18.7:0.5` + `p011runrole=calibration` + `thresholdcalibrationid=hysteresis-calib-v1` + `thetastartdeg=26` + `thetastopdeg=20`
- runId `p011-run-1792570372889-3`（诊断标记）；240s 覆盖全时序；ledger 6830 记录（末行截断为 recv 时序所致，已修剪归档 ledger-trimmed.ndjson）

### 2.1 metadata 武装 ✅ 真机确认

| 字段 | 实测 |
|---|---|
| runRole | `calibration`（全部 6830 记录） |
| thresholdCalibrationId | `hysteresis-calib-v1` |
| thetaStartDeg / thetaStopDeg | 26 / 20（executed=configured） |
| executedThresholds | 非空，slopeStart 0.48773 / slopeStop 0.36397 正确 |
| **双阈值状态机真机驱动** | ✅ 90 个转换（对照 smoke1 的 LEGACY 3238 个），分布与影子审计预测同构（L2/H2 start@α≈26.0、L3/L7 stop@α≈20.0） |
| segment | H0..H8 + L1..L8 全部 |
| drift | P50=41 / P95=84 / max=99ms |

### 2.2 执行身份字段 ❌ 缺口

`executedThresholds` 实测：
```json
{"thetaStartDeg":26,"thetaStopDeg":20,"slopeStart":0.4877…,"slopeStop":0.3640…,
 "stateMachineVersion":"natural-relax-hysteresis-v1",
 "sourceCommit":"","sourceFileSha256":"","buildSha256":""}
```
**三个实现身份字段为空串**。checker `executed_identity_ok` 要求四字段非空 → `executionIdentityMissing:sourceCommit` → `EXECUTION_THRESHOLD_IDENTITY_MISMATCH` → 四 cell 全部携带该码、overall=INVALID（smoke2 实跑 checker 确认）。

**根因**：`execsourcecommit/execsourcefilesha256/execbuildsha256` 三参数仍只在 wstilt 路径（`P011TiltWant.ets:242`）解析并经 `setNaturalObservationExecutionIdentity` 设置；69345cc 的 schedule 武装只覆盖 metadata 四参数，未覆盖身份三参数。

**注意**：这不是"checker 刁难"——v3 核验（test-cases §11.11 核验 1）明确把"实现身份四字段齐备"作为身份真值的判据，空串意味着 ledger 无法自证被测构建。

## 3. Checker 实跑结果（smoke2，供对照）

| 检查项 | 结果 |
|---|---|
| executionThresholdIdentity | ❌ `executionIdentityMissing:sourceCommit` |
| scheduleTiming gate | ✅ 基本干净：1486 digested，**breached 仅 1/1486**（ramp 间隔 P50=100/P95=132/max=201ms，hold P50=5129ms 无伪造）；driftRole=telemetryOnly |
| platformReachability | ±15° 各 206/204 稳定记录（恰 15.0°）；±30 项=0（本 schedule 峰值 18.7，属预期，诊断项） |
| overall | INVALID（仅由身份缺失驱动）；thresholdVerdict=NOT_EVALUATED |

## 4. 判定

正式 clean pilot **再次停跑**：唯一阻断 = `SCHEDULE_EXEC_IDENTITY_NOT_CARRIED`。与上轮不同，本次缺口的修复面更小（一个参数组的透传）。

## 5. 移交

**developer（很小改动）**：schedule want 补 `execsourcecommit/execsourcefilesha256/execbuildsha256` 三参数解析（成组可选，与 §26.5 metadata 同模式），在 SandCanvas 武装块随 `setNaturalObservationRunMetadata` 一并调用 `setNaturalObservationExecutionIdentity`。runner 侧我在正式 pilot want 已准备传入这三参数（commit 69345cc 全 hash / WaterSandField.ets SHA256 / HAP SHA256）。

**runner 已就绪**：正式 pilot want 模板含 metadata 四参数 + 身份三参数，developer 交付后 effector 复跑 smoke（确认 executedThresholds 三字段非空）→ 正式 pilot 单 run。

*红线自查：smoke2 标记诊断不进 fit/holdout；未改代码/阈值；A=18.7 与 schedule 摘要未变；真机已释放。*
