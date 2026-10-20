# P-011 滑移收敛修正速率：独立子通道与贡献度加权 v1

日期：2026-10-20。角色：designer。状态：`DESIGN_ONLY / NOT_IMPLEMENTED / NOT_VALIDATED`。设计前置已解除：§19.5 材料 guard 已随 **891370d** 入库，本稿直接对接该实现，不再以“等 guard 落地”为阻断。独立候选路线：`relax-correction-weighted-v1`；关闭时回到原路线，不能混用旧标定 run 或宣称旧验收自动适用。

依据：[algorithm-P011.md](algorithm-P011.md) §8.12–8.16、[architecture-P011.md](architecture-P011.md) §19.5、§23.1，以及 [P011MaterialGuard.ets](../entry/src/main/ets/physics/P011MaterialGuard.ets)、[WaterSandField.ets](../entry/src/main/ets/physics/WaterSandField.ets) 当前实现。本稿仅设计，不修改代码、用例、阈值、观测协议或采集授权。

## 1. 预算语义与设计范围

**所选方案：独立建账、优先预留、独立空间分配；不乘普通通道的 totalMotionBudgetScale，实际搬运仍全额计入300/s总运动硬上限。** 修正额度从现有 total 中预留，不从被 guard 拒绝的请求、净位移抵消、虚拟库存或下一子步借款中产生。

任务中的“独立于 total 300/s”存在两种不同含义，必须在设计评审时明示：

| 解释 | 可行性与本稿裁决 |
|---|---|
| 独立于普通通道竞争/全局比例缩放，拥有专用额度和账本 | 采用；普通通道先扣除修正预留后再分配，修正不被其他通道的大请求按比例稀释 |
| 普通搬运可用300/s，修正另外增加R/s且不计total | 与“不放宽任何阈值”不相容：实际总量可达300+R。对任意R>0都存在违反原total闸门的请求组合；本稿不以改名绕过该闸门，不提供此分支的上线授权 |

若任务要求的是后一种**额外额度**，则需另行裁决总预算的定义/上限；本稿不能同时证明其满足原300/s。预写设计不因该语义冲突停工，以下给出满足现有全部硬门槛的实现候选。历史 bottom 独立通道也仍扣 total，并非额外免费额度（算法§2、当前 `clipBottomCreepMotion`）。

需要改善的是：（1）settle/percolate/bottom/lift 对 relax 的比例挤占；（2）内部边同向近均匀通量相互抵消。预算独立化只能处理第一项，贡献度必须计算邻边影响才能处理第二项。不得把 grant 增长直接当作收敛加速。

## 2. 当前接入点与边界

现有通道顺序是 `settle → percolate → bottomCreep → relaxAngle → lift`。relax collect 请求约为 `min(excess/2, maxAvalancheFlux*dt, donorStock, receiverCapacity)`，apply 经 `clipSubstepMotion` 扣 local+total，再经 `materialGuardAdmitIncrement` 拦截，最后 `splitTopV2` 转移 heavy/light。并非显式“每边均分”代码：现状是全局比例缩放叠加相近请求，产生近均匀 grant。

本路线在 **同一 relax 槽位**替换其预算分配，不叠加一遍旧 relax、不改其他算子相对顺序。增加子步开始时的纯预留计划；前序 writer 只能使用未预留的额度，relax 到达槽位后对当前场重新核验，不能套用过期库存/残差。当前双角阈值尚待标定，本设计不填默认值、不把新路线数据追认为观测③数据。

“超角”分开定义：`H=tan30°` 是提交安全上限；`h` 是本路线已批准的 relax 目标残差界。当前引擎取 `tan(params.angleDeg)`；将来双角版本只有在真实 active 状态下使用已经批准的 `tan(thetaStop)`，起动仍由既有 thetaStart 判据决定。不得为了消费修正额度启动 inactive 边，也不得把 h 改成新的阈值。`|r|>h` 可以仍在H以内，是常规合法收敛；`|r|>H` 是违规态，只能在未提交事务内尝试修复，最终仍违规则整步拒绝。

