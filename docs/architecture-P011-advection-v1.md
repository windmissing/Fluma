# P-011 方案 3 悬浮平流工程设计 v1：advection 通道工程化 + 起沙 b 落点 + developer 任务书

日期：2026-10-23。责任：architect。状态：**ENGINEERING_DESIGN / READY_FOR_DEVELOPER（a 阶段先行：仅搬现有悬沙）**——b 阶段（起沙链）实现解锁等 P-3 manager 独立批准，工程接口本稿已预置。依据：[方案 3 算法设计](algorithm-P011-phase2-suspension-advection-v1.md)（已验收）+ [designer 移交稿](task-P011-phase2-advection-architect-v1.md)（P-2/P-4/P-5/P-6 四项——**本卡照此范围**）+ [§28 退款核验](architecture-P011.md)（P-1 已修 commit `8df1aae`：refundLocal 只退实际扣额——**前置①已达成**）。基底：**correction 终审 17:32 维持候选归档（无 correction 快车道）**——108run 82/108 封存（EARLY_STOPPED，closure-receipt），dryrun 管线 D vs A median −2.7280 CI 全负（INTERIM_NOT_SUPPORTED_SIGNAL 方向维持）；转正分支预案保留不删（§7）。

代码现状核对（2026-10-23）：`diffuseSuspension:4445`（无预算历史语义——advection 的对照面）；`saveMotionSnapshot:3012`（七数组快照，collect 模式专用 :3085）；step 序 `waterLevel→diffuseSuspension→[snapshot]→settle→percolate→creep→[deepCreep]→relax→lift`；通道枚举含 deepCreep=6/correction=7（S4 已实现）；ledger schema 已 v5（S3 占用——advection 扩展走 v5 内追加字段或 v6，§5 裁定）；`liftByShake:5151`/`splitTopV2:5231` 现存；recordSuspensionDiffusion 遥测先例 ：717。

## 0. 总体结构裁定

**advection = 第七个 writer 通道，双 slot 位（collect 侧+apply 侧同址），S-invariant + 真实扣费**。与 diffuseSuspension 的根本区别：diffusion 是无预算的浓度均衡（历史语义保持），advection 是**受四层预算约束的定向搬运**（每相每边 q 计双端 local+total）——两者并存不合并，遥测分池。调度落点照 designer 建议（§2 详证）：**settle 之前、motion snapshot 之后**——与 S4 的 bottomCreep 后位置构成「悬浮域在沉积域之前」的完整次序。

## 1. P-2 advection 通道账本身份（工作 1）

### 1.1 通道枚举与 routeId

```ts
// P011TiltPlanningPass.ets 扩展（deepCreep=6/correction=7 既有）：
export type P011ExistingChannelId = ... | 'advection';   // rank = 8
// RateCapRemaining 加 advection 字段（记账+限流——与 S4 首版不限流不同：
// advection 的 channel cap 是 designer §2 明确的截断维度 q_admit 的第五项）
export const P011_ADVECTION_ROUTE_ID = 'suspension-advection-v1';
// WaterSandField：
private advectionRouteActive(): boolean {
  return this.naturalObservationRouteId === P011_ADVECTION_ROUTE_ID
    && this.advectionParams !== null;    // { kDrag, uCap, channelRateCap, advectionGain }
}
```

**routeId 互斥预登记**：advection 与 S4 deepCreep、correction 三 route 互斥（`setNaturalObservationRouteId` 单值字段天然互斥）；同开须 manager 授权（S4 §5 共同不变量同款声明）。**advectionGain=0 = 位精确 OFF**（designer P-2 要求）：gain=0 时请求生成直接跳过（不进 collect、不进 plans、不进 ledger advection 块）——与 route 未绑定的 OFF 语义相同（单测：gain=0 vs 未绑定，锚点序列逐位一致）。

### 1.2 请求生成（collect slot）与同快照防超卖

**调度落点（裁定，designer §2 建议 + snapshot 位置论证）**：

```text
substep(dt)（collect 模式）与 applyMotionChannels（apply 模式）同址插入：
  waterLevel(dt)
  diffuseSuspension(dt)
  saveMotionSnapshot()          // 既有 :3085（仅 collect 模式）
  → 【新增】advectionSlot(dt)   // ★ 位置：snapshot 后、settle 前
  settleSandV2(dt) ...
```

