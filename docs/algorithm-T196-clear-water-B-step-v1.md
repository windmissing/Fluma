# T196 单沙明水B段：完整子步、事务与聚合成本 v1

2026-10-28，designer，原卡 `6ae14cf6e4b0f11bdf2356a5`。目标7/P3，低于前六，compute目标6仍最高调度优先。路线 `SINGLE_SAND_CLEAR_WATER_V1`；本补篇 `CW_B_STEP_DESIGN_V1`，状态 **DESIGN_READY_FOR_ARCHITECT，未实施／未验证**。消费T122/T106、T107工程、T188开发及T189独立报告；A独立15/15仅证明其限定库存／源事务范围，不能外推B/C/restore。

不使用真机、不实施业务、不重跑产品测试、不回退共享修改。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列／队列／滴答同名清单。所有渲染含背景前景仅新XComponent+shader，禁止Canvas上屏／失败回退；故障FAULT或显式STALE上一有效完整新管线帧。manager按交付通知登记，不新增验收门。

## 1. 定稿范围及与A的兼容

采用**一次有界事件推进＋邻接水流／床流＋整步影子审核／线性重放**。B不启用merge/lift/隐藏列悬沙，不逐birth全池排序／复制／重建，不引入λ回退链。首版池满继续A的资源拒绝，所有出生仍由A源事务完成。B只处理已有粒子移动、介质切换、跨列、接触和LAND，床单相松弛与水邻接流。

不改变A：W/bh/mh权威，S/eta/P/R的RN顺序与ID递增归约；单沙phase0/source1，L/U=0；source行20/dedup56/event24原布局、key与payload位比较、幂等重放／成功序号／初始基准、queue64/dedup4096及不驱逐规则均保持。sourceId相同不等于同一成功源事务，新B来源扩展另列。

新增版本：`CW_B_STEP_V1`、`CW_B_EVENT40_V1`、`CW_B_LINEAGE3_V1`、`CW_B_SERIAL_PRIORITY_V1`、`CW_POINT_CONTACT_V1`、`CW_MASS_ACCUM70_V1`。适用legalState继续T107的 `CW_EXISTING_FROZEN_GUARD_POLICY_V1`，**不是完整P011_STRICT_REPRESENTABLE_STATE_V1**；不借本卡宣布其它严格核已支持。

A原`advanceSubstepInto(dt,acceptedAngle,out)`签名可保留。B在cold构造额外BConfig，不把缺字段当0默认；未装B仍维持A的正dt NOT_IMPLEMENTED，B支持后out.implementation升`CW_B_STEP_V1`。旧A snapshot V1维持原导出能力，不静默复用为B可恢复状态，见§9。

已实现事实以四个clearwater源码及T188/T189报告为准，指纹见 [设计证据](design-T196-clear-water-B-evidence-v1.json)。T189文字有将geometry列入key及将−0叫不同key的表述；本方案只沿A源码实际slots 0/3/4/5/6/7作为key、geometry与−0位差作为payload冲突，不修改原测试报告或自行重归因。

## 2. 入口、时序和唯一提交边界

N=128、C=256、K≤512沿A；帧dt≤1/30、子步dt≤1/60，最多两子步/active tick。angle单位是**度**，只接收已接受角、范围沿A±30，`rad=angle*Math.PI/180`，tan/sin/cos每子步固定一次并带角身份；B不直接采raw sensor，不自行额外搜角度或重试旧场景。

```text
boundary: A.enqueue / commitQueuedSourcesInto -> success source commit(s)
advanceSubstepInto(dt, acceptedAngle, out):
  dt==0 -> strict no-write; do not consume queue / event / budget / serial
  validate owner/config/B versions/time/angle/tokens/resources
  canonical.audit once; freeze stepBefore AFTER all successful source commits
  work <- stepBefore; reset private budget/plans/event arenas; build P exact accumulator
  water: visit even/odd edges once each, alternated first parity by stepSerial
  freeze etaMedium and wet mask
  particle: one ascending-ID activity-chain pass, <=e event rows per particle
  bed: one even/odd edge visit each, same fixed angle and remaining budgets
  final ID-order fold: reconcile post-bed point contacts, rebuild caches once
  full original audit + B contracts
  replay accepted rows from stepBefore in independent fixed replay bank
  compare actual inventories/pool/ID/free-list/events/budgets to work
  atomically accept state + B metadata + sealed rows + physicsCommit
  source queue/dedup/event24/initial/I_kind/sourceCommit are unchanged by this call
```

