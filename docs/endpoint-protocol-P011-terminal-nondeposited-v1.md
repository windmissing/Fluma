# P-011 终点未沉积沙比例 endpoint protocol revision v1

日期：2026-10-24。发布：tester（P-011）。状态：**METHOD_FROZEN / APPROVED_MANDATE / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_KALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION**。

## 0. 身份与适用范围

| 项 | 取值 |
|---|---|
| `protocolId` | `p011-endpoint-terminal-nondeposited` |
| `protocolRevision` | `endpoint-terminal-nondeposited-v1` |
| `endpointMetricRevision` | `terminal-nondeposited-v1.1`（报告字段取值，见 §6.1） |
| `endpointAuthority` | `APPROVED` |
| 依据裁决 | `docs/decision-P011-terminal-nondeposited-fraction-v1.md`（用户 2026-10-24 批准，5 项逐条裁定 + 配套规定） |
| 依据设计 | `docs/design-P011-terminal-nondeposited-fraction-v1.md` §7（v1.1 定稿） |
| 依据预注册 | `docs/preregistration-P011-dual-field-particle-v1.md`（CONS/BUD/LIFE/SRC/GES estimand **原样保留**，本 revision 不改一个字） |
| 依据字段权威 | `docs/architecture-P011-dual-field-particle-v1.md` §5.6（v7 `particles` 块、七类事件字段名） |
| 现行 checker | `test/p011_slip_formal_check.py`（SLIP 侧）、WS 侧残余项采集（`pages/SandCanvas.ets` `WS_METRIC`） |
| 历史结果身份 | `docs/test-report-P011-slip-v1.md`（旧口径 `legacy-susp-only`，原 PASS/FAIL 保留，不追认） |

**本 revision 冻结什么**：终点未沉积沙比例的分子、分母、阈值、末 1s 采样窗、固定终点定义、采样与身份契约、时钟契约、状态分类、双算规则、不涉及项（§1–§9），并给出现行 checker 的适配说明（§7）。

**本 revision 不做什么**：不实现、不改代码、不改既有 estimand、不改冻结指标、不改历史报告结论、不授权采集、不追认旧 route 结果。

**适用对象**：路线 C（真实活动粒子 + 128 列高度场双场）的终点效果判定。OFF/B 路线无真实粒子层时，候选数学对照可取结构 `P=0`，但不得向旧文件补造 v7 字段，且不要求 C 对其他路线全量占优——只在同路线内比较基线改善。

## 1. 冻结口径（分子 / 分母 / 阈值 / 容差）

### 1.1 分子 M = U + P

同一**成功提交子步** `k` 的 post-commit 库存，全部为 model material-volume units（与 `bh/susp/pMass` 同单位；位置、速度、粒子个数、像素面积都不是质量代理）：

```text
D_H(k) = sum(stateAfter.bh) + sum(stateAfter.mh)
D_L(k) = sum(stateAfter.bl) + sum(stateAfter.ml)
U_H(k) = sum(stateAfter.suspH);  U_L(k) = sum(stateAfter.suspL)
P_H(k) = particles.massH;        P_L(k) = particles.massL
D = D_H+D_L;  U = U_H+U_L;  P = P_H+P_L;  M = U+P
```

分子细则（逐条可执行，缺一即判 INVALID_LEDGER）：

| # | 细则 | 说明 |
|---|---|---|
| M1 | `P_H/P_L` 必须等于 `pState∈{1,2}` 的 `pMass` 按 `pPhase` 求和 | `pState` 语义见架构稿 §1.2：`0=空槽 1=飞行 2=静置待准入`；`state=0` 空槽**不计** |
| M2 | 水下、水面上方、接触待准入的粒子**全部计入** `P` | `airborne` 不作“位于空气中”的字面解释 |
| M3 | `DEGRADE`/`TO_SUSP` 的**累计事件量永不进分子** | `particles.degraded.{count,massH,massL}` 是累计量，不是当前存量；降级后该质量以 `U` 计一次 |
| M4 | `MERGE` 后只计现存包质量**一次** | 不得把合并前各包质量与合并包相加 |
| M5 | `LAND` 只扣 `granted` | 已提交进 `mh/ml` 的 granted 部分计 `D`；余量留原粒 `state=2` 继续计 `P`（`granted+remainder==requested`） |
| M6 | `TO_SUSP` 若被显式启用，提交后该量只计 `U` | 架构稿 rev1 的 T3 固定保留原粒，`TO_SUSP` 本版不启用，但口径先冻结 |
| M7 | B tracer 无质量，一律不计 | `P012SandTracer` 是纯视觉层 |
| M8 | 摘要与逐粒缓存不得同时相加 | 摘要与逐粒求和不一致即 `INVALID_LEDGER` |

