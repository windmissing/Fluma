# T106：单沙、明水与双注入场景算法方案 v1

> 2026-10-27 T122交接补记（`T122_HANDOFF_V1`）：本路线现归**目标7单沙＋明水**，P3排在前六目标之后；目标6是仿真compute shader管线、保持GPU最高调度优先。下文2026-10-26正文中的“目标六／前五”仅保留历史含义，当前排期以本补记为准。算法、接口、18组CW合同及阈值不因编号调整改变。T107/T108消费定位及存档身份见 [T122交接说明](algorithm-T122-single-sand-clear-water-handoff-v1.md)。原盘点为2026-10-26快照，当前输入差异单列于T122交接manifest；未重跑测试、未宣称最新产品实现／验证通过。

2026-10-26，designer。原卡 `6adf343de4b06ee35743c769`，目标六，P3，排在现有前五目标之后。路线 `SINGLE_SAND_CLEAR_WATER_V1`；状态 **DESIGN_READY_FOR_ARCHITECT，尚未实施／验证**。本轮不使用真机、不改业务代码、不修改前五目标及其阈值。共享文件冲突让位前五目标。

所有画面包括背景／前景均在新XComponent+shader管线；禁止Canvas上屏及失败回退。失败报告FAULT，或保留并明确标注STALE的上一有效新管线帧。

## 1. 推荐方案和可见效果

推荐“独立水库存＋守恒列间水流＋真实单沙粒子＋沉积床面”的最小闭环。用户选择掉沙或掉水：掉沙生成真实粒子，空气中下落、入水后减速、接触后通过LAND进入床库存；掉水在触点对应列的水面入口增加真实水量，水向低水头处流动。落床抬高床面与本列水面，随后挤出的水保守地流向邻列。倾斜改变重力方向和静水水头，水面、粒子和床使用同一已接受物理角。

**近似边界**：水采用过阻尼水头松弛，没有惯性波；飞行沙采用稀疏点粒子近似，入水时不占据水体积，LAND后才以床体积排水。床视为不可渗透的有效体积，不模拟孔隙、湿沙黏聚或毛细。因而这是可测的最低沙水耦合范围，不能称为密集悬沙双向排水、SPH、浪涌或飞溅。掉水首版是水面质量入口，**不声明已模拟空中水滴飞行**；不能画无库存的飞行水滴冒充该能力。

若后续效果要求“大团沙入水即抬水／水花”，须升级带粒子浸没体积的排水模型；本版不靠降低透明度掩盖近似。全局仿真目标与其它场景继续保留，本路线不是其替代品。

## 2. 现有能力盘点：源码存在不等于目标六通过

本轮读取源文件及历史报告，指纹见 `design-T106-single-sand-water-evidence-v1.json`。这是设计盘点，未重跑产品或设备测试。

| 位置／能力 | 已有事实 | 目标六差距与复用边界 |
|---|---|---|
| WaterSandField 的 bh/bl/mh/ml、suspH/L、pool、审计 | 床／列悬沙／真实粒子与分相账已存在 | 单沙身份和独立W尚缺，不能把两色隐藏成单沙就改物理定义 |
| DRY_AIR_V1、T59及T73 | 干空气触点粒子路径已实现；T73报告17/17离线PASS | dry waterTop是S＋U＋P占据包络、实际水为0；不能把它拿来画水面。未证明目标六设备表现 |
| 旧wet粒子推进 | 根据粒子y与旧waterTop选择空气／湿阻力，有终端沉降 | 旧waterTop与空气中P的体积语义不适合直接当明水η；需明确新模式与W。空气段倾角也必须统一 |
| runtime addWater/drainWater | 真实水事务及waterTransactionNet存在；DRY_AIR拒绝 | 加水按全场余量分配，缺触点局部源与双源成功身份；不能直接标为新交互完成 |
| legacy inject／initialize | 可初始化或改变场景 | 部分路径重设initial守恒基准，禁止作为运行掉水／掉沙入口 |
| levelWater | 有倾角目标水位与水量搬运 | 现有全局目标＋donor/receiver扫描不是本版局部水头通量，不能继承其结果为新路线通过 |
| P051TouchSourceAdapter、GestureRateClock | 单主指针、几何代、停止代、明确ns/ms、首采样0 | adapter当前绑定干沙demo与dry源revision；缺kind、独立水账及按住切换屏障 |
| T56新scene、ScenePreparation／Types | 同帧publication、完整scene、新管线LastGood／FAULT机制存在 | Scene packet没有本版独立W合同；历史水雾层不等于明水 |
| scene_layers.cpp / scene_compositor.cpp | 历史WATER_FOG灰绿不透明／雾纹与界面雾层 | 新模式必须禁用这些雾化水层，采用透射合成；不能仅换标签或调旧fog density |

