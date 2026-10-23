# P-011 阶段二方案3：两个legacy用例的可达性审计 v1

日期：2026-10-22。责任：designer。状态：**ANALYTICAL_AUDIT_COMPLETE / CONDITIONAL_REACHABILITY / NOT_IMPLEMENTED**。依据[重分类预研§4](legacy-fail-reclassification-P011-v1.md)、[阶段边界审计大纲](review-basis-P011-stage-boundary-v1.md)、[架构§24.3](architecture-P011.md)。本轮纯分析，未运行产品/host用例或真机，不改变用例、机制、预算、阈值或白名单。

**结论：仅搬已有悬沙对两个原场景都没有新增作用；起沙链在原场景缺少来源，且不能替代KF-2的bottom专属指标；体力蠕动存在满足预算/guard的条件性反向构造，但仍需完整调度验证。两用例均建议③重新设计阶段二场景语义，保留原回归，不永久豁免、不调参数求绿。** 本审计不证明128列±45°/5.1s整体目标可达。

## 1. 审计身份与目标

只读基线HEAD=`a46fcb5`。实现：[WaterSandField](../entry/src/main/ets/physics/WaterSandField.ets)、[MaterialGuard](../entry/src/main/ets/physics/P011MaterialGuard.ets)、[用例](../entry/src/test/WaterSandFieldTest.ets)。源码SHA-256分别为：

```text
WaterSandField.ets  0a7d329ee03a0878d76b73cf070c10f88cdfbd2dd46a1759f2ac9d81f50abe66
P011MaterialGuard.ets  caa9780a44ea114592c3f7cf444af47c2799d543b53279a486c29151b13f2ef6
WaterSandFieldTest.ets  c8fa14caf95cdf3beb1b35311e85f18c9b5b1bcdf596931f28ed2f244f5b0bcf
```

| 对象 | 冻结原场景与验收对象 |
|---|---|
| KF-1 `bottomCreep_tiltMovesLowerLayers` | 16列，capacity192，逐列bh=72/bl=72/mh=ml=suspH=suspL=0，waterTop182；输入+30°，60×1/60s。legacy angleDeg=5、maxAvalancheFlux=60、local28/s、bottom10/s、total300/s。最终max(|Δbh|,|Δbl|)>0；step.ok、分相及水守恒<1e-8。**bl变化就满足弱断言，并不要求heavy或2px可见** |
| KF-2 `p011_commandSignFlipResetsPhaseAndReversesDisplacement` | 2列同初态；+20°8步后−20°8步，每步1/60s；legacy angleDeg=30、local1/s、bottom10/s、total100/s。phaseId1→2/sign−1，phase内heavyActual/lightActual均<−1e-12，heavyShapeRms/MaxDelta>1e-12 |

两者当前helper显式`relaxMode=legacy-single`、diffusion=0、waterLevelRate=0.001；不是产品双角26/20，也不是correction矩阵A。输入没有shake，初始shakeEnergy=0。bottomCreepRate=96不是可用96/s额度。

历史FAIL身份保留。[u27_check](../test/reports/u27_check.json)、[hfix_check](../test/reports/hfix_check.json)、[verify_d1_final](../test/reports/verify_d1_final.json)均已有两用例PASS记录；这些汇总无本次完整逐writer轨迹，不冒称当前HEAD新复跑。因此审计的是**候选机制是否适用/可达**，不是预设原用例当前一定失败，也不把旧PASS升级成方案3验证。

## 2. 统一可行域、预算与新增发现

令列间距dx=1，`S=bh+bl+mh+ml`，`g_e=S[e+1]-S[e]`，`t=tan(thetaAccepted)`，`d_e=g_e-t`，`H=tan30≈0.577350269`。一笔q的列增量为qv，`k_e=v[e+1]-v[e]`：

```text
q ∈ [0,Q] ∩ ⋂e {q: -L_e ≤ d_e+k_e*q ≤ L_e}
L_e=H（原合法边）；L_e=|d_e|（原违规边：不得恶化或新增越界）
Q=min(请求、供体库存、接收容量、实际剩余额度、耗散上界如适用)
```

相邻搬运联合中央边与两邻边，lift/settle检查列两侧；悬浮平流ΔS=0仍登记事务及库存/容量，不因此绕过planner。参考角由既有两轮planning决定，不能用命令角代替实际角。clamp/held允许真实材料提交；BLOCKED没有该笔有效运动，不能算作构造的一步。

