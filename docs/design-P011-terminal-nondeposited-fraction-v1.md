# P-011 路线 C：终点未沉积沙比例方案 v1

> 最新交付：**§7 designer 方案定稿 v1.1（2026-10-24）**。§1–§6 保留为 v1 历史提案；冲突处以 §7 为准。定稿指设计内容完整，可交 manager 转用户裁决，**不代表协议获批或效果 VALIDATED**。

日期：2026-10-24。责任：designer。状态：**DESIGN_PROPOSAL / READY_FOR_MANAGER_TO_ROUTE_TO_USER / NOT_APPROVED / NOT_IMPLEMENTED**。本稿给出唯一推荐方案，供 manager 转用户最终裁决；用户裁决前不修改 `goals.md`、冻结验收指标、tester 预注册 estimand、代码或测试。输入：[路线 C 工程稿 §3.3/§5.6](architecture-P011-dual-field-particle-v1.md)、[路线 C 预注册](preregistration-P011-dual-field-particle-v1.md)、[现行 SLIP/TC 用例](test-cases.md)和 `WaterSandField.getMetrics()`。

## 1. 结论与现行语义

**推荐将路线 C 的终点 `≤5%` 判据改为“所有尚未进入沉积层的沙”，即列悬沙 `suspH+suspL` 加活粒子池全部质量；阈值仍为 5%。** 一份沙只处于沉积、列悬沙、粒子三桶之一。这里沿用架构字段名 `airborneFraction`，其语义为**活粒子池中尚未沉积的质量**，包括在水中飞行、在水面之上飞行及 `state=2` 接触沙床但等待合法落床的粒子；它不是“位于空气中的沙”或“速度非零的沙”的字面分类。

现行 `getMetrics()` 以 `Σ(suspH+suspL)/totalSand` 输出 `suspensionFraction`，未有粒子桶；[TC-WS-08](test-cases.md) 用这个量判断摇晃峰值 `≥0.5` 与静置 8s 后残余 `≤0.05`。[SLIP-03](test-cases.md) 的**正式终点门槛却不是同一分母**：相位终点 `Σ(suspH+suspL)≤0.05×run 首子步 stateBefore 的 Σ(bh+bl)`，坡度稳定另看最后 1s。历史 [SLIP 报告](test-report-P011-slip-v1.md) 的“悬沙≤5% PASS”沿用这个旧口径，不能追认包含粒子。`goals.md` G2.6 要求停止摇晃后 ≤8s 再沉降堆积，但没有把飞行粒子单列为新沙相。三桶仍是沙相内部库存，不改 H/L/水守恒目标。

## 2. 公式、单位与分母

对同一**已提交物理子步** `k`，均用 model material-volume units（现有 `bh/susp/pMass` 的同一单位，不是像素或粒子个数）：

```text
D(k) = Σ_c [bh[c]+bl[c]+mh[c]+ml[c]]
U(k) = Σ_c [suspH[c]+suspL[c]]
P(k) = Σ_{p: pState[p]∈{1,2}} pMass[p]
M_nonDep(k) = U(k)+P(k)
V_expected(k) = VH0+VL0+sourceInjectedH(k)+sourceInjectedL(k)
V_actual(k) = D(k)+U(k)+P(k)
```

`state=0` 空槽不计；`state=1` 飞行、`state=2` 静置待准入**均计**。`MERGE` 后只计合并包当前质量一次。已通过 `DEGRADE`/`TO_SUSP` 转到列悬沙的量只计 `U`，**不再计 `P`，也不能再把 `particles.degraded.massH/massL` 累计事件量加进分子**。部分 `LAND` 授予后留在池中的余量按其当前质量计 `P`，已提交到 `mh/ml` 的授予量计 `D`。B 路线纯视觉 `P012SandTracer` 一律不计。要求 `V_actual≈V_expected` 先过现有分相守恒硬闸门；若分母非正、字段缺失、非有限、粒子缓存与逐粒求和不一致，结果为 `INVALID_LEDGER/NOT_IMPLEMENTED`，绝不当作零或 PASS。

**不悄改已有分母**，按用例身份定义 `R`：

| 终点用例 | 推荐判定量 | 分母 `R` | 原因 |
|---|---|---|---|
| `TC-P011-SLIP-03` 每相位 | `F_SLIP(k)=M_nonDep(k)/R_SLIP` | `R_SLIP=Σ_c(bh[c]+bl[c])`，取 run 首子步 `stateBefore`，整个 run 冻结 | 原正式 5% 门槛只扩分子，不移动其冻结基准；`mh/ml` 未在旧基准内，不能顺手加入 |
| `TC-WS-08` 摇晃后静置 8s | `F_WS(k)=M_nonDep(k)/V_expected(k)` | 累计期望 H+L 总量，含已准入外源一次 | 与现 `getMetrics.totalSand` 在守恒通过时等价到数值容差；若有源事件，按其准入后总量归一化 |

