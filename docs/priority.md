# 优先级队列（priority）

> 维护人：manager。一轮聚焦最多 1~3 个高优先级问题。
> **任务固定编号台账见 `docs/task-registry.md`**（每个 task 一个终身固定编号，卡关闭后编号退役不复用）。每轮后续工作清单从该台账取编号，并按优先级排序。

## 2026-10-25 09:10 固定编号台账建立（当前生效）

依据：用户 2026-10-25 09:05 指示「后续工作清单里每个 task 的 #，固定一个就数字就不要每次都变，还是按优先级排，# 可以乱序」。

1. **台账载体**：`docs/task-registry.md`（manager 维护）。三张表——「编号 → task 对照」（含 task / 角色 / 优先级 / 卡 ID）、「已关闭退役编号」、「使用规则」。
2. **编号规则**：每个 task 一个终身固定编号；卡关闭 → 编号退役并登记，**退役编号不复用**；新卡取下一未用最大编号 + 1。现有 T01–T25，退役 R01–R11。
3. **清单排序**：每轮清单**按优先级排序**（P5 → P4 → P3），因此**编号出现顺序与优先级顺序不一致是正常的**——编号只用于跨轮次指代同一件事，不是排名。用户按编号指代（如「T05 验收」）即指该卡。
4. **台账与本文件的关系**：本文件（priority.md）记验收结论与排期裁决；`task-registry.md` 记编号归属。两者分离，避免在 priority.md 里改编号。

## 2026-10-25 09:50 三件事优先级重排（用户 09:45 再次明确三件事定义）+ T03 验收通过关闭

依据：用户 2026-10-25 09:45 指示——「1. 仿真线：高度场+粒子场的仿真方式，分别仿真沉积的沙子和扬起的沙子 2. 渲染线：使用 XComponent + shader 3. 交互线：通过触摸交互掉沙，并在地上累积，掉沙用粒子仿真，累积用高度场仿真 —— 相关的提优先级，不相关的降优先级」。用户同时指示「5完成」（= T05 掉沙接线，已于 09:44 通过关闭）。

### 三件事的明确口径（本次为最权威表述，与 10-24 14:15 重排一致并更细化）

1. **仿真线**：**高度场 + 粒子场双场**，**分别**仿真——**沉积的沙子**走高度场（累积到床面），**扬起的沙子**走粒子场（活动粒子）。二者职责不可混。
2. **渲染线**：**XComponent + shader**（P-013 换栈）。
3. **交互线**：**触摸交互掉沙**，**掉沙用粒子仿真**（注入为活动粒子），**累积用高度场仿真**（落地进床面高度场）。

### 相关性判定与优先级调整（dida365 API 枚举 P5=5 / P4=3 / P3=1 / 最低=0）

| 编号 | task | 角色 | 调整 | 归入 |
|---|---|---|---|---|
| T01 | D1-R3 M1 粒子场热接口（零分配收口） | developer | P5 维持 | ①仿真线（粒子场本体） |
| T02 | D2 M2 场集成（高度场+粒子场九接入点=双场合并） | developer | P5 维持 | ①仿真线（双场合并出口） |
| T04 | 终点指标 D-4/O-2（粒子与高度场账本字段） | developer | P4→**P5** | ①仿真线（双场账本完整性） |
| T07 | peak 字段 revision（扬沙采集身份） | architect | P4→**P5** | ①仿真线（扬起沙的时序身份） |
| T08 | peak 实现（扬沙采样与流式峰值） | developer | P4→**P5** | ①仿真线（扬起沙量测） |
| T09 | P1 CPU fixture + 五量 RED（G1 前置） | tester | P4→**P5** | ②渲染线（XComponent+shader 前置） |
| T10 | 掉沙 V1-V4 真机返修复测收尾 | effector | P4→**P5** | ③交互线（触摸掉沙真机取证） |
| T13 | 方案3b 起沙链入库 | developer | P3→**P4** | ①仿真线（扬起沙的 susp 通道侧） |
| T12 | S4 sweep 三轮 | developer | P3→**0** | 不相关（深蠕动，非三件事） |
| T28 | S4 物理定义（pEff/rhoEff） | designer | P3→**0** | 不相关（同上） |
| T14–T19 | 大角度路线 B/C/D/E/F/G | 多角色 | P3→**0** | 不相关（新 route，非三件事） |
| T22 | 方案3 悬浮平流启动裁决 | manager | P3 维持（带 blocked 标签） | 不相关 |
| T26 | goals.md 四子项落地 | designer | P3→**0** | 不相关 |
| T27 | P-012 C 补采 | effector | P3→**0** | 不相关 |

**T03（D-2/D-6 实施）验收通过并关闭**（编号退役 R14）：commit `93a9922`（5 文件 +329/−8）已入库——`P011NaturalObservation.ets` +4、`P011ParticlePrototype.ets` +10（`eventLevelValidationOwnership` / `transactionLayerEventValidationRole='EMIT_AND_ATOMIC_COMMIT_ONLY'` / `substepId` 逐笔标注）、`p011_endpoint_check.py` +230（诊断三具名回填 + `eventValidationFailures`）、`test_p011_endpoint_check.py` +89（三个 D-2/D-6 用例）、`p011_endpoint_protocol_v1_execution.py` +4。manager 复跑：Python **25/25 OK**、协议反例 **10/10 MATCH**、Node endpoint **9/9**——全绿，零回归。

**T04 复验（09:50）**：`a5e197f`（+13）已落 `row['endpointSampleStage']` 与 `row['externalAdmissionCount']` 回填（:665），新增断言覆盖「占位值不得掩盖缺样本」「三字段不在 reportMissingFields」——**D-4 已落地**。但 **O-2 仍未落地**：`WaterSandField.ets` 传给 `stampEndpointObservation` 的第 8 实参**仍是 `this.sourceEpoch`**（与第 7 实参同值）→ `EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH` 仍结构性不可触发。**T04 卡保持开启**（一项完成、一项未完成）。

**T01 复验（09:50）**：`5fd647e`（`feat(p011): complete S1 fixed-buffer dual particle collect`，+192）已入库——**S1 双 collect 切片落地**。hot 单测 **11/0**（新增四例：S1 全相位候选与运动对 64 个冻结场景等于冷参考 / S1 比对 lineage 并拒绝相等 NaN、别名 plan 与伪造容量 / S1 精确 Q/N 边界 + stale epoch + 溢出不改 canonical / S1 重复粒子 ID 与非法 state 不写 canonical）。但探针仍 **exit=2 / 60 处**、`applyParticlePlanInto` / `preparePatchHot` 仍零命中 → **S2/S3/S4 未开工，T01 保持开启**。

**T13 复验（09:50）**：起沙源族仍**未入库**（`WaterSandField.ets` 与 `run_p011_advection_units.cjs` 仍为工作区 M）→ **保持开启**，因它是①仿真线「扬起沙子」的 susp 通道侧证据，已提至 P4。

## 2026-10-25 09:45 T09 验收（tester，6adcc00b…0dda）——**零交付，尚未开工，卡保持开启**；同时 T05 入库通过关闭（T10 真机数据反证掉沙修复已生效）

依据：manager 09:38–09:45 独立复核。用户 09:38 指示「9完成」。

1. **T09 P1 CPU fixture + 五量 RED 基线（tester）零交付**：`docs/` 与 `test/` 下未找到任何本卡产物——CPU fixture 用例登记文件、RED 基线记录、`docs/test-cases.md` 里的 P1 用例条目（`TC-P013*`/`TC-P1*` 计数 = 0）**全部缺失**；git log 09:00 后三笔（`1511fbf` D1-R3 S0、`e117a60`/`4591b46` peak 协议、`1249c07` 掉沙接线）**均非本卡**。判定**尚未开工**（非交付不合格）。
2. **G1 解冻前置确认仍缺**：P1 工程稿 rev2 §9 第 2 项与签收表明写「G1 资产/CPU 参考」的解冻前置 = 本 rev2 经 manager 验收 + **tester 登记 CPU fixture/材质耦合用例及 RED 基线**，且 §9 末尾明写**「G0 真机 probe 仍须先过，不得用 rev2 文档完成代替 G0 实测」**——即 T09 是 G1 的**两项前置之一**，另一项是 G0 真机 probe（需 effector 队列独占）。**T09 卡正文已加开工提示**（列出四项工作的完整规格与产出路径），保持开启。

## 2026-10-25 09:45 T05 掉沙接线入库验收（developer，6adca854…cdcc）——**通过并关闭**（编号退役 R13）+ T10 真机数据反证修复已生效

3. **commit `1249c07` 已入库**（09:38:32，`fix(p011): commit gesture sand wiring repairs`，3 文件 +173/−8）：`SandCanvas.ets` +107、`WaterSandField.ets` +29、`run_p011_gesture_inject_units.cjs` +45。**六项符号逐个核实齐全**：`gesture_sand` 1 处、`shake_lift` 1 处、`pan_update` 2 处、`sourceInjectedHeavy` 2 处、`setShakeLiftEventSink` 3 处、`resetPanGestureBoundary` 4 处。
4. **★hunk 精选执行正确**（这是 manager 本轮最担心的风险点）：`git show 1249c07` 中 `peak` / `shakeSource` 相关新增行数 = **0** —— **T08 的 peak 字段族与 T13 的 `shakeSource*` 起沙源族均未被误提交**；`git status` 显示 `WaterSandField.ets` 仍为 M（其它卡的改动原样留在工作区）。**四卡共用同一文件的冲突被正确化解**。
5. **★T10 真机返修数据反证掉沙修复已生效**（manager 直接解析 `test/artifacts/p012-fallsand-fixretest-20261025/`，37 个文件，含 jpeg 截图与事件/指标日志）：**`WS_EVENT gesture_sand` 逐笔日志已在真机落出**——C 长时注入场景 1135 笔（sourceId 分 24 段、**admittedH 合计 4540 / admittedL 合计 4540、拒绝 0 笔**）、D 场景 41 笔（admittedH/L 各 164、拒绝 0）、A-swipe 场景 88 笔（各 352、拒绝 0）；`WS_EVENT pan_update` 同步落出（1159 / 42 / 89 笔）且 `coordinateSource:"finger_local"`、`accepted:true`（**坐标回退修复生效，不再用累加后的 lastPanLocalX**）；`WS_EVENT shake_lift` 在 B 控制场景落 7128 笔（`lifted` 与 `columns:128` 齐全，**shake Want 直通生效**）。**这与首轮回执「掉沙准入为零、shake 未进物理场」直接相反**——两项功能验收的 BLOCKED 条件已解除。
6. **一处需登记的观察（非缺陷）**：C 场景 `sequence` 在跨 `sourceId` 时重置（44 → 1，见 09:36:33.140 的 sourceId=3 起点），**按 sourceId 分段后每段内严格单调**（24 段全部 True）——符合 `docs/algorithm-P011-gesture-sand-source-v1.md` §4「每笔外源事务带 sourceId/sequence」的语义（sequence 是**单个 sourceId 内**的事务序号），**不判为缺陷**。
7. **另一处需登记的观察（待 effector 说明）**：A-drag 场景（`A-events-drag-v200.txt`）有 `shake_lift` 247 笔但 `gesture_sand` **0 笔**——该场景只落 shake 事件、未落掉沙笔（可能是拖拽手势未被识别为沙源路径，属场景差异而非修复失效；D 场景与 A-swipe 场景均正常落出）。**T10 回执文档尚未落盘**（`docs/` 下只有首轮的 `p012-fallsand-rerecord-receipt-v1.md`），artifact 目录也未入 git → **T10 卡保持开启**，待回执成文并入库。
8. **T05 关闭后真机队列首位仍为 T10**（设备已由 effector 独占，37 个 artifact 已产出）——**P6 endpoint 合规设备 run（T11）必须等 T10 回执入库后再排位**，不得并行占用设备。

## 2026-10-25 09:35 六卡验收（T01/T04/T05/T07/T12/T13）——**T06 补交通过关闭；T13 起沙链落地；T12 诚实 BLOCKED；T04 做了 T03 的活；T01/T05/T07 未动**

依据：manager 09:28–09:35 逐项独立复核 git 提交、工作区 diff 与文件存在性。用户 09:27 指示「1,4,5,7,12,13完成，验收」。

### T06 peak 协议（tester，6adcd3ba…f8c8）——**补交四项全部落地，通过并关闭**（编号退役 R12）

