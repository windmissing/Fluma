# P-011 阶段二方案 3：悬浮平流定向搬运机制设计 v1

日期：2026-10-23。责任：designer。状态：**DESIGN_OPENING / NOT_IMPLEMENTED / NOT_VALIDATED**。技术路线：`suspension-advection-v1`。本稿只定义机制和送审条件，不授权代码、用例或真机采集。实现时的状态机基底须待 correction 裁决；本文按 **无 correction 快车道** 为主线，保留转正分支。

依据：[阶段二可达性审计](phase2-reachability-audit-P011-v1.md)、[架构 §28](architecture-P011.md)、[S4 深层蠕动开题](design-P011-S4-deep-creep-bh-v1.md)、[两阶段裁决](priority.md)。历史裁决中“约 30 行、一个参数、无 schema 变化”仅是早期范围估计；§28 已明确 P-2/P-5 新账本和遥测，故**不能作为本路线工程承诺**。安全上限仍为既有每子步 `totalMotionBudgetLimit=300·dt`（若场景配置更低则用更低值）、local/bottom、tan30、容量、分相守恒。出厂 26°/20° 起停角为可调推荐值，不是本路线定标结果。

## 1. 效果对象、论文适用边界与选择

本路线只使**已有水中悬沙**沿列间移动；它不直接改变 `bh/bl/mh/ml`，也不凭倾角生成重沙。用户所说“倾斜时沙向低处聚”，在本路线中须经过 **起沙→定向悬浮搬运→合法落沙**，最终看沉积库存的低侧富集。只看一帧悬沙云偏移不算沉积聚集。