架构 §5.6 的分项展示字段按**同一个 `V_expected` 分母**定义：`suspFraction=U/V_expected`、`airborneFraction=P/V_expected`、`depositedFraction=D/V_expected`；三者相加应为 1（守恒容差内）。`WS_METRIC.suspensionFraction` 保留旧名和旧 `U/V_actual` 计算法，用于历史报告对照，不能在 C 路线作单独终点放行。SLIP-03 checker 必须从 v7 原始质量重算 `F_SLIP`，**不得直接把显示用 `suspFraction+airborneFraction` 与 5% 比较**，因为它们的分母是 `V_expected`，不是 `R_SLIP`。报告须同时列 `R_SLIP/V_expected`、`U`、`P`、`M_nonDep` 与对应比例，避免两个“5%”混名。

## 3. 终点与判定规则

**阈值不变：** `F≤0.05`；5% 是现行残余容许值，不从 §3.3 的粒子容量反推新阈值。先校验守恒、库存、容量、guard、预算和 route 身份，再判效果终点；任一硬闸门失败不得用 5% PASS 掩盖。

**预先固定终点 `t_e`：** SLIP 取预注册每个相位的最后一个已提交子步；TC-WS-08 取 `shake=0` 后静置满 8.0s 的最后一个已提交子步。不得在曲线上挑第一次低于 5% 的子步、延长观察到通过为止，或用 250ms `WS_METRIC` 抽样替代 v7 每子步账本。终点取 `stateAfter` 与同子步 `particles` 摘要，禁止把粒子提交前的 `stateBefore` 与提交后的悬沙相加。物理时间戳与 run/phase/substep 身份必须单调、连续可核；缺末端子步或窗口数据记 `INCOMPLETE`。

**稳定窗口：** 为防单步瞬时低谷误判，路线 C 新 revision 推荐要求固定终点前 1.0s（含 `t_e`）内**每个已提交子步** `F(k)≤0.05`，等价 `max_window F≤0.05`；同时单列披露 `F(t_e)` 与窗口峰值。SLIP 的坡度最后 1s `≤1°` 判据仍独立执行，不改 30°±2°/R²/target-based t90 量测；TC-WS-08 仍是原 8s 静置窗口，不能用这 1s 缩短等待。若固定窗口未覆盖完整 1s，不作 PASS。窗口内必须无新 `GESTURE_EXTERNAL` 准入、无摇晃、无 reset/invert/模式或 route 切换，`sourceEpoch` 恒定；相位内预注册的恒定倾角可保留。输入事件落进窗口或事件时序不明，标 `ENDPOINT_NOT_QUIET/INCOMPLETE`，由既定 schedule 重跑，不以重选窗口求绿。持续手势没有静置终点，直到预注册另给停止输入与观察窗，不能套用本门槛。

```text
if missing_v7_postcommit_samples_or_mass_check_failed: NOT_IMPLEMENTED/INVALID_LEDGER
if last_1s_not_fully_covered_or_endpoint_not_quiet: INCOMPLETE
R = first_stateBefore.sum(bh+bl) if SLIP-03 else VH0+VL0+IH+IL
if R <= 0: INVALID_DENOMINATOR
endpointFraction = (U(t_e)+P(t_e))/R
windowPeakFraction = max_{k in [t_e-1s,t_e]} (U(k)+P(k))/R
endpointMassGate = PASS iff windowPeakFraction <= 0.05; otherwise FAIL
```

SLIP-03 不含手势外源；若该 run 在首子步后发生外源注入，应判场景身份不符并重跑，不能仍用旧 `R_SLIP` 却把新增沙纳入终点分子。TC-WS-08 如有预注册允许的外源变体，则按每子步累计期望量更新分母，但终点窗口内仍不得继续注入。

**滞留与降级：** `state=2` 永久待准入不会从分子消失；若比例超 5% 判 FAIL。若 `≤5%` 却在最后 1s 持续有 `state=2`，本指标可报数值 PASS，但另报 `CONTACT_WAIT_PERSISTENT`，C 路线整体出口不得只凭 5% PASS 签字；须查 `LAND` guard/budget 拒绝及接触余量生命周期。`DEGRADE/TO_SUSP` 后仍以当前 `U` 计，累计降级事件量只是诊断；池满不提供扣除、豁免或第二分母。若未提供 `state=2` 分项遥测，则 5% 可以从池总量重算，但滞留诊断记 `NOT_OBSERVABLE`，不能宣称落床闭环已证。