### 2.1 成本归属（不是新增预算授权）

| 通道 | local | bottom | total | 相库存/guard |
|---|---|---|---|---|
| 现有lift/settle | 本列q | 0 | q | splitTop或按悬沙分相；ΔS=−q/+q |
| 现有relax / correction | 双端各q | 0 | q，一笔只计一次 | top split；三边联合guard |
| 现有bottom或建议替换它的body creep | **不扣普通local（当前实现）** | q | q | phase-balanced split；三边guard |
| percolation | 0（当前实现） | 0 | 实际分配额，q/r再细分；不得重复收费或记位移 | ΔS=0，每相列总量不变 |
| 候选suspension-advection | **提议双端各q**，保守显式计入local | 0 | 每相每边搬运q合计，不能免费跨多边 | ΔS=0，双方悬浮库存/水深容量；新channel分列 |

local上限KF-1=28dt/KF-2=1dt，bottom合计≤10dt，total分别≤300dt/100dt。当前`diffuseSuspension`并不扣上述total/local，但原用例扩散系数为0；**不以该旧实现路径为新平流的免费额度来源**。新平流归属是审计提出的设计约束，立项后必须由architect落入预演/实际统一账本。

### 2.2 发现预算退款风险：不得据此获得“可达”

`clipBottomCreepMotion`只扣total并累计bottom；bottom guard截断时却调用`materialGuardRefund(rejected,[donor,receiver],true)`，其`refundLocal`无条件增加localRemaining并减少localUsed。由源码可构造：本步local初值L、used=0，bottom拒绝r>0后localRemaining=L+r，used仍0，**退了未扣的local**；若settle已用local，还可能抹掉其used。

这是静态代码路径发现，**没有本轮运行证据证明原两用例触发了多少次**。不能据此推翻历史所有PASS，也不能把增出的额度写入方案3可达证明。交developer/architect核验扣账来源与preview镜像，退款最多为该笔先前实际扣额，各层`used+remaining=limit`成立；本轮不修代码。以下预算上界/构造均按规范额度计算，含“退款账正确、无隐式增额”的前提；正式方案3完整轨迹证书应等待此项解释或修复回执。

## 3. 机制A：只平流已有悬沙

假定保守平流只从供体现有susp相取量，`q_p≤susp_p[donor]`，无负库存。原场景初始suspH=suspL=0，shakeEnergy与shakeInput均为0，lift每步返回；settle/percolate不产生悬沙，bottom/relax只改沉积。归纳得到所有子步suspH=suspL=0，故新平流逐笔q=0。

- **KF-1：对新增下层运动不可达/原样无效。** 原bottom仍可能通过旧断言，不可误读为整个引擎不动。
- **KF-2：对新增反向bottom通量不可达。** 除零库存外，即便未来外部注入悬沙，新平流也不写`phaseHeavyActual/phaseLightActual`；它们仅由实际bottom交易更新。

把初态悬沙改非零、偷偷加shake、将命令倾斜当作shakeEnergy，都改变场景或机制，必须另立语义/路线，不能称原样解决KF。

## 4. 机制B：合法起沙→相邻平流→落沙

### 4.1 悬沙来源及heavy可提取性

现lift使用步前requestEnergy，随后才更新shakeEnergy；即使另加非零shake输入，也不能假设第一步就按新energy起沙。`splitTopV2`顺序是混合层按比例→bl→bh，不能按heavyLiftRate=36直接从底部拿heavy。

在原厚层初态，任何小q起沙首先全是light。预算正确、没有外加上层heavy/新分相源的条件下，首个heavy被top提取以前，任一列bl的下降至多来自普通local表层搬运与bottom搬运：

`bl[c,T] ≥ 72 − (vSandTopMax + vBottomCreepMax)T`。

这是宽松上界：把全部bottom额度和该列全部local都用于剥掉light，忽略回填；percolation在此前无mh来源。KF-1的T=1s仍≥34；KF-2整段T=16/60s仍≥69.0667。故**保持上层提取规则时，在两个时间窗内不可能靠剥去72轻层再起heavy**。若其他新机制先把heavy送到mh，必须另列那一段的来源/预算；不能把它藏进lift。

独立看lift，KF-1 bandBase=0.5、energy≤1、纯轻面desired≤20/s，也远不足1秒剥72；实际无shake时上界收紧为0。此证明不否认bottom直接搬bh，也不否认KF-1仅bl变化即可通过。

