# P-011 KNOWN FAILS 台账（KNOWN_FAIL / DEFERRED_TO_PHASE_2）

> 建立日期：2026-10-20。依据：`docs/priority.md` 2026-10-20「P-011 阶段一·B architect CONDITIONAL_PASS 裁决」第 3 条（C-2｜选择维持文档化 FAIL 至阶段二）。维护人：tester（本清单）/ manager（裁决）/ designer（阶段二复核）。
>
> 定性（architecture-P011 §22 R-2，architect 独立验证）：两个 legacy FAIL **不是限幅实现的缺陷，而是新闸门如实暴露了旧引擎从未拦截的真实 tan30 违约**（e2f02a1 时代 `audit()` 无 tan30 检查；v1 证据 484 违约子步坐实该违约真实存在）。**该暴露是 §20 闸门的正确行为，不得为恢复测试绿色而放宽闸门。**

## 状态总览

| # | 用例名 | 状态 | 测试代码位置 | 最新失败证据 | 阶段一处置 | 阶段二处置 |
|---|---|---|---|---|---|---|
| KF-1 | `bottomCreep_tiltMovesLowerLayers` | KNOWN_FAIL / DEFERRED_TO_PHASE_2 | `entry/src/test/WaterSandFieldTest.ets:393` | `test/reports/p011_planning_units.json`（48 用例中 FAIL，2026-10-20 10:54）、`p011_planning_units_verify2.json`、`recon_units_check.json` | 维持文档化 FAIL，总验收报告中明示 | tester/designer 结合 B·续 guard 与候选机制重新分类并重测 |
| KF-2 | `p011_commandSignFlipResetsPhaseAndReversesDisplacement` | KNOWN_FAIL / DEFERRED_TO_PHASE_2 | `entry/src/test/WaterSandFieldTest.ets:512` | 同上三份报告 | 同上 | 同上 |

## KF-1 bottomCreep_tiltMovesLowerLayers

- **场景**：16×192 场域，angleDeg=5，maxAvalancheFlux=60，bottomCreepRate=96——验证倾斜时底层蠕动是否移动下层沙。
- **失败表现**：断言 `false !== true`（倾斜后下层位移断言未满足新闸门下的可行域约束）。
- **归因**：旧引擎真实 tan30 违约被新闸门（4b1f635 planning pass 可行域闸门）暴露；**不归因于 4b1f635 新回归**。
- **失败证据保留**：`test/reports/p011_planning_units.json` / `p011_planning_units_verify2.json` / `recon_units_check.json` 原样保留，不删除、不重写。

## KF-2 p011_commandSignFlipResetsPhaseAndReversesDisplacement

- **场景**：2×192 场域，vSandTopMax=1，totalMotionBudget=100，setTilt(20) 后符号翻转命令序列——验证相位重置与位移反向。
- **失败表现**：断言 `false !== true`（同上，新闸门可行域约束下旧预期不可满足）。
- **归因 / 证据保留**：同 KF-1。

## 纪律（本台账生效期间，以下全部为硬约束）

1. **不放宽 tan30 判据**：可行域/安全判据维持原值，任何放宽须 manager 授权。
2. **不改场景参数/测试期望**：不重调 30° 测试场景参数，不修改断言期望（改测试期望须 tester 授权）。
3. **不删除失败证据**：三份报告与未来重跑的失败记录原样保留。
4. **不删除白名单标记**：`test/p011_planning_pass_check.py` 中 `legacy` 集合的 3 项（含 `p011_directionAccuracyCovers200SamplesAndSignSwitches`，其 BLOCKED 语义同族）保留，处理完成前不得移除。
5. **不声称已解决**：阶段二重测通过并裁定前，任何文档/报告不得写这两项"已修复/已解决/已通过"。
6. **总验收明示**：阶段一总验收报告必须列示这两项 KNOWN_FAIL 及本台账链接。

## 阶段二重测计划（预设，届时按当时裁决细化）

1. **前置**：B·续（§19.5 全通道 pre-commit 材料提交 guard）交付并评审通过；designer 候选机制可达性审计结论就位。
2. **重新分类**：tester/designer 联合判定每项属（a）新 guard 下自然满足 → 直接重测；（b）场景需重设计 → 修改用例（走用例变更流程，旧用例归档不删）；（c）机制能力不足 → 转入目标复议。
3. **重测**：按重分类结果执行，报告追加至对应 test-report，本台账状态更新为 RESOLVED / RECLASSIFIED / STILL-KNOWN-FAIL。
4. **豁免不溯及既往**：阶段一期间的 FAIL 记录不因阶段二通过而改写。

## 关联文档

- 裁决：`docs/priority.md` 2026-10-20 C-2 条
- 定性：`docs/architecture-P011.md` §22 R-2（:2326）
- 例外列示：`docs/gap.md` 2026-10-20 B 评审裁决节
- 用例引用：`docs/test-cases.md` §11.6
- 白名单：`test/p011_planning_pass_check.py` `legacy` 集合（:127-131）
