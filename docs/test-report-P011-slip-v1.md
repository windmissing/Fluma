# P-011 SLIP formal RED baseline

- Verdict: **FAIL**
- Evidence: historical read-only `slip-overshoot-v1` run
- Budget rule: unchanged local/bottom/total ceilings; reconcile clipped with requested-used within 1e-9; relax activity is permitted.

## Budget

- Violating substeps: 756
- Bottom clipped/requested/used mismatches: 742
- Total clipped/requested/used mismatches: 486
- Current violations are accounting mismatches; the historical prior-rule count of 1,738 included the invalid relax-zero and clipping-zero requirements.

## Hard slope constraint

- Checked edges: 319024
- Violating substeps: 484
- Violating edges: 59413

## Overshoot convergence

| Phase | Final slope (deg) | 30 +/- 2 deg | R2 >= 0.90 | First 27 deg time (ms) | t90 <= 5100 ms |
|---|---:|---|---|---:|---|
| positiveOvershoot | 0.226404 | false | false | null | false |
| negativeOvershoot | 0.523789 | false | false | null | false |

Full machine-readable result: `test/artifacts/p011-slip-formal-v1/formal-checker-output.json`.

## 2026-10-20 新口径重测（±45° tan30 参考线形式判定豁免）

- 被测代码 commit：`e2f02a15720a181107dd169b017127bc85c64ca4`。
- 裁决依据：`docs/priority.md` 2026-10-20「±45° 补采 gate 几何冲突裁决」。
- checker：`test/p011_slip_formal_check.py`。豁免仅作用于 `positiveOvershoot`、`positiveOvershootReturn`、`negativeOvershoot`、`negativeOvershootReturn` 四相位的 `|ΔS−tan(effectiveTilt)|≤tan30°` 参考线形式判定；0°/±15° 与其它相位照旧。豁免子步仍完整执行有限值、守恒、库存、容量、预算、账本对齐、边身份与 stop 语义闸门。
- **明文声明**：豁免 ≠ 坡度达标、≠ 无物理越界、≠ run 自动合格。豁免只是把「参考线形式判定」从 ±45° 平台子步移除，材料侧约束全部保留。
- 旧判定审计保留：本文件上方历史结论与旧数值原样保留，不追认、不重写；新旧结果并列可查。

| Run | 硬坡度（新口径） | 豁免子步/边 | 预算违约子步 | 收敛 30°±2° | R²≥0.90 | t90≤5100ms | 终点稳定 | 悬沙≤5% | 账本对齐 | 观察 contract | 总判定 |
|---|---|---:|---:|---|---|---|---|---|---|---|---|
| v212 run-001 | PASS（0 违约边） | 1077 / 136779 | 717（bottom 对账 704，total 对账 469） | FAIL（+0.117° / −0.506°） | FAIL（0.0003 / 0.309） | FAIL（均未达 27°） | PASS | PASS | PASS | PASS | **FAIL** |
| v212 run-002 | PASS（0 违约边） | 1083 / 137541 | 679（bottom 对账 666，total 对账 445） | FAIL（+0.146° / −0.481°） | FAIL（0.0005 / 0.319） | FAIL（均未达 27°） | PASS | PASS | PASS | PASS | **FAIL** |
| v212-relax-hysteresis calibration | PASS（0 违约边） | 1085 / 137795 | 657（bottom 对账 644，total 对账 443） | FAIL（+0.170° / −0.449°） | FAIL（0.0007 / 0.284） | FAIL（均未达 27°） | PASS | PASS | PASS | PASS | **FAIL** |

- 机器可读结果（追加，不覆盖旧文件）：
  - `test/artifacts/p011-v212-recalib-20261020/run-001/formal-checker-output-ruling-20261020.json`
  - `test/artifacts/p011-v212-recalib-20261020/run-002/formal-checker-output-ruling-20261020.json`
  - `test/artifacts/p011-v212-relax-hysteresis/calibration/formal-checker-output-ruling-20261020.json`
- 结论：新口径下 tan30 参考线形式违约清零（此前 run-001/002 45° 平台系统性失败的指标被豁免），但三份 run 的总判定仍为 FAIL：预算 clipping 对账违约未清零，±45° raster 收敛、R² 与 target-based t90 仍未达标。按裁决，只有修订后适用 hard-gate 全部通过才可进入补采候选池，因此三份 run 均不具备补采资格。

## 2026-10-20 阶段适用性说明（阶段边界复议裁决，标签更新不改数值）