### 4.2 逐笔安全构造：证明通路存在，但前提不是原场景

构造B1采用**合法可获得的平坦+30°快照作为条件初态**，16列其余库存同原场景，假设已有正起沙驱动且其它writer不抢占/改写本见证；这不是把原run瞬间强设30°，初态到达和并发条件仍须完整shadow证明。取q=0.01，全light：

这里的来源条件是“能合法提供并计费这一包q，后续没有未计入的追加起沙”，**并非现有全列shake输入已经满足**。现lift会在energy衰减期间持续申请，可能从左端再产生包；若选择该来源，须把所有追加包及竞争一并纳入轨迹，不能只截取其中一包宣称全链证书完成。新的可控起沙来源仍待独立设计/批准。

| 交易 | 状态与guard | 每笔预算/容量 |
|---|---|---|
| 最左列0 lift q | bl0=71.99，suspL0=.01；仅edge0从d=−H变为−H+q，合法 | local0=.01，total=.01；S+susp不变，水体空间38保持 |
| 0→1→…→15，逐相邻边搬已有suspL q | ΔS=0；中央/邻边残差不变，不能触碰沉积数组 | 15笔各total=.01、双端local=.01；每列接收前容量≥38（供体0在lift后略增），q远小于容量；可按每子步最多一跳登记，禁止一次q预算传15边 |
| 最右列15逐步settle | 第k笔a_k使edge14残差由−H变为−H+累计落沙；累计≤.01，始终合法；沉积写ml而非bl | 每笔local15=a_k、total=a_k，且a_k遵循现有settle速率，不能瞬间把剩余悬沙全落下 |

最大总成本为`q+15q+q=17q=0.17`，bottom=0，local沿途每列累计≤2q（分步时每步更小）；不与横向收益`15q=0.15`重复计功。悬沙最大q，总初始沉积量2304，比例`q/2304=4.3403e-6`（0.000434%），低于5%。本例不借全扬起制造可见形状。

**时间/落沙不能省略：** 候选平流若允许一条边每1/60s运完整包（CFL≤1、|u|≥60列/s的候选上限；不是已标定材料速度），lift一子步＋15跳用16/60s；为保守示例余下44子步只做终点settle。对深度约38的light，`a_k = Q_k min(1,38dt/depth_k)`，因此1秒时`Q_end≈0.01(59/60)^44≈0.00477`，并非0；已落约0.00523至ml。总实际成本≈0.16477，仍安全。低速若不能15跳，不宣称在1秒完成跨容器运输。

在实际“平流→settle→…→lift”时序中，途中每步也会尝试settle：+30°平坦内列的增加型settle会恶化一侧d=−H而被guard拒绝，故该特定构造能把包保留到右端。若接受角<30°或中途已有非平坦S，途中settle可能获准，必须重放它，不能人为关掉来维持完整包。最左列若先settle再平流会回填原孔，故顺序必须冻结；不能把改顺序当成免费证明。

**KF-1语义：** 在该条件构造中，供体bl下降.01，settle落到ml，无heavy时percolation不能将ml自动还原成bl，因此1秒的弱lowerMove可保持非零；**bh全程未变**。相邻近距离版本（0→1）在θ<30有余量时只需3q成本、更短输运时间，同样只能证明条件性轻层变化。原场景来源仍为0，所以本构造不证明原配置靠方案3达到目标。

**KF-2语义：** 镜像链可产生负向`Mdep,L+Msusp,L`，但无top-heavy来源，也没有任何bottom字段写入。因此仅此链对“两相负向bottomActual”仍不可达。若它改变S使旧bottom后来获准，只能归因于那笔真实bottom交易，需联合轨迹证书，不能把advection的q填进bottom账。

### 4.3 终点与稳定性

5%悬沙条件只是一个必要终点条件；B1的小q即使未落完也满足它，不能据此声明稳定终点。settle的指数尾随、输入持续起沙、残差导致的普通relax/body请求都可能继续演化。阶段二5.1s、末1s坡度变化≤1°等目标仍需完整尾段检查；B1不包含该证明，也没有给出全宽R²≥0.90、30°±2°或≥2px heavy视觉证据。

## 5. 机制C：体力驱动蠕动的条件可达构造

### 5.1 候选力律与耗散证明