1. **commit `e117a60` 已入库**（09:28:02，6 文件 +1802 行）：`docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（318 行）+ `docs/peak-adaptation-checklist-P011-ws08-peak-nondeposited-v1.md`（100 行）+ `docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md`（113 行）+ `docs/test-cases.md`（+60 行，§15 `TC-P011-PEAK-01..08`）+ `test/p011_peak_protocol_v1_execution.py`（602 行）+ `test/reports/peak_protocol_v1_execution.json`（609 行）。**上轮指出的 5 项缺失全部补齐**。
2. **manager 复跑执行器**：`counterexamples=8 verdictMatches=8`（P1–P8 逐条 MATCH）、`tierTable/reachability/phaseSplit/dualChain` 全 MATCH、`redStatus=NOT_IMPLEMENTED`、`legacyFullSequenceTakesPreShake=True`——**结论未被改软**。
3. **RED 基线的诚实性经核实（关键）**：全量 hilog 归档扫描 `test/artifacts/*.txt` 共 **83 个文件**，`WS_EVENT` 191 条、`WS_METRIC` 1896 条，**`WS_EVENT shake` 事件 0 条** → 即现存 250ms 抽样数据**全部来自无摇晃输入的 run**，连「同轨迹下界」都构造不出来；因此新量在全部现存归档上为 `NOT_IMPLEMENTED`，本基线不提供也不接受任何数值峰值结论，旧 `peakSusp` 的 `BLOCKED` 结论原样保留不追认不推翻。归因链已量化分离（窗口效应 −5.5pp / 密度效应 +3.0pp / 分子分母效应 +7.5pp），未把差异全归因于物理改善。协议源 SHA-256 `e9929058…` 已登记。
4. **适配清单**按 10 个差异点组织，含「会因此改变的结论（迁移后必须重算且不得追认旧值）」与「不受影响的结论（适配不得触碰）」两栏，适配对象正确指向 `test/water_sand_check.py` 的 `evaluate_shake()`。
5. **协议仍未生效**：`peakProtocolEffective=false`，`peakAuthority=APPROVED` 只覆盖方法口径。T08（developer 实现）与 T07（字段稿对齐）仍待放行。

### T13 方案3 b 起沙链（developer，6adc6272…f9dd）——**B-1/B-2/B-3 落地，卡保持开启（未入库）**

6. **起沙源登记已落地（此前 manager 误判为未实现，本轮更正）**：实现用的是 `shakeSource*` 命名族而非「起沙源/liftSource」字面——`shakeSourceLiftedH/L`、`shakeSourceLandedH/L`、`shakeSourceDegradedH/L`（`WaterSandField.ets:397-400` 公开字段、:994-997 私有累计）、逐列 `shakeSourceH[c]`（:2893 进入 `sourceTransitH`）。**上轮 manager 用中文关键字 grep「起沙源/liftSource/SANDING_SOURCE」判为「B-1 未实现」是误判**——英文命名族未被中文检索覆盖。
7. **守恒与互斥断言已落地**：`test/run_p011_advection_units.cjs` 新增用例 `adv_shakeLiftSourceConservationAndRouteExclusion`——先断言 `shakeSourceLiftedH/L > 0`（起沙源已登记），再逐列扣减后断言 `shakeSourceLifted - ΣshakeSourceH[c] ≈ shakeSourceLandedH/L`（**守恒成立**），最后置 `endpointParticleRouteActive = true` 后再 `liftByShake` 断言累计量不变（**粒子通道激活时 susp 起沙源被排除，不双记**）。
8. **判定**：B-1/B-2/B-3 三项实质落地且断言齐备，**但未入 git**（`WaterSandField.ets` 与 `run_p011_advection_units.cjs` 仍是工作区 M，且与 T08 的 peak 改动同文件共存）→ **卡保持开启**，标题标「三段守恒与通道互斥断言已落地，待入库」。

### T12 S4 sweep 三轮（developer，6adcd331…a94b2）——**诚实 BLOCKED，卡保持开启**

9. **交付**：`docs/report-P011-S4-proxy-upgrade-sweep3-v1.md`（13 行）+ `test/reports/s4-proxy-upgrade-sweep3.json`（09:22 落盘，均未入库）。**状态 `BLOCKED_MISSING_PRESSURE_DEFINITION`**——与建议稿 §3 的规定完全一致：先冻结量纲再扫 μ。
10. **根因判定属实**：现有实现的 `pEff` 是相邻列 `sandTop` 的几何平均（约为长度/材料高度代理），代码把 `deltaBh` 与 `muY*pEff` 直接比较，**缺少有效压力 `pEff(Pa)`、有效密度 `rhoEff` 与固定参考压力，两边量纲不闭合** → 第 2 轮（从固定非均匀场驱动比 `P_drive/p_eff` 分位数生成候选 μ）因分位数不可计算而 BLOCKED、候选网格留空，第 3 轮（2–8px 域复验稀疏低/中/高 μ）因依赖第 2 轮未执行。**这正是卡正文要求的「不得用扫参替代定义」情形**。
11. **判定**：**这是诚实且正确的 BLOCKED，不是零交付**——把「无法从现有字段定义压力」如实登记并把缺失的物理定义交 designer/manager，未拿 μ=0 基线冒充三轮完成。**卡保持开启**（完成条件要求三轮产出），正文改为「补物理定义后再跑第 2/3 轮」。

### T04 终点指标 D-4/O-2 修复（developer，6adcd497…0569）——**两项目标均未落地，实际做的是 T03 的活，卡保持开启**

12. **D-4 未落地**：manager 精确 grep `row['endpointSampleStage']` → **零命中**；`test/p011_endpoint_check.py` 中 `endpointSampleStage` 仍只出现在 `:421`（读 `c.get`）、`:436-437`（缺字段判定）、`:667`（必填清单）、`:761`（commitStatus 判定）——**报告行构造处仍不含该键**，即双报告补充1 §1 变动表第 7 行指出的原始 D-4 未被修复。
13. **O-2 未落地**：`WaterSandField.ets:3601` 传给 `stampEndpointObservation` 的第 8 实参仍是 `this.sourceEpoch`（与第 7 实参同值），**与 tester 补充1 §2.1 观察 O-2 描述的现状完全一致**——「字段与 sourceEpoch 恒等、`EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH` 在真实 run 上结构性不可触发」这一缺陷未被修复。
14. **实际做的是 T03 的 D-2/D-6**：`test/p011_endpoint_check.py` 新增 `row['eventLevelValidationOwnership'] = 'ENDPOINT_CHECKER'`、`row['eventValidationFailures']`（从 hard failures 过滤 `PARTICLE_EVENT_` 前缀）、诊断三具名字段回填（`particleResidueDiagnosticStatus`/`Codes`/`Meaning` + 缺字段进 `reportMissingFields`）；`P011ParticlePrototype.ets` 新增 `eventLevelValidationOwnership` / `transactionLayerEventValidationRole='EMIT_AND_ATOMIC_COMMIT_ONLY'` / `substepId` 并在事件构造处逐笔标注（**事务层只 emit-and-atomic-commit、声明「本职责不在本层」**）；`test/test_p011_endpoint_check.py` 新增三个用例（`test_d2_diagnostic_fields_have_one_consumer_name_and_no_fabricated_missing_values` / `test_d6_event_checker_accepts_enabled_transaction_schemas` / `test_d6_event_checker_rejects_bad_mass_substep_and_merge_double_count`）。**这些是 T03 卡的工作范围，不是 T04 的**。
15. **manager 处置**：**T04 卡保持开启**，正文改写回两项原目标（D-4 报告行回填 + O-2 改真实外源准入计数），并明确「T03 的 D-2/D-6 实现已由另一张卡承担，不要重复做」；**T03 卡**同步记录这些改动已在其名下，避免两卡互相覆盖或重复实现。T04 的两项改动**尚未入 git**，须与 T03 的改动一并协调提交。

### T01 D1-R3（developer，6adcc2c1…1cf05）——**无新进展，卡保持开启**

16. **09:35 实测**：`P011ParticleHot.ets` mtime 仍 **09:00**（与 S0 提交同批，此后未动）；7 个关键接口逐个 grep 仍**全零命中**（`applyParticlePlanInto` / `preparePatchHot` / `publishPatchHot` / `guardAdmitInto` / `reserveInto` / `refundInto` / `closeInto`）；探针仍 **exit=2 / NOT_ZERO_ALLOCATION / 60 处**。**S0 之后未开工 S1**，09:05 的验收结论不变。

### T07 peak 字段 revision 收尾（architect，6adcd3d3…51f1）——**未动，卡保持开启**

17. **09:35 实测**：`docs/architecture-P011-peak-fields-revision-v1.md` mtime 仍 **09:08:59**（本轮无修改）；状态行仍为 `FIELD_DESIGN_READY / PENDING_TESTER_PROTOCOL_ALIGNMENT / PENDING_MANAGER_ACCEPTANCE / ...`；§13 处说明仍写「本地尚无 tester 独立 peak protocol」，`peakFieldAlignment` 仍为 `PENDING_TESTER_PROTOCOL`。
18. **★关键时序已变**：T06 的 tester 协议已于 09:28 入库（`e117a60`，协议源 SHA-256 `e9929058…`），**T07 的 3 项待对齐现在具备对齐对象**（此前 architect 写稿时协议尚未落盘，故如实标 PENDING 是正确的）。卡正文已写明「协议现已落盘，可补做 hash 核对」——**T07 收尾现在可立即开工**。

### T05 掉沙接线入库（developer，6adca854…cdcc）——**未入库，卡保持开启**

19. **09:35 实测**：`git log -- entry/src/main/ets/pages/SandCanvas.ets` 最近仍是 `34c7087`（方案3 a 阶段），**HEAD 版本的 `gesture_sand` 命中数为 0**；三个文件（`SandCanvas.ets` / `WaterSandField.ets` / `run_p011_gesture_inject_units.cjs`）**仍全部是工作区 M**。六项代码仍在工作区，未提交。
20. **文件冲突已加剧**：`WaterSandField.ets` 的工作区 diff 现为 **+244 行**，其中 T05 的部分只是 `sourceInjectedHeavy/Light` 与 `setShakeLiftEventSink`，其余是 T08（peak 字段）与 T13（`shakeSource*` 起沙源族）——**三张卡共用这一个文件**。T05 卡正文已写明必须按 hunk 只提交自己的部分。

## 2026-10-25 09:20 peak 协议（T06）与 peak 字段 revision（T07）验收——**两份稿本体均通过，引用资产与用例条目缺失，卡不关闭**

依据：manager 09:12–09:20 独立复核两份落盘稿（均**未入库**，`git status` 显示 `??`）。用户 09:12 指示「6完成，1完成」→ 6 = T06（peak 协议）、1 = T01（D1-R3）。

### T06 peak 协议（tester，6adcd3ba…f8c8）——**协议本体通过，5 项引用资产 + §15 用例条目缺失，卡不关闭**

1. **协议本体 `docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md`（317 行，09:11 落盘）通过**：身份登记齐全（`protocolId=p011-ws08-peak-nondeposited` / `protocolRevision=ws08-peak-nondeposited-v1` / `peakMethodId=peakNonDepositedFraction` / `peakAuthority=APPROVED` / 状态 `METHOD_FROZEN / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_CALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION` 且 `peakProtocolEffective=false`、生效宣布权归 manager）。§1 冻结口径四小节齐备（分子 M=U+P 含 M1–M8 细则 / 分母 E=VH0+VL0+IH+IL / 分档锚点 10-8-6-4-2-0 与 `M≥0.5E` 无新容差 / 分相诊断 E_phase=0 记 N/A 且分相峰不可加）；§2 摇晃窗与采样身份；§3 判定式与峰值选取（先同提交求和再取 max）；§4 判定顺序（顺序不可调换）与状态码（均不得记 PASS）；§5 双算规则（三个 legacy 峰值量 + `sameReferenceSuspOnlyPeak` + `OBSERVED_SAMPLED_MAX` 下界 + 恒等式自检 + 差异归因分离）；§6 **8 条自检反例**（表示漏计 / 降级奖励旧量 / 跨时刻加峰值假 PASS 80% vs 正确 40% / 输入平台响应滞后 / 全序列污染假 PASS / 接触库存伪装运动 / 250ms 漏峰假 FAIL / 停止后才高）+ **可达性必要条件 `M0+300·tau_W≥0.5·E`**（明确否定「改成 U+P 就自然能过 0.5」，并给出 M0=0/tau=3s 时需 E≤1800 的算例，且声明「不是对实际 build 的无解证明」「不暗增 300/s、不延长摇晃、不调低 0.5、不减总沙量求绿」）；§7 与已批准终点 revision 的严格边界（不改一字）；§8 生效规则与七步执行序列；§9 九项不涉及项；§10 报告字段契约六组。
2. **★五项引用资产与用例条目均未落盘（manager 逐个查文件存在性，全部 MISS）**：`docs/peak-adaptation-checklist-P011-ws08-peak-nondeposited-v1.md`（适配清单）、`docs/peak-red-baseline-P011-ws08-peak-nondeposited-v1.md`（RED 基线）、`test/p011_peak_protocol_v1_execution.py`（只读执行器）、`test/reports/peak_protocol_v1_execution.json`（执行证据，协议 §6 明写「逐条 MATCH 方可采信」）、`docs/test-cases.md` **§15**（`TC-P011-PEAK-01..08` 版本化用例条目）。协议 §12 资产索引把这五个路径列为资产，§8.2 第 1 步明写 tester 本卡应「发布本 revision + §15 版本化用例条目 + checker 适配清单 + RED 基线记录」——**卡正文四项工作（协议 / 适配清单 / RED 基线 / 用例预注册）目前只落地第一项**。协议内 §6 已把 8 条反例的**期望数值**写全（如反例 3：k1 U=400/P=0/D=600、k2 U=0/P=400/D=600 → 分别取峰相加得 80% 假 PASS、正确总量峰 40% 不及格），故 RED 基线与执行器可据此直接构造，属**可实施规格已足**、不是设计缺口。
3. **manager 判定**：**协议本体通过并作为下游实施依据**；但**卡完成条件（四项落地 + 三个自测零失败 + 每个数字可追溯到重跑产物 + commit）未满足**——缺 4 个文件 + §15 用例条目 + 未入库。**卡保持开启**，正文改写为「补齐五项 + 入库」单一收尾动作；不改判据、不降低要求。
4. **对下游的影响**：T08（developer peak 实现）依赖本卡的适配清单与字段契约才能开工 → **T08 暂缓于本卡补齐之后**；T07（architect 字段稿）已可独立验收（其依据是设计稿 §5.3 拟议字段，不依赖本卡正文）。

### T07 peak 字段 revision（architect，6adcd3d3…51f1）——**字段稿本体通过，3 项待 tester 对齐已如实登记，卡不关闭**

5. **`docs/architecture-P011-peak-fields-revision-v1.md`（256 行，09:08 落盘）本体通过**：§3.1 新增原始输入事件流 `peakShakeInputEvents[]`（eventId / `peakInputCommitOrder` / effectiveElapsedMs / `endpointClockId` / value / requestedValue / kind∈{SHAKE_ON,OFF,UPDATE} / shakeOnEventId / caseOccurrenceId / `endpointPhaseOccurrenceId` / commandSequence / sourceKind∈{SCHEDULE,TILT_DERIVED,MANUAL}），**on/off 边界四字段**（`shakeOnEventId` / `shakeOffEventId` / `shakeOnEffectiveElapsedMs` / `shakeOffEffectiveElapsedMs`，**复用既有同名 off 边界、不给 endpoint 另造时钟/别名**）+ `peakWindowStart/EndElapsedMs`（后者 = t_on+3000，**不能改成 t_off 或数据最后时间**）；§3.2 每成功提交的消费证据（`context.peakFieldRevision` / `context.peakInputCommitOrder` / `context.peakConsumedShakeInputEventId` / `context.peakConsumedShakeOnEventId` / `context.peakConsumedShakeInputValue`，明确「在子步真实读取输入时固定，collect/preview/apply 共享同一快照，post-commit 只复制，不得在 finish 时回读最新输入」）；§5 checker 独立报告字段字典 **43 个字段逐项给类型/语义/时机/必需性/缺失处理**（含 `peakMasses` 六类质量与 H/L 分项、三显示名沿批准口径、三个 legacy 栏目各自 denominatorKind/sampleDensity/window/evidenceKind/status、`peakParticleByState`、`particleResidueDiagnosticStatus` 复用、`hardGateStatus`、`naturalnessEvidenceStatus`、并明写「报告必须区别数值子项 / hard gates / endpoint / 自然度与生命周期结果」「尚未独立生效时即使内部 peakStatus=PASS 也标 `CANDIDATE_DIAGNOSTIC / FORMAL_RELEASE_NOT_AUTHORIZED`」）。
6. **★同毫秒全序的落点已明确**（这是 designer 设计稿里最易被实现错的一处）：**事件与提交两处同名 `peakInputCommitOrder`**，由物理侧单一计数器串行分配、禁止 worker 重排或用 JSON 文件顺序猜测；成员判定先满足 `t_on ≤ context.endpointSampleElapsedMs < t_on+3000` 再查消费身份与共享全序；on 之前的同 ms 提交排除，off 同 ms 之前的提交仍需满足固定半开窗，恰等于 `t_on+3000` 一律排除；全序**不把右端同 ms 的提交挪回窗口**；时钟分辨率无法证明边界归属时记 INCOMPLETE 交 tester 定义，**不伪造小数时间或容差**。
7. **manager 交叉核对（协议 §10 vs 字段稿 §5）**：核心 30 个字段名**逐字一致**（`peakNonDepositedFraction` / `peakStockMass` / `peakReferenceMass` / `peakCommitKey` / `peakSampleElapsedMs` / `peakWindowSampleCount` / `peakWindowCoverageStatus` / `peakScore` / `peakStatus` / `missingFields` / `peakNonDepositedHeavy/LightFraction` / `legacyPeakSuspReported` / `legacyPeakHistoricalReference` / `legacyPeakLiveFormula` / `sameReferenceSuspOnlyPeak` / `baselineNonDepositedFraction` / `peakNetIncreaseFraction` / `cumulativeLiftGrantedMass` 等）；字段稿比协议**多 30 个字段，全部集中在诊断层**（`peakMasses` 及其子字段、`peakBoundaryEvidencePaths` / `peakMissingCommittedOrdinals` / `peakDuplicateCommitKeys` / `peakFailedAttemptCount` / `peakIntegratedPhysicsSeconds` / `peakWindowIntegratedPhysicsSeconds` / `peakInputEligibilityStatus` / `peakInputEligibilityReasons` / `peakCommittedOrdinal` / `peakInputCommitOrder` / `peakFieldRevision` / `peakShakeInputEvents` / `peakProtocolSourceHash` / `legacyPeakEvidence` / `legacyPeakResult` / `naturalnessEvidenceStatus` / `hardGateFailures` / `endpointResultReference` 等）——**无冲突、无同名不同义**（协议 §10 本就写明「拟议落点，待 architect 登记」，字段稿即该登记的落定）。
8. **三项待 tester 对齐（architect 已如实登记 `PENDING_TESTER_PROTOCOL_ALIGNMENT`，未假装无差异）**：①正式 tester 协议文件/版本/hash 当时尚未交付（现 T06 协议已于 09:11 落盘，可补做 hash 核对）；②新增输入事件/消费字段的精确名称与结构；③同 ms 不确定边界的协议接纳与最终状态优先级。**这 3 项正是 T06 补齐后要做的事** → 合并到 T06 的收尾动作里一并做，不另起卡。
9. **manager 判定**：**字段稿本体通过并可作为 T08 实施的字段依据**；但**卡完成条件（四项齐备 + 对照表无未决差异 + 红线自检）未满足**——对照表 13 行中 3 行待对齐、且未入库。**卡保持开启**，正文改写为「补齐与 T06 协议的对齐核对 + 入库」。

### 处置与派单

10. **T06 正文改写为单一收尾动作**（不重做协议）：补齐 4 个文件（适配清单 / RED 基线 / 只读执行器 / 执行证据 JSON）+ `docs/test-cases.md` §15（TC-P011-PEAK-01..08）+ 对 architect 字段稿的 hash/版本核对与 3 项待对齐的收口 + 入库 commit。协议正文**不得再改**（已冻结口径）。
11. **T07 正文改写为单一收尾动作**：待 T06 补齐后完成 3 项对齐（正式协议 hash / 事件与消费字段精确名 / 同 ms 边界接纳规则）+ 更新对照表 + 入库 commit。字段字典**不得另造一套**。
12. **T08（developer peak 实现）暂缓**，等 T06/T07 对齐收口后放行——避免按未定稿的字段名实现后再返工。

## 2026-10-25 09:10 D1-R3 零分配实现验收（developer，6adcc2c1…1cf05）——**S0 切片通过，S1–S4 未开工，任务不关闭**

依据：manager 09:05–09:10 独立复核 commit `1511fbf`（`feat(physics): add initial particle hot workspace`，3 文件 +483：新增 `entry/src/main/ets/physics/P011ParticleHot.ets` 364 行 + `P011ParticleField.ets` +32 + 新测试通道 `test/run_p011_particle_hot_units.cjs` 87 行）。

1. **已落地并实测通过的部分**：新增 `P011ParticleHot.ets` 提供 `P011ParticleHotConfig` / `P011ParticleHotFieldBuffer` / `P011ParticleHotPlanBuffer` / `P011ParticleHotResultBuffer` / `P011ParticleHotWorkspace` 五个类型，PlanBuffer 用 `Uint8Array`/`Int32Array`/`Uint32Array`/`Float64Array` 预分配 request/motion/lineage 定容行，ResultBuffer 用 `Int32Array` 定容——**「不用 push/slice/subarray/map/filter/Array.from、行长度固定为 capacity」这一条在已写部分成立**。新通道单测 **7/0**（configure 定容与 plan 缓冲互异 / 非法 config 在建 workspace 前被拒 / begin 在改 epoch 或 field 状态前校验输入 / step 身份复制标量输入且多 step 缓冲稳定 / stale epoch 拒绝后 workspace 无副作用 / 双 collect 数值化且确定性 / request 容量溢出在发布有效 plan 前被拒）。既有通道回归 manager 实测 **particle_field 33/0、particle_refund 11/0、gesture_inject 18/0、planning 106/0**，零破坏。
2. **19 个热接口命中实测（manager 逐个 grep `entry/src/main/ets/physics/`）**：`configureHot`/`warmUpHot`/`beginStepHot`/`copyStateToHot`/`checkpointHot`/`collectParticlePlanInto`/`compareParticlePlansHot`/`rollbackStepHot`/`finishStepHot`/`nextLeaseToken`/`expectedStepEpoch`（8 处引用）**已落地**；**`applyParticlePlanInto`/`preparePatchHot`/`publishPatchHot`/`restorePatchHot`/`discardPatchHot`/`finishPatchHot`/`guardAdmitInto`/`reserveInto`/`refundInto`/`closeInto` 共 10 个仍为零命中**。热模块当前实际只暴露 `copyStateToHot` 一个方法（另三个为构造函数）。
3. **零分配探针结论**：`test/probe_p011_particle_allocations.cjs` → **exit=2 / NOT_ZERO_ALLOCATION / remainingSiteCount=60**（P011ParticleField.ets 31 + P011ParticlePrototype.ets 29；kind：object-literal 22、allocating-helper 20、array-literal 12、constructor 4、closure 2），与 10-24 23:41 基线**完全一致**——S0 按设计**不迁移任何分配点**（只建骨架），故探针不降是预期结果，但**卡完成条件要求 ZERO_ALLOCATION 且 exit=0**，故本卡不满足完成条件。
4. **S0 判据三条逐条核对（迁移设计稿 §5 S0 行）**：①固定 capacity/typed-array 引用及长度在多 step 不变 → **已满足**（单测第 4 例覆盖）；②坏 owner/alias/size 输入零副作用 → **已满足**（单测第 2/3/5 例覆盖 config 非法、begin 输入校验、stale epoch 零副作用）；③旧 33+11 既有用例保持原结果 → **已满足**（manager 实测 33/0 + 11/0）；④**AST registry 仍完整含 60 条并新增 Reachability 列 → 未满足**（探针 `test/probe_p011_particle_allocations.cjs` 仍输出 `{file, owner, line, kind, text}` 五字段，无 Reachability 列；S0 判据把它划在本片，S4 判据又要求「原 60 逐项新状态 + 文件/行/Reachability 证据」——两处口径需 manager 明确归属，见下条）。
5. **manager 裁定（09:10）**：**S0 切片验收通过**（三条实质判据已达标，第四条的 Reachability 列归 S4 统一收口，不阻塞 S0）；**本卡不关闭**，因为卡完成条件是整体 ZERO_ALLOCATION + 五组验证 + 22 项遥测，而 S1–S4 尚未开工。**任务保持开启**，等 S1（双 collect 计划生成）落地后由 manager 复验；不改判据、不降门槛。
6. **归口明确（避免两卡抢同一处）**：探针 registry 扩展（新增 Reachability 列 + 保全原 60 条 ID）**归 S4**，S0 阶段只在探针输出中保留现状；S1–S3 期间探针的 `remainingSiteCount` 应随各片迁移进度下降，但**静态项不得在 S4 之前标 PASS**。
7. **后续切片入口（迁移设计稿 §5 依赖图 `S0 → S1 → S2 → S3 → S4`）**：S1 双 collect 计划生成（收口 F01–07、F17–31 的 HOT 替代 + 固定排序/request/motion/identity + plan A/B compare）→ S2 数值 patch 与 Pool 发布（F08–16 + prepare/edit/publish/restore/finish + 复用 transactionBefore/After/Scratch）→ S3 Prototype sink + Guard/Budget/R9 固定事务（P01–29 + fixed guard/receipt/event/result + R9 refund + 事务全序 + 原子回滚）→ S4 全图门槛与原 60 结项。

## 2026-10-24 23:55 WS-08 峰值指标迁移裁决（用户已批准五项，当前生效）

依据：`docs/design-P011-ws08-peak-nondeposited-migration-v1.md`（designer，manager 19:15 验收）+ `docs/retro-P011-ws08-peak-nondeposited-v1.md`。用户 2026-10-24 23:55 裁决：**五项全部采纳候选 B**。

1. **新量身份**：独立命名 `peakNonDepositedFraction` = 固定摇晃段内、每个成功提交子步的未沉积沙比例 `(U+P)/E` 的最大值。语义=「摇晃期间最多有多少已准入沙尚未合法进入床层」，**不**回答「摇动输入最强瞬间有多少沙」，也**不**证明全部非沉积量正在流动或由本轮新近扬起。
2. **分子**：含全部活粒子（P 精确为 `pState∈{1,2}` 按相求和，水内/空气/接触待准入均计；state0 与视觉 tracer 不计）。
3. **分母**：`E`（累计期望，WS-08 原无外源场景故 E=VH0+VL0 恒定）；E≤0 记 `INVALID_DENOMINATOR`，初值/注入前身份不可证记 `INVALID_BASELINE`。
4. **采样窗**：固定 `W=[t_on, t_on+3000ms)`，只取该窗内实际消费该 shake-on 输入的全部**成功提交子步**；窗口右边界（shake-off 后提交）不纳入；不用 250ms 抽样估峰值（250ms 旧数据只能记 `OBSERVED_SAMPLED_MAX`，是同轨迹真 max 的下界，不是合格证据）；连接键 `(runId,sequence,frameId,substepId)` + `context.endpointPhaseOccurrenceId`，采样复用 `context.endpointSampleElapsedMs`，另记 `integratedPhysicsSeconds`。
5. **阈值与分档**：保留原 `0.5` 及格线（比较为 `M≥0.5E`，**不新增向下放宽的容差**）与原分档锚点（≥0.90→10、≥0.75→8、≥0.50→6、≥0.35→4、≥0.20→2、<0.20→0）。分相另报 `peakNonDepositedHeavyFraction`/`peakNonDepositedLightFraction`（某相 E_phase=0 记 N/A，不 EPS 造比例），总量峰值提交同时披露 H/L 分项，独立分相峰值不得相加。
6. **与已批准终点口径的边界**：已批准终点 revision（M=U+P 含 state2、WS 用 E、5% 与 epsMass=1e-9、固定末 1s 每提交达标）**不改一个字**；峰值无 5% 要求，也不借用末 1s 最大值作摇晃峰值。两者时段与阈值分别判断。
7. **legacy 双报与不追溯**：旧 `peakSusp`/`peakSuspHeavy/Light` 保留原身份、原初值分母与旧列悬沙语义，旧结果带 `legacy-susp-only` 及原 build/protocol/采样身份，旧分数与结论保留，不互为别名。旧 C 账本若具备真实 P、同提交、完整 W 及必要时钟字段，可离线补算为 `RETROSPECTIVE_DIAGNOSTIC`，**不追认旧验收**、不向旧文件补造 v7 字段。
8. **执行序列**：tester 独立 peak protocol + RED 基线 + 用例预注册 → architect 登记独立 peak 字段 revision（复用既有 commit/elapsed/particles 契约，补 shake-on/off 有效边界与子步实际消费输入身份）→ developer 实现采样/流式峰值/checker → 同 build 同输入同账本双算（另报共同 250ms 样本对照，把分子/分母差异与采样密度差异分开）→ effector 真机串行采集 → designer 效果验证 → manager 确认适用范围与 `peakProtocolEffective`。**批准口径 ≠ 协议生效**，未实现/未验证不得直接放行。
9. **诚实性登记**：历史直接证据为 P003/P005 的 BLOCKED RED，未找到正式数字峰值结论，无证据不宣称历史通过。分母差异不是无条件 P/E（守恒成立时 f-l=P/E、f-h=PD/[E(D+U)]≥0，D=0 或 P=0 时可能相等）。

## 2026-10-24 23:55 大角度新 route 登记（p011-external-tilt-transient-v1，当前生效）

登记依据：用户已批准「外部姿态可造成受控短暂超临界」为新 route 的安全语义候选（`docs/prompt-pack-P011-large-angle-v1.md` A 段共同输入；`docs/design-P011-large-angle-external-tilt-v2.md`、`docs/design-P011-large-angle-sand-water-v1.md`）。本 route 与旧 route（`natural-relax-hysteresis-v1`）及阶段一 CLOSED 严格隔离，不追认旧 route 结果。

| 项 | 登记内容 |
|---|---|
| route 名 | `p011-external-tilt-transient-v1` |
| 状态 | `NOT_VALIDATED / NOT_IMPLEMENTED`；安全语义=候选（外部姿态可造成受控短暂超临界），未获正式放行 |
| 适用边界 | 128 列平沙 + 已形成沙堆 + 外部姿态输入（TEST 构造倾角与 EXPERIENCE 传感器同源物理角路径）；结论必须注明几何、初态、材料身份、预算与指标定义 |
| 算术基线 | 128 列平沙达到全宽线性 28° 沉积坡时，一阶矩至少约 92,917，5.1 秒额度仅 1,530 —— 条件性下界，不得忽略，也不得外推为所有局部观感或替代通道无解 |
| 角色交付门 | ①目标定义与证明（B designer）②客观用例预注册（C tester）③主观用例预注册（D effector）④工程化复审（E architect，默认关闭）⑤默认关闭实现（F developer）⑥只读审核（G reviewer）⑦阶段二路线判定（designer 双报告） |
| 依赖与解除 | s134 渲染回归已由 60d24bf（离线）+ bcada09（真机定点）解除，s136 真机六场景 34.4–35.9fps、bake 频率 0.08%；真机数据可信度前置不再阻塞本 route 的用例预注册 |
| 排期 | 按 2026-10-24 14:15 重排节：A-G 七卡整体降 P3，B 卡明确暂停执行，三件事窗口后恢复；本登记不另起排期整合 |
| 口径待裁决 | ①±45° 目标的测量对象（局部崩塌坡面 / 全宽沉积面 / raster OLS 三者各测什么）②300/s 下全通道可达性与条件性下界 ③超临界恢复及水沙同角输入的验证；未经用户裁决不得改 `docs/goals.md` 或冻结阈值 |

## 2026-10-24 13:30 用户今日三件事排期（manager 派发，四卡已建）

依据：用户 2026-10-24 口述三件事；[掉沙纵向路线设计](design-P011-falling-sand-vertical-routes-v1.md) §6 选型回执（路线 C=真实活动粒子层+128列高度场，为「手指落沙」「摇晃扬沙」共同架构，USER_SELECTED_TARGET_ROUTE_C / NOT_IMPLEMENTED / NOT_VALIDATED）。

1. **仿真线+交互线=路线 C 双场架构**（高度场仿真沉积沙、粒子场仿真掉落/扬起沙；触摸掉沙累积=GESTURE_EXTERNAL 粒子源+落床沉积）：designer 选型稿已交付；本轮派 architect 工程设计（6adc4394…0f7f，P5）+ tester 用例预注册（6adc4394…2edc，P4）。developer 实现卡待架构 manager 验收后建（隔离原型先行）；B tracer 为过渡外观保留，不作 C 的守恒/落床/扬沙验收证据。
2. **渲染线=P-013 主体**（XComponent+shader）：主动步=architect「P0 主体骨架预设计」（【architect双小活】卡 6adb68b3 第②项）；下游已建 developer P0 主体实现卡（6adc4394…2edd，P4，前置=预设计验收+P0a 真机确认双满足）与 effector P0a 真机确认卡（6adc4394…609d，P4，真机队列接 s134 二期后；T6 此前从未建卡，本日补建）。
3. **s134 渲染回归**：离线修复已验收（60d24bf，77/0）；真机二分闭环进行中（effector 6adb7038…c155，P5）——今日三件事的真机数据一律等该闭环或附数据污染声明。
4. **方案3 a 阶段已验收通过**（34c7087：advection 单测 8/0 + 全通道回归 planning106/s3 14/s4 10/gesture_inject 10/input 124/d3_wiring 6/tracer 9/render 77 零失败，manager 13:40 实测）；b 起沙链（B-1..B-3）接口预置不动，待 manager 批准；KF-1/KF-2 诚实登记生效（零悬沙初态 a 通量恒 0，效果验收用 shake 场景 OFF/ON 配对）。
5. 既有队列不变：大角度路线 A-G（A/B 为 P5）、E1 v2/E4（designer P4）、correction 归档（tester P4）、S4 代理升级+P0 预设计（architect P4）、C 补采+s134 二期（effector，真机串行）。

## 2026-10-24 14:15 用户裁定重排：今日三件事为全链唯一优先项（相关提级/不相关降级）

依据：用户 2026-10-24 14:09 指令「今天优先这三件事，相关的优先级提升，不相关的优先级下降」。三件事=①仿真线双场（高度场+粒子场）②渲染线 XComponent+shader 换栈 ③交互线触摸掉沙累积——三者共同落在路线 C（仿真+交互）与 P-013 主体（渲染）两条既有架构决策上。以下为 dida365 实际调整（API 枚举 P5=5/P4=3/P3=1），被调整卡的正文开头均有 manager 重排说明行，旧 P 字样作废。

1. **提升（今日三件事链）**：路线 C 工程设计 architect（6adc4394…0f7f）P5 维持；路线 C 用例预注册 tester（6adc4394…2edc）P4→P5；P0 骨架预设计从双小活卡拆出独立成卡（6adc4c91…dcd5）P5；P0 主体实现 developer（6adc4394…2edd）P4→P5；P0a 真机确认 effector（6adc4394…609d）P4→P5。
2. **保持**：s134 二期真机闭环（6adb7038…c155）P5 保持——真机数据可信度是今日三件事真机验收的前提，属相关前置，不降。
3. **降级（让位三件事）**：大角度路线 A-G 七卡整体降 P3（A/B 由 P5 降，B 卡明确暂停执行）；E1 v2、E4（designer）P4→P3；P-012 C 补采（effector）P4→P3（当前配对采完即停）；S4 代理升级（原双小活余项，6adb68b3 改造）P4→P3；s134 主任务（6adb3508）P5→P3（收口载体，主动推进在二期卡）。
4. **真机队列（effector 串行）**：C 补采让位 → s134 二期（P5）→ P0a 确认（P5）→ 三件事窗口后恢复补采。s134 二期是分钟级以上大活，排在 P0a 前保数据口径。
5. **manager 职责**：今日聚焦三件事链验收（路线 C 架构稿、P0 预设计稿）+ 大角度 A 卡降级后的门序登记按本节执行，不另起排期整合。

## 2026-10-24 14:50 路线 C 工程设计验收 + D-C1 阻塞缺陷登记（当前生效）

依据：manager 14:50 验收路线 C 工程设计稿（docs/architecture-P011-dual-field-particle-v1.md，561 行，七项工作全覆盖，实测核实含 D-C1 探针复跑）。用例预注册（tester，6adc4394…2edc）已于 14:25 验收通过。路线 C 从设计进入实现链。

1. **★ 阻塞级缺陷 D-C1（今日新发现，实测确认）**：`audit(checkDrift=true)`（WaterSandField.ets:6045-6052）守恒漂移分母用原始 initial*，与 getDrift（:3360-3373）已验收的 expected=initial+sourceInjected* 口径分叉。持续外源准入超 DRIFT_LIMIT=0.001×max(initial,EPS) 后 audit 返回 errorCode 5 → frozen=true（:1889-1896）→ 物理永久冻结只能 reset。Node 探针 test/probe_p011_audit_drift_gate.cjs 实证（准入 48 units=0.375% 后 step 即冻，getDrift 三项为 0 证明真守恒）。影响面：693a724 掉沙链真机长时注入试验全部被误冻结；路线 C 手势落沙的硬前置。**修复=分母公式归正（heavy/light: initial+sourceInjected*；water: initialWater+waterTransactionNet），DRIFT_LIMIT 不动**——实现对齐已验收设计，非放宽阈值。修复卡 6adc52e4…2a84（D0，developer，P5）。
2. **路线 C 实现链（已建卡，串行门序）**：D0 D-C1 修复（6adc52e4…2a84，P5）→ D1 M1 粒子池+双跑积分器+六事务（6adc52e4…2a85，P5，前置 D0）→ D2 M2 场集成九接入点+账本 v7+13 项单测=隔离原型出口（6adc52e4…2a86，P5，前置 D1）→ manager 原型出口验收 → D4/D5/D6（渲染接线/页面/真机定标，出口后建）。数值参数全部按架构稿 §9 候选值并标待定标；route v1 独占绑定（manager 裁决：同意，组合路由待原型出口后评估）。
3. **协议 revision 待裁决项（不阻塞实现）**：终点悬沙口径「susp」vs「susp+airborne」（架构稿 §5.6 提案）——designer 证据包卡 6adc52e4…1949（P3）出证据包+待用户裁决项；C run 期间双口径并报，不用旧口径单独放行，用户裁决前不改 goals.md/冻结指标/tester 预注册 estimand。
4. **验收基准**：tester 预注册稿 DUALFIELD-* 16 例（METHOD_FROZEN）estimand 为唯一验收基准；架构稿 §5.6 是遥测字段名落地权威；架构稿 §8 的 13 项 Node 清单是实现侧单元边界。GES-01/02 真机计时用例等 s134 二期闭环（6adb7038）后解冻。

## 2026-10-24 15:15 D-C1 修复验收 + s136 定点修复验收（当前生效）

依据：manager 15:15 验收两张 developer 卡（实测复跑，非采信回执）。

1. **D-C1 已修复关闭（卡 6adc52e4…2a84，commit fdc6a5a）**：audit(true) 改为复用 getDrift()（单一权威源，两处口径不可能再分叉）；探针复跑同场景（准入 48 units=0.375%）step ok=true 不再冻结；单测 13/0（新增三例：越界准入后持续步进/audit 与 metrics 同值/无源 run 对历史公式逐位等价）；开发者负对照（旧公式下新测失败并重现 errorCode=5）证明测试有效；DRIFT_LIMIT=0.001 未动——基线公式归正非放宽阈值。副作用解除：693a724 掉沙链真机长时注入的误冻结阻断已消除。
2. **s136 定点修复离线验收通过（卡 6adc5132，commit bcada09）**：治本=未 wrap 全局相位累加器 globalPhasePx 取代 mean(uv) wrap 均值（近空列钳幅 4px + 亚视觉死区 0.25px/帧）；保底=shiftAndBake 窗口平移门控 2px（几何变化/resetContinuity 强制重烘）；渲染单测 82/0、新增 5 断言逐例 PASS；全通道 372/0；雾带质量结构性保持（烘焙内容函数零改动）；离线量化 settle 尾段 300 帧噪声场重烘=0（真机 63%@133ms 主因的 110ms 全量重烘已消）、缓冲重烘=44（真实层高蠕变 20ms 正确成本）。★ 真机 60°C 下是否消除 133ms 尚未验证。
3. **D1 解锁**（卡 6adc52e4…2a85）：D0 前置已满足，M1 粒子池+双跑积分器+六事务立即可开工。
4. **新建 effector 真机验证卡 6adc5a56…3631（P5，真机队列首位）**：seg3 三场景复测（settle ≥30fps 或剩余瓶颈归因）+ 同温对照 s106 + E3 锚点 std/J95/J90 回归 + 视觉连续性目视确认 + 数据落盘。s134 主任务（6adb3508）降为收口载体（blocked，等此卡）。
5. **终点悬沙口径方案由 designer 出定稿**（卡 6adc52e4…1949，P3）：用户 15:10 指示由 designer 明确方案而非只出证据包——产出=口径定义+判定规则+接口迁移+风险反例+推荐方案，manager 转用户最终裁决；C run 期间双口径并报。

## 2026-10-24 15:25 P0a hello-surface 真机确认验收通过（当前生效）

依据：manager 15:25 验收 effector 真机确认卡（6adc4394…609d，manager 逐项查原始数据非采信回执）。

1. **P0a 真机确认通过并关闭**：EGL GLES3 上下文成功（vendor=HUAWEI）、表面 1280×1559 仅创建一次（持久表面路径成立）、28 次心跳 swap 全 ok、**稳态 59.9fps**（manager 按 hilog 时间戳独立复算：13 段间隔恒为 5009ms=300/5.009；冷启动前两段 63/83fps 为启动期）、前后台 10× 切换 RSS 平稳（VmRSS 108.9→117MB，**VmHWM 自 cycle1 起恒定 122556kB**，无泄漏）、清屏色 (46,51,55) 经原图像素复核命中、无应用 ERROR（唯一 napi_unwrap 为注册瞬间）。数据产物 5 件 + 工具 3 件在盘；构建产物 entry-default-s137-signed.hap。
2. **与回归链独立性经结构核验**：hello_surface.cpp（288 行）include 仅 napi/xcomponent/EGL/GLES3/hilog，对 P012/WaterSand/physics **零引用**，故 P0a 结论不受 s134/s136 Canvas2D 回归影响；s137 构建早于 s136 修复提交（14:39 vs 15:01）不影响结论。临时 harness 门（p013page）经扫 entry/src 与 git grep HEAD 确认**已清干净**，主干无残留。
3. **P0 主体实现双前置进度**：②P0a 真机确认已满足；①P0 骨架预设计（6adc4c91…dcd5）architect 未交付 → P0 主体卡（6adc4394…2edd）暂不开工，等待期只做不依赖前置的准备工作。P0a 交接项已并入该卡：旋转 viewport 真机手动复核（设备无 wm/settings/display/hc，强制旋转不可脚本化）、不得引入调试门、s137 HAP force-stop 收尾、遥测工具复用。
4. **P-012 线真机口径仍需 s136 真机验证**（6adc5a56…3631，P5，effector 队列首位）：P0a 是独立 native 路径，不替代该卡。
5. **收据勘误**：p013-p0a-receipt-v1.md 表格中 t60 的 VmHWM 写作 122556，原始 RSS 曲线为 108880（t60→cycle1 间 HWM 抬升一次后恒定）——不影响“无泄漏”结论（收据正文表述正确），已回写纠正。

## 2026-10-24 15:35 路线 C 架构稿 rev1 补契约（当前生效）

依据：developer 在 D1 开工时按纪律停手反馈三项阻断，manager 15:35 逐条核实**全部属实**（详见 D1 卡评论 6adc5eb8…c836）。

1. **缺陷定性（架构稿 v1 内部矛盾 + 与冻结预注册冲突）**：§2.3 第 4 步与 §4.2 apply 模式描述两处写「T3 deposit：…→mh/ml+=q→**粒子消失**（释放槽）」——若 granted<requested 则未授予部分随粒子消失=质量丢失；与同稿 §2.3 开篇「等待与降级都不产生质量丢失」、§5.6 事件契约「余量留接触粒子(state=2)或一笔 TO_SUSP，不得删除」、预注册 CONS-03（MUST-PASS）「落床部分授予余量滞留不丢不双计」三处冲突。
2. **manager 直接裁定（立即生效）**：deposit 只扣 granted；余量留原粒、置 state=2；**槽释放条件=质量归零**（不得以 granted>0 为释放条件）。路由门归属：M1 只出契约（接口声明+纯谓词），接 WaterSandField 属 D2（原 D1 卡「实现路由门」措辞是 manager 之误，已更正）。
3. **manager 复核新发现的第四项缺口**：稿中 collect 跑用 sink.clip*、apply 跑用 clipSubstepMotion，两条机制是否同型/共享同一分母口径未定义——这是「两跑只差预算裁剪」结论的前提，双跑对账不可实现。已并入补契约项。
4. **新建 architect 补契约卡 6adc5ed0…c94e（P5，优先于 P0 预设计处理）**：六项=①修正两处落床质量丢失表述并同步 §4.2/§5.6/§8 引用点 ②补提交接口契约（guard 准入调用/床层写入归属与原子性边界/T5 susp 写入/collect 隔离接口/sink 完整签名语义）③collect-apply 分母同源性 ④路由门归属澄清 ⑤D1/D2/D4-D6 交付边界清单 ⑥全稿一致性自检+逐条修订索引（位置/原文/新文/依据）。产出=架构稿 rev1。
5. **D1 转为两段执行（不整体停摆）**：A 段立即开工（SoA 池+空槽栈+每列在途质量缓存+事务流水+路由门契约+双跑骨架+桩 sink+CONS-03 余量断言）；B 段等 rev1 验收后开工（真实 guard/床层扣写/susp 提交/collect 隔离接法）。D1 状态=部分阻断（blocked 标签因 dida365 标签接口故障暂无法写入，以卡内状态说明为准）。

## 2026-10-24 15:40 P0 骨架预设计验收通过（当前生效）

依据：manager 15:40 验收 architect 预设计卡（6adc4c91…dcd5），逐项查代码与上游文档，非采信回执。

1. **P0 预设计通过并关闭**（docs/architecture-P013-p0-skeleton-predesign-v1.md，35945B，纯设计零代码已核：cpp/render/pages 无未提交改动）：四件套接口契约精确到可实施（napi_bridge 完整帧契约+错误码数值表 / egl_context 生命周期状态机+世代表+线程协议 / shader_pass 坐标映射+mask 分类+GLSL 草案 / tdata_uploader 布局+GL 状态+固定内存下界）、三版本号与 T_data 布局、双轨四模式共存规则、逐模块遥测字段与采样口径全部登记。
2. **与上游文档逐字一致（非自创）**：T_dataLayoutVersion=1 / slopeKernelVersion=E2_APPROX_P0 / zOriginVersion=1 与四标量 sandTop/waterTop/bh/mixStart 同立项书 v1.1 §2.4–2.5 定稿一致；上传<0.1ms、60Hz×30min 无增长（补 108000=60×1800 口径）同立项书 §2.4；profiles 来源经代码核对为 buildDisplayProfiles 四遍三点平均+边缘复制（:2201-2222，mixStart=bh+bl）。
3. **native 事实核对全中**：回调名/尺寸 API/窗口转型/OH_NATIVE_XCOMPONENT_OBJ/CMake 目标 p013hello/五库实名（libace_napi_arkui 确不存在，代码注释已载明）。
4. **P0 主体实现即刻放行**（卡 6adc4394…2edd）：双前置齐备（预设计 15:40 + P0a 真机 15:25），开工依据=预设计稿 §1–§10 六步切片；与 D1 A 段文件面不重叠可并行。
5. **manager 登记项（实施时须处理）**：N1 §4.1 签名用 NativeWindow* 而 SDK 实际为 void* window；N2 尺寸权威源须定为 eglQuerySurface（XComponent size 仅提示，否则误判 STALE_SURFACE）；N3 立项书 §2.5 shader 草案与本稿定稿三处差异（移除 u_sigma 运算槽 / u_ellLight→u_ellLightPx / 透明输出+DIAG_GRAY_V1）须由 architect 走立项书 revision；N4 两份架构稿未入库，按 PIPELINE 由 reviewer 提交环节入库。
6. **P0a 遗留随 P0 主体卡处理**：旋转 viewport 真机手动复核、s137 HAP force-stop 收尾、禁引入调试门、遥测工具复用（p013_p0a_telemetry.ps1 / p013_telemetry_analysis.py）。

## 2026-10-24 15:55 终点未沉积沙比例方案定稿验收通过（待用户裁决）

依据：manager 15:55 验收 designer 方案定稿（6adc52e4…1949，docs/design-P011-terminal-nondeposited-fraction-v1.md §7 v1.1 + 复盘 retro-P011-terminal-nondeposited-v1.md），逐条查源码复核。

1. **designer 的源码引用全部属实（manager 复核）**：①getMetrics() 的 suspensionFraction = suspended/totalSand（totalSand=当前实际总量）；②getMetrics() 的 depositedHeavy/LightFraction 除 initial+sourceInjected（:3397）而 updateMetrics() 的 t90Heavy/t90Light 仍除 initial（:6000/:6003）——口径分叉属实；③注入分支 `if (t90Heavy < 0 && sourceEpoch > 1) t90Heavy = -1`（:1585-1586）确为空操作、已触发 t90 不被清、light 无处理；④pages/SandCanvas.ets:2040 `setShakeInput(min(1,|effectiveTilt|/90))` 确认存在——故「SLIP 各相位一律要求 shake=0」是过度表述，定稿已纠正；⑤nextP011TimeMs() 确为 Date.now() 递增（非 simulationTime，不能单独证明 1s 窗口覆盖）。
2. **推荐方案（designer 唯一推荐=候选 C）**：终点分子=列悬沙+全部活粒子质量（含 state=2 接触待准入）；各用例历史分母不变（R_SLIP=首子步 stateBefore 的 bh+bl；R_WS=累计期望 E）；阈值 5% 不变；新增「固定终点前最后 1s 每个已提交子步均≤5%」质量窗（单列披露 endpointFraction 与 windowPeakFraction）。
3. **★待用户裁决项（manager 不代裁）**：①终点分子扩为 susp+粒子（含 state=2）②分母保持各用例历史分母不动 ③阈值 5% 不变 ④新增末 1s 质量持续达标窗 ⑤TC-WS-08 的 peakSusp≥0.5 迁移另案裁决（不并入本次）。批准后序列：tester 出 endpoint protocol revision → architect 字段 revision → developer 实现 → checker 同 run 双算 → 同 build 同输入双报告 → designer 验证 → manager 确认适用范围。批准前：C run 双口径并报，任一单口径不单独放行。
4. **新发现代码缺陷已转 developer 卡**（6adc6412…beac，P3）：t90 epoch 重启空操作 + t90Light 未处理 + t90 分母与 depositedFraction 口径分叉；其中「t90 分母是否含注入」属验收口径问题，并入上述裁决，不由 developer 单方决定（卡内只做接口准备与开关位）。
5. **量级诚实登记**：架构稿 §3.3 的 450 units 与 Nmax×nominal m（25.6/51.2/128）均为候选非测量值；T6 允许同相包合并增质量，故 Nmax×m 不是合并后严格上界——严格界需实际池质量或额外包质量上限契约，已由 designer 向架构稿登记条件差异。
6. **D1 A 段范围收窄（manager 修正）**：A 段原写法混入了两项依赖架构稿 rev1 的内容——双跑「预算分母取数与 scale 应用」及「两跑只差预算裁剪」对账断言、「预算身份/guard 身份」两项单测的最终期望值。已把这两项移入 B 段（等 rev1），A 段现严格限于：池/空槽栈/每列在途质量缓存、事务流水、路由门契约、双跑骨架（不含预算分母机制）、六事务纯逻辑（guard 用桩）、CONS-03 余量断言、桩实现。

## 2026-10-24 15:56 终点未沉积沙比例口径：用户批准（当前生效）

裁决记录：**docs/decision-P011-terminal-nondeposited-fraction-v1.md**（APPROVED / 协议未生效 / 未实现 / 未验证）。批准内容（designer 推荐=候选 C）：①终点分子=U+P（列悬沙+全部活粒子质量，含 state=2）②分母=各用例历史分母不变（SLIP: R_SLIP；WS: R_WS=累计期望 E）③阈值 5% 不变 ④新增固定终点前末 1.0s 每已提交子步均 ≤5%×R+1e-9 的质量窗 ⑤TC-WS-08 `peakSusp≥0.5` 另案裁决。配套规定：三分项定名（suspFraction/airborneFraction/depositedFraction 共用 E）；旧 `WS_METRIC.suspensionFraction` 保留旧公式旧名、不得互为别名；正式判定从原始 v7 账本重算 M/R（不得用显示分项之和 M/E）；SLIP 各相位不额外要求 shake=0（既有耦合不改）；状态分类 NOT_IMPLEMENTED/INVALID_LEDGER/INVALID_BASELINE/INCOMPLETE/INVALID_DENOMINATOR 均不得记 PASS；state=2 永久滞留输出 PERSISTENT_PARTICLE_RESIDUE 交 manager 审查。

**生效条件**：批准的是口径与末窗要求；协议正式生效前 C run 双口径并报、任一单口径不单独放行；旧报告保留 legacy-susp-only 身份不追认；回算值标 RETROSPECTIVE_DIAGNOSTIC。

**执行链（已建卡）**：
1. tester endpoint protocol revision 6adc64e7…bc1（P4）——冻结分子/分母/5%/末 1s 窗/固定终点/采样身份与时钟/状态分类/双算规则 + test-cases.md 版本化条目；保留 DUALFIELD estimand
2. architect endpoint 字段 revision 6adc64ff…b44（P4）——post-commit 采样身份/endpointSampleElapsedMs/integratedPhysicsSeconds/particles.byState 诊断/三分项定名/报告字段清单；与 tester revision 字段名须一致
3. developer 终点指标实现 6adc6513…db0（P4，前置 1+2）——三分项输出 + endpoint 采样 + checker 双算 + 状态码 + 单测
4. tester 出双报告 → designer 验证 → manager 确认适用范围
5. 另案：designer peakSusp 迁移方案 6adc64ff…b43（P3）待用户另案裁决
6. 沉降 t90 口径（累计期望分母 + 跨 epoch 无效，依据 CONS-04）随本次裁决明确，developer 卡 6adc6412…beac 已由「仅接口准备待批准」更新为「可实现」

## 2026-10-24 16:12 D1-A 验收 + 架构稿 rev1 验收（当前生效）

1. **D1 A 段验收通过**（卡 6adc52e4…2a85，commit 0e92729，3 文件 +573 行）：单测实测 12/0（manager 逐例复跑）；CONS-03 原语 `retainAfterDeposit`（:169-190）逐行核实——只减 granted、pMass=remainder、仅全额才清槽、否则原 id/phase/source 保且 state=2、子 EPS 尾数不删；结构为 SoA+O(1) 空槽栈+每列分相缓存+完整独立快照+纯 route 谓词；红线核实 render/P012 引用=0、未触碰 liftByShake/B tracer/WaterSandField；全通道 287/0、hvigorw BUILD SUCCESSFUL。**A 段自验不冒充完整 D1 或原型出口**（开发报告自列 B 段未交付项）。任务不关闭（B 段待做）。
2. **架构稿 rev1 验收通过并关卡**（卡 6adc5ed0…c94e）：稿 561→827 行，新增 §13（提交/分母/路由契约）与 §14（九条修订索引+八项一致性自检）。六项补契约逐条落地：落床余量唯一处理（§13.1）、完整 sink/guard/写入归属（§13.2）、collect-apply 分母同源与双跑可核验（§13.3）、路由门归属（§13.4+§13.5）、D1-D2-D4-D6 交付边界表（§13.5）、一致性自检与索引（§14）。关键裁定与 manager 15:35 一致：分母 T3/T4 各一次、T2 仅 rise、T1/T5/T6 为 0（R5）；Nmax×m_i 只作等包示例、免费合并限同列同相同来源、跨列须独立 T4 授权（R8，堵预算旁路）。
3. **★接口面变更（manager 复核 rev1 时发现）**：rev1 §13.3 明确旧 `integrate(...,budgets,collect:boolean)` 不是 rev1 接口，改 `collectParticlePlan(identity,frozenField,frozenPool,collector)` / `applyParticlePlan(plan,sink)`（能力隔离+不可变 plan+双 collect 逐字段对照→不一致即 PARTICLE_PLAN_MISMATCH 整子步回滚）。A 段的池/快照/journal/CONS-03 原语可复用，对外集成面须在 B 段按 rev1 收口——已写入 D1 B 段工作项 1。
4. **D1 B 段解锁立即开工**（双前置已满足）；**D2 卡正文已按 rev1 收口**（新增 §13.2 真实 sink 侧、§13.3 planner 影子镜像=候选 preview/verify/live 同槽镜像、§13.4 唯一物理 route 门；范围以 §13.5 为准，原 §11 分卡表作历史预注册）。
5. **遗留登记**：架构稿未入库（工作区未提交），按 PIPELINE 由 reviewer 提交环节入库；D2 的 planner 影子镜像为 rev1 新增义务，已并入 D2 卡。

## 2026-10-24 16:25 真机使用登记（当前生效，用户 16:20 指示）

规则：**真机是共享串行资源**——每张卡的 prompt 必须写明「是否可以使用真机 + 队列位置」；需要真机的卡在标题/标签上带「真机」标识。

| task | 角色 | 真机 | 队列位置 | 声明位置 |
|---|---|---|---|---|
| s136 真机验证 6adc5a56…3631 | effector | **需要（独占）** | 队列第 1 位 | 正文【真机使用】段 + 标题［真机·渲染线·effector］ |
| P0a 真机确认 6adc4394…609d | effector | 需要（已用完） | 队列第 2 位（已完成） | 正文 |
| P0 主体实现 6adc4394…2edd | developer | **不使用** | — | 正文【真机使用】段（明确不得自行安装 HAP 占机） |
| D1 B 段 6adc52e4…2a85 | developer | **不使用** | — | 正文【真机使用】段（Node 通道） |
| D2 M2 场集成 6adc52e4…2a86 | developer | **不使用** | — | 正文【真机使用】段（真机定标属 D6） |
| 终点指标实现 6adc6513…bdb0 | developer | **不使用** | — | 正文【真机使用】段 |
| 方案3 b 起沙链 6adc6272…f9dd | developer | **不使用** | — | 正文【真机使用】段 |
| t90 epoch 修复 6adc6412…beac | developer | **不使用** | — | 正文【真机使用】段 |
| 终点指标 protocol 6adc64e7…bc1 | tester | **不使用** | — | 正文【真机使用】段 |
| 终点指标字段 revision 6adc64ff…b44 | architect | **不使用** | — | 正文【真机使用】段 |
| peakSusp 另案 6adc64ff…b43 | designer | **不使用** | — | 正文【真机使用】段 |
| E1 C 层理锚点 v2 6adb3052…c0 | designer | **不使用** | — | 正文【真机使用】段 |
| E4 沙丘占比量测 6adb6898…96 | designer | **不使用** | — | 正文【真机使用】段 |
| S4 代理升级 6adb68b3…f30 | architect | **不使用** | — | 正文【真机使用】段 |
| 掉沙 V1-V4 重录 / E2 出数 / 参数面板 / S3-S4 真机 | 全链 | 需要 | 队列第 3 位起（等 s136 真机验证） | 待建卡时声明 |
| D6 真机定标与双报告 | developer/effector | 需要 | 原型出口后 | 待建卡时声明 |

**声明位置规则（16:35 修正）**：真机声明只写在**卡正文**的【真机使用】段，不得用卡内评论承载——评论是智能体反馈问题的通道，manager 不占用。

## 2026-10-24 16:35 卡内通道纪律（当前生效，用户指示）

- **卡正文 = 唯一指令来源**：角色/任务/输入/工作/产出/红线/完成条件/解锁/前置 + 【真机使用】段，全部在正文；增量一律用 update 重写正文。
- **评论 = 仅智能体反馈问题的通道，manager 不写评论**（建 task 时尤其禁止）。已累计清除 19 条 manager 留言（8 条真机声明 + D1 裁定 + D1-A 验收 + 9 条已关卡验收回执）；D1 卡上 developer's 反馈评论保留。
- **manager 结论去处**：验收结论/台账/实测数据 → 本文件 + memory 日志；用户裁决 → 独立 `docs/decision-*.md`（权威载体）。

真机队列（串行，effector 执行）：**掉沙 V1-V4 返修复测（6adcd2dd…cb99，已建卡，队列第 1 位）→ P6 endpoint 协议合规设备 run（待 D-2/D-6 实施卡 6adcd2a1…e9d8 交付后建卡，属协议生效硬前置）→ E2 出数/参数面板/S3-S4 → D6**。已完成：s136 真机验证（6adc5a56）、P0a 确认（6adc4394）、掉沙 V1-V4 重录首轮（6adc861c，硬指标 PASS、功能验收 BLOCKED）。其它角色的卡若需真机，一律反馈 manager 建卡排队列，不得自行安装 HAP 或占用设备。

## 2026-10-24 23:30 四卡验收（②③④⑤）+ D1-R3 智能体反馈处置（manager 口径：只判「是否做完」，质量由消费输出的智能体验证）

**② 终点指标缺陷修复（developer，6adcc830…33fc）——通过并关闭。** 交付 commit `8e498c5`（6 文件）。manager 实测：D-1 枚举约束落地（`EVIDENCE_MODES=('CANDIDATE','RETROSPECTIVE_DIAGNOSTIC')`，非法值记 `INVALID_EVIDENCE_MODE` 且 report 状态降为 INCOMPLETE，**不静默回落 CANDIDATE**；报告新增 `evidenceClassification` 区分候选/回算）；D-3 `protocolSourceHash` 产出（SHA-256 取协议稿字节流，附 `protocolSourceHashAlgorithm`/`protocolSourcePath`，行内回填并从 manifest 读取名单中移除）；D-4 `endpointSampleStage` 经 `row.setdefault` 字段表回填（不再恒进 reportMissingFields）；D-5 `externalAdmissionCount` 在物理侧产出（`P011NaturalObservation.ets` context 写入，与 `sourceEpoch` 一致性校验，不一致记 `EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH` 硬失败，缺失时不伪造、如实进 missingFields）。自测：Python **20/20**、Node **9/9**、协议十条反例 **10/10 MATCH**（manager 亲自复跑）。P5（缺陷修复）前置满足。

**⑤ 终点指标协议勘误（tester，6adcc83c…a1c1）——通过并关闭。** 交付 commit `f2d0aee`（5 文件 +347/−6）+ `docs/endpoint-errata-P011-terminal-nondeposited-v1.md`（179 行，叠加式勘误，**已发布协议文件零改动**）。三项落地：E-1 协议 §10 示例表第 1 行按裁定 A2/A3 把两个 legacy 量分列（A2 `322/7872=4.0904%`、A3 `322/8000=4.025%`，差额恒等式按分母配对陈述）；E-2 三处 `4.0894%`→`4.0904%`（manager 复算 322/7872=4.090447154471544 确认原值笔误，勘误后值正确；残留项均为对照表的「勘误前值」列与 commit 历史，属有意保留的痕迹）；E-3 条款计数口径统一（查明「28/30」在节点表无对应口径，实为 9+11+6+8+11=**45 条**判定口径类条款、43 条一致、2 条指向 D-1；双报告 P4 行已改并注明来源，P4 仍「已满足」）。manager 复跑：十条反例 10/10、Python 20/20、Node 9/9；协议正文 `git diff` 为空。

**④ endpoint 诊断字段与 particleEvents 职责登记（architect，6adcc824…4303）——设计稿已落盘，通过并关闭，D-2/D-6 转 developer 实施。** `docs/architecture-P011-endpoint-diagnosis-particle-events-v1.md`（工作区未入库）：D-2 裁定 checker 唯一使用 `particleResidueDiagnosticStatus`/`Codes`/`Meaning` 三具名字段、`endpointDiagnosis` 不再作为第二消费名（协议 v1 保持原样，新 revision 由 tester 登记）；D-6 裁定七类事件级校验归 endpoint checker（`eventLevelValidationOwnership=ENDPOINT_CHECKER`），事务层只负责原子生成/发布并声明「本职责不在本层」；七类事件表七行齐备，两层「不在本层」声明齐备。D-2/D-6 的实施由 developer 承接。

**② 掉沙接线修复（developer，6adca854…cdcc）——六项已落地但未入库，任务保持开启。** manager 逐项核查工作区改动：①`WS_EVENT gesture_sand` 逐笔日志已接（`setGestureSandEventSink` + 字段齐全 `sourceId/sequence/inputTime/requested/admittedH/admittedL/rejectedReason`，物理侧 `emitGestureSandReceipt` 覆盖准入与 `NO_WATER`/`CAPACITY_BLOCKED` 两条拒绝路径）；②`sourceInjectedHeavy/Light` 已进 metrics；③回退路径已改为 fingerList 为空时直接拒绝并落 `WS_EVENT pan_update`（`coordinateSource:'missing_finger'`），不再用累加后的 `lastPanLocalX`；④`resetPanGestureBoundary` 已在 `onActionStart` 复位 `lastPanRouteTime`/`lastPanLocalX/Y`；⑤Pan 可观测点已加（`panUpdateCount`/`panLastLocalX/Y`/`panLastEventAccepted`）；⑥shake Want 直通已补（`liquidStartShake` 同时下发 `waterSandShakeInput` → `waterSandField.setShakeInput`，并落 `WS_EVENT shake` source:'WANT'），`liftByShake` 落 `WS_EVENT shake_lift`（起沙量与列数）。新增 Node 单测覆盖逐笔落行（含拒绝）、外源账本一致、shake lift 遥测；`gesture_inject` 通道 manager 复跑 **18/0**。**未完成项：产品代码与测试代码未入 git**（`SandCanvas.ets`/`WaterSandField.ets` 仍为工作区 M 状态）——完成条件要求 commit，故本卡不关闭，标题已标注「六项已落地待入库」。

**⑧ 方案3 b 起沙链（developer，6adc6272…f9dd）——零交付，任务保持开启。** manager 实测：`git log` 仅有 a 阶段 `34c7087`（对流通道实现），b 阶段 B-1 起沙源登记在 `entry/src/main/ets/` 中**零命中**（`起沙源`/`liftSource`/`shakeLiftSource` 全无）；`advectionSlot` 仍是 a 阶段原样，未接起沙量；无本卡相关提交。**判定为尚未开工**（非交付不合格）。

**① D1-R3 智能体反馈处置（developer，6adcc2c1…1cf05）：反馈属实，任务改判为「等设计」而非「未开工」。** developer 报告：检查契约与代码后发现现有 M1 物理逻辑、预算/退款、回滚与事件日志都还依赖对象式 API，仅加一层 HOT 外壳会漏掉计划生成与事务语义，不能算生产实现，故撤下了试写的接口骨架。manager 逐条核实其主张**全部属实**：①`grep` 确认 19 个热接口与 `nextLeaseToken`/`expectedStepEpoch` 在 `entry/src/main/ets/physics/` 下**零命中**，无残留骨架（撤下干净，未留不完整代码或伪通过结果）；②探针复跑仍 **exit=2 / NOT_ZERO_ALLOCATION / 60 处**（Field 31 / Prototype 29；kind 分布 object-literal 22 / allocating-helper 20 / array-literal 12 / constructor 4 / closure 2），与 22:40 逐字一致；③现有 `collectParticlePlan`（`P011ParticleField.ets:757`）确实用 `const order: number[]=[]` + `order.push(i)` + `order.sort((a,b)=>...)` 排序回调 + `deposits/crossings/motions` 三个对象数组 + `requestKey: JSON.stringify([...])`，另有 `JSON.stringify(plan.identity/requests/motions)` 哈希比较（:383/:401-403/:750）——正是契约 §5 明令禁止的形态；④契约 §10 映射表把「collectParticlePlan 返回对象计划 → collectParticlePlanInto」列为**生产替代**关系，即必须替换而非加外壳。**裁定：developer 的判断正确，不是拖延；本卡不是「零交付」而是「发现契约未给出可实施的迁移路径」。** 处置：新建 architect 迁移设计卡 **6adccf16…b81f**（P5）——交付对象式→定容数值路径的分步设计、探针 60 处逐项归类（契约 §8 要求「已迁冷/已替代/仍残留」+ 证据路径）、3–5 个可实施切片及每片判据；D1 R3 卡正文已补入该设计来源并改标题为「等迁移设计卡 6adccf16…b81f」，其交付后按切片实施。

**新卡登记**：architect 迁移设计 `6adccf16…b81f`（P5）；tester 双报告补充证据 `6adccef4…26e1`（P4）——把 22:50 已验收双报告因 `8e498c5` 并行落地而变动的元数据计数更新为现值（manager 实测 missingFields **6→7 类**、reportMissingFields **37→38 项**、`protocolSourceHash` 已有值、`endpointSampleStage` 键存在但为 null、`externalAdmissionCount` 仍缺），并刷新 D-1~D-7 状态与 P1–P9 前置清单；旧双报告结论不改写（legacy 逐位一致 7/7、NOT_IMPLEMENTED/INVALID_LEDGER、CANDIDATE_ONLY 三项主结论不变），P9 仍只能由 manager 宣布。

## 2026-10-24 22:50 终点指标双报告验收（tester，6adcbef8…9cde）——**通过并关闭**（协议仍未生效，D-1~D-7 缺陷清单转 developer/architect/tester）

交付（commit `63fccf7`，4 文件 +3576 行，全新文件，**未触碰实现代码与判据**）：`docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md`（141 行）+ `docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md`（165 行）+ `test/p011_endpoint_protocol_v1_execution.py`（404 行，tester 侧只读）+ `test/reports/endpoint_protocol_v1_execution.json`（2866 行）。

manager 独立复核（全部自己跑）：
- **协议 §10 十条自检反例复跑 10/10 全部 MATCH**（`python test/p011_endpoint_protocol_v1_execution.py` → `counterexamples=10 verdictMatches=10`，逐条 R1–R10 实测输出与协议预期一致，无一条需改判据或放宽断言）。
- **交付自测复跑通过**：Python `test.test_p011_endpoint_check` → **17/17 OK**；Node `run_p011_endpoint_units.cjs` → **8/8 PASS**。
- **旧口径逐位交叉核对 7/7 bitwiseAgree=true**（真实归档 run `test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`，我实测 **2512 行**、sha256 前缀 **f2815c98f9a886f5**，与报告登记一致；七个相位的 `recomputedLegacyEndpointFraction` 与 `expectedFractionFromHistoricalFormula` 逐位相等，容差 1e-15；旧 PASS/FAIL 原样保留、标 `legacy-susp-only`）。
- **一致性核对表逐节点数核对**：§1.1 分子细则与守恒先决 **9 条**、§1.2 固定终点与末 1s 窗 **11 条**、§1.3 状态分类与判定顺序 **6 条**、§1.4 双算规则 **8 条**、§1.5 checker 适配 N1–N11 **11 条** = **45 条**，与报告 §4 的「45 条 / 43 条完全一致 / 2 条指向同一枚举级缺陷 D-1」吻合；§1.6 字段级另列 27 行。
- **实现行号抽查全部命中**：`:139` evidenceMode 透传 manifest（无枚举约束 → D-1 成立）、`:335` reference=SLIP 用 R_SLIP 否则 E、`:336` legacy_reference=WS 用 D+U、`:346/:347` 两个 legacy 量各自独立、`:397–402` 状态码覆盖次序（hard 覆盖一切 → 硬闸门优先）、`:419` externalAdmissionCount 只在 reportMissingFields 元组、`:208` endpointSampleStage 只校验不回填（→ D-4 成立）、`:214–215` particleEvents 仅校验 isinstance list（→ D-6 成立）、`:429/:443/:472` endpointProtocolEffective=False / status=CANDIDATE_ONLY / CLI 退出码无放行路径。`grep` 全仓 `protocolSourceHash` **0 命中**（→ D-3 成立）、`externalAdmissionCount` 仅出现在 checker 的缺失字段元组、无 ArkTS 产出点（→ D-5 成立）。协议 §6.1 写 `endpointDiagnosis` 为 object、实现拆三字段（→ D-2 成立）。
- **D-7 数值我复算**：R1 的 A2 口径 U/(D+U)=322/7872=**4.0904%**，A3 口径 U/max(A,EPS)=322/8000=**4.025%** —— 协议 §10 表第 1 行把 legacy 列写作 `U/E=4.025%`，实为 A3 值；A2 值 4.0904% 在报告与 JSON 中均已如实给出。**判定结论不变**（legacy PASS / 新 FAIL）。

**manager 实测发现的两处报告内部不一致（不影响任何判定结论，已转 tester 勘误卡处理）**：①一致性报告 §3 D-7 与执行脚本/JSON 的 note 把 A2 值写成 **4.0894%**，正确值应为 **4.0904%**（我按 322/7872 复算）——同一份报告 §1.6/§3 另处已写 4.0904%，属文本笔误；②双报告 §4.2 的 P4 行写「**28/30** 实质条款一致」，与一致性报告 §4 的「45 条 / 43 条一致」口径不一致，来源未注明。

**红线核验**：commit `63fccf7` 只含 4 个新文件，**未改**实现代码、`p011_endpoint_check.py`、`test-cases.md`、协议正文与冻结指标；未追认旧 route；未宣布协议生效。

**处置**：卡关闭。**协议仍未生效**——报告与实现均保持 `endpointProtocolEffective=False` / `formalReleaseAuthorized=False` / `status=CANDIDATE_ONLY`，须待下列前置全部满足后由 manager 宣布：P1 已满足、**P2 现已满足**（architect 字段 revision rev2 已 ALIGNED 并与 tester 稿同名，卡 6adc64ff…ccb44 本轮关闭）、P3 已满足（实现落地并自测通过）、P4 已满足（一致性核对无未决口径差异）、**P5 未满足**（D-1~D-6 待修复或 manager 裁定豁免）、**P6 未满足**（缺一次符合协议采集口径的设备 run）、**P7 未满足**（该设备 run 的路线专属双报告）、**P8 未满足**（designer 验证）、**P9 未满足**（manager 确认适用范围并登记 effective=true）。**P6 需真机且真机为串行共享资源**，由 manager 排队列后交 effector。

## 2026-10-24 22:50 E4 沙丘占比量测方案验收（designer，6adb6898…96）——**通过并关闭**（纯量测设计，未出数、未冻结任何阈值）

交付：`docs/algorithm-P012-E4-dune-share-v1.md`（120 行）+ `docs/retro-P012-E4-v1.md`（复盘）。

manager 逐条核实：
- **五项全覆盖**：①量测方案（主口径 `A_geom` = sandTop 轮廓以下几何面积 / 备选轮廓 IoU+边界距离 / 辅助 W50·H95）；②差异归因框架（初态沙量 × 雾带双因素交叉格 + 三类结论分类表）；③调整路径只登记不实施；④与 E1/E2/E3/S4/P-011 的依赖与推荐时序；⑤与需求锚定的衔接（占比与形态分立定义）。
- **核心设计判断正确**：把「沙量不足」与「雾遮挡」拆成 `A_geom`（模型几何，不随雾 alpha/顶光/颗粒变）与 `A_visible`（图像分割，仅作遮挡诊断），**不用可见量冒充几何量**；目标帧边界不可唯一判定时输出 `REFERENCE_BOUNDARY_AMBIGUOUS` + 不确定像素上下界，不猜单值；每档初态沙量**独立仿真**、雾两档**严格同物理快照**，明确不声称跨档同快照。
- **明确不采用的路径写清理由**：拒绝整图 SSIM（背景/雾稀释沙形差异）、拒绝单阈值自动分割（E2/E3 改颜色会改「面积」）、拒绝单用水平宽度（同宽可对应不同高度/面积）。
- **与冻结锚点边界守住**：E3 的 `stdRatio [0.90,1.10]`、雾带窗 rows `950–1100`、A 档雾 alpha 上限 `0.78` 均作既有约束引用（我已在 `e3-grid-display-v1-freeze-revision.md` / `architecture-P012.md` / `algorithm-P012-appearance-controls-candidates-v1.md` 核到出处），且明写「不得重解释冻结窗、不得用诊断关闭雾替代产品配置、任何候选须独立走 revision 并由 manager 批准」。
- **诚实状态标注**：`MEASUREMENT_DESIGN_READY / NOT_MEASURED / NO_THRESHOLD_FROZEN`，首轮只报原值与重复性，无 manager 冻结的数值线时写 `MEASURED_NOT_CALIBRATED`。
- **红线核验**：纯设计零代码；不改物理初态代码与渲染；不改冻结锚点；不预填精确比例；未宣称 VALIDATED。

**处置**：卡关闭。E4 的**测量执行**（同构图量测、目标帧 ROI 复核、初态/雾消融实验）尚未派工——按其 §6 推荐时序，须待 E1/E2/E3/S4 身份确定后重采基线，本次不做即时量测。

## 2026-10-24 22:40 D1 R3 零分配收口验收（developer，6adc52e4…2a85）——**未开工，任务关闭**（零交付；契约已就绪，卡保持在手可重派）

manager 核查结论（22:40）：
- **19 个热接口一个都不存在**：`configureHot` / `warmUpHot` / `beginStepHot` / `copyStateToHot` / `checkpointHot` / `collectParticlePlanInto` / `compareParticlePlansHot` / `applyParticlePlanInto` / `rollbackStepHot` / `finishStepHot` / `preparePatchHot` / `publishPatchHot` / `restorePatchHot` / `discardPatchHot` / `finishPatchHot` / `guardAdmitInto` / `reserveInto` / `refundInto` / `closeInto` 在 entry/src/main/ets/physics/ 下**全部零命中**；`nextLeaseToken` / `expectedStepEpoch` 同样零命中。
- **无新 Hot 模块文件**：physics 目录只有 P011ParticleField.ets(42411B, 21:51) / P011ParticlePrototype.ets(23805B, 18:55) / P011ParticleRefundAdapter.ets(10263B, 18:55) / Particle.ets，**后三者修改时间均早于本卡 19:25 放行时刻**（Field 的 21:51 改动是终点指标卡遗留的 `endpointByState`，非 R3）。
- **探针未动**：manager 复跑 `test/probe_p011_particle_allocations.cjs` → **exit=2 / NOT_ZERO_ALLOCATION / remainingSiteCount=60**（与 19:05 立项时完全一致，探针文件本身 18:58 后未修改）。
- **无任何交付物落盘**：docs/ 与 test/ 下无 R3 相关新文件（22:30 后新增的仅 gesture_inject / d3_wiring 两个既有单测文件的时间戳变动）；无 R3 开发报告、无新增 Hot 单测。
- **卡上无 R3 相关评论**：仅有 7-24 07:24 的旧反馈（对应已由 rev1 补契约解决的落床余量与提交接口两项阻断），非本次交付说明。

**判定**：R3 **零交付**，本卡完成条件（探针 ZERO_ALLOCATION 且 exit=0 + §9 五组验证 + 遥测字段就位 + commit）一项未达。**不构成验收通过，也不构成「交付不合格」——是尚未开工**。

**处置**：任务关闭（无交付物可验收，避免卡片长期挂在手造成「以为在推进」的错觉）；**契约与门序已全部就绪**（热接口契约 v1 19:25 验收、主稿 rev3 已补 HOT 验证五组、R9 裁定与边界清晰、探针与基线数据在盘），随时可重派给 developer 开工。重派时沿用同一正文（卡内 prompt 已是纯 prompt 形态，无需重写）。

**顺带修正**：上一卡（终点指标实现）遗留的 `P011ParticleField.ets` 未提交改动（32 行 `endpointByState` + `P011ParticleByState`/`P011ParticleStateMass`）已在本轮核实——它是 c73ed63 的一部分但未随该提交入库（git status 显示为工作区 M）。该改动不属 R3 范围，已登记待 developer 在下次提交时一并入库（不阻塞 R3，但不得在 R3 提交中被误当作新内容）。

## 2026-10-24 22:30 P1 稿 rev2 验收（architect，6adcb885…0739）——**通过并关闭**（G1 解冻前置已满足，待放行）

交付：`docs/architecture-P013-p1-material-composite-v1.md` 升 rev2（274→346 行，22:13，工作区未提交态，按 PIPELINE 由 reviewer 入库）。

manager 逐条核实：
- **§5.2 配方与 designer 定稿逐字一致**：Rbase 取色路径（低端 palette H=(145,168,111)/L=(216,196,150) → M 按连续 fH 在**编码字节域**插值 → 乘连续列插值 gain(x) → clamp 248 → `round_nonnegative=floor(v+0.5)` → /255 → 逐分量 Q；明写「顺序不得换成先 Q 后 gain 或先 ToneMap」）；`grainBaseSrgb` M=(181,182,131)/255 且生成器 raw 仍为 (180.5,182,130.5)，**零噪声时 δB 严格为 0**；C1 中性=字节 128/255（非线性 0.5/127.5）；C3 `shadeIdx=[48,16]`→x=[0.75,0.25]→按 `rgb()` 的非负 Math.round 得 **[206,159]**，C3 为固定无光照样式、其中点分块边界是**样式阶跃须单列报告**；合成序 `Rpre=Rbase+(δC1+δC3)+δB` → 仅一次 `Rmat=max(vec3(0),Rpre)`，不逐项 clamp、不设额外上界、不做 source-over。
- **§4.2 坡度核一致**：σx=6 列索引单位 / Rx=18 列 / 37 taps / `g[j]=exp(−j²/72)` 一次归一化 / 列端点复制不环绕不镜像不二次归一化 / **深度方向 σz=0、Rz=0 恒等** / 只在沙 mask 内求值 / Float64 计算上传前量化 Float32；明写「P1 列点差分与 P0 连续差分非逐 fragment 等价」。
- **七版本号齐备**：materialRecipeVersion=P1_ALBEDO_DELTA_V1、slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2、colorPipelineVersion=LIN_SRGB_REINHARD_V1、textureSamplingVersion=P1_TEXEL_CANVAS_PX_V1、strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1、T_dataLayoutVersion=2、zOriginVersion=1；**P0 仍 layout1/E2_APPROX_P0/zOrigin1**（不给 P0 同名换义）。
- **五量方法齐备**：methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1、Ycode=0.2126·R8+0.7152·G8+0.0722·B8（截图不得再 EOTF 后仍称同一 Ycode）；R_B(亮/暗分开)/dark 绝对幅度族/Q_couple(B·C)/C_band 向量/ΔY_face 逐条给出公式、单位、采样支持集与判定方向；异常分流 EMPTY_ROI / INSUFFICIENT_SUPPORT / UNDEFINED_RATIO **不加 EPS、不默认比值为 1**；「可先固定的结构条件」与「只有实测后才能提出的数值线」分列。
- **跨栈差异清单 X1–X3 编号冻结**（各带「是否需另版配方 / 若保留旧效果怎么办」栏）：X1 旧二维 8px spark overlay 不进 P1（禁止暗补、禁止 Canvas 在 GLES 后叠画沙面亮粒）；X2 去 Hi/shade 第二光场故 OFF 非旧 Canvas E 底色逐位复制；X3 旧 EAB CLOSED / s133 E3 PASS / P0 热图不得继承 P1 效果分数。并完整列出 designer §8 的**五项 OFF 基线重采内容**。
- **门序与状态正确**：designer 项由「待签收」转「已签收」；**G1 解冻前置=本 rev2 经 manager 验收 + tester 登记 CPU fixture/材质耦合用例与 RED 基线**；**G0 surface/sRGB/三层 z 序 probe 仍须先过**（需真机、effector 队列独占），明写「不得用 rev2 文档完成代替 G0 实测」「P0 主体已验收并不豁免 P1 的测试先行/平台 probe/效果签字」。
- **修订索引 R1–R5 + 变更记录 + 全稿自检齐备**。
- **红线核验**：纯设计零代码（entry/test 的未提交改动属各线既有工作区状态，非本卡）；P1 稿与 designer 稿均为工作区未提交态（按 PIPELINE 由 reviewer 入库）；**冻结项 diff 为空**（E3 J95/J90/std、30°±2°/t90/R²、5% 全未动）；**8–20/255 与 [0.8,1.2] 三处明写仍为旧扫描域、不升级为 PASS 线**；未新建数值容差线；未自行解冻 G1；未宣称实现或验证。

**登记项**：①G1 解冻待 manager 放行（前置已满足：rev2 验收 + tester 登记 CPU fixture/材质耦合用例与 RED 基线；G0 probe 仍需真机排队）；②材质包 metadata（materialBundleId/paletteHash/grainHash/strataHash/grainBaseSrgb[3]/warpRangeCycles 等）已写入 §5.1 的 manifest 要求，资产 hash 以实际字节与生成参数为准、不以文件名当 hash；③本稿与 designer 稿均未入库，按 PIPELINE 由 reviewer 提交环节处理。

## 2026-10-24 22:15 终点指标实现验收（developer，6adc6513…bdb0）——**通过并关闭**（口径落地，协议仍未生效）

交付 commit `c73ed63`（10 文件 +1160/−6，自包含）：新增 `P011EndpointMetrics.ets`(142) / `P011EndpointCaptureSession.ets`(53) / `P011EndpointStartupClock.ets`(22)，改 `P011NaturalObservation.ets`(+55) / `WaterSandField.ets`(+140)，新增 `test/p011_endpoint_check.py`(476) / `test/test_p011_endpoint_check.py`(150) / `test/run_p011_endpoint_units.cjs`(90)，改 `p011_check.py` / `p011_slip_formal_check.py`。

manager 实测核实：
- **单测 8/0 全绿**（逐例：三分项用 E 且缺 C 时 P 保持 null 且无效 E 不得 EPS 兜底 PASS / 时钟用实际 STARTUP ms 且同 ms 允许且不猜分辨率 / 采样器拒绝重复与回退时钟且同 ms 提交身份不可变 / 真实场两子步同帧 POST_COMMIT 且累计 dt 恰好一次 / 物理审计失败记录 attempt 但不推进 stamp·ordinal·integration / reset 清采样且停用写入器时旧账本字段保持缺席 / **旧 suspensionFraction 公式不变且缺 C 摘要时不伪造** / byState 用精确正余量且忽略空槽并校验缓存闭合）。
- **合成自测 17/0 全绿**（python unittest）。
- **全通道零回归（实测）**：planning 106 / gesture_inject 13 / input 124 / render 82 / particle_field 33 / s3 14 / s4 10 / advection 8 / tracer 9 = **399/0**。
- **口径落地逐条核实**：分子 = U+P（checker `nondepositedMass=u+p`，`endpointFraction=(u+p)/reference`）；分母各自历史基准（SLIP=R_SLIP，WS=E；legacy 侧 SLIP=R_SLIP、WS=D+U）；阈值 5%+EPS；**在途不计为已沉积**（`depositedFraction=d/e`，实现侧 deposited=Σ(bh+mh)/Σ(bl+ml) 不含 P）；DEGRADE/TO_SUSP 累计量不入分子、MERGE 只计现存包、LAND 只扣 granted。
- **末 1s 窗用单调实际 elapsed**（`endpointSampleElapsedMs` 取 [start,end]，且强制 `start == end - 1000`，否则记 INVALID_FIXED_WINDOW）；**时钟用 `systemDateTime.getUptime(STARTUP,false)`**（P011EndpointStartupClock.ets，与 manager 先前核实的 SDK 签名一致），**不回退 Date.now、不臆造 tick**；窗口内每子步 (U+P)/参考 全部 ≤5%+EPS 才 `massWindowPass=True`，并另报 `windowPeakFraction` 与最差子步 commitKey。
- **双算实现正确**：`legacyEndpointFraction=U/legacy_reference`、`legacyMetricSuspensionFraction=U/max(A,EPS)`（A=D+U+P）；两者分别标注且**禁止互为别名/单独放行**（裁定 §1 A2/A3 语义分工落实）；旧 `WS_METRIC.suspensionFraction` 公式逐字未动（git diff 中 `suspensionFraction: suspended / totalSand` 无任何改动行）；`legacyMetricSuspensionFractionSource=RECOMPUTED_FROM_SAME_POST_COMMIT_STATE`。
- **状态码分流完整**：INCOMPLETE / INVALID_DENOMINATOR / INVALID_BASELINE / NOT_IMPLEMENTED / INVALID_LEDGER 五档且**硬闸门优先于 5% PASS**（hardGateStatus 独立于 endpointStatus）；`formalReleaseAuthorized=False`、`endpointProtocolEffective=False`、`report['status']='CANDIDATE_ONLY'` —— **协议未生效、效果未 VALIDATED**，与裁决记录一致。
- **字段名纪律**：`context.endpointPhaseOccurrenceId`（P011NaturalObservation.ets:709 写入 + CaptureSession 接口声明）、`legacyEndpointFraction`/`legacyMetricSuspensionFraction`、`suspFraction`/`airborneFraction`/`depositedFraction`、`endpointSampleElapsedMs` 全部按对齐裁定 §1 唯一落点；`endpoint*` 字段名集合与 architect 字段稿 §6 报告清单**逐项吻合，无自造名**；旧名 `legacySuspOnly`/`wsMetricSuspensionFractionLive` 零残留（checker 内 proof 侧 `phaseOccurrenceId` 属证明记录内部键，非裁定管辖的 `context.` 生效字段名，且与 client 侧 `endpointPhaseOccurrenceId` 对应核对）。
- **可观测性诚实**：byState 缺失 → `NOT_OBSERVABLE`（不由总量反推宣称已排除）；CONTACT_WAIT 存在 → `PERSISTENT_PARTICLE_RESIDUE` 且明写「no lifetime/permanent-suspension conclusion」；报告必需元数据缺失 → `reportMissingFields` 列出且 `reportIdentityStatus=INCOMPLETE`，不伪造。
- **红线核验**：冻结阈值 diff 为空（30°±2°/t90 0.9/R²/5% 全未动）；预注册稿 `preregistration-P011-dual-field-particle-v1.md` diff 为空（CONS-04 等 estimand 未改）；既有 v7 字段语义未改（只追加）；历史报告未改；硬闸门失败不得用 5% PASS 掩盖（实现侧即如此）。

**登记项**：①本卡只实现口径与 checker，**协议仍未生效**——按裁决记录，正式生效须待 tester 按 revision 出双报告 → designer 验证 → manager 确认适用范围；本卡交付后 C run 仍按「双口径并报、任一单口径不单独放行」执行。②`endpointBoundaryElapsedMs`/`windowStartElapsedMs` 由 occurrence 提供，窗口 1000ms 为固定值（与批准的 1.0s 一致），若将来要改须走 revision。③checker 的 proof 侧记录键 `phaseOccurrenceId` 未改名（在裁定管辖范围外），如需全局一致可另行登记。

## 2026-10-24 21:55 P1 配方确认 + 粒子视觉接线验收（designer + architect）——**两卡均通过并关闭**

### ④ P1 材质配方与坡度核确认（designer，6adc92ba…d272e）——通过

交付：`docs/design-P013-p1-material-recipe-slope-confirmation-v1.md`（28399B）+ `docs/reference-P013-p1-material-recipe-v1.json`（22291B，数学参考）+ `docs/retro-P013-p1-material-recipe-v1.md`（复盘，按 PIPELINE 的方案复盘规则主动沉淀取舍）。

manager 独立复算核实（脚本 .codex/local-test/verify_p1_recipe.py）：
- **Q 表 6 项全部逐位命中**：byte 0→0 / 128→0.275283288427 / 216→2.191679291427 / 248→15.309416630893 / 254→111.386032168802 / 255→4095（EOTF + L=min(EOTF,1−1/4096) + Q=L/(1−L)）。
- **C3 块值正确**：[48,16] 索引 → x=0.75/0.25 → 255*shade = 205.5947/159.2241 → `Math.round` 得 **[206,159]**（源码 `rgb()` 用 Math.round、`clamp255` 亦用 Math.round，非 floor——初查得 205 是我用 floor 所致的误判，designer 的 206 正确）；δC3 = 0.13370916946 / 0.02554165571 复算一致。
- **Gaussian 核**：37 项、w0=0.0666251318775、Σw=1、截断方差 35.2406243011 列²（稿称≈35.2406243）✓。
- **pb 参考值全部逐位命中**：常数 fixture 全 0 ✓；线性坡内域 c=23..104 恒 0.2 ✓；端点 c0=0.047385794656 / c1=0.101346621479 / c22=0.199999086249 ✓（**须在四遍三点平均之后再做 Gaussian**——初查缺四遍时得 0.0533/0.1132/0.2 差异，补上四遍后逐位一致）。
- **源码事实核实**：palette 精确字节 H=(145,168,111)/L=(216,196,150)/Hi=(188,205,158)/(238,226,192)（:39-52）✓；`P012_LUT_STEPS=64` → LUT 实为 65 项（:63/:917-925）✓（稿对「注释说 8 档、实际 65 项」的更正是对的）；`clamp255`=Math.round 钳位（:1184-1186）、`rgb()`=Math.round（:2406-2410）✓。
- 九项全覆盖：σ6 列域语义（σx=6 列 / Rx=18 列 / 37 项一次归一化、**深度方向 σz=0 恒等**、端点复制不环绕、只算沙 mask 内）✓；与 P0 E2_APPROX_P0 / Canvas toplight 原型的逐项对照表 ✓；CPU 对拍判据（6 个 fixture + 解析式 pb[64+d]=5·(v[d+1]−v[d−1])）✓；zOrigin1 与 C1 关系（zMaterial 用 S_D 非 S_G、C 位图不做 σ6 模糊）✓；Rbase 取色路径（低端 palette 字节 → 列级 sparkGain → clamp 248 → round，**先乘 gain 再 clamp 再 round**）✓；grainBaseSrgb 三层定义（M 用 clamp255 量化后的 (181,182,131)/255 而非 runtime fH 色，**零噪声时 δB 严格为 0**）✓；Q 8bit 适用性与触发率报告要求（仅 255 触发上界、254→255 跃迁 3983.614、Float32 ULP 0.00024414）✓；五量可测定义（R_B/dark/Q_couple/C_band/ΔY_face 的输入输出单位采样窗判定方向）✓；OFF 跨栈基线重出五项 ✓。
- 版本号建议明确：materialRecipeVersion=`P1_ALBEDO_DELTA_V1`、slopeKernelVersion=`E2_GAUSS_COL6_R18_P1_V2`、colorPipelineVersion=`LIN_SRGB_REINHARD_V1` 等七项。
- 红线核验：纯设计零代码（未改 P1 工程稿/代码/测试/冻结锚点）；未新冻数值容差（明确「须 tester 重复性/manager 冻结」、未冻结只报 MEASURED_NOT_CALIBRATED）；未把 8–20/255 或 [0.8,1.2] 旧扫描域转成 PASS 线；显式披露旧二维 8px spark overlay 不进 P1（须 manager 知悉 + architect 在 rev2 跨栈差异清单登记 + 重采 B/E 基线）；C3 样式阶跃如实披露（不得从 C_band 报告删去伪装连续）。

**登记项**：①旧二维 spark overlay 的取舍是本稿主动提出的跨栈风格差异，须由 architect 在 P1 rev2 的跨栈差异清单登记（已建卡 6adc8b… 见下）；②本稿把「R_B 保留率」与「dark 绝对幅度」拆成两个输出族（上游 R_B,dark 的 dark 原为 ROI 下标），属定义澄清非新增效果线；③G1 解冻仍未发生（须 architect 写入 rev2 后由 manager 放行）。

### ⑤ 粒子视觉接线架构稿（architect，6adc92d0…28228）——通过

交付：`docs/architecture-P011-particle-visual-wiring-v1.md`（24805B，21:47）。

manager 核实：
- **事实主张全部命中**：M1 SoA 确有 pMass/pX/pY/pVx/pVy/pPhase/pSource/pState/pId/pSourceId（:236-245），且 **pX 为连续列坐标、pY 为材料高度**（:299 写入、:320/:531 `Math.floor(this.pX[slot])` 取列、:326 state=2 时 pY=sandTopAfter+contactEps）→ 稿中「不是整数列+列内高度丢精度表示、不是像素坐标」属实；`P012FrameContext`（P012RenderTypes.ets:75）现**无** renderFrameId/particleSnapshot/particleLayerEnabled/physicsCommitId（计数 0）→ 稿中「需新增」表述与现状一致；引用的四个文件全部存在；`stageGestureSandAndFeed`（SandCanvas.ets:2383）、`p012Renderer?.sandTracer.feed(receipt,...)`（:2393）、`sandTracerEnabled`（:2449）行号全部命中；M1 对 render/P012 引用计数 0（渲染零依赖纪律成立）。
- **推荐明确且理由成立**：路径甲（P-012 Canvas2D 粒子层）作为 D4/D5 首个真实粒子呈现路径——理由是当前运行画面与 sampler/遮蔽/层序都在 Canvas2D，且 P-013 P1 的三 surface/sRGB 生命周期尚待实现与真机验证；路径乙明确「不是本卡首选，待 P1 composite 实装 + G0/G5 前置通过后由 manager 另开迁移评估」，并禁止把粒子临时画到 P0 诊断 pass 或宣称色彩等价。
- **七项全覆盖**：位置数据形态与只读投影契约（`P012ParticleRenderSnapshot` 15 字段含 physicsCommitId/particleRevision/active）+ 明确「池不存 age/seed → 不得用 particleId/sourceId/receipt 时间差伪造粒子 age」✓；两路径六维对照 + 明确推荐 ✓；state=2 可见性（**固定仍画、不得先淡出隐藏删除**，仍计入 count 与 byState.CONTACT_WAIT）与真实入床（按 grant 扣减、落床粒子同帧不再出现、**不按 y<=sandTop 自行删粒**、不制造残影补质量）✓；手势连续性（不从手势点另生成第二批视觉粒子、触点钳制归 M2、renderer 只映射）✓；模块落点表（M1 projection / M2 frame snapshot / M4 adapter / M3 P012ParticleLayer.ets / renderer z order / page-want-lifecycle / v7 telemetry）与单帧时序 ✓；并行期规则（C active ⇒ 只画粒子且 feed=false/draw=false；**C active 但 count=0 ⇒ 两层都不画、不回退 tracer**；视觉 ablation 只关显示不卸 route 不清池；切入 C 的边界帧清 B 视觉点）✓；B tracer 三条件退出 + 六步下线顺序 ✓。
- 与架构稿 rev2 的互斥纪律一致（§6.2 绘制/feed/记账/消融/拒绝注入五条规则逐条呼应），未改 §6.1「现状冻结」与 §13.5 交付边界。
- 红线核验：纯设计零代码；未自定 Nmax/粒径/alpha/速度/寿命（明确列为 D6 定标或另案）；B tracer 视觉点明确不作为 C 的守恒/落床证据；不占真机；如实登记 D1 B 未全绿、D2 未交付的现状，不宣称 D4/D5 可开工。

**登记项**：①D4/D5 实现卡未建（须 D2 原型出口后由 manager 放行）；②`P012FrameContext` 的三个新字段与 `copyParticleRenderSnapshot` 属 D2/D4 接线，架构稿只定契约；③若 D2 无法提供预分配投影入口，D4 首版退化为单一复用 adapter FrameSlot 一次 copy（稿已登记该退路并要求登记帧时与分配事实）。

**后续**：designer 稿要求 architect 将 §2–§7 定稿与跨栈差异写入 P1 rev2 —— 已建 architect 卡（见后续清单）。

## 2026-10-24 20:40 掉沙 V1-V4 重录验收（effector，6adc861c…1b12）——**部分通过，任务关闭**（硬指标 PASS；两项功能验收 BLOCKED 并转 developer 修复卡）

交付：`docs/p012-fallsand-rerecord-receipt-v1.md`（13679B，19:43）+ 产物目录 `test/artifacts/p012-fallsand-rerecord-20261024/`（163 文件：6 段 mp4、93 帧序列、7852/11935 帧 WS_FRAME、ab-control/before/after/shake 读数、速度扫描与手势候选 jpeg、9 个采集脚本 + 分析脚本）。

manager 实测核实：
- **长时注入无冻结 = PASS（硬指标成立）**：独立复跑 `frames-longinj-wsframe.txt`（UTF-16 with BOM，7.9MB）→ WS_FRAME 行 11935、全部可解析且均含 `seg=`；frameTimeMs min/P50/P95/P99/max = 7/11/23/41/232；**>1000ms 帧 0**、>200ms 帧 1；dormant 88%、substeps>0 11%；phys max/P95 = 26/12ms。→ H01 冻结门槛零触发成立（回执称 7852 帧 / max 237ms，与原始文件实测不符，见「登记项 R1」）。
- **掉沙准入为零 = 复现成立**：ab-before 与 ab-after 的 WS_METRIC **逐位相同**（16 位有效数字）：depositedHeavyFraction 0.9999616691564155、suspensionFraction 0.0009893040214389464、phaseOrderScore 0.9999914665357793、maxSandTopSubstepDeltaObserved 0.0003214331209022703、physicsTimeMs 0、dormant true；control（不同进程 33315）读数同量级。→ 注入后字段仍 dormant、suspF 未变、沙面位移未变 = **该次 swipe 准入量为严格零**。回执对「depH 是比值、对加性注入不敏感故不能作判据」的判据选择判断正确。
- **shake 未进物理场 = 复现成立**：ab-shake 与 control 同量级（suspF 0.0009973170882261835 vs 0.0009958862763543835）、dormant 恒 true。
- **画面结构核验**：跑 effector 自带 `tools/analyze_fallsand_rerecord.py` → 六段分带一致（air 0.2-42.0% RGB(16,16,29)、water 42.0-96.3% RGB(195~206)、sand 96.3-100.0% 绿砂带存在）；V2 倾斜态 air 顶界 41.1% 与其余 42.0% 偏差 1% 内；shake 段首帧 water 延伸到 98.6%（末帧回到 96.3%）；longinj 27 帧 avgDiff 0.1、maxStillRun 26 帧（~6.0s）被脚本标 suspected_freezes=1——与回执「满容后画面静止但帧时正常」的判读一致（像素差法与帧时法结论不同，非矛盾）。

**回执四条根因主张的 manager 复核（三条属实、一条不成立）**：
- ✅ 主张 C（可观测性缺失，验收阻塞第一优先项）：`injectGestureSand`（WaterSandField.ets:1508）只返回 receipt（admittedH/admittedL/rejectedReason），**全仓无 `WS_EVENT gesture_sand` 日志**；拒绝原因枚举确实存在（:1550 NO_WATER、:1619 CAPACITY_BLOCKED）但不上报 WS_METRIC → 真机侧无法区分「手势未到达」与「到达但被拒」。成立。
- ✅ 主张 B（局部坐标二次累加）：`routeGestureSand`（SandCanvas.ets:2317）每事件先 `this.lastPanLocalX = localX`（:2320），而调用点 :3149 在 `e.fingerList` 为空时回退用 `e.offsetX + this.lastPanLocalX` → 局部坐标累加放大，越出 `boxX+boxW` 被 `gestureSandGate`（:2295-2307）拒绝。成立。
- ✅ 主张 A（Pan 竞技场未判定成立）：回执给出 TTHRTI 中 PanRecognizer 状态恒 `T: Stack`（未 Accepted）；配合回退路径缺陷，自动化注入下手势很可能未进入 onActionUpdate。**注：自动化手势能否被判 Accepted 属环境事实，未由 manager 独立复现**。
- ❌ **主张「shake 门控错配」不成立**：回执称 `SandCanvas.ets:908` 的门控被 `MODE_LIQUID` 包住而对象是 waterSandField。manager 复核工作区与**全部 6 个提交点**（693a724/beb11fa/60d24bf/bcada09/de0da61/HEAD）：门控**一直是 `MODE_WATERSAND`**，且已有 `WS_EVENT shake` 日志（:910-913）。**shake 不可达的真实原因不是门控错配，而是 Want 入口只把 shake 送给 LiquidCA**：`EntryAbility.ets:333` 写 AppStorage `liquidStartShake` → `SandCanvas.ets:637` 赋 `liquidShakeInput` → :1271-1272 推给 `liquidController.setShake(...)`；`waterSandShakeInput` 仅在 :907（面板滑块回调）赋值 → :909 调 `waterSandField.setShakeInput`。**两条 shake 通道互不相通**。移交 developer 的修复项已按此更正（不要求改门控，改为补 Want → waterSandField 的直通或改门控为 MODE_WATERSAND 的写法是错的）。

**移交 developer 的修复项（已建卡 6adc8a… 见下）**：①`WS_EVENT gesture_sand` 逐笔事务日志（按 algorithm-P011-gesture-sand-source-v1 §4 的 sourceId/sequence/inputTime/requested/admittedH/admittedL/rejectedReason）——**验收阻塞第一优先项**；②`sourceInjectedHeavy/Light` 并入 WS_METRIC（或旁增外源账本行）消除比值盲区；③`routeGestureSand` 局部坐标回退路径修正 + `lastPanRouteTime` 手势边界复位（:1682 已有一次复位，需在每个手势开始时复位）；④PanGesture 补可自动化观测点（onActionUpdate 计数与末次 localX/localY）；⑤shake 的 Want → waterSandField 直通（按更正后根因，不改门控）；⑥`liftByShake` 生效时落 WS_EVENT。

**登记项**：
- R1 回执帧数与 max 与原始文件实测不符（回执 7852 帧 / max 237ms / dormant 81.9% / substeps 18.1%；manager 实测 11935 帧 / max 232ms / dormant 88% / substeps 11%）。**结论不受影响**（>1000ms 帧 0 在两种口径下均成立），但回执的数字与冻结判据表须按实测更正。
- R2 `tools/analyze_fallsand_rerecord.py` 的 WS_FRAME 解析依赖 `split('seg=',1)`，本轮日志恰好全部含 `seg=` 才解析成功（11935/11935）；若日志行不含该字段，解析将全部失败并被 `except: pass` 静默吞掉（脚本输出会缺 WS_FRAME 行而不报错）——属分析脚本健壮性缺口，登记待修。
- R3 `longinj-wsframe-live.txt` 为 UTF-16LE 无 BOM，被按 UTF-16 读会产生乱码（回执称其含 14 个实时采样，实测该文件无 WS_FRAME 行）——文件命名与内容口径需核对。
- R4 方法学限制（回执已诚实登记，本轮确认）：hdc snapshot_display 单帧 1–2.7s、设备无系统录屏，帧序列为 4–7fps 采样非连续视频；帧率不足以逐帧观察注沙喷流，故注沙是否发生以字段状态量判定而非画面判读——**这是本轮方法学的正确取舍**。

## 2026-10-24 19:25 M1 热接口契约验收（architect，6adc91c3…02a3）——**通过并关闭**（两处小缺返修，不阻塞 D1 开工）

交付：`docs/architecture-P011-M1-hot-buffer-contract-v1.md`（29055B，19:17，独立附件）+ 主稿新增 §17 与变更记录条目（主稿 827→924 行）。

manager 实测核实：
- 现状主张全部复现：探针 `probe_p011_particle_allocations.cjs` exit=2、sites=60（Field 31 / Prototype 29），与卡内登记一致；`warmUpTransactions()`/`copyStateTo(out)` 存在（P011ParticleField.ets:657/:662）；journal 默认 limit=4096（:695）→ 附件 Q/E 默认 4096 有源码依据。
- 载荷算术逐位复算：55N+16C（N=512、C=128）= 30208 B/份，6 份 = 181248 B —— 与附件数字完全一致；并诚实标注「不是整个 workspace 内存、不用下界冒充 RSS」。
- 纯设计零代码：entry/ 与 test/ 的未提交改动全部为此前 D1/P0 各线交付（19:07/19:22 修改时间在本卡 19:08 建卡前后，属既有工作区状态），本卡两文件均为 docs，无产品/测试代码新增。
- 六项全覆盖：①八类定容缓冲清单（§2 容量符号表 C/N/Q/M/E/G/P + §2.1 行格式 + 定长上限与 liveCount×2 相容）；②数值身份与代际 token（§4.1 唯一令牌、单调不回卷、MAX_SAFE_INTEGER 耗尽进 TOKEN_EXHAUSTED；§4.2 expectedStepEpoch 由调用方保存，plan 引用本身不是授权句柄——正面回应了 developer 提出的 ABA 顾虑）；③§13.3 双跑等价性（§5 数值 tuple + seenEpoch 去重 + hash 不得替代逐值核对）；④验证要求（§9 五组可执行项 + 22 项只读遥测字段）；⑤交付边界（§8 文件责任表 + D2 仍归真实接线）；⑥方案 B 明确不采用（首段与 §10 表各一处）。
- 红线核验：§4.4 零分配硬性措辞未降；R9 逐端实际扣额/total 一次/bottom 0/负对照语义未变（§3.1 明写「不因为 API 变了改成旧平分退款」）；冻结阈值与预注册 estimand 未动；不授权 D2、不关 D1；状态自标 DESIGN_READY / PENDING_MANAGER_ACCEPTANCE / NOT_IMPLEMENTED / NOT_ZERO_ALLOCATION_VERIFIED。

两处小缺返修（不阻塞 D1 开工，manager 直接改）：
1. §13.6 未同步热接口验证要求（该节现仍只有 CONS-03/BUD-01-02/分母/原子性/路由五条旧项，HOT 词命中 0）；卡上第 4 项明确要求「同步更新 §13.6 验证要求」——须补一条指向 §17/附件 §9 的 HOT 验证子项并注明「附件 §9 五组为 HOT 阶段的追加验证要求，不豁免既有五条」。
2. 主稿页首「当前有效修订」仍写 rev2（待 manager 验收），未更新为含 §17 的 rev3；§16 索引未增 rev3 条目。

解锁：developer 的 D1 R3 收口（卡 6adc52e4…2a85）按本契约实施——零分配门槛不变、方案 A 已定；D1 R3 通过后才是 D2 解锁。

## 2026-10-24 19:15 P1 工程设计 + peakSusp 另案方案验收（当前生效）

### 1. P1 工程设计 —— **验收通过**（卡 6adc861c…d612 关闭）

交付：`docs/architecture-P013-p1-material-composite-v1.md`（274 行/十章，19:02 落盘）+ 立项书同步 §8 附件登记（`docs/proposal-P013-incorporation-v1.md` 追加 P1 附件索引，保留立项历史）。

**方案主线**：CPU 保留现有确定性材质资产生成 + GLES 完整沙区单 pass + Canvas 水雾/前景双层共存。三种做法明确不采用：跨 Surface 乘法、每帧读回完整 Canvas 再上传、把旧已受光画面当反照率纹理。ArkUI Stack 责任划分=Canvas background（水体底色+水体雾）/ XComponent（完整沙体底材+C/C3+B+E2 单 pass）/ Canvas foreground（tracer+活动粒子+液面高光+interface 雾+容器边框）。**并诚实登记三 Surface 无法保证同次呈现**（Canvas 无与 EGL 共享的已证实呈现 fence）→ 若 z 序/透明合成不可用则标 `COMPOSITION_ORDER_BLOCKED/FRAME_ALIGNMENT_UNPROVEN` 并保留 Canvas，不把静态 PASS 当动态共存已过；明确「不能把双 Canvas+Surface 写成天然性能收益」。

**manager 逐条核实其 P0 现状描述——全部属实**（manager 亲查代码）：

| P1 稿主张 | 实测 |
|---|---|
| `P012_B_GRAIN_ALPHA=0.75` | ✅ `P012WaterSandRenderer.ets:117` |
| `P012_C_ALPHA=0.5` | ✅ :147 |
| `P012_C3_ALPHA=0.10` | ✅ :152 |
| sparkGain clamp 到 248 | ✅ :2400-2402 `Math.min(248, r*gain)` |
| C 位图灰度中性值 128 | ✅ :146 注释「位图灰度 128±deltaY」 |
| shader 关闭 blend/深度/模板/裁剪/背面剔除 | ✅ `shader_pass.cpp:80-84` |
| EGL 未指定 sRGB surface | ✅ `egl_context.cpp` 无 COLORSPACE 相关调用 |
| T_data layout1 = 128×1 RGBA32F | ✅ `P013RenderTypes.ets:28` `T_dataLayoutVersion=1`；`tdata_uploader.cpp:9` `glTexStorage2D(...,128,1)` |

**版本隔离正确**：P1 固定 `T_dataLayoutVersion=2 / slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2 / zOriginVersion=1 / materialRecipeVersion=P1_ALBEDO_DELTA_V1 / colorPipelineVersion=LIN_SRGB_REINHARD_V1 / textureSamplingVersion=P1_TEXEL_CANVAS_PX_V1 / strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1`，**P0 仍为 layout1/E2_APPROX_P0，不把三行 buffer 交给旧 P0 校验器**。§5.1 色彩链唯一（sRGB EOTF→Q 映射→Rbase+δC（含 C3）+δB 下界 0→乘同一 Lmat→一次 ToneMap `x/(1+x)`→sRGB default framebuffer 硬件 OETF 一次），并逐项列禁止做法（pow(c,2.2) 冒称同版本、字节 255 映射无穷、每帧自动曝光、先 ToneMap 再乘 Lmat、对 sRGB framebuffer 双编码、负 RGB/非沙白底/半透明 gamma 域预乘含糊、P3/HDR 截图混测）。sRGB surface 失败或实际非 GL_SRGB → `COLORSPACE_UNSUPPORTED` 降级 Canvas，**无未登记的「自动 shader gamma 兜底」**；并指出「本地 SDK 头有 token 只证明可编译，不证明真机扩展/系统标色能力」。

**诚实性核验**：状态行 `ENGINEERING_DESIGN_READY / PENDING_MANAGER_ACCEPTANCE / PENDING_DESIGNER_RECIPE_CONFIRMATION / NOT_IMPLEMENTED / NOT_VALIDATED`；§9 明写「所有以下为待实现测试清单，不是本稿已跑的结果；数值容差不由 architect 新冻」；§5.2 的 Rmat 公式明标「是 designer Rbase+δB+δC 的一版可实施提取方案，不宣称与 Canvas 旧伪法向底色逐位等价」；§10 列出四项待签收（designer 确认 σ6 与 Rbase/δB/δC 提取配方、manager 验收双 Canvas 共存成本与 P1 范围、tester 登记色彩重复性与容差与 RED 资产、设备 probe 确认 sRGB attachment 与三层合成）；G0–G5 实施切片需 manager 分卡授权，**G0/G5 需真机时由 manager 建卡排队，architect 不得自行装 HAP 占设备**。未改冻结锚点、未动 P0 layout1 与三版本号、未越界设计 P2/P3。

**manager 裁决**：验收通过并关卡。**P1 实施暂不排期**——按 §9 门序，起点是 G0 前置 probe（sRGB surface/profile + 三层 z 序/透明/输入透传，**需真机**）+ designer 确认材质配方（§5.2 的 Rbase/δB/δC 与 σ6 列域）+ tester 用例与 RED 基线先行。**下一张卡建议为 designer 的材质配方确认卡**（离线，不占真机），G0 probe 待真机队列。

### 2. peakSusp 另案方案 —— **验收通过**（卡 6adc64ff…b43 关闭，转用户另案裁决）

交付：`docs/design-P011-ws08-peak-nondeposited-migration-v1.md`（201 行）+ 复盘 `docs/retro-P011-ws08-peak-nondeposited-v1.md`。状态 `DESIGN_FINAL / READY_FOR_MANAGER_TO_ROUTE_TO_USER / PROPOSED / NOT_APPROVED / NOT_IMPLEMENTED / NOT_VALIDATED`。

**唯一推荐 = 候选 B**：固定摇晃段 W=`[t_on, t_on+3000ms)` 内每个成功提交子步的 `max (U+P)/E`，独立命名 `peakNonDepositedFraction`（P 含 state=1/2，接触待准入也计）；保留 0.5 及格线与原 10/8/6/4/2/0 分档锚点；旧 `peakSusp` 保留原身份双报不替代；获批且实现验证后才可替代 C 的峰值子项。

**manager 核实其关键论证——成立**：

1. **失真机制正确**：工程稿 §2.2 把合法 LIFT 的汇由 susp 改写为真实粒子、IH/IL 不增 → 同量沙从床进 P 时旧分子 U 可不变甚至下降；池满时 DEGRADE 又把 P 转 U、旧值可能上升 → 只看旧量会把表示方式与合并/降级策略当成扬起效果差异，**可能**结构性低估。
2. **量级标注诚实**：450 units（300 unit/s 共享额度全给 LIFT、候选平均在途 1.5s）与 Nmax=512 对应名义质量 25.6/51.2/128 units 均标 `ASSUMPTION，均非测量`；并指出 §3.3 已澄清「Nmax×nominal m 不是合并后的质量硬上限」。
3. **历史出处核查扎实**：唯一可核查的直接出处是 test-report-P003-P005.md 的 2026-10-13 RED 基线（TC-WS-08 记 `BLOCKED / NOT_IMPLEMENTED`，产物 ws_check.json）——**用于判定「证据不可测」，没有实测峰值通过结论**；effect-report 的 EC-WS-06 同样为未实现基线、无主观评分；P-011 SLIP 的「悬沙≤5%」属终点不能冒充摇晃峰值。
4. **发现既有适配缺口且不越界**：`test/water_sand_check.py::evaluate_shake()` 只检查存在 shake 事件、随后 `max(values)` 取**全序列**，未落实摇晃 3s 分段、未完整实现 TIERED 评分 → 登记为既有脚本与书面用例的适配缺口，**本卡不修脚本**，迁移时由 tester 单列分段/评分适配。
5. **差值恒等式正确且不冒名**：仅在守恒 A=E 且 D+U>0 时 `f−l=P/E`、`f−h=P·D/[E(D+U)]≥0` → 与历史分母的差值**不是无条件 P/E**；`max(U/E)+max(P/E)` 通常**不等于** `max((U+P)/E)`（分项须在总量峰值提交同时披露）。
6. **可达性必要界成立**：无外源无 invert/reset 场景下 `M(k)=M0+累计已准入LIFT−累计已沉降/落床`，故 `peak M ≤ M0+300·tau_W`，必要条件 `M0+300·tau_W ≥ 0.5E`；假设 M0=0、tau_W=3s → 最乐观峰 ≤900 units，要到 50% 须 E≤1800；若 E=8000 则需 lift 积分时间 ≥13.33s，**原 3s 假设下不能达标** → 足以否定「改成 U+P 就自然能过 0.5」。明写不暗增 300/s、不延长摇晃、不调低 0.5、不减总沙量求绿。
7. **边界清楚**：§7 明列已批准终点 revision 的残余口径/末 1s 窗/state=2 滞留诊断/DUALFIELD estimand/30°±2°/t90/R²/E2/E4 全部不改一个字；「用户已批准终点不等于批准本案峰值」；新峰值用独立 `peakProtocolId` 身份、不升级或覆盖 endpoint 身份。

**manager 裁决**：验收通过并关卡。**转用户另案裁决**（五项）：①采纳候选 B（库存峰值）②分子含全部活粒子（含 state=2 接触待准入）③分母用 E ④固定 3s 摇晃窗内每成功提交子步取 max ⑤原 0.5 及格线与 10/8/6/4/2/0 分档锚点迁移。批准后的执行序列（稿 §8 六步）：tester 发独立 peak protocol → architect 登记独立 peak 字段 revision → developer 实现采样/流式峰值/checker → tester RED 基线 + 同账本 legacy 与新量双算（含共同 250ms 样本对照与全部子步新峰值，分子分母差异与采样密度差异分开归因）→ effector 真机串行排队 → designer 验证 + manager 确认适用范围/独立 `peakProtocolEffective`。

## 2026-10-24 19:05 D1 B 段 R9 收口验收（当前生效）——**部分通过，任务不关闭**（R3 零分配待用户裁定）

交付：commit `ae3b8b9`（8 文件 +507/−18：`P011ParticleRefundAdapter.ets` +164 新建、`P011ParticlePrototype.ets` ±31、`P011ParticleField.ets` +36、两个单测文件 +177、新增 `probe_p011_particle_allocations.cjs` +57、报告 2 份）+ `docs/dev-report-P011-D1-R9-v1.md` + `docs/proposal-P011-D1-R3-buffer-contract-v1.md`。

**manager 实测复现**：

| 通道 | 结果 | 说明 |
|---|---|---|
| M1 A+B（600s/36,000 子步沙盒） | **33/0** | 上一轮那条 FAIL 已按裁定转为「负对照」形式并 PASS：`R9 reserve matches real clip grid; adapter refund vs rev1; legacy pair is known-incompatible`（真实 reserve 网格逐值相等仍为硬断言，旧平分差值逐值断言并输出 negativeControls，未删断言、未改判据） |
| R9 adapter/旧退款守卫（新文件） | **11/0** | 双端 WAIT 两端各退 0.4+total 一次+CLOSED 净差为零 / 单端部分授予终值 0.3 不被误归零 / 两次合法部分退款+零退款+超退拒绝且账本不变 / 畸形·NaN·负数·未扣足·已关操作无部分写入 / 短 buffer 与别名 out 在改账前拒绝 / 过期·ID 代际·sidecar 溢出原子拒绝 / close 残差拒绝保留 sidecar 与 out / adapter 存储可替换且正常路径无分配语法 / 旧平分负对照保留 0.2/0.2 差异且 total 只退一次 / 旧平分与保守不重分配行为被钉住 / **旧 refundLocal 与 materialGuardRefund 方法体规范化 SHA256 守卫**（且断言方法体恰为 1 处） |
| 既有通道回归 | **372/0** | gesture 13 / planning 106 / S3 14 / S4 10 / input 124 / d3 6 / tracer 9 / advection 8 / render 82 |
| R3 零分配探针 | **NOT_ZERO_ALLOCATION（exit=2）** | manager 亲自复跑确认 exit=2；全 M1 静态审计仍登记 **60 处**潜在分配站点 |

**红线核验（manager 实测 git diff）**：`P011MaterialGuard.ets` 与 `WaterSandField.ets` 在 `b9b3884→ae3b8b9` 区间**均无 diff** → 旧 `refundLocal`/`materialGuardRefund` 零改动、**未越界写 WaterSandField**（与 R9 契约「本卡只做桩形态」一致）；旧函数行为由测试钉住（方法体 SHA256 + 平分/保守不重分配语义双守卫）。未改冻结阈值、未改预注册 estimand、未做真机操作。

**★ R3（生产热路径零分配）——未通过，已如实上报并提交待裁提案**

- 架构稿 §4.4 原文要求「粒子池定容预分配，**运行期零分配**（事务日志用定长环形缓冲…）」；rev2 稿 §13.5 与 §16 **明写「旧函数回归守卫、热路径零分配仍为 developer/manager 尚需验收的独立项，本文不豁免、不关 D1」**——architect 已把它列为不豁免项。
- developer 已完成的收口：R9 adapter 的 sidecar 为构造时定容 TypedArray/字符串槽（10,000 次跨子步注册·refund·close 的构造器 trap + 正常路径 AST 审计通过，**只覆盖 adapter**）；M1 新增 `warmUpTransactions()`/`copyStateTo(out)`，prepare 复用一个 scratch 池与 before/after 完整状态缓冲（旧 `snapshot()` 保留诊断端独立副本语义）；独立探针 `probe_p011_particle_allocations.cjs`（预热后 100 次零授予 deposit，scratch 与两份状态缓冲引用恒定、TypedArray 构造数 0）。
- **未完成路径（60 处站点分布）**：`P011ParticlePrototype.ets` apply 19 / executeParticlePlan 5 / guard 3 / reserve 1 / view 1；`P011ParticleField.ets` collectParticlePlan 11 / applyParticlePlan 5 / beginStep 2 / checkpoint 2 / edit 3 / finish 3 / mergePair 2 / prepare 1 / record 1 / rollback 1。涵盖 `beginStep` 的参数与冻结视图对象及身份序列化、collect 的请求/运动数组与排序闭包、journal finish 的复制与序列化、apply 的结果数组、prepare 的独立 patch 句柄及出生/合并事件对象、prototype 字段与预算 checkpoint、动态 key Set 等——**删除 TypedArray 分配不能令这些路径自动变成零分配**。
- developer's 处置正确：未自行降低标准、未把 319/0 冒充全完成、未自行采用降级方案、未自行启动 D2；提案明标 `PROPOSED / NOT_APPROVED / 不替代原零分配门槛 / 不关闭 D1`。
- **manager 裁定项（待用户）**：
  - **方案 A（保持严格门槛）**：需 architect 先确认补充热接口契约——保留原公开 API 作冷诊断/兼容入口，新增调用方拥有的定容 plan/request/motion/result/field/budget/guard/event 缓冲（显式 count/index，不用 push/slice/临时 Map/Set）；热路径用数值 request 身份与代际 patch 句柄，序列化字符串 requestKey 与 JSON 只在证据导出边界生成；**patch 句柄不能简单换成被循环覆写的同一返回对象**（调用方保留旧引用会别名到后续新句柄，破坏「旧句柄/跨子步拒绝」要求）→ 须由 architect 明确数值代际令牌或 caller-owned out 的生命周期与 publish/restore 校验契约。验收以完整 M1 可达调用链的显式分配审计 + 长时固定缓冲身份 + 错误回滚 + 既有全部守恒/预算用例验证为准（不能只测 refund 或只禁 TypedArray）。
  - **方案 B（有界分配降级）**：显式接受「定容 canonical/scratch/票据/日志存储，热交易无 TypedArray/池重建；允许有界计划/句柄/序列化轻量分配」的隔离原型门槛，严格零分配另立独立卡。**此方案改变当前完成标准，须用户显式批准**。
- manager 倾向：**方案 A**，理由=①§4.4 的零分配是原稿硬性措辞且 rev2 明写不豁免；②D1 是 D2 生产接线与真机帧时的前置，零分配缺口在 D2 才暴露的代价更高（ArkTS 真机解释开销本就是桌面数倍，§4.4 已登记「以真机 P50/P95 为准」）；③方案 A 需 architect 补接口契约，成本约一轮文档往返，而方案 B 会把门槛永久降级且与 P-013 真机帧时预算口径不一致。**但这是完成标准的变更，须用户裁定。**

**任务状态**：**不关闭**。剩余范围=①R3 零分配收口（待用户裁定 A/B；若选 A 需 architect 补热接口契约卡）②manager 验收后进入 D2。

## 2026-10-24 18:40 架构稿 rev2 + 字段对齐验收（architect，当前生效）

**验收通过**（卡 6adc85fc…81cb 关闭）。交付：两份稿的 rev2（工作区未提交态，按 PIPELINE 由 reviewer 提交环节入库）——`docs/architecture-P011-dual-field-particle-v1.md`（827→rev2，新增 §13.2 R9-REFUND_ADAPTER、§15 endpoint 对齐、§16 rev2 索引）与 `docs/architecture-P011-endpoint-fields-revision-v1.md`（升 rev2）。

**一、R9-REFUND_ADAPTER（退款 adapter 契约）——manager 逐条核实**

- **接口形态**：保留 M1 公开 `P011ParticleBudgetSink.reserve/refund/close` 签名与 scalar ticket；**新增 D2 内部 adapter**，以票据 sidecar 保存逐端实际扣额（`localReserved`/`localRefunded`/`netLocalUsed`/`netLocalDelta` 等），sidecar 生命周期同 substep、预分配复用，不以 array push/对象新建实现生产热路径。
- **原子动作五步**（契约可执行）：①校验（substep 有效/未 close/amount 有限非负/amount≤totalReserved−totalRefunded 且不超每端 reserved−refunded；无票据·过期·已关·重复·超额·非有限全拒绝，抛契约错误由上层回滚，拒绝前后账本逐值不变；amount=0 为幂等空操作但已 close 仍拒绝）②预检实际 localUsed/totalUsed 足以退本次 amount，不足即报错回滚，**不用 min(amount,used) 静默吞掉欠退**、不借其它票据额度补本票据、冻结 1e-9 仅作审计残差不授权多退 ③各端 `localUsed−=amount; localRemaining+=amount; localRefunded+=amount`，**两端各退 amount，不是 amount/2** ④**循环外一次** `totalUsed−=amount; totalRemaining+=amount; totalRefunded+=amount`，bottom 不读写 ⑤填 out 并核查 Δ 逐端同量、used+remaining 维持原 limit，任一阶段失败恢复全部已动端/total/sidecar/out，不出现半退款。
- **close 与净差定义**：`netLocalUsed=a−f`、`netTotalUsed=a−f`；close 要求均等于最终 committed g（`netLocalDelta=0`、`netTotalDelta=0`，按冻结 1e-9 审计），底账全 0；**REFUNDED 未 close 时 `netDeltaValid=false`，只有 CLOSED 的净差有最终审计效力，禁止把占位 0 报为已通过对账**；close 后不可再 refund/close，不隐式退回未解释余额。合法多次部分退款允许（累计不超各端与 total 实际扣额）；「重放退款拒绝」指超额/重复已完成操作不可再退；异常整笔中止时直接恢复 before-image，**禁止又 restore 又 refund 导致双退**。
- **两种禁止接法已明确写入**：①`materialGuardRefund(q,[source,target],false)` 会平分而欠退；②为每端各调一次 `materialGuardRefund(q,[c],false)` 虽 local 能退足但 **total 会退两次**。D2 本版采用新增 adapter 直绑票据存储，**不修改旧 materialGuardRefund/refundLocal**；manager 裁定的「新增逐端实际扣额入口」只作等价实现选择，若必须改旧函数内部则全部旧调用须逐值不变并有回归守卫。
- **reserve 记录规则**：`localColumns=[source]` 或 `[source,target]`（source≠target 不重复列）、每端 `localReserved[i]`=本票据实际 local 扣额、`totalReserved`=本票据实际 total 扣额；当前粒子 reserve 保证每端 localReserved=totalReserved=reserved，**requested 可大于 reserved 但 requested 从未成为退款上界**；两端点扣 a 不意味 total 扣 2a。
- **§13.6 已同步**：BUD-01/02 增「退款按 R9 对照粒子 adapter 桩」（覆盖 local 先截断后 scale/guard 部分退款/T4 部分许可整笔退/重复与超额退款拒绝/bottom 不变），旧平分函数双端退款唯一对照按裁定作为**保留差异数值的已知不兼容负对照**，不得把差异掩成生产 adapter 等价。
- **§13.2 旧句已改写**：原「包一层票据校验后传双端列表」明确标注**不能满足本契约**（:666）；§16 索引 R9-REFUND_ADAPTER 条目已登记（:895）。
- **诚实性核验**：稿头标注「D1 退款接法与零分配等剩余项仍由 developer 完成后交 manager 验收，本稿不代关卡」；§16 末段声明「旧退款负对照是 manager 指定的唯一对照口径修订，不豁免 adapter 桩、其它 §13.6 验证或生产零分配」；§13.6 明写「不是本次已执行的测试结果」。

**二、字段 revision R1–R3 —— 全部落文**

- R1：§1 身份表登记 `endpointProtocolId=p011-endpoint-terminal-nondeposited`、`endpointProtocolRevision=endpoint-terminal-nondeposited-v1`（取自 tester 稿 §0）、`endpointFieldAlignment=ALIGNED`（证据=裁定 + tester 首版 commit 69e27b9 + 本稿改名记录及导出 hash），并明写「ALIGNED 仅指字段契约对齐，不等于已实现/已验证/正式放行」。
- R2：§1 16:24 的「尚未落盘/不得登记 ALIGNED」**诚实声明按要求保留**（:18 标为「历史核查记录（保留原诚实声明，R2）」），并补 16:26 落盘 + 18:05 裁定事实链。
- R3：§6 报告字段清单「展示与双算」组已含 `legacyEndpointFraction`（SLIP U/R_SLIP、WS U/(D+U)）与 `legacyMetricSuspensionFraction`（U/max(A,EPS)），并新增 `legacyEndpointReferenceKind`/`legacyEndpointReferenceMass`/`legacyEndpointResult`/`legacyReportIdentity`；§4「bySourceKind 不可与 byState 再相加」保留。三处唯一落点 = `context.endpointPhaseOccurrenceId` / `legacyEndpointFraction` / `legacyMetricSuspensionFraction`，与裁定 §1 A1–A3 逐字一致。
- **红线核验（manager 实测）**：tester 协议稿与 test-cases.md **git diff 为空**（逐字未动，architect 未越权改 tester 协议）；未改产品/测试代码（纯设计零代码）；未改冻结阈值、未改预注册 estimand；旧 refundLocal 平分语义明确写为「不得修改」。

**三、architect 主动提出的一处超出 R1–R3 字面范围的澄清 —— manager 裁定：采纳，但须补登记**

architect 在字段稿 §5 指出：原 v1 把 `endpointFractionDelta=P/R` 无条件接在 legacyEndpointFraction 后，**在 WS 场景不成立**（WS 的 legacy 分母 `A_hist=D+U`、新口径分母 `E`，两者不等），故精确改定义为 `endpointFractionDelta=endpointFraction−legacyEndpointFraction`，并另报 `sameReferenceSuspOnlyFraction=U/R_new`、`sameReferenceParticleContribution=P/R_new`（不冒名 legacy）。

- **manager 复核：澄清在数学上成立且必要**——裁定 §1 A2 规定 `legacyEndpointFraction=U/R`（WS 的 R=A_hist=D+U），tester 稿 §1.2 规定 WS 新口径 `R_WS=E`，故 WS 下 `P/R` 恒等式确实不成立；新判式 `M/R`、阈值 5%、末 1s 窗**未被改动**，澄清只限定「差值字段」的含义，未触碰任何冻结口径。
- **裁定：采纳**（`endpointFractionDelta` 与两个 `sameReference*` 辅助量作为字段 revision 的补充定义），但**要求补两处登记**：①architect 稿 §16 rev2 索引须增列该澄清条目（现 §16 只记了 R9/R1/R2/R3 四条，未记此澄清）；②本裁定记入 priority.md 与 memory，供 developer 实现时以「差值字段按此定义」为准。
- **不要求改 tester 稿**：tester 稿未出现 `endpointFractionDelta`（实测计数 0），不存在两稿冲突。

**四、解锁与后续**

- **D1 B 段剩余三项的前置全部解除**（卡 6adc52e4…2a85）：R9 契约已到位 → 可开工 adapter 桩 + 逐值断言、零分配收口、旧退款函数不变守卫。
- **终点指标实现前置解除**（卡 6adc6513…bdb0）：两稿字段名已完全一致（tester ALIGNED + architect ALIGNED），developer 可按裁定 §1 唯一契约开工。
- **D2 前置**：D1 全部完成 + rev2 的 R9 契约（已到位）。

## 2026-10-24 18:30 endpoint protocol 返修验收（tester T1–T3）

**验收通过**（卡 6adc64e7…bc1 保持完成态）。交付：commit `a3ca53c`（2 文件 +29/−14，protocol 稿 33 行变更 + test-cases.md 10 行）。

- **T1 完成**：`context.phaseOccurrenceId` → `context.endpointPhaseOccurrenceId`，落点 §3.2（正文+说明行）、§6.2 遥测字段表行、§11 字段名唯一落点表、§14 用例条目。
- **T2 完成**：§5.3 与 §6.1 的 legacy 双算字段名统一——`legacySuspOnly.recomputed` → `legacyEndpointFraction`（U/R，R=该 case 历史分母：SLIP `R_SLIP`、WS `A_hist=D+U`）、`wsMetricSuspensionFractionLive` → `legacyMetricSuspensionFraction`（U/max(A,EPS)，A=D+U+P）；§6.1 的 `legacySuspOnly` 整块拆为两个具名字段，块内「原分母/原 PASS-FAIL/身份 legacy-susp-only」随更名保留。`docs/test-cases.md` §14 同步改名 5 处（§14.0 采样身份行、§14.0 报告字段行、ENDPOINT-04、ENDPOINT-05、§14.3 checker 适配摘要）。
- **T3 完成**：§13 追加记录已补「字段名统一（依对齐裁定 §1 A1–A3），语义未变」条目。
- **登记项（合规）**：§0 身份表新增 `endpointFieldAlignment=ALIGNED`（证据=对齐裁定 + commit 69e27b9 + architect 字段稿）；§11 增补三字段唯一落点表。**§5.3 核心结论逐字保留**：C run 中两个 legacy 量必然不同、禁止互为别名、禁止任一单独放行、报告分别标注；另补一句边界说明（P=0 且两分母口径一致时数值可相同，不构成双算失败，禁止项不变）。
- **一处超出 T1–T3 字面列举的连带改名，已显式登记**：§7.2 的 N10 原引用 `legacySuspOnly` 块，该块已在 §6.1 拆为两具名字段，故 N10 字段引用同步改；**N10 判定内容（旧判据+原分母+原 PASS/FAIL+身份标记）不变**。
- **manager 逐行核实**：改动全部为改名与登记，25 增/10 删。**冻结内容逐字未动**——§1 分子细则 M1–M8、§2 固定终点与末 1s 窗、§4 状态分类与判定顺序、§8 九项不涉及项、§10 十条自检反例全部无 diff；§7 的 N1–N9/N11 判定要求无 diff（仅 N10 字段引用随更名同步）。
- **旧名残留 3 处已核实合规**：均在 §13 追加记录内（:380 的改名对照说明、:382 的「逐字未改动」声明），属历史变更描述而非生效字段名；正文与 §6.2/§11/§14 中无旧名字段引用。`docs/test-cases.md` 旧名计数 0、新名计数 5。
- **字段名一致性现状**：tester 稿已完成对齐（ALIGNED），architect 稿 R1–R3 仍在进行（卡 6adc85fc…1cb）——architect 完成后两稿即完全一致，developer 实现卡（6adc6513…bdb0）按裁定 §1 唯一实现契约开工。

## 2026-10-24 18:20 四卡验收：s136真机 / D1-B / P0主体 / 字段revision（当前生效）

### 1. s136 真机验证 —— **验收通过**（卡关闭，s134 主任务可随之关闭）

交付：`docs/p012-s136fix-receipt-v1.md`（8061B，16:22）+ 6 块 hilog（`test/artifacts/p012-measurements/seg3-s136fix-{settle,tilt,heavy}-{off,toplight}-hilog.txt`）+ 4 张截屏 + 签名 HAP（`entry-default-s136fix-signed.hap`）+ 工具 `tools/analyze_p012_s136fix_visual.py`。

**manager 独立复跑全部六块**（`tools/seg3_quick.py`，与回执逐字一致）：

| 场景/臂 | n | phys P50/P95 | render P50/P95 | bake 帧(>50ms) | 吞吐 |
|---|---|---|---|---|---|
| settle/off | 1203 | 13/17 | 6/27 | 1 @206ms | 34.4 fps |
| settle/toplight | 1225 | 12/17 | 6/27 | 1 @214ms | 35.1 fps |
| tilt/off | 1059 | 12/17 | 6/27 | 1 @206ms | 35.4 fps |
| tilt/toplight | 1067 | 12/17 | 6/27 | 1 @208ms | 35.7 fps |
| heavy/off | 1216 | 12/17 | 6/27 | 1 @206ms | 35.9 fps |
| heavy/toplight | 1218 | 12/17 | 6/27 | 1 @213ms | 35.9 fps |

- **完成条件「settle ≥30fps」达成**（34.4fps，目标 30）。同温对照：render P95 133–138ms → 27ms（降幅 ~80%）、bake 帧频率 28–65% → 0.08%（仅首帧初始化，稳态零 dirty）、吞吐 9.6–17.9 → 34–36fps（2.0–3.6×）。
- 物理层未动（phys P95 17ms 与基线一致）→ 增益纯来自渲染修复，守恒与静止零抖动等物理验收不受影响。
- 视觉连续性：雾带顶边界四图一致（42.0/42.0/42.0/41.8%）、空气色 (16,16,29) 相同、绿沙带两版均存在；水体均色差 ≤10 RGB 属模拟进度差异（post 版快 3.6×），非渲染回归。
- **真机数据可信度已恢复** → 解锁 E2 出数 / 掉沙 V1-V4 重录 / 参数面板 / S3-S4 真机各链；s134 主任务（6adb3508，收口载体）可关闭。

### 2. D1 B 段 —— **部分通过，任务不关闭**（隔离原型已成型，但有 1 项真实缺陷 + 2 项自陈缺口）

交付：commit `b9b3884`（7 文件 +1200/−5：`P011ParticleField.ets` +416、`P011ParticlePrototype.ets` +403 新建、单测 +312、ohosTest smoke +34、dev-report 28 行、feedback 12 行）+ `docs/dev-report-P011-D1-B-v1.md` + `docs/feedback-P011-D1-refund-adapter-v1.md`。

**manager 实测复现：粒子单测 32 PASS / 1 FAIL**（与开发报告自报一致，未删除或放宽该断言）。既有通道回归 275/0（gesture 13/planning 106/S3 14/S4 10/input 124/advection 8），合计 307/1。

- **★唯一 FAIL 是真实接口缺陷，manager 已独立复核源码确认**：用例「B budget stub matches actual clipSubstepMotion and refunds across local/scale/total grid」失败（断言差异见 `run_p011_particle_field_units.cjs:431`）。developer 的主张**逐条属实**——`clipSubstepMotion`（`WaterSandField.ets:3548-3566`）对两端 local **各扣 `allowed`**（:3560/:3562），而 `P011GuardLedgerRefund.refundLocal`（`P011MaterialGuard.ets:214-232`）先 `share = amount / columns.length` **平分**、各退 m/2（:221）→ 跨列部分授予时按 rev1「双端各退实际扣额、total 只退一次」调用旧退款函数会留下 local 净扣。定性为**旧退款接口与 rev1 契约不兼容**（非物理失恒、非阈值问题）。
- developer's 建议正确且不越界：新建 D2 粒子票据 adapter 按各端实际扣额退款、total 仅退一次，**旧通道行为保持不变**；不得简单复用双端列表原样调旧 `materialGuardRefund`，不得对 total 双退。
- developer 自陈缺口（如实登记，不掩盖）：①prepare/collect/snapshot 仍用 scratch 对象与数组分配，**未达原稿 §4.4 生产热路径零分配目标**，合并候选为有界池内搜索 → 不宣称性能达标；②真实 λ/η/sourceEpoch、WaterSandField 的 guard/床层/susp/route/preview 适配与 checker v7 属 D2；③ohosTest 全量构建有 **175 个既有测试目标错误**（HeightFieldTest/WaterSandFieldTest/P012FrameContext 等，本卡文件已无错）→ 不宣称 ohosTest HAP 构建成功；④默认 HAP `assembleHap` BUILD SUCCESSFUL（18.607s，零错误）、TS strict/noEmit 通过。
- 裁定：**任务不关闭**（完成条件含「§13.6 各组验证全绿」，现 1 FAIL）。剩余范围=①退款 adapter 契约（待 architect/manager 明确接法后由 developer 实现）②生产热路径零分配收口 ③D2 真实接线。

### 3. P0 主体实现 —— **验收通过，任务关闭**（P0 主体里程碑达成）

交付（**工作区未提交态**，9 个新文件 + 4 个改动文件）：`entry/src/main/cpp/p013renderer/`（egl_context.cpp 7.4K/napi_bridge.cpp 24.5K/shader_pass.cpp 4.1K/tdata_uploader.cpp 1.7K + 3 头 + render_contract.h + glsl/e2_lmat.{vert,frag}）、`entry/src/main/ets/render/P013{FrameAdapter,RenderController,RenderTypes}.ets`、页面接线（P013HelloSurface.ets/SandCanvas.ets）、`test/run_p013_p0_units.cjs`（18 例）+ `test/p013_native/` + `test/p013_build_manifest.py` + `.codex/local-test/` 4 件产物。

**manager 实测**：P0 单测 **18/0 全绿**（逐例 PASS：profiles 与 Canvas DISPLAY_4X3_V1 逐通道逐列一致 / 非法输入不改可复用 2048B 包 / 非零 byteOffset 精确带 512 float / 平沙 fixture 中性参考且 mask 外无沙 / 180° 与非全屏 box 映射同一物质点 / 深度跨 H/L/M 趋近 1 且无物质边界跳变 / 镜像光向与双坡交换符号 / 空沙与单列边坡保持采样器 mask 连续 / controller 基线不调 bridge 且 overlay 单缓冲新 ID / ON→OFF 先提交透明清再移除层 / swap 失败保留故障遥测并移除诊断层 / 陈旧几何隐藏旧热图且消耗 frame ID 后重试新 ID / 后台门控与 resume 复用 owner 且 dispose 幂等 / 同 native session 重挂载 ID 递增且 canonical evidence 重置模式 / controller ring 有界并报丢帧且排出不可变副本 / 不可恢复异常不逃逸进 Canvas ticker / 最近秩汇总对不可用指标保持 null / native 遥测暂存有界且合并 native 与 controller 丢失）；P-012 既有证据臂回归 **82/0**；native host 测试双 PASS（lifecycle 八种部分初始化故障/权威尺寸/后台/陈旧回调/swap 与 context loss/有界恢复/重复 release/帧契约 + bridge 实际路径 offset Float32Array·长度与 detach·NaN·版本·陈旧拒绝·已消费 ID·错误 owner·无半上传·透明 OFF·swap 失败·ring 丢帧·release 重初始化）；构建 `assembleHap` BUILD SUCCESSFUL（9.156s）；CMake 已加 `p013renderer` 共享库目标 + GLSL 字节嵌入与 SHA256（`kP013ShaderHash`，构建源表可核验）+ 五库实名链接。

红线核验：未引入调试开关/临时门进主干（页面改动仅 `P013Mode` 四模式 + 生命周期 setActive/release，无 test/probe 分支）；未动 P-012 Canvas2D 链路；未改 §9 三版本号与 §5.1 T_data 布局；未改物理。

遗留登记（不阻塞本卡）：①**全部改动未入 git**，按 `.codex/PIPELINE.md` 由 reviewer 提交环节按完整路径入库；②P0a 交接项「旋转 viewport 真机手动复核」随 P-013 后续真机窗口做（本卡不用真机）；③P0 主体里程碑达成 → 后续 E2 诊断 pass/合成链排期可启动。

### 4. endpoint 字段 revision（architect）—— **返修未完成，任务保持开启**

`docs/architecture-P011-endpoint-fields-revision-v1.md`（26442B，16:24）内容质量已在上轮验收认可，但 **R1–R3 对齐修正尚未执行**：§1 仍写「工作区尚未发现 tester 稿落盘」「`endpointProtocolId/endpointProtocolRevision` 在对齐前必须为 null」「字段名对齐状态不得写 ALIGNED」（:17/:26/:29），§7 字段对齐表末行仍为「待 tester 文档发布后逐字段比对，当前不登记 ALIGNED」（:221）——而 tester 稿已于 16:26 落盘（commit 69e27b9），对齐裁定已于 18:05 出（`docs/decision-P011-endpoint-field-alignment-v1.md`）。

同时核实：tester 稿 T1–T3 **也尚未执行**（`docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` 中 `context.phaseOccurrenceId` 计数已为 0、`legacySuspOnly`/`wsMetricSuspensionFractionLive` 计数亦为 0 → 改名已完成，仅剩 §13 追加记录待确认）。

→ 两卡 R1–R3 / T1–T3 完成后，字段名即与对齐裁定 §1 一致，developer 实现卡（6adc6513…bdb0）前置满足。

## 2026-10-24 18:05 终点指标 protocol + 字段 revision 验收（当前生效）

1. **tester endpoint protocol revision 验收通过**（卡 6adc64e7…bc1，tester 16:29 自标完成，commit 69e27b9，2 文件 +427 行）：交付 `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（368 行，protocolId=p011-endpoint-terminal-nondeposited / protocolRevision=endpoint-terminal-nondeposited-v1 / endpointMetricRevision=terminal-nondeposited-v1.1 / endpointAuthority=APPROVED，METHOD_FROZEN / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_KALIBER_MANDATORY）+ `docs/test-cases.md` §14（TC-P011-ENDPOINT-01..08，MUST-PASS 7 + TIERED 1，逐例带缺字段路径）。
   - manager 复核：§1 分子细则 M1–M8（state=2 计入 / state=0 不计 / DEGRADE·TO_SUSP 累计量不入 / MERGE 只计现存包一次 / LAND 只扣 granted / B tracer 不计 / 摘要与逐粒不得同加）+ 守恒先决 ≤1e-9；§1.2 分母各自历史分母（SLIP R_SLIP / WS E）；§1.3 阈值 5% + epsMass=1e-9；§2 固定终点与末 1s 每子步窗（含窗口资格/覆盖不足/不外推不插值）；§3 采样身份与单调 elapsed 时钟；§4 判定顺序与六码状态分类；§5 双算与报告身份；§7 checker 适配 N1–N11（含双窗并存：坡度稳定样本选择不得改）；§8 九项不涉及项；§10 十条构造反例——**逐条正确**。
   - manager 复核实源码主张全部属实：`getTotals()` = `bh+mh+suspH`/`bl+ml+suspL`（:1936-1947，**不含粒子桶**）；`getMetrics().suspensionFraction = suspended/max(totals.heavy+totals.light,EPS)`（:3388/:3400）；`updateMetrics()` 内另有 `suspended/max(initialHeavy+initialLight,EPS)`（:6006，仅 dormant 用、未发射）；`nextP011TimeMs()` 为 Date.now() 递增（:1919）；checker 现按 `context.phaseD` 分组（:213，同名重复相位会合并）、`initial_deposit_total`=首条 `before.bh+before.bl`（:228-229，即 R_SLIP）、末 1s 窗按 `physicsTime`（:345-346）、`EPS=1e-9`（:58）——**tester 对 checker 的适配说明全部对应真实现状**。
2. **architect 字段 revision 验收：需修正后通过**（卡 6adc64ff…b44，未关卡）：交付 `docs/architecture-P011-endpoint-fields-revision-v1.md`（226 行，未提交态）。内容质量高：post-commit 采样身份/原子采样接口（`P011EndpointClock`/`P011EndpointCommitKey`/`P011EndpointPhaseIdentity`/`P011EndpointPostCommitStamp` 四个 TS 接口）、采样点时序风险、`endpointSampleElapsedMs` 与 `integratedPhysicsSeconds` 契约、`particles.byState` 滞留诊断两级结论、三分项与双算定义、完整报告字段清单（8 组）、实现面/风险/验收准备清单。
   - manager 复核属实：①`finishNaturalObservation(null)` 在 :1902 成功分支，**早于** `updateMetrics`（:1918）与 `p011SubstepId += 1`（:1921）——「采样点须绑成功封装点、只增新 ordinal 一次、不把旧 substepId 与下一帧 particles 串接」的风险描述成立；②失败分支 `finishNaturalObservation(error)`（:1886）先于 restoreBackup——失败 attempt 不得进 endpoint 窗但须留账做硬闸门审计；③SDK `systemDateTime.getUptime(timeType: TimeType, isNanoseconds?: boolean): number` 与 `TimeType.STARTUP = 0` **在本机 SDK 26.0.0 实测存在**（`ets/api/@ohos.systemDateTime.d.ts:256`/`:231`）——§3.1 时钟依据成立。
   - **两稿字段名不一致（正是本卡完成条件要求的一致项）**：相位身份 tester=`context.phaseOccurrenceId` vs architect=`context.endpointPhaseOccurrenceId`；legacy 双算 tester=`legacySuspOnly.recomputed`+`wsMetricSuspensionFractionLive` vs architect=`legacyEndpointFraction`+`legacyMetricSuspensionFraction`。**语义相同、仅命名不同**。
3. **★manager 已出对齐裁定**（权威载体 `docs/decision-P011-endpoint-field-alignment-v1.md`）：字段名唯一落点 = architect 名（`context.endpointPhaseOccurrenceId`、`legacyEndpointFraction`=U/R、`legacyMetricSuspensionFraction`=U/max(A,EPS)）；tester 侧 T1–T3 改名（只许改名与登记，§1/§2/§4/§7/§8/§10 逐字不动）；architect 侧 R1–R3（补 protocolId/protocolRevision + alignment 改 ALIGNED + 报告清单补双算名）。**tester §5.3 请求的历史分母结论：manager 采纳 tester 方案**——C run 中两个 legacy 量必然不同（引入第三桶的必然结果，非双算失败），禁止互为别名、禁止任一单独放行、报告分别标注。
4. **两卡已重写正文**（含对齐修正项与裁定文件路径）；**developer 实现卡 6adc6513…bdb0 的前置状态**：两版 revision 均已交付，字段名经裁定统一后即一致 → 两卡完成 T1–T3 / R1–R3 后 developer 可开工（**不得自行选名**）。
5. **P0a 真机确认（6adc4394…609d）已于 15:25 验收通过并关闭**（EGL GLES3 上下文成功、稳态 59.9fps、前后台 10× RSS 无泄漏；结构性结论：hello_surface.cpp 对 P012/物理零引用，与 s134/s136 回归链独立），本轮无新增交付，故不重复验收；真机已从队列第 2 位释放。

## 2026-10-23 P-003/P-005 分阶段范围重排（当前生效）

> 用户裁决：场景二先做“水 + 一种沙”，当前迭代不做重/轻双沙分选和三相模型。G2 的完整三相目标保留为后续阶段，不代表最终验收范围取消。本条覆盖下方旧 P-003/P-005 排期中“直接做三相”的当前执行范围。

1. **第一阶段范围**：P-003 先实现水与一种沙。水和沙的总量分别守恒；同一种沙在沉积区/悬浮态之间的转移仍属于沙相内部状态，不得被计成第三相。先验证单沙沉降、堆积/安息角、水面、动态倾斜和摇晃后恢复；不做轻/重沙间分选或相间渗透。
2. **测试先行**：P-005 tester + effector 的本轮用例、RED 基线和报告均聚焦单沙+水。既有三相分选、重/轻比例、percolationRate 等用例/方案留档，不在本轮实现或判 PASS；只标记为后续阶段。
3. **顺序与依赖**：P-005 单沙用例就位 → P-003 designer 重设单沙算法/架构 → developer 实现并经 reviewer 自动审核 → tester/effector 独立验证 → designer 判定。P-009 快照链路可并行；P-011 D-1 修复独立排队但不改变本阶段物理范围。单沙阶段验收后再由 manager 决定何时恢复双沙/三相与 P-008。
4. **不变口径**：不修改 `docs/goals.md` 中 G2 的最终三相目标，不改守恒、安全、有限值、动态连续性及既有冻结门槛；本条只是分阶段开发次序，不把简化阶段结果写成 G2 完成。

## 2026-10-22 15:02 P-012 Q1 三分支记录联动（渲染线当前口径）

依据 [Q1 三分支记录](q1-three-branch-record-P012-v1.md)（manager 15:02 正式签发，用户15:00渲染哲学总纲）；designer按本次明确任务授权登记下游联动，不新增裁决。

1. **Q1=PENDING / HF_COMPARABILITY_REVIEW_PENDING / NON_BLOCKING**：采纳分支③，独立轨道，不阻塞阶段二。EAB display hfH=4.3932与参考静态描述带[1.3325,1.4673]的差异登记为“**风格参数差异候选**”，不登记成待修复缺陷，不设“修到参考带内”的收敛目标；参考带保留描述性语义，不是验收线。
2. **历史验收不回退**：第一梯队E/A/B维持VALIDATED/CLOSED，SV-06=3维持合格。Q1未关闭；G2–G5补证用于理解尺度/编码/对比度的参数贡献，不用于反向调整口径。旧记录中收敛目标的解释以本条及正式签发记录为准，历史数值保留。
3. **渲染参数可调化候选已登记，P4**：[参数域与锚点候选](algorithm-P012-appearance-controls-candidates-v1.md)，`DESIGN_ONLY / ANCHOR_PROPOSAL / AFTER_E3_VALIDATION`。覆盖颗粒粗细/对比、雾浓度、层理疏密/强度、清水斑驳；目标为同风格真实感＋可调性。范围和验证锚点尚未冻结，不代表实现或PASS。
4. **排期：E3优先，参数可调化在E3验证后**。C/D闭环作为本次任务给定前提；参数域、锚点适用域revision及后续实现由manager另行验收/排期。本条不授权采集，不改变物理安全门槛、评分量表或冻结附表。

## 2026-10-22 11:17 🎯 P-011 阶段一 CLOSED（用户宣布，manager 执行登记）

> 依据：2026-10-22 10:58 配置机制回执完成（五项证据清单齐备——执行链回执/配置机制回执/安全回归/状态路线/B-C 维持）+ 用户 11:17 宣布阶段一 CLOSED，配套批准标定/标定预备/1B 影子审计三任务 DEFERRED 挂起。本条为阶段一收尾登记；不等于 P-011 整体完成。

1. **阶段一 CLOSED 生效**：A（执行链验证完成+参数机制就绪，per 2026-10-21 23:06 修订标准）+ B/C（2026-10-20 阶段边界及后续正式口径）全部满足，已解决硬/软门槛不退化。证据锚点：扩批 34 runs 全 PILOT_REPORT（9d20488）/参数外置实现 10e4838（u27 13 用例）/配置机制回执 receipt-v1（117/117+planning 100/100+D-1 链 PASS）。
2. **观测③标定登记**：`DEFERRED / NOT_CALIBRATED / NON_BLOCKING_FOR_STAGE1`——估计/CI 保持 null；推荐值 26/20 不写成标定结论；标定协议/34run 统计门槛/资格缺陷登记全部保留，未来预设场景包重新立项（标定脚本已预备可直接复用）。
3. **DEFERRED 任务**：①标定双角实值（6ad5fae3）②标定预备（6ad8c628）③1B 影子审计设计（6ad977db）——三任务挂起，随未来场景包启用复活。
4. **±45° 目标包**：30°±2°/target-based t90≤5100ms/R²≥0.90 与适用终点稳定/悬沙约束继续阶段二 DEFERRED；安全不变量（守恒/库存/容量/预算/有限值）不延期。
5. **阶段二衔接**：correction A/B/C/D 矩阵（预注册 PREREGISTRATION_PENDING_MANAGER，h 一致性前置已修 94041d2）/方案 3 可达性审计/变体泛化策略（§10 已备）按阶段二排期。渲染线按15:02签发记录维持Q1=PENDING非阻断、风格参数差异候选；E3优先验证，渲染参数可调化候选排其后，F1归因独立节奏继续。

## 2026-10-21 23:06 P-011 路径5：参数外置与阶段一关闭条件修订

> 依据：用户明确裁决“先不针对场景标定，把起停角做成用户可调参数，继续推进后面工作”。designer按本次任务授权登记范围调整；[双角用户配置设计](algorithm-P011-user-threshold-config-v1.md)待manager验收，参数机制尚未实现/验证。本条覆盖旧阶段一·A“必须完成双角统计估计”的依赖，不改历史数据结论。

1. **路径选择**：起停角转为显式用户配置，出厂推荐26°/20°，标注“非标定值，用户可调”。配置安全域沿用现有 `0≤stop<start≤30` 校验，非法拒绝而非clamp。tan30、planner、全writer guard、冻结预算、守恒/库存/容量/有限值门槛全保留。用户调参不默认激活correction。
2. **阶段一·A新MUST-PASS**：观测③的**执行链验证完成＋参数机制就绪**。保留真实状态机/边身份/限流不误报stop/账本对齐回执；补齐configure成对校验、配置原子生效与生命周期、来源和configured/executed一致性、UI/持久化/TEST与EXPERIENCE、路线隔离与安全回归证据。既有candidate读取成功不能替代新增产品机制验收。
3. **场景标定状态**：`DEFERRED / NOT_CALIBRATED / NON_BLOCKING_FOR_STAGE1`。34run等统计门槛不再阻断当前阶段一关闭；原协议、搜索窗、样本/CI门槛和不足报告保留，未来预设场景包可重新立项。估计/CI仍为null，推荐值不能写成标定结论，旧14事件不追认通过。
4. **总关闭条件**：A按本条、B/C按2026-10-20阶段边界及后续正式口径分别验收，已解决硬/软门槛不退化，manager再评估**阶段一CLOSED**。当前登记为待设计验收/实现验证，不立即CLOSED；不等于P-011整体完成。±45°全宽30°±2°/target-based t90/R²与适用终点目标包继续阶段二DEFERRED，所有安全不变量不延期。
5. **后续工作**：暂停以“补齐本批双角标定”为目的的1B扩采/变体逐场景标定排期；1A已发现的资格缺陷保留登记，不追认旧统计结果。correction A/B/C/D统一固定26°/20°推荐配置、同来源/参数hash，A为双角下原relax；不与legacy单角混作收益基线。correction保留独立工程/遥测、正式对照和manager是否转正的依赖，本条不授权采集或默认转正。
6. **路由**：manager验收设计→architect细化configure/模式/来源/生命周期/UI接口→tester登记执行验证→developer实现→tester/effector回执→manager关闭评估。其余阶段二全通道可达性审计顺序不变。

## 2026-10-21 P-012 第一梯队 E/A/B 交付闭环（当前生效）

> 依据：本次用户转达的 manager 已验收裁决；[designer 正式判定 §6](retro-P012-first-tier-signoff-v1.md)、[S1 正式 SV 回执](sv-receipt-P012-S1-v1.md)、[Q2 全锚点映射表](anchor-mapping-P012-v1.md)（两份回执已入库 `7a15067`），以及 [tester 独立复验](test-report-P012-v1.md)。本条由 designer 按本次明确授权追加回写。

| 交付/后续项 | 状态 | 范围与路由 |
|---|---|---|
| P-012 第一梯队 E/A/B，`p012-render-material-v1` watersand | **VALIDATED / CLOSED** | s99性能34–60fps通过既定窗口口径；S1正式SV=4/3/4；Q2映射收口。此前EAB性能退化`PERF_BLOCKED`解除 |
| C层理 / D流痕 | **UNLOCKED / 待 manager 调度 / NOT_VALIDATED** | 第一梯队依赖解除，后续依C→D顺序及各自验收排期；本条不是已实现或通过声明 |
| Q1 hfH参考带可比性 | **HF_COMPARABILITY_REVIEW_PENDING / NON_BLOCKING_FOR_EAB_SIGNOFF** | 独立轨道，由manager路由tester/effector按附表§5处理；不撤销1–2参考带，不记Q1已关闭 |
| p012ablate/p012fillbox | **OPEN / NON_BLOCKING** | developer诊断工具后续项、tester交付后补跑；不影响本次交付闭环 |

关闭范围仅为第一梯队E/A/B，不将P-012整体、C/D、hybrid或P-011写成CLOSED。Q2确认 `<8` 为实现侧段阶梯口径，冻结验收采用逐像素 `≤30量级`，16.3满足后者；不将16.3写为通过<8，不改评分或附表。原复盘§3“三项齐备”依赖按本次裁决更新为S1/Q2关闭、Q1独立非阻断；历史判定保留。SV-06的3分仍为可接受档，雾带块状提升属于后续优化，已通过的门槛转入维持项。

## 2026-10-21 P-012 渲染现状 vs G2 差距排期建议（当前建议）

> 依据：`docs/gap.md`「渲染现状 vs G2 目标差距（2026-10-21）」及 `docs/goals.md` G2.7。仅盘点和排期建议，不启动实现；效果优先于效率。

| 顺序 | 项目 | 建议排期 / 路由 | 状态边界 |
|---|---|---|---|
| 1 | C 层理 | 作为下一档效果工作先行；designer 明确可见效果与验收锚点，再进入后续实现、effector 验证。 | `UNLOCKED / NOT_VALIDATED`，未验收前不写成完成。 |
| 2 | D 流痕 | 排在 C 之后；先完成动态流痕效果定义，再接续验证。 | `UNLOCKED / NOT_VALIDATED`；遵守 C→D。 |
| 3 | 顶光立体感 | 作为新的效果维度开题，先定义受光/背光表现与评测指标；与 C/D 协调，不挤占已关闭项的验收结论。 | 未开题；尚无验收状态。 |
| 4 | 雾带连续悬浮感 | 作为 SV-06 由 3 向 4 的后续提质候选，效果与帧时一起评估。 | SV-06=3 仍是可接受且已通过；不回开 E/A/B。 |
| 5 | 沙丘形态 / 画面占比 | 先协调场景初态及物理分选测量，并与 P-011 观测③标定链对齐后再排效果复核。 | 属目标差距，不改变 P-011 已裁决状态或口径。 |
| 6 | `fpsP95≥100` 归因 | 效果工作之后安排 developer 归因；核对 G2.7 窗口口径、Canvas2D 逐帧绘制瓶颈及可能的预合成/硬件加速方向。归因报告后由 manager 决定是否立项或升级为 P-013 级议题。 | 仅建议排位；当前 34–60fps 与目标约差 2–3 倍。可达性未定，不启动实现。 |

本次排期建议不改变任何已冻结口径或已 CLOSED 项状态；100fps 的架构归因是待排路由，不代表已认定必须改架构或必然可达。

## 2026-10-21 P-011 D-1 correction 越权激活裁决（当前生效）

> 日期：2026-10-21。依据 `docs/architecture-P011.md` §26.2、§24.2–24.3，以及 v3 preregistration 的路线身份定义。裁决选择 **其他：Option A 加显式路线隔离**。

1. **LEGACY 位精确恢复**：developer 为 correction apply 加资格门；非候选 LEGACY 路径必须恢复旧 `relaxAngle` apply 语义，按 architect 建议用 `relaxHysteresisThresholds !== null` 排除普通 LEGACY。需补跨版本多步轨迹回归，覆盖 step25 首个分歧点、step214 BLOCKED 差异及完整 240-step 锚点序列；身份字段测试不能替代行为等价证据。
2. **显式隔离 correction 路线**：`relaxHysteresisThresholds !== null` 是 correction 候选运行的必要条件，但不是充分条件。只有显式启用并记录 `observationRouteId=relax-correction-weighted-v1` 的 correction 实验 run 才可进入 `applyCorrectionSlot`；`natural-relax-hysteresis-v1`、SLIP 正式验证、观测①b 和所有 LEGACY/默认 run 继续旧 apply。不得把 v3 状态机身份作为 correction 的隐式开关。correction 路线仍是候选，未完成 §24.3 A/B/C/D 对照矩阵和效果报告、未经理裁决前不转正、不默认启用。
3. **D-1b 遥测**：`correctionRequested/Granted/Rejected/StalledCount` 缺项不阻断恢复 LEGACY，也不阻断隔离后的 v3 身份核验；但在 correction 路线的对照 run 前，必须补进 audit/ledger/checker 并验证账本闭合，否则 correction 效果报告不具备放行资格。
4. **复核顺序与路由**：developer 先实现 LEGACY 回退 + 显式 route gate，并补轨迹等价与 correction/非 correction 分流测试；architect 复核路线身份和 apply 落点；tester 在修复构建上重跑 legacy 轨迹及 v3 核验（原 6ad709d6 核验对象不得直接沿用为修复后结论）。D-1 修复通过后可解锁 v3 tester 链路；correction 路线另外按 §24.3 RED 对照用例 → 工程/遥测 → A/B/C/D 实验 → 效果报告 → manager 是否转正的顺序执行。
5. **状态**：D-1 `OPEN / BLOCKING_FOR_V3_TESTER_SIGNOFF`，直至行为轨迹等价证据通过；D-1b `OPEN / BLOCKING_FOR_CORRECTION_VALIDATION`；correction 路线 `CANDIDATE / NOT_PROMOTED`。本裁决不追认已经发生的 LEGACY 行为差异，不改阈值、预算、安全 gate 或历史 run。

## 2026-10-20 P-011 观测③ v2 pilot 停机与 v3 识别方向批准（当前生效）

> 日期：2026-10-20。依据 `docs/effect-report-P011-obs3v2-pilot-run1.md`（v2 clean pilot 报告）与 `docs/observation3-v3-identification-redesign-v1.md`（designer 识别策略重论证）。v2 pilot（run `p011-run-1792472946390-3`，构建 891370d）三维判定：clean=INVALID（5 条边 tan30 违约，量级 1.25e-7 浮点 epsilon，seq2020 回摆段，非豁免相位 → 按预注册稿 §6 隔离整 run）、platformReachability=PARTIAL（±15° 恰 15.0000° 极稳 PASS；±30° 每子步 `TILT_PLANNING_BLOCKED` 物理冻结 ~29.88°，完全不可达）、pilotCoverage=INCOMPLETE（四 cell 全 0 合格 bracket；459 个原始 start 全部发生于 ramp 段）。run 证据保留于 `test/artifacts/p011-obs3v2-20261020/run-001/`。

1. **v2 pilot 判定确认**：run-001 维持 INVALID，不进入 fit/holdout，不追认；观测③维持 `BLOCKED / DESIGN_PENDING_OBSERVATION`。effector 按执行单停止扩批、照实报告、未改 schedule/物理/用例——纪律执行合格。
2. **接受 v3 复核结论（根因层缺陷）**：designer 对照 source-manifest SHA-256 复核构建源码确认 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`——该构建 relaxAngle 实际由 `tan(angleDeg=4°)` 驱动，候选阈值 28°/22.911° 未进入状态机；ledger 复核 459 个原始 start 的 alpha 实测 4.0°~14.3°，全部为旧请求态诊断事件，非双阈值观测。v2 pilot 的"start/stop"数据不构成对任何候选阈值的检验。
3. **批准 v3 方向 2 进入新协议设计**：稳定平台 + 安全提交 ramp 联合激励。保持原搜索窗 `thetaStart∈[25°,30°)`、`thetaStop∈[15°,25°]` 与全部安全/统计门槛不变；新资格规则允许受控、完整提交、无 hold/回滚/裁剪歧义的 ramp 转折纳入标定（ramp leg 预注册、决策残差可得、bracket 宽 ≤0.5° 采样分辨率）；建议序列 `0→+15→+A→+15→0→−15→−A→−15→0`（峰值 A 由修复后 planner+guard 影子审计确定，不是固定 30°）。否决方向 1（滞后窗口不能合法制造 active）、方向 3（搜索窗迁移无证据支持，为 manager 保留事项）、方向 4 降为后备（当前无 stop 半份证据）。
   - **速率档口径**：0.5°/s 主档必须独立满足 34run/pair 及全部 cell 门槛；1.0°/s 敏感性档作为可行性审计项纳入 v3 协议，若要求两档各自独立确认则须各自足额（最低 68run/pair），本裁决不预授权任何扩批。
   - **估计对象声明（随协议冻结）**：v3 manifest 必须分列 `configuredThresholds` / `executedThresholds` 及状态机实现版本/代码 hash；给软件预设候选后重现该阈值转换只证明执行语义可复核，不得自证材料「天然阈值」或候选最优；候选优选仍按独立行为/效果判据与 holdout 验证。
   - **右删失新定义纳入 v3 协议**：仅当新 v3 存在完整合法风险暴露、alpha 上界受 tan30 安全约束截断且未穿过候选 start 时，方可记 `RIGHT_CENSORED_AT_TAN30`（附实际 alpha 上界、风险状态、planner 原因），不产生点估计，不将缺样本本身当右删失。
4. **结构性缺失预案**：v3 预演/试采必须输出可达覆盖矩阵（风险状态、alpha 范围、真事件端点、噪声层可定义性）。若某噪声子 cell 在现有机制下结构性缺失，交回 manager 裁决实验激励/验收适用范围；禁止默默合并 cell 或删除难采层。manager 届时裁决选项限于：调整实验激励、上报用户决策验收适用范围变更、或维持 INCOMPLETE 如实报告；不自动降门槛。
5. **执行身份整改路由（先于任何新 pilot）**：designer 冻结 v3 协议及候选估计对象（configured/executed thresholds 分列、decision 时点字段、transitionReason）→ architect 核对真正双角状态机、decision 时点与 planner 事务语义 → developer 补齐执行与遥测（相名映射九相 + ledger schema v3 增补 thetaAccepted/requestRejected/plannerHold，任务已建）→ tester 核验执行身份/完整性/新资格过滤 → effector 待 manager 新 pilot 批准。
   - **采集侧修复并行启动**：任务 6ad6fb88（九相映射 + schema v3 planner 字段）按任务卡「无论识别策略怎么改都得修」定性**立即执行**，不等待 architect 方案；其产出并入 developer 身份整改交付。v3 新序列（含 A 段）的相位映射增量在 designer 冻结协议后叠加，v2 九相向后兼容照做。
   - **新 pilot 批准前置**：须身份整改全链（designer→architect→developer）完成、tester 核验通过、v3 协议冻结且覆盖矩阵预演方案就位后，manager 另行单独批准；本裁决不授权任何新采集。
6. **非目标**：本裁决不放宽 tan30/planning 闸门、不移动搜索窗、不追认 v2 pilot 标定资格、不预断 v3 可辨识性。若 v3 预演显示原统计门槛在可达域内不可满足，交 manager 另行裁决，不自动降门槛。
7. **本轮调度指令（2026-10-20 下午启动）**：
   - **designer（立即，先行）**：输入 `docs/observation3-v3-identification-redesign-v1.md` §4（工程/观测契约草案）与 §5（统计门槛）。任务：将 §4 草案升格为正式 v3 协议（含 §4.1 身份字段、§4.2 ramp bracket 资格、§4.3 序列与速率档、§5 门槛与覆盖矩阵预演方案、估计对象声明与右删失新定义），冻结后输出候选估计对象清单；不实现代码、不授权采集。
   - **developer（并行，任务卡 6ad6fb88）**：输入任务卡与 `P011NaturalObservation.ets:261-286`、calibration checker 缺字段清单。任务：九相映射更新（protocolId 区分向后兼容）+ ledger schema v3 增补 thetaAccepted/requestRejected/plannerHold + 单测 + commit。完成标准按任务卡；不改物理、不改闸门。
   - **architect（等待 designer 协议冻结）**：核对真正双角状态机、decision 时点、planner 事务语义与 developer schema v3 字段的一致性。
   - **tester（等待 architect 核对）**：核验执行身份、相名/完整性、硬安全与新 ramp 资格过滤的 checker 落地。
   - **effector**：待 manager 新 pilot 批准，本轮无动作；继续停扩批纪律。

## 2026-10-20 P-011 观测③平台可达性矛盾裁决（历史；v2 pilot 结果已由上条更新）

> 日期：2026-10-20。依据 `docs/effect-report-P011-recalib3-gate-run1.md`。835e663 基线的阶段一 MUST-PASS 全部通过：164,084 条边 0 tan30 违约、A-1 对账 0 不匹配、账本对齐 0、边身份 0 错位；本次失败仅由 deferred SLIP-02 收敛包驱动。与此同时，4b1f635 planning pass 将运行时 `thetaAccepted` 限制在约 29.9°，冻结 ±45° schedule 因而没有 ±45° 稳定平台或有效 bracket。

1. **三选一裁决：选择方案 3——接受可达平台边界，重定义观测③为 ±15°/±30° 平台的双角阈值标定；±45° 行为和全局收敛目标继续留在阶段二。** 不采用标定 run 绕过 planning guard；阶段一安全 MUST-PASS、tan30 约束和阶段一 B 的实现边界不变。
2. **协议改版授权与前置**：现有 ±45° schedule 及其随机化层不再适用于新校准协议；effector 不得在冻结 schedule 上现场替换角度。designer §8.16 已完成边界论证：±15°不能单独激励 `thetaStart∈[25°,30°)`；±30°可能触及低段，而有效角约 29.9° 会使未触发的高端候选右删失。manager 复核接受这一**显式识别边界**，不要求把右删失伪装成完整可辨识；允许进入新 protocol 的单 run clean pilot，以测实际平台/bracket yield。凡被 29.9° ceiling 右删失的候选必须标记 `RIGHT_CENSORED_AT_TAN30`，不产出点估计、不算校准通过。effector 建立全新 protocol/schedule 版本并重新预注册；不得沿用旧版 34-run 分层/随机化 manifest。
3. **tester 闸门**：新 schedule 预注册后，tester 同步 checker、test-cases 和阶段标签。校准 run 的资格检查仍执行 tan30 hard gate、budget、conservation、stock、capacity、finite、telemetry alignment、edge identity、stop semantics 等全部 MUST-PASS；不设 capture-mode 豁免。checker 必须明确识别可达平台上限、右删失、有效 bracket 与“真实 stop vs planner hold/限流”。平台不稳定或无可用 bracket 的 run 不能进入 fit/holdout，保留为 pilot/invalid evidence 并说明原因。
4. **执行顺序**：designer §8.16 论证已由 manager 复核，允许继续协议化；effector 发布新 protocol/schedule 并重新预注册 → tester checker/cases 就位 → 仅执行单 run clean pilot。clean pilot 只评估可达平台和 bracket yield；manager 审核 pilot 结果后再决定是否扩充 fit/holdout，绝不自动开跑 34-run 批次。旧 `0/+15/0/+45/0/-45/0` protocol 的 run-001 证据保留、不计入新 protocol 的 fit/holdout；不得把历史无平台 run 追认为有效。
5. **Blocked 状态与阶段边界**：观测③整体仍 `BLOCKED（尚无合格 bracket/阈值估计）`；旧协议停止扩批，新协议仅获 clean pilot 授权，fit/holdout 扩批仍冻结。835e663 阶段一安全验收记录为 PASS，不能据此声称阈值标定完成。SLIP-02 ±45° 收敛目标仍为 `DEFERRED / NOT VALIDATED`，阶段二再做全通道可达性审计；45° 目标不进入本次改版标定 schedule。
6. **不变量**：禁止放宽 tan30、取消 planning 闸门、修改 `thetaAccepted` 安全语义、在 calibration capture mode 下绕过产品保护，或将硬闸门豁免外推到运行时材料提交。任何后续安全判据修改须另行 manager 裁决。

## 2026-10-20 P-011 阶段一·B architect CONDITIONAL_PASS 裁决（当前生效）

> 日期：2026-10-20。依据 `docs/architecture-P011.md` §22 对 commit `4b1f635` 的独立评审。七项 checklist 全 PASS；R-1 为逐 writer pre-commit guard 尚未交付，R-2 为两个 legacy 30° 用例 FAIL。

1. **B 实现状态**：接受【阶段一·B】实现完成（`CONDITIONAL_PASS`）。当前倾角可行域闸门、纯预演/apply 镜像一致性与每子步核验共同构成已交付保护；不得把 CONDITIONAL_PASS 扩写成全通道逐 writer guard 已完成或所有旧用例已通过。
2. **C-1｜立【阶段一·B·续】**：实现 §19.5 全通道 pre-commit 材料提交 guard，包括逐 writer 闭式拦截、违规态 non-worsening 处理和四层预算回写；同步完成 `hardGuardAvailable` telemetry 与 checker 逻辑。该任务与阶段一 A/C 验证并行排期；但必须在阶段一整体验收签字前交付、评审并纳入集成安全验证。若尚未完成，只可报告 B 当前 commit 的条件性实现状态，不得签阶段一整体完成。
3. **C-2｜选择维持文档化 FAIL 至阶段二**：`bottomCreep_tiltMovesLowerLayers` 与 `p011_commandSignFlip...` 作为旧引擎真实 tan30 违约被新闸门暴露的具名 legacy FAIL 保留并隔离记录，不归因于 4b1f635 新回归；当前不授权改变可行域/安全判据，也不授权重调测试参数或测试期望。它们不阻断本次 B 实现完成，但在阶段二由 tester/designer 结合全通道方案与新 guard 对这两个场景重新分类和验证前，不得删除白名单标记或声称解决。
4. **安全不变量**：维持 tan30、预算、几何及实际接受倾角语义；不得用 epsilon/margin 放宽边界。只有用于有限精度数值稳定且不改变判定阈值的实现容差，才可按现有设计契约另行审查。
5. **状态/路由**：B implementation `CONDITIONAL_PASS`；B·续 `OPEN / IN_PROGRESS`；两个 legacy FAIL `KNOWN_FAIL / DEFERRED_TO_PHASE_2`。developer 实现 B·续，architect 复审，tester 补齐直接 guard 与回写覆盖并重跑；与 A/C 验证并行但纳入阶段一总验收 gate。阶段二再复核两项 legacy 用例，未裁定前原始测试和 FAIL 证据完整保留。

## 2026-10-20 P-011 阶段边界复议裁决（当前生效，覆盖下文旧阶段一标准）

> 日期：2026-10-20。依据 designer `docs/review-basis-P011-stage-boundary-v1.md` 与 `docs/algorithm-P011.md` §8.15。裁决：接受设计复议建议。相邻输运必要条件只证明该路线在 5.1s、总预算 1530 下从平坦场最多形成约 0.501627° 全宽 OLS 斜率；不外推为所有含升降/悬沙方案无解。45° 参考下同向 30° 线性沙面本身满足 tan30 局部硬约束，停机报告把该几何关系解释为“目标本身不相容”有误；真正未证成的是相邻机制在时限/预算内达到全局目标的能力。

1. **A｜阶段一·A 双角滞回/状态机完成标准**：逐边 start/hold/stop 语义、真实 active 转换、限流不误报 stop、物理边身份和遥测对齐全部通过；对原始观测按既定 fit/holdout、CI、样本数门槛完成 `thetaStart/thetaStop` 估计，不填默认阈值。阶段一可报告实测阈值及其 CI；`thetaStop` 映射到 ±45° 全局目标窗口 `[20.563°,25.093°]` 的适配检查延期到阶段二目标包验收。标定③在样本与合格 run 不足期间继续 `BLOCKED`，不因阶段边界调整而伪称完成。
2. **B｜阶段一·B 安全限幅与提交 guard 完成标准**：每个提交子步按实际接受参考角检查全边 `|ΔS−tan(effectiveTiltAccepted)|≤tan30°`，零违约；只允许在完整事务可行时提交。若可行域为空或回退计划仍不合法，必须不提交并记录 `hold/BLOCKED` 原因、未接受角度与最紧约束边。预算、守恒、库存、容量和账本对齐保持现行 MUST-PASS。不得要求系统为证明安全而成功追随到 ±45°；无限 hold 也不算收敛或体验通过。
3. **C｜阶段一·C 现有机制效果完成标准**：分别验证（a）`±15°` 静止回归无新增运动/越界，（b）滑移方向、active/stop 及预算裁剪语义正确，（c）有可复核、用户可见的局部渐进斜坡响应。局部效果必须由 raster/录像可见证据支持，非零 grant 不算通过；报告全宽 OLS 坡度、R²、通量与 run 时间作诊断/能力记录。守恒、库存、容量、预算、±15°安全检查均不得退化。阶段一·C 不要求 ±45° 全宽坡面收敛到 30°±2°。
4. **阶段一·验证与总完成标准**：tester 对 A/B/C 各自出具独立结果；阶段一可完成的条件为 A、B、C 按上述边界通过，且逐边安全、有限值、守恒、库存、容量、总/local/bottom 预算、遥测/账本一致性全部 MUST-PASS。不得把阶段一标记为 P-011 CLOSED，也不得写成 SLIP 全 PASS。±45° 目标包仍保留为项目目标：`30°±2°`、target-based `t90≤5100ms`、`R²≥0.90` 联合验收；整个 SLIP-02 包（含适用的 ±45° 终点稳定/悬沙约束）移至阶段二可达性审计之后，不以 FAIL 阻断阶段一完成，但须持续列为 `DEFERRED / NOT VALIDATED`，原始测量继续记录。SLIP-03/04 中守恒/库存/容量/预算/有限值等安全不变量不延期。
5. **方案 3 与阶段二入口**：阶段二开端由 designer 先对全部相关通道做离线/账本驱动可达性审计，覆盖相邻输运、lift/settle、悬浮输运与库存变化，逐项结算表面一阶矩、共享预算、稳定终点和悬沙≤5%；先证明可达构造或必要条件，再决定是否进入方案 3 设计。方案 3 保持阶段二候选，不提前实现；本次也不预断它必然达标。阶段二审计不通过则保留目标未达成并报告冲突，不靠调参、放宽预算或改阈值放行。
6. **Blocked 状态**：解除“±45° 目标包未达成因此阻断阶段一”的阻断关系；SLIP-02 标记 `DEFERRED / NOT VALIDATED`，仍不是 PASS。`slip-overshoot-v2` 在相邻表面输运路线下仍为 `BLOCKED（条件性不可达）`；v2.14 限幅方案继续 `BLOCKED / DESIGN_NOT_CALIBRATED`，直到 omega/真机数据达到其自身解除条件。观测③保持 `BLOCKED（合格标定 run / 样本不足）`，旧 run-001/002 不追认。停机报告的 ±45° gate 豁免仍仅用于标定 run 资格；它不豁免阶段一实际提交的安全 guard。
7. **路由**：tester 更新 `test-cases.md` 与 checker，将阶段标签和 SLIP-02 目标包的适用阶段分开；保留全量测量，将 ±45° 目标包标为延期而非删除，并重测/更新报告。designer 维护算法文档中的复议状态，并在阶段二开端负责全通道可达性审计；审计通过后再交 architect 评审。developer 只按阶段一批准边界实现，不能将目标未达成伪装成通过。

## 2026-10-20 P-011 ±45° 补采 gate 几何冲突裁决（补采资格规则仍生效；阶段边界已由上条更新）

> 日期：2026-10-20。依据 `docs/effect-report-P011-recalib-gate-run1.md`（run-001/002 复测停机报告）、`docs/architecture-P011.md` §21、`docs/algorithm-P011.md` §8.13/§8.14。run-002 复现 run-001：修复构建 e2f02a1 的边身份错位为 0、限流误标 stop 为 0，但 45° 平台 hard-gate 系统性失败，且 0/454 子步合规；相邻滑移 5.1s 实测只实现 0.17°~0.45°，远低于 45° 平台 tan30 判据要求。此为验收与当前机制能力几何不相容，不归因于构建污染或已修复的 A/B 问题。

> 适用性更新（2026-10-20）：上方最新阶段边界裁决纠正了“30°线性坡面与45°参考几何不相容”的说法，并将全宽±45°目标包延期至阶段二。以下 tan30 形式 gate 豁免只作为补采 run 资格口径继续有效；它不定义阶段一运行时的提交安全规则。

1. **组合裁决：短期采用方案 1，中期按方案 2 重议目标，方案 3 仍留在阶段二排期。** 不提前启动悬浮平流实现；先让不受该冲突影响的标定转折数据继续采集，同时要求 designer 对机制能力上限和目标/机制路线提出量化复议依据。
2. **判据修订授权 tester 执行**：仅对 `+45°/-45°` 稳定平台及其 overshoot/return 子步，豁免 `|ΔS−tan(±45°)|≤tan30°` 这一参考线形式判定；保留 `0°/±15°` 以及非 45° 子步现有 tan30 判据。45° 子步继续执行并报告有限值、守恒、库存、容量、预算、字段/账本对齐、边身份和 stop 转换语义等其余闸门；不得将豁免描述为坡度达标、物理无越界或整体 run 自动合格。对补采资格而言，run 只在修订后的适用 hard-gate 全部通过后才可进入候选池；转折事件和 cell/bracket 仍按原 §8.11/补采执行单过滤、分组与样本门槛，原 run-001/002 不追认、不纳入 fit/holdout。
3. **明确非目标**：本裁决不改变 `30°±2°` 收敛目标、预算、几何、相位时长、转折阈值或样本配额；不声称现有相邻滑移机制能在 45° 达成 tan30 形式判据。若后续阶段一验收仍要求该机制实现不可能的 45° 参考线约束，designer 应按 §8.13/§8.14 提交机制能力上限与目标适配的正式重议，而不能靠再次补采解决。
4. **路由与前置**：
   - **tester（立即）**：同步修订 checker 与 `docs/test-cases.md`，以本裁决口径重测并更新报告；保留旧 run 与旧判定审计，不改物理、不删除其他 gate。新口径就位后再由 effector 按冻结 schedule 继续采集，仍先做单 run gate。
   - **designer（立即，机制/目标复议）**：依据 §8.13/§8.14 与停机报告量化说明 0.17°~0.45° 实测能力对 ±45° 目标、限幅+guard 与验收判据的影响，提出是否推迟目标及阶段一/二边界建议；方案 3 保持阶段二排期，除非后续正式复议另有裁决。
   - **developer WIP（下一次补采前置条件②）**：认领并说明主工作区两份 WIP 源文件 `P011TiltPlanningPass.ets`、`WaterSandField.ets` 的归属，整理为可审阅提交并修复当前 10 个 ArkTS 编译错误；提供成功构建证据。修复提交不得混入或覆盖 e2f02a1 补采基线；未满足前不得启动下一轮 gate run。
   - **effector**：本次停止扩批继续有效；待 tester 新 checker/cases 就位且 developer 前置条件②满足后，按同一冻结 schedule 与单 run gate 恢复采集。不得自行修改判据或 schedule。
5. **状态**：P-011 保持 `P0 / IN_PROGRESS`；观测③继续 `BLOCKED`，目前有效合格 run 为 `0/34`，本裁决不输出 `thetaStart/thetaStop`，不解除统计门槛。

## 2026-10-19 P-011 两阶段定案（历史；阶段一验收范围由 2026-10-20 阶段边界裁决更新）

> 日期：2026-10-19。原因：用户在 `slip-overshoot-v2` 条件性不可达结论（相邻输运 66 倍缺口）、方案 3（悬浮平流定向搬运，纯增加、理论出处 246/251/252 平流项）与「现有机制潜力未兑现」三者之间做出裁决：**先把当前方案（梯度驱动机制）能达到的效果做对，然后考虑引入方案 3**。本节确立两阶段结构，取代同日可达性复核裁决中的路线选择部分；其验收口径（硬约束/收敛/预算/账本对齐/OLS R²）全部保留并适用于阶段一。

1. **阶段一（当前执行）：现有梯度驱动机制的潜力全部兑现**。三项任务：
   - **A. 双角度滞回落地**（v2.12 `natural-relax-hysteresis-v1`）：逐边三态状态机 + 角度标定（依赖逐边转折观测，`thetaStart ∈ [25°,30°)` 严格排除上端点、`thetaStop` 由观测定）；
   - **B. 484 次越界修复**：tilt 参考更新动态可行域限幅（v2.14 设计）+ pre-commit admission guard（全通道 tan30° 提交前拦截）+ ledger 对齐修复（context 采样移至 updateTilt 之后）；
   - **C. 现有通道表层效果调对**：滑移收敛行为、安息角响应、±15° 静止回归。
2. **阶段一完成标准**：现有机制能产生的所有行为均正确——±15° 静止（已验证）、±45° 表层崩塌可见且渐进、守恒零漂移、127 边零越界、OLS R² ≥ 0.90、稳定终点悬沙 ≤ 5%。
3. **阶段二（评估后启动）：引入方案 3 悬浮平流定向搬运**。
   - **前提**：阶段一完成，且 `advectionGain=0` 的状态自然构成 A/B 对照基线；
   - **触发条件**：表层效果做对后，若「倾斜时沙向低处聚」的观感仍不足（缺失定向输运维度被实际确认）；
   - **实现形态**：纯新增平流项（漂移速度从 leveling 迁移场推导，量级由拖曳定标），与现有扩散项叠加；约 30 行新代码 + 1 个新参数，无 schema 变更、无现有代码改动、无用例修改；
   - **理论依据**：246 三相平流-扩散方程的平流项（深度平均降阶），非新物理。
4. **两阶段的衔接**：阶段一的观测数据（大角度崩塌行为、滑移速率-角度关系）恰好是阶段二平流项参数定标的输入；每阶段独立可验证，避免两改动混叠。
5. **流水线路由（阶段一）**：
   - **designer**：双角度标定所需的逐边转折观测协议补充 + tilt 限速方案的 architect 对接材料；
   - **tester**：P011-SLIP 用例固化 + 账本对齐核验（承接可达性复核裁决的任务）；
   - **architect**：待 designer（A+B 设计定稿）+ tester（用例就位）后进；
   - **developer**：等 architect + tester 用例，先 RED 后实现；
   - **体力驱动蠕动项**：保留为将来候选，登记不删，阶段二评估时一并考虑。
6. **不变量**：全部验收阈值不放宽、预算不提高、守恒审计照常、routeId 隔离、双报告独立成文；方案 3 引入时同样受「新增项自身守恒 + 可开关退化 + 不改现有通道」三重约束。

## 2026-10-19 P-011 slip-overshoot-v2 可达性复核裁决（历史；两阶段定案承接其验收口径）

> 日期：2026-10-19。原因：architect 复核确认 designer 的相邻输运下界与 raster 口径，并发现 tester 现行 `slope90TimeMs` 是相对本相位实测终值而非 30° 目标。manager 依此裁决路线、终点状态与正式验收口径。本节取代同日早前的上层沙调度口径，历史结论保留。

1. **可达性结论**：接受 `slip-overshoot-v2` 在当前 `maxAvalancheFlux=80/s`、`totalMotionBudget=300/s`、`5.1s` 时限与相邻输运路线下**条件性不可达**。全宽 OLS 斜率达 `30°` 需相邻输运总量至少 `100,893` units，`28°` 也需 `92,917`；对照 5.1s 总额度上限 `1,530`，缺口约 `66` 倍。中央单边需求 `>=794` units，也高于 5.1s 单边 cap `408`。该结论只否证相邻输运路线，不等于所有物理通道绝对无解。
2. **路线裁决**：维持 `30°±2°` 与 `5.1s`，不放宽目标、不增加预算、不改几何。`slip-overshoot-v2` 标记为 `BLOCKED（条件性不可达）`，本轮不交 architect/developer。授权 designer 对 `lift/settle/suspension-diffusion` 替代路线做**离线可达性审计**，只使用现有 `ledger.ndjson`，不写新物理、不改参数、不授权新观测。
3. **终点状态**：相位结束必须是**稳定/沉积完成**状态，不允许大量悬沙留在终点来制造瞬态 OLS 斜率。正式门槛：每个 `±45°` 相位最后 `1s` 的 OLS 斜率变化不超过 `1°`，且 `suspH+suspL <= 0.05 * 初始沉积沙总量`；若离线审计无法证明该稳定终点，不能放行。
4. **t90 口径**：正式 `t90` 必须是 **target-based**，即首次达到 30° 目标的 90%——约 `27°`，不是相对本相位实测终值的 90%。现有 `6.5s` 只能作为诊断，不得作为通过或失败结论。tester 须修正 checker 并同时保留 relative-final 值作诊断。
5. **raster 口径**：OLS 斜率仍是主判据，但不能只看斜率。正式验收须同时 gate 拟合优度 `R² >= 0.90`（128 列有效表面），并报告 RMSE；逐边 `tan30°` 硬约束仍独立生效。不得用局部噪声或瞬态悬沙拟合出斜率后放行。
6. **新观测授权**：本轮不授权新观测。仅当离线审计无法闭合时，manager 再决定是否授权一次**零物理改动**的 `lift/settle/suspension` 观测；该观测不得改参数、不得改物理、不得用于直接放行，只用于补足可达性证据。
7. **下一步流水线**：
   - **designer（立即）**：基于现有 `ledger.ndjson` 对 `lift/settle/suspension-diffusion` 做离线可达性审计，输出每通道的空间定向能力、库存/容量、总预算、稳定终点与 5.1s 内能否达到 `30°±2°` 的必要/充分界；不写新物理。
   - **tester（立即，与 designer 并行）**：修正 `slope90TimeMs` 为 target-based 口径，新增 OLS `R²/RMSE` 与终点稳定/悬沙阈值；用现有证据输出 RED 基线，不改物理。
   - **architect（等待 designer 审计）**：复核替代路线可达性与工程边界，确认是否值得进入新方案设计。
   - **developer（等待 architect + tester）**：在替代路线被接受且用例就位前不得实现；不得修参数或加预算。
   - 后续按 reviewer → tester → effector → designer 独立复盘闭环。

## 2026-10-19 P-011 上层沙优先与 slip-overshoot 违约裁决（历史；被同日可达性复核裁决取代）

> 日期：2026-10-19。原因：`gravity-advection-v2.11` 经数值复核被否决；新七相真机观测暴露 `484` 次 `tan30°` residual 越界、表层滑移未收敛到安息角，以及账本 `context/audit` 子步错位。用户已裁决先修上层沙，重沙机制押后。本节取代 2026-10-18 v2.9 的当前调度口径，历史结论保留。

1. **违约定性**：`484` 次 `audit.range=false` 是真实物理违约，不是单纯测量口径问题。独立复盘确认 `updateTilt()` 在 `substep()` 内先执行；首个 crossing 在任何 material grant 前，`effectiveTilt` 已推进到 `36°`，边 `c=57` 的 residual 已为 `-0.729661`，超过 `tan30°=0.577350`。后续 484 次失败集中在正向/负向 overshoot 及其 return 段，说明是持续的角度推进与材料响应失配，不是一次性数值抖动。
2. **口径缺陷**：首个 failure 的 `context effectiveTilt=24°` 与实际校验角度 `36°` 不一致，来源是 `context/stateBefore` 在 `substep()` 前采样，而 audit/grants 对应子步内更新后的状态。该缺陷影响事件标签与归因展示，不改变“真实违约”的结论；须由 tester 独立核验并修复观测对齐，不得用 `context=24°` 重新解释为通过。
3. **路由决定**：`P-011-A`（硬坡度违约 + 表层滑移收敛）直接路由 **designer** 修物理，不等 tester 先完成口径核查；**tester** 并行固化客观用例并修复/核验账本对齐。两条线可并行，但 developer 必须等 tester 用例就位且 architect 复审通过后才可实现。不得把本问题退回为“仅测量问题”。
4. **上层沙验收口径**：
   - 硬约束：同一七相序列 `0/+15/0/+45/0/-45/0` 下，每个提交子步的全部 `127` 条边均满足 `|S[c+1]-S[c]-tan(effectiveTilt_committed)| <= tan30°`，violation 数必须为 `0`；不得用事后修复或下一子步恢复放行。
   - 收敛：`+45` 与 `-45` 相位结束时，raster 可见线性坡度均须落在 `30° ± 2°`；从相位开始到 90% 收敛的 `t90 <= 5.1s`，`t50` 只作诊断记录。`±15` 无超角段保持回归，不得引入新违约。
   - 不变量：材料守恒、库存、容量、local/bottom/total 预算全部保持原阈值；不得增加预算、不得放宽 `tan30°`、不得预设目标形状或 ROI 偏置。
   - 观测：同一 ledger 记录必须同时给出 pre/post `effectiveTilt`、committed `sandTop` 与实际 audit residual，确保 `context` 与 audit/grants 属同一子步快照；不得覆盖旧证据。
   - 重沙：本轮不以重沙界面 `>=2px` 作为上层沙子目标的放行条件；该要求押后，只要求不引入守恒或预算回归。
5. **下一步流水线**：
   - **tester（立即，与 designer 并行）**：输入 `docs/observation-report-P011-slip-overshoot-v1.md`、`docs/retro-P011-v2.13.md`、`docs/algorithm-P011.md` §8.12~8.13。任务：独立核验 484 次违约的计算口径与 `updateTilt` 时序；把七相观测转正为 `P011-SLIP` 客观用例（硬约束、收敛、预算、守恒、账本对齐）；输出 RED 基线与正式报告，不改物理。
   - **designer（立即，与 tester 并行）**：完成 `slip-overshoot-v2` 的联合限幅设计，覆盖倾角参考速度、材料响应、pre-commit hard guard、共享预算竞争与观测对齐；不得重开 v2.11 重力交换，不得为追角增加预算。产出更新 `docs/algorithm-P011.md`，状态须等 tester 用例就位后再转 `READY_FOR_ARCHITECT_REVIEW`。
   - **architect（等待 designer + tester）**：把通过口径复核的设计翻译为工程方案，明确 pre-commit guard 的落点、账本字段、回归矩阵与实现边界；确认后才可 `READY_FOR_DEVELOPER`。
   - **developer（等待 architect + tester 用例）**：只实现上层沙修复与观测对齐，不改重沙机制、不改阈值与预算；先跑 tester 提供的 RED 用例，再交 reviewer。
   - **后续闭环**：developer→reviewer→tester 正式客观复测→effector 上层沙主观评估→designer 双报告独立复盘→manager。重沙机制另行排期。

## 2026-10-18 P-011 默认重层 FAIL 与 v2.9 设计调度（历史；被 2026-10-19 上层沙裁决取代）

> 日期：2026-10-18。原因：dual-input-v1 完成审核、安装后，tester 新包正式 green 预检确认默认重层响应未达硬门槛，effector 已追加前置 BLOCKED。P-011 仍 P0、`IN_PROGRESS（默认重层响应失败，转 designer v2.9）`；不是用例争议，不等待用户操作，不改门槛。

1. **已完成输入工程**：dual-input-v1 的39项实际产品测试独立重跑 PASS；模式冷热启动、首 Want、体验模式拒绝脚本、真实静态 sensor callback 和面板双向切换 smoke PASS。reviewer 业务与 host 审核 PASS，本地提交 `130a52c74e8964fb024677b672cad1965f5cf1db`。上述通过保留，不等于默认物理效果或正式双测通过。
2. **包与源码身份**：已安装 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`。实际构建基线 `fdc8190`，sourceCommitType=`reviewed P011 baseline + recorded dirty source snapshot`；48源哈希 manifest 绑定包，不能宣称该 reviewer commit 单独 clean 重现。身份明细见最新客观报告与 `test/artifacts/p011-v28-dual-input-20261018/source-manifest.json`。
3. **最新硬前置 FAIL**：`docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 最新正式轮及 `test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json` 确认默认128列/capacity192/max28，单 run script 0/+15/0/-15/0、seq1..5准确，±15各约5.1秒。heavy非零样本124/154达标，但RMS `1.0555e-7/5.3672e-7`（需≥0.5）、max `1.7731e-7/1.6499e-6`（需≥1.0）失败，green质心仅0.006/0.001px；light RMS可达1.719不能代替heavy。旧预检10列/capacity1000/max30与默认几何不同，旧PASS保留但不外推。
4. **立即路由 designer**：输入 `docs/goals.md`、最新P-011客观FAIL/主观 `BLOCKED(green_default_scene_gate_failed)` 双报告、上述gate JSON及算法/架构最新版本。期望在 `docs/algorithm-P011.md` 追加默认几何heavy静止的证据归因、候选方案/取舍与v2.9设计，保留双模式、全部阈值与10列压力回归；不得把非零flux计数当可见形变。随后 architect→developer→reviewer→tester默认green硬预检/模式回归/canonical531冻结审计/完整压力suite→effector自动连续录像视觉评估→designer双报告验证/独立复盘→manager。
5. **测试资产整改直路由 tester**：`p011_v28_preflight_check.py` 仅ROI存在即OBSERVATION_READY/exit0，漏实现已有heavy样本/RMS/max硬门槛。tester 按已有用例先补失败测试，再最小修工具并用当前失败证据复跑RED；不经过设计/业务链、不降低阈值、不覆写旧JSON，与designer文档设计无写冲突可并行。该工具标签不具有放行效力。
6. **执行闸门**：当前canonical531/冻结审计/压力suite与正式subjective均因默认green FAIL标BLOCKED，未执行项不伪记自身FAIL。原生录像能力已具备，effector仅host准备，正式采集需修复后新包硬前置与完整客观PASS、身份固定及设备串行交接。双模式通过、旧物理PASS、录像能力均不能据此CLOSED；最终仍需最新双报告及designer验证/复盘。

## 2026-10-18 P-011 业务开发状态同步（历史）

> 日期：2026-10-18。原因：双模式测试先行与设计前置已交付，流水线进入业务实现阶段。P-011 保持 P0、`IN_PROGRESS（双模式业务开发）`，本次不改变目标或任何门槛。

- **前置完成**：tester `docs/test-cases.md` §10 MODE01~07 及 RED 基线、effector 自动姿态双模式协议及 RED 基线已交付；designer `docs/algorithm-P011.md` §5.8 与独立复盘、architect `docs/architecture-P011.md` §15 `READY_FOR_DEVELOPER` 已交付。
- **当前执行**：developer `p011_input_impl` 正在实现两模式输入隔离及真实 helper unit；host capture/compact 整改已有 19 项回归、PowerShell 与 build 检查，reviewer 已 PASS，尚未 commit。业务实现尚未审核，也未完成新 HAP 正式双测。
- **录制前置解除**：原生录像自动启动、停止、导出能力已实测成功，设备已释放给业务 developer。历史 `BLOCKED(no_screen_recording)` 保留为历史结论，当前能力可进入后续正式采集准备；能力验证不等于主观效果 PASS。
- **下一接力**：业务 developer 交付→reviewer 统一复审并定向提交→记录匹配新 HAP/源码/日志身份→tester canonical precheck + objective→effector 自动连续录像与视觉评估→designer 双报告验证/独立复盘→manager。设备实测串行；原阈值、审计及独立双报告条件保持。

## 2026-10-18 P-011 双模式输入决策（历史前置；输入策略继续有效）

> 日期：2026-10-18。原因：用户正式要求测试模式自行构造手机倾斜状态、体验模式读取真实传感器。输入与验收方式变更已获用户授权，不再要求用户手持操作或代录；物理机制与全部验收阈值不变。

- **归属/状态**：保留 P-011、P0，当前 `IN_PROGRESS（双模式设计前置 + 验证/录制整改）`，不新增独立问题 ID。测试模式生成可复现姿态，体验模式读真实传感器，输入互斥并共用后续处理链路；目标见 `docs/goals.md` 当日双模式说明。
- **测试先行**：tester/effector 输入当日目标与各自 P-011 用例，先补模式隔离、序列复现、进入/退出/重启、传感器接线与自动视觉证据用例及可行 RED 基线；按用户授权追加模拟姿态替代人工输入，不放宽 HG-TT-04、≥6/10 或其他门槛。设计可先准备，输入模式实现等待用例就位。
- **设计交接**：designer 输入上述目标/用例，追加 `docs/algorithm-P011.md` 输入契约、方向/时间语义、共用链路与能力范围；architect 据此追加 `docs/architecture-P011.md` 模式入口、生成器、传感器订阅/滤波隔离、pending/script/forceInvert 状态重置及观测/runner 接入方案。仅输入工程，不扩大物理机制。
- **汇合/路由**：当前 developer 的 host capture/compact 整改继续；双模式用例→designer→architect→developer 与其汇合→reviewer 审核/提交及匹配 HAP→tester canonical precheck+objective→effector 自动视觉评估→designer 双报告验证/独立复盘→manager。共享设备执行串行。
- **录制与边界**：保留历史 `BLOCKED(no_screen_recording)` 至取得连续视频能力；effector 自行探测系统录制 UI 等能力，探测不算正式评分。合成姿态可完成自动验证，无需用户手持/代录；连续视频、hilog 时间对齐与源码/HAP/序列/日志身份要求不变。报告明确自动输入范围，未经实测不声称真实传感器噪声、标定或人体体验已验证。

## 当前优先级

| 优先级 | 问题 ID | 状态 | 路由 | 依赖 / 前置 | 备注 |
|---|---|---|---|---|---|
| P0 | P-005 | IN_PROGRESS | tester + effector 并行 | 单沙+水用例和 RED 基线先就位 | 本轮只覆盖水与一种沙的守恒、沉降/堆积、动态响应、摇晃恢复与观感；三相/双沙用例留后续阶段 |
| P0 | P-003 | IN_PROGRESS | P-005 → designer → architect → developer | 单沙+水验收用例就位 | 场景二第一阶段：水 + 一种沙；不含重/轻分选、第二种沙或三相渗透；完整 G2 三相目标保留后续 |
| P1 | P-001 | OPEN | tester + effector（实测）→ 视失败归因回流 designer/architect/developer | **P-006 已闭环**；场景一客观/主观测试资产可执行，可进入实测排期 | B1/M1’，场景一核心能力已实现但未完成客观/主观实测闭环 |
| P1 | P-006 | CLOSED | tester | 2026-10-15 manager 复验通过 | 场景一测试资产缺口已清零：客观脚本、正式 JSON/Markdown 报告、主观证据模板与三件套输入校验均可执行 |
| P2 | P-002 | OPEN | designer → architect → developer | 相关客观/主观用例就位 | B2/M3，倒立渲染 want 路径已 PASS，面板路径与完整回归未闭环 |
| P2 | P-004 | OPEN | tester + effector 并行 | B3 与 B1/B2 完成后推进 | B4，全量回归与判定体系；依赖前置实现与用例齐备 |
| P1 | P-007 | IN_PROGRESS | designer → developer → tester | v2.4 设计已就位，待 developer 实现并进入验证 | slopeLimit 姿态修正：`|d - tan(effectiveTiltDeg)| > tan(α)`；`α` 语义为“崩塌角” |
| P2 | P-008 | DEFERRED | designer → architect → tester/effector → developer | 单沙+水阶段完成后另行决定是否进入双沙/三相阶段 | percolationRate 参数化与干湿门控；当前不排期、不实现 |
| P0 | P-009 | IN_PROGRESS | architect → tester | 立即并行，不阻塞 v2 基线提交 | WS_SNAPSHOT 4KB hilog 截断，阻塞 E2E 客观验收 |
| P2 | P-010 | CLOSED | architect | 2026-10-15 manager 复核通过 | architecture §10 已包含“本节预算接口已被 §11 v2.2 取代”导航提示 |
| P0 | P-011 | IN_PROGRESS（观测③ v3 方向 2 已批准；执行身份整改链启动，v2 pilot 维持 INVALID） | **designer（v3 协议冻结）∥ developer（采集侧修复 6ad6fb88）→ architect → tester → effector（待新 pilot 批准）** | 身份整改全链完成 + tester 核验 + 协议冻结 + 覆盖矩阵预演方案就位 → manager 另批新 pilot | EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因确认；搜索窗与统计门槛不变；ramp 新资格已批；结构性缺失交 manager 裁决 |

状态说明：OPEN → IN_PROGRESS（designer 已开工）→ VALIDATING（测试中）→ CLOSED（双测试达标，designer 验证通过）

## 2026-10-15 P-006 验收结论（当前生效）

1. **复验通过**：`test/liquid_mode_check.py` docstring 与结果 `problemId`、`test/reports/p006_liquid_mode_check.json`、`docs/test-report-P006.md` 均已统一标识 `P-006`；未再检出 `P-001` 遗留。manager 临时输出复跑自检结果为 `PASS`。
2. **TC-12 通过**：硬门槛 `HG-RECOVERY-TIME` 断言 `recoveryTime <= 4s` 且稳定 1 秒；峰值混色、恢复时间、终态混色三项 TIERED 映射齐全，最低分为及格线 6。最新自检实测恢复时间 `3.308s`，TC-12 最低分 6。
3. **TC-13 与帧输入通过**：脚本校验事件边界外的质心位移与像素分类变化连续性，且 TC-13 TIERED 最低分达标；帧目录、时间戳 manifest、颜色标定 JSON 必须三件套成组输入，并执行存在性、严格递增、目录一致性、颜色空间/颜色值与阈值校验。
4. **闭环结论**：P-006 是测试资产缺口，客观脚本、正式报告与主观证据模板均已可执行，标记 `CLOSED` 并移出 `docs/gap.md`。P-001 的测试前置解除，可进入场景一实测排期。

## 2026-10-18 P-011 验证流程整改与接力（历史；被 2026-10-19 上层沙裁决取代）

> 日期：2026-10-18。原因：继续流水线时复核到最新 architecture §7.5.6/§7.5.7 的采集生命周期要求尚未实现；原 v2.8 客观报告未携带完整冻结/身份审计。此节取代下方历史路由，目标、算法、验收阈值不变；本轮仅聚焦 P-011。

1. **现状与优先级**：P-011 保持 P0。底层倾斜响应直接影响 G2+G3 一级体验，当前阻断可限定为测试采集链路的最小整改，成本低于重设物理机制。历史 `docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 的 PASS、531/531 与行为指标作为已有证据保留，但其 JSON 缺少 `capturePath`、`captureFrozen`、`captureAudit`，不能充当新增闸门的 PASS。reviewer 首轮已明确该阻断项，随后因 workspace requirements 环境异常终止，尚未完成正式复审/提交；这不是物理机制失败结论。
2. **测试先行及责任边界**：已有 `docs/test-cases.md` P-011 §9、`docs/effect-cases.md` P-011 及 architecture §7.5.6/§7.5.7 的明确判定要求。一般测试用例/断言缺口仍直接路由 tester/effector；本次是已进入 developer 的 reviewer 阻断项整改，只允许按“先补阻断项测试，再最小实现修改”处理，禁止扩大到业务/算法或调整阈值。tester 在正式复测前核对这些验证项与用例描述一致，不足时先补测试资产及 RED 基线。
3. **developer → reviewer**：输入为 `docs/algorithm-P011.md` §5.7、`docs/architecture-P011.md` §7.5.6/§7.5.7 与 reviewer 阻断清单。仅整改 `test/p011_tc02_formal_suite.ps1`、`test/p011_tc02_schedule.py`、`test/p011_check.py` 及对应测试：formal suite 和 `schedule-precheck-only` 共用唯一 lifecycle；同步注入结束后停止采集并冻结；输出 canonical 身份与完整 pre/post size/mtime/SHA256 审计；拒绝活动日志、残留句柄、`capturePathMismatch`、`captureNotFrozen`。期望产出“阻断项 → 最小修改 → 测试证据”映射供 reviewer 复审。
4. **reviewer → tester**：复审通过并定向 git 提交后，交付实际新 commit、相关源文件身份、HAP SHA256、检查结果。现有 HAP 哈希一致不等于源码已被旧 commit 完整收录；`fdc8190` 只是原构建/历史代码基线，当前 v2.8 业务改动仍有 dirty 内容，不得标作完整 v2.8 源码身份。若沿用现有 HAP，正式报告必须同时注明原 build 基线、reviewer 新 commit、与包内容对应的源文件身份及 HAP 哈希，明确测试脚本整改与包构建的关系。
5. **tester → effector**：tester 先通过同一 canonical runner 执行独立 schedule precheck，再执行全量 objective。必须携带相同 schedule CSV/注入脚本 SHA256、命令模板、shell 间隔与 hilog 参数，验证 capture audit，随后验证单 run、531/531、连续 sequence、accepted gap ≥100ms、target 一致及全部既有观测/预算/守恒/稳定性门槛。期望在独立 v2.8 客观报告中追加新一轮结果与全部身份、日志哈希；保留原 PASS 和历史失败，不追认旧证据满足新闸门。未通过前不得进入主观正式采集。
6. **effector → designer**：effector 当前仅静态准备采集工具；旧后台主观批次已定向停止，其证据保留为探索材料。tester 交付当前闸门 PASS 后，effector 接管同一设备，按 `docs/effect-cases.md` 独立产出 `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md`、录像/逐帧证据和身份/哈希；不得复用 v2.7.x 或探索批次替代正式验收。`HG-TT-04` 必须通过，TIERED 仍为 0~10 分且 ≥6；共享设备全部安装、操作和 hilog 采集串行。
7. **designer → manager**：输入为同 route、可追溯 HAP 身份的最新双报告与原始证据。designer 必须验证已解决硬门槛保持、软门槛及格、当前问题改善，给出 `VALIDATED` 或未达成归因，并沉淀独立 `docs/retro-P011-v*.md`（可扩展已有相应轮次复盘，须注明日期原因）。manager 仅在双测达标且 designer 验证/独立复盘完成后 CLOSED 并移出 gap；未达成则按算法/工程实现/测试资产归因回流，不自动修改用例。

## 2026-10-18 P-011 v2.7.3 双测复核（历史；路由由上节取代）

1. **客观结果**：`docs/test-report-P011-bottom-creep-v2.7.3-20261018.md` 记录 routeId=`bottom-creep-v2.7.3`，总体 **PASS**；TC02 schedule 校验 531/531，硬门槛与 TC02 行为分档通过。reviewer 提交 `fdc8190` 可作为当前代码基线。
2. **主观结果**：`docs/effect-report-P011-bottom-creep-v2.7.3.md` 总体 **FAIL**。水与轻沙可见响应，但绿色底层在所有有效脚本倾斜场景中保持视觉静止，触发 `HG-TT-04`；所有底层相关 TIERED 维度低于及格线 6。证据根目录为 `demo/effect/P011-v273/`。
3. **闭环判定**：不 CLOSED。客观 PASS 与主观 FAIL 构成观测分歧，必须先归因：客观 telemetry 是否命中了用户不可见的状态、场景/颜色/证据采样是否遮蔽底层，或算法/实现是否实际未驱动底层。
4. **designer 复核**：`docs/algorithm-P011.md` §5 定义 `bottom-creep-v2.8-heavy-interface`，状态 `DESIGN_PENDING_APPROVAL`。归因：绿色层渲染绑定 `bh`，v2.7.3 优先消耗 `bl`，正式数据 `maxSubstepBh=0`、`maxSubstepBl>0`，客观 checker 把聚合位移复制为 H/L，因此 PASS 不代表绿色 heavy 层移动。
5. **architect 复核**：`docs/architecture-P011.md` 已整体切到 `bottom-creep-v2.8-heavy-interface`，状态 `READY_FOR_DEVELOPER`；批准 phase-balanced split、35% heavy floor、compact telemetry、schema v2 checker isolation 与 green-ROI preflight。阈值与硬门槛不变。
6. **developer 复核**：v2.8 实现自检 `p011-selfcheck.json` 总体 PASS，schema v2；Hvigor 编译/打包通过并产出已签名 HAP。green-ROI preflight 为 `OBSERVATION_READY`。已知 TC02 schedule precheck FAIL 属于已单独归因的输入观测状态机问题，不作为 v2.8 行为验收结论。
7. **tester 复核**：v2.8 客观报告总体 PASS，green-ROI preflight `OBSERVATION_READY`；TC02 schedule precheck 531/531、0 gap violation；formal suite 所有 MUST-PASS 通过，TC01/02/03 为 10/8/10。证据与哈希独立成文于 `docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md`。
8. **路由**：P-011 → **effector**。输入：v2.8 签名 HAP、主观用例、客观报告与 green-ROI 证据。期望产出：独立 `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md`；主观 PASS 后交 manager 最终复核。不得合并 v2.7.x 或放宽 HG-TT-04。
9. **effector 阻塞**：`docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md` 正式判定为 `BLOCKED(no_screen_recording)`。设备原生 `uitest` 无屏幕录像能力，连续性、瞬移、撕裂、冻结与抖动门槛无法正式判定；9 轮探索证据仅存于 `demo/effect/P011-v28-heavy-interface/subjective/`，不评分、不判 HG-TT-04 PASS。解除条件：恢复设备原生/外部连续屏幕录制或用户手动采集同 run 连续视频，并提供录制与 hilog 时间对齐；解锁后继续 effector 复测。

## 2026-10-16 P-011 调度（历史）

1. **设计状态**：`docs/algorithm-P011.md` §3 已将 `bottom-creep-v2.7` 标记为 `READY_FOR_DEVELOPER`；`docs/architecture-P011.md` 同步状态，并给出实现步骤、测试钩子与验收口径。
2. **路由**：P-011 → **developer + tester + effector 并行**。developer 按算法与工程方案实现业务代码；tester 完善 `test/p011_check.py` 与客观报告资产；effector 准备 `bottom-creep-v2.7` 独立主观场景、录像与逐帧证据模板。三角色不得修改 manager 独占文档。
3. **串行约束**：tester 与 effector 本轮仅准备测试资产，不启动真机/模拟器执行；后续实测须串行或分设备，避免同机资源竞争。
4. **验收前置**：开发实现必须输出 `P011_EVENT input`、`P011_METRIC`、`P011_BOTTOM` 钩子，且报告固定 `routeId=bottom-creep-v2.7`；闭环仍以客观与主观双测达标为准。
5. **主观阈值裁定（2026-10-16）**：`docs/effect-cases.md` P-011 已定义 0~10 分、及格线 6；`docs/algorithm-P011.md` §3.7 与 `docs/architecture-P011.md` §9.2 的 `3.5/5` 属口径冲突。manager 裁定以 **6/10** 为准，designer/architect 需在闭环前同步修正各自文档；本轮不阻塞 developer、tester、effector 并行。

## 2026-10-15 v2.5 冻结条件复核

1. **复核通过**：`docs/algorithm-P003-P005.md` §20.1 已明确 `effectiveTilt > 0` 时左列为 donor、右列为 receiver；已采用独立 `vBottomCreepMax=10 列/秒` 子通道预算；§18.1 已明确 `slopeLimit` 中 `α` 的语义为“崩塌角”，不是最终收敛角。
2. **调度结论**：三项冻结条件在设计层已清零。因该文档 §20 同时将 v2.5 标记为 `REJECTED`，不恢复 v2.5 实施；P-011 后续指向 `docs/algorithm-P011.md` §3 的 `bottom-creep-v2.7`。
3. **路由**：P-011 → **designer**。输入：`docs/algorithm-P011.md`、`docs/algorithm-P003-P005.md` §18/§20、P-011 客观与主观报告。期望产出：将 `bottom-creep-v2.7` 从 `PROPOSED` 推进到 `READY_FOR_DEVELOPER`，保留上述三项语义、补齐总预算与观测钩子口径，并给出 tester/effector 验收断言；不得修改业务代码。
4. **测试现状**：P-011 客观报告仍因输入时间戳、逐子步 `effectiveTilt` 与 `bh/bl` 钩子缺失而 `BLOCKED`；主观报告因缺少录像与逐帧证据无法评分。闭环仍以双测达标为准。

## 当前阻塞

> 2026-10-14 更新：本节为 2026-10-13 历史阻塞记录；当前由下方“2026-10-14 G2 体验优先级与推进口径（当前生效）”取代，P-003 已解除 `BLOCKED`，保持 `IN_PROGRESS`。

**阻塞项**：`docs/algorithm-P003-P005.md` §3.3 的渗透公式 `hl[c] -= q; hs[c] += q` 会把轻沙转换为重沙，导致 `VH` 增加、`VL` 减少，只保持 `VH + VL` 不变，无法满足三相各自守恒。

**影响与路由**：

1. P-003 不能进入开发实现；developer 不得实现该公式。
2. P-005 不被该阻塞项卡住：tester/effector 可先补客观/主观用例，并把三相守恒写成硬断言，用于暴露此类公式错误。
3. P-003 保持 `IN_PROGRESS（BLOCKED）`；P-005 保持 `IN_PROGRESS`，按新路由分别交 tester 与 effector。
4. **测试先行**：P-003 在 `docs/test-cases.md` / `docs/effect-cases.md` 仍为占位的情况下，不得先交 designer 修订；必须等 P-005 的客观/主观用例就位后再进入实现闭环。

**下一步**：

1. P-005 → tester + effector：立即并行补客观/主观用例，不经过 designer/architect/developer；客观用例必须包含三相守恒断言。
2. P-003 → designer：待 P-005 用例就位后，修订 `docs/algorithm-P003-P005.md` §3.3，给出保相守恒的渗透/分选表达，或明确 v1 禁用该子机制并改由沉降顺序与表层滑移实现分选。
3. P-003 后续仍需 architect 更新工程方案；developer 待 designer 修订且 architect 确认后才可开工。

## 本轮选定

**问题 ID：P-003 + P-005（分路推进）**

### 选择理由

- 用户最新指令明确“场景二（水中两种重量的沙，堆积成形）优先，其次是场景一（两种不互溶液体油水分层）”，因此 B3/P-003 提升为当前最高优先级。
- P-003 是 G2 的最小可验证核心，直接覆盖三相守恒、重沙/轻沙分选、安息角、摇晃悬浮与再堆积等场景二关键目标。
- P-005 是 P-003 的验证前置：当前 `docs/test-cases.md`、`docs/effect-cases.md` 均为占位，仓库中也没有 WaterSandField/三相判定脚本；不补齐测试用例，P-003 无法客观/主观闭环。
- 当前 `test-report.md` 与 `effect-report.md` 均无有效数据，优先完成 P-003 + P-005 可先建立场景二的客观守恒、堆积/分选与主观观感基线，再反哺场景一的倒立与渲染验收。
- 修复成本相对可控：P-003 主要新增 WaterSandField 列标量三相模型、第四渲染模式与 Node 单测；P-005 可复用现有两相脚本框架扩展为三相与主观用例。

### 验收要求

1. **P-003 客观验收**：多轮旋转、倒立与摇晃角序列下，water、Σhs、Σhl 各自总量守恒，漂移 < 0.1%；susp ↔ 沙堆只做相内转移，不改变各相总量。
2. **P-003 分选与堆积**：静置后重沙在沙堆上 1/3 的占比 / 下 1/3 的占比 ≤ 0.5（初值）；沙堆轮廓平滑、坡度不超过安息角上限；长时静置零漂移。
3. **P-003 动态表现**：重沙沉降快于轻沙（vH > vL）；摇晃时沙堆表层按比例扬起为悬浮态，E 衰减后重新沉降并堆积；全过程连续平滑，无粒子瞬移，实测帧率 ≥ 100fps。
4. **P-005 客观测试用例**：产出可执行用例，覆盖三相守恒、分选比、沉降时序、安息角、水面水平/倾斜、倒立相序、摇晃→悬浮→再堆积、长时静置零漂移与 FPS；实现或扩展判定脚本以支持 VV-07/08/09 与三相像素/粒子分类。
5. **P-005 主观测试用例**：产出可复现的主观用例，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷；评分标准需可量化。
6. **闭环条件**：P-003 与 P-005 均需 tester 的客观测试与 effector 的主观效果测试达标；任一不达标则保持 OPEN，不得仅凭实现完成或用例编写完成而关闭。

### 路由与交接说明

#### P-003 → designer

- **前置条件**：P-005 的客观/主观用例已就位，能验证三相守恒、分选、堆积、沉降、水面、倒立与主观观感。
- **输入文档**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四（B3）、`docs/algorithm-P003-P005.md` v1、`docs/architecture-P003-P005.md` §0.1、补齐后的 `docs/test-cases.md`、`docs/effect-cases.md`。
- **期望产出**：修订 `docs/algorithm-P003-P005.md` §3.3，保持 `VH = Σ(hs + suspH)` 与 `VL = Σ(hl + suspL)` 各自守恒；如 v1 无法给出保相渗透公式，应明确禁用该子机制并说明替代分选机制，同时在变更记录中写明原因。
- **必须包含**：不改变其他章节的修订说明；替换公式或明确的禁用策略；对测试钩子的影响；以及 developer 可据此实现的伪代码。
- **后续依赖**：designer 修订完成并经 architect 确认后，developer 才可按更新后的 `docs/architecture-P003-P005.md` 开工。

#### P-005 → tester + effector

- **tester 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/test-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`。
- **tester 期望产出**：补齐 `docs/test-cases.md` 中 P-005 的客观用例与可执行判定脚本，覆盖三相守恒、分选比、沉降时序、安息角、水面、倒立、摇晃→悬浮→再堆积、长时静置与 FPS。
- **effector 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/effect-cases.md`。
- **effector 期望产出**：补齐 `docs/effect-cases.md` 中 P-005 的主观场景与评分标准，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷。
- **并行规则**：tester 与 effector 可并行；二者不修改 algorithm/architecture/developer 代码，不等待 P-003 修复。
- **测试先行**：P-005 是 P-003 的前置；用例未就位前，P-003 不进入 designer 修订，developer 更不得开工。

### 2026-10-14 G2 体验优先级与推进口径（当前生效）

**用户指令**：G2 实现优先级为：

1. **一级**：手机翻转时沙子随重力移动，覆盖重力方向连续变化时的响应。
2. **一级**：手机倒扣时，原位于下方的轻沙慢慢上浮、重沙下沉，完成分选反转的动态过程。
3. **二级**：剧烈摇晃时的混乱效果，包含悬浮/扬起。
4. **二级**：混乱后的沉淀与再堆积效果。

**放行规则**：

- 一级体验不达标时，二级项不作为放行理由。
- 二级项的验收要求不降低，仅调整推进顺序；摇晃混乱与再沉淀仍须按原阈值闭环。
- P-003 的 v2.2 修订与 developer 修复，应优先保证一级体验的物理正确性与观感自然度，再优化二级混乱与沉淀链路。
- P-005 的 tester/effector 应优先补齐并执行一级体验的动态验收，再执行二级体验回归。

**测试覆盖评估（2026-10-14）**：

- `TC-WS-07` 目前覆盖 180° 倒立后的最终相序与守恒，但对“重力方向连续变化时沙的响应”以及“倒扣后轻沙上浮、重沙下沉的分选反转动态”覆盖不足；需补连续重力扫描、倒置过程时序和相序交换时间指标。
- `EC-WS-07` 覆盖倒立/翻回的最终形态与过渡自然度，但对倒扣后轻沙慢慢上浮、重沙下沉的过程缺少明确时序与评分锚点；需补充过程动态与节奏评分。
- `TC-WS-08`、`TC-WS-09` 与 `EC-WS-06` 已分别覆盖摇晃扬起、再沉降、再堆积与观感节奏，与二级体验优先级基本对齐；后续只需按原阈值执行，不降低要求。

**当前推进状态**：

- P-003 保持 `IN_PROGRESS`：designer 正在产出 v2.2 修订，developer 正在修复实现问题；二者不得因二级效果达标而提前放行一级缺口。
- P-005 保持 `IN_PROGRESS`：tester 与 effector 需并行补齐一级体验的客观/主观动态指标，二级项排在一级之后执行。

### 2026-10-14 跨角色同步决策与路由

#### G3 事件式 90° 语义

- **决策**：G3 当前目标采用“±89° 连续重力变化 + 90° 由经审计的 `invert180()` 事件跨越”的事件式语义；严格连续过 90° 不纳入本期验收，如有需要另立 v3。
- **已更新**：`docs/goals.md` 的 G3 指标与变更记录。

#### `docs/effect-cases.md` H06 硬门槛措辞

- **评估结论**：外部评审指出 H06 现有“倒立/翻回状态破坏，含数据模型被改写→FAIL”与经审计的 `invert180()` 物理事件设计冲突，结论成立。
- **路由**：P-005 → **effector**。
- **输入文档**：`docs/effect-cases.md` 的 H06 硬门槛条目。
- **期望产出**：将 H06 修改为“绕过 API 私改内部数组/状态 = FAIL；经 `invert180()` 且通过守恒审计，不因事件式跨越 90° 本身判 FAIL”。
- **原因**：区分违规改写与合法事件式翻转，避免误伤经审计的实现。

### 2026-10-14 v2.2/v2.3 批量路由决策

| 事项 | 决策 | 路由 | 优先级 | 执行时机 |
|---|---|---|---|---|
| H06 硬门槛措辞 | 同步为“绕过 API 私改物理数组=FAIL；经 `invert180()`、带事件日志且过三相守恒审计的坐标切换不判 FAIL” | **effector** | P0 | 立即执行 |
| HG-04 单帧位移口径 | 明确为“物理子步”检查；30Hz 两子步分别校验，不得相加后冒充单子步 | **tester** | P0 | 立即执行 |
| TC-WS-08 分相悬浮记录 | 增加 `suspensionFractionHeavy/Light` 分相指标与记录 | **tester** | P0 | 立即执行 |
| slopeLimit 姿态修正 | 认定为真实算法缺陷，批准为独立小修，不并入 v3；正确判据为 `|d - tan(tiltDeg)| > tan(α)` | **designer → developer → tester** | P1 | P-003 当前修复完成后立即执行 |
| percolationRate 常数 | 当前固定场景可作工程近似；后续需参数化并增加干湿门控 | **designer → architect → developer** | P2 | G2+G3 主线闭环后推进 |
| WS_SNAPSHOT 4KB 截断 | 登记 E2E 客观验收阻塞项；需 architect 确认快照分片或文件落盘方案，tester 同步解析与校验 | **architect → tester** | P0 | 立即并行，不阻塞 v2 基线提交 |
| architecture §10 取代提示 | 增加“本节已被 §11 v2.2 取代”导航行 | **architect** | P2 | 下一轮文档维护 |

### 2026-10-14 等待期可并行启动项

> 2026-10-15 备注：本节为历史调度记录。P-006 已复验通过并闭环；P-007 的算法设计已由 v2.4 §18 完成；P-011 的后续调度以 2026-10-19 上层沙裁决为准。

> 目标：在 P-003/P-005 主线等待 developer/reviewer/tester 期间，优先补齐测试用例与支撑能力，不抢占场景二主线资源。

| 问题 ID | 类型 | 现有用例情况 | 可启动内容 | 路由 | 启动时机 | 与主线关系 |
|---|---|---|---|---|---|---|
| P-006 | 测试用例缺口 | 客观：`test/TC_TEST_CASES.md` 已有 TC-11~13 草案；主观：`docs/effect-cases.md` 仍为占位 | 将 TC-11~13 转正并补齐场景一客观/主观用例与判定脚本 | **tester + effector 并行** | 立即启动 | 不阻塞 G2+G3 主线；为 G1+G3 支线提前铺路 |
| P-009 | 测试支撑缺口 | 无可执行的快照分片/落盘方案 | 确认 `WS_SNAPSHOT` 分片输出或文件落盘方案，并同步 tester 解析与校验 | **architect → tester** | 立即启动 | 支撑 G2+G3 主线 E2E 客观验收 |
| P-010 | 文档支撑缺口 | 无 | 在 architecture §10 增加“本节已被 §11 v2.2 取代”导航行 | **architect** | 立即启动 | 低成本文档修正，不阻塞主线 |
| P-004 | 回归测试缺口 | 客观/主观用例均未成体系 | 先行设计两场景组合线的全量回归矩阵与判定脚本框架 | **tester + effector** | 可预研，执行待 P-003/P-005 就位；P-006 前置已解除 | 为 G2+G3 与 G1+G3 最终回归做准备 |
| P-007 | 功能缺口 | 客观用例尚未覆盖倾斜姿态修正 | designer 可先行准备 `slopeLimit` 修正方案与测试预期 | **designer** | 可预研，实施待 P-003 当前修复闭环 | 不抢占主线；避免后续返工 |

### 2026-10-14 子目标排序与路由

> 全局排序遵循组合目标线 **G2+G3 → G1+G3**；G3 不是独立交付项，而是分别并入两条组合线的共享容器动态能力。组内按 **重要 > 视觉效果好 > 容易实现** 排列。`需补用例` 表示现有 TC/EC 覆盖不足，需由 tester/effector 先补齐，再进入实现闭环。

#### G2+G3 主线（当前优先）

| 排序 | 子目标 | 重要度 | 视觉效果 | 实现难度 | 当前路由 | 对应用例 / 需补用例 |
|---|---|---|---|---|---|---|
| 1 | G2.1 三相守恒与静置分选基线 | 高 | 中 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-01~04`、`TC-WS-10`；`EC-WS-01`、`EC-WS-03`、`EC-WS-04` |
| 2 | G2.2 静态倾角响应 | 高 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-05~06`；`EC-WS-02`、`EC-WS-05` |
| 3 | G2.3 连续翻转时序 | 高 | 高 | 高 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补连续重力扫描与时序用例 |
| 4 | G2.4 倒置分选动态 | 高 | 高 | 高 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补倒置过程时序与交换完成度用例 |
| 5 | G2.5 相序交换完成度 | 高 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补交换完成度评分用例 |
| 6 | G2.6 摇晃混乱与再沉淀 | 中 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-08~09`；`EC-WS-06` |
| 7 | G2.7 渲染一致性与流畅度 | 中 | 高 | 低 | P-003/P-005：designer + developer + tester + effector | `TC-WS-11~12`；`EC-WS-08` |
| 8 | G3.1 事件式 90° 跨越语义 | 高 | 中 | 中 | P-003/P-005/P-004：developer + tester + effector | `TC-WS-07`、`EC-WS-07` 部分覆盖；需补专项事件语义用例 |
| 9 | G3.2 动态守恒与状态稳定 | 高 | 中 | 中 | P-003/P-005/P-004：developer + tester + effector | `TC-WS-01`、`TC-WS-10`；`EC-WS-08` |
| 10 | G3.3 长时回归矩阵 | 高 | 中 | 高 | P-003/P-005/P-004：developer + tester + effector | 需补全量回归用例 |

#### G1+G3 支线（另行开题）

| 排序 | 子目标 | 重要度 | 视觉效果 | 实现难度 | 当前路由 | 对应用例 / 需补用例 |
|---|---|---|---|---|---|---|
| 1 | G1.1 双相守恒与基础分层 | 高 | 中 | 中 | P-001：tester + effector | `TC-11`、`EC-OL-01`；P-006 用例已就位，待实测 |
| 2 | G1.2 翻转交换时序 | 高 | 高 | 中 | P-001：tester + effector | `TC-13`、`EC-OL-03`；P-006 用例已就位，待实测 |
| 3 | G1.3 界面恢复与视觉稳定 | 高 | 高 | 中 | P-001：tester + effector | `TC-12`、`EC-OL-02`；P-006 用例已就位，待实测 |
| 4 | G3.1 事件式 90° 跨越语义 | 高 | 中 | 中 | P-001/P-004：developer + tester + effector | `TC-13` 部分覆盖；需补专项事件语义用例 |
| 5 | G3.2 动态守恒与状态稳定 | 高 | 中 | 中 | P-001/P-004：tester + effector | `TC-11~13`、`EC-OL-01~03`；P-006 用例已就位，待实测 |
| 6 | G3.3 长时回归矩阵 | 高 | 中 | 高 | P-001/P-004：tester + effector | 需补全量回归用例 |

## 已闭环

| 问题 ID | 闭环日期 | 结论 |
|---|---|---|
| P-006 | 2026-10-15 | 场景一测试资产补齐并复验通过；客观脚本与正式报告统一为 P-006，TC-12/TC-13 判定、TIERED 映射、帧输入三件套与主观证据模板均满足执行要求 |
| P-010 | 2026-10-15 | `docs/architecture-P003-P005.md` §10 已加入“本节预算接口已被 §11 v2.2 取代；本节仅保留为历史设计记录”的导航提示 |

## 变更记录

- 2026-10-20：原因：【观测③·v3】外部评审验收 designer 识别策略重论证。manager 三项裁定：① 批准方向 2（稳定平台+安全提交 ramp 联合激励）进入新协议设计，搜索窗 [25°,30°)/[15°,25°] 与全部安全/统计门槛不变，ramp leg 预注册/决策残差时点/0.5° bracket 分辨率冻结，序列 0→+15→+A→+15→0→−15→−A→−15→0（A 由影子审计定）；② 确认 EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因（构建实际 tan(4°) 驱动、候选未进状态机），执行身份整改路由生效：designer 协议冻结 ∥ developer 采集侧修复（6ad6fb88，立即）→ architect → tester → effector 待批；③ 结构性缺失预案：预演覆盖矩阵若显示子 cell 结构性缺失，交 manager 裁决（限调整激励/上报用户/维持 INCOMPLETE 三选），禁默默合并。补全速率档、估计对象声明、右删失新定义三项口径；新 pilot 批准前置=身份整改全链+tester 核验+协议冻结+覆盖矩阵方案。

- 2026-10-19：原因：architect 复核确认 slip-overshoot-v2 相邻输运下界与 raster 口径。manager 接受条件性不可达，维持 30°±2°/5.1s/预算不变，改路由 designer 做 lift/settle/suspension 离线可达性审计、tester 修正 target-based t90 与拟合优度/稳定终点口径；本轮不授权新观测或实现。

- 2026-10-19：原因：用户裁决 P-011 先修上层沙。manager 复核 v2.11 否决结论与 slip-overshoot-v1 证据，判定 484 次坡度越界为真实物理违约、context/audit 错位为观测缺陷；新增 P-011-A/B/C 分线，tester 与 designer 并行，重沙机制押后，并固化硬约束、收敛、预算与账本对齐验收口径。

- 2026-10-18：原因：新双模式包默认128列green正式硬前置FAIL，主观相应BLOCKED。记录双模式工程/commit/HAP真实交付身份，P-011从业务开发推进到designer v2.9归因设计；既有预检硬门槛工具遗漏另直路由tester，不改阈值或历史结果。

- 2026-10-18：原因：双模式用例/RED 基线与设计前置完成，原生自动录制能力验证成功。P-011 推进到双模式业务开发，等待交付后 reviewer 统一定向提交及新 HAP 自动双测；保留历史结论，不改阈值。

- 2026-10-18：原因：用户正式要求测试/体验双模式。将输入工程纳入 P-011，采用先补用例→designer→architect→developer 与采集整改汇合路径，取消用户手持/代录前置，保留全部阈值和连续录像证据要求。

- 2026-10-18：原因：发现最新 architecture §7.5.6/§7.5.7 的 canonical runner 与冻结审计证据缺失。保留历史 v2.8 objective PASS，将 P-011 从 VALIDATING 调整为 IN_PROGRESS（验证流程整改），接力 developer→reviewer→tester→effector→designer 双报告验证/独立复盘→manager；补充源码/HAP 身份说明与共享设备串行要求。

- 2026-10-18：原因：manager 复核 P-011 v2.7.3。客观 suite 与 reviewer 提交 `fdc8190` 均确认 PASS，但同路线主观报告缺失，因此状态改为 `VALIDATING` 并路由 effector 产出独立主观报告；未确认 CLOSED。
- 2026-10-18：原因：effector 产出同路线主观报告。EC-TT-01~05 真机证据显示底层完全不动并触发 `HG-TT-04`，总体主观 FAIL；P-011 保持不闭环，改为路由 designer 做客观遥测与主观静止的分歧归因和下一版方案。
- 2026-10-18：原因：designer 复核 v2.7.3。证据链确认 heavy/light 分配失衡与 checker 聚合口径造成客观 PASS 但主观底层静止；新增 `bottom-creep-v2.8-heavy-interface` 设计，状态 `DESIGN_PENDING_APPROVAL`，下一站 architect。
- 2026-10-18：原因：architect 审批 v2.8。确认现有架构可实现 phase-balanced split 与观测链路改造，状态升级为 `READY_FOR_DEVELOPER`；下一站 developer 实现，随后 tester/effector 独立复测。
- 2026-10-18：原因：developer 完成 v2.8。route/相位拆分/compact telemetry/checker schema v2/green-ROI preflight 已实现，checker 自检与打包自检通过；P-011 改为 `VALIDATING` 并路由 tester 先行。
- 2026-10-18：原因：tester 完成 v2.8 客观复测。green-ROI、TC02 schedule 与 formal suite 全部 PASS，客观与主观观测链路分离；路由 effector 执行最终主观验收。
- 2026-10-18：原因：effector 复核 v2.8 主观链路。设备可用但无连续屏幕录制，正式主观判定登记 `BLOCKED(no_screen_recording)`；未评分，HG-TT-04 保持未判定，P-011 不 CLOSED。

- 2026-10-15：原因：manager 复核 P-010。确认 architecture §10 导航提示已就位，满足文档支撑验收条件；P-010 标记 CLOSED 并从 gap 清单移除。

- 2026-10-15：原因：manager 复验 P-006 客观脚本返工。确认脚本与正式 JSON/Markdown 报告均标识 P-006，TC-12 恢复时间、TIERED 映射、TC-13 连续性与帧输入三件套校验全部通过；P-006 标记 CLOSED 并移出 gap，P-001 测试前置解除。

- 2026-10-15：原因：manager 复核外部专家提出的 v2.5 三项冻结条件。确认方向语义、独立 `vBottomCreepMax=10` 与 `α=崩塌角` 均已在算法文档清零；因 v2.5 为 `REJECTED`，将 P-011 指向 `bottom-creep-v2.7` 并路由 designer，同时将 P-007 更新为设计就位、待实现验证。
- 2026-10-15：原因：manager 复验 P-006 返工。`liquid_mode_check.py` 仍输出 P-001，复验不通过并路由 tester 修复；其余客观判定项与 `environment.json` 模板校验通过。
- 2026-10-14：原因：用户真机反馈底层沙能动但不跟手，且临时调参未确认。新增 P-011 为 P0，先补客观/主观用例；实测不达标后再进入 designer。

- 2026-10-13：初始创建。
- 2026-10-13：原因：启动第一轮调度。按 `docs/DESIGN_GAP_CLOSE.md` 的 B1→B4 顺序建立优先级；因两份最新报告均无有效数据且 P-001 是 G1 的前置核心，本轮仅选定 P-001 交由 designer 输出算法方案。
- 2026-10-13：原因：用户最新指令调整场景推进顺序。将 B3/P-003 提升为当前最高优先级并选定为本轮问题，B1/P-001 与 B2/P-002 降为次优先级，B4/P-004 保持最后；同步更新选择理由、验收要求与 Designer 交接说明。
- 2026-10-13：原因：用户补充测试用例也属于 gap 管理范围。新增 P-005/P-006 独立测试缺口，本轮选定 P-003 + P-005，并要求算法方案提供可测试钩子以支撑 tester/effector 后续落地用例。
- 2026-10-13：原因：architect 发现 algorithm v1 §3.3 渗透公式破坏分相守恒。登记当前阻塞项，将 P-003/P-005 标记为 `IN_PROGRESS（BLOCKED）`，并明确 developer 需等待 designer 修订与 architect 确认后开工。
- 2026-10-13：原因：用户更新 `.codex/PIPELINE.md` 问题路由规则。为每个问题标注路由：P-003 走 designer→architect→developer 完整闭环；P-005 由 tester+effector 并行直补，客观/主观用例不被算法修订阻塞。
- 2026-10-13：原因：用户新增“测试先行”原则。将 P-005 明确为 P-003 的前置依赖；在客观/主观用例就位前，P-003 不进入 designer 修订或开发实现。
- 2026-10-14：原因：用户明确 G2 实现优先级。将 P-003 状态更新为 `IN_PROGRESS`，补充一级体验（翻转重力响应 + 倒扣轻沙上浮分选）与二级体验（摇晃混乱 + 再沉淀）的验收排序；一级不达标时二级不作为放行理由，二级阈值不降低；同时评估 `TC-WS-07` / `EC-WS-07` 的一级动态覆盖不足，`TC-WS-08` / `TC-WS-09` / `EC-WS-06` 与二级链路基本对齐。
- 2026-10-14：原因：designer v2.2 完成后的跨角色同步。固化 G3 为“±89° 连续 + 90° 事件式”语义，并将 `effect-cases.md` H06 硬门槛措辞修正路由给 effector，要求区分违规私改与经审计的 `invert180()` 事件。
- 2026-10-14：原因：用户要求拆小目标。新增 13 个可独立验收子目标并按“重要 > 视觉效果好 > 容易实现”排序，标注当前路由与现有用例/需补用例，支撑后续逐项闭环。
- 2026-10-14：原因：用户纠正目标结构。将排序从三条独立目标线改为 **G2+G3 主线** 与 **G1+G3 支线** 两条组合目标线，G3 作为共享动态能力并入两线，不再独立排队。
- 2026-10-14：原因：v2.2 复核与 designer v2.3 提出批量路由请求。新增 P-007~P-010，分别登记 slopeLimit 姿态修正、percolationRate 参数化、WS_SNAPSHOT 4KB 截断与 architecture §10 导航；同时将 H06 与 HG-04/TC-WS-08 口径修正纳入 P-005 的 effector/tester 立即执行事项。
- 2026-10-14：原因：用户要求识别主线等待期可并行启动项。将 P-006、P-009、P-010 标记为立即启动，P-004、P-007 标记为可预研，明确测试用例缺口优先于功能实现，且不抢占 G2+G3 主线。
