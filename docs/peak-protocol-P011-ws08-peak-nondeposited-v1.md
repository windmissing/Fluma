# P-011 WS-08 摇晃未沉积沙峰值独立 peak protocol revision v1

日期：2026-10-25。发布：tester（P-011）。状态：**METHOD_FROZEN / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_CALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION**（`peakProtocolEffective=false`，生效宣布权在 manager）。

## 0. 身份与适用范围

| 项 | 取值 |
|---|---|
| `protocolId` | `p011-ws08-peak-nondeposited` |
| `protocolRevision` | `ws08-peak-nondeposited-v1` |
| `peakMethodId` | `peakNonDepositedFraction` |
| `peakMetricRevision` | `shake-peak-nondeposited-v1`（报告字段取值，见 §10） |
| `peakAuthority` | `APPROVED`（依据：用户 2026-10-24 五项裁决，manager 23:55 下达；**仅覆盖方法口径**，不含生效宣布、不含实现、不含验证） |
| `peakProtocolEffective` | `false`（宣布权在 manager；本协议不自行宣布） |
| `peakFieldAlignment` | `PENDING_ARCHITECT_REGISTRATION`（peak* 字段名由 architect 登记独立 peak 字段 revision 后对齐；本稿 §10 字段为拟议落点） |
| 依据裁决 | 用户五项裁决（manager 2026-10-24 23:55 下达）：①采纳候选 B ②分子含全部活粒子（含 `state=2` 接触待准入）③分母用 `E`（累计期望）④固定 shake-on 后 3s 窗、每成功提交子步采样 ⑤原 0.5 及格线与原分档锚点随新量迁移 |
| 依据设计 | `docs/design-P011-ws08-peak-nondeposited-migration-v1.md`（designer 2026-10-24 定稿，manager 19:15 验收） |
| 依据终点协议 | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（已发布，**本协议不改其一字**；两份协议各自独立、互相引用） |
| 依据终点裁决 | `docs/decision-P011-terminal-nondeposited-fraction-v1.md`（残余 `M=U+P` 含 state2、WS 用 `E`、5% 与 `epsMass=1e-9`、固定末 1s 每提交达标） |
| 依据字段稿 | `docs/architecture-P011-endpoint-fields-revision-v1.md` rev2（endpoint 字段；新峰值用独立 peak* 方法身份） |
| 现行 checker 适配对象 | `test/water_sand_check.py::evaluate_shake()`（`peakSusp` 取全序列 max，未落实书面摇晃段）——适配清单见 `docs/peak-adaptation-checklist-P011-ws08-peak-nondeposited-v1.md` |
| 历史结果身份 | `test/artifacts/P003-P005-baseline-20261013/ws_check.json`（TC-WS-08 `shake` 记 `BLOCKED / NOT_IMPLEMENTED`，**从未测得峰值**；RED 基线见 `docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md`） |

**本 revision 冻结什么**：`peakNonDepositedFraction` 的分子、分母、固定摇晃窗与采样身份、分档锚点与 0.5 及格比较、判定式与连接键、状态分类、双算规则、可达性必要条件、与已批准终点 revision 的边界、生效规则（§1–§9），并给出报告字段契约（§10）。

**本 revision 不做什么**：不实现、不改代码、不改 endpoint 协议正文与已发布版本、不改任何已批准 estimand、不改冻结指标（30°±2°/t90/R²/5%）、不改历史报告结论、不授权采集、不追认旧 route 或旧 build 结果、不宣布 `peakProtocolEffective`。

**适用对象**：路线 C（真实活动粒子 + 128 列高度场双场）`TC-WS-08` 的摇晃段峰值子项。原场景时序/输入不变：default、TEST、tilt=0、已形成沙堆，静置 3s → shake=1 持续 3s → 停止静置 8s。OFF/B 路线无真实粒子层时只能作标注清楚的数学对照（结构 `P=0`），不得向旧文件补造 v7 字段。

**与 endpoint 协议的关系**：本协议测量摇晃段（shake-on 后 3s）的未沉积库存峰值；endpoint 协议测量停止后静置 8s 的终点残余与末 1s 质量窗。两个时段、两个阈值、两套身份，分别判定。**峰值不借用末 1s 最大值，终点也不借用摇晃峰值**；任一方 PASS 不替另一方签发，也不合成"WS-08 全链通过"。

## 1. 冻结口径（分子 / 分母 / 阈值 / 分档）