**守恒先决**：每个已提交子步须先过冻结守恒 `abs(D_H+U_H+P_H-E_H)≤1e-9`（L 同理）。任一未过，不给正式终点 PASS。

### 1.2 分母 R（各用例历史分母不变）

| 用例 | `endpointReferenceKind` | `R` 定义 | 冻结理由 |
|---|---|---|---|
| `TC-P011-SLIP-03` 每相位 | `SLIP_INITIAL_BH_BL` | `R_SLIP` = run **首子步** `stateBefore` 的 `Σ(bh+bl)`，**整 run 冻结** | 原正式 5% 门槛只扩分子，不移动冻结基准；`mh/ml` 未在旧基准内，不得顺手加入 |
| `TC-WS-08` 摇晃后静置 8s | `EXPECTED_TOTAL` | `R_WS` = 累计期望 `E = VH0 + VL0 + IH + IL` | 与历史 `totalSand` 在守恒通过时数值等价；含已准入外源**一次** |

- `VH0/VL0` 取 reset 后、任何外源准入前的**首条** `audit.heavyTotalBefore` / `audit.lightTotalBefore`（账本顶层 `audit` 块，现制字段已在 `WaterSandField.ets` 审计块内）。
- 采集若从注入后开始且无法证明初值身份 → `INVALID_BASELINE`；**不得**把已注入库存再加一遍 IH/IL。
- `R ≤ 0` → `INVALID_DENOMINATOR`；不以 EPS 替代使空沙场景通过。
- SLIP run 若在首子步后发生外源注入 → 场景身份不符，须重跑；不得仍用旧 `R_SLIP` 却把新增沙纳入分子。

### 1.3 阈值与容差

| 项 | 取值 | 来源 |
|---|---|---|
| 阈值 | **5%**（不变） | 裁决第 3 条；不从粒子容量反推新阈值 |
| `epsMass` | `1e-9` material-volume unit（绝对容差） | 复用现 SLIP checker `EPS = 1e-9` |
| 判定式 | `M(k) ≤ 0.05 × R + epsMass` | 逐子步 |
| 比例展示 | 四舍五入**不参与判定** | 质量/分相守恒闸门仍用各自原容差 |

**新旧同分母关系（恒等式，用于自检）**：`F_new − F_old = P/R`（同分母下只加粒子时旧 FAIL 不会变 PASS）；若 `P=0` 则新旧数值相同。

## 2. 固定终点与末 1s 采样窗

### 2.1 固定终点 t_e（禁止挑选）

| 场景 | t_e | 依据 |
|---|---|---|
| `TC-P011-SLIP-03` | 每**相位 occurrence** 的最后一个已提交子步 | 裁决配套规定；预注册相位边界 |
| `TC-WS-08` | shake-off 生效后**满 8.0s** 的最后一个已提交子步 | 现行 WS-08 的 8s 静置窗口不得缩短 |
| `TC-WS-09` | 原采样段（静置 10s、末段 5s）不变；新末 1s 窗**嵌在**既定末段内 | 裁决配套规定 |

禁止项（违反即 `INVALID_LEDGER` 或 `INCOMPLETE`）：

- 不得挑曲线上**首次**低于阈值的子步；
- 不得延长观察等绿；
- 不得用 250ms 的 `WS_METRIC` 抽样替代每子步账本；
- 不得用 `stateBefore`（粒子提交前）与提交后的悬沙相加。

### 2.2 末 1s 质量窗（新增要求）

- 窗口 = 固定边界 `t_e` 前闭区间 `[t_e − 1000ms, t_e]`，含边界处已提交状态。
- 窗内**每一个成功提交子步**均须 `M(k) ≤ 0.05×R + epsMass`；等价 `windowPeakFraction = max_{k∈窗} M(k)/R ≤ 0.05 + epsMass/R`。
- 单列披露两个量：`endpointFraction = M(t_e)/R`（终点值）与 `windowPeakFraction`（窗内峰值）。二者**不得**互相替代。
- 窗口资格：窗内须无新外源准入、无摇晃输入、无 reset/invert、无模式或 route 切换，即 `sourceEpoch` 恒定；不满足 → `INCOMPLETE`（记 `ENDPOINT_NOT_QUIET`），由既定 schedule 重跑，不以重选窗口求绿。
- 窗口覆盖不足（未覆盖完整 1s、或缺末端子步）→ `INCOMPLETE`；**不外推、不插值、不拿 250ms 抽样代替**。若物理进入合法休眠，可用带时间/身份的只读状态复核证明同一提交状态保持至边界；**不得伪造新提交子步**，冻结/失败回滚、漏记子步、采集提前终止不能作为休眠证据。
- SLIP 的**末 1s 坡度变化 ≤1°** 是既有冻结安全/稳定不变量，**独立执行**，不受本质量窗影响；两个窗并存、各司其职（见 §7.4）。
- WS-08/09 原时序不变：WS-08 仍是原 8s 静置窗口，不能用这 1s 缩短等待。
- 持续手势没有静置终点：连续手势不是静置终点，只能在另行预注册的 stop+观察窗附本指标诊断（`DUALFIELD-GES-*` 另案，见 §8）。