**为什么在 snapshot 后**：①designer §2 明令「所有边的请求从同一悬沙快照生成」——snapshot 正是冻结点：全部 127 边读 motionSnapshot.suspH/suspL（collect 态），apply 侧读 restoreMotionSnapshot 后的同一状态（apply 链先 restore 再走通道——预演/apply 同源）；②**为什么在 settle 前**：起沙 b 的沙「最快下一子步进入 advection」（designer：不允许一子步起沙后瞬移）——settle 先跑会把本子步新悬沙立刻落回，advection 拿不到；advection 先跑则本子步搬运的是**上一子步已存在的悬沙**（快照语义），新起沙下一子步才被搬运——正是 designer 的次序语义。③与 S4/deepCreep 的次序无冲突（deepCreep 在沉积链中段，advection 在悬浮链首——两域不相邻）。

**同快照防超卖（多出边同供体）**：请求生成阶段对每列 c 汇总其全部出边请求 `Σq_out,c`；若 > 快照 susp_p[c]，按**供体比例缩放**（`scale = susp_p[c]/Σq_out,c`，各出边请求 ×scale——designer §2「按供体比例或确定性缩放」取前者：比例保形+确定性）；缩放后的请求才进准入链。**禁止把更新后的入流当同子步下一边的免费供体**（快照冻结的自然结果——所有边读同一快照，先执行的边不改变后执行的边的供体可见性……**修正**：准入是逐笔的，笔间账本（双端 local/total/channel）是串行扣减的——**悬沙库存的供体扣减也是串行的**：边 e1 准入 q1 后，e2 的供体余量=susp_p[donor]−q1（**不是快照值**）——这与「同快照生成」不矛盾：**快照约束的是请求生成**（防请求膨胀），**准入链约束的是实际扣减**（防超卖）——两级防护。实现：请求从快照生成（冻结读），准入时对 donor 的**运行时余量**再 min 一次（`q_admit ≤ susp_p[donor]_runtime`）。

### 1.3 预算扣费与四层回写

每相每边一笔（H/L 各 127 边上限，实际活跃边远少）：

| 层 | 扣法 | 回写 |
|---|---|---|
| 双端 local | donor 与 receiver 各扣 q（照 pair 先例） | clip 拒绝量退两端（§28 P-1 修复后的语义：只退实际扣额——但 advection 笔内原子：先扣后写失败则整笔回退，无部分扣留） |
| total | 每笔扣 q（**不是 2q**——单相单边的真实位移量是 q；H/L 各笔独立计） | 同上 |
| channel cap | advectionRateCap·dt 每子步池，笔间串行扣 | clip 量记 channelClipped |
| bottom | 不扣 | — |

**与 300/s 分母的关系**：advection 请求计入 totalMotionBudgetRequested（collect 阶段），进入既有 scale 分母——无第二额度、无快车道（红线）。**幂等式断言**（单测）：每笔 `requested = granted + localClipped + totalClipped + channelClipped + inventoryClipped + capacityClipped`（六段分解——designer P-2「requested=granted+各原因截断」的逐字段定义）。

### 1.4 预演/apply 同序伪代码

```text
advectionSlot(dt):
  if (!advectionRouteActive()) return;           // route+gain 双门
  // ── 请求生成（读快照——collect 模式读 motionSnapshot，apply 模式读当前态=restore 后）──
  for e in 0..127:                               // 边序固定（字典序 §20.6.2）
    g_e = S[e+1] − S[e];  t = tan(θ_eff)         // θ_eff = planner 接受角（只读）
    F_e = t − g_e;  u_e = clip(kDrag·F_e, ±uCap)
    ν_e = min(1, |u_e|·dt)                        // CFL≤1（dx=1）
    for p in {H, L}:                              // 相序固定
      donor,recv = (e,e+1) if u_e>0 else (e+1,e)
      q_req[p,e] = ν_e · snap.susp_p[donor]       // 快照读
  // ── 供体超卖缩放（快照级）──
  for c: Σq_out[c] = Σ_e q_req[p,e] where donor=c → scale → q_req 修正
  // ── 准入链（笔间串行，运行时余量）──
  for e in 边序: for p in 相序:
    q = min(q_req[p,e], susp_p[donor]_runtime, receiverSuspCapacity(recv),
            channelRemaining, localRemaining[donor], localRemaining[recv], totalRemaining)
    if q > 0: 原子写 susp_p[donor]−=q; susp_p[recv]+=q; 扣四层; 遥测一笔
  // S 不变（ΔS=0）——不调 updateSandTop（快照内 sandTop 本来未变）
```

