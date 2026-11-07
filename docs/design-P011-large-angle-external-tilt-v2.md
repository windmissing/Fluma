# P-011 大角度外部倾角与瞬态超临界：用户批准后的算法修订 v2

日期：2026-10-23。责任：designer。状态：**USER_APPROVED_SEMANTIC_DIRECTION / DESIGN_FOR_ARCHITECT_REVIEW / NOT_IMPLEMENTED / NOT_VALIDATED**。用户本轮批准 [v1 §2 路线 B](design-P011-large-angle-sand-water-v1.md) 的第 1 项：外部转动可产生**受控的短暂超临界沙床**。本批准不包含提高 300/s、修改 128 列几何、放宽 26°/20° 用户双角校验、宣称 ±45°/5.1s 全宽目标可达，或追认历史 SLIP 报告。现行代码和 checker 未据此改动；本稿交 architect/tester 工程与验收翻译。

## 1. 必须改的合同与保持的合同

现行 `|g_e−tan(effectiveTilt)|≤H`（`g_e=S[e+1]−S[e]`，`H=tan30°≈0.577350269`）对**所有已提交子步**要求零违约；平沙突转45°时 `|0−tan45°|=1>H`，故旧 planner 钳角约30°。新合同将**外部姿态改变**与**材料 writer 改形**分账：真实设备姿态/TEST 命令的同一 `θgravity` 驱动水和沙；姿态可以令床面暂时超临界，材料事务不能制造新的超临界边、不能把既有超临界边推得更坏，且必须报告恢复状态。数值 `H` 仍为安息角参考；它不等于可写入任意超角形状的通行证。§19.5 的合法态→合法、违规态→non-worsening 原则保留，预算/守恒/库存/容量/有限值及四层对账全程照旧。

形式化的子步快照：`S0` 为姿态变化前床面，`θ0` 为旧物理角，`θ1` 为本步同源输入的物理角。令

```text
r0[e] = (S0[e+1]-S0[e]) - tan(θ0)
rE[e] = (S0[e+1]-S0[e]) - tan(θ1)    // 姿态作用后，材料写入前
excess(r) = max(0, |r|-H)
poseAdded[e] = max(0, excess(rE[e])-excess(r0[e]))
rW[e] = (S_after[e+1]-S_after[e]) - tan(θ1)
```

每个改变 `S` 的 writer，在**该 writer 之前的同角快照**上预演所影响的全部边：前态合法则后态仍 `|r|≤H`；前态超临界则 `|r_after|≤|r_before|`，若后态仍超临界还须符号不反转；无影响边保持原样。由 writer 导致的 `max(0,excess_after−excess_before)` 必须为0。多 writer 顺序以现有权威调度和 preview/apply 镜像为准，不能只查整步末态掩盖某一笔先推坏、下一笔补好的轨迹。`poseAdded>0` 是外部驱动的可解释事件，**不是 writer 安全 PASS，也不是沙面效果 PASS**。本规则只在新 routeId 生效；旧 route/checker 原报告身份不重解释。

## 2. 姿态输入与水沙共用重力

建议 routeId `p011-external-tilt-transient-v1`：TEST 构造倾角与 EXPERIENCE 传感器只更换命令来源，经过同一有界噪声滤波/角速率约束得到 `θgravity`；同一子步的水面、沙的体力/双角残差、guard、粒子重力和 ledger 均读这一角。最大角速率仍须以真实姿态响应 `P95≤100ms` 和抖动证据定标，不能把“避免超临界”作为无限减慢跟手的理由。旧 `targetTilt/effectiveTilt` 字段需由 architect 定义向后兼容映射：命令角、物理重力角、旧 planner 接受角分别保留身份，不让水读命令角而沙读旧接受角。

新 route 的倾角提交**不能继续以旧两轮可行角交集作为前置**，否则平沙45°仍在上游被拒。它应在保存完整材料/水/预算/路线状态后：提交本步 `θgravity` 候选 → 以该角从同一快照预演所有材料请求与水更新 → 完整核对 writer 安全和各库存/预算 → 原子提交。任何水量非负/容量、材料预算/守恒或 preview/apply 身份失败仍整步回滚并记失败 attempt；材料无合法运动时可以提交真实姿态和保守水运动，先标记 `SUPERCRITICAL_ACTIVE/NOT_EFFECT_VALIDATED`。只有在预注册的固定姿态观察窗口内持续无有效修正进展，才标记 `SUPERCRITICAL_STALLED`；不能靠水动而冒称沙已收敛。当前 `waterLevel` 只按该角松弛到目标平面，适合作为隔离几何验证，不等于惯性水流通过；后续水的 `h,hu` 有限体积升级见 [v1 §3](design-P011-large-angle-sand-water-v1.md)。