### 2.3 SLIP 相位输入资格

SLIP 各相位**不额外要求 `shake=0`**：现行 `pages/SandCanvas.ets` 在 thick 分支每帧执行 `setShakeInput(min(1,|effectiveTilt|/90))`，属既有场景耦合；本 revision 只改终点量，不改该耦合。SLIP 依原输入仍须通过未沉积量判据，失败如实 FAIL。

## 3. 采样与身份契约

### 3.1 post-commit 同子步读取

- 终点量必须取**同一成功提交子步**的 `stateAfter` 与 `particles` 摘要（post-commit 存量）。
- 禁止跨子步拼接（提交前 `stateBefore` + 提交后 `stateAfter` 的悬沙相加）。
- `context.physicsTime`（现制取自 `nextP011TimeMs()`）用于各自阶段审计，**不要求时间戳逐值相等**（context 在 begin 取值、metric 在 finish 后另取值）。

### 3.2 连接键与 phase occurrence 身份

- 连接键：`(runId, sequence, frameId, substepId)`。
- `phase occurrence` 用**命令/阶段身份**区分（`context.phaseOccurrenceId`：同一 `phaseD` 的第 n 次出现），不把重复同名 phase 拼接为一个组。现行 checker 按 `context.phaseD` 分组，重复同名相位会被合并——实现须改用 occurrence 身份分组。
- 字段名 `phaseOccurrenceId` 由 architect 在字段 revision 中登记；若 architect 另定名，须出对齐结论并同步本 revision 的 §11 用例条目。

### 3.3 时钟契约

| 项 | 冻结规则 |
|---|---|
| 窗口时钟 | **单调实际 elapsed 时钟**（相对 run 起点），与命令接受/阶段边界同源 |
| 字段 | `context.endpointSampleElapsedMs`（成功提交后取；新字段，由 architect 登记） |
| 禁止 | 不得把 `sum(physicsDt)` 当真实静置时间；不得用现制 `context.physicsTime` 冒充 elapsed。`nextP011TimeMs()` 取自 `Date.now()` 且多次调用递增，既不是纯 simulationTime，也不能单独证明完整 1s 覆盖 |
| 诊断披露 | `context.integratedPhysicsSeconds = ΣphysicsDt`（仅诊断，不参与判定） |
| 缺 elapsed | 无 `endpointSampleElapsedMs` 时，**新口径**判 `NOT_IMPLEMENTED(missing_field)`；legacy 口径继续按原判据输出，两口径身份分列 |

### 3.4 采样密度

- 终点判定必须基于**每子步账本**；`WS_METRIC` 的 250ms 抽样只可作旁证与诊断，**不得**替代每子步窗口判定。

## 4. 状态分类与判定顺序

### 4.1 判定顺序（顺序不可调换）

```text
1. 路由与协议身份校验（particleRouteActive/particleRouteId；routeId 归属）
2. 硬闸门：守恒 / 库存 / 容量 / guard / 预算 / 有限值 / 账本对齐 / 边身份
3. 资格：基线身份（R_SLIP 冻结、E 初值）、终点/窗口覆盖、窗口安静性
4. 效果终点：分子/分母/末窗达标
```

**硬闸门失败不得用 5% PASS 掩盖**：任一硬闸门失败 → 该 run 整体不合格，先报失败闸门，不给效果终点 PASS。

### 4.2 状态码（均不得记 PASS）