正dt每次只有一个物理candidate；任一fatal错误整子步FAIL、canonical不写；**无内部重试**。局部受物理预算／容量／guard阻挡可返回合法WAIT或零通量，区别于fatal（非法输入、有限域溢出、RN审计失败、arena不足／重放不一致）。本帧第二子步失败不撤销第一成功子步；source成功后两个子步都失败也保留source，返回lastAcceptedCommit＋failedAttempt/stage/ordinal。

step只从已有队列边界之后推进；不在逐粒运动里调用Source.commitNext或legacy inject。源队列若还含未提交请求，仍留待显式源边界，不让step暗中消费。busy从入口至最后封存，禁止重入；失败attempt编号可增长但不是成功physicsCommit/source序号。

## 3. 真实库存、容量与不逐粒重建的做法

### 3.1 相交换表

| 动作 | W | 床S | pool | 来源／预算 |
|---|---|---|---|---|
| 空气→湿／湿→空气 | 不变 | 不变 | 同ID/质量，仅运动介质 | 非源，不制造水／沙；纯运动不扣质量转移预算 |
| 水邻接流q | donor−q、receiver＋q的实际RN值 | 不变 | 不变 | 无I_water；独立水传输统计，不扣固体预算 |
| 粒子跨列 | 不变 | 不变 | 同ID整包从旧列到邻列，无分裂 | 质量q扣一次total，两端各扣一次local；不动源账 |
| LAND q | 不变 | mh增加实际q | 同ID减q，partial保留，full退slot一次 | total/local扣实际控制量q，不造新ID／来源 |
| 床邻接流 | 不变 | donor减、receiver mh增的RN图 | 不变 | total一次、两端local一次；单种组织变化不成第二沙种 |

因此实数中每个内部动作保持ΣW、Σ床＋Σpool；LAND使η=S+W抬升，随后下一子步水流响应。仍采用稀疏点粒子／不渗透有效床：飞行粒子不排水，水内阻力无水流携沙，水流无惯性浪／飞溅；点沙进入水时**没有**隐藏的W→P转换或水源事件。

### 3.2 A的RN缓存不可用逐次加减冒充最终值

A的P[c]是递增ID顺序逐个RN加质量，且audit要求P、pool.colParticleMassH与独立同序归约相等。逐个LAND/crossing对P作`P+=/-=m`，通常不等于末尾重归约；每个事件都全池rebuild又会变成O(KL)。本版两者都不选。

step私有 `CW_MASS_ACCUM70_V1` 为每列维护**精确存储质量之和**，只用于局部容量预检；不取代A的最终P定义。binary64非负m以2^-1074单位编码为整数：次正规整数是fraction位，正规是`(2^52+fraction) << (exponentField−1)`。定长70个16-bit limb、低位在前，1120 bits，scale=2^-1074；每次只做加／减／比较，无乘除、无FMA、无浮点求和误差。每个16-bit进位中间量≤131071，可用整数精确的Number计算；cold DataView复用8B，不热new。

由A合法R≤C、所有质量非负得每单粒质量≤C。最多K项，S/W≤C，精确容量量E=S+W+Σm≤(K+2)C，主档<2^18；70limb可承载<2^46的实际数值，含subnormal全部位型，足够上述域。超域、负减法或非finite返回错误，不截高位。这个新加法专用核须独立oracle，不冒用T133短表达式白名单已获认证。

在子步入口冻结每列 `E0=exact(S0)+exact(W0)+exactPoolSum0`，私有容量界 `Bcap=max(exact(C),E0)`。A的RN合法态可能E0在C之上不足一次舍入而R仍≤C；保留其输入，不原地修库存。每个局部实际patch要求新E≤Bcap，超过则该patch受阻；Bcap整个candidate固定，不逐事件换基准。此为**保守预检／原态non-worsening辅助界**，不是新产品容量容差：最终仍无条件执行A原R=RN(RN(S+W)+P_IDfold)≤C，失败整步回滚。不能拿Bcap把原R超C认证为成功。

crossing整包只需精确P_acc旧列减m、新列加m；LAND用实际`mAfter−mBefore`更新、S用实际CanonicalS，故能捕获RN余量与床加法差异；床／水patch也用实际S/W值而非名义±q计算E。每事件最多两列，每列O(70)固定工作，不扫pool。额外的守恒实际差分按§5执行。

最后清P及colParticleMassH，按同一递增ID单遍折叠，向两者同时写入完全相同RN序列；重建S/eta/R一次，再由A.audit独立fold检查。这个重建不修改权威W/bh/mh/pMass，不是钳零或修质量。私有exact accumulator可重建比对，但不能把其rounded结果替代IDfold。

