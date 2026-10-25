# P-011 WS-08 摇晃峰值 RED 基线记录 v1

日期：2026-10-25。责任：tester（P-011）。状态：**RED_BASELINE / NOT_IMPLEMENTED / NOT_VALIDATED / NO_DEVICE_RUN / FORMAL_RELEASE_NOT_AUTHORIZED**。
协议依据：`docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（`protocolId=p011-ws08-peak-nondeposited` / `protocolRevision=ws08-peak-nondeposited-v1` / `peakMethodId=peakNonDepositedFraction` / `peakAuthority=APPROVED` / `peakProtocolEffective=false`，协议源 SHA-256 `3f043d404224b8290248d04ea600e5a7ac08b763625f28385d3c3063755ebfe9`）。
执行证据：`test/reports/peak_protocol_v1_execution.json`（执行器 `test/p011_peak_protocol_v1_execution.py`，全部数字可由重跑复现）。本卡**不使用真机**、不采集新数据、不复用任何合格摇晃证据（不存在）。

## 1. 总状态

**新量 `peakNonDepositedFraction` 在全部现存归档数据上为 `NOT_IMPLEMENTED`。** 没有任何归档 run 满足新协议的采集资格（无 shake 事件、无每提交 v7 账本、无 on/off 边界）。本基线不提供、也不接受任何数值峰值结论；旧 `peakSusp` 的既有 `BLOCKED` 结论原样保留，不追认、不推翻。

**边界说明**：本 RED 判定的是**数据/证据不可得**（归档缺字段），不评价并行开发中的 checker 代码；checker 实现与同 build 双报告属后续步骤（peak 协议 §8.2）。执行器输出路径 `test/reports/peak_protocol_v1_execution.json` 与并行开发会话共用，若工作区该文件被覆盖，以本卡 commit 版本与重跑产物为准。

| 项 | 值 |
|---|---|
| 新量状态 | `NOT_IMPLEMENTED`（缺字段清单见 §3） |
| `peakProtocolEffective` | `false`（宣布权在 manager） |
| `formalReleaseAuthorized` | `false` |
| 证据模式 | 合成算例（§5–§7 全部数字为构造值，非测量、非真机数据） |
| 设备使用 | 无（真机队列首位仍为掉沙 V1-V4 返修复测，归 effector） |

## 2. 归档勘查（三条独立证据）

1. **2026-10-13 WS RED 基线**（`test/artifacts/P003-P005-baseline-20261013/ws_check.json`）：`shake` 项 = `BLOCKED`，`note` = "missing WS_EVENT shake, suspensionFraction"，`evidence_count=0`。该基线判定的是"证据不可测"，**从未测得任何峰值**；不能事后当作已完成摇晃峰值采集。
2. **全量 hilog 归档扫描**（执行器 `artifactScan` 段）：扫描 `test/artifacts/*.txt` 共 **83** 个文件，其中 `WS_EVENT` 行 **191** 条、`WS_METRIC` 行 **1896** 条，**`WS_EVENT shake` 事件 0 条**。即：现存 250ms 抽样数据全部来自无摇晃输入的 run，连"同轨迹下界"都无法构造。
3. **归档 SLIP 账本**（`test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`，2512 条，sha256 `f2815c98f9a886f5`，路线 `natural-observation-v2.10`）：相位为 7 个倾斜相位（baseline/positive/negativeOvershoot/…），**无 `particles` 块、无 `endpointSampleElapsedMs`、无 `endpointPhaseOccurrenceId`、无 `integratedPhysicsSeconds`、无 shake-on 输入窗**；用例族为 `TC-P011-SLIP-03` 而非 `TC-WS-08` → 对本峰值**不适用**（`applicableToWs08Peak=false`）。

## 3. `NOT_IMPLEMENTED` 缺字段清单（确切路径）

| # | 缺失字段/证据 | 用途 |
|---|---|---|
| 1 | `context.particles.massH/massL`（+ `pMass/pPhase/pState` 数组、`byState`、`bySourceKind`、`degraded`） | 分子 `P`（含 state=2） |
| 2 | `context.endpointSampleElapsedMs` | 窗选取与覆盖（单调实际 elapsed） |
| 3 | `context.endpointPhaseOccurrenceId` | case occurrence 绑定 |
| 4 | `context.integratedPhysicsSeconds` | 诊断披露（`tau_W` 复算） |
| 5 | `shakeOnEffectiveElapsedMs` / `shakeOffEffectiveElapsedMs` | on/off 实际生效边界 |
| 6 | 每子步实际消费的 shake-on 输入身份 | 输入接受证据（预更新 shakeEnergy 不替代） |
| 7 | 峰值窗 occurrence manifest（caseId、边界、覆盖证据） | 固定窗资格与覆盖校验 |

说明：`stateAfter.suspH/suspL` 等列字段在旧账本中存在，但**不能替代**粒子桶与 elapsed/occurrence 身份；缺任一即整量 `NOT_IMPLEMENTED`，不打 0 分凑平均。

## 4. `OBSERVED_SAMPLED_MAX` 声明（不可用）

按协议 §5.3，250ms 旧数据最多只能记 `OBSERVED_SAMPLED_MAX`（同轨迹真 max 的下界）并明确标注不是合格证据。**现存归档不存在摇晃段 250ms 样本**（§2 证据 2：0 个 shake 事件），故本基线连该下界都不可得：`OBSERVED_SAMPLED_MAX = 不可用`。不得用倾斜/手势 run 的 `suspensionFraction` 序列冒充摇晃段样本。

## 5. 机制证据一：现行 checker 全序列 max 的假 PASS（执行器 `legacyCheckerBehavior` 段）

对构造 WS-* 日志（静置段 3 条 `suspensionFraction=0.80` → `WS_EVENT shake` → 摇晃段 0.40/0.42/0.38 → 停止后 0.05/0.03）运行**交付的** `water_sand_check.evaluate_shake()`：

| 量 | 值 | 说明 |
|---|---|---|
| 现行 `peakSusp` | **0.80** | `max(values)` 取全序列，**混入摇晃前静置段** |
| 现行判定 | **PASS**（`peakSusp≥0.5` 且 `residualSusp=0.03≤0.05`） | 二元判定，无 TIERED |
| 新口径固定窗峰值 | **0.42**（分档 4，不及格） | 窗内仅 0.40/0.42/0.38 |

即：同一条轨迹下，现行实现可因窗段外样本报 PASS，而新口径正确判不及格。该演示同时证明适配必要性（差异点归适配清单 D-1/D-6）。

## 6. 机制证据二：同账本双算框架与归因链（执行器 `dualCalculation` 段，合成账本）

合成同轨迹账本（`E=1000`，`VH0=600`、`VL0=400`，逐提交守恒成立；129 个提交、W 内 60 个提交、33 个 250ms 样本、W 内 12 个样本；静置段 `M=470`，摇晃窗 `W=[3000,6000)ms`，峰值提交 `t=4150` 处 `M=520`）。legacy 与新量在**同一原始账本**上计算，差异按三类效应分开：

| 步 | 量 | 定义 | 值 | 效应 |
|---|---|---|---|---|
| L1 | `legacyPeakSuspReported` | 全序列 + 250ms 样本 + `h=U/(D+U)` | **47%**（静置段） | 基线（legacy 报告值） |
| L2 | 同公式窗内样本 | 固定 W + 250ms 样本 + `h` | **41.5%**（t=3500） | **窗口效应 −5.5pp** |
| L3 | 同公式窗内每提交 | 固定 W + 每提交 + `h` | **44.5%**（t=4100） | **密度效应 +3.0pp** |
| N1 | `peakNonDepositedFraction` | 固定 W + 每提交 + `(U+P)/E` | **52%**（t=4150） | **分子/分母效应 +7.5pp** |

- legacy→新量净差 **+5pp**（47%→52%），**不得**归因于物理改善：其中窗口 −5.5pp、密度 +3.0pp、口径 +7.5pp。
- 同账本其它披露：`legacyPeakHistoricalReference=44.5%`、`legacyPeakLiveFormula=44.5%`、`sameReferenceSuspOnlyPeak=44.5%`（三者同在 t=4100，该提交 `P=0`）；`OBSERVED_SAMPLED_MAX=46%`（W 内样本 `f` 最大值，t=4250）**是真峰 52% 的下界**，且 46%<0.5 会假 FAIL（即 P7 机制）。
- 初态/响应：`baselineNonDepositedFraction=47%`（shake-on 前最近提交）、`peakNetIncreaseFraction=5%`。
- 峰值提交分项：`suspFraction=32%`、`airborneFraction=20%`、`depositedFraction=48%`；H/L 分相 heavy 66.67% / light 30.00%（§7）。
- 恒等式自检（峰值提交，守恒且 `D+U>0`）：`f−l=0.20=P/E` ✓；`f−h=0.12=P·D/[E·(D+U)]` ✓（执行器 `identityChecks` 两 `Matches` 均为 true）。**不得**把与历史分母的差无条件写成 `P/E`。
- legacy 历史分母失效情形：C 全部入粒子时 `D+U=0` → `legacyHistoricalReferenceStatus=INVALID_DENOMINATOR`，不阻断新量计算。

## 7. 机制证据三：分相规则与可达性（执行器 `phaseSplit` / `reachability` 段）

- 总量峰值提交（t=4150）同时记录 H/L：heavy 66.67%、light 30.00%。
- 独立分相峰值：heavy 66.67%（t=4150）、light **49%**（**t=4200**，另一提交）；两者之和 115.67% ≠ 总量峰值 52% —— 独立分相峰**不可相加**，且可发生在不同提交。
- `E_phase=0` 记 `N/A`（如 `E_H=1000, E_L=0` 时轻相 `N/A`），不得 EPS 造比例。
- 库存恒等式（合成账本逐提交复算）：`M(k)=M0+cumAcceptedLift−cumSettledOrLanded`，最大绝对残差 **0.0**；`M0=470`、W 内 `cumAcceptedLift=115`、`cumSettledOrLanded=403`、`peakM=520`、`tau_W=3.0s`；`peakM≤M0+cumLift` ✓、`cumLift≤300·tau_W` ✓。
- 可达性必要条件（冻结算术，执行器 `necessary` 段）：`M0=0, tau_W=3.0s` → 最乐观峰 ≤900 units，50% 需 `E≤1800`；`E=8000` → 需 lift 积分 ≥**13.33s**。本合成账本满足必要条件（`M0+300·tau=1370≥500`）且峰值为 52%，说明必要条件**不是充分条件**的补证仍以实际账本为准。

## 8. 反例执行结果（协议 §6 八条，全部 MATCH）

| # | 反例 | 关键数值 | 结论 |
|---|---|---|---|
| P1 | 表示漏计 | `E=1000, D=480, U=320, P=200`：`h=40%`、`l=32%`、`f=52%` | 旧量假 FAIL，新量 52%（分档 6）；旧结果不倒改 |
| P2 | 降级奖励旧量 | 200 全 DEGRADE 后 `h` 40%→52%，`f` 恒 52% | 旧结论改变≠扬起改善 |
| P3 | 跨时刻加峰值 | k1 `f=40%`、k2 `f=40%`；分项相加 80% | 正确总量峰 40% 不及格 |
| P4 | 输入平台响应滞后 | 首提交 `f=10%`，平台 2.4s `f=55%` | 输入峰时刻法与库存峰是不同估计对象 |
| P5 | 全序列污染 | 静置段 80%、摇晃段 40% | 全序列 max 假 PASS（分档 8）；固定窗 40% 不及格（分档 4） |
| P6 | 接触库存伪装运动 | `U=20, P_state2=500` → `f=52%` 数值及格 | 不得解读为 52% 真实飞行/自然悬浮 |
| P7 | 250ms 漏峰 | 真峰 52%、样本 max 48% | 稀疏采样假 FAIL；必须保留全部成功提交 |
| P8 | 停止后才高 | 摇晃段 48%、off 后 55% | 不扩大窗口求 PASS；endpoint 独立算 |

另：分档边界表 12 行 + 0.5 比较 4 行全部 MATCH（`0.90→10 / 0.75→8 / 0.50→6 / 0.35→4 / 0.20→2 / <0.20→0`；`M=500,E=1000` 恰及格、`500−1e-12` 不及格，无新放宽容差）。

## 9. 转绿条件（字段 → 用例 → 验收链）

1. architect 登记独立 peak 字段 revision（复用 commit/elapsed/particles 契约，补 shake-on/off 有效边界与子步输入消费身份）。
2. developer 实现摇晃段每提交采样与流式峰值，`water_sand_check` 按适配清单落地（不改 legacy 输出身份）。
3. tester 按 `docs/test-cases.md` §15（`TC-P011-PEAK-01..08`）执行：字段齐备后先在合成/归档账本上复算 §5–§7 全部数值，再等同 build 同输入真机账本出 legacy/新量双报告（归因分离）。
4. designer 效果验证（同路线前版为基线）→ manager 确认适用范围并宣布 `peakProtocolEffective`。
5. 生效前：WS-08 摇晃峰值子项继续双口径并报，任一单口径不单独放行；旧 `peakSusp` 保留原身份原结论。

## 10. 红线（本基线遵守项）

- 不把新协议写成 tester 已实现（字段未实现即 `NOT_IMPLEMENTED`）；
- 不用旧 250ms 抽样冒充全子步峰值；`OBSERVED_SAMPLED_MAX` 仅作带标注下界；
- 不改 endpoint 协议正文与已发布版本、不改判定式/阈值/窗口/终点定义与双算规则；
- 不改 `DUALFIELD-*`  estimand 与冻结指标（30°±2°/t90/R²/5%）；
- 不追认旧 route 或旧 build 结果、不改历史报告结论；
- 不宣布 `peakProtocolEffective`（宣布权在 manager）；不代 developer 实现采样与流式峰值；不使用真机。