复用pool身份／LAND事务骨架、守恒审计、输入停止语义、FrameToken与shader呈现事务；新增库存／水通量／源接口／水光学合同。T99/T105条带等其它路线资产只有实现并注明版本后才能可选复用，不作为本方案已具备能力。

## 3. 权威状态、单沙身份与守恒

### 3.1 状态定义

物理模式新增 `SINGLE_SAND_CLEAR_WATER_V1`，不随W变0而切到DRY_AIR，也不复用WET_COLUMN字符串冒充语义兼容。单一物种 `SAND_1`，可用现有phase H/0承载；bl/ml/suspL及L粒子严格为0。bh/mh分别是同种沙的固定／活动床组织，不是两种材料或颜色。所有颗粒使用同一套材质参数。

每列宽Δx=1，单位为模型长度，体积为单位厚度下的模型面积；不是直接把列高叫毫升。定义：

```text
S[c] = CanonicalS(bh,0,mh,0)       床顶，仍沿既有求和次序
W[c] >= 0                        显式实际水库存
P[c] = Σ live particle.mass       本列全部真实飞行／CONTACT_WAIT沙
U[c] = 0                         本路线不准隐形列悬沙替代pool
eta[c] = RN(S[c]+W[c])            真实自由水面（W>0才有水层）
R[c] = S[c]+W[c]+P[c] <= C        保守容量占据账
```

W是权威量，η是同帧衍生量。W=0时η=S，但不绘制水；不得通过`max(0,旧waterTop−S−U−P)`从非法态反推W并吞掉负值。旧dry占据包络若调试需要，另名`occupiedEnvelope`，不能再叫真实水面供shader／湿判定使用。

R把空气中粒子也预留为未来落床体积，**是保守容量政策，不是液体体积公式**。它可能在η尚未到顶时拒绝加水，回执应给出PARTICLE_RESERVED_CAPACITY，不能谎称水已满。粒子横向换列、床搬运、lift、LAND都要在同一R约束下准入；不得只限制外源而让内部转移越容量。粒子半径不计入体积分配，沿既有边界几何处理。

### 3.2 独立成功源账

初始化／reset形成新run和一次性基准；运行注入不重设基准：

```text
SandExpected = SandInitial + I_sand
WaterExpected = WaterInitial + I_water
SandActual = ΣS + ΣP             U=0，无隐形fallback
WaterActual = ΣW
```

I_sand、I_water为各自成功提交的体积累计，分开成功次数、序号和事件。首版封闭容器无流出，墙面零通量；未准入余量是rejected，不是已注入又消失。将来排水必须新增可审计O_water，不能夹在本版add事务里。

保留当前审计函数的原比较方式／归一化／零基准处理，现行DRIFT_LIMIT=0.001与相关逐步守恒1e-6不变；T107须逐项映射原审计到新权威W与S＋P，不能新造分母或容差。源累计数学量和库存RN后的实际差分分别记录，不把浮点加减宣称逐位精确守恒。遇原线失败整个候选回滚，交architect归因。

