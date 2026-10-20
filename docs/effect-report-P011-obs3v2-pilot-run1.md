# P-011 观测③ v2 clean pilot 报告（单 run）

- **日期**：2026-10-20；执行：effector。授权：priority.md 2026-10-20 裁决第 2/4 条（单 run clean pilot），manager 批准 2026-10-20 13:06 确认。
- **runId**：`p011-run-1792472946390-3`；设备 62T0225B10010020；构建 `891370d`（HAP `c9867dac…2d6f`，含 §19.5 逐 writer 材料 guard）。
- **protocol**：`p011-observation3-v2 / preregistration-v1`；schedule `0/+15/0/+30/0/-15/0/-30/0`（`scheduleSha256=685be2bc…e5a4`，runner `893a9928…a64f`，候选 manifest `fbe0b488…68ac`，全部摘要启动前复核一致）。
- **判定身份**：`analysisRole=pilot`；`thresholdVerdict=NOT_EVALUATED`（不拟合、不估 wNoise、不碰 holdout）。

## 1. clean 资格判定：**INVALID（hard gate failure）→ 按预注册稿 §6 隔离整个 run**

safety checker（schemaV2）发现 **5 条边级 tan30 违约**（1 个子步，seq 2020，eff=-29.1235°，单边超出上限 **1.25e-7**（浮点 epsilon 量级：`|ΔS−tan(eff)|=0.57735039 vs 0.57735027`），发生于 −30° 目标被 planning 连续 BLOCKED 后的有效角回摆段（phaseD=`unknown`，不在豁免相位清单内）。其余安全项全过：预算对账 0 不匹配、守恒/库存/容量 0、遥测对齐 0 错、raster 0 错。

**隔离处置**：该 run 不进入 fit/holdout，不折算任何"好子步"；全部证据保留于 `test/artifacts/p011-obs3v2-20261020/run-001/`（ledger 2,020 记录、九相 raster、hilog、schedule.csv、双 checker summary）。

## 2. 平台可达性：**PARTIAL PASS**（±15° 可达且极稳，±30° 被 planning 闸门完全阻断）

| 目标平台 | 实际 accepted | 稳定窗口（容差 0.20°、rate=0、≥3 子步/≥200ms） | 判定 |
|---|---|---|---|
| +15° | 恰 15.0000° | 206 条记录、跨 6,921ms | ✅ `PLATFORM_REACHED`，质量极好 |
| −15° | 恰 −15.0000° | 204 条记录、跨 6,855ms | ✅ `PLATFORM_REACHED`，质量极好 |
| +30° | 峰值 29.8761°（rate=352°/s，仍在爬升） | **0 条稳定记录** | ❌ `PLATFORM_UNREACHED`（见下） |
| −30° | 峰值 −29.1235°（同上） | **0 条稳定记录** | ❌ `PLATFORM_UNREACHED` |

**±30° 阻断机制（新发现，与 designer §8.16 预测的形态不同）**：目标 +30° 下 **每个子步都被 planning pass 判 `TILT_PLANNING_BLOCKED`（error code 5）**——hilog 显示该 hold 内 1,318 帧 `substeps:0`，物理完全冻结在 ~29.88°，**一个子步都无法提交**。即：不是"到达 29.9° 平台后高端右删失"，而是 **theta≈30° 的可行域交集为空，accepted angle 根本无法在 ±30° 稳定**。designer §8.16 预测的"29.9° accepted 平台 + 右删失"在本构建上不成立——**±30° 平台在 891370d 的 planning/tan30 闸门下不可达**（`PLATFORM_UNREACHED`，非 `RIGHT_CENSORED_AT_TAN30`——删失码要求平台已到达）。

## 3. bracket yield：四 cell 全为 0 个合格 bracket

| cell | 原始真实转换 | 平台内合格 bracket | 判定 |
|---|---|---|---|
| start/+ | 236 | **0** | `INCOMPLETE`（全部发生于 ramp/回摆段；+30° 平台不可达，无平台内 start 可言） |
| start/− | 223 | **0** | 同上 |
| stop/+ | 0 | 0 | `STOP_TRANSITION_UNOBSERVED` |
| stop/− | 0 | 0 | `STOP_TRANSITION_UNOBSERVED`（±15° 平台到达时边已 inactive，符合预注册稿 §8 预警的识别边界） |

`pilotCoverage=INCOMPLETE`。原始 start 转换量不小（459 个）但全部发生在 ramp/倾斜过程中，无一个落在稳定平台内——平台的激励幅度（±15° 静止角以下）设计上就不产生新 start，±30° 又不可达。

## 4. 附带发现的契约错位（移交 designer/architect/manager）

1. **app 相名映射仍是旧七相 45° schedule**：`p011NaturalObservationPhaseForCommand`（P011NaturalObservation.ets:261-286）硬编码 seq4=+45、seq6=−45、只到 seq7；九相新命令全部落 `phaseD=unknown`。后果：a) safety checker 的 ±45° 豁免相位名单不覆盖 ±30° 段（本 run 的 5 条 epsilon 违约因此被计入）；b) 后续任何按 phaseD 分组的分析对 ±15/±30 相失效。**需要 developer 按新协议更新映射后重新出包**（本 pilot 不改代码，照实报告）。
2. **`thetaAccepted`/`requestRejected`/`plannerHold` 字段缺失**（calibration checker 明示）：无法从 ledger 区分"planner hold"与"ramp"，预注册稿 §4.2 的 bracket 资格过滤目前只能靠 rate/容差近似。schema v3 需增补。
3. **planning 闸门在 ±30° 的"全 BLOCKED"行为**：§20.4 的 invalid continuation 在本场景退化为"物理冻结 6.9s"——帧循环活着但子步全拒。这是安全正确的，但意味着 **±30° 在当前 tan30 约束下不是"边界可达"而是"完全不可达"**，designer §8.16 的识别边界论证需要按此事实修订。

## 5. 结论与移交

- **本 pilot 结果**：`clean=INVALID(ε-violation) + platformReachability=PARTIAL(±15 PASS / ±30 UNREACHED) + pilotCoverage=INCOMPLETE(4×0 bracket)`。
- **不产出任何阈值估计**；观测③维持 `BLOCKED`；不扩批、不改 schedule、不改闸门、不改代码。
- **交 manager 裁决的核心事实**：±30° 平台被 planning 闸门结构性阻断（连 accepted 都无法稳定），方案 3（±15°/±30° 平台标定）的 ±30° 半边在本构建上无观测对象；±15° 平台本身质量极好但角度低于 thetaStart 搜索窗下缘 25°，设计上不激励 start。**若要在阶段一继续观测③，需要 designer 重新论证识别策略**（例如：靠 ±15°→0°→±15° 循环中"回零后再起动"的滞后窗口观察 start/stop？或修订 §8.16 的平台定义？）＋ developer 修复相名映射 + ledger schema 增补 planner 字段。
- 全部证据：`test/artifacts/p011-obs3v2-20261020/run-001/`（ledger/raster/hilog/schedule/manifest/safety-summary/calibration-summary）。