推荐作为**bottom请求生成器的替代候选**，不叠加旧target-residual请求：

```text
F_e = tan(thetaAccepted) − (S[e+1]−S[e])    // 不是targetTilt
j_req = dt * mu_e * F_e                   // mu_e≥0，单位1/s；符号决定方向
q_req = min(|j_req|, |F_e|/2, 库存, 容量)
q = budget_allocate_then_guard(q_req)     // 重算当前边和邻边，不增额度
```

这是列标量**设计代理律，未标定**，不是已经验证的真实应力模型。固定本笔实际theta，取离散势能代理 `Phi=0.5ΣS_c²−tΣc*S_c`，沿sign(F)相邻搬运q有 `ΔPhi=−|F|q+q²`。限制`q≤|F|/2`得到`ΔPhi≤−|F|q/2≤0`，任何预算/guard缩减保持耗散方向。theta变化产生外部输入功，不能跨翻转把Phi变化全部叫材料耗散。密度/浮力若未来加入，应重新给相势能和迁移率证明，不凭2–3锚点插值指定mu。

若增静摩擦阈值 `max(0,|F|−F_y)` 且F_y≥tan20，则小坡原场景可能重新零请求；所以“体力”二字不能保证运动。示例取mu=1、F_y=0，只证明该候选类有可行成员，不是推荐用户默认定标。该底层慢蠕动不改表层双角active判据；产品26/20路线需另审是否改变已关闭的±15°外观约束，不将原legacy见证直接搬过去。

### 5.2 KF-1：早期有合法重轻通量，最终不抵消仍有条件

在平坦12°快照，F=0.21255656，取每条边q=.001（mu=1请求足够，分配器的保守cap候选），按右→左逐边处理15边：每笔中央边+2q、邻边−q，瞬态最大残差≤tan12+2q< H；全部完成S0减q、S15增q，中间S不变。总bottom=.015<10/60、total=.015<300/60；即使额外按普通local核验每列≤.002，也小于28/60，但实际bottom不扣它。供体两相都有72，phase-balanced heavy share∈[.35,.65]，故端点bh及bl均非零变化。

该构造是一个早期子步的可达见证，不是当前分配器必然按此cap执行。精确+30平坦态上内部单笔三边交集可为0；增加mu不能跨过该零可行域。后续planner、普通relax与其它bottom如何作用决定1秒末是否保留变化；若其它通道最终精确抵消，则不满足原最终快照断言。故KF-1结论是**条件可达**：需要完整提交序列及末态不抵消证书，不能以第一步非零代替终点。

### 5.3 KF-2：原8步反转窗口内的完整两列力律见证

条件：两列其他通道没有非零请求（此初态无悬沙/混合，legacy tan30下下列残差不触发relax），候选body替代原bottom请求，mu=1、dt=1/60，planning接受角速率上限720°/s所允许的序列，即每步最多12°；下表明确列出角度。预算无竞争，q=|F|/60，小于耗散cap和bottom/total，未触及guard。令g=S1−S0：

`g_(n+1)=(1−2/60)g_n+(2/60)tan(theta_n)`。

| 步 | accepted角 | 有符号j | 提交后g | 提交后|d| |
|---:|---:|---:|---:|---:|
| 1 | 12 | +.003542609 | .007085219 | .205471343 |
| 2 | 20 | +.005948084 | .018981386 | .344988848 |
| 3 | 20 | +.005749814 | .030481014 | .333489220 |
| 4 | 20 | +.005558154 | .041597322 | .322372913 |
| 5 | 20 | +.005372882 | .052343085 | .311627149 |
| 6 | 20 | +.005193786 | .062730657 | .301239577 |
| 7 | 20 | +.005020660 | .072771976 | .291198258 |
| 8 | 20 | +.004853304 | .082478585 | .281491650 |
| 9（翻转后1） | 8 | +.000967704 | .084413993 | .056126842 |
| 10 | −4 | −.002572347 | .079269300 | .149196112 |
| 11 | −16 | −.006100245 | .067068810 | .353814196 |
| 12 | −20 | −.007183984 | .052700842 | .416671076 |
| 13 | −20 | −.006944518 | .038811806 | .402782040 |
| 14 | −20 | −.006713034 | .025385738 | .389355972 |
| 15 | −20 | −.006489266 | .012407206 | .376377440 |
| 16 | −20 | −.006272957 | −.000138709 | .363831525 |