### 1.1 分子 M = U + P

对成功提交子步 `k`，全部为 model material-volume units（与 `bh/susp/pMass` 同单位；位置、速度、粒子个数、像素面积都不是质量代理）：

```text
D_H(k) = sum(stateAfter.bh) + sum(stateAfter.mh)
D_L(k) = sum(stateAfter.bl) + sum(stateAfter.ml)
U_H(k) = sum(stateAfter.suspH);  U_L(k) = sum(stateAfter.suspL)
P_H(k) = particles.massH;        P_L(k) = particles.massL
D = D_H+D_L;  U = U_H+U_L;  P = P_H+P_L;  M = U+P
```

（与 endpoint 协议 §1.1 同构；`pState` 语义与 M1–M8 细则逐字继承该协议，避免两协议漂移。）

分子细则（逐条可执行，缺一即判 `INVALID_LEDGER`）：

| # | 细则 | 说明 |
|---|---|---|
| M1 | `P_H/P_L` 必须等于 `pState∈{1,2}` 的 `pMass` 按 `pPhase` 求和 | `pState` 语义：`0=空槽 1=飞行 2=静置待准入`；`state=0` 空槽**不计** |
| M2 | 水下、水面上方、接触待准入的粒子**全部计入** `P` | 裁决第②条：分子含全部活粒子，含 `state=2`；`airborne` 不作"位于空气中"的字面解释 |
| M3 | `DEGRADE`/`TO_SUSP` 的**累计事件量永不进分子** | `particles.degraded.{count,massH,massL}` 是累计量，不是当前存量 |
| M4 | `MERGE` 后只计现存包质量**一次** | 不得把合并前各包质量与合并包相加 |
| M5 | `LAND` 只扣 `granted` | 已提交进 `mh/ml` 的 granted 部分计 `D`；余量留原粒 `state=2` 继续计 `P`（`granted+remainder==requested`） |
| M6 | `TO_SUSP` 若被显式启用，提交后该量只计 `U` | 口径先冻结，与 endpoint 协议 M6 一致 |
| M7 | B tracer 无质量，一律不计 | 纯视觉层 |
| M8 | 摘要与逐粒缓存不得同时相加 | 摘要与逐粒求和不一致即 `INVALID_LEDGER` |

**守恒先决**：每个已提交子步须先过冻结守恒 `abs(D_H+U_H+P_H-E_H)≤1e-9`（L 同理），`E_H/E_L` 定义见 §1.2。任一未过，不给正式峰值 PASS。

### 1.2 分母 E（累计期望）

```text
E_H = VH0 + context.sourceInjectedH;  E_L = VL0 + context.sourceInjectedL;  E = E_H + E_L
```

- `VH0/VL0` 取 reset 后、任何外源准入前的**首条** `audit.heavyTotalBefore` / `audit.lightTotalBefore`，身份与 endpoint 协议 §1.2 相同。
- WS-08 为原无外源场景：整 run 不得外源准入，故 `E=VH0+VL0` 恒定、`sourceEpoch` 不变。公式保留累计期望定义以便账本对齐，**不因此授权新的有源 WS-08 变体**；窗内 `sourceEpoch` 变化 → 资格不符。
- `E ≤ 0` → `INVALID_DENOMINATOR`；不以 EPS 替代使空沙场景通过。
- 初值/注入前身份不可证 → `INVALID_BASELINE`。

### 1.3 分档锚点与 0.5 及格线（裁决第⑤条：原锚点随新量迁移）

| `peakNonDepositedFraction` | 分数 |
|---|---|
| ≥0.90 | 10 |
| ≥0.75 且 <0.90 | 8 |
| ≥0.50 且 <0.75 | 6（及格） |
| ≥0.35 且 <0.50 | 4 |
| ≥0.20 且 <0.35 | 2 |
| <0.20 | 0 |

- 用**未四舍五入**的 `M/E` 评分；`peakScore` = 上表中适用的最高锚点分。
- **0.5 及格比较为 `M ≥ 0.5×E`**（等价 `peakNonDepositedFraction ≥ 0.5`），**不新增向下放宽的判定容差**；库存/分相守恒闸门仍执行各自原有容差（`1e-9`）。
- 及格 ⟺ `peakScore ≥ 6`。并非全部峰值数值都是 MUST-PASS 放行项：安全硬闸门失败时只可留原始候选峰值诊断，不签正式峰值 PASS。
- 字段/资格/覆盖无效**不打 0 分凑平均**，输出对应无效状态（§4）。
- 0.5 代表"一半总沙为非沉积库存"的数值及格锚点；没有观测依据支持调低。保留数值不等于新旧测量等价，不等于已标定效果；不能用这个峰值单独签发整个 WS-08 PASS（粒子生命周期、可见扬起/自然、无永久悬浮各按既有证据链判断）。