## 4. 遥测、现有用例与 revision 接口

| 接口 | 推荐对接规则 |
|---|---|
| v7 ledger | 每个 `stateAfter` 同子步使用 `particles.massH/massL`（两者之和=`P`）与 `stateAfter.suspH/suspL`；`particles.count`、`bySourceKind`、`degraded` 沿 §5.6 原名。为了判断长期接触滞留，建议在后续**字段 revision** 追加 `particles.byState.{FLYING,CONTACT_WAIT}.{count,massH,massL}`，快照粒子块同时保留 `pState`；该字段是诊断扩展，不偷改已冻结 DUALFIELD-* estimand。字段未落地前不虚构 state 分项。 |
| `WS_METRIC` 与 checker | 增加 §2 三分项；原 `suspensionFraction` 保留旧义。C checker 从原始 v7 账本重算终点量，记录 `endpointMetricRevision=terminal-nondeposited-v1`、`endpointReferenceKind=SLIP_INITIAL_BH_BL/EXPECTED_TOTAL`、`endpointReferenceMass`、`windowPeakFraction`、`endpointFraction`。这些是**拟议报告字段**，用户裁决后由 tester/architect 按协议 revision 定名，不在本稿改预注册。 |
| `SLIP-02/03/04` | `30°±2°`、R²≥0.90、target-based t90≤5100ms 的 raster/时间估计不变；SLIP-03 终点质量门槛按本稿扩分子且终点窗单列 revision，最后 1s OLS 稳定、安全闸门和 SLIP-04 预算对账不动。SLIP-02 仍是阶段二 DEFERRED，不能因新终点量 PASS 追认为整包 PASS。 |
| `TC-WS-03/08/09` | `t90Heavy/Light` 仍从**真实已沉积**分相质量除当前累计期望求首次 90%，有源 epoch 内旧 t90 无效；在途粒子不得算作 deposited。TC-WS-08 的静置残余改判本稿 `F_WS`；原 `peakSusp≥0.5` 是**另一个冻结估计对象**，C 把 lift 从 susp 改写粒子后它可能结构性 FAIL。本稿不暗改峰值；建议另案走 `peak(susp+airborne)` 的协议 revision，获批前不能以新峰值替代旧用例。TC-WS-09 的再堆积读真实床层，分项曲线并报。 |
| `DUALFIELD-*` 16 例 | 守恒/预算/生命周期/来源与 GES-01/02 的预注册 estimand 原样保留。GES 的首粒/漏段/落点/落床 grant 仍按事件测；只在完成手势且预注册静置窗后附本终点诊断，不给 GES 另造 5% PASS。`CONS-04` 的累计期望分母及 epoch 纪律正是本稿的输入。 |
| P-012 E2/E4 | E2 顶光只读渲染效果、E4 沙丘面积与画面占比均不从 `airborneFraction` 推出 PASS；E4 沉积沙丘 mask 只取沉积床，不把飞沙云当沙丘增高/面积；E2 材质光照测量保持原场景快照/掩膜身份。粒子渲染可在相同帧旁证，但不改两项已冻结或候选锚点。 |

**迁移顺序（须先有用户最终裁决）：** ① manager/用户批准这一版的分子、两种历史分母、5% 和最后 1s 窗口；② tester 发布 `TC-WS-08`/`SLIP-03` 的 endpoint protocol revision（峰值口径如要改另起 revision），保留原 estimand 与历史结果；③ architect/developer 追加 v7 同子步摘要与 metric 输出，checker 双算旧/新并校验 `D+U+P≈V_expected`；④ 用同 build/输入做路线 OFF、B、C 对照，OFF/B 的粒子质量视为结构性零但**原始旧 ledger 不补造粒子字段**；⑤ C run 双口径并报，直到 revision 正式生效，新口径单独终点 PASS 方可参与 C 出口验收。旧报告保留原 PASS/FAIL 与 `legacy-susp-only` 身份；只有原始 v7 账本具备真实粒子摘要、同子步时间戳时才能离线补算候选新值，补算值标 `RETROSPECTIVE_DIAGNOSTIC`，不追认旧验收。

## 5. 量级、反例与边界

