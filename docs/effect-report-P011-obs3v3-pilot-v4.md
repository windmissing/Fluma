# P-011 观测③ v3 单 run clean pilot 报告（正式 pilot 已执行）

- **日期**：2026-10-21
- **执行**：effector
- **任务**：v3 pilot smoke3 + 正式 pilot（任务 6ad82794，c869cda 解锁）
- **被测构建**：c869cda（`entry-default-s106-signed.hap` SHA256 `e9f3f928…b6a9`，干净 worktree 重建签名）
- **runId**：`p011-run-1792575546108-3`（**正式 pilot**，analysisRole=pilot）
- **结论**：**三次前置缺口全部解除（执行通道→metadata→身份），正式 pilot 首次完整跑通——v3 checker 身份/时序/遥测全过、无硬闸门失败；pilotCoverage=INCOMPLETE（单 run 预期内）、thresholdVerdict=NOT_EVALUATED（协议 §8 恒定）。overall=PILOT_REPORT（非 INVALID）——这是 v3 协议下第一个合格 pilot run，待 manager 验收后决定扩批**

---

## 1. 执行链验证状态（三次停机缺口 → 全部解除）

| 缺口 | 修复 commit | 真机验证 |
|---|---|---|
| EXECUTION_CHANNEL_BLOCKED | d4aa997（v3 播放器） | ✅ smoke1（1513 命令全派发） |
| SCHEDULE_METADATA_NOT_ARMED | 69345cc（metadata 武装） | ✅ smoke2（runRole/26/20 全落账） |
| SCHEDULE_EXEC_IDENTITY_NOT_CARRIED | c869cda（身份三参数） | ✅ 本次 formal run（见 §2） |

## 2. 正式 pilot 结果（runId `p011-run-1792575546108-3`）

**want**：`p011schedule=18.7:0.5` + metadata 四参数（calibration / hysteresis-calib-v1 / 26 / 20）+ 身份三参数（commit `c869cda…`、WaterSandField.ets SHA256 `752a01b8…`、HAP `e9f3f928…`——XOR 组完整）

**ledger**：6780 记录（末行 recv 截断已修剪归档 `ledger-trimmed.ndjson`），segment H0..H8 + L1..L8 全部出现。

### 2.1 v3 checker 判定（`v3-summary.json`，实跑）

| 检查项 | 结果 |
|---|---|
| **executionThresholdIdentity** | ✅ **ok=true**（sourceCommit=c869cda…/双 SHA-256 全非空且与 runner 绑定一致——前两轮缺口闭环） |
| protocolMetadataErrors | ✅ 0 |
| runInvalidHardGate | ✅ false（无硬闸门失败） |
| telemetryIncompleteRecords | ✅ 0 |
| scheduleTiming gate | ✅ breached 2/1484（ramp 间隔 P50=100/P95=133/max=203ms；hold P50=5125ms min=5071 无伪造；driftRole=telemetryOnly） |
| drift | P50=51 / P95=90 / max=99ms（与 smoke 一致，无追赶） |
| overall | **PILOT_REPORT**（非 INVALID——三次停机缺口下首次） |
| thresholdVerdict | NOT_EVALUATED（§8 恒定） |
| pilotCoverage | **INCOMPLETE**（单 run，30 状态/10 run/20 事件门槛本来就是扩批后的事） |

### 2.2 观测产出（供 manager 评估扩批价值）

- **平台质量**：±15° 平台各 206 条稳定记录、accepted 恰 ±15.0°、0 planner 受限——平台窗口质量极好；±30 项=0（本 schedule 峰值 18.7°，预期内，诊断项）
- **机器驱动转换**：全 ledger 88 个真实转换（状态机驱动）；**平台内（checker cell 语义可归属）18 个**：H2 start×9（α≈26.0 钳位，+峰平台残余爬升触发）、H6 start×7、H5 start×1、H8 stop×1（尾部）
- **四 cell 计数**：start/+ 0、stop/+ `STOP_TRANSITION_UNOBSERVED`、start/- 0、stop/- `STOP_TRANSITION_UNOBSERVED`——单 run 不产出任何合格 bracket（预期内：影子审计的 bracket 激励主要在 ramp leg，且单 run 远低于统计门槛）

## 3. 移交 tester/manager 的两个语义复核项

1. **ramp 内转换的 cell 归属**：checker 的 `realTransitions` 只累计 `cell_sign` 非 None（stable 平台内）的转换——ramp leg 内 70 个转换（含影子审计预测的主激励 L2/H2 start、L3/L7 stop）未被归属到四 cell。checker 注释自称"v3 allows ramp legs as eligible segments"，但实现中 ramp 段 `cell_sign=None` 直接 `continue`（在排除计数之前）——**这与 §5.2 "same preregistered ramp leg OR same eligible stable-platform segment" 的实现存在语义分歧**。影子审计的 SHADOW_COVERED 判定基于 ramp leg bracket；若 checker 维持现语义，扩批的 bracket 产出会系统性为 0。需 tester 确认是否为有意保守（扩批前必须裁决，否则 34 run 会重演"影子有覆盖、checker 全零"）。
2. **H8 段尾污染**：播放器结束后 segment 标记保持 H8，采集窗口尾部 ~44s 自由演化计入 H8（2445 条中 2297 条在正式 hold 之外）。建议 checker 按播放器结束时间截断（或 ledger 加 schedule-finished 标记）。

## 4. 判定与移交

| 项 | 状态 |
|---|---|
| 执行链三缺口 | ✅ 全部解除（真机验证） |
| 正式 pilot 硬闸门 | ✅ 全过（runInvalidHardGate=false） |
| v3 checker 六项 | ✅ 身份/相名/时序/遥测全过（§8 判定：clean 资格成立） |
| pilotCoverage | INCOMPLETE（单 run 预期，非失败） |
| thresholdVerdict | NOT_EVALUATED（恒定） |

**移交 manager**：① 本 run 按 §8 判定达到"clean pilot 完成"标准（六项检查全过、无硬闸门失败）——是否验收并解锁扩批 34 runs 由 manager 裁决；② 扩批前需 tester 裁决 §3.1 的 ramp 归属语义（否则影子审计预测的 ramp bracket 无法在 checker 里兑现）；③ H8 尾部截断建议随扩批 runner 一并处理。

*红线自查：A=18.7 与 schedule 摘要 7b59b2e4…bd46 未变；未改物理/阈值/闸门；ledger/raster/hilog/run-identity.json 全套归档 `test/artifacts/p011-v3pilot4-20261021/run-173859/`。*