若某配置h=H，则合法静态场没有正超角请求，本子通道不会凭空生效；不能为证明提速而下调h。双角路线在H以内的持续滑移资格来自已批准的active/stop逻辑，不能从历史angleDeg=4°算例外推其触发范围。

## 3. 请求、贡献度与单笔下降证明

以下以列高为搬运量单位；当前引擎 dx=1，不改128列几何。令

```text
r[e] = (S[e+1] - S[e])/dx - tan(effectiveTiltAccepted)
e[e] = max(0, abs(r[e]) - h)
E(S) = 0.5 * sum_e e[e]^2
```

E是实际残差能量，不是指定坡形或新的验收阈值。一次 relax 将q从donor移至相邻receiver：`ΔSdonor=-q, ΔSreceiver=+q`。修边c时，s=sign(r[c])，影响系数为 `k[c]=-2s/dx`、`k[c−1]=k[c+1]=s/dx`（越界邻边省略）。r正时右列搬到左列，r负时左列搬到右列；只使用 accepted tilt。

在当前快照上定义这笔搬运的**净贡献导数**：

```text
D[c] = -sum_(e in {c-1,c,c+1}) e[e] * sign(r[e]) * k[e]
K[c] = sum_(affected e) k[e]^2     // 内部边=6/dx²，端部=5/dx²
qDescent[c] = max(0, D[c]) / K[c]
qNatural[c] = min(e[c]*dx/2, maxAvalancheFlux*dt,
                  donorStock, max(0, receiverCapacity))
gamma[c] = max(0,D[c]) / (2*e[c]/dx)  // e[c]>0；否则0
w[c] = e[c]^2 * gamma[c]
```

仅既有滑移判据允许且 e[c]>0、D[c]>0 的边进入候选。`w=e²×gamma`：相同邻域贡献效率下，超角越深权重越大；gamma扣除把问题推向两邻边的负贡献。D≤0不是stop，标为 `NO_POSITIVE_LOCAL_CONTRIBUTION`，active保持既有状态语义。不加人为 epsilon 权重把零贡献边强行激活。

函数 `0.5(max(0,|r|−h))²` 的导数为 `sign(r)*e`，导数Lipschitz常数≤1；故沿单笔方向有

```text
E(S+ΔS(q)) <= E(S) - D*q + K*q²/2
0 < q <= D/K  =>  E_before-E_after >= D*q/2 > 0.
```

把请求进一步截至 qDescent，之后任何库存/预算/guard缩减仍保留下降证明。它比孤立边 excess/2保守，但保证的对象是三边合计实际残差，避免用中央边改善掩盖邻边恶化。此证明要求h、accepted tilt在这笔事务内固定；倾角变化和其他writer的E变化单独归因，不能声称整步E必降。

**均匀场边界：** 内部三边同号且 e相等时D=(2−1−1)e/dx=0；纯e²加权仍均匀，不能解决抵消。本方案不在该处花费零一阶贡献预算，优先有正D的边界/非均匀区域。若所有可搬运边D≤0或guard均拒绝，报告停滞，不能承诺靠加权解决全局可达性。尤其多条相邻边已超H时，逐笔non-worsening可能使每笔修正都不可行；禁止为启动传播而暂时恶化邻边。

## 4. 额度来源、上限与空间分配

### 4.1 子步专用预留

子步dt固定，现有硬额度 `B=300dt`、各列 `L_i=28dt`（若运行配置更小用更小值）、bottom `10dt`、单边 avalanche `maxAvalancheFlux*dt` 全部保留。maxAvalancheFlux读取冻结配置：场景历史有80/s，但引擎默认24/s，本设计不把80当全局新默认。

预留快照取本子步 planner 候选角下、材料通道尚未执行时的材料场。对活跃候选计算 qNatural、qDescent 和当前 guard可准入量。为保证预留不会超占同列额度，令 `degree_i` 为该列关联候选搬运边数（最多2），定义

```text
u[c] = min(qNatural[c], qDescent[c], guardAdmissible[c],
           L_donor/degree_donor, L_receiver/degree_receiver)
R_limit = min(B, sum_c u[c])
```