以上为公式逐步计算，非引擎实验。每步q≤.007184，小于bottom .166667、total1.666667；甚至小于KF-2普通local .016667（只作保守余量比较）。实际参考角切换后的**提交前**残差也全合法，最大约.431039<H；库存每相约72、接收水深约38，均远离耗尽。无悬沙，终点悬沙比例0。

翻转后正向总q=.000967704、负向总q=.042276351。即便每笔heavy/light share分别在[.35,.65]中变化，每相phaseActual的保守上界都是 `.65*.000967704−.35*.042276351=−.014167715 < −1e-12`，所以两相都反向；相对于翻转baseline的bh净变化也非零，满足两列RMS/max非零的弱形状目标。phaseId仍须由命令翻转独立更新。

候选方向选择必须以F为准；不能在F已反向时继续沿debounce旧方向搬运，否则破坏耗散证明。若保留80ms方向保持，应在方向与F不一致时只允许q=0。本见证沿用初始t=0建立正向方向，首次反向已超过80ms，故不被该窗阻挡。第9步正向是实际accepted仍+8°且F>0的结果，不得强按新target负号造负功。若旧`directionCorrect`要求每步与命令符号一致，候选需另行声明“命令方向/驱动力方向”的诊断语义，不能伪造旧字段求绿。

因此KF-2在此候选/两列无竞争前提下**数学可达，工程归类条件可达**；不证明现有实现已经采用该律，不证明128列共享预算也有同样响应。

## 6. 分相一阶矩与成本闭合

定义 `D_H[c]=bh[c]+mh[c]`、`D_L[c]=bl[c]+ml[c]`、`U_p[c]=susp_p[c]`，`Mdep,p=Σc*c*D_p[c]`、`Msusp,p=Σc*c*U_p[c]`，`Vp=Σ(Dp+Up)`。列坐标而非像素；质心变化为`Δ(Mdep,p+Msusp,p)/Vp`，不能把通量和像素位移直接等同。

| 交易p相q，c→c+1取正 | ΔMdep,p | ΔMsusp,p | 总相矩变化 |
|---|---:|---:|---:|
| 相邻沉积bottom/relax | +q | 0 | +q |
| 同列c lift | −c*q | +c*q | 0 |
| 相邻悬浮advection | 0 | +q | +q |
| 同列c settle | +c*q | −c*q | 0 |
| 同列percolation相内重排 | 0 | 0 | 0 |

反向搬运符号相反；每相体积守恒ΔVp=0，水体总量也单独核对。悬浮横移改变列内水体占据，但在未触容量截断时两列水量变化相反，不能只查沙守恒漏掉水。

B1在1秒尾端：`ΔMdep,L=15*(.01−Q_end)`、`ΔMsusp,L=15*Q_end`，总和=.15；H均0。不能把lift/settle与advection收益相加得到三倍。KF-2的body相位总通量与相矩一致因为dx=1且只有一个边；多列场景的bottomActual是所有边通量有符号和，仍不等于全通道矩或heavyShapeRms。

预算必要界：若新平流也逐相邻边扣total，任何期望全材料矩增量|ΔMtotal|至多为横向实际grant总和×dx；lift/settle另耗额度而不增加总矩。仅有 `|ΔM|≤Btotal*T` 是必要条件，尚需扣垂向/percolation/correction消耗和local瓶颈，不能据此给充分可达结论。S-invariant相交换满足总矩不变，不能同时让两相都获得同方向非零总矩而不引入其它路径。

## 7. 与correction矩阵及preview/apply的对接