架构 §3.3 的**候选而非测量值**：摇晃最多可申请约 300 unit/s，若飞行约 1.5s，则在途峰值量级约 **450 units**；`Nmax=512` 且 nominal `m_i=0.05/0.1/0.25` 时，未合并的池名义容量分别 **25.6/51.2/128 units**。不足部分可能进入 `susp`，因此旧分子是否漏判取决于合并/降级比例与实际总沙量。**`Nmax×m_i` 不是 T6 合并后的严格质量上界**：合并包可大于原始 `m_i`，正式上界需实际 `pMass`/`particles.massH+massL` 和包质量限制；不能把 128 当作所有场景的必然粒子质量封顶。

条件示例：假设某端点 `V_expected=8000` units、`U=322`、`P=128`、`D=7550`，守恒且总未沉积为 450。旧总量口径 `322/8000=4.025%` 会给**假 PASS**，本稿 `450/8000=5.625%` 为 FAIL；128 只是上述名义容量示意，非实测。另一端若 450 中有 398.8 降入 `U`、51.2 留在 `P`，旧为 4.985% 仍可能假 PASS，新仍为 5.625%。只看 `particles.count` 或 `degraded` 累计量更会把质量包和转列后的历史量错当当前库存。

**同分母、非负库存时，旧 `U/R` 的 FAIL 不可能被新 `(U+P)/R` 判为 PASS**，所以旧“假 FAIL”不能归咎于加粒子这一项；若出现，必是分母/时间戳混用、数据污染或讨论的是视觉“看似已落”而非物理已入床。新量也不保证完全稳定：例如 `P/R=4%` 的 `state=2` 永久静置会数值 PASS、但物理未落床，须由 §3 的接触滞留诊断和独立生命周期审查拦住整体签字。反过来，已落在床面上但 guard 不准提交的 `state=2` 若使比例 >5%，画面可能“看着已落”而本量 FAIL；这是按 canonical 沉积语义的**正确 FAIL**，不能以改渲染 mask 当作新质量已沉积。摇晃结束前或源持续注入时的短暂 >5% 不叫终点失败；若固定静置 8s/相位末仍超限，则不是可事后延时排除的“假 FAIL”。

## 6. 交 manager 转用户的裁决摘要

请批准一套明确口径：**C 路线终点分子=`Σ列悬沙+Σ活粒子质量（含 state=2）`；原用例各自的分母不变；阈值 5% 不变；固定终点前最后 1s 每个已提交子步均≤5%，且终点前无新沙源/摇晃；分报 `suspFraction/airborneFraction/depositedFraction`。** 这修复漏计真粒子的假 PASS，同时避免把“在床面视觉静止但尚未合法落床”的质量抹掉。与现有最大差异是 C 路线增加粒子分子和最后 1s 连续达标要求；`TC-WS-08 peakSusp` 的迁移另案裁决。批准后按 §4 出 protocol revision；批准前保持 C run 双口径并报、任何单口径都不单独放行。

## 7. designer 方案定稿 v1.1（2026-10-24，待用户最终裁决）

**状态：DESIGN_FINAL / READY_FOR_MANAGER_TO_ROUTE_TO_USER / NOT_APPROVED / NOT_IMPLEMENTED / NOT_VALIDATED。** 任务：[6adc52e4…1949](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adc52e4e4b06ee357191949)。原因：逐项复核任务五项要求及现行源码，补齐可实现契约，纠正 v1 对 SLIP 无摇晃条件、时间戳同值与 t90 已实现状态的过强表述。仅追加设计；不变更 goals、用例、预注册、工程稿或代码；不阻塞 D0–D2。

### 7.1 唯一推荐及候选取舍

**推荐终点统一统计“未合法入床的全部沙”=`列悬沙+活粒子质量`，保留每个历史用例的分母和 5% 阈值；新 revision 再要求固定终点前最后 1s 全部采样达标。** 两个改动（扩分子、增加质量稳定窗）均须用户明确批准；批准前只输出候选结果。术语采用“终点未沉积沙比例”，避免把 `airborne` 字面解释成仅空气中的粒子。

| 候选 | 决定 | 理由 |
|---|---|---|
| A：只算 `U`，维持旧悬沙分子 | REJECTED_FOR_C_ENDPOINT | 可把大量真库存留在粒子桶，旧悬沙比例仍低于 5%，漏判未沉积量 |
| B：`U+P_FLYING`，排除接触待准入态 | REJECTED | `state=2` 尚未获得 LAND grant；排除会奖励 guard/budget 阻断，使“看似落床”冒充真实沉积 |
| C：`U+P_all_live`，原分母、5%、固定末 1s 窗 | **RECOMMENDED** | 不受飞行/接触/转列表示方式影响；与真实床层和三桶守恒一致；窗口防止挑单步低谷 |
| D：提高阈值，或全部改用同一总量分母 | REJECTED_THIS_REVISION | 无效果证据支持新阈值；统一分母会同时移动 SLIP 冻结基准，混入无关标准变更 |

