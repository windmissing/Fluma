# P-011 S4 深层蠕动工程设计 v1：逆向交换通道的工程落点与离线原型任务书

日期：2026-10-23。责任：architect。状态：**ENGINEERING_DESIGN / READY_FOR_DEVELOPER（离线原型）**——仅授权离线原型（Node 通道+默认关闭的 ArkTS 通道代码），真机/合入排期等 correction 裁决落定后另批。依据：[S4 开题](design-P011-S4-deep-creep-bh-v1.md)（manager 15:20 验收，含 §4.1 势能式独立验证——**本稿照搬不重估**）。代码现状核对（2026-10-23）：通道枚举+rank 在 `P011TiltPlanningPass.ets:14-130`（correction=rank 6 先例）；`applyMotionChannels:2588` 顺序 settle→percolate→creepBottom→(correction|relax)→lift→updateSandTop→audit；route 隔离先例 `setNaturalObservationRouteId:2069`+`correctionRouteActive:2094`（双条件门）；pair 型 guard 先例 `:4183`（relax 中央边+外侧）；pair 型 local 双端扣账先例 `:124-125`（donor/receiver 各扣）；108run 正式批进行中（batch-log r072/108，15:36）——**批内不合入，工作区开发**。

## 1. 调度落点（工作 1）

### 1.1 裁定：独立第六通道 `deepCreep`，位置在 bottomCreep 之后、relax/correction 之前

```text
applyMotionChannels（v1 后）：
  settle → percolate → creepBottom → deepCreep(S4, 新增) → (correction | relax) → lift
rank：deepCreep = 6（correction 顺延为 7；suspensionDiffusion 5 不动——rank 是纯字典序
工具值，一次性平移，无兼容负担：rank 只用于 preview 排序与 plans 数组顺序，不进 ledger）
```

**为什么独立通道而非并入 bottomCreep**：①bottomCreep 的驱动是 `targetShape*` 请求（目标形状残差，:225-226）——S4 的驱动是 bh 界面势能梯度，语义不同源，复用即把两种物理混进一个 writer 的请求账，审计无法分池归因（开题 §1「可独立归因」直接要求）；②bottomCreep 有 10/s 底部专账（vBottomCreepMax）——S4 开题 §6 明令「不能无说明复用既有 10/s bottom 专账」；③通道枚举扩展是既有模式（correction 先例，纯类型追加+rank 一行），成本低于在 bottomCreep 内做条件分支的语义污染。

**为什么在 relax/correction 之前**：deepCreep 是 ΔS=0 通道（逆向交换），先跑它意味着 relax/correction 随后看到的 S 与 sandTop 残差**不因 S4 而变**（S4 只改 bh/bl 内部配比）——relax 的残差判定基准不被 S4 污染，两通道相互作用降为纯预算竞争（谁先扣谁多得），无状态耦合。反向顺序（relax 先）则 relax 的 grant 改变 S，deepCreep 的 bh 梯度评估对象不变但 q≤bh_i 库存约束在 relax 后的 bh 上验——先 deepCreep 让 S4 在当子步拿到的 bh 是「本子步最完整的状态」之前的一致视图，与 shadow 镜像时序对齐简单。

### 1.2 预算分配：不设独立 cap（首版），2q 全计 total+双端 local

开题 §3 红线「不能因净沙顶位移零而免费」的落地：

| 账本 | 扣法 | 理由 |
|---|---|---|
| total | 每对交换扣 **2q**（重沙 i→j 一笔 q + 轻沙 j→i 一笔 q，两笔都计） | 开题 §3：两笔相反搬运按实际运动量 2q 计入共享 300·dt |
| local | donor 端与 receiver 端**各扣 q**（列 i 扣 q（重沙出）、列 j 扣 q（轻沙出）——**注意方向**：逆向交换的两笔搬运出发端分别是 i（重沙）与 j（轻沙），local 账按搬运出发端计） | 照 pair 先例 :124-125（donor/receiver 各扣 amount） |
| bottom 专账 | **不扣**（S4 不是底部通道） | 开题 §6：不复用 10/s bottom 子账 |
| channelRateCap | `deepCreep` 字段登记实际使用量（类型扩展后**记账但不限流**——首版不设 cap，开题 §6「channel cap 候选」留到定标后再决定是否启用限流值） | 先观测后定标：没有 cap 数据就没有定标输入 |

