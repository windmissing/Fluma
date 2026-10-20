# P-011 阶段一·预验证报告：C 线子集（±15° 静止+安息角）+ SLIP 安底线摸底（合并 session）

- **日期**：2026-10-20；执行：effector/developer（预验证子集）。
- **性质**：**预验证，非正式验收**——全部结果不计入 SLIP-01/03/04 正式验收，用于提前暴露问题。
- **被测构建**：`5e60f2d`（= 891370d + nine-phase mapping/planner fields capture fixes；worktree 干净重建签名，HAP SHA256 `7ffc667f…7b6a`）。
- **session**：两 run 串行（真机 62T0225B10010020）：
  - **runA-seven45**（`scheduleid=seven-phase-45-v1`，0/+15/0/+45/0/−45/0）：C 线 ±15° 静止回归 + 安息角响应主数据。
  - **runB-nine30**（`scheduleid=nine-phase-30-v2`，0/+15/0/+30/0/−15/0/−30/0）：验证 5e60f2d 相名映射/planner 字段修复的真机效果；±15° 静止回归补第二符号。
- **证据**：`test/artifacts/p011-preverify-20261020/{runA-seven45,runB-nine30}/`（ledger/raster/hilog/schedule/safety-summary）。

---

## 1. 安全底座（SLIP-01 tan30 / SLIP-03 守恒库存容量 / SLIP-04 预算对账）摸底

| 检查项 | runA-seven45 | runB-nine30 | 摸底判定 |
|---|---|---|---|
| tan30 硬约束（豁免后） | 134,620 边 **0 违约**（588 子步豁免） | 256,286 边 **0 违约**（0 子步豁免） | **PASS** |
| 预算对账（A-1 恒等式） | bottom 0 / total 0 不匹配 | 0 / 0 | **PASS** |
| 守恒/库存/容量/有限值 | 0 失败 | 0 失败 | **PASS** |
| ledger/telemetry 对齐 | PASS | PASS | **PASS** |
| 相位终点（坡度稳定+悬沙 ≤5%） | 七相全过 | 九相全过 | **PASS** |
| checker overall | FAIL | FAIL | **仅由 DEFERRED 的 SLIP-02 收敛包驱动**（阶段边界裁决不阻断），非安全问题 |

**结论：安全底座在 5e60f2d 上全线干净**——SLIP-01/03/04 的 MUST-PASS 集摸底无任何违约。同口径下 835e663 曾有的 441–468 条越界已消失（891370d 材料 guard + 5e60f2d 修复后）。

## 2. C 线·±15° 静止回归

| 检查项 | runA +15° | runB +15° | runB −15° | 判定 |
|---|---|---|---|---|
| 平台稳定（rate=0、|eff−15|≤0.2°） | 135 子步 | 120 子步/6.9s | 110 子步/7.3s | PASS |
| 平台内 start 转换 | **0** | 0 | 0 | **PASS**（静止死区无误触发） |
| 平台内 stop 转换 | 0 | 0 | 0 | PASS（无翻转） |

**DEVIATION-1（静止段 relax 持续请求/发放）**：+15° 稳定平台内 `relaxTriggered=true` **135/135 子步**，平均 `relaxRequestedTotal≈12.09`/`relaxAppliedTotal≈2.88`——安息角机制（thick `angleDeg=4`）持续请求并发放崩塌量，表面 `max|ΔS|` 从 0.065 漂移到 0.173（仍在演化，未静止）。SLIP 静止回归预期「稳定保持段 relax grant 应为 0」**按字面不满足**；但 algorithm-P011 §8.15 已注明 ±15° 死区口径只约束 **start 转换**（0 达标），不约束 tan4 崩塌请求。**移交 designer 裁决**：正式 SLIP 静止用例的判定口径须写明 relax 持续请求是预期行为还是 DEVIATION，避免正式验收时口径争议。

## 3. C 线·安息角响应（±45° 大角度，runA）

| 观察项 | 结果 |
|---|---|
| +45° accepted | 29.9° 上限行为延续（planning 闸门钳位，符合 §8.16 已知边界） |
| **−45° accepted** | **钳在 −26.88°**（对比 +45° 的 +29.9°——**正负不对称 3°**） |

**DEVIATION-2（planning 可行域正负不对称）**：负向可行域上限比正向低约 3°（−26.88° vs +29.9°）。材料初始态对称、几何对称，该不对称大概率来自初始沉积形状或 guard 交集的符号相关行为。不影响安全（0 违约），但**影响观测③ v3 的平台角可达性假设**（负向平台可能更低）——移交 designer/designer 在 v3 可辨识性论证中使用本实测值。

## 4. 观测③ v2 修复真机验证（runB-nine30，5e60f2d 交付项）

| 检查项 | 结果 | 判定 |
|---|---|---|
| 九相 phaseD 映射 | positiveLow/positiveHigh/negativeLow/negativeHigh 全部正确落名（无 unknown） | **PASS**（修复生效） |
| `scheduleid` want 参数 | 两 schedule 均被接受，INVALID_SCHEDULE_ID 未出现 | PASS |
| ±30° 可达性 | **+30° 稳定平台出现**：101 子步/6,676ms（accepted 29.885°，容差内）——对比 pilot run-001 的 0 记录 | **DEVIATION→改善**：可达性结论从「完全不可达」变为「正向可达」 |
| **−30° 可达性** | **仍 0 稳定记录**：accepted 停在 −29.035°，rate=0 无 BLOCKED，但 |−29.035−(−30)|=0.965° > 容差 0.2° | **DEVIATION-3：负向平台差 0.965° 不可达**（与 DEVIATION-2 同源的不对称） |
| planner BLOCKED | runB hilog model_error code5 = **0**（pilot 为全 BLOCKED） | 修复确认 |

## 5. 汇总判定表

| # | 项目 | 判定 | 说明 |
|---|---|---|---|
| 1 | SLIP-01 tan30 安全线 | **PASS** | 两 run 0 违约（豁免口径内） |
| 2 | SLIP-03 守恒/库存/容量/悬沙 | **PASS** | 全 0 失败，悬沙 ≤5% |
| 3 | SLIP-04 预算+对账 | **PASS** | A-1 恒等式 0 不匹配 |
| 4 | C·±15° 静止回归（start 死区） | **PASS** | 平台内 start=0 |
| 5 | C·±15° 静止 relax 请求 | **DEVIATION-1** | tan4 机制持续请求/发放，口径待 designer 裁决 |
| 6 | C·安息角响应 +45° | PASS（已知钳位） | 29.9° 符合 §8.16 |
| 7 | C·安息角响应 −45° | **DEVIATION-2** | 钳位 −26.88°，正负不对称 3° |
| 8 | 九相映射修复 | **PASS** | 相名全部正确 |
| 9 | +30° 平台可达 | **改善**（101 子步稳定） | 5e60f2d 修复生效 |
| 10 | −30° 平台可达 | **DEVIATION-3** | 差 0.965° 不可达，与 #7 同源 |

## 6. 移交与纪律

- 本报告为**预验证**，所有 PASS 不等于正式验收；正式 SLIP-01/03/04 验收须按 test-cases 正式口径另跑。
- **DEVIATION-1**（静止 relax 口径）→ designer：写正式静止用例判定口径。
- **DEVIATION-2/3**（planning 可行域正负不对称，负向少约 3°）→ designer：v3 可辨识性论证按实测修订；若需对称，developer 排查 guard 交集/初始态符号相关行为。
- 未改代码、未改阈值、未改用例；真机串行占用已释放。