方向翻转时重新按**新外部姿态**计算 `rE/poseAdded`，保留物理材料与水量；旧超临界区不可用重置计时/改符号藏掉。run/reset 清本路线的计时与 pending 事务；TEST↔EXPERIENCE 切换保留合法材料/水场但开新输入 epoch，清滤波和判定缓存并记录姿态连续性。切回旧 strict route 时若当前场不满足旧逐子步硬谓词，禁止无声迁入/瞬间捏造坡形：报告 `ROUTE_HANDOVER_INCOMPATIBLE`，待物理恢复或显式重开 run。模式切换不改变 128 列或历史账。

## 3. 单边 guard 会死锁：需要整组相邻通量的原子准入

平沙刚到正45°时每条内部边 `rE=-1`。单独把一笔 `q` 从左列搬到右列使中央边 `Δg=+2q`（修正），却使左右邻边各 `Δg=-q`（恶化）；现行 [P011MaterialGuard](../entry/src/main/ets/physics/P011MaterialGuard.ets) 的“已违规边 non-worsening”会正确截断这一笔。因此“让角度先到45° + 保留原逐对 writer”**可能整片零 grant**，不能作为完整方案。

新流动层按**同一物理快照**生成一组相邻面通量 `f_e`，计算各列 `ΔS_c=f_{c-1}−f_c`（墙外 `f_{-1}=f_{N-1}=0`），预演整组后的 `Δg_e=2f_e−f_{e-1}−f_{e+1}`。仅在全组满足上节逐边谓词、分相供体/受体容量、local/channel/total 实际额度后，作为**一笔复合 writer 原子提交**；任一检查失败则对整组按共同 `λ∈[0,1]` 截断或拒绝。固定请求方向和候选组后，`r'_e=r_e+λΔg_e`，每边仍是线性不等式；取所有边、库存与预算给出的 `λ` 区间交集即可闭式求最大准入比例，无需事后修坡或迭代偷运。每条真实 `f_e` 独立计入相应相别和列边的预算/一阶矩，整组不能只计一次 `max_e f_e`。preview 与 apply 必须逐浮点操作对齐，保证无部分组已写后失败。

**仅作可行性数学例子，不是生产目标形状律：** `N=128`、`f_e=a(e+1)(127−e)`、`a>0`，可得所有127边 `Δg_e=2a>0`，在平沙45°超临界态逐边修正且无邻边恶化；总横向通量成本 `Σf_e=349,504a=174,752·(2a)`。若所有跨边沙通量仍按 total 真实计费，5.1s 总额1530至多让全宽线性坡度增 `1530/174752≈0.00876`（角度约0.50°）；目标28°至少需约92,917，差约60.7倍。它证明**原子组能解除单笔局部死锁，却不能解除冻结额度与全宽目标的缺口**。生产通量必须由移动表层的驱动、摩擦/湿度和实际库存产生，不能把上述抛物线 `f_e` 硬编码成沙丘目标。

## 4. 超临界的完成、失败与遥测

新增一条独立恢复轨：`Emax(t)=max_e excess(r_e(t))`、`EΣ(t)=Σ_e excess(r_e(t))²`、有效超临界边数与连续持续时间；同时记录真实纠正通量/预算截断/guard拒绝。材料不作功时外部继续加角可使 E 增加，但必须归因 `poseAdded`；固定姿态的窗口里 writer 不得让任一边恶化，目标是 E 衰减。仅“E 不变”不是恢复成功。完成判定需要姿态到目标、终点 `E=0` 或经正式批准的稳态口径、末段稳定与原 SLIP-02 效果包分别满足；不能凭角度到45°就写 PASS。