| 文献 | 可直接借用的结构 | 本稿不从文献冒领的内容 |
|---|---|---|
| [246 Thornton–Gray–Hogg 2006](https://doi.org/10.1017/S0022112005007676) | 大/小颗粒与被动孔隙流体的三相**密集颗粒流偏析**；粒相随混合物运动的平流与相对偏析可分开记账 | 该文不是 P-011 水中稀悬沙的已定标平流律；不能从三相名称推出本项目水流速度、drag 系数或起沙阈值。它讨论的粒径分相不自动等于本项目 heavy/light 密度身份 |
| [251 Garres-Díaz 等 2020](https://arxiv.org/abs/2001.04672) | 固液两相质量交换与多层速度、膨胀和孔压耦合提醒：起沙来源与速度闭合不能凭空省掉 | P-011 没有孔压、固相体积分数或水动量场，不能称本稿已实现其两相动力学 |
| [252 Barker 等 2021](https://www.maths.manchester.ac.uk/~cjohnson/papers/barker_et_al_jfm_2021.pdf) | 相浓度方程中 `∂t φν + ∇·(φν u) + ∇·Fν = ∇·Dν`（原文式 2.11）：共享 bulk 速度的**平流**、分相偏析、扩散是不同通量 | 原文 `u` 来自动量/流变求解；本稿没有该解，只把已接受倾角与已有 leveling 迁移场组成**项目代理速度**。原文空气相也不等于本项目水中悬沙 |

因此“246/251/252 证明了某个 `u=kF` 数值”不成立。下面的 `F`、拖曳系数、起沙开关均是 **ASSUMPTION / 待定标项目闭合**，只借质量守恒的通量结构。推荐先审查有源完整链（候选 b），将仅有悬沙平流（a）作为最小安全基线；体力蠕动（c）是另一条路线，不伪装成悬沙平流的收益。

## 2. 状态、离散通量与速度闭合候选

列 `c=0…N−1`，列距 `dx=1`（代码材料单位/列）；`S_c=bh_c+bl_c+mh_c+ml_c`，`U_H,c=suspH_c`，`U_L,c=suspL_c`。对边 `e=(c,c+1)`，只读取**planner 实际接受**的 `θ_eff`：

```text
g_e = S[c+1] - S[c]
t = tan(θ_eff)
F_e = t - g_e                        // 与既有 leveling 迁移场同向的项目代理
u_e = clip(k_drag · F_e, -u_cap, u_cap)  // 列/s；k_drag≥0，u_cap≥0，待定标
ν_e = min(1, |u_e| · dt / dx)           // CFL≤1；单次只跨相邻一边
donor,receiver = (c,c+1) if u_e>0 else (c+1,c)
q_req,p,e = ν_e · U_p,donor^snapshot    // p∈{H,L}，两相同一u但各用自身库存
q_admit,p,e = min(请求, donor库存, receiver悬沙容量,
                    新通道rate cap, 双端local余额, total余额)
```

`F_e` 取正时沿列编号增大方向搬运：水平沙面且正倾角时指向右侧低位；反号镜像。`F_e=0`、无水深、无供体悬沙或 `advectionGain=0` 则请求零。`u` 是**leveling 场驱动的方向代理，不是已求得水流**；真实水自由面/黏滞拖曳若与此方向相反，应根据配对实验否决本闭合，而非修改统计定义求绿。`k_drag` 量级只能由阶段一速度证据加方案 3 OFF/ON 真机轨迹定标；文献没有 P-011 可直接移植的数值。H/L 先共享 `u`，保证 bulk 平流语义；沉降速度仍由现有 `heavySettleSpeed/lightSettleSpeed` 区分。未来若加相对漂移须另立分相律和守恒审计。

所有边的请求从**同一悬沙快照**生成，按冻结的相/边顺序逐笔准入；同列多个出边请求的总和不得超过快照供体库存。承诺先按供体比例或确定性缩放收敛该上限，不能把更新后的入流立刻当同子步下一边的免费供体。容量约束为提交后 `S_c+U_H,c+U_L,c≤waterTop_c` 且整体 `≤capacity`；一笔平流的供/受两列同相 `−q/+q`，两相库存非负，边界无通量。因 `S` 完全不变，`d_e=(S[e+1]−S[e])−tanθ_eff` 不变；它登记为 **§19.5 S-invariant writer**，不运行材料角度数值缩减，却仍逐笔核对容量、库存、预算及 BLOCKED 路径。`updateTilt` 的突变仍由 §20 planner 管，不可把 `ΔS=0` 说成“全局零违约”。

推荐调度：`waterLevel → diffuseSuspension → motion snapshot → advection → settle → percolate → bottom → relax/correction → lift/新起沙 → audit`。旧 diffusion 的无总预算历史语义保持原样；**新 advection 必须单独真实扣费**，不得借 diffusion 路径免费搬运。新起的沙最快下一子步进入 advection；这样预演与 apply 使用同一来源/同一快照，不允许一子步“起沙后沿 127 边瞬移”。若 architect 证明当前 snapshot 位置要求调整，则两轮时序须一起改并出位精确/账本证据，不可只改 apply。

复杂度：单次遍历 `N−1` 边、两相，各笔 O(1) 库存/预算与日志，时间 O(N)，快照及逐通道遥测 O(N)；N=128 不变。若加入全局申请排序，复杂度可能 O(N log N)，工程评审须说明确定性与帧时。速度、通量的候选值均**非冻结参数**。

## 3. P-3 起沙来源：三个候选与推荐

当前 `liftByShake` 依赖前一子步 `shakeEnergy`，按 `splitTopV2` 从混合层→轻层→重层提取。KF-1/KF-2 均初始 `suspH=suspL=0`、无 shake；按原场景归纳，单纯平流请求恒零。重沙上覆 72 单位轻沙；审计给出 KF-1 一秒后轻层仍至少 34，KF-2 全程至少 69.0667，因而普通表面起沙**无法在用例时间窗内触及 bh**。

| 来源候选 | KF-1 / KF-2 可达性 | 预算、guard、库存语义 | 观感适配与风险 |
|---|---|---|---|
| a. 仅搬现有悬沙（由旧 shake/lift 等产生） | 两原场景新增通量都是 0；不能令 KF-2 `phaseHeavyActual/LightActual` 反向 | 平流每相每边扣双端local与total；旧 lift 按原账，平流 `ΔS=0` | 最小安全基线，适合摇晃后云团随倾角移动；单靠倾斜不会出现“沙向低处聚”，不宜作为完整产品效果宣称 |
| b. 新的**合法表层剪切起沙**→平流→落沙 | 改了来源后 KF-1 **弱的 light 下层变化条件可达**；原 legacy 初态不自动变成可达。KF-2 可有反向悬浮/沉积矩，却不写 bottom 专属 phaseActual，原强断言仍不可由它直接满足；重沙目标仍被覆层阻断 | 新 source 每笔先按 `q≤exposed top库存、water容量、channel cap、local、total` 裁剪，再用 §19.5 以 `ΔS_c=−q` 联合预演邻边，准入后才原子地 `deposited_p−q, susp_p+q`；平流及 settle 各自再扣真实 q。BLOCKED 或 guard 拒绝不得起沙 | **推荐作为方案 3 的完整链研究路径**；源必须有独立、可观测的剪切/摇晃驱动，不能以“倾角非零”免费掏厚层。起沙、平流、沉降三段可分别开关与归因 |
| c. body-force 深层蠕动替代原 bottom 请求律 | 审计给 KF-2 两列弱目标一个有条件、守预算/guard 的解析见证；KF-1 仍需终点轨迹。**不是悬浮平流** | 计 bottom+total（遵循当前 bottom/local 规则），逐笔三边 guard；相别底层搬运，不能填到 advection 账 | 更适合厚层 heavy 和反向 bottom 位移；可与方案 3 并行比较，不作为其起沙来源。S4 的 H/L 深层逆向交换又是另一个不同机制 |

**b 的 source 候选闭合（待定标）**：定义仅来自已接受的**动态表层剪切事件**的 `a_c∈[0,1]`（例如当前/前子步已准入且处于真实 sliding 状态的表层通量占其通道额度），以 `q_req,c = dt·k_entrain·a_c·E_c` 请求，其中 `E_c` 是可暴露表层库存及水空间的无量纲/归一化因子。持续运行的 tan4 `relaxAngle` 请求本身**不算起沙活动证据**：±15° 稳定平台即使 relax 有 grant，只要没有真实 sliding 转换/摇晃，`a_c=0`，以保护阶段一静置语义。`a_c` 不能使用尚未准入的请求、纯 commandTilt、目标 dune 或未验证的孔压；零活动/零 shake 下若要允许倾角自发起沙，须先有独立效果证据与 manager 批准。`k_entrain`、活动窗口和相配比均 **ASSUMPTION / 待定标**；分相按实际 `splitTopV2` 的暴露顺序，绝不按 desired heavy 比例直接取底层重沙。若效果确实要求被 72 轻层覆盖的 heavy 在 1 秒内迁出，须另立“深层起沙/露头”物理来源或交 body/S4，不能暗改 b 的 top split。

推荐实施决策次序：①先做 a 的 OFF/ON 安全与悬沙位移证书；②P-3 独立批准 b 的源门控、审计可见性和起落完整链；③仅当 b 有实际的沉积聚集收益时讨论产品转正；④厚层 heavy 与 KF-2 bottom 强目标交 c/S4 或重定义场景。这个次序是**设计推荐**，不是已授权排期。P-4 可先用审计的条件包 `q=0.01` 核对算法通路：16 列 15 跳、起落各一笔的总成本上界 `17q=0.17`，但该包未证明原场景存在合法 source，也未证明 1 秒内达到 2px 或 5.1 秒收敛。跨 127 边搬 `q` 至少付 `129q`（起+127 跳+落）；即使把 5.1 秒全部 300/s 给该链，`q≤1530/129≈11.86` 也只是**忽略局部、guard、settle、其它通道的宽松必要上界**，不是可达证书。

## 4. P-2/P-5 账本与分域一阶矩

对每相 `p∈{H,L}` 定义 `D_H,c=bh_c+mh_c`、`D_L,c=bl_c+ml_c`、`U_p,c=susp_p,c`，总量 `V_p=Σ(D_p,c+U_p,c)`；`Mdep,p=Σc·D_p,c`、`Msusp,p=Σc·U_p,c`，单位为材料高度·列。若显示映射需物理长度，另乘冻结的 `dx`，不在账本中混入像素。

| 原子交易（相 p，q>0） | `ΔMdep,p` | `ΔMsusp,p` | `Δ(Mdep+Msusp)` | 预算归属 |
|---|---:|---:|---:|---|
| 同列 `c` 起沙 | `−c q` | `+c q` | 0 | source local c=q、total=q；不记成横向运输 |
| 平流 `c→c+1` | 0 | `+q` | `+q` | **两端local各q、total q**；H/L 各笔相加；bottom=0 |
| 平流 `c+1→c` | 0 | `−q` | `−q` | 同上 |
| 同列 `c` 落沙 | `+c q` | `−c q` | 0 | settle local c=q、total=q；不重复计算平流的矩收益 |
| 原有沉积列间 writer / S4 | 按其真实相与方向计算 | 0 | 等于 `ΔMdep,p` | 归自己的通道账，不得填入 advection 通量 |

每笔检查 `ΔV_H=ΔV_L=0`；起/落虽在两域各出现 `cq`，**总矩净贡献零**，不能把两域变化绝对值相加声称 `2cq` 的搬运收益。平流的 `|ΔMtotal|=q` 是一条边的真实位移；跨多边每跳各收费。每子步账本可计算

```text
R_M,p = [Mdep,p(t+dt)+Msusp,p(t+dt)] - [Mdep,p(t)+Msusp,p(t)]
        - Σ_e signed(advection_p,e · dx)
        - Σ_e signed(other_actual_phase_transport_p,e · dx)
        - Σ_e signed(existing_diffusion_p,e · dx)
```

`R_M,p≈0` 才算分域对账；旧 diffusion 也改变悬浮矩，须单列其真实通量，不能扣在 advection 头上。水体通过悬沙体积占用自动发生同列空隙变化，但不把水矩误作沙矩。遥测最少记录：`runId/substepId/routeId/θ_eff/edge/phase/donor/receiver`、`u/F/ν`、`sourceKind`、`requested/granted/budgetClipped/capacityClipped/inventoryClipped/guardRejected`、`localUsed` 双端、`totalUsed/channelUsed`、`SInvariant`、`MdepH/L` 与 `MsuspH/L` 前后值、`R_M,H/L`、各相总量残差、终点悬沙率。记录真实 grant 后状态，不只记录计划通量；schema 向后兼容并标识新 route。新通道的 `requested=granted+各原因截断`、四层 used/remaining 恒等式由 architect 逐字段定义并在 preview/apply 双轮一致。

P-6 判断：**advection 本身 `ΔS=0`，不需要角度数值 guard，但必须注册 S-invariant 事务并进入 shadow/ledger；source 与 settle 是 `ΔS≠0` writer，必须按 §19.5 逐笔 guard。** 若 S 已因参考更新处于违规态，advection 不恶化 `d`、不新增违规边；但它不能修复旧违规，整步是否 BLOCKED 仍按既有 planner/shadow 语义，不得靠平流日志将 BLOCKED 子步记成成功提交。

## 5. 可测效果候选与否决规则（非冻结）

同初态、seed、输入、参数（factory 26/20 或明确用户 pair）、真实 renderer 栈的 `advectionGain=0/ON` 配对；另外分开 `source=旧shake only / b新source`，不得以 b 的收益归给 a。低侧 `L(t)` 由**接受角符号**定义为容器末端固定宽度 ROI（候选 1/4 宽，具体由 effector/manager 冻结）；翻转阶段另开 ROI，不跨符号拼峰值。记录 0、1、5.1 秒及稳定尾窗，同时覆盖 ±15° 静置、±45° 渐进崩塌、摇晃后平流、反转、厚层遮挡；`θ_eff` 未到命令角时以实际角分组。

| 候选量 | 定义与预期方向 | 防误判 |
|---|---|---|
| 低侧沉积富集 `C_dep,p(t)` | `Σ_{c∈L}D_p,c(t)/V_p` 相对起点及 OFF 的配对差；H/L 分报 | 只看 susp 云团不算沉积；若全部 source=0，零收益是机制预测，不按参数小判失败 |
| 横向位移 `ΔMtotal,p` | 上节总矩除 `V_p` 报等效列位移，ON 应与倾角同向且反转后能改变符号 | 同时报 `Mdep/Msusp`，禁止把 lift/settle 的分域交换当双倍运输 |
| 可见沙顶/重沙边界 | 低侧 ROI 的 `S`、`bh` 显示像素位移及连续帧变化；参照旧“局部 heavy 边界 ≥2px”仅作候选对照 | b 起表层 light 时 `bh` 可严格为 0；不能拿局部 2px 目标替代全宽 30°±2° 目标包 |
| 时间与稳定 | 首次低侧沉积响应时间、5.1 秒曲线及尾 1 秒坡度变化、末态悬沙占比 | `悬沙≤5%` 是必要不充分；持续起沙/尾部迁移不可被终点一帧掩盖 |
| 安全、性能、观感 | 全相守恒、容量、127 边 tan30、所有预算恒等式必须零违规；每子步 O(N) 帧时配对；effector 判“向低处聚”且沙/雾可辨 | 任一硬闸门失败停该路线；旧 SLIP/渲染锚点不变，主观成功不能覆盖账本失败 |

候选效果门槛由同尺度目标视频/真机 OFF 基线给效应量和置信区间再冻结；本文不填“低侧增加 X%”拍脑袋数。定量否决条件：合法 source 频率为零、`granted` 长期近零、`Mtotal` 方向与接受角系统性相反、或终点沉积富集对 OFF 无可分辨增益，分别归因驱动、预算/容量、速度闭合和效果通路，不调宽安全线。阶段二 ±45° `30°±2°`、target-based `t90≤5100ms`、`R²≥0.90` 仍是独立**未验证**目标，不因本路线设计完成而转 PASS。

## 6. 与 S4、correction 及立项闸门的衔接

S4 首选候选是底层 H/L 逆向交换：`ΔS=0`、`bh` 起伏、实际两相搬运计 **2q**，慢时标；本路线是悬浮相 H/L 各自定向平流，`ΔS=0`、快时标，只有落沙后才改沉积。二者不得共享 `channelRateCap` 身份或把 `S-invariant` 当免费运动。调度建议把 S4 放在沉积侧 writer 槽（bottom/relax 后、lift 前，由 architect 结合 correction 裁决冻结），advection 在 settle 前；同一快照下 source→advection 最快下一子步。若两者同开，共享的 **300·dt total 和 local** 先以同一请求分母分配，S4 的 `2q` 与 advection 的每相每边 `q` 逐笔真实扣费；拥堵时报告谁被谁挤占，不许新设不受 total 约束的快车道。

- **correction 归档（主线）**：恢复自然 `relaxAngle` 槽及其正常预算竞争；方案 3 的 routeId、OFF 基线、请求预演按这一基底冻结。当前 interim 负信号只支持按此准备，非正式归档结论。
- **correction 意外转正**：保留其显式 route gate/预留账，advection 与其在预演和 apply 共用真正的总额；先重算请求分母和优先级，重新做 OFF/ON 配对及 P-4 证书。禁止把 correction 的独立修正额度借给 advection，亦不把其收益混入平流收益。
- **P-1**：§28 的 bottom guard 退款潜伏缺陷须先修并以“先扣多少、最多退多少”单测证明，才可据新链做正式可达证书。新平流自身 guard 不扣退角度额度，但库存/容量或预算 clip 不得退未扣账。
- **P-4**：需要从真实 accepted 角、初态、每步 source/平流/落沙真实 grant、其它 writer、最紧 guard 边、四层余额和终点形状导出**全链 shadow 轨迹证书**；条件包、单笔合法、解析上界均不能替代它。若 128 列 5.1 秒或 heavy ≥2px 不可达，如实给出 `SOURCE_UNAVAILABLE / SHADOW_BLOCKED / BUDGET_EXHAUSTED / CAPACITY_BLOCKED / EFFECT_INSUFFICIENT`，交 manager 决定目标或路线，不放宽阈值。

设计结论：**推荐 b 作为完整效果路线，a 作为最小安全基线，c 保持独立深层候选。** P-3 的“起沙驱动可观测且合法”与厚层 heavy 来源是下一次设计/经理裁决点；P-2/P-4/P-5/P-6 的工程化输入已移交[architect 任务书](task-P011-phase2-advection-architect-v1.md)。本稿不宣称两个 legacy FAIL 已转正，也不宣称方案 3 能独力完成全宽 5.1 秒目标。