## 4. 水核：有界邻接流和RN后的真实检查

沿T106/T107，x=c+.5、u=tan(rad)、z=S−x*u、head=eta−x*u。每个物理邻边一次，先访问某奇偶集合、再另一集合，总H=N−1条，不是每列扫描所有接收列，也不是无限迭代到水平。S/P在本阶段固定，W实时更新。

```text
pick d/r with head[d]>head[r] (tie -> ZERO)
delta = head[d]-head[r]
q = min(kappaW*dt*delta, delta/2, W[d],
        max(0,head[d]-max(z[d],z[r])), max(0,C-S[r]-P_IDfold[r]-W[r]))
stage Wd'=RN(Wd-q), Wr'=RN(Wr+q)
check finite/W>=0, both actual capacity E<=fixed Bcap,
      local actual sum conservation <=1e-6 and positive changes on both ends
if q>0 but either W store unchanged: ZERO/NO_EFFECT, no water success event
if auxiliary E>Bcap: ZERO/CAPACITY_NO_GRANT, no partial pair write
if actual numeric audit invalid: STEP_FAIL (no adaptive q search)
else accept local pair; append WATER row
```

q=0不写±0、不扣预算；q上界中max(0,available)仅裁剪**请求余量**，不能把旧负W改0。有限域运算溢出FAIL。实数模型势能ΔE=−q*delta+q²≤0，但RN实测前后能量和控制量公式分栏，不声称RN也逐位守恒／绝不增能。完整子步再查总水绝对差≤1e-6及A全局drift，不能因每对≤1e-6就省总检查。新增能量数值误差观察未标定时记录值，不捏造通过门。

水流结束冻结etaMedium[N]与wetMask[N]；之后LAND／床变更更新当前S/eta，但不回写介质面。抬面排水有明确一子步延迟。下一子步必须重新建面，不用旧publication的水位。

## 5. 固体预算、guard和有界patch

子步预算：totalLimit=RN(300*dt)，每列localLimit=RN(28*dt)，沿T107既有ceilings、不跨步积攒、不放大dt。LAND用本列local，cross/bed用两端local和一次total；纯运动/介质/水不消耗固体质量预算。处理顺序固定为递增ID粒子事件→床边奇偶顺序，版本 `CW_B_SERIAL_PRIORITY_V1`；不是旧多通道全局比例调度的逐位等价声明。预算不足允许WAIT／部分LAND，可能出现晚ID或床通道当步无grant，记录clipped，不提高上限掩盖。

每ticket先算qBudget=min(request,remainingTotal,affectedLocal)，借用一次reservations语义：预留→capacity/guard裁剪→退还未用差额一次→接受，或完整abort。不能先在真实budget反复reserve/refund试q。固定1次shadow局部实际patch，若无法安全接受则材料ZERO／WAIT；局部fatal则整步abort。所有candidate资源只在私有budget，canonical成功时封存；失败预算整步恢复，无退款外发。

guard以实际changed columns取全部受影响边（单列≤2，相邻对≤3）。writer入口冻结d=RN(RN(Sright−Sleft)−u)，k按ΔS系数；bound为原H或原违规|d|，沿p011GuardAdmitNumericInto的闭式区间规则，H=tan30及原EPS不变。得到名义q后**实际执行bh/mh/CanonicalS RN图**，再按同入口bound复核实际边，不将k*q当真实ΔS。实际越bound可材料ZERO而非加epsilon／逐ULP重试；其余局部stock／容量／守恒失败按明确错误回整步。整步另以入口每边`max(H,abs(dEntry))`核最终non-worsening，避免逐writer换基准累计恶化。

不宣称所有初态／新角立即满足全局|d|≤H。A能初始化陡床，已接受新角也可能使原床暂时违规，B只按冻结规则不恶化并由床流逐步降低；out同时报告maxResidual/allEdgesWithinH。若上层需要完整strict FINAL，必须用已证明的另一revision和支持核，不能把本policy的合规step叫严格全局PASS。

真实patch必须同时检查：非负库存／同ID、phase0/source1、Pacc/Bcap、实际局部水／沙前后差≤1e-6、预算剩余／grant/refund与原EPS作用域、受影响边actual guard。最终总质量和A drift比较不改：`abs(actual−expected)/max(expected,1e-9) >= .001`为失败；绝对界仍1e-6，不能把小库存／次正规沙当0丢弃。控制q、pool实际差、床实际差和RN函数值分栏，不补尾相凑守恒。