### 1.4 分相诊断（不设分相通过线）

- `peakNonDepositedHeavyFraction = max_{k∈W}[(U_H(k)+P_H(k))/E_H]`，轻相同理。
- 某相 `E_phase = 0` 记 `N/A`，**不能 EPS 造比例**。
- 总量峰值提交**同时记录** H/L 分项；独立分相峰值**不能相加**（`max_W(U_H+P_H)/E_H + max_W(U_L+P_L)/E_L` 通常 ≠ 总量峰值）。
- 不新增 H/L 通过阈值；总量达标而 heavy 几乎未扬起时，仍按现用例要求登记"摇晃交融构成未达成"。
- 保留旧 `peakSuspHeavy/Light` 原初值分母/旧列悬沙语义，**不互为别名**。

## 2. 摇晃窗口与采样身份

### 2.1 固定摇晃窗 W

- `t_on` = 物理输入链**实际接受** shake-on 的单调实际 elapsed（`context.endpointSampleElapsedMs` 同源时钟域）。
- **`W = [t_on, t_on+3000ms)`**（左闭右开），成员为 W 内**实际消费该 shake-on 输入**的全部成功提交子步。
- 窗口右边界的 shake-off 后提交**不纳入**峰值；不把停止后 shakeEnergy 衰减产生的更晚峰值移进来。
- 原调度 off 稍迟时，超过固定 3000ms 的样本仍不计入本峰值；提前 off、或窗内输入不是原 1.0 → 场景资格不符（§4 `INCOMPLETE`）。
- 以输入事件/提交全序判定同一毫秒边界归属；必须记录 on/off 实际生效边界及本子步消费的输入身份，**只看 context 内预更新 shakeEnergy 不能替代输入接受证据**。
- 输入时序漂移按既有 schedule/后续 tester 预注册的适用门禁单列审查；**不新增允许延长摇晃的容差，不更改 endpoint 的 shake-off+8s 边界**。场景合规性未经确认时不得据候选峰值签整体 PASS。

### 2.2 采样密度与身份契约

- **每个成功提交子步**采样一次（post-commit 同子步读 `stateAfter` + `particles`）；`WS_METRIC` 的 250ms 抽样只可作旁证与诊断（§5.3 `OBSERVED_SAMPLED_MAX`），**不得**替代每子步峰值。
- 连接键 `(runId, sequence, frameId, substepId)`；同时绑定 case occurrence 与 `context.endpointPhaseOccurrenceId`（引用现成身份，不更名）。
- 时钟复用 `context.endpointSampleElapsedMs`（成功提交后单调实际 elapsed）；记录 `context.integratedPhysicsSeconds`，**不得用它代替 3s 实际时间**；context/metric `physicsTime` 可处于不同取时阶段，不要求时间戳逐值相等。
- 必须有整个摇晃窗每成功提交的原始 v7 状态/粒子摘要、on/off 及采集边界证据；断档/提前退出记 `INCOMPLETE`。合法休眠段可带身份只读复核保持值；**不能把冻结/回滚/漏记当休眠，不能伪造成功子步**。
- 窗内资格：无新外源准入（`sourceEpoch` 恒定）、无 reset/invert、无 route/模式切换（`resetCount/invertCount/routeChangeCount/modeChangeCount` 恒定）；不满足 → `INCOMPLETE`。

### 2.3 禁止项（违反即 `INVALID_LEDGER` 或 `INCOMPLETE`）

- 不新增峰值末 1s 持续窗、不取均值/平滑值、不等待达标延长输入；
- 不用 250ms 抽样估"精确子步峰值"、不插值补峰；
- 峰值所需字段缺失记 `NOT_IMPLEMENTED`，不用替代量冒充；
- 异常尖峰先查守恒/有限值/身份/事务，**不能仅因短暂而删除它**（单个真实且守恒有效的提交可为峰值）。

## 3. 判定式与峰值选取

