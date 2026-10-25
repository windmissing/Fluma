# P-011 WS-08 摇晃峰值：现行 checker 全序列 max 适配清单 v1

日期：2026-10-25。责任：tester（P-011）。状态：**CHECKLIST_FROZEN / NOT_IMPLEMENTED（适配实现归 developer）/ NO_CODE_CHANGE_BY_TESTER**。
依据：`docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（本清单的判据来源）、`docs/design-P011-ws08-peak-nondeposited-migration-v1.md` §2.1/§5、`docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（终点协议，不改）。
适配对象：`test/water_sand_check.py::evaluate_shake()`（第 476–492 行）。**本清单不改任何代码**；实现归 developer，验收归 tester。

## 1. 现行实现逐条（源码事实，2026-10-25 核对）

| # | 现行行为 | 源码位置 | 问题 |
|---|---|---|---|
| C1 | `has_shake = any(event["type"]=="shake")`：只查 shake 事件**存在性**，不取时刻、不取 on/off 边界、不取输入值 | `evaluate_shake` 第 479 行 | 无法绑定固定 3s 窗；窗口右边界不可证 |
| C2 | `values = [row["suspensionFraction"] for row in rows]`：取**全部** `WS_METRIC` 行（整个 run 序列） | 第 480 行 | 静置段/停止后样本混入峰值 |
| C3 | `peak = max(values)`：**全序列 max** | 第 483 行 | 反例 P5：初始静置段高值可造成假 PASS |
| C4 | `residual = values[-1]`：最后一条样本当残余 | 第 484 行 | 非 endpoint 协议固定终点（shake-off 后满 8.0s 的最后一个已提交子步）；残余判定已归 endpoint 协议 |
| C5 | `passed = peak >= 0.5 and residual <= 0.05`：二元判定 | 第 485 行 | 无 TIERED 分档（10/8/6/4/2/0）；`peakSusp` 从未产出过档位分 |
| C6 | 缺 shake 事件或缺样本 → `blocked("shake", [...])` | 第 481–482 行 | BLOCKED 语义可保留，但缺字段清单须按新协议列确切路径 |
| C7 | 分子只用 `suspensionFraction`（历史公式 `U/max(D+U,EPS)`），250ms 抽样 | 第 480 行 + `getMetrics()` 源码事实 | 分子漏计真实粒子 `P`（含 state=2）；密度非每提交 |

**结论**：现行 `evaluate_shake` 的 `peakSusp` 是"全序列、250ms 抽样、列悬沙口径、二元判定"的量，与书面用例 TC-WS-08 的"摇晃段峰值"语义自始存在适配缺口（设计稿 §2.1 已判定为"既有脚本与书面用例的适配缺口，不是已获批的另一估计对象"）。

## 2. 差异点逐条（旧口径 → 新口径）

| # | 维度 | 旧（现行 `peakSusp`） | 新（`peakNonDepositedFraction`） | 协议落点 |
|---|---|---|---|---|
| D-1 | 测量窗 | 全序列（rest+shake+post 全部样本） | 固定 `W=[t_on, t_on+3000ms)`，左闭右开 | peak 协议 §2.1 |
| D-2 | 采样密度 | 250ms `WS_METRIC` 抽样 | W 内**每个成功提交子步** | §2.2 |
| D-3 | 分子 | `U`（列悬沙） | `M=U+P`，`P` 含 `pState∈{1,2}` 全部活粒子（含接触待准入） | §1.1 M1–M8 |
| D-4 | 分母 | 历史公式 `D+U`（`getTotals()` 不含粒子桶） | `E=VH0+VL0+IH+IL`（WS-08 无外源时 =`VH0+VL0` 恒定） | §1.2 |
| D-5 | 输入资格 | 仅存在性 | on/off 实际生效边界 + 本子步消费的输入身份 + 连续性（`sourceEpoch/reset/invert/route/模式` 恒定） | §2.1/§2.2 |
| D-6 | 评分 | 二元 `peak>=0.5 and residual<=0.05` | TIERED 10/8/6/4/2/0；及格比较 `M≥0.5E`，无新放宽容差 | §1.3 |
| D-7 | 残余 | `values[-1]` | **不在本协议**；委托 endpoint 协议固定终点（shake-off 后满 8.0s 最后已提交子步） | §3/§7 |
| D-8 | 峰值提交披露 | 无 | `peakCommitKey`（并列留最早）、峰值提交 `D/U/P/E` 与三个显示分项、H/L 分相同时记录 | §3/§1.4 |
| D-9 | 状态码 | BLOCKED/PASS/FAIL | 五码分流（`NOT_IMPLEMENTED`/`INVALID_LEDGER`/`INVALID_BASELINE`/`INVALID_DENOMINATOR`/`INCOMPLETE`）+ 诊断码 | §4 |
| D-10 | 双算 | 无 | `legacyPeakSuspReported` / `legacyPeakHistoricalReference` / `legacyPeakLiveFormula` / `sameReferenceSuspOnlyPeak` / `OBSERVED_SAMPLED_MAX`，差异归因分离 | §5 |