### 7.2 可直接实现的口径、单位与数据输入

同一成功提交子步 `k` 的 post-commit 库存定义如下。全部为现有模型 material-volume units；比例为无量纲，展示百分数才乘 100。位置、速度、粒子个数、像素面积均不是质量代理。

```text
D_H = sum(stateAfter.bh + stateAfter.mh)
D_L = sum(stateAfter.bl + stateAfter.ml)
U_H = sum(stateAfter.suspH); U_L = sum(stateAfter.suspL)
P_H = particles.massH;       P_L = particles.massL
D = D_H+D_L; U = U_H+U_L; P = P_H+P_L; M = U+P
E_H = VH0 + context.sourceInjectedH
E_L = VL0 + context.sourceInjectedL
E = E_H+E_L; A = D+U+P
R_SLIP = sum(firstRunRecord.stateBefore.bh + firstRunRecord.stateBefore.bl)
R_WS = E
F_SLIP = M/R_SLIP; F_WS = M/E
```

- `P_H/P_L` 必须等于 `pState∈{1,2}` 的 `pMass` 按 `pPhase` 求和；`state=0` 不计。水内、水面上方及接触待准入粒子全部计入。摘要与逐粒缓存/事件账本须闭合，不能同时相加摘要和逐粒和。
- LAND 仅扣 `granted`，已入 `mh/ml` 的部分计 D，余量留同 id/state=2 计 P（与工程稿 rev1/manager 15:35 裁定一致）。T3 不隐式转悬沙；若未来启用显式 TO_SUSP，提交后只计 U。DEGRADE 同理；`particles.degraded.*` 是累计事件量，永不加入当前 M。MERGE 后只计现存包质量一次。B tracer 无质量，不计。
- `VH0/VL0` 沿预注册 §2.4，来自 reset 后、任何外源准入前的首条 `audit.*TotalBefore`。采集如从注入后开始且无法证明初值身份，记 `INVALID_BASELINE`，不能把已注入库存再加一遍 IH/IL。`R_SLIP` 按旧用例仅取 bh+bl，整 run 冻结，不能偷偷加入 mh/ml。
- 每子步先过冻结守恒：`abs(D_H+U_H+P_H-E_H)≤1e-9`，L 同理，以及既有水/库存/容量/有限值/guard/预算/身份闸门。任一未过，不给正式终点 PASS。任一必要字段缺失记 `NOT_IMPLEMENTED(missing_field)`，断档记 `INCOMPLETE`；`R≤0` 记 `INVALID_DENOMINATOR`，不以 EPS 替代使空沙场景通过。
- §5.6 的显示名固定为 `suspFraction=U/E`、`airborneFraction=P/E`、`depositedFraction=D/E`；三项共用 E，并同时记录原始质量。守恒通过时和约为 1。`WS_METRIC.suspensionFraction` 保留旧公式 `U/max(A,EPS)`，新旧字段不得互为别名。**SLIP 正式候选判定从 M/R_SLIP 重算，不能直接使用显示分项之和 M/E。**

### 7.3 固定终点、采样窗口和防误判规则

| 场景 | 固定终点及分母 | 输入资格 |
|---|---|---|
| TC-P011-SLIP-03 | 每个预注册 phase occurrence 的固定结束边界之前最后一个成功提交子步；R_SLIP | 整 run 无外源准入；保留原 tilt/shake schedule，不额外要求 shake=0 |
| TC-WS-08 | 预注册 shake-off 生效后满 8.0s 的固定结束边界之前最后一个成功提交子步；E | 最后 1s 外部 shake 输入为 0，无新外源准入、reset/invert/模式/route 切换；残余 shakeEnergy 按原动力学衰减，不要求浮点精确为 0 |
| TC-WS-09 | 保留原“静置 10s、末段 5s”采样段；同批报告中的残余判据按获批 revision 用 M/E，不能改成 WS-08 的 8s 终点 | 原场景输入和末段窗口不变；新末 1s 窗嵌在既定末段内 |
| DUALFIELD-GES-01/02 | 原五观测点/同路径 estimand 不变，不新增通用 5% 门槛 | 连续手势不是静置终点；只能在另行预注册的 stop+观察窗附本指标诊断 |