```text
f(k) = M(k) / E(k)                      # 同一提交先求和再取 max
peakNonDepositedFraction = max_{k in W} f(k)
peakCommitKey = W 内达到该 max 的最早提交   # 并列留最早
peakStockMass = M(peakCommitKey);  peakReferenceMass = E(peakCommitKey)
peakSampleElapsedMs = peakCommitKey 的 endpointSampleElapsedMs
peakScore = 最高适用锚点分（§1.3）
及格判定：M(peakCommitKey) >= 0.5 * E(peakCommitKey)
```

- 分项必须在**总量峰值提交同时**披露（§1.4）；`max(U/E)+max(P/E)` 通常 ≠ `max((U+P)/E)`。
- 采集停止后 8s 的终点判定**完全委托**已批准 endpoint 协议，本协议不重算、不引用其数值。

## 4. 状态分类与判定顺序

### 4.1 判定顺序（顺序不可调换）

```text
1. 路由与协议身份校验（particleRouteActive/particleRouteId；routeId 归属；无外源准入；基线 E>0）
2. 硬闸门：守恒 / 库存 / 容量 / guard / 预算 / 有限值 / 账本对齐 / 边身份（既有 safety checker 原样执行）
3. 窗口绑定与资格：on/off 有效边界、输入身份与连续性、覆盖完整性
4. 字段完整性：峰值所需字段缺失 -> NOT_IMPLEMENTED(missing_fields)
5. 账本/基线/分母：坏账本 -> INVALID_LEDGER；初值身份不可证 -> INVALID_BASELINE；E<=0 -> INVALID_DENOMINATOR
6. 窗口资格不足 -> INCOMPLETE
7. 效果峰值：逐提交 f(k) -> max / 最早键 / 分档 / 及格比较
```

**硬闸门失败不得用峰值 PASS 掩盖**：任一硬闸门失败 → 该 run 整体不合格，只可留原始候选峰值诊断，先报失败闸门。

### 4.2 状态码（均不得记 PASS）

| 状态码 | 触发条件 |
|---|---|
| `NOT_IMPLEMENTED(missing_field)` | 峰值所需字段缺失（须列确切字段路径，不用替代量冒充） |
| `INVALID_LEDGER` | 账本无效：摘要与逐粒不一致、跨子步拼接、非有限值、非单调 elapsed、重复提交键 |
| `INVALID_BASELINE` | 初值身份不可证（无法证明 `VH0/VL0` 取自准入前） |
| `INVALID_DENOMINATOR` | `E ≤ 0`；分相 `E_phase=0` 仅该相记 `N/A`，不阻断总量 |
| `INCOMPLETE` | 窗覆盖不足、断档/提前退出、输入时序资格不符（提前 off、窗内输入非原 1.0）、窗内 `sourceEpoch`/reset/invert/route/模式变化 |
| `NOT_OBSERVABLE` | 诊断码：逐粒/状态时间证据不存在（`byState` 缺失） |

## 5. 双算规则与报告身份

### 5.1 同 run 双算（legacy 与新量并列，禁止任一单独放行）

| 项 | legacy 口径 | 新口径 |
|---|---|---|
| 分子 | `U`（列悬沙） | `M = U + P` |
| 分母 | 历史分母语义 `A_hist = D+U`；live 代码路径 `A = D+U+P` | `E = VH0+VL0+IH+IL` |
| 窗口 | 旧 `peakSusp` 原窗口（现行实现为**全序列**） | 固定 `W = [t_on, t_on+3000ms)` |
| 采样 | 250ms `WS_METRIC` 抽样 | 每成功提交子步 |
| 评分 | 现行二元 `peak>=0.5 and residual<=0.05`（无 TIERED） | §1.3 分档 + 0.5 及格比较 |
| 身份标记 | `legacy-susp-only` | `peakNonDepositedFraction` / `shake-peak-nondeposited-v1` |
| 原结论 | **保留原 PASS/FAIL/BLOCKED**，不追认也不推翻 | 新判定 |

### 5.2 三个 legacy 峰值量（同 W、逐子步复算，各自标注分母/密度/证据类型）