**与 300/s 的兼容性论证**：S4 是新增消费者不是新增额度——total 池 300·dt 由全部通道竞争，S4 上电后 settle/creep/relax 可用额度单调下降。**可达性预判**（开题 §4.1 单对下界的工程含义）：2-8px 的 P90−P10 幅度按单对下界 q≥A_px/(2·pxPerMaterial) 需 2q≥A_px/pxPerMaterial 的预算支出——128 列典型几何 pxPerMaterial≈2.4（192 容量/约 80 显示 px per material 单位换算按真机几何标定），8px≈3.3 材料单位→单对 2q≈6.7；若 8-40 列宽缓结构同时活跃 ~10 对，瞬时需求 ~67/子步·dt(1/60)≈1.1/s 量级总通量——**远低于 300/s**，预算不构成 S4 的物理瓶颈（长时标慢速率是设计特征不是预算不足）。此项进离线原型 §4 验证清单实测确认。

### 1.3 guard 语义：S-invariant 登记制（percolate 先例），不设数值 guard

deepCreep 的 ΔS=0（bh_i−q+bh_j+q 与 bl_j−q+bl_i+q 对每列 S=bh+bl+mh+ml 严格不变）——与 percolate（列内 mh→bh）同属 S-invariant 通道。落地：

1. **逐笔登记不拦截**：每笔交换进 probe 遥测（`recordDeepCreep(c, requested, allowed, q, drive, reason)`），照 `recordPercolate:4057` 模式；**不调 materialGuardAdmit**（guard 只管 S 沙顶残差，S4 不碰 S——调了恒 admissible，纯开销无语义）。开题 §3「guard 调用/审计保留」按登记形式满足。
2. **内部有效性自查替代 guard**（事务级，每笔或每批一次）：`0≤bh≤S`、`0≤bl≤S−bh`、q≤bh_i、q≤bl_j、无界面反转（q≤(bh_i−bh_j)/2 由势能式保证）——失败按整笔回滚（§2.4 原子性）。
3. **§19.5 五 writer 清单更新**：deepCreep 以「S-invariant+登记」类目加入（与 percolate 同行），不增加拦截义务。

### 1.4 routeId 隔离落地

照 correction D-1 先例逐项复刻：

```ts
export const P011_DEEP_CREEP_ROUTE_ID = 'deep-creep-bh-v1';
// WaterSandField 追加：
private deepCreepRouteActive(): boolean {
  return this.naturalObservationRouteId === P011_DEEP_CREEP_ROUTE_ID
    && this.deepCreepParams !== null;   // 参数已配置（armed 语义）
}
// applyMotionChannels：
if (this.deepCreepRouteActive()) this.applyDeepCreep(dt);
// 双条件门语义与 correctionRouteActive 相同：route 绑定（充分）+ 参数武装（必要）——
// 两条件缺一即完全旁路，行为与现行 HEAD 逐位一致
```

**默认关闭**：`deepCreepParams=null`（构造/reset 置 null）→ 全部现有 run（SLIP/观测①b/v3 候选/correction 矩阵）零影响；want 参数 `p012route` 不动——S4 用**独立 want 参数 `p011s4`**（`deep:<k_deep>:<mu_y>:<hMin>` 三段式，照 p011route 解析模式追加；非法值拒绝整 want），避免一个 route 参数承载两个互斥路线的身份。**同 seed 配对**：OFF/ON 臂共享 seed/初态/输入序列（离线原型 §4 的配对口径）。

## 2. 逆向交换实现设计（工作 2）

### 2.1 列对选择规则（开题留白，本节裁定）

**裁定：能量梯度最大优先（贪心逐对），每子步最多 K=4 对，字典序 tie-break**。

```text
候选边生成（每子步，deepCreep 激活时）：
  对每相邻列对 (i, i+1)：
    Δbh = bh[i] − bh[i+1]
    D_raw = (ρH−ρL)·g·Δbh − μ_y·p_eff(i,i+1)        // 开题 §4.1 D_ij 原型
    D = max(0, D_raw)                                  // 屈服门：负值零驱动
    候选条件：D>0 且 Δbh>2·EPS 且 bh[i]>hMin 且 bl[i+1]>hMin（双端库存+最小层厚）
  排序：按 D 降序取前 K=4 对；D 相等按 i 升序（字典序，确定性——§20.6.2 先例）
  每对请求：q_req = min(k_deep·dt·D, Δbh/2, bh[i], bl[i+1], localRemaining_i, localRemaining_j, totalRemaining/2)
```