u非负且无候选时R=0。任何分配p[c]≤u[c]都有 `sum_(c touching i) p[c]≤L_i`。这是保守的预留上界：会牺牲少量局部额度复用，避免为“独立子通道”额外提高local速率。cap中的库存/容量是单笔上界，多个候选共享库存时仍须逐笔重验，不能把这些上界当作可同时兑现的承诺。

R没有拍定一个新增速度参数，按当步可用请求与原上限推出：

```text
sum correctionGrant <= R_limit <= 300dt
sum correctionGrant <= sum_i L_i/2 = 1792dt  // 128列、每笔双端扣账
sum correctionGrant <= (N-1)*maxAvalancheFlux*dt
ordinaryUsed + correctionUsed <= 300dt.
```

300/s为本约束组中更紧的全局上界，不能用较大的local合计或127条边速率合计制造额外额度。R对其他通道的代价是本步可用total减少R；预算满载时可能牺牲沉降/分选/扬起速率，必须作为路线效果代价报告。

### 4.2 贡献度加权的封顶分配

对w>0、u>0候选求唯一有效分配：`p[c]=min(u[c], λ*w[c])`，使 `Σp=R_limit`（若R等于Σu则直接p=u）。按 `u/w` 升序排序，逐个固定饱和项，剩余项 `λ=(R−Σ已固定u)/Σ剩余w`；有限次排序扫描即可，不用二分求预算，更不以二分代替guard闭式区间。

预留 `P_i=Σ触及i p[c]`，普通通道的local可用量为 `L_i−P_i`、普通total可用量为 `B−R_limit`。普通通道的总比例缩放只针对它们自己的请求和剩余额度；correction不参与该比例、不再另乘totalMotionBudgetScale。修正仍扣完整total账，只是在可用额度分区中享有预留优先级。

其他通道不借用尚未执行的预留；guard拒绝或材料变化造成未用预留时归还账本，但本子步不重新派单、不中途给另一边加量、不向未来累积。未用量报告为unused reservation，可用于后续评审分配效率，不隐去。

到relax槽位时，按冻结的 `w降序、physical edge index升序` 执行每边至多一次；每笔重新算当前方向、e/D/qDescent、库存/容量。方向已反转则本笔取消，下一子步重建；当前请求更小则下调，不突破p，也不临场重排序。新的超角边下一子步再计划，倾角提交仍须通过整步可行域，不因延后修正容忍越界。

## 5. 对接891370d闭式guard

现有导出函数实际拼写为 **`p011GuardAdmit(requested, constraints, slopeHardMax)`**，输入 `P011GuardEdgeConstraint{residual,influence}`，输出 `P011GuardOutcome{accepted,rejected,allLegalBefore,limitingEdge}`。不假设另有大写 `P011GuardAdmit` API；可沿用WaterSandField的 `materialGuardAdmitIncrement` 构造列增量映射。

每笔预算允许q后、splitTopV2写入前，构造 donor:-1/receiver:+1 两列计划增量，按当前真实S生成中央边及左右邻边约束（边界裁掉不存在的边）：

```text
L[e] = H                 if abs(r[e]) <= H
       abs(r[e])         otherwise
I[e] = { q : -L[e] <= r[e] + k[e]*q <= L[e] }
I = [0, qBudgetAllowed] ∩ I[c-1] ∩ I[c] ∩ I[c+1]
qAccepted = max(I), 空集则0
```

k≠0时由两个线性端点除k并排序得I[e]，k=0按现有helper语义处理。合法态保持合法；违规态各违规边non-worsening且原合法边不得新增违规。完成一笔后更新S，再为下一笔生成约束；没有“先修中央、末尾再查邻边”的路径，也不把一次初始guard结果复用整轮。

`limitingEdge`当前函数返回的是constraints数组位置；诊断层必须持有同序 `affectedEdgeIds[]` 映射到真实物理边，不能把返回值0/1/2直接当全局边号。有限值与H有效性先校验；helper的未armed回退不构成本路线绕过guard的授权。