**覆盖 v1 §3/§6：** 不把“无摇晃”作为所有 SLIP 相位的资格。现行 `pages/SandCanvas.ets` 在 thick 分支每帧执行 `setShakeInput(min(1,abs(effectiveTilt)/90))`；±15°/±45°平台本就有非零派生输入。把它设零会改场景；本方案只更改终点量，不修改该耦合。SLIP 依原输入仍须通过未沉积量判据，若失败如实 FAIL。

采样与时间契约：

1. 以 `(runId, sequence, frameId, substepId)` 连接同一次提交的 `stateAfter` 与 `particles`；phase occurrence 用命令/阶段身份区分，不把重复同名 phase 拼接。context/metric 的 `physicsTime` 用于各自阶段审计，**不要求时间戳逐值相等**：现代码 context 在 begin 时取值，metric 在 finish 后另取值。
2. 时间窗使用预注册 schedule 的单调实际 elapsed 时钟（ms），与命令接受/阶段边界同源，不把 `sum(physicsDt)` 当真实静置时间。`nextP011TimeMs()` 当前来自 `Date.now()` 且多次调用递增，既不是纯 simulationTime，也不能单独证明完整 1s 覆盖；完整 elapsed 边界信息缺失时记 `INCOMPLETE`。新 revision 建议追加 capture 字段 `endpointSampleElapsedMs`（成功提交后、相对 run 起点的单调实际 elapsed），并披露 `integratedPhysicsSeconds=sum(physicsDt)` 作诊断；这是新字段提案，不改旧时间戳定义。
3. 观察固定边界 `t_e` 前闭区间 `[t_e-1000ms,t_e]`，包含边界处的已提交状态，全部成功提交样本均须 `M(k)≤0.05×R(k)+epsMass`。新质量比较的 `epsMass=1e-9 unit`，复用现 SLIP checker 的绝对数值容差，比例展示四舍五入不参与判定；质量/分相守恒闸门仍独立执行原容差。记录固定边界、实际末样本 elapsed、endpointFraction 和 windowPeakFraction，不能以最后一条残缺日志反推提前结束的 `t_e`。
4. 边界附近须有采集完整性证据，确认观察持续至预定边界且子步未丢失。若物理进入合法休眠，可用带时间/身份的只读状态复核证明同一提交状态保持至边界；不能伪造新提交子步。缺这类证据不外推、不插值、不拿 250ms 的 WS_METRIC 代替全子步窗口。冻结/失败回滚、漏记子步或采集提前终止不能作为休眠证据。
5. 最后 1s 的质量窗是**新增且待批准的要求**，原 SLIP 最后 1s 的 OLS 坡度变化≤1°仍独立执行。质量超限记录 FAIL；资格/覆盖不足记 INCOMPLETE，不挑首个达标点、不延时等绿。新的外源场景如获预注册，WS 分母 E 含窗口前实际准入一次；原 WS-08/09 无源 schedule 本稿不扩成有源场景。

```text
validate_route_build_protocol_and_fixed_boundaries()
validate_all_committed_records_and_existing_hard_gates()
if required_fields_missing: return NOT_IMPLEMENTED
if ledger_or_baseline_invalid: return INVALID_LEDGER / INVALID_BASELINE
if endpoint_coverage_or_input_identity_invalid: return INCOMPLETE
freeze R_SLIP at first run stateBefore  # WS uses current E
for each successful postcommit sample k in fixed last-1s window:
    read D, U, P, E from this same commit
    R = R_SLIP if SLIP else E
    if R <= 0: return INVALID_DENOMINATOR
    massWindowPass &= (U+P <= 0.05*R + 1e-9)
    windowPeakFraction = max(windowPeakFraction, (U+P)/R)
emit endpointFraction at fixed terminal sample
emit massWindowPass, windowPeakFraction, legacySuspOnly, hardGateStatus
# approval pending: above is CANDIDATE_DIAGNOSTIC, never automatic release
```

**长期不落与满额：** FLYING 或 CONTACT_WAIT 不因速度小、贴床、超时、池满而从 P 扣除；超过 5% 照常 FAIL。若永久残余仅占 4%，5% 本身仍可数值 PASS，故它不能证明“全部落完”或“无永久悬浮”。报告并列 `airborneFraction` 与可观测的 state 分项、同 cohort 的等待/飞行持续时间、LAND/guard/budget 阻断。持续同粒滞留输出 `PERSISTENT_PARTICLE_RESIDUE` 诊断并交 manager 按既有无永久悬浮/生命周期证据审查；**不凭任意 1s 或假设 1.5s 新设寿命 FAIL 阈值**。没有逐粒/状态时间证据记 `NOT_OBSERVABLE`，不得宣称长期不落已排除。5% 数值 PASS 不替代其余出口条件。降级时 P 减 q、U 加 q，所以 M 不变；MERGE 也不改变 M，无法靠池策略改善本指标。