**为什么选能量梯度而不是曲率**：①开题 §4.1 的 D_ij 就是势能差的一阶近似——能量梯度优先是势能式推导的自然执行（贪心最陡下降，每子步固定预算下 ΔE 下降最大）；②曲率最大（二阶）选点在均匀坡上会选到平肩部（曲率大但梯度零——正是开题对称性闸门要防的「自发波形」入口）；③确定性可实现：D 是标量，排序+tie-break 纯整数列号，无浮点排序歧义（§20.7-□5 先例）。

**p_eff 代理（首版最简）**：`p_eff(i,j) = ρ_L·g·(S[i]+S[j])/2`（上覆平均柱压，两列均值）——只用既有字段，零新状态；μ_y 定标域 [0, 0.5]（0=无屈服门，全梯度驱动）。**不做**浮力校正/有效应力分解（开题声明势能式只覆盖简化条件，首版把简化条件如实带上——若真机倾斜场景能量账异常，定标阶段再升级 p_eff）。

**K=4 的依据**：300/s 预算下单子步 2q 总量 ≤5（300/60）——K=4×q 上限（q≤Δbh/2，典型 Δbh≈2-6）在预算内留出其它通道余量；K 也是空间尺度的间接控制（开题 §2 主起伏 8-40 列，K=4 对/子步×60 子步/s 的作用点扩散由 D 场自然分布——参数扫描验证）。

### 2.2 q 步长控制：四重 min 截断（势能约束的实现）

`q = min(k_deep·dt·D, Δbh/2, bh_i, bl_j)` 的四重语义：

| 截断项 | 语义 | 违反后果 |
|---|---|---|
| k_deep·dt·D | 速率上限（定标参数） | 无——速率自由参数 |
| **Δbh/2** | **势能不过冲**（开题 §4.1：q≤Δbh/2 保证 ΔE<0 且界面不反转） | 界面反转=能量注入，违反热力学方向性（单测断言） |
| bh_i | 重沙库存（donor 不透支） | bh<0 破坏相非负（audit 拦截） |
| bl_j | 轻沙库存（receiver 端轻沙不透支——逆向交换的轻沙 donor 是 j） | bl<0 同上 |

再加预算截断：q 还要 ≤min(local_i, local_j)（双端 local 余量）且 2q≤totalRemaining——**截断顺序固定**（速率→势能→库存→local→total，逐级 min，写死不许重排：重排会让「预算截断优先于势能约束」的极端情况下 q 超过 Δbh/2——虽然四重 min 数学上交换律成立，但浮点边界+审计对账要求固定顺序的可复现性）。

### 2.3 与 relaxAngle 并发的顺序语义（同列同子步被两通道触碰）

调度序（§1.1）：deepCreep 先于 relax。同列 c 被 deepCreep（作为某对的端点）和 relaxAngle（作为某边的 donor/receiver）同时触碰时：

1. **状态可见性**：deepCreep 改 bh/bl（S 不变）→ relaxAngle 的残差判定读 S（`sandTopScratch`），**S 未变**——relax 的决策输入不受 deepCreep 影响（这是 ΔS=0 的直接红利）。
2. **预算竞争**：deepCreep 先扣 local/total → relax 的 clipLocalPair 看到的余量已扣减——relax 被**量上挤压**但**语义上无损**（预算裁剪是既有机制，A-1 对账覆盖）。反向不公平：若 relax 先扣，deepCreep 的 q 已按余量截断——两侧对称，先到先得；选 deepCreep 先是因为它量小（2q 总量预算占比 <2%）且确定性（无概率因素），先扣小通道让大通道的裁剪可预测。
3. **shadow 镜像**：deepCreep 进 shadow 管线（§2.5）——预演侧同序执行，verify 一致性比对自动覆盖。