| 状态码 | 触发条件 |
|---|---|
| `NOT_IMPLEMENTED(missing_field)` | 必要字段缺失（须列确切字段路径，不用替代量冒充） |
| `INVALID_LEDGER` | 账本无效：摘要与逐粒不一致、跨子步拼接、非有限值、跨子步时间不单调连续可核 |
| `INVALID_BASELINE` | 初值身份不可证（无法证明 `VH0/VL0` 取自准入前） |
| `INVALID_DENOMINATOR` | `R ≤ 0` |
| `INCOMPLETE` | 末 1s 窗覆盖不足、缺末端子步、窗口不安静（`ENDPOINT_NOT_QUIET`）、输入身份不足 |
| `PERSISTENT_PARTICLE_RESIDUE` | **诊断码**（非状态码）：终点仍有 `state=2` 接触待准入粒子持续存在 |
| `NOT_OBSERVABLE` | 逐粒/状态时间证据不存在（见 §4.3） |

### 4.3 滞留诊断的两级结论（必须分开写）

| 结论 | 可观测条件 | 表述 |
|---|---|---|
| 终点态滞留 | 终点摘要给出 `byState.CONTACT_WAIT` 计数/质量 | 可报 `PERSISTENT_PARTICLE_RESIDUE`，并交 manager 按既有无永久悬浮/生命周期证据审查 |
| 长期不落 | 需**逐粒年龄/状态时长**证据 | 架构稿 §1.2 明确**不存 age/seed**，故 rev1 无逐粒滞留时长字段 → 记 `NOT_OBSERVABLE`，**不得**宣称长期不落已排除，也不得由总量反推后宣称已排除 |

**5% 数值 PASS 不替代其余出口条件**：`state=2` 粒子不因速度小、贴床、超时、池满而从 `P` 扣除；持续滞留照常 FAIL；池满不提供扣除、豁免或第二分母；不凭任意 1s 或假设 1.5s 新设寿命 FAIL 阈值。

## 5. 双算规则与报告身份

### 5.1 双算（同一 run 同时算两口径）

| 项 | legacy 口径 | 新口径 |
|---|---|---|
| 分子 | `U`（列悬沙） | `M = U + P` |
| 分母 | SLIP：`R_SLIP`；WS：`A_hist`（见 §5.3） | SLIP：`R_SLIP`；WS：`E` |
| 窗口 | 现行单终点样本（SLIP checker 现行实现） | 末 1s 窗内**每子步** |
| 身份标记 | `legacy-susp-only` | `endpoint-terminal-nondeposited-v1` |
| 原结论 | **保留原 PASS/FAIL**，不追认也不推翻 | 新判定 |

### 5.2 正式判定与并列披露

- **正式判定从原始 v7 账本重算 `M/R`**（SLIP 用 `M/R_SLIP`，WS 用 `M/E`）；**不得**用显示分项之和 `M/E` 判定 SLIP（两个分母不同：`R_SLIP ≠ E`）。
- 报告须并列：`R`（及 `E`）、`U`、`P`、`M` 与各自比例（`U/R`、`P/R`、`M/R`、`U/E`、`P/E`、`M/E`、`D/E`），避免两个「5%」混名。
- 显示字段定名（与 architect 字段 revision 一致）：`suspFraction = U/E`、`airborneFraction = P/E`、`depositedFraction = D/E`（共用 `E`，守恒通过时和约为 1）。
- `WS_METRIC.suspensionFraction` **保留旧公式与旧名**，**不得与新字段互为别名**。
- 失败闸门与不可观测项**分开记录**（不得把 `NOT_OBSERVABLE` 记成 PASS，也不得把硬闸门失败藏在效果项里）。

### 5.3 legacy 分式的历史分母语义（裁决与设计未显式写明，此处冻结）

现制历史事实（源码核对）：

- `WS_METRIC` 发射的是 `getMetrics()` 的返回（`pages/SandCanvas.ets` 每 250ms 一条）；
- `getMetrics().suspensionFraction = suspended / max(totals.heavy + totals.light, EPS)`，其中 `getTotals()` = `bh+mh+suspH`（重）与 `bl+ml+suspL`（轻），**不含粒子桶** → 历史分母 `A_hist = D + U`；
- `updateMetrics()` 内另有一个同名局部量 `suspended / max(initialHeavy+initialLight, EPS)`，只用于 `dormant` 判定，**未发射**，与本指标无关。

C 实现后若 `getTotals()` 纳入粒子桶，同名 live 值的分母将变为 `D+U+P`，与历史 run 不再逐位可比。因此双算必须分列两个 legacy 量，禁止混用：

| 字段 | 定义 | 用途 |
|---|---|---|
| `legacySuspOnly.recomputed` | 按**历史分母语义** `A_hist = D+U` 重算的 `U/max(A_hist,EPS)` | 与历史 run 逐位对照 |
| `wsMetricSuspensionFractionLive` | 现行代码路径实测发射值 | C run 内的现况披露 |