## 3. 结论影响面

### 3.1 会因此改变的结论（迁移后必须重算，且不得追认旧值）

1. **任何引用旧 `peakSusp` 数值的摇晃强度结论**：窗口污染（D-1）、密度漏峰（D-2）、分子漏计（D-3）三者都可单向或双向改数值；旧数值不构成新协议的合格证据。
2. **TIERED 档位分从未被现行脚本产出**：历史 `shake` 检查只有二元 PASS/FAIL/BLOCKED；迁移后首次出现 10/8/6/4/2/0 档位，历史报告无档位分可对照（不补造）。
3. **"peakSusp≥0.5 ⟺ 摇晃达标"的等价性不成立**：新口径下同轨迹可出现 legacy 报告 47% 而新量 52%（或反向），差异必须按窗口/密度/分子分母归因分解（见 RED 基线记录 §5 归因链）。
4. **BLOCKED 的原因字段变化**：旧 BLOCKED 因缺 `WS_EVENT shake`/`suspensionFraction`；新 BLOCKED/NOT_IMPLEMENTED 因缺 §10 报告契约字段（`particles`、`endpointSampleElapsedMs`、on/off 边界等）。

### 3.2 不受影响的结论（适配不得触碰）

1. 历史报告原 PASS/FAIL/BLOCKED 结论（含 2026-10-13 RED 基线的 `shake=BLOCKED`）——不追认、不推翻、不改写。
2. endpoint 协议的终点残余与末 1s 质量窗判定及其全部身份字段。
3. 全部安全硬闸门（tan30、四层预算、守恒 1e-9、库存、容量、有限值、账本对齐、边身份）。
4. `DUALFIELD-*` 16 例 estimand 与 GES 五观测点；冻结指标 30°±2°/t90/R²/5%。
5. 旧 `peakSusp` 字段本身：保留原名、原公式、原样本与原窗口，标 `legacy-susp-only`；迁移期双报，另案获批并完成独立协议/实现/验证后才替代 C 的峰值子项。
6. `evaluate_shake` 中与峰值无关的其它 evaluator（conservation/sorting/settling/angle/water/invert/long_rest/fps/rate）——本清单只针对 shake 峰值路径。

## 4. 适配后 checker 的期望行为（逐条可执行）

### 4.1 选取与计算