## 4. 水流与沙水运动闭环

### 4.1 局部保守水流

使用与粒子、床相同的已接受θ，x_c=c+0.5，`t=tan(theta)`：

`z_c=S_c−x_c*t`，`head_c=eta_c−x_c*t=z_c+W_c`。

相邻列按水头高低选donor d、receiver r；头差非正则零通量。固定一遍偶数边、一遍奇数边，子步奇偶交替首遍顺序，边内实时读已接受状态；无全列两两配对。每次固定床与θ：

```text
delta = head[d] - head[r] > 0
crest = max(z[d],z[r])
F_r = C - S[r] - P[r] - W[r]
q = min(kappaW*dt*delta, delta/2, W[d],
        max(0,head[d]-crest), max(0,F_r))
preview W[d]-=q; W[r]+=q
audit positive / capacity / actual conservation
commit pair or zero, never clamp an illegal old W into range
```

这是本方案的有界水头松弛，**不是浅水方程求解器**。越床门槛约束避免干高床无水也向另一侧“传水”；邻接与墙面约束避免跨山全局瞬移。每子步固定两遍，余量留给后续子步，不为“立即水平”开无限迭代。

实数模型中q≤W_d保证非负，q≤F_r保证容量，成对加减保证总水量不变。固定S/θ的势能 `E=Σ(0.5W²+zW)` 对本对变化为 `−q*delta+q²≤0`，所以无源、无床动的这一步不放大水头能量。浮点实现仍需真实操作后审计，这个实数证明不能替代RN检查。connected wet区域趋向head相同，即自由面η=常数＋x*t；干床可分离水域。dt=0整步不改状态，有限dt>0；κ单位1/s，数值和响应标定见§8。