两者在 C run 中**必然不同**，这不是双算失败，而是引入第三桶的必然结果；报告须分别标注，禁止互为别名、禁止任一单独放行。

### 5.4 回算与历史报告

- **回算**：仅当原始 v7 账本具备真实粒子摘要与同子步时间戳时，方可离线补算候选值，标 `RETROSPECTIVE_DIAGNOSTIC`；标了就不构成协议生效后的验收证据。
- **历史报告**：`docs/test-report-P011-slip-v1.md` 等旧结论**原样保留**，标 `legacy-susp-only` 身份与原分母；**不追认**旧验收，也**不推翻**。

## 6. 冻结字段清单

### 6.1 报告字段（checker 输出）

| 字段 | 类型 | 语义 |
|---|---|---|
| `endpointMetricRevision` | string | 恒为 `terminal-nondeposited-v1.1` |
| `endpointAuthority` | string | `APPROVED`（本 revision 已获用户裁决批准） |
| `endpointReferenceKind` | string | `SLIP_INITIAL_BH_BL` / `EXPECTED_TOTAL` |
| `endpointReferenceMass` | number | `R`（SLIP=`R_SLIP`，WS=`E`） |
| `endpointFraction` | number | `M(t_e)/R` |
| `windowPeakFraction` | number | `max_{k∈末1s窗} M(k)/R` |
| `massWindowPass` | bool | 末窗每子步均达标 |
| `endpointStatus` | string | `PASS`/`FAIL`/`NOT_IMPLEMENTED`/`INVALID_LEDGER`/`INVALID_BASELINE`/`INVALID_DENOMINATOR`/`INCOMPLETE` |
| `endpointWindowStatus` | string | `PASS`/`FAIL`/`INCOMPLETE`/`NOT_IMPLEMENTED` |
| `endpointStatusDetail` | object | 缺失字段路径 / 失败闸门 / 不可观测项，分开列 |
| `legacySuspOnly` | object | legacy 口径原判定 + 原分母 + 原 PASS/FAIL + 身份 `legacy-susp-only` |
| `endpointDiagnosis` | object | `PERSISTENT_PARTICLE_RESIDUE` / `NOT_OBSERVABLE` 等诊断码与依据 |

### 6.2 遥测字段（native 采集 → architect 登记）

| 字段 | 位置 | 语义 | 可选性 | 缺失状态码 |
|---|---|---|---|---|
| `particles.massH` / `particles.massL` | `context.particles` | post-commit 存量质量（= `pState∈{1,2}` 求和） | 必登 | `NOT_IMPLEMENTED` |
| `particles.bySourceKind.*` | `context.particles` | 来源分账（`GESTURE_EXTERNAL`/`SHAKE_LIFT`） | 必登（来源分账用例需要） | `NOT_IMPLEMENTED` |
| `particles.degraded.*` | `context.particles` | 累计降级量（**永不进分子**） | 必登 | `NOT_IMPLEMENTED` |
| `particles.byState.FLYING/CONTACT_WAIT.{count,massH,massL}` | `context.particles` | 滞留诊断扩展 | 可选 | 缺失 → 滞留诊断 `NOT_OBSERVABLE` |
| `context.particleRouteActive` / `particleRouteId` | `context` | 粒子路由身份 | 必登 | 缺失 → 按未实现处理；关闭时出现粒子字段 → `LEAKAGE` |
| `context.endpointSampleElapsedMs` | `context` | 成功提交后、相对 run 起点的单调实际 elapsed（ms） | 必登（末窗判定前提） | 缺失 → 新口径 `NOT_IMPLEMENTED`；legacy 继续输出 |
| `context.integratedPhysicsSeconds` | `context` | `ΣphysicsDt`，**仅诊断** | 可选 | 缺失 → 记 `NOT_OBSERVABLE`，不参与判定 |
| `context.phaseOccurrenceId` | `context` | 相位 occurrence 身份（区分同名重复相位） | 必登（SLIP 分组前提） | 缺失 → 新口径 `INCOMPLETE`（分组不可信） |
| `context.sourceInjectedH/L` / `sourceEpoch` | `context` | 现制 schema v4 字段 | 已在位 | — |
| `audit.heavyTotalBefore` / `lightTotalBefore` | `audit` | `VH0/VL0` 取值来源（首条记录） | 已在位 | 缺失 → `INVALID_BASELINE` |
| `particleEvents[]`（七类） | `context.particleEvents` | 逐笔可溯（`particleId` 全链） | 必登（原子性/余量判定） | 缺失 → `NOT_IMPLEMENTED` |

## 7. 现行 checker 适配说明（改哪些判据、如何双算；本卡不改代码）