**唯一例外情形**：relaxAngle 的 `relaxSliding` 状态机资格判定（候选 run）读的是 S 残差——不受影响；但 relax 的 apply 写 mh/ml（重沙转移进混合层）会改 bh 吗？核对：relaxAngle 的 grant 语义是 donor/receiver 的 S 平移（bh/bl/mh/ml 按比例拆分）——**若 relax 拆分比例涉及 bh**，deepCreep 先跑后 relax 拆分读到的是已交换的 bh——这是**合法的状态演进**（同子步多 writer 顺序语义，与 settle→creep 的既有链相同），不是竞态；shadow 镜像同序保证预演/apply 一致。

### 2.4 事务原子性（开题 §6「固定事务原子提交或全部回滚」）

每对交换是一个**微事务**：两笔（bh_i−q,bh_j+q 与 bl_j−q,bl_i+q）在预演通过后**一次写入**——实现为：先在本地变量算全部四个新值+四项自查（§1.3-2），全过才写回四个数组元素+扣账（local×2、total 2q、channelRateCap 登记一笔）；任一自查失败→该对跳过（不部分写）+遥测 reason。**不用 saveBackup/restoreBackup**（那是子步级机制，S4 微事务四元素原子写不需要——数组元素写不可分割，先算后写即为原子）。

**多对之间的顺序**：字典序处理（i 升序），前一对的扣账影响后一对的余量截断（预算串行竞争，确定性）——与 §20.6.2 的字典序遍历纪律一致。

### 2.5 shadow 镜像扩展

deepCreep 进 `allocatePreviewPlans` 镜像（R-3-a 管线）：

1. shadow 数组扩展：bh/bl 已在 shadow 六数组内（R-3-a 的 split 镜像本就维护 shadowBh/shadowBl——核对 ：192 shadowS 同批）——**零新数组**；deepCreep 的 shadow 执行读 shadowBh/shadowBl/local/total 影子账本，同序同规则。
2. `P011ChannelPlan` 的 plans 数组多一个 deepCreep 条目（requested/plannedGrant/budgetUse=2q/deltaSandTopByColumn=全零——S-invariant 的 plan 特征）；`verifyPreviewCommitConsistency` 逐项比较自动覆盖（deltaSandTop 全零两边一致即过）。
3. **deepCreep 的 q 决策读 shadow 态的 bh**（不是真实态）——预演纯度规则（§20.7-□2）不变；候选角预演时 deepCreep 的行为在不同 theta 下相同（ΔS=0 通道与倾角无关——**除非 p_eff 含 S 且 S 随 theta 预演变化**：p_eff=(S[i]+S[j])/2 在预演中读影子 S，theta 改变预演 S→p_eff 变→D 变→q 变——**这是 deepCreep 与 planner 的唯一耦合点**，shadow 镜像正确处理，无需特殊分支）。

## 3. 参数与定标计划（工作 3）

### 3.1 参数组（离线原型要扫的）

| 参数 | 域（首版扫描） | 语义 | 定标目标 |
|---|---|---|---|
| k_deep | {0.002, 0.005, 0.01, 0.02, 0.05} /s·单位梯度 | 速率系数（q_req∝k·dt·D） | T_bh 落入数十秒-分钟窗（开题 §2） |
| μ_y | {0, 0.1, 0.25, 0.5} | 屈服门（D_raw 减去 μ_y·p_eff） | 均匀场零通量的稳健裕度（μ_y 越大越保守） |
| hMin | {0.5, 1.0, 2.0} 材料单位 | 双端最小层厚门（bh/bl 低于此不出对） | 防薄层界面噪声交换 |
| K | 固定 4（不扫，结构参数） | 每子步最多对数 | — |
| (ρH−ρL)·g | 固定引擎现值（不扫——开题势能式的给定常量） | 驱动力常数 | — |

5×4×3=60 组合×每 run 120s 模拟×Node 通道 ~30s/组合（128 列 60fps 等效）→ **全网格 ≈30 分钟单机可跑完**——不留抽样，全网格。

### 3.2 定标判据（每组输出的验收量）

开题 §2 候选量的工程化（离线原型自动报告，不做 PASS/FAIL 判定——判据冻结归 tester）：