preview 侧（allocatePreviewPlans 的 advection 槽位）同伪代码、同快照源（shadow 的 motion snapshot 镜像——advection 读 shadow suspH/suspL，R-3-a 六数组内**已含** suspH/suspL，零新数组）、同笔序——plans 数组多一个 advection 条目（deltaSandTopByColumn 全零——S-invariant plan 特征，verify 逐项比较自动覆盖）。

## 2. P-6 advection guard 语义（工作 4）

**S-invariant 登记制（percolate/deepCreep 先例）**：

1. **不调 materialGuardAdmit**：advection 的 ΔS=0（suspH/suspL 列间对倒，bh/bl/mh/ml 不动）——`d_e=(S[e+1]−S[e])−tanθ_eff` 数值严格不变（designer P-6 原文），guard 的角度数值缩减无对象。受影响边集登记为空（plans 的 advection 条目 deltaSandTop 全零即登记）。
2. **内部有效性自查（每笔）**：susp 非负（双端）、`S+U_H+U_L ≤ waterTop`（receiver 容量——designer §2 容量约束）、有限性；失败该笔跳过+遥测 reason（不部分写——微事务原子性照 S4 §2.4 先算后写）。
3. **BLOCKED 语义不变**：若子步因其它 writer/planner 进 BLOCKED，advection 已执行的笔**随子步整体回滚**（restoreMotionSnapshot）——不得靠平流日志把 BLOCKED 记成成功（designer P-6 红线）。advection 自身不产生 BLOCKED（无角度约束）。
4. **§19.5 writer 清单更新**：advection 以「S-invariant+登记」类目加入（percolate/deepCreep 同行）。
5. **措辞红线**：禁止「平流被 guard 豁免所以整步安全」——advection 的安全性来自库存/容量/预算自查，guard 只是**不适用**而非豁免。

**调度落点（已在 §1.2 裁定：snapshot 后 settle 前）**——与 designer 建议一致，本节补 snapshot 位置论证（designer P-2 要求「若 architect 证明 snapshot 位置需调整，两轮时序须一起改」：本稿**不调整**——motion snapshot 位置（diffuseSuspension 后）对 advection 恰好正确：diffusion 先均衡浓度（无预算历史语义），advection 再定向搬运（读均衡后快照）——两级悬浮更新次序清晰，preview/apply 同源）。

## 3. 起沙来源 b 的落点（P-3 工程化，实现等 manager 批准）

### 3.1 a_c 门控实现位置

**独立于 liftByShake（不共享路径，共享底层 splitTopV2）**：

| 项 | liftByShake（既有） | b 起沙源（新增） |
|---|---|---|
| 驱动 | shakeEnergy（摇晃） | **a_c 门控**（真实 sliding 事件） |
| 提取 | splitTopV2（混合→轻→重顺序） | **同款 splitTopV2**（暴露表层顺序——designer §3「分相按实际 splitTopV2 的暴露顺序」） |
| ΔS | ≠0（mh/ml 减、悬沙加——**逐笔 §19.5 guard**） | 同（ΔS_c=−q 联合预演邻边，designer §3 表） |
| 预算 | 既有 lift 账 | source 独立笔：local c=q、total=q（不记横向运输） |
| 通道 | lift（既有 rank 4） | **新 sub-slot：entrainSource**（advection 块内分列——`sourceKind: 'shake-legacy'|'shear-b'|null` 遥测区分） |

**a_c 的数据来源（designer §3 闭合的工程化）**：`a_c = clamp(已准入表层通量 / 该通道额度, 0, 1)`——具体取 `relaxAngle` 当子步对该列所在边的**真实 granted 量 / 该子步 relax 通道可用额度**（ granted>0 即有真实滑动活动；**不含**：请求未准入、commandTilt、dune 目标、tan4 静态请求——±15° 静置平台 relax 有 grant 但无 sliding 转换时 a_c 的保护语义由「sliding 状态机的 active 转换事件」而非 grant 量决定——**修正细化**：a_c 触发条件 = 该列参与的边在本子步发生 **active 状态转换**（inactive→active，读 relaxSliding 转换沿）**或** shakeEnergy>阈值；仅 grant 无转换不算（防静置平台起沙——designer §3「持续 tan4 请求不算活动证据」的机械实现）。转换沿的检测点：relaxAngle collect 扫描内（:4375 一带状态机更新处）登记 per-edge 转换事件数组（`edgeActivatedThisSubstep[127]`，beginSubstepMotionBudget 清零）——advection 下一子步读。