### 7.4 接口迁移与报告身份（拟议，批准后执行）

| 影响面 | 定稿对接要求 |
|---|---|
| ledger v7 | 保留架构 §5.6 `particles.{count,massH,massL,bySourceKind,capacityUsed,capacityLimit,degraded}` 与事件原名，语义为 post-commit 存量；追加 elapsed/状态诊断先由 architect 登记字段 revision。`byState`/pState/粒子年龄当前并非全部已冻结或已实现，不假称已有。无粒子摘要的 C run 不默认为 P=0 |
| WS_METRIC / checker | 三分项按 §7.2 输出。建议报告 `endpointMetricRevision=terminal-nondeposited-v1.1`、`endpointReferenceKind`（`SLIP_INITIAL_BH_BL` / `EXPECTED_TOTAL`）、`endpointReferenceMass`、`endpointFraction`、`windowPeakFraction`、`endpointStatus`、`endpointWindowStatus`、`endpointAuthority=PROPOSED/APPROVED`；分开记录失败闸门与不可观测项，不把 schema v7 当成方法已批准 |
| SLIP 目标包 | 30°±2°、target-based t90≤5100ms、R²≥0.90、原阶段适用性全保留；原 OLS/target-based t90 不读取粒子作为已沉积床面，不因新残余 PASS 追认 SLIP-02 或 P-011 CLOSED |
| 沉降 t90 | 与 target-based 坡度 t90 区分。CONS-04 要求真实沉积分相质量/E_H或E_L、sourceEpoch 隔离；在途不得算沉积、活动源 epoch 不沿用旧首次90%。**源码核对缺口**：`getMetrics()` 的 depositedHeavy/LightFraction 已用 initial+injected，但 `updateMetrics()` 的 t90 仍除 initial，且注入分支仅在 `t90Heavy<0` 时重写 -1，未清已触发 t90，light 亦需核查。故不能把工程稿“epoch 重启照旧”当作实现证明；交 architect/developer 在 C 集成检查，tester 按既有 CONS-04 验证。本卡不修代码或改 estimand |
| WS-08/09 | 残余项迁移本稿分子，保留各自时序；WS-08 原 `peakSusp≥0.5` 及分相旧观测仍照报。本稿不把新 `peak(U+P)/E` 替代冻结 peakSusp；如需迁移峰值，另起 protocol revision 和用户裁决。不能把只过残余项写成整个 WS-08 PASS |
| DUALFIELD-* 16例 | CONS/BUD/LIFE/SRC/GES estimand 原样。GES 五观测点和同路径判据零改动，时延阈值仍待试采；本方案不能解除真机/计时前置。质量诊断可旁列，不替代事件/床面增长验收 |
| P-012 E2/E4 | 顶光 E2 使用原画面/ROI/受光定义，E4 沙丘占比/几何只取真实沉积床层；飞沙云、接触待准入和视觉 tracer 不并入沙丘 mask。录像/ledger 同帧对齐仅作旁证，不修改 E2/E4 已冻结或候选锚点 |
| 历史/OFF/B | 旧报告原结论保留，标 `legacy-susp-only` 及原分母/协议身份；C 新 revision 不追认旧 PASS。确认 route OFF/B 且没有真实粒子层时，候选数学对照可取结构 P=0，但不得向旧文件补造 v7 字段，不要求 C 对其他路线全量 dominance；只在同路线内比较基线改善 |

**迁移步骤：** manager 接收完整定稿→转用户裁决 §7.1 的分子/原分母/5%/最后1s质量窗；批准后 tester 发布独立 endpoint protocol revision（保留 DUALFIELD v1 estimand）→architect 对齐 post-commit/时间/诊断字段 revision，developer 实现→checker 同 run 双算旧/新，做守恒、部分LAND余量、MERGE/DEGRADE 不改 M 的核查→同 build 同输入分别运行路线自身基线与候选，出路线专属双报告→designer 验证→manager 确认适用范围。C 原型出口前本设计须已交付，**协议尚未批准时继续双口径并报，任一单口径都不单独放行**；不提前阻塞 D0–D2。回算如有真实原始三桶记录，只标 `RETROSPECTIVE_DIAGNOSTIC`，不追认方法生效。