1. `A_bh`：P90−P10（去趋势，宽窗=有效列全宽的 1/2 移动均值）+ A_bh/S 比值——对照 2-8px 探索域（pxPerMaterial 按几何换算）。
2. `L_bh`：去趋势自相关首 1/e 距离 + 相邻列跳变 P95——对照 8-40 列主结构。
3. `T_bh`：单峰-单谷对的 50% 幅度到达时刻分布（从 run 内首个非零 q 起算）——对照数十秒窗。
4. 零通量闸门：均匀初态 run 的 Σq 恒 0（**硬断言，任何参数组违者该组直接出局**）。
5. 归因账：每子步的 NO_DRIVE/NO_INVENTORY/BUDGET_CLIPPED 计数（开题 §4 要求的分解）——判据冻结后 tester 按此分布挑可解释参数域。

### 3.3 四格消融（S4×C）的离线半边

离线原型只做物理半边（S4 OFF/ON 的 bh 形状+账本对照）；渲染半边（C ON 的条纹弯曲）等物理原型过审后由 effector 真机四格——**离线不模拟 C**（开题 §5-2 的层纹跟踪是渲染域任务，Node 通道无 renderer）。

## 4. developer 离线原型任务书（工作 5）

### 4.1 实现拆解

| # | 模块 | 内容 | 验证 |
|---|---|---|---|
| S-1 | 通道类型扩展 | `P011ExistingChannelId` 加 `'deepCreep'`；rank 表加行（deepCreep=6，correction→7）；RateCapRemaining 加字段；plans 数组自然扩展 | 编译+既有 81 用例零回归 |
| S-2 | deepCreep 主体 | `applyDeepCreep(dt)`：候选边生成（§2.1）+四重 min 截断（§2.2）+微事务原子写（§2.4）+扣账（2q total/双端 local/cap 登记）+遥测 recordDeepCreep | 单测 §4.2 |
| S-3 | route 隔离 | `P011_DEEP_CREEP_ROUTE_ID` 常量+双条件门+want 参数 `p011s4` 解析（fail-closed，照 p011route 模式） | 单测：route 未绑定零调用、字段逐位一致 |
| S-4 | shadow 镜像 | allocatePreviewPlans 加 deepCreep 槽位（读 shadowBh/shadowBl/影子账本，同序同规则）；plans 条目+verify 覆盖 | preview/apply 逐位一致单测（复用 R-3-a 资产模式） |
| S-5 | 参数与 want | `P011DeepCreepParams`（k_deep/mu_y/hMin）+configure 校验（照 §27.2 fail-closed：非 finite/负值拒绝）+deepCreepParams 武装路径 | 单测：非法参数拒绝+零状态变化 |
| S-6 | 定标脚本 | `test/run_s4_param_sweep.py`：60 组合全网格+§3.2 五项指标自动报告（JSON+摘要表）；种子固定 20261023 | 跑通全网格<1h |

### 4.2 单测范围（Node 通道，断言级）

1. **守恒恒等式**：随机非均匀初态 600s 模拟 → 逐相 `Σ(bh+mh+suspH)` 与 `Σ(bl+ml+suspL)` 漂移 <1e-9；每列 ΔS=0 逐子步断言。
2. **对称性闸门（硬断言）**：均匀初态（bh/bl/S 全列等值）任意参数组 → Σq=0 全程（连 D_raw=0 的边界：μ_y=0 时 D=(ρH−ρL)g·0=0——max(0,0)=0 零请求）。
3. **q≤Δbh/2**：构造 Δbh 大的初态 → 每笔 q 严格 ≤该对交换前 Δbh/2；界面无反转（交换后 bh_i≥bh_j 保持）。
4. **能量单调**：非均匀初态全程 E_c（开题 §4.1 公式重算）单调非增（每子步 ΔE≤0，允许=0 零交换子步）。
5. **2q 预算扣额**：每子步 deepCreep 的 total 扣额 == 2×Σq（对账恒等式）；local 双端各 q；bottom 专账零扣额。
6. **原子性**：构造 bl_j 不足的场 → 该对零写入（bh 四元素交换前后逐位不变）+遥测 reason=NO_INVENTORY。
7. **route 隔离**：p011s4 未传/非法 → applyDeepCreep 零调用、行为与 HEAD 逐位一致（锚点序列复用 D-1/U11 资产模式）。
8. **shadow 一致**：S4 ON 的 run preview/apply plans 逐位一致（verifyPreviewCommitConsistency 全程 PASS）。
9. **性能**：128 列 60fps 等效 600s，deepCreep 每 substep 开销 <0.02ms（Node 计时——真机帧预算 0.15ms 的一部分，留足余量）。
10. **归因分解**：三种拒绝原因（NO_DRIVE/NO_INVENTORY/BUDGET_CLIPPED）各构造一例断言计数正确。