| 字段 | 定义 | 用途 |
|---|---|---|
| `legacyPeakSuspReported` | 旧原字段、原样本/原窗口的实测峰值（现行实现=全序列 max of `U/max(D+U,EPS)`） | 与历史 run 对照；**不是**新协议的合格证据 |
| `legacyPeakHistoricalReference` | 同 W 逐子步 `max h(k)`，`h(k)=U/(D+U)` | 旧列模型质量口径复算；C 全部入粒子时 `D+U=0`，记 `legacyHistoricalReferenceStatus=INVALID_DENOMINATOR`，**不能阻断数据完整且 E>0 的新量计算，也不能冒称 legacy 有可比值** |
| `legacyPeakLiveFormula` | 同 W 逐子步 `max l(k)`，`l(k)=U/max(A,EPS)`、`A=D+U+P` | C run 内现况披露；旧代码对极小分母的 EPS 钳位如实保留原样并报告 clamp，不以其冒充 `h` 的有效分母 |
| `sameReferenceSuspOnlyPeak` | `max_{k∈W} U(k)/E(k)`，独立命名 | 同分母（E）下的纯悬沙峰值，供与 `peakNonDepositedFraction` 直接对照 |

### 5.3 恒等式与差异归因（自检用）

- 仅在守恒 `A=E` 且 `D+U>0`、同提交：`f−l = P/E`、`f−h = P·D/[E·(D+U)] ≥ 0`。与历史分母的差值**不是无条件 `P/E`**；`D=0` 或 `P=0` 时可能相等。容差内守恒时另报实际差值/守恒残差，不断言机器数严格相等。
- 同一 W、同样本且有效分母时 `max f ≥ max h`，但峰值可发生在不同提交。
- **旧峰与新全子步峰之差必须分解归因**：窗口效应（全序列→固定 W）、采样密度效应（250ms→每提交）、分子/分母效应（`U/(D+U)`→`(U+P)/E`）**分开列**；不得把三者之差全归因于物理改善（归因链框架与执行证据见 RED 基线记录）。
- **250ms 旧数据另记 `OBSERVED_SAMPLED_MAX`**：仅作为同轨迹真 max 的**下界**，明确标注它不是新协议的合格证据；即便已采样值 ≥0.5，也不据此宣称新协议采集完整。

### 5.4 初态/响应/滞留披露（不设新通过线）

- `baselineNonDepositedFraction = M0/E`（shake-on 前最近可证 post-commit 的 `M0` 与提交键）。若 `M0` 本身 ≥50%，B 可数值达标，但并未证明新增扬起，必须报告 `M0`/净增量/LIFT 证据。
- `peakNetIncreaseFraction = max_{k∈W}[(M(k)−M0)/E]`；本轮 `cumulativeLiftGrantedMass` 及 guard/budget 拒绝记录。
- 峰值提交的 FLYING/CONTACT_WAIT 分项（`byState` 缺失记 `NOT_OBSERVABLE`）；不虚构"长期不落已排除"。
- `byState` 不存在时，只要 canonical P 及原安全账本完整，库存主量可算，但状态归因记 `NOT_OBSERVABLE`。
- 本案**不新增基线 ≤5%、最小 LIFT 量或粒子最大寿命**等通过线；产品若需这些因果/动力学门槛，另行明确测试目标，不把终点 5% 自动套到 shake-on 前。

## 6. 自检反例与可达性约束（重算表）

以下全部为构造算例（非测量、非真机数据），用于核对判定式与身份标注；执行证据见 `test/reports/peak_protocol_v1_execution.json`（`counterexamples` 段，逐条 MATCH 方可采信）。

| # | 场景 | 数值与结论 | 防止的误读 |
|---|---|---|---|
| 1 | 表示漏计 | `E=1000, D=480, U=320, P=200`；`h=40%`、`l=32%`、`f=52%` | 按"总未沉积"目标旧量会假 FAIL；按旧"列悬沙"语义它仍是正确的旧结果，不能倒改报告 |
| 2 | 降级奖励旧量 | 上例 200 全 DEGRADE：`D=480, U=520, P=0`；旧变 52%，新仍 52% | 旧结论改变不等于扬起改善；新量对转列/合并不敏感 |
| 3 | 跨时刻加峰值 | `E=1000`，k1：`U=400/P=0/D=600`；k2：`U=0/P=400/D=600` | 分别取峰再相加得 80% 假 PASS，正确总量峰 40% 不及格（先同提交求和再取 max） |
| 4 | 输入平台响应滞后 | 首个 shake=1 提交 `f=10%`，平台 2.4s 时 `f=55%` | 最早输入峰时刻法为 10%，B 为 55%；二者不同估计对象，不能互相追认 |
| 5 | 全序列污染 | 初始静置段 `f=80%`，摇晃段 max=40% | 全序列 max 可假 PASS；正式峰只取固定 W。若 `M0` 本身仍 ≥50%，B 可数值达标，但并未证明新增扬起，必须报告 `M0`/净增量/LIFT 证据 |
| 6 | 接触库存伪装运动 | `E=1000, D=480, U=20, P_state2=500` | B 库存 52% 正确，但若解释成"52% 真实飞行/自然悬浮"是假 PASS；分报 state2、生命周期、主观效果，不能只凭峰值放行 |
| 7 | 250ms 漏峰 | 真子步峰 52%，所有 250ms 样本最多 48% | 稀疏旧采样可在新目标下假 FAIL；新量必须保留全部成功提交，不插值补峰 |
| 8 | 停止后才高 | shake 段 max48%，off 后因残余能量到 55% | 摇晃段峰仍不及格，不能扩大窗口到 rest 求 PASS；endpoint 照已批准协议独立算 |