### 7.5 定量依据、风险反例与可验收性

现行依据链：`goals.md` G2/G2.6 的“停止后≤8s再沉降与堆积稳定”→`test-cases.md` TC-WS-08/09 的残余≤5%→`WaterSandField.getMetrics()` 的 U/A；另一路为 TC-P011-SLIP-03 的 U/R_SLIP，见 `test/p011_slip_formal_check.py` 的 `initial_deposit_total`/`phase_endpoint`，历史 `test-report-P011-slip-v1.md` 只证明该旧量。原源码/报告均无真实粒子，不能证明 C 的 P 已沉积。新旧同分母差值严格为 **P/R**；因此只加粒子时不会把旧 FAIL 变 PASS。

| 条件反例（全部为假设，非测量） | 旧口径与新口径 | 判断 |
|---|---|---|
| E=8000，D=7550，U=322，P=128，末窗保持 | 旧4.025%，新5.625% | 旧假PASS；新正确FAIL。P=128只是未合并候选名义容量示例 |
| 同总量，把其中76.8粒子降到列：U=398.8，P=51.2 | 旧4.985%，新仍5.625% | 转列可改变旧结论，新M不变，不奖励降级 |
| D=E−0.04E，U=0，P=0.04E且同粒永久待准入/飞行 | 旧0%，新4% | 两者数值均PASS，但若当成“完全落床”均是假PASS；须另审生命周期/无永久悬浮，不能删P或凭5%结项 |
| 某末窗早期M/R=6%，固定末步4% | 旧单终点可PASS；推荐质量窗FAIL | 对新“末1s持续达标”是正确FAIL；若仍用旧单步定义会被称假FAIL，必须显式revision，不混标 |
| R_SLIP=7000、E=8000、M=375 | 显示M/E=4.6875%，SLIP M/R=5.3571% | 错用显示分项会假PASS；两种分母必须并报、各自判定 |
| LAND提交20，提交前P=80，提交后U=30、P=60 | 正确M=90；错混提交前P得110 | 若R=2000，正确4.5%，混时5.5%假FAIL；仅统计同提交身份 |

**架构 §3.3 的量级解释：** ASSUMPTION：每秒都能把300 units共享额度用于lift、平均在途1.5s，存量规模约450；LAND/CROSS/其他通道也争该额度，故450不是已证可达峰值，更不是静置终点的预测。候选 Nmax=512、未合并等质量包 m=0.05/0.1/0.25时名义库存25.6/51.2/128；若还强制每包质量≤m，这才是严格上限。**现 T6 允许同相包合并增质量，不能把 Nmax×nominal m 当作合并后质量封顶**；严格界需额外包质量上限契约或直接读实际池质量。本设计不更改合并规则、不暗加该上限，向 manager/architect登记工程稿 §3.3 “封顶”说法与合并语义的条件差异。漏计比例的最坏偏差应由实际 P/R 给出，不能拍定永远不超过128/E。

实现量测成本：扫描128列+至多Nmax个槽，O(128+Nmax)/子步；已有正确池摘要可将在线质量累计降为O(128)，独立checker仍重核摘要/事件；预定窗口可流式维护末样本、最大值与资格标记，O(1)累计状态（逐粒滞留诊断另用O(Nmax)）。不新增物理事务、draw或预算。

设计交付检查（不是执行测试）：五项要求齐备；反例质量和/百分比可重算；同分母 `F_new≥F_old`；DEGRADE/MERGE不改M；LAND使M只减granted；缺字段/分母无效/缺窗口不PASS。真正C方案验收仍待获批revision、实现和路线专属客观/主观双报告，不在本卡标VALIDATED。

**交 manager 的最终建议：采用候选 C。** 批准的是清楚的物理口径和新增末窗，不是新阈值或新的粒子寿命标准；峰值悬沙迁移另案处理。当前可交接、可实现的方案已定稿，用户裁决权保留。

### 7.6 追加记录

- 2026-10-24：v1.1定稿。保留v1历史；补充候选方案取舍、post-commit身份/实际elapsed时钟、WS-09残余迁移、绝对质量容差与输入资格；纠正SLIP统一要求shake=0、时间戳必须同值、t90 epoch已实现及Nmax×m名义容量可当严格上限的误读；不改冻结文件、代码或测试。探索复盘见 [retro-P011-terminal-nondeposited-v1.md](retro-P011-terminal-nondeposited-v1.md)。