## 6. 分段粒子、等待和LAND

### 6.1 明确积分图和事件顺序

BConfig显式加入gModel、dragAir、contact/motion版本及e/E/lineage资源上限；gModel/dragAir候选均PENDING_CALIBRATION，不把现有9.8/.01当已批准目标7值。湿dragWet/settlingSpeed继续A配置。空气段h：vx'=RN(vx*exp(−dragAir*h)+gModel*sin(rad)*h)，vy'=RN(vy−gModel*cos(rad)*h)；湿段vT=(settlingSpeed*sin(rad),−settlingSpeed*cos(rad))，v'=vT+(v−vT)*exp(−dragWet*h)。位置提出 `p*=p+v'*h`。湿段不再加重力／浮力，不引入未定义水流速度。

在当前列用候选**直线弦段**与床、墙、列边界、etaMedium求最早λ∈[0,1]，时间hEvent=RN(h*λ)，位置取弦段交点、速度按hEvent重算；剩余时间继续。它是显式分段近似，不声称求了真实非线性轨迹根。事件同时间优先：当前／目标床接触→容器墙→列边界→水面；所有tie依据实际同一λ，不引入新epsilon。移动进入较高邻床时，在邻列边界等待，不把粒子瞬移到高床顶再叫LAND。

水面λ=0根据出射方向／前事件plane tag明确下一介质，不能两次处理同一plane零时间来回。列边界也用入／出方向tag防重复；若正时间hEvent经RN使remaining不减且无合法拓扑变化，STEP_TIME_NO_PROGRESS，不能塞epsilon时间推进。总event计数含MOVE、MEDIUM、CONTACT等，达上限不能继续就整步EVENT_LIMIT，不丢剩余时间或把尚未推进称成功。

递增ID活动链每个存活入口节点只访问一次，处理前保存next；full LAND O(1)摘链／退slot，B无birth/merge，故不会漏／重复访问后续ID，不每粒从链头找自己。先后粒子可能改变当前S，接触用**当前床**而非冻结床；介质使用冻结eta，若当前S已高于eta，床接触优先，不让粒子深入床再按旧水面算湿运动。

### 6.2 跨列是整包，接触等待与材料ZERO分开

跨列预检整包质量m与接收Bcap，预算不足m不做部分cross／不分裂ID；退回全部预留且不改质量／来源。右边界受阻位置为nextDown(c+1)，左边界为c，确保floor(x)仍旧列；容器墙同理，位置合法域[0,N)。接受向右跨列x=c+1、向左x=nextDown(c)，列缓存仅更新私有acc；不使用1e-9几何魔数。

等待时法向速度置0、切向保留，余下本子步停止，记录等待原因与位置/速度变化；`pState=2`配独立waitReason区分BED、SIDE_CAPACITY、SIDE_BUDGET、SIDE_HIGH_BED。下一子步重新评估，不能把侧等待永久当床接触而LAND。质量m超过单步local上限的包会长期不能cross，本版明确BUDGET_QUANTUM_BLOCKED诊断，不放大28/s、跨步攒预算或偷偷拆包；合适massPackage/dt候选须标定／用例覆盖，不对A所有配置承诺可横移。

点粒子床接触位置取**当前S**（不额外加会越C的contactEps），在[S,C]内；屏幕半径不是体积／碰撞半径。已有粒子若因之前LAND/床变化处于床内，只能经明确CONTACT运动patch投影到当前S，标出old/new位置，不能暗改当无写ZERO。材料q=0时mh/pMass/free-list/ID/预算无写；首次contact的state/y/vy变化另记CONTACT。这沿T110对无写ZERO的区分，不把新contact核说成旧strict contact已获证。

床流发生在粒子推进之后，可能新抬床埋住已推进粒子。因此在末尾既有ID-order缓存fold中融合一次几何收尾：若live粒子y<当前S，追加一行CONTACT，将其点位置置当前S／vy=0并转BED_WAIT，质量及W不变，下子步再请求LAND。若BED_WAIT的支撑下降使y>S，则追加CONTACT解除支撑，保留原y并转AIRBORNE／waitReason=NONE，下子步正常重力推进，不能无时间地瞬移下落。每粒e上限**预留1行**给该收尾，运动主循环最多e−1行（e>=2）；总数仍≤e。收尾不重新推进时间／做二次LAND／再次床流，不开启嵌套循环。最终B几何要求live y>=S，SIDE_WAIT若被新床覆盖则明确转BED_WAIT。它是现有最终fold的固定工作，不额外每床边扫全池。

### 6.3 LAND操作图及退役

```text
request = current pMass m
q = budget+guard candidate in [0,m]
newM = (q===m ? 0 : RN(m-q)); newMh=RN(mh+q)
newS = RN(RN(RN(bh+0)+newMh)+0)
if q>0 and (newM==m or newMh==mh): material ZERO / NUMERIC_NO_EFFECT
stage actual mass differences, acc(P) adjustment, geometry and budget
partial -> same ID/source/origin, state=BED_WAIT, y=newS, vy=0
full -> mass0/state0; unlink once; freeSlots push once; liveCount--
W unchanged; nextId unchanged; I_sand/sourceCommit unchanged
```

newMh加法可能舍入，newM残量也可能不是实数m−q，不能口称S＋P逐位守恒；用真实差分和整步审计判定。partial仍在同一slot，同来源成功事务；full原ID终结，后续source在该slot出生也必须是新ID。NO_EFFECT不产生成功LAND／来源结算，接触运动可单独成立。W不因床排水直接减掉，避免漏水。

## 7. 单相床流：固定边数、完整stock图

粒子之后，对N−1床边各访问一次，奇偶先后随成功substepSerial切换。取r=RN(RN(Sright−Sleft)−u)，若|r|≤H请求0；否则donor为高相对水头的床列，`qReq=min((abs(r)−H)/2, donor mh+bh, maxAvalancheFlux*dt)`。maxAvalancheFlux作为B显式候选PENDING_CALIBRATION（旧24可参考但不冒已标定）；本版固定一次奇偶扫描，不继承旧5轮或其它通道。

在接收容量、total/local与三边guard后，固定stock图：`fromM=min(q,mhD)`、`fromB=RN(q−fromM)`，核fromB≤bhD；donor mh'=RN(mhD−fromM)、bh'=RN(bhD−fromB)，`hActual=RN(fromM+fromB)`，receiver mh'=RN(mhR+hActual)。CanonicalS按A次序；事件记录q控制、fromM/fromB和所有真实前后库存，实际边／守恒／容量审核。即使单沙也不能将q直接当精确S变化。

不额外固结mh→bh，不开percolation/deep creep/lift/merge或双沙hysteresis。bh可作为同种固定床被top不足时取用，与T106 top-first定义一致；若场景需要不可动基岩，另配置／方案，不能擅自把所有bh当永久不可动。一次扫描未达到H并非无限修到通过，残差真实输出。

## 8. 事件、origin和线性重放

A source event24原样保留，B独立七kind：MOVE/MEDIUM/CROSS/CONTACT/LAND/BED/WATER。event40一行40个8B槽：

```text
0 kind,1 ordinal,2 slot,3 particleId,4 originSourceCommit,
5 fromColumn,6 toColumn,7 requested,8 qControl,
9 massBefore(or BED fromM),10 massAfter(or BED fromB),
11 x0,12 y0,13 vx0,14 vy0,15 x1,16 y1,17 vx1,18 vy1,
19 bhD0,20 bhD1,21 mhD0,22 mhD1,23 bhR0,24 bhR1,25 mhR0,26 mhR1,
27 Wd0,28 Wd1,29 Wr0,30 Wr1,31 totalBefore,32 totalAfter,
33 localD0,34 localD1,35 localR0,36 localR1,
37 lineageStart,38 lineageCount,39 flags
```

flags位域冻结old/new pState、old/new waitReason、old/new medium及reason；不用字符串热拼接。同行无意义字段清0，BED9/10的kind分义由版本明示，不把0当粒子有效ID。metadata头统一run/config/input/angle/substep/attempt/physics/source commit、arenaSerial/revisions；成功事件只随整步commit封存。

对MOVE/MEDIUM/CROSS/CONTACT，19–26不需要床库存时明确复用为`remainingTime0/remainingTime1/hEvent/lambda/planeType/planeCoordinate/medium0/medium1`，使事件分段时间可复核；LAND/BED仍按床字段定义，必要的contact动作单列CONTACT行，不把它塞入看不见的LAND副作用。kind决定字段解释，manifest冻结每kind掩码。双particle summary的16槽定为`id,slot,origin,state,waitReason,column,mass,x,y,vx,vy,processedTime,remainingTime,eventStart,eventCount,flags`，不能摘要声称完整动作。

lineage3为 `{particleId,originSourceCommit,edgeKind}` 三个8B槽：MOVE/MEDIUM/CROSS/CONTACT是同一live ID状态边，partial LAND是同ID剩余＋向床边，full LAND标终结。每event至多2边，BED/WATER无粒子边。B不创造新birth／merge或新source身份，不能把SAND sourceId替代成功transactionId。

每slot新增originSourceCommit和waitReason；source正成功birth时在同一影子事务按birth范围填origin=sourceCommit，失败不写。已有A run进入B时可做一次**cold attach**：扫描source event24，跳过water行，将沙行压到定容区间索引 `{firstId U32,lastId U32,originCommit F64}`，每行16B、最多D4096行。按其已递增次序验证不交ID区间后对每live ID二分查origin，O(D+K logD)，不对夹有water零ID的原事件表直接假设firstId全局有序，不逐粒逐事件扫D。无来源区间／歧义拒B attach，不能猜sourceId；A source仍可独立使用。这个attach不是snapshot restore，也不改旧event24/dedup布局。索引为cold工作区，计入attach峰值，保留复用或释放的生命周期须明确。

arena双bank，一份候选、一份封存／比对；固定E行、Lcap≤2E条。每次写之前检查剩余行与lineage，无空间整步失败，不在已移动一半后丢日志。资源不足不通过降低样本粒数／省事件伪成功。

第二固定replay bank从stepBefore复制一次，按事件顺序核旧位型／ID／origin／局部预算，重放actual新值及O(1)链退役；每row至多2列、≤3边、O(70)容量检查，末尾重建及独立A audit一次。两份固定particle summary plan各K×16 F64用于before/after摘要，事件才是完整动作证据，不能用summary冒七类事件。summary.eventStart是该粒首ordinal、eventCount是总数，含BED后的收尾CONTACT，**不承诺该粒所有事件在arena连续**；重放按全局ordinal，不能将start/count直接当连续slice。work与replay的authority/pool/free/active-chain/origin/wait/budget结果逐位相同，才封存；无需把同一个轨迹再完整推演λ次。replay不是独立tester oracle，仍需独立用例，不据自重放签产品正确性。

## 9. snapshot、restore与C边界

B成功返回的是物理commit和受控只读／冷复制的B证据，不等于C已实现FrameToken／页面。C publication必须等待B真实交付和独立验证，再从一个已接受commit组装S/W/eta/P/pool/source版本；不能拿A库存snapshot截图冒完整同帧明水。触摸切水沙／时钟输入Controller与shader透明层仍属C，不因B无页面而绕到Canvas。

保留A `snapshot()` V1原行为：它只覆盖A字段，不可恢复B。新增冷 `snapshotB()` revision `CLEAR_WATER_SNAPSHOT_B_V1` 才包含全部canonical authority、A queue/dedup/events和环头/序号/时钟、acceptedAngle/substepSerial、origin/wait、B配置及event/lineage封存身份。私有work/stepBefore/replay/accumulator不能作为可恢复authority。

**本B实施包不实现restore**，接口若被请求返回UNSUPPORTED_FEATURE，不做部分复制。未来restore工程要完整版本／长度／alias／位型／来源／free-chain／预算/guard状态校验后全体原子替换，旧V1／旧wet无W或缺B字段明确INCOMPATIBLE；不能从waterTop或sourceId猜W/来源。现在的cold attach只能从同一个活A owner读取受控原件；不接受调用者改过的A snapshot作为迁移捷径。reset明确新run，清队列、源／B事件／来源／等待／stepSerial与clock，不沿旧ID恢复，原A初始库存规则不变。

## 10. 整step／帧复杂度：把所有乘数列出

变量：N列、K池容量、A_s为第s子步活粒数，b_i为第i源事务birth数，Q源事务数（单边界≤64）、D幂等／来源事件数（≤4096）、F帧子步数≤2、e每粒事件cap，L_s≤e*A_s实际粒子事件，H=N−1水边、J=N−1床边，V为上层完整候选／重试次数。**本版V=1、局部材料patch最多一次、无λ、无merge重建；replay是一次线性证据检查，不多一次物理候选。**

```text
T_enqueue per request = O(D+Q+N+K) worst
  // A.reject/prepareReceipt includes global totals; not pretend enqueue is O(1)