**可达性必要条件（否定"改成 U+P 就自然能过 0.5"）**：对无外源、无 invert/reset 的本场景，若全部床↔非沉积变化只有已记账 LIFT 与 SETTLE/LAND，则精确库存结算：

```text
M(k) = M0 + cumulativeAcceptedLift(k) - cumulativeSettledOrLanded(k)
peak M <= M0 + cumulativeAcceptedLift(W)
cumulativeAcceptedLift(W) <= 300 * tau_W
必要条件：M0 + 300*tau_W >= 0.5*E
tau_W = W 内成功子步的 sum(physicsDt)，单位秒，不是 wall elapsed
```

假设 `M0=0`、`tau_W=3s`，最乐观峰 ≤900 units，要到 50% 必须 `E≤1800`；若假设 `E=8000`，则仅此条件就要求 lift 积分时间 ≥13.33s，原 3s 假设下不能达标。真实 settle/LAND 及其他通道分走预算只会收紧这个条件。存在其他合法床↔非沉积通道时，必须在恒等式中逐笔补入，不能漏记后套这个界；MERGE/DEGRADE/CROSS 只改表示或位置，不直接改变 M。上述不是对实际 default build 的无解证明（本卡没有实际 E、M0、tau 轨迹）。如后续实际账本证明目标不可达，应按原流程向 manager 报告目标/场景/模型冲突，由用户另裁；**不暗增 300/s、不延长摇晃、不调低 0.5、不减总沙量求绿**。

## 7. 已批准终点 revision 的严格边界

以下全部引用既有批准记录/协议，不由本稿重新裁定：

| 已批准内容 | 本案关系 |
|---|---|
| 终点批准记录与 endpoint 协议：残余 `M=U+P` 含 state2、WS 用 `E`/SLIP 用 `R_SLIP`、5% 与 `epsMass=1e-9`、固定末 1s 每提交达标 | **不改一个字**；峰值没有 5% 要求，也不借用其末 1s 最大值作为摇晃峰值 |
| WS-08 实际 shake-off 后 8s 终点，SLIP 原耦合与阶段适用性 | 不改；本峰值窗是 shake-on 后 3s，两个时段及阈值分别判断 |
| endpointMetricRevision/Protocol/Authority 及字段对齐裁定 | 引用现成 post-commit/clock/key 定义；新峰值使用独立 peak* 方法身份，不升级或覆盖 endpoint 身份 |
| endpoint state=2 滞留诊断、无永久悬浮、各硬闸门 | 继续执行；本案只在峰值另报 state 分项，不创设寿命标准 |
| DUALFIELD CONS/BUD/LIFE/SRC/GES、30°±2°/target-based t90/R²、E2/E4 | estimand/目标不变；本峰值不代替手势/落床/坡度/渲染验收，不解除真机队列前置 |

用户已批准**终点**不等于批准本案峰值（本案依据的是用户对本峰值的五项裁决）。终点 METHOD_FROZEN 也不意味着 peak* 生效；C run 的旧/新终点双报和单口径不放行纪律仍按原协议，不能用本案覆盖其权限。

## 8. 生效规则与执行序列

### 8.1 生效规则

- 本 revision 冻结方法口径（依据用户五项裁决），但**尚未实现、尚未验证**：实现落地并经 designer 验证、manager 确认适用范围之前，协议**不产生放行效力**，`peakProtocolEffective=false`。
- 生效前：WS-08 摇晃峰值子项继续**双口径并报**（旧 `peakSusp` 原身份 + 新 `peakNonDepositedFraction`），**任一单口径都不得单独放行**；旧 `peakSusp` 保留原身份与原结论，标 `legacy-susp-only`。
- 旧 C 账本若有真实 P/同提交/完整 W 及必要时钟字段，可离线补算为 `RETROSPECTIVE_DIAGNOSTIC`，不追认旧验收；旧 OFF/B 明确无 P 时数学对照 `P=0`，但不向旧文件补造 v7 字段。
- 获批但未实现/未验证也不能直接放行；批准前不替代旧量。