## 6. 四层回写、守恒与生命周期

correction新增真实channel账本，不能继续填 `channelRateCap=0`。四层不是四份可相加的容量，而是同一实际grant的多重约束/记账：

| 账本层 | 预扣q | guard拒绝δ=q−accepted后 |
|---|---|---|
| local | donor与receiver各used+=q、remaining−=q | 两端各used−=δ、remaining+=δ；P_i预留与普通可用量另列，不能重复释放 |
| bottom | correction不扣bottom | 不回补bottom、不改creep账；其他bottom writer仍按原逻辑回写 |
| total | used+=q、remaining−=q，覆盖全部真实搬运 | used−=δ、remaining+=δ；remaining含保留/本步不可重派部分，不等于可任意消费 |
| correction/channel | used+=q、remaining−=q，同时占本边p | used−=δ、remaining+=δ；拒绝额度只归还、不重发，channel实际used=Σaccepted |

现有 `materialGuardRefund(rejected,[donor,receiver],false)` 已覆盖local+total；扩展设计只增加correction账回写，不能再手动重复退local/total。最终 `requested=used+clipped`；分列raw request、contribution/local/total分配截断、apply库存重验截断、guard rejected和unused reservation，逐层瀑布计数防止重复相加。totalRequested仍包含修正原始请求一次，totalUsed必须包含实际修正一次；不为让旧checker通过而将修正隐藏在total之外。

实际材料只执行一次 `splitTopV2(donor,qAccepted)`；得到qH+qL=qAccepted，donor减qH/qL、receiver的mh/ml加同量，因此分别 `ΔVH=0, ΔVL=0`，`ΣΔS=0`。加权函数仅改变各笔q，不改变组分拆分。donor相库存与receiver容量必须当笔校验；若split未兑现完整q，则视为预演/提交不一致回滚，不以虚报grant结账。

run/reset/模式切换清除预留、权重、排序、request、used/remaining、原始预算快照和guard映射缓存；方向翻转在下一子步重算，不沿用donor或旧p。每子步额度重新由dt产生，拒绝量不跨步累计。preview快照、回滚必须覆盖新增全部账本/临时状态；不得因guard拒绝或D≤0写active=false，真实stop仍由原状态机定义。

## 7. 与倾角规划的对接及R-3

891370d已具备材料guard，但architecture§23.1明确记录 `PASS_WITH_DEFECT`：旧预演分配器不包含guard缩减，可能承诺实际不能兑现的修复量，倾角先穿透一步。本稿不把此缺陷当作已解决，也不等待它才写设计。

本路线的preview/apply须共享**同一预留、排序、逐笔guard与退款算法**。每个候选theta在纯影子材料场上执行全部writer及本修正槽位，用guard后真实可实现的ΔS判断倾角可行域；最终接受theta后按同计划重放并核验真实ΔS、各账本和最终全边残差。所有候选角间完整恢复状态，不能把前一候选的退款带入下一候选。

R-3-a的guard镜像是接入实现的必要集成条件；实际提交与预演不同必须整步回滚/hold并记录原因，不能到下一子步才发现。若初态已经违规，non-worsening只是单笔许可，最终全边仍未恢复H以内则该事务不得提交。增加修正优先级不豁免updateTilt的§20可行域限幅，不授予放大omega权限。

## 8. 量化预期与不能承诺的结果

历史§8.12首次越界子步：dt=1/60s，total请求52.39441、cap5，slip请求40.81759、实际3.89522。只取消其他通道竞争、把全部5给slip的乐观上界是 **1.28362倍**；整体比例从9.543%到12.250%，该边0.32986698请求的比例grant最多约0.040407（旧0.031479）。这些是忽略新的贡献/guard/local截断的预算上界，不是891370d轨迹复算，更不是5.1秒收敛证明。