1. [现有A/B/C/D矩阵](preregistration-P011-correction-matrix-v1.md)保持26/20推荐值及四因素不变，**方案3不混入旧矩阵**。先在独立`phase2-transport-candidate-v1`设计身份下审计/验证；若将来联合，新增正交因素和manifest，由manager批准，不追认原矩阵结论。
2. 预算沿架构§24：correction是total内优先预留，`Bordinary=Btotal−Rcorr`。lift/advection/settle/percolate/body均计ordinary实际额度；body与旧creep二选一请求，共享同一个bottom10dt池。不能新增第二个10dt池，也不能把ordinary无法执行的量收费到correction来躲避上限。
3. local同样扣除已有correction预留后再供新advection/lift/settle使用；底层蠕动按其原本独立local语义不借用/退款local。未用correction预留是否再分配沿已批准算法，不为方案3临场改变。每笔退款按实际扣账令牌回写local/bottom/total/channel，非零计数不当作体积。
4. 新通道位置提议为**water更新→已有扩散→新平流→settle→percolation→bottom或body→原relax或correction→lift**。新lift产生的悬沙最早下子步平流；不在同collect阶段先假借本步尚未批准lift库存。平流使用步首库存限流并预扣供体，单子步不因扫描方向让同一颗粒免费跨多列；库存可用量与逐笔容量同时校验。
5. 不能简单在当前collect前加一次真实advection而不纳入回滚。必须把六材料数组、水体、方向/phase、预算/请求计划与路线缓存纳入同一候选theta事务；两轮preview、collect、verify、apply共享力律、资格、算子顺序、tie-break、phase split与guard闭式求交。逐笔更新S后再判下一笔；BLOCKED/重试不能泄漏库存、phaseActual或悬浮位移。
6. 联合衡量须分别输出全局相矩收益、relax槽位ΔE、ordinary挤占、guard拒绝与CORRECTION_STALLED。方案3改变S产生的E变化不灌给relax贡献；correction停滞也不自动证明全通道无解。

复杂度：固定列邻边单扫描O(N)，guard每笔最多3条边O(1)；固定通道与有限轮planning为O(N)时间/O(N)影子存储。禁止隐式“循环到达目标”使子步复杂度失控；若申请多跳，显式限制hop数、逐跳收费且验证CFL，不依赖扫描次序获得高速。

## 8. 逐机制结论、重分类与移交

| 机制 | KF-1 | KF-2 |
|---|---|---|
| 仅现有悬沙平流 | **不可达新增作用**：零库存不变性 | **不可达新增作用**：零库存且字段不同 |
| 原lift＋平流＋settle，输入保持不变 | **不可达由该链驱动**：energy=0；不否定原bottom PASS | **不可达由该链驱动**：来源0且不是bottom |
| 另获合法来源的起沙链 | **条件可达弱bl变化**：B1展示逐笔路径/成本/残留；原1秒heavy top提取不可达 | **单独不可达原bottom目标**；联合旧bottom仅条件可达，需额外真实bottom证书 |
| 体力驱动bottom替代律 | **条件可达**：有合法首步，完整1秒末态/竞争未闭合 | **条件可达，有16步解析见证**；路线实现、identity与退款问题未验证 |

| 三选一裁决建议 | KF-1 | KF-2 |
|---|---|---|
| 重调参数转正 | 不选；历史已有PASS，不以加预算/降角/延窗求绿 | 不选；当前是±20°，不是把30°调低的问题 |
| 维持KNOWN_FAIL永久豁免 | 不选；安全与原回归不能永久放过 | 不选；命令相位和bottom账本不能豁免 |
| **重新设计阶段二场景语义** | **推荐**。保留原16列30°/1秒原回归；新增分开的“light起沙链”“暴露heavy后的合法链”“body下层净变化”研究场景，明确改初态/来源是新增场景 | **推荐**。保留原两列±20°/两相bottom指标；新悬浮路线另看Mdep/Msusp和终点，body路线保留真实bottom记账及accepted驱动力方向，不把其它通量混入旧字段 |

本次已完成原预研要求的**两legacy用例范围**分析，以上建议不再仅为待启动大纲，但仍待manager正式重分类；产品128列目标包的全通道稳定终点审计未由此替代。历史KNOWN_FAIL台账是否改状态须tester绑定构建复核＋manager裁决，不由本稿直接删项。

下一步建议：①developer/architect先解释/核验§2.2退款风险；②manager选择是否立项body替代律（有KF-2弱目标见证）或新增起沙来源设计（原厚层heavy不可绕过）；③立项后冻结源/速率/通道身份和调度，做逐笔shadow轨迹，输出实际accepted角、每相库存/矩、各层扣退账、最紧边、phase形状和尾段悬沙；④证书完整才交架构实现评审。不要仅为修复已有PASS的旧回归引入整套新机制。

**未证明事项明确保留：** 单个mu或速度未标定，不做多材料/容器2–3锚点插值；B1需要新来源和特定状态，不授权隐式shake；KF-2见证不保证用户pair或大场景可达；未完成预算退款路径实证；未证明±45°30°±2°、5.1s全宽形状/2px heavy或稳定终点，阶段二整体不标VALIDATED。
