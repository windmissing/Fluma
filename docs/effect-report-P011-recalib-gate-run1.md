# P-011 观测③补采首跑（run-001）资格审计与停机报告

日期：2026-10-20；effector 执行单：按 `docs/effector-collection-plan-P011-v212-hysteresis-v1.md` 冻结 schedule 采集。
设备 62T0225B10010020；runId `p011-run-1792453263758-3`；candidate pair `hysteresis-calib-v1`。

> **复测更新（2026-10-20 08:04）**：用户质疑代码被污染、要求重测失败项。已执行 run-002 复测（同冻结 HAP `9c08ac10…a36`，worktree 源码树复核干净，仅 local.properties 非源码差异）。**复测结论与 run-001 完全一致**：hard-gate 441 条（posOvershoot 222 / posReturn 3 / negOvershoot 212 / negReturn 4）、45° 稳定平台 0/430 子步合规、边身份 0 错位、stop 0 误标、预算 0 超额。首转折产出同构（pos45 start 1.3315 / pos45 stop 0.6585 / neg45 stop 0.5087/0.9098/1.1144）。**失败可复现，非采集/构建污染，停机判定维持**。run-002 证据同目录保留（runId p011-run-1792454581994-3）。
>
> 另核实：主工作区 developer WIP（§20 planning pass）当前仍编译失败（10 个 ArkTS 错误，arkts-no-untyped-obj-literals 等），且 run-001/002 的 HAP 均构建自独立 worktree 的干净 e2f02a1，与 WIP 无任何代码交叉；WIP 亦未改 tan30 审计逻辑（WaterSandField.ets:1144-1148 与 e2f02a1 相同）。developer decision-P011-SLIP-A2-A1-v1 裁决改的是 checker 判定（A-1/A-2/B-1），不涉及 hard-gate 口径。

## 被测构建身份（防旧包）

- 修复 commit：`e2f02a1`（[P-011] Fix edge identity and budget-limited active semantics，含 reviewer 阻断项 A/B 修复与 2 条回归测试）。
- 工作区 HEAD 构建**失败**：并行会话在主工作区留有未提交的 developer WIP（P011TiltPlanningPass.ets / WaterSandField.ets，引用不存在的 `previewChannelPlansAt`，编译 5 错）。**未触碰该 WIP**。
- 处置：`git worktree` 检出干净 `e2f02a1` 至 `D:/hmprojects/hw-recalib-e2f02a1` 独立构建+签名。
- HAP SHA256：`9c08ac10db713afebe9bdeeaa7ddc84344422f71d00171c84e9562b91e795a36`（runner 内置 digest 闸门校验通过）。

## 逐项资格判定（执行单 run-level 闸门 1~5）

| # | 闸门 | 结果 | 证据 |
|---|---|---|---|
| 1 | 七相完整、schedule/身份字段齐全 | **PASS** | 七相齐全、commandSequence 1..7、runRole=calibration、transitionTelemetryAvailable=true、checker 契约校验 0 违规 |
| 2 | 边身份 interfaceIndex=min(donor,receiver) | **PASS** | 113,848 条 grant，0 条身份错位（d5a4e4f 污染为 34.46%，e2f02a1 修复生效） |
| 3 | stop=真实 activeBefore→activeAfter，限流不标 stop | **PASS** | 365 条 stop，0 条 requested>granted（fix B 生效） |
| 4 | hard-gate violation=0 | **FAIL（468 条）** | positiveOvershoot 232 / positiveOvershootReturn 3 / negativeOvershoot 226 / negativeOvershootReturn 7；预算/守恒/库存/容量全 0 |
| 5 | 稳定平台过滤字段齐全 | **PASS** | targetTilt/effectiveTilt/effectiveTiltRate/连续 substepId/边身份/r/d/active 前后/request/grant 全部可复核 |

## 硬门槛失败定性（与旧 run 对照，非同因）

- 旧 d5a4e4f run：439 条越界，最大残差 1.398，且 **34.46% 边身份污染**——两因叠加不可归约。
- 本 run：468 条越界集中于**同一模式**——+45° 稳定平台内 `maxSandTopInvariantError` 全部 454 个稳定子步持续 >tan30°（最大 1.528，集中在 col 0~3 附近，`dS≈-0.5` 即界面局部反向 ~26° 上坡），**0 个合规子步**；负面 -45° 对称。这是 e2f02a1 修复后**残留的独立几何问题**：45° 目标下 `|ΔS-tan(45°)|≤tan30°` 要求界面平行于 +45° 参考线，而材料崩塌把界面松弛回 30° 安息角方向（含局部反向坡），按协议这是**真实的 hard-gate violation，不是参考线瞬态**（平台期不收敛、不消退）。
- 判定：按 algorithm-P011.md §1.0.5 与补采执行单"任何 hard-gate violation 的 run 不用于标定"，**run-001 资格判定 invalid/excluded**，原始证据完整保留于 `test/artifacts/p011-v212-recalib-20261020/run-001/`（ledger 2434 记录/raster 7 相/manifest/schedule/summary），不用于 fit/holdout，不作阈值估计，不改分组。

## 停机决定（执行单"clean fit gate"分支）

执行单明文：**"正式补采前先校验新 HAP/source manifest 不再是 d5a4e4f 污染构建"**、首跑即 gate。本 run 证明 hard-gate 违约为 e2f02a1 构建的**系统性**问题（每 run 预期同型 400+ 条），继续跑满 34 run 只会批量产出无效数据（每 run 约 2 分钟 + 46 秒采集，纯浪费）。触发执行单停止条件："若 clean pilot 的稳定平台过滤后某一方向无 start……停止扩批并将该 schedule 的证据缺口提交 manager"。本例是更直接的版本：hard gate 系统性失败，**停止扩批，上报 manager**。

## 客观可量化问题移交 tester/designer（effector 纪律）

1. **45° 目标与 tan30 形式验收在几何上不相容**：界面松弛到 30° 安息角后 `|ΔS-tan45°|≈1.5>tan30°=0.577`。除非启发 45° 专用 relaxAngle 或把 hard gate 参考改为分段（0/15° 合规、45° 按安息角豁免），任何 run 都过不了闸门 4。这是 algorithm 层决策（§8.13 动态可行域限幅未实现、DESIGN_NOT_CALIBRATED），非 effector 可修。
2. e2f02a1 的 A/B 修复本身**验证有效**（边身份 0 错位、限流 0 误标 stop）——reviewer 阻断项已闭环，问题只剩上述几何冲突。
3. 附带发现：主工作区有并行会话 developer WIP 未提交（3 个 entry 文件 + 4 个 docs/test 文件，HEAD 编译失败）。前置条件②"developer 确认工作区 2 文件归属并 commit"**尚未满足**，需 developer 处置后再约下一轮 gate run。

## 当前补采进度

- 有效合格 run：**0/34**（run-001 invalid，保留审计）。
- 观测③保持 BLOCKED；本报告不解除、不输出 thetaStart/thetaStop。