| # | 期望行为 | 对应反例/约束 |
|---|---|---|
| A1 | 从输入接受证据取 `t_on`，窗 `W=[t_on, t_on+3000ms)`；超过 3000ms 的样本不计入；提前 off 或窗内输入非原 1.0 → `INCOMPLETE` | P8；协议 §2.1 |
| A2 | 窗内取每个成功提交子步的 post-commit `stateAfter`+`particles`；同提交先求和 `M=U+P` 再取 max | P3；§3 |
| A3 | 并列最大值留**最早**提交键；输出 `peakCommitKey`/`peakStockMass`/`peakReferenceMass`/`peakSampleElapsedMs` | §3 |
| A4 | 分档按 §1.3 锚点；及格比较 `M≥0.5E` 精确判定（四舍五入不参与） | §1.3 |
| A5 | 250ms 样本只能进 `OBSERVED_SAMPLED_MAX`（下界），不得插值补峰、不得替代每子步峰值 | P7；§5.3 |
| A6 | 分相：`peakNonDepositedHeavyFraction`/`LightFraction`；`E_phase=0` 记 `N/A`；总量峰值提交同时记录 H/L；独立分相峰不相加 | §1.4 |
| A7 | 全序列 max 一律不作为正式峰；仅在 legacy 披露块以原身份输出 | P5 |

### 4.2 资格与状态

| # | 期望行为 |
|---|---|
| A8 | 字段缺失 → `NOT_IMPLEMENTED(missing_fields)`，列确切路径（§10 契约字段）；不得用结构 `P=0` 或旧样本冒充 |
| A9 | 摘要与逐粒不一致/跨子步拼接/非有限值/非单调 elapsed/重复提交键 → `INVALID_LEDGER` |
| A10 | `E≤0` → `INVALID_DENOMINATOR`；初值身份不可证 → `INVALID_BASELINE` |
| A11 | 窗覆盖不足/断档/提前退出/窗内 `sourceEpoch` 或 reset/invert/route/模式变化 → `INCOMPLETE` |
| A12 | 硬闸门失败 → 只留原始候选峰值诊断，不签正式峰值 PASS（硬闸门优先级最高） |
| A13 | 残余列不再由 shake 评估器产出；endpoint 结果以引用方式并列披露，两时段分别判定 |

### 4.3 双算输出（同 run 同账本）

- `legacyPeakSuspReported`：旧原字段、原样本/原窗口（现行=全序列 max），标 `legacy-susp-only`，保留原 PASS/FAIL；
- `legacyPeakHistoricalReference`：同 W 逐子步 `max h`（`h=U/(D+U)`）；`D+U=0` 时记 `legacyHistoricalReferenceStatus=INVALID_DENOMINATOR`，不阻断新量；
- `legacyPeakLiveFormula`：同 W 逐子步 `max l`（`l=U/max(A,EPS)`，`A=D+U+P`），EPS 钳位如实报告；
- `sameReferenceSuspOnlyPeak`：`max_W U/E`，独立命名；
- 差异归因：窗口效应、密度效应、分子/分母效应**分开列**（框架见 RED 基线记录 §5），不得把旧峰与新峰之差全归因于物理改善；
- 自检恒等式（守恒且 `D+U>0`、同提交）：`f−l=P/E`、`f−h=P·D/[E·(D+U)]`；不得无条件写成 `P/E`。

### 4.4 可达性必要条件（适配后报告须能复算）

- 恒等式 `M(k)=M0+cumulativeAcceptedLift(k)−cumulativeSettledOrLanded(k)`；
- `peak M ≤ M0+cumulativeAcceptedLift(W) ≤ M0+300·tau_W`（`tau_W`=W 内成功子步 `ΣphysicsDt`，单位秒）；
- 必要条件 `M0+300·tau_W ≥ 0.5·E`；不满足时如实报告目标/场景/模型冲突，交 manager 上报用户另裁；
- **不暗增 300/s、不延长摇晃、不调低 0.5、不减总沙量求绿**。

## 5. 迁移期行为与红线

- 协议生效前：legacy 照常输出（原 PASS/FAIL 不变），新口径缺字段输出 `NOT_IMPLEMENTED` 并列缺失路径；**任一单口径不得单独放行**。
- 不得向旧账本补造 v7 字段；不得用 250ms 旧数据冒充全子步峰值；`OBSERVED_SAMPLED_MAX` 必须带下界标注。
- 不代 developer 实现采样与流式峰值；不修改已发布用例判据；不宣布 `peakProtocolEffective`。
- 真机采集由 manager 交 effector 串行排队；本清单不占设备。