### 8.2 执行序列

1. **tester**（本卡）：发布本 revision + `docs/test-cases.md` §15 版本化用例条目 + checker 适配清单 + RED 基线记录（`NOT_IMPLEMENTED` 起步，不冒充）。
2. **architect**：登记独立 peak 字段 revision（复用既有 commit/elapsed/particles 契约，补齐 shake-on/off 有效边界与子步实际消费输入身份），与本 revision 出字段对照表。
3. **developer**：按获批契约实现摇晃段每提交采样、流式峰值与 checker 适配（§适配清单）；现行 `WS_METRIC` 原名原式继续输出。
4. **tester**：先用最新协议输出诚实 RED 基线（字段未实现即 `NOT_IMPLEMENTED`），之后同 C 路线、同 build/输入/参数跑同一原始账本的 legacy 与新量；另报共同 250ms 样本对照与全部子步新峰值，将分子/分母差异与采样密度差异分开。
5. 真机需求由 manager 交 effector 串行排队（原 3s 静置/3s 摇晃/8s 静置；配对录像只验证扬起/自然与永久残余，不以画面代替守恒账本）；本卡不占设备。
6. 路线 C 专属客观/主观报告齐备后 designer 验证（同路线前版为基线），manager 确认适用范围并宣布 `peakProtocolEffective`。
7. 旧结果带 `legacy-susp-only` 及原 build/protocol/采样身份，原分数与结论保留。

## 9. 本 revision 不涉及项（逐项列明）

| # | 不涉及 | 归属 |
|---|---|---|
| N1 | `TC-WS-08` 停止后 8s 终点残余与末 1s 质量窗 | endpoint 协议（已批准），本协议只委托、不重算 |
| N2 | 旧 `peakSusp` 字段本身 | 保留原身份原式；迁移期双报，另案获批并完成独立协议/实现/验证后才替代 C 的峰值子项 |
| N3 | `SLIP-02/03/04` 的 `30°±2°` / target-based `t90` / `R²≥0.90` 与全部安全闸门 | 不动；峰值 PASS 不得追认 SLIP-02 或 P-011 CLOSED |
| N4 | `DUALFIELD-*` 16 例的 CONS/BUD/LIFE/SRC/GES estimand 与 GES 五观测点 | 零改动；不另造通用通过线 |
| N5 | 粒子终端速度律、`gestureSandRate`、`Nmax`、300 unit/s 预算值 | 非冻结；本案只把 300/s 用作可达性上界输入，不改其值 |
| N6 | P-012 `E2`/`E4` | 不从本指标推出 PASS |
| N7 | 历史报告结论（含 2026-10-13 RED 基线的 `BLOCKED`） | 不改、不追认、不推翻 |
| N8 | `ESTIMAND` 变更 | 本 revision **不含**任何 estimand 变更；若需变更，另起 revision |
| N9 | 自然度/主观效果/无永久悬浮的最终结论 | 各按既有证据链；峰值数值不单独签发 |

## 10. 报告字段契约（拟议落点，待 architect 登记）