不把此方案称为已有Audusse等静水平衡浅水格式；该原始研究可作为后续升级参考：[Hydrostatic reconstruction, 2004](https://perso.math.u-pem.fr/bouchut.francois/publications/hydroproj_published.pdf)。本版安全性只依赖上述成对界与自身证据。

### 4.2 空气／入水／沉降

空气中采用既有干沙重力与阻力结构，统一 `gVec=gModel*(sinθ,−cosθ)`，gModel单位模型长度/s²；传感器m/s²先经已有映射，不能直接混单位。水内判定为W[c]>0且S[c]<y≤η[c]，床接触优先处理。

水内使用明确的有效终端速度模型：

`v' = vTerminal + (v−vTerminal)*exp(−dragWet*dtWet)`，
`vTerminal = settlingSpeed*(sinθ,−cosθ)`。

dragWet单位1/s、settlingSpeed模型长度/s。首版忽略水流平移对颗粒的水平携带，避免从极浅水W反推无界流速；水对沙的作用体现为入水阻力与终端沉降，沙对水体现为落床排水。**不额外再叠一次湿段重力／浮力**，否则双算终端模型。未来水流携沙另版本，不宣称本版已有。

冻结水流阶段结束、粒子阶段入口的η作本子步介质边界。在既有列边界分段内，用该段积分器提出的有限候选线段与本列水平η求首次交点；有交点时按线段比例分配空气／湿时间，空间位置取交点，速度按该段时长推进，再继续剩余时间。每个单列线段内最多一次介质切换，跨多个列的切换总数受已有事件容量约束，不能把整个子步强行只算一次。此为显式分段近似，不声称交点是连续非线性轨迹的精确根。不能只用起点状态把整个子步都算空气。床／列边界继续使用已有有界分段机制；超事件容量回滚／显式故障，不跳过接触。接触时间与水面时间相等时床优先。水／床在本子步后续改变，其效果进入下一子步介质判定。

横向换列必须把P从旧列移到新列并检容量；满接收列保留原ID／质量且在原列边界等待，不做跨越成功事件。pool满／合并或日志容量不足拒绝准入，不转为U。已有merge若需要保持本单沙来源账，仍按原lineage合同，不能用新生粒子冒旧ID。

### 4.3 LAND、床移动与挤水

成功LAND控制量q按原grant／真实phase／实际库存变化分栏；从同ID pool减量、向同列mh加量，W不变，更新S、η和R。实数中P减q、S加q，R不变，η升q；随后水流对升高水头作响应。partial保留原ID/来源/CONTACT_WAIT，full只释放一次slot。不能在LAND里减掉同量W再丢弃它，那会凭空排水。

同种床松弛仍沿适用的既有坡度／预算合同，同时受接收列容量约束。先得候选完整patch、再验证水沙容量及守恒，不能接受床后才发现负水。bed移入某列会抬η，移出会降η；下一水流阶段再平衡。无需把每个水通量计入固体300/s运动预算；水有独立传输统计与固定工作上限，固体原预算不变。

此模式的严格合法态revision须在发布元数据明确声明。T104/T110仍有未支持writer；**不能把T106当作已启用完整严格route**。新模式若涉及未登记严格核，由architect明确支持域，禁止绕检查／调大H／借新模式悄悄修其它目标。

### 4.4 固定子步次序与原子发布

```text
at boundary: validate and commit queued external source transactions in order
freeze accepted angle and new substep snapshot
shadow: water even/odd pair sweeps
freeze eta for particle medium decisions
shadow: real particle motion / crossing / CONTACT_WAIT / LAND
shadow: applicable bed relaxation under original solid budget + capacity
derive S, eta, P caches; audit all water/sand/source/identity/capacity invariants
accept whole substep or restore whole substep input
assemble one immutable publication; render with one FrameToken
```

子步后床动产生的水头在下一子步被水流处理，故水面响应有明确一子步分裂延迟，不承诺瞬时静水。入口源事务若已成功提交，后续step失败只回滚step，不撤销已提交源量；失败时报告该物理commit及呈现状态。一次源自身失败则源／pool／水／床／缓存／预算／序号／事件全部恢复。不能把两个事务混成重复发放源或吞掉既有成功量。

## 5. 双注入、容量与输入状态机

接口版本 `SAND_WATER_SOURCE_V1`；三类模式分开命名：物理mediumMode、输入sourceKind、渲染sceneRevision。切sourceKind不重初始化物理。

```text
SourceRequest {runId, configRev, geometryEpoch, inputGeneration,
 gestureId, sourceId, requestSeq, kind:SAND|WATER,
 x,y,requestedVolume,timestamp,timestampUnit}
SourceReceipt {same identity, requested, admitted,rejected,
 reason, transactionId?, sourceKindSuccessSeq?,
 waterBefore/After, sandBefore/After, physicalCommitId}
```

所有输入有限／范围有效，requested>0。触点坐标沿既有vp→模型映射并绑定几何代。SAND在真实(x,y)生成phase0粒子，点位必须处于可生成区域且不在床内；WATER用x选列、y只验证有效触摸区域且不在床内，**入口在该列自由面，不是深水点源或空中水滴**。UI提示可简述为“在此处加水”；不能让用户以为水在指尖悬空飞行。水量流入后真实W及η变化；全干列W从0开始，仍同一mediumMode。

水准入 `min(request, C−S−P−W)`；非正余量拒绝。沙准入还受pool可用slot、源包规格与已有外源/高度准入约束，不允许只改床实现掉沙。触列满时不自动把水均摊到全场；先拒绝／部分准入，后续物理水流自行扩散。物理墙封闭，容器满不溢出到不存在的库。

正量成功才推进对应kind的成功序号、成功次数／I_kind及提交身份；拒绝／零量不推进成功身份。请求序号可用于有界去重，但不是成功源号。相同幂等身份＋相同payload重试返回原回执，不重复注入；同身份不同payload为IDENTITY_CONFLICT。去重记录沿活跃run固定容量；记录耗尽显式拒绝，不忘记后重放旧源。重置新run才能归零源账。

输入状态机：

| 事件 | 行为 |
|---|---|
| DOWN | 锁定主指针、kind、run/几何/输入代；clock.begin，首sample=0 |
| MOVE／物理tick采样 | 只用主指针最新位置；一个受控时钟算量，不同时按MOVE数与帧数重复注入 |
| 选择另一kind（按住中也一样） | stop clock、释放主指针、增加输入代；丢弃尚未提交的旧代请求；必须新DOWN，不追赶旧时长 |
| UP / CANCEL | 停止并清积时／未提交本手势请求；已成功物理量保留 |
| 后台／页面退出／surface或geometry失效 | 同停止屏障；恢复首采样0，禁止恢复按住追赶 |
| reset | 停源→清待提交→新run／基准／源账→新publication，旧run请求全部拒绝 |

速率单位分别为`rateSand`、`rateWater`模型体积/s，`requested=rateKind*activeSeconds`。复用GestureRateClock明确ns/ms、首采样0和stop/suspend语义；它现有1s上限不能自动等同新场景准入上限。采样仅在活跃物理tick；若样本跨过暂停或超过该配置的一次物理采样窗口，则suspend并返回0，以新采样重建时基，不补发缺帧时间。窗口沿物理dt合同映射，T107记录具体字段／版本，不能静默把1s的积压一次倾倒。固定受控队列，不为每个MOVE创建无限请求；越界显式拒绝且无源量。

## 6. 明水同帧渲染合同

新增 `CLEAR_WATER_SCENE_V1 / CLEAR_WATER_OPTICS_V1`，scene packet显式携带S、W、η、真实pool与wet标识／模式、source ledger commit、accepted θ、physical/config/geometry revisions，绑定一个FrameToken。不得在旧SCENE_PACKET_V1水雾槽位塞一个新义数字就宣称兼容；T107给出明确版本检测和Native布局。

绘制：背景→同种床与真实粒子→明水透射／水面→前景。床与粒子先建立可见颜色，水层只用本帧S/W/η产生覆盖。水面外空气粒子不染水；跨面的粒子按像素位置分界。水下床仍可见并经水深衰减，不把床裁掉当透明；前景最后正常遮挡。

首版采用标量消光的预乘alpha覆盖，无需同FBO读写反馈：

```text
if W[c] == 0 or y > eta[c]: alpha=0
else path = clamp(eta[c] - max(y,S[c]), 0, W[c])
     T = exp(-sigmaWater*path)
     alpha = 1-T
     water.rgb = alpha * waterTintLinear
blend = ONE, ONE_MINUS_SRC_ALPHA
out = T*underlyingLinear + (1-T)*waterTintLinear
```

水下床像素用路径W衰减；水中粒子用其到面的深度；水面以上alpha=0。只在W>0的真实水面画独立细高光，强度参数待标定。sigma=0时水体内部完全透射但仍可用真实水面辨识；不把透明等同“未画水”，也不以alpha=1的灰绿雾遮沙。关闭该模式的WATER_FOG／INTERFACE_FOG历史密度纹理，禁止白云雾冒清水。sigma、tint、surface参数独立revision，不借用旧fog density最小值。

沿现有新管线线性合成与sRGB编码合同，水颜色不重复套沙材质L或ToneMap；相同水深样本的参考结果由上述式计算。shader映射同publication坐标约定，表面profile采样与粒子湿判定共享S/W/η离散定义；若做视觉平滑须另声明profile revision并保留诊断原始轮廓，不用平滑图反写物理。

整scene成功swap才更新LastGood；任一水层／配置／token／GPU失败保留标明STALE的上一完整新管线scene或FAULT，不单独保留旧水层配新沙。所有背景前景也禁止Canvas上屏／失败回退。

## 7. 三线分阶段与工程交接

不在本卡派开发、不修改T107/T108状态。manager按完成通知登记；T107工程和T108测试先行已有卡可据本方案开展，不新增manager验收门。

| 阶段 | 仿真 | 输入 | 渲染／证据 |
|---|---|---|---|
| A 库存与事务 | 新mode、独立W、单沙不变量、容量和原审计 | kind源／幂等／切换屏障／真实局部加水 | 离线publication合同；未实施项预测试登记NOT_IMPLEMENTED |
| B 耦合闭环 | 水头对流、入水分段、LAND挤水、床移动容量 | 空／湿／满列、pool满、原子回滚 | 同帧S/W/η/P快照；闭合来源与质量证据 |
| C 明水场景 | 固定同路线上述物理版本 | 页面选择与生命周期整合 | 新shader透射/遮挡/水面/LastGood；独立tester与effector取证 |

必要版本：medium、water inventory、hydraulic kernel、particle medium、source API、input mapping、publication、scene/native packet、optics、legalStateRevision。T107落真实接口/字段/布局、模块所有权、错误码、定容上限与测试入口后再转developer。不得修改当前模式的数组语义影响前五目标；优先独立模块／显式模式分派，禁止以legacy initialize作为运行注入兼容层。

复杂度：N列，K活粒子，L既有有界事件数。水每子步2遍N−1边O(N)，派生／审计O(N+K)，粒子按既有有界事件路径O(K+L)；不增O(N²)全列配对或全粒子两两作用。新增W权威＋rollback＋publication三份F64为24N字节（N=128为3072字节），η及通量缓存若各一份F64另16N字节；实际double buffering／Native packing由T107另列，不能把此小计当全程序内存。每帧不新建粒子／源／证书容器，队列与日志定容，耗尽原子拒绝。

GPU新增一个O(像素数)透射pass及水面绘制，复用既有完整scene事务，不自行增加反馈FBO。帧预算／池容量／固体速率沿适用既有合同；本方案不批准更低FPS或更大内存。新水量速率、κ、湿阻力、消光参数需用同路线标定记录，见§8。

## 8. 客观／主观可测条件与待定参数

以下为T108可直接转用的合同；尚无目标六产品报告，不写VALIDATED。既有指标继承原比较符及原线；新效果数值门槛显式PENDING_CALIBRATION，不凭空填已批准分数。

| ID | 场景与可量测断言 |
|---|---|
| CW-01 | 新run全空：ΣW=ΣS=ΣP=0、L/U=0，无水mask／源成功，medium仍为新mode |
| CW-02 | 干列连续加水：只该列准入事务直接改变W；独立I_water与实际总水差满足原审计；不重置initial |
| CW-03 | 湿列／满列／P预留列：按真实余量部分准入或拒绝，rejected可对账，无全场均摊、无负W |
| CW-04 | 掉沙触点出生→空气→入水→LAND：同ID／来源可追、U=0、L=0、H相总量守恒 |
| CW-05 | 同初速度普通单粒子水/空气对照：记录跨面时间、湿时长、速度与位移；与各段解析速度式比对，分段误差门槛沿数值合同／新增部分待标定 |
| CW-06 | LAND q后W不变、P减／S增、η抬升；后续水从高head流出，ΣW与Σ沙满足原线；partial/full身份正确 |
| CW-07 | 无沙纯水、无源定角：每对通量记录q及5项上界，成对水量审计、非负、容量、势能不增；平头零流 |
| CW-08 | 干高床隔水／左右墙：无跨床瞬移、无墙外水损；量化相邻流动与连通水域最终head残差 |
| CW-09 | θ=0和适用正负倾角：水面head趋同，粒子g方向一致；相同accepted θ进入物理与shader，不用raw传感器角画水 |
| CW-10 | 床搬运／粒子换列至满接收列：容量拒绝仍守恒，无P丢失／隐形U／伪成功事件 |
| CW-11 | 注入失败：pool满、日志/队列满、非法位置／NaN、旧run、旧geometry、幂等冲突，库存／身份／成功账完整不变 |
| CW-12 | 同幂等请求重试只计一次；正成功kind序号独立，零量／拒绝不推进；water与sand不串账 |
| CW-13 | 按住sand→water及反向切换，MOVE风暴、次指针、UP/CANCEL：旧代停止，新DOWN首采样0，无时间追赶／双时钟重复注入 |
| CW-14 | 后台／reset／几何代／surface失效：待提交源清理，已提交源不消失，新run拒旧请求，恢复无旧时长补发 |
| CW-15 | step审计失败与source事务失败分别注错：rollback边界正确，不吞前一已提交源，不残留半帧／半事件 |
| CW-16 | 固定W／S／粒子样本：水上原色、水中按路径T混合、水下床可见、W=0无水覆盖；同FrameToken全层一致 |
| CW-17 | shader／token／配置／swap失败：FAULT或标明STALE完整LastGood，禁止Canvas／混代水沙／伪当前成功 |
| CW-18 | 固定N/K/事件上限长时运行：队列、ID／源账、rollback、帧内分配与复杂度实测；沿既有性能合同，不用平均FPS掩盖峰值 |

新参数待定表：κ及水面松弛时间／残差；rateWater/rateSand与一次源量上限；dragWet/settlingSpeed与入水减速观感；sigmaWater/tint/高光及清晰度；分段积分新增误差（若原合同未覆盖）。它们必须以配置revision＋候选值＋同路线测量结果记录。未标定效果不能写达标，测试可先报告测量值；manager不因此增加审批门。现有EPS/H/质量容差/固体预算不得作为调这些参数的旋钮。

effector同路线上记录：①能辨水面且透过水辨床/粒子；②掉沙在水中明确减速并积床；③掉水后真实水面抬升、沿低头方向扩散；④轻倾斜后的水面与沙运动方向一致；⑤快速切换不继续掉旧物；⑥全干／浅水／深水／接近容量／两水域隔床的观感。使用固定背景、视角、场景与记录的参数revision，附物理源量／水深作为同帧证据。首轮建立定量观察表，不自造既有评分基线或跨路线全量dominance。

## 9. 备选方案、风险与反例

| 候选 | 取舍 |
|---|---|
| A 旧wet参数开关＋雾改透明 | REJECTED：水位/粒子占据语义不清、无局部双源事务；材质改动不能闭合库存 |
| B 本版显式W＋过阻尼邻接流＋稀疏点沙＋床排水 | RECOMMENDED：线性成本、真实源和守恒闭环、可沿新管线逐阶段实施；近似范围明确 |
| C 同时解浸没粒子占据＋自由面＋密集双向耦合 | DEFERRED：更适合密集落沙即时排水，须处理浸没几何非唯一／容量／运动反力与额外迭代，超最小范围 |
| D SPH/FLIP／水滴全粒子 | DEFERRED：能进一步做浪／滴／溅，但端侧成本与新守恒路径大，用户未要求本轮扩展 |

反例必须保留：空气沙P使旧waterTop虚高；0水dry包络画出假水；legacy inject使成功账归零；砂落床同时减W造成漏水；水注入成功但只改shader贴图；全局均摊让水穿过高床；大q水流左右振荡；q小于库存ULP导致账与实际差分分离；零LAND仍改CONTACT_WAIT；按住切换累积后一次补发；旧雾alpha=1遮床；scene水面来自新state而床来自LastGood。各反例对应上述可测合同，不把已知未实现预测试RED派成未知产品缺陷。

下一步按已有T107工程、T108测试先行推进。设计交付完成不等于目标六实现／效果达标，后续双测完成后designer按同路线证据验证；失败先由architect归因。仅自身任务行状态有限例外，不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容或滴答同名清单。