本稿**不拍恢复时限或最小衰减率**。tester/effector 用真实 ±45° 输入、干/湿/水下分组和 128 列账本预注册 `T_recover`、最小进度、时间窗与测量误差；若现行300/s理论下界已否定5.1s同达目标，须先出冲突报告交 manager，不能选一个虚假易过时限。无纠正源、单边死锁、复合组仍为0、预算耗尽、材料不足、连续超临界未降均分开报 `SOURCE_UNAVAILABLE / LOCAL_GUARD_DEADLOCK / GROUP_BLOCKED / BUDGET_EXHAUSTED / INVENTORY_BLOCKED / SUPERCRITICAL_STALLED`。这套诊断是**诚实失败出口**，不是新路线自动放行。

最少逐子步/逐边字段：`runId/substepId/inputMode/routeId/inputEpoch`，`thetaCommand/thetaGravity/thetaOld/tiltRate`，`r0/rE/rW/poseAdded/excessBeforeWriter/excessAfterWriter`，`writerId/groupId/f_e/requested/granted/λ/limitingEdge/guardReason`，local/bottom/channel/total 的 used/remaining、H/L 和水量前后、`Emax/EΣ/age/stallReason`、`previewHash/commitHash`。原记录不得把新 `poseAdded` 伪装成原 SLIP-01 零违约。

## 5. 验收口径修订与工程交接

| 原用例/模块 | 新 route 必须怎样改；旧 route 怎样保留 |
|---|---|
| SLIP-01 / checker | 旧 route 的逐子步零残差硬线及历史结果不动。新 route 改为**来源分类**：`poseAdded` 允许但单列计数/时长，任意 writer 新增/恶化超临界边为硬 FAIL；复合 writer 整组前后逐边检查；恢复轨另评。不能沿用旧 `SKIP_TAN30_PHASES` 相位豁免来替代来源分类，也不能只在 ±45° 放行而漏掉实际 ramp。需由 tester 形成 versioned `test-cases`/checker revision。 |
| SLIP-03/04 与其它安全 | H/L/水守恒、库存、容量、有限值、HG-04 单步表面速度、预算上限与 clipped 对账全部不变；旧场景 bit-exact/容差回归独立跑。超临界窗口内失去完成证据时标 `INCOMPLETE/FAIL`，不能以新语义撤掉预算或质量失败。 |
| 双角/阶段二目标 | 26°/20°为用户可调出厂推荐，起停判据在新 `θgravity` 下按真实残差算，不能把阈值当外部姿态限幅；观测③既有标定/右删失结论不追认。±45°的 `30°±2°、t90≤5.1s、R²≥0.90、末1s稳定、终点悬沙≤5%` 按阶段二原数值保留 `NOT_VALIDATED`，直到全通道证书。 |
| 水与外观 | 水在同一 `θgravity` 更新并逐笔保水；首轮只验证当前松弛水面在45°几何/容量的行为，惯性晃动须增 `h,hu` 后单独验。P-012 云雾/沙粒只读物理，不用 shader/画面假装 sand grant。 |

建议实施门序：① designer 本稿交 architect 复审复式 writer 的快照/预算/guard 可行性，同时 tester 预写新路由用例与版本隔离；② developer 在**独立 routeId、默认关闭**的 TEST/EXPERIENCE 共用输入路径实施姿态来源分账与复合 writer，先跑无水/静水隔离；③ tester 跑平沙 0→+45/−45、保持、回零/反转、±15、无沙、预算零、局部复合组死锁反例、preview/apply 一致性；④ effector 记录真实水/沙响应、P95≤100ms 与端点；⑤ 依据全通道证书决定新水动量和移动沙层的后续实现次序。manager 以每门证据决定转正，未过仍维持 `NOT_VALIDATED`。P-011 阶段一已 CLOSED 的旧路线不被本试验回写。

**交 architect 的关键问题：** 复合 writer 是否可在现有 `P011GuardAdmit` 全边线性区间上一次准入；各 `f_e` 的 local/total 实际计费怎样既不重复也不漏；姿态提交与水更新在材料不可动时怎样独立提交而仍保持 run 失败身份；旧两轮 planner 与新 route 的 routeId/备份/回滚/telemetry 怎样隔离；新角速率能否满足 P95≤100ms。任一项无解，应返回 designer 修订，不靠旧 checker 豁免求绿。