加权分配算例（只演示冻结分配器，不冒充整场轨迹）：40边e=0.4、10边e=0.1，gamma=1、dx=1，假设各u分别至少0.133333/0.033333且本步R=5。权重比16:1，未饱和解为每个深超角边0.1230769、浅超角边0.0076923，合计5；普通按这些u比例分配为0.1176471/0.0294118。后续实际写入仍逐笔重算D并过三边guard；相邻耦合可能减小兑现量，不能把50个单笔改善直接相加作最终坡度。

实际收敛评价使用每笔 `ΔE_relax`、总修正grant、每单位grant降低E、maxResidual及guard拒绝原因；同一accepted角/材料初态下，本方案每笔accepted>0必须满足§3下降界。若guard后grant为0，独立额度再大也无效；若均匀通量或库存/容量导致无贡献，报告 `CORRECTION_STALLED`，不伪称完成。

5.1秒全部总搬运仍≤1530，既有全宽OLS可达性下界没有被推翻。±45°到30°±2°目标仍留阶段二；本设计不承诺它或2px可见性已经实现，也不以单边residual的改善替代raster观感。

## 9. 交architect的伪代码、复杂度与验证设计

```text
for each planner candidate theta on restored shadow state:
    derive current eligible relax edges; compute r/e/D/K and natural caps
    build current three-edge guard caps; derive local degree caps u
    reserve R=min(300*dt,sum(u)); capped weighted allocation -> p, P_i
    run settle/percolate/bottom with unreserved budgets and existing guards
    for correction edge in frozen (-w, edgeId) order:
        refresh residual/direction/stock/capacity/D/K
        if direction reversed or D<=0: cancel this reservation; keep active semantics
        q=min(p, refreshed natural/descent caps, applicable remaining budgets)
        debit local/total/correction; call p011GuardAdmit on three edges
        refund rejected in every debited ledger; split and apply accepted only
        update S; no retry or redistribution within this substep
    run lift with its ordinary allocation; audit final field and all ledgers
    accept theta only if final full transaction satisfies existing gates
replay selected plan; verify actual guarded deltas/ledgers; rollback on mismatch
```

N=128，候选构造/三边guard为O(N)，封顶加权与顺序排序O(N log N)，每笔当步重算O(1)，内存O(N)。按既有有限候选角集合重复，无收敛迭代/二分guard。先profile再确认帧率，不在设计稿宣称真机52–60FPS已维持。

以下为验证设计供architect/tester后续承接，本稿不修改用例：

| 对照/断言 | 目的与必须记录 |
|---|---|
| A现状 / B仅专用预留 / C预留+e² / D本稿e²×净贡献 | 固定构建之外的初态、输入、dt和参数；分离竞争收益、深超角优先收益与邻边抵消修正；不得混入新阈值 |
| 逐笔三边闭式准入 | 中央合法/违规、两邻边分别贴H、边界列、k正负/零、零grant；任何合法边不得新增超H，违规边non-worsening |
| 能量与守恒 | 固定参考角时每accepted>0满足ΔE≥Dq/2（按既有数值契约核验）；H/L分别守恒、库存/容量合法，非零q不得虚记为0 |
| 四层回写 | guard部分/全部拒绝后used/remaining一致；correction不动bottom、local双端退、total仅退一次；全部通道sumUsed≤300dt，无重派、无跨步累积 |
| 均匀/热点/相邻冲突场 | 验证均匀内部D=0、热点优先、相邻guard截断及停滞报告；不能仅以relaxAppliedTotal提高判优 |
| R-3及生命周期 | 预演guard后ΔS与真实逐笔一致；角反转、reset、模式切换无旧预留，限流/guard拒绝不误报stop |
| 效果与代价 | 同输入报告E/maxResidual/raster局部响应、各通道被挤占量、未用预留、P95计算耗时/FPS；有改善证据才升级VALIDATED，±45°目标仍延期 |

交接结论：设计现已可评审；额外叠加在300/s之外的修正预算不满足原硬约束，保留为明确未授权分支。所选预留方案与891370d逐笔guard/四层回写一致，新增真实channel账；接入时须同步处理R-3预演保真及路线身份，不能仅改一个budget系数就宣布收敛修复。