### 3.2 q_req 与 E_c 的字段来源

```text
q_req,c = dt · k_entrain · a_c · E_c
E_c = 暴露表层库存归一化因子 = (mh[c]+ml[c]的表层可提取量) / 参考厚度
  ——工程定义：E_c = min(1, 表层库存/S_ref)，S_ref=定标参考厚度（候选 8 材料单位）
  表层库存 = splitTopV2(c, ∞) 会提取的量（混合层+轻层+重层的暴露序总和——只读探测，不实提取）
q_admit,c = min(q_req, 表层实际库存, waterCapacity(c), channelCap 余量, local, total)
准入后原子写：deposited_p −= q（splitTopV2 按相提取）；susp_p += q；§19.5 以 ΔS_c=−q 联合预演邻边（p011GuardConstraintsForDelta——writer 型 guard，照 settle 先例）
```

**b 的调度位置**：liftByShake 之后（apply 链尾）——新起沙最快下一子步进 advection（§1.2 的次序语义）；同子步内 b 在 advection 后执行保证不瞬移。routeId：b 阶段与 a 共用 `suspension-advection-v1`，但 want 参数分层（`p011s3adv` 的 a/b 开关位——见 §6 任务书）保证 a/b 可独立开关与归因（designer §5「起沙、平流、沉降三段可分别开关」）。

## 4. P-4 shadow 证书架构（工作 2）

**证书 = 全链可复现轨迹的结构化导出（生成工具设计，本次不生成）**：

```ts
// tools/export_p011_advection_certificate.py（实现后跑）：
// 输入：run 的 ledger.ndjson + 构建身份（sourceCommit/buildSha256）+ 初态 JSON
// 输出：certificate.json——每子步一条记录：
//   { substepId, thetaAccepted, plannerStatus, limitingEdge,
//     source: { kind, perColumn: [{c, qReq, qAdmit, reason}] },        // b 阶段
//     advection: { perEdgePerPhase: [{e, p, u, nu, qReq, qAdmit,
//                   clipReasons: {local|total|channel|inventory|capacity},
//                   guard: 'S_INVARIANT_REGISTERED' }] },
//     settle: { perColumn: [{c, p, q}] },                               // 既有遥测
//     others: { relax/deepCreep/correction per-channel 真实 grant },     // 既有遥测
//     budgets: { local/bottom/total/channel 四层 used/remaining（子步末快照） },
//     moments: { MdepH, MdepL, MsuspH, MsuspL, RM_H, RM_L },            // P-5
//     invariantCheck: { tan30MaxErr, phaseH/L residual, capacityMax,
//                       SPlusU_le_waterTop violations: 0 } }
// 帧间稳定性：末 1s 悬沙占比、终点形状（sandTop/bh 摘要）
// 证书判定字段（导出时计算，不放宽）：SOURCE_UNAVAILABLE / SHADOW_BLOCKED /
//   BUDGET_EXHAUSTED / CAPACITY_BLOCKED / EFFECT_INSUFFICIENT / TRACEABLE
```

**P-4 前置链**（designer 移交稿依赖顺序）：①P-1 已修（8df1aae）✅；②16 列 q=0.01 条件包复核（实现后首跑——验证通路非可达性）；③128 列/5.1s 正式证书（b 阶段批准后）。**证书的证据边界**（designer §6）：条件包≠产品可达——证书报告「必要上界与真实可提交链的差距」，不将条件包写成可达。

## 5. P-5 分域遥测（工作 3）

### 5.1 Mdep/Msusp 字段与 R_M 对账式

```ts
// ledger schema v5 → v6（v5 已被 S3 占用；advection 追加走 v6）：
//   context.advection = { routeId, gain, kDrag, uCap, channelRateCap,
//     perSubstep: { MdepH, MdepL, MsuspH, MsuspL, RM_H, RM_L,
//                    diffusionFluxH, diffusionFluxL,     // 旧 diffusion 单列（防扣错账）
//                    advectionFluxH, advectionFluxL,     // 有符号边通量和
//                    otherPhaseTransportH, L } }          // bottom/relax/deepCreep 的相输运（既有遥测聚合）
// R_M,p = [Mdep,p(t+dt)+Msusp,p(t+dt)] − [Mdep,p(t)+Msusp,p(t)]
//         − Σ_e signed(advection_p,e) − Σ_e signed(other_p,e) − Σ_e signed(diffusion_p,e)
// 容差：|R_M,p| ≤ 128·EPS·max(1,|Mdep|+|Msusp|)（列数×机器精度×量级——浮点容差来源显式）
// 旧 run 缺字段：按 0 容忍（向后兼容惯例），R_M 不计算（无新字段即无对账义务）
```

