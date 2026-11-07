# P-011 终点未沉积沙比例口径裁决记录 v1

日期：2026-10-24。裁决人：用户（项目负责人）。登记：manager。状态：**APPROVED / 协议未生效 / 未实现 / 未验证**。

依据：[终点未沉积沙比例方案定稿 v1.1](design-P011-terminal-nondeposited-fraction-v1.md)（designer，§7 为权威版本）+ [探索复盘](retro-P011-terminal-nondeposited-v1.md)；manager 已逐条复核 designer 引用的五处源码事实（getMetrics().suspensionFraction 分母、depositedFraction 与 t90 的口径分叉、注入分支 epoch 空操作、shake 输入的 tilt 派生耦合、nextP011TimeMs 时钟性质）全部属实。

## 1. 裁决内容（逐项）

| # | 裁决项 | 裁定 |
|---|---|---|
| 1 | 终点分子 | **列悬沙 + 全部活粒子质量**（`U + P`，含 `state=1` 飞行与 `state=2` 接触待准入；`state=0` 空槽不计；DEGRADE/TO_SUSP 的累计事件量永不计入分子；MERGE 后只计现存包质量一次） |
| 2 | 分母 | **各用例历史分母不变**：`TC-P011-SLIP-03` 用 `R_SLIP`=run 首子步 `stateBefore` 的 Σ(bh+bl)，整 run 冻结；`TC-WS-08` 用 `R_WS`=累计期望 `E=VH0+VL0+IH+IL` |
| 3 | 阈值 | **5% 不变**（不据粒子容量反推新阈值） |
| 4 | 采样窗 | **新增**：固定终点前最后 1.0s 内每个已提交子步均须 `M(k) ≤ 0.05×R(k) + 1e-9`（`epsMass=1e-9 unit`，复用现 SLIP checker 绝对容差）；单列披露 `endpointFraction` 与 `windowPeakFraction`；终点前须无新外源准入/摇晃/reset/invert/模式或 route 切换（`sourceEpoch` 恒定） |
| 5 | `TC-WS-08` 的 `peakSusp ≥ 0.5` | **另案裁决**，不并入本次 revision；本 revision 不以新 `peak(U+P)/E` 替代该冻结估计对象 |

附加采纳（方案 §7.2–§7.4 内的配套规定，随本次裁决生效）：

- 分项披露字段固定为 `suspFraction=U/E`、`airborneFraction=P/E`、`depositedFraction=D/E`（共用 E，守恒通过时和约为 1）；`WS_METRIC.suspensionFraction` 保留旧公式与旧名，不得与新字段互为别名。
- 正式判定必须从**原始 v7 账本**重算（`SLIP-03` 用 `M/R_SLIP`），**不得**直接用显示分项之和 `M/E` 判定；报告须并列 `R_SLIP`/`E`、`U`、`P`、`M` 及各自比例，避免两个「5%」混名。
- 固定终点定义：SLIP 取预注册每相位最后一个已提交子步；WS-08 取 shake-off 生效后满 8.0s 的最后一个已提交子步；**不得**挑曲线上首次低于阈值的子步、不得延长观察等绿、不得用 250ms `WS_METRIC` 抽样替代每子步账本。
- SLIP 各相位**不额外要求 `shake=0`**：现行 `pages/SandCanvas.ets` 在 thick 分支每帧执行 `setShakeInput(min(1,|effectiveTilt|/90))`，属既有场景耦合，本次只改终点量、不改该耦合。
- 状态分类沿用方案：字段缺失 `NOT_IMPLEMENTED`、账本/基线无效 `INVALID_LEDGER`/`INVALID_BASELINE`、覆盖或输入身份不足 `INCOMPLETE`、分母非正 `INVALID_DENOMINATOR`；均不得记 PASS。
- 滞留诊断：`state=2` 永久待准入不因速度小/贴床/超时/池满而从 P 扣除；持续滞留输出 `PERSISTENT_PARTICLE_RESIDUE` 并交 manager 按既有无永久悬浮/生命周期证据审查；**5% 数值 PASS 不替代其余出口条件**。

## 2. 生效条件与不追溯

- 本裁决**批准的是口径与新增末窗要求**，不是新阈值、不是新的粒子寿命标准、不是效果 VALIDATED。
- 协议**正式生效**前：C run 继续**双口径并报**（旧 `susp`-only 与新 `U+P` 并列），任一单口径都不得单独放行；旧报告保留原 PASS/FAIL 与 `legacy-susp-only` 身份，不追认旧验收。
- 回算：仅当原始 v7 账本具备真实粒子摘要与同子步时间戳时方可离线补算候选值，标 `RETROSPECTIVE_DIAGNOSTIC`。
- 效果验收仍需：获批 revision 落地 → 实现 → 路线专属客观/主观双报告 → designer 验证 → manager 确认适用范围。

## 3. 执行序列（按方案 §7.4 迁移步骤）

1. tester 发布 endpoint protocol revision（`TC-WS-08`/`SLIP-03`；保留 DUALFIELD v1 estimand 与历史结果；峰值口径如要改另起 revision）。
2. architect 对齐字段 revision（post-commit 同子步采样身份、elapsed 采样字段 `endpointSampleElapsedMs`、`particles.byState.{FLYING,CONTACT_WAIT}` 诊断扩展、`integratedPhysicsSeconds` 诊断披露；字段名与 tester revision 一致）。
3. developer 实现（v7 摘要与三分项输出、endpoint 采样、checker 同 run 双算旧/新并校验 `D+U+P≈E`）。
4. 同 build 同输入跑路线自身基线与候选，出路线专属双报告（OFF/B 的粒子质量取结构 P=0，但**不得**向旧文件补造 v7 字段）。
5. designer 验证 → manager 确认适用范围。

## 4. 与其他冻结项的关系

- `30°±2°`、target-based `t90 ≤ 5100ms`、`R² ≥ 0.90`、原阶段适用性全部保留；新残余 PASS **不得**追认 `SLIP-02` 或 P-011 CLOSED。
- `target-based` 坡度 t90 与沉降 t90 区分：在途粒子不得算作已沉积；活动源 epoch 不沿用旧首次 90%（`CONS-04`：真实沉积分相质量/累计期望、跨 epoch 无效）。
- `DUALFIELD-*` 16 例的 CONS/BUD/LIFE/SRC/GES estimand 原样保留；GES 五观测点零改动，不另造 5% PASS。
- P-012 `E2`/`E4` 不从本指标推出 PASS；`E4` 沙丘 mask 只取真实沉积床层，飞沙云与接触待准入粒子不并入。

## 5. 待另案（不在本次裁决内）

- `TC-WS-08` `peakSusp ≥ 0.5` 的迁移：另起 protocol revision 与用户裁决（designer 出方案）。
- 沉降 t90 的实现口径：按 `CONS-04`（累计期望分母 + 跨 epoch 无效）落地，developer 修复现存 epoch 重启空操作与 light 分量缺失；该修复不属本次 revision，但口径依据一致。