以下针对 `test/p011_slip_formal_check.py`（SLIP 侧）。**只给判据与适配要求，实现归 developer**；WS 侧（WS-08/09）在 WS checker 侧实现，口径与本 revision 同构（分母 `E`）。

### 7.1 保留不动的部分（红线）

- `initial_deposit_total = sum(stateBefore.bh) + sum(stateBefore.bl)`（现行实现取首条记录）**即 `R_SLIP`，语义已正确，不改**（不得加 `mh/ml`）。
- 硬闸门：tan30 判据与 `SKIP_TAN30_PHASES` 豁免范围、四层预算判据、守恒/库存/容量/有限值、边身份、账本对齐——**逐字不动**。
- 收敛诊断块（`30°±2°`、target-based `t90≤5100ms`、`R²≥0.90`）**逐字不动**。
- SLIP-03 的 legacy 判定 `suspensionWithin5PercentOfInitialDeposit` 的**原公式与原 PASS/FAIL 保留**（仅改字段名/加身份标记，见 §7.4）。

### 7.2 需要新增的判据

| # | 新判据 | 落点 | 说明 |
|---|---|---|---|
| N1 | 每子步 `M(k) = U(k)+P(k)` | phase 记录聚合 | 现 `phase_records` 只存 `suspH/suspL`，须并入 `particles.massH/massL`（post-commit 存量） |
| N2 | 分子细则 M1–M8 校验 | 聚合时 | 摘要与逐粒不一致 → `INVALID_LEDGER`；不得摘要与逐粒同加 |
| N3 | 固定终点 t_e = 该 **phase occurrence** 的最后一个已提交子步 | 终点选取 | 现按 `context.phaseD` 分组会把同名重复相位拼接；须改用 occurrence 身份分组 |
| N4 | 末 1s 窗按 `endpointSampleElapsedMs` 选取 `[t_e−1000ms, t_e]` | 窗口选取 | 现按 `physicsTime` 选窗（墙钟，非 elapsed）；新窗**另取**，不改 legacy 窗 |
| N5 | 窗内**每子步** `M(k) ≤ 0.05×R + 1e-9` | 质量窗判定 | 输出 `massWindowPass` 与 `windowPeakFraction` |
| N6 | `endpointFraction = M(t_e)/R` | 终点值 | 与 `windowPeakFraction` 分别披露，不得互相替代 |
| N7 | 窗口资格：覆盖完整 1s + `sourceEpoch` 恒定 | 资格校验 | 不足 → `INCOMPLETE`（`ENDPOINT_NOT_QUIET`） |
| N8 | 状态码分流（§4.2 六码）+ 失败闸门与不可观测项分列 | 输出 | 不得把缺字段当 0、不得把硬闸门失败藏在效果项里 |
| N9 | `R ≤ 0` → `INVALID_DENOMINATOR` | 分母校验 | 不以 EPS 替代 |
| N10 | `legacySuspOnly` 块（旧判据 + 原分母 + 原 PASS/FAIL + 身份标记） | 输出 | 与新判据并列输出 |
| N11 | 回算值标 `RETROSPECTIVE_DIAGNOSTIC` | 输出 | 仅当原账本具备真实粒子摘要与同子步时间戳 |

### 7.3 正式判定口径

- 正式判定用 `M/R`（SLIP：`M/R_SLIP`），**不得**用显示分项之和 `M/E`（`R_SLIP ≠ E`；反例见 §10 第 2 行）。
- 报告并列 `R`/`E`、`U`、`P`、`M` 及各自比例。

### 7.4 双窗并存规则（易错点，必须遵守）

现行 checker 用 `physicsTime` 选出的末 1s 样本集，同时服务两件事：**坡度稳定**（`last1sSlopeStable`，冻结不变量）与 legacy 悬沙判定。新口径引入 `endpointSampleElapsedMs` 窗后：

- 坡度稳定判据的**样本选择必须保持现行实现不变**（否则等于改动冻结的坡度稳定判据）；
- 新的质量窗**单独**按 elapsed 选取；
- 两个窗的样本集可能不同，须在报告中分别披露（`legacyLast1sWindow` 与 `endpointLast1sWindow`），不得合并成一个「最后 1s」。

### 7.5 迁移期行为（协议尚未落地时）

- legacy 口径按现行实现照常输出（原 PASS/FAIL 不变）；
- 新口径在缺 `particles` 摘要或缺 `endpointSampleElapsedMs` 时输出 `NOT_IMPLEMENTED(missing_field)`，并列出缺失字段路径；
- **任一单口径都不单独放行**：协议生效前 C run 的终点验收必须双口径并报；
- 不得向旧账本补造 v7 字段，不得用结构 `P=0` 冒充实测粒子质量（OFF/B 路线且无真实粒子层时，只能作为标注清楚的数学对照）。