- 裁决依据：`docs/priority.md` 2026-10-20「P-011 阶段边界复议裁决」第 4/6/7 条。
- 本节仅更新判定标签与适用阶段，**不运行新测试、不修改任何历史数值**；上方 2026-10-20 新口径重测三行结果与所有旧结论原样保留、并列可查。
- **SLIP-02 整包 → 阶段二（DEFERRED / NOT VALIDATED）**：`30°±2°` 收敛、`R²>=0.90`、target-based `t90<=5100ms` 及适用的 ±45° 终点稳定/悬沙收敛约束移出阶段一 MUST-PASS，不以 FAIL 阻断阶段一完成；原始测量继续记录，作为阶段二可达性审计的诊断/能力数据。`DEFERRED` 不是 PASS。
- **SLIP-01（豁免口径）→ 阶段一 MUST-PASS 保留**。
- **SLIP-03 安全部分 → 阶段一 MUST-PASS 保留**：守恒、库存、容量、有限值、终点悬沙 `<=5%`、最后 1s 坡度稳定不延期；±45° 收敛达标归属 SLIP-02 阶段二口径。
- **SLIP-04 全部 → 阶段一 MUST-PASS 保留**：预算上限、clipping 对账、预算超额、账本对齐不延期。
- **新阶段一 A/B/C 标准**接入总验收：A 双角滞回/状态机、B 安全限幅与提交 guard、C 现有机制效果（详见 `docs/test-cases.md` §11.0）。
- checker 同步更新：`test/p011_slip_formal_check.py` schemaVersion 1→2，JSON 输出新增 `phaseApplicability` 顶层节与 `convergence[*].applicablePhase/deferred/deferredBasis` 标记字段；报告模板新增「Phase applicability」节。合成 ledger 验证输出见 `test/artifacts/p011-slip-formal-v1/formal-checker-output-phase-tag-verify.json` 与同名 `.md`（验证运行仅确认新字段落盘，不构成新的正式测试结果）。
- 阶段一完成判定改用 `docs/test-cases.md` §11.0 阶段一验收清单；阶段一不得标记 P-011 CLOSED，也不得写成 SLIP 全 PASS。

### 2026-10-20 新口径 checker 重测三历史 run（schemaVersion 2，deferred 标记生效）

- checker：`test/p011_slip_formal_check.py`（schemaVersion 2，带 deferred 标记）。三份历史 run 用更新后 checker 全量重跑，新输出文件（追加，不覆盖旧文件）：
  - `test/artifacts/p011-v212-recalib-20261020/run-001/formal-checker-output-phase-tag-20261020.json`
  - `test/artifacts/p011-v212-recalib-20261020/run-002/formal-checker-output-phase-tag-20261020.json`
  - `test/artifacts/p011-v212-relax-hysteresis/calibration/formal-checker-output-phase-tag-20261020.json`
- **数值一致性验证**：新输出剥除新增标签字段（`phaseApplicability`、`convergence[*].applicablePhase/deferred/deferredBasis`）后与旧 `formal-checker-output-ruling-20261020.json` 逐键比对，三份 run 均 identical——deferred 标记只加标签、不改任何闸门判定与实测数值。
- **deferred 字段落位确认**：三份 run 输出 JSON 中 `phaseApplicability.phase2Deferred = ["convergence (SLIP-02: slope 30+/-2deg, R2>=0.90, target-based t90<=5100ms)"]`，`convergence[*].applicablePhase="phase-2"`、`deferred=true` 均正确出现。

| Run | 硬坡度（阶段一） | 预算对账（阶段一） | 账本对齐 | 终点稳定/悬沙（阶段一） | SLIP-02 收敛 30°±2° | SLIP-02 R²≥0.90 | SLIP-02 t90≤5100ms | SLIP-02 判定 | 总判定 |
|---|---|---|---|---|---|---|---|---|---|
| v212 run-001 | PASS（0 违约边） | FAIL（717 子步；bottom 对账 704，total 对账 469） | PASS | PASS（7/7 相位） | FAIL（+0.1166° / −0.5061°） | FAIL（0.0003 / 0.3092） | FAIL（均未达 27°） | **DEFERRED（阶段二，不计入阶段一判定）** | **FAIL（阶段一口径）** |
| v212 run-002 | PASS（0 违约边） | FAIL（679 子步；bottom 对账 666，total 对账 445） | PASS | PASS（7/7 相位） | FAIL（+0.1457° / −0.4809°） | FAIL（0.0005 / 0.3190） | FAIL（均未达 27°） | **DEFERRED（阶段二，不计入阶段一判定）** | **FAIL（阶段一口径）** |
| v212-relax-hysteresis calibration | PASS（0 违约边） | FAIL（657 子步；bottom 对账 644，total 对账 443） | PASS | PASS（7/7 相位） | FAIL（+0.1702° / −0.4487°） | FAIL（0.0007 / 0.2840） | FAIL（均未达 27°） | **DEFERRED（阶段二，不计入阶段一判定）** | **FAIL（阶段一口径）** |

- 表列口径说明：**总判定 = 阶段一口径 FAIL**，唯一未过的阶段一闸门是预算 clipping 对账；SLIP-02 三项（收敛/R²/t90）实测数字照录但按裁决标 DEFERRED、**不计入阶段一判定**——若不计 deferred 项，三 run 的阶段一硬门槛（SLIP-01/03/04 安全不变量）中仅预算对账未过。
- DEFERRED 不改变 FAIL 语义：SLIP-02 仍不是 PASS，实测值（坡度远低于 30°±2°、R² 远低于 0.90、t90 未达 27°）原样保留，供阶段二 designer 可达性审计使用。
- 补采资格结论不变：三份 run 预算对账违约未清零，按裁决仍不具备补采候选池资格；本节重测仅更新标签与口径呈现，不产生新的补采资格。