T_sourceBoundary = O(Q*(N+K) + sum b_i), plus actual enqueue costs
  // A each transaction copies/audits/rebuilds a bounded number of times,
  // its birth loop is O(b_i), not b_i*(N+K); pool-full has no merge.
T_step(s) = O((N+K) + 70*(N+A_s+L_s+H+J))
  // fixed entry/copy, acc build, local events, two final/replay audits/copies
T_frame = actual enqueue sum + T_sourceBoundary
          + sum_{s=1..F} V_s*T_step(s) + T_publication_C_when_implemented
```

上式采用一个active tick一次源边界G=1、随后至多两子步。若调用方在一帧显式提交G次源边界，必须把`T_sourceBoundary`改为`sum_{g=1..G} T_sourceBoundary(g)`、Q/b_i逐边界累计；接口允许反复被调用不等于成本免费。无B出生／退役插入的单个源边界内，成功birth总数≤入口freeCount≤K；多边界之间若有LAND释放，整帧birth可大于K，不能拿单边界K限制漏算。

70是固定数值字宽，故主域T_step=O(N+K+L)，e固定则O(N+K)。空间O(N+K+E+Lcap)；不隐含O(KL)、O(K²)、逐birth sort或逐事件whole-pool copy。cold attach O(D+KlogD)不进每帧；snapshot复制O(D+Q+K+N+E)，禁止UI每帧调用或从性能计时隐藏。A线性dedup扫D成本真实保留，没在本卡顺手改索引；若成为热点另经architect归因。

主档资源候选e=8（资源上限，不是精度／效果合格线），E=eK+H+J：

| 主档 | 水边H | 床边J | 粒子事件上界 | event40容量E | lineage上界2E | 全池audit调用上界／step |
|---|---|---|---|---|---|---|
| N128/K192 |127|127|1536|1790|3580|3（入口/work final/replay final）|
| N128/K512 |127|127|4096|4350|8700|3 |

每粒e含全部MOVE/MEDIUM/CROSS/CONTACT/LAND及床后收尾CONTACT，主循环e−1、收尾预留1，不能在统计中只数crossing而将motion另无限增长。任何超e、E、Lcap失败保留原始计数／首次ordinal；不丢时间／事件。e增大须配置／内存manifest升版，不作为通过测试自动调大。最终fold与audit每次O(N+K)，只限上述固定阶段；“活动链一轮”仅指轨迹积分遍历，不能据此隐藏固定copy/fold/audit/replay遍历成本。性能采样不插入每事件snapshot。

新增B载荷**建议上界**（未分配／未测量，不是整机RSS预算通过），在A实际2,802,784B基础上加：

| 新增项 | 字节公式 |
|---|---|
| replay state bank（含A pool/活动链、7数组/auditMass） |63K+80N |
| 五state bank的origin F64＋wait U8 |45K |
| etaMedium F64＋wet mask U8 |9N |
| P精确acc、入口Bcap各70N个U16＋两行scratch及bit view |280N+288 |
| budget固定local相关6N F64及标量区 |48N+256 |
| 双particle summary plan，K×16 F64 |256K |
| 双event40 arena |640E |
| 双lineage3 arena，每bank 2E |96E |
| 五bank B标量／封存header预留 |2048 |
| cold attach沙源区间工作区（D=4096，非热增长） |16D=65536 |

K512/N128/e8下新增B热载荷3,443,936B，含cold attach区间工作区的峰值3,509,472B；加A为6,312,256B，精确拆分见manifest。不含C双publication/native/profile/GL、对象／对齐／栈／driver、旧场景存活和调用者保留snapshot。B snapshot的A原2,674,176B加B权威扩展及一份封存事件/lineage另累加，不将它塞进A固定表。事件arena比数值数组大，工程应优先固定字段／复用bank；若资源冲突GPU，目标7让位，不偷偷少记事件或驱逐dedup。

计时覆盖source enqueue/commit、copy/audit/acc、hydro、particle各kind、bed、replay、publication／bridge后续成本；报告真实Q/B/F/L/V、p50/p95/最坏/首次超限、主规模192/512及无匹配/满包/事件上限矩阵。不从离散模型秒数推端侧帧时。没有设备实测，不声称不卡死或达到既有帧预算；不发明新百分比性能门。上层若未来引入V>1、worker/GPU或merge，必须重写聚合界和测量完整事务，不能继续引用本版V=1结论。

## 11. 候选取舍与可测合同

推荐本版：固定数值acc作局部预检＋末尾A同序fold、单candidate事件推进／固定replay，保留顺序与审计。拒绝每event rebuild/copy/audit（O(KL)）、每birth seed/λ/full-index（与目标7当前无merge范围不符）、增量P加减当最终精确cache（RN不等价）、全量改树形P归约（破A位序兼容）、无界q二分／逐ULP／事件丢弃（不可证明上限）。全部浸没体积水动力/SPH与完整strict／compute生产迁移仍另案，不借B扩大范围。

独立tester合同沿CW原18项，补以下B编号；缺实现预测试NOT_IMPLEMENTED，不冒产品缺陷／PASS：

| ID | 场景及可核断言 |
|---|---|
| B-01 | 逐字保留A回执/key/负零payload/源账、队列拒绝与15项独立范围；dt0所有库存/队列/成功ID/预算无写 |
| B-02 | source成功→hydro/运动/guard/audit/replay任一注错step：保留source commit，step/event/预算无半成功；第二子步失败保留第一 |
| B-03 | 水每对q五上界、空/浅/隔床/墙/倾角、q低于ULP无假成功；ΣW绝对差及原drift审核，实数/实际能量分栏 |
| B-04 | AIR/WET跨面、出水、列边同刻/λ0/λ1/床优先，速度按给定RN图、位移按弦分段；finite失败不透床，参数待标定 |
| B-05 | 横向整包／预算不足／接收满／邻高床等待：无partial ID分裂、x仍正确floor；SIDE_WAIT不误LAND |
| B-06 | CONTACT材料0不改质量/床/free/成功LAND；运动patch可变state/y/vy并单列；S=C不加contactEps越顶；床后覆盖/支撑改变融合最终fold，事件预留与末态y>=S正确 |
| B-07 | full/partial LAND实际m/S变化、同origin/ID、slot释放一次、nextId不变、W不变及下一步水头响应 |
| B-08 | 床top-first mh/bh的RN图、两外邻边与冻结non-worsening、容量拒绝、300/28按dt及refund一次；不启旧额外通道 |
| B-09 | adversarial质量1、2^-53、2^-54等：incremental P不等IDfold；最终P/cache与A同序位型相同，exactacc不代替canonical P |
| B-10 | acc70对独立精确整数oracle覆盖normal/subnormal、跨limb进退位、重复cross/LAND、入口RN合法但exact E>C；Bcap不豁免最终R<=C |
| B-11 | 来源区间cold attach、ID上限/事件耗尽/lineage cap/下一run、replay前置位型损坏：错误有身份、无半日志、无来源串账 |
| B-12 | step调用计数：whole-audit≤3、活动链只一轮、无逐birth rebuild/merge/λ；N/K/B/Q/F/e矩阵及所有失败阶段成本完整 |
| B-13 | snapshot V1仍只A、B snapshot完整且隔离；restore明确未实现、旧schema拒；reset新run源及B代全清，不猜W/来源 |
| B-14 | B输出物理commit不冒C FrameToken／页面完成；C/触摸切换/透明同帧及设备观感继续真实NOT_IMPLEMENTED／待独立卡 |

新增响应、减速、清晰度和事件分段误差仍PENDING_CALIBRATION；逻辑不变量必须测，新增数值门槛待同路线测量／裁定，原DRIFT/EPS/H/预算不动。效测后续聚焦可见入水减速／同种积床抬面、真实水向低头流、轻倾角方向、浅深干满及切源停止；不拿A15/15或本模型代替效果／设备数据。

## 12. architect可执行任务书

不使用真机。依据本篇、T107及A现行接口，先修订对应工程文档，新增B补篇／版本与逐字段manifest，再交developer；不是本卡实施授权。优先新增clearwater Hydraulics/Particles/Guard/Step/Events/ExactMass模块；Engine/Types/State/Source只明确所需owner hunk（busy/原子commit、B metadata copy/reset/source birth hook），不动旧WaterSandField/M2/GPU/renderer热文件，不回退共享修改。

工程须落实：①70limb加减比较/encode和域错误、原cache位序；②BConfig/accepted-angle身份/资源caps与错误码映射，旧22个status不重编号，新增step reason独立版；③event40/lineage3/flags与cold origin attach，源24行不重释；④完整source/step/replay/arena commit协议与5bank无alias；⑤上表allocation精确数、热零分配、整frame计数入口；⑥保留restore/C待实施边界和对应测试先行。

失败首次归因仍architect；新B未实现预测试RED到件后登记NOT_IMPLEMENTED结束本轮归因，不能误派缺陷修复。开发仅完成实现／编译／离线自测，独立tester与后续设备effector另卡；不把真机、manager核查或特定回执措辞作为开发关闭前置。若实施需改变本版本无merge、V=1、预算顺序／近似或A接口语义，明确设计变更回designer，不逐热点补丁后宣称整体复杂度解决。