## 8. 本 revision 不涉及项（逐项列明）

| # | 不涉及 | 归属 |
|---|---|---|
| N1 | `TC-WS-08` 的 `peakSusp ≥ 0.5` | **另案**（designer 另起 protocol revision + 用户裁决，已建卡 `6adc64ff…ccb43`）；本 revision 不以 `peak(U+P)/E` 替代该冻结估计对象 |
| N2 | `TC-WS-09` 的采样段（静置 10s、末段 5s） | 不变；新末 1s 窗嵌在既定末段内 |
| N3 | `SLIP-02/03/04` 的 `30°±2°` / target-based `t90` / `R²≥0.90` 与全部安全闸门 | 不动；新残余 PASS **不得**追认 SLIP-02 或 P-011 CLOSED |
| N4 | `DUALFIELD-*` 16 例的 CONS/BUD/LIFE/SRC/GES estimand 与 GES 五观测点 | **零改动**；不另造通用 5% PASS；连续手势不套用本门槛 |
| N5 | `DUALFIELD-CONS-04`（沉降 t90）口径 | 不动；其 estimand（真实沉积分相质量/累计期望、跨 epoch 无效）继续有效；实现缺口由 developer 按 CONS-04 修，本 revision 不改其定义 |
| N6 | 粒子终端速度律、`gestureSandRate`、`Nmax`、GES 观测点阈值 | 一律非冻结，试采后另稿 |
| N7 | P-012 `E2`/`E4` | 不从本指标推出 PASS；`E4` 沙丘 mask 只取真实沉积床层，飞沙云与接触待准入粒子不并入 |
| N8 | 历史报告结论 | 不改、不追认、不推翻 |
| N9 | `ESTIMAND` 变更 | 本 revision **不含**任何 estimand 变更；若需变更，另起 revision |

## 9. 生效规则与执行序列

### 9.1 生效规则

- 本 revision **立即生效为判定协议**（口径已冻结），但**尚未实现**：实现落地并经 designer 验证、manager 确认适用范围之前，协议**不产生放行效力**。
- 生效前：C run 继续**双口径并报**（旧 `susp-only` 与新 `U+P` 并列），**任一单口径都不得单独放行**。
- 旧报告保留原 PASS/FAIL 与 `legacy-susp-only` 身份，不追认旧验收。
- 适用范围确认后：路线 C run 的终点效果判定以新口径为准（`M/R` + 末 1s 窗），legacy 口径继续并列披露；OFF/B 路线无真实粒子层，legacy 口径仍为其适用判据，候选数学对照取结构 `P=0` 并明确标注。
- 回算一律标 `RETROSPECTIVE_DIAGNOSTIC`。

### 9.2 执行序列（承接裁决 §3）

1. **tester**（本卡）：发布本 revision + `docs/test-cases.md` §14 版本化用例条目 + checker 适配说明。
2. **architect**：登记字段 revision（采样身份、`endpointSampleElapsedMs`、`byState` 诊断扩展、三分项定名、报告字段清单），与本 revision 出字段对照表，差异给结论。
3. **developer**：实现三分项输出、endpoint 采样、checker 同 run 双算（并校验 `D+U+P≈E`）。
4. 同 build 同输入跑路线自身基线与候选，出路线专属双报告（OFF/B 取结构 `P=0`，不得向旧文件补造 v7 字段）。
5. **designer** 验证 → **manager** 确认适用范围。

## 10. 自检反例（重算表，用于核对实现是否照协议执行）

以下全部为构造算例（非测量），用于核对判定式与身份标注：