### 5.2 防重复计功的账本结构（designer §4 表的落地）

| 交易 | 记账 | 防重复计功 |
|---|---|---|
| 起沙（同列） | Mdep −cq、Msusp +cq | **总矩净零**——两域变化不得绝对值相加 |
| 平流（跨列） | Msusp ±q（Mdep 不动） | 每跳收费（每相每边 q）——不按跨列距离打折 |
| 落沙（同列） | Mdep +cq、Msusp −cq | 同起沙 |
| 既有 writer | 各自通道遥测的 signed flux 进 otherPhaseTransport | 不填入 advectionFlux |
| 旧 diffusion | recordSuspensionDiffusion 既有遥测聚合进 diffusionFlux | **单列**（designer P-5「不能扣在 advection 头上」） |

**性能**：Mdep/Msusp 是 O(N) 求和（每子步 128×4 次乘加——可忽略）；遥测开关：per-substep 记录仅在 advectionRouteActive 时开启（OFF run 零开销——route 隔离的性能红利）。

## 6. developer 任务书（工作 5）

### 6.1 实现拆解（a→b 两阶段）

| # | 模块 | 内容 | 阶段 |
|---|---|---|---|
| A-1 | 通道类型 | 枚举加 'advection'（rank 8）+RateCapRemaining 字段+routeId 常量 | a |
| A-2 | advectionSlot 主体 | §1.4 伪代码全量（请求生成/快照读/超卖缩放/准入链/四层扣费/遥测） | a |
| A-3 | route+参数 | 双条件门+`P011AdvectionParams`（kDrag/uCap/channelRateCap/gain）+want 参数 `p011adv`（`adv:<gain>:<kDrag>:<uCap>:<cap>` 五段式，非法拒绝整 want） | a |
| A-4 | shadow 槽位 | allocatePreviewPlans 的 advection 槽位（shadow suspH/suspL+影子四层账本，同序同规则）+plans 条目+verify 覆盖 | a |
| A-5 | 遥测 | ledger v6 advection 块（per-edge 笔+分域矩+R_M）+幂等式断言字段 | a |
| A-6 | 单测 a | §6.2 1-8 项 | a |
| B-1 | entrainSource | §3.1/3.2 全量（a_c 转换沿门控+splitTopV2 只读探测+writer guard+独立账） | b（等 P-3 批准） |
| B-2 | 证书工具 | tools/export_p011_advection_certificate.py（§4 契约） | b |
| B-3 | 单测 b | §6.2 9-12 项 | b |

### 6.2 单测范围（Node 通道）

**a 阶段（1-8）**：
1. **守恒恒等式**：随机悬沙初态 600s → 逐相 `Σ(bh+mh+suspH)`/`Σ(bl+ml+suspL)` 漂移 <1e-9；每列 ΔS=0 逐子步。
2. **矩账本 R_M≈0**：含 diffusion 单列的全对账式（|R_M,p| ≤ 容差）——构造 shake 起沙→advection→settle 全链。
3. **CFL≤1**：任意参数下 ν_e=min(1,|u|·dt)≤1（含 uCap 极值、F_e 极值边界）。
4. **同快照请求**：构造多出边共享供体——请求生成读快照（不受先执行边影响）；准入链运行时余量防超卖（供体扣空后第二边 q_admit=0）。
5. **容量/库存/预算截断**：六段分解幂等式逐笔成立（requested=granted+五类 clipped）；四层 used+remaining 恒等式。
6. **route 隔离逐位**：未绑定/gain=0 → 锚点序列与 HEAD 逐位一致（D-1 资产模式）。
7. **方向语义**：F_e>0 沿列号增大方向搬运（正倾角平沙→右移）；反号镜像；θ_eff 用 planner 接受角（非命令角）——断言 u_e 的符号与 accepted tilt 一致。
8. **性能**：128 列悬沙半满 600s，advectionSlot 每 substep <0.05ms（Node 计时；真机归 effector 配对块）。