### 4.3 离线验收口径（architect 复核原型质量，非效果验收）

1. **bh 界面起伏**：P90−P10 在 2-8px 探索域内存在至少一个参数组（不是全组——全组达标反而是过驱动嫌疑）。
2. **均匀对照零驱动**：全部 60 组的均匀臂 Σq=0。
3. **账本合格**：守恒/预算/原子性单测全绿。
4. **归因可解释**：挑出的候选参数组其 NO_DRIVE 占比、T_bh 分布、L_bh 落域在报告里有解释链（为什么这组像目标视频）。
5. 四格消融的物理半边（S4 OFF/ON 配对快照序列）落盘 artifacts 供后续渲染半边使用。

## 5. correction 两分支衔接（工作 4，预登记）

**分支一：correction 转正（成为 relax 槽位的默认实现）**。S4 槽位不变（§1.1 的顺序在两分支下同构——deepCreep 在 correction/relax 之前对两者等价）；预算交互增加一项：correction 的预留 R_limit 在 scale 折算时把 deepCreep 的 2q 已计入 requested——**顺序耦合点**：correction 预留读 totalRequested（collect 态），deepCreep 的请求若在 collect 后 apply 先扣，预留与 apply 的差额一致性由既有 A-1 对账覆盖；若 correction 预留须包含 deepCreep 的预期量，预留公式加一项 Σq_s4_expected（**首版不做**——S4 量级 <2% 预算，预留近似忽略；真机若见 correction 饥饿再升级）。遥测分池：ledger 的 channelUse 分列自然区分（deepCreep vs correction 各自字段）。

**分支二：correction 归档（回退原 relaxAngle）**。S4 零改动——deepCreep 槽位与 relax 的顺序关系在分支二下就是 §1.1 的直接形态；routeId 隔离使 S4 的 ON/OFF 不依赖 correction 的去留；唯一差异：分支二下 relax 无预留机制，deepCreep 先扣的预算直接挤压 relax 的比例缩放（对称公平，§2.3-2 已论证）。

**两分支共同不变量**：S4 不占 correction 预留额度（开题 §3 红线）；S4 的 routeId 与 correction routeId 互斥（一个 run 只能绑一个 route——`setNaturalObservationRouteId` 单值字段天然互斥，S4 ON 的 run 不能同时是 correction run；若未来要同时开，须 manager 授权双 route 语义，本稿不预设）。

## 6. 红线复述

不改冻结锚点（E3/B/C/s133 锚点测量域不含 deepCreep——S4 默认关闭，OFF 态逐位一致有单测）；不放宽 300/s 与 §19.5 guard（S4 是消费者不是新额度；S-invariant 登记不新增拦截义务）；108run 批内不合入（工作区开发，S-1..S-6 全部在独立分支/工作区，等批后合入排期）；对称性闸门作为单测硬断言（§4.2-2，违者参数组出局）；外观生成不偷渡（静置长花纹=manager 授权项，原型只验守恒交换的形状学，不做任何静置波形生成）。

## 7. 结论与路由

五项工作落盘：调度落点（独立第六通道+bottomCreep 后 relax 前顺序论证+2q 预算账本+S-invariant 登记+routeId 双条件门）/逆向交换实现（能量梯度最大优先+K=4+四重 min 截断+字典序微事务+并发顺序语义+shadow 零新数组）/参数定标计划（60 组合全网格+五项指标自动报告+归因分解）/correction 两分支预登记（转正：预留近似忽略；归档：零改动；共同不变量=route 互斥）/developer 任务书（S-1..S-6+10 单测+离线验收五条）。**离线原型开工不待 correction 裁决**（任务书原文）；真机与合入排期等裁决落定后另批。

变更记录：2026-10-23，依据 manager【P-011·S4工程设计】任务产出。开题核心结论照搬（势能式/对称性闸门/效果链/三候选取舍）；工程裁定五项（通道落点/列对规则/q 截断/原子性/route 隔离）+定标网格+两分支衔接+任务书。