| 组 | 字段 |
|---|---|
| 独立方法身份 | `peakProtocolId=p011-ws08-peak-nondeposited`、`peakProtocolRevision=ws08-peak-nondeposited-v1`、`peakMetricRevision=shake-peak-nondeposited-v1`、`peakAuthority=APPROVED`、`peakProtocolEffective=false`；route/build/params/input/hash/clock 身份 |
| 窗口/证据 | `peakWindowStartElapsedMs`、`peakWindowEndElapsedMs`、`shakeOnEffectiveElapsedMs`、`shakeOffEffectiveElapsedMs`、`peakWindowSampleCount`、`peakWindowCoverageStatus`、`missingFields`；输入消费/阶段 occurrence、提交序列及 `integratedPhysicsSeconds` |
| 主结果 | `peakNonDepositedFraction`、`peakStockMass`、`peakReferenceMass`、`peakCommitKey`、`peakSampleElapsedMs`、`peakScore`、`peakStatus`；峰值提交的 `D/U/P/E` 与 `suspFraction`/`airborneFraction`/`depositedFraction` |
| 分相 | `peakNonDepositedHeavyFraction`、`peakNonDepositedLightFraction`（`E_phase=0` 记 `N/A`）；总量峰值提交同时记录 H/L 分项 |
| 双算 | `legacyPeakSuspReported`、`legacyPeakHistoricalReference`（+ `legacyHistoricalReferenceStatus`）、`legacyPeakLiveFormula`、`sameReferenceSuspOnlyPeak`、`OBSERVED_SAMPLED_MAX`，各自分母/密度/证据类型 |
| 初态/响应/滞留 | `baselineNonDepositedFraction`（+提交键）、`peakNetIncreaseFraction`、`cumulativeLiftGrantedMass` 与 guard/budget 拒绝、峰值 FLYING/CONTACT_WAIT 分项（`byState` 缺失记 `NOT_OBSERVABLE`） |
| 结果分离 | `hardGateStatus`、已批准 endpoint 协议的独立结果引用、自然度/无永久悬浮证据状态；**不得把三者合成"峰值已证明全链"** |

三显示名复用已批准定义 `suspFraction=U/E`、`airborneFraction=P/E`、`depositedFraction=D/E`，不重命名已有字段，也不把 endpoint 字段换成 peak。

## 11. 版本化用例条目索引

本 revision 的可执行用例条目登记在 `docs/test-cases.md` **§15**（`TC-P011-PEAK-01..08`），字段名与版本号与本 revision 一致（`protocolRevision=ws08-peak-nondeposited-v1`、`peakMetricRevision=shake-peak-nondeposited-v1`）。

## 12. 资产索引与交接

| 资产 | 路径 | 用途 |
|---|---|---|
| 本 revision | `docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md` | 判定协议权威（口径/窗口/身份/分档/双算） |
| 适配清单 | `docs/peak-adaptation-checklist-P011-ws08-peak-nondeposited-v1.md` | 旧全序列 max → 新口径的差异与期望行为 |
| RED 基线 | `docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md` | 诚实 RED 记录与双算归因框架 |
| 执行证据 | `test/reports/peak_protocol_v1_execution.json` | 分档/反例/必要条件/归档勘查/双算链的重跑产物 |
| 执行器 | `test/p011_peak_protocol_v1_execution.py` | tester 侧只读执行器（非生产 checker） |
| 依据设计 | `docs/design-P011-ws08-peak-nondeposited-migration-v1.md` | 候选取舍、反例、可达性推导 |
| 终点协议（不改） | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` | 终点残余与末 1s 窗的权威 |
| 用例条目 | `docs/test-cases.md` §15 | 本 revision 的版本化用例条目 |
| 适配对象（不改代码） | `test/water_sand_check.py::evaluate_shake()` | 现行 `peakSusp` 全序列 max 行为 |

交接：manager 验收 → architect 登记独立 peak 字段 revision → developer 实现采样/流式峰值/checker → tester 同账本双报告 → designer 效果验证 → manager 宣布 `peakProtocolEffective`。

---

## 13. 追加记录

- 2026-10-25：v1 首次发布。依据 designer 迁移设计稿（manager 19:15 验收）与用户五项裁决（manager 23:55 下达：采纳候选 B、分子含全部活粒子含 state=2、分母用 E、固定 shake-on 后 3s 窗每提交采样、原 0.5 及格线与分档锚点迁移），冻结 `peakNonDepositedFraction` 身份、分子 `M=U+P`（M1–M8）、分母 `E=VH0+VL0+IH+IL`、固定摇晃窗 `W=[t_on,t_on+3000ms)` 与每提交采样身份、分档锚点（10/8/6/4/2/0）与 `M≥0.5E` 及格比较（无新放宽容差）、分相诊断（`E_phase=0` 记 N/A、分相峰不可加）、五码状态分流与判定顺序、同 run 双算（三个 legacy 峰值量 + `sameReferenceSuspOnlyPeak` + `OBSERVED_SAMPLED_MAX` 下界）、差异归因分离（窗口/密度/分子分母）、八条反例与 `M0+300·tau_W≥0.5E` 必要条件、与 endpoint 协议的严格边界（不改一字、不借末 1s 最大值）、九项不涉及项。本 revision 不含 estimand 变更，不宣布 `peakProtocolEffective`（宣布权在 manager），不使用真机。