**b 阶段（9-12，等 P-3 批准后执行）**：
9. **a_c 门控**：静置平台（relax grant>0 但无 active 转换）→ a_c=0 零起沙（±15° 保护语义）；真实转换沿 → a_c>0；shake 路径独立。
10. **E_c 归一化**：表层库存探测与 splitTopV2 实提取一致（只读探测不改变状态）。
11. **writer guard**：b 起沙的 ΔS=−q 联合预演邻边（合法态提交后合法；违规态 non-worsening）；BLOCKED/guard 拒绝不留半笔。
12. **证书 OFF-ON 配对**：同 seed/初态/输入的 gain=0/ON 配对证书导出——per-substep 可复现（同 seed 重跑逐位一致）。

### 6.3 验收口径（designer §5 五量+四否决）

**五量**（effector 真机/离线先跑 Node）：低侧沉积富集 C_dep,p（H/L 分报）、横向位移 ΔMtotal,p（ON 与倾角同向+反转变号）、可见沙顶/bh 位移、时间与稳定（首响应/5.1s 曲线/末 1s/悬沙占比）、安全性能观感（全相守恒/127 边 tan30/预算恒等/O(N) 帧时/effector 判「向低处聚」）。

**四否决**（不调宽安全线）：合法 source 频率零 / granted 长期近零 / Mtotal 方向与接受角系统性相反 / 终点沉积富集对 OFF 无可分辨增益——分别归因驱动、预算容量、速度闭合、效果通路。

**a 阶段的验收预期（诚实登记）**：KF-1/KF-2 两原场景 a 新增通量恒 0（零悬沙初态）——**a 的安全证书与悬沙位移验证用 shake 造悬沙的场景**，不以两原场景零收益判失败（designer §3 表 a 行「零收益是机制预测」）。

## 7. correction 基底分支（预登记）

**主线（本稿冻结）：无 correction 快车道**——108run 82/108 封存+管线负信号（median −2.7280，CI 全负）方向维持归档；advection 的请求分母、预演、OFF 基线全部按自然 relaxAngle 基底冻结。**转正分支预案（保留不删）**：若未来 correction 意外转正，须重算——①请求分母（correction 预留 R_limit 与 advection 请求的竞争序：advection 在 collect 计入 requested 后，correction 的预留分母含 advection 请求量——预演同分母）；②OFF/ON 配对基线重出（基底变了配对必须重做）；③P-4 证书重跑。禁止把 correction 的独立修正额度借给 advection（designer §6 红线照抄）。

## 8. 红线复述

不放宽 300/s 与 §19.5 guard（advection 是消费者；S-invariant 登记不新增拦截义务；b 起沙是 writer 型逐笔 guard）；Mdep/Msusp 分域不重复计功（总矩净零/每跳收费/diffusion 单列）；不设不受 total 约束的快车道（请求计入分母）；数值候选（kDrag/uCap/channelRateCap/k_entrain/S_ref）全标待定标；246/251/252 结构引用不冒领（u 是项目代理速度非水流——设计稿 §1 边界照抄）；工作区 commit（合入等 manager 确认）；b 阶段实现等 P-3 manager 独立批准。

## 9. 结论与路由

四项工程化落盘：P-2 账本身份（第七通道 rank 8+routeId+同快照两级防超卖+四层扣费+六段幂等式）/P-6 guard 语义（S-invariant 登记+内部自查+BLOCKED 随子步回滚+措辞红线）/P-4 证书架构（结构化导出契约+前置链+条件包边界）/P-5 分域遥测（schema v6+R_M 对账式含 diffusion 单列+防重复计功表）；起沙 b 落点（a_c 转换沿门控+splitTopV2 共享底层+独立 sub-slot+writer guard）。**调度裁定：advection 在 motion snapshot 后、settle 前**（快照冻结+起沙下一子步语义+两级悬浮更新次序）。**实施次序照 designer 推荐：a 先行（安全证书+悬沙位移）→ b 等 P-3 批准 → b 有沉积收益才谈转正**。developer 任务 A-1..A-6（a 即刻）+B-1..B-3（等批准）；单测 12 项+验收五量四否决。

变更记录：2026-10-23，依据 manager【P-011·方案3工程设计】任务产出。P-2/P-4/P-5/P-6 四项+起沙 b 落点+任务书；基底=correction 归档（17:32 终审，转正预案保留）；调度落点=snapshot 后 settle 前；P-1 前置已达成（8df1aae）。