| # | 输入 | 旧口径 | 新口径 | 结论 |
|---|---|---|---|---|
| 1 | `E=8000, D=7550, U=322, P=128`，末窗保持 | `U/E=4.025%` PASS | `M/E=5.625%` FAIL | 旧口径假 PASS，新口径正确 FAIL；只加粒子不会把旧 FAIL 变 PASS（`F_new−F_old=P/R`） |
| 2 | `R_SLIP=7000, E=8000, M=375` | 显示 `M/E=4.6875%` | 正式判定 `M/R_SLIP=5.3571%` FAIL | 错用显示分项假 PASS；两分母必须并报、各自判定 |
| 3 | 末窗早期 `M/R=6%`，末步 `4%` | 旧单终点 PASS | 质量窗 FAIL | 新「末 1s 持续达标」正确 FAIL；旧单步定义不得冒充新口径 |
| 4 | `LAND` 提交 20；提交前 `P=80`，提交后 `U=30, P=60` | 混提交前 `P` → `M=110` | 同提交身份 → `M=90` | `R=2000` 时混时 5.5% 假 FAIL，正确 4.5%；只统计同提交身份 |
| 5 | `DEGRADE`：`P` 减 q、`U` 加 q | — | `M` 不变 | 降级不改善本指标，不得靠降级刷分 |
| 6 | `MERGE`：合并包质量 = 各包之和 | — | `M` 不变 | 合并不改善本指标 |
| 7 | `D=E−0.04E`，`U=0`，`P=0.04E` 且同粒永久待准入 | 旧 0% PASS | 新 4% PASS | 两者数值均 PASS，但都**不是**“完全落床”；须另审生命周期/无永久悬浮，不得凭 5% 结项 |
| 8 | C run 的 `U=100, D=900, P=50` | 历史 `U/(D+U)=10%` | legacy 重算（`A_hist=D+U`）10%；live `U/(D+U+P)=9.52%` | 两个 legacy 量在 C run 中必然不同（§5.3），须分列标注，不得互为别名 |
| 9 | 同总量但把 76.8 粒子降到列：`U=398.8, P=51.2`（`M` 不变） | `U/E` 下降 | `M/E` 不变 | 新口径不奖励降级/转列表示方式 |
| 10 | 窗内 `sourceEpoch` 变化（发生新外源准入） | — | `INCOMPLETE`（`ENDPOINT_NOT_QUIET`） | 不外推、不插值、不重选窗口求绿 |

## 11. 版本化用例条目索引

本 revision 的可执行用例条目登记在 `docs/test-cases.md` **§14**（`TC-P011-ENDPOINT-01..08`），字段名与版本号与本 revision 一致（`protocolRevision=endpoint-terminal-nondeposited-v1`、`endpointMetricRevision=terminal-nondeposited-v1.1`）。

## 12. 资产索引与交接

| 资产 | 路径 | 用途 |
|---|---|---|
| 本 revision | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` | 判定协议权威（口径/窗口/身份/状态码/双算） |
| 裁决记录 | `docs/decision-P011-terminal-nondeposited-fraction-v1.md` | 用户批准的 5 项裁定与配套规定 |
| 设计定稿 | `docs/design-P011-terminal-nondeposited-fraction-v1.md` §7 | 口径推导、反例、迁移步骤 |
| 架构字段权威 | `docs/architecture-P011-dual-field-particle-v1.md` §5.6/§1.2 | v7 `particles` 块、七类事件、`pState` 语义 |
| 预注册（不改） | `docs/preregistration-P011-dual-field-particle-v1.md` | DUALFIELD-* 16 例 estimand |
| 用例条目 | `docs/test-cases.md` §14 | 本 revision 的版本化用例条目 |
| 现行 checker | `test/p011_slip_formal_check.py` | 适配说明对象（本卡不改代码） |

交接：architect（字段 revision 对齐，卡 `6adc64ffe4b017ab33dccb44`）→ developer（实现，卡 `6adc6513e4b017ab33dccdb0`）→ tester（按本 revision 出双报告）→ designer（验证）→ manager（确认适用范围）。

---

## 13. 追加记录

- 2026-10-24：v1 首次发布。依据用户 2026-10-24 裁决（5 项逐条裁定）与 designer §7.1–§7.5 定稿，冻结分子 `U+P`（含 `state=2`）、分母（各自历史分母不变）、阈值 5%（`epsMass=1e-9`）、末 1s 每子步质量窗、固定终点（SLIP 每相位最后已提交子步 / WS-08 满 8.0s）、post-commit 同子步采样与连接键、单调实际 elapsed 时钟、五类状态码分流、同 run 双算与 `legacy-susp-only` 身份、九项不涉及项。本 revision 不含 estimand 变更。
- 2026-10-24（补充冻结，§5.3）：现制 `WS_METRIC.suspensionFraction` 的历史分母为 `A_hist = D+U`（`getTotals()` 不含粒子桶）；C 实现后同名 live 值分母将变为 `D+U+P`，故双算须分列 `legacySuspOnly.recomputed` 与 `wsMetricSuspensionFractionLive`，两者在 C run 中必然不同。此点裁决与设计稿未显式写明，按裁决第 1/4 条（保留旧公式旧名、不得混名）与源码事实推出并冻结；如 architect 字段 revision 给出相反结论，须走对齐结论并同步修订本节。
