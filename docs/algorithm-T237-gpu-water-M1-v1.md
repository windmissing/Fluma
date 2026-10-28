# T237：GPU驻留沙水 M1 算法定版 v1

2026-10-28，designer；原卡`6ae1ec3be4b0d25fef7ba6a7`。目标6/P5，新增仿真效果GPU优先。路线 **`gpu-resident-water-M1-v1`**；状态 **DESIGN_READY_FOR_ARCHITECT / NOT_IMPLEMENTED / NOT_VALIDATED**。消费T230/T233、T231、T221/T222、T209/T215、T226及当前CPU参考源码，输入指纹与算术核见`design-T237-gpu-water-M1-evidence-v1.json`，独立复盘另文。

不使用真机，不改业务/在途M0、不先实现CPU-only效果，不回退共享修改。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改其它行列/队列或滴答同名清单。所有背景/前景及沙水仅新XComponent+shader；失败FAULT或显式STALE完整有效旧新管线帧，禁止Canvas上屏/回退。manager登记交付和工程接力，不加验收门。

## 1. 本轮定稿与边界

定稿为同一GPU owner内**J在途水→真实运动/接触→ARRIVE转W→水头邻流**，以及沙的AIR/WET分段、沉降/落床、床面抬水位。全阶段在GPU，CPU只输入/时钟与生命周期调度/有界completion，CPU旧clearwater只作兼容和行为参考，独立整数模型才是新policy逐word oracle。没有逐水包读回、CPU查找/排序/exp后传回或预制水柱。

保留M0 source/step边界、3state banks、spare replay、双frame事件/双publication、尾fence后native接受的框架；**M0 reader、布局、shader及证据不被本稿覆写**。M1新空run开启feature bitmap **0b0011**（bit0 M0基础、bit1 WATER_J）；M2/M3位仍0，不包含merge、双沙、U、渗透、扬沙、压力/惯性波/SPH/飞溅、水平水流携沙。状态只能单H、bh/mh、P、W/J，L/U全0且审核。

版本：`GRS_Q20_WATER_M1_V1`、`GRS_FIXED120_TIME_V1`、`GRS_M1_SEGMENT_T16_V1`、`GRS_M1_SOURCE32_V2`、`GRS_M1_EVENT64_V2`、`GRS_M1_STATE_LAYOUT_V2`、`GRS_M1_COMPLETION256_V2`、`GRS_GPU_SCENE_Q20_M1_V2`。质量/位置网格仍Q=2^20、h=1/120、N128/C256、Hq605394和原固体预算。**运动分段/水头/阻力与M0及旧F64存在真实差异**，不声称dry trace也逐位继承M0，更不以float替换T221公式冒等价。

T226已交C基础及编译/离线自测，仍CPU F64源直接写W，没有真实J/M1；它的透射/面积高光可作shader参考，不能拿CPU snapshot→native上传接成“GPU驻留”。T227未开工CPU生产效果按用户最新调度等本方案工程分流；本稿不改该卡状态，不回退任何已做参考工作。

## 2. 权威库存、容量与source

### 2.1 精确Q20账

每列U32整数：`S=bh+mh`, `P=Σ活sand.mass`, `J=Σ活water.mass`, `eta=S+W`, `R=S+W+P+J<=Cq=256Q`。P/J是实际质量的精确列归约，非浮点incremental cache；所有加法在checked宽域核不溢出后存U32。全局`Sand=ΣS+Σsand.mass`，`Water=ΣW+Σwater.mass`用U64；分别严格等`initial+I_sand/I_water`。闭箱不流失、无重设初始基准。单列质量单位正的一量子不能被当0丢掉；内部转移绝对漂移0，满足而不是放宽原1e-6及<.001门。

J只保留**当前列**容量，不抬eta/水头、不当光学深度。P是点沙，不在飞行时排水，但预留落床空间。LAND中P−q/S＋q，W不变而eta抬q；ARRIVE中J−m/W＋m，R逐位不变而eta抬m。容量、床guard、300/s total和28/s local是不同账：水流/水包运动/ARRIVE不扣固体budget；沙CROSS/LAND/床流仍扣T230预算，不混用水量。

输入原F64 bits由GPU解码、质量floor(requested*Q)、坐标向网格floor；正量化为0拒绝NUMERIC_RESOLUTION_REJECTED，不增I/成功序号。原requested、量化requestUnits、admitted、容量/slot/分辨率拒量分别回报。源量域(0,256]、坐标/版本/时钟域沿T230；kappa/drag等冷配置不自动从旧F64静默截断。旧run/snapshot拒导入，M0→M1也新空run，不将已有已准入浮点小质量丢掉。

### 2.2 双选源事务

命令仍32word、receipt/dedup64word的物理长度沿T233，但**revision V2**、kind `SAND_H=0/WATER=1`。exact identity/payload、FIFO16、D4096不驱逐、append-only有效prefix/原始bits比较、尾部native接受后成功规则不变。终态32word的firstId/birthCount/nextId按kind解释，deliveryMode `SAND=0/DIRECT_W=1/FLIGHT_J=2`；waterBefore/After明确W+J，不是ΣW。

```text
SOURCE root，G<=1/active tick:
  GPU copy current→candidate，查身份/去重和资源，核原state
  c=floor(xQ/Q)，free=Cq-S-W-P-J
  SAND_H: q<=free/maxSource/freeSand*package；沿M0按最低free slot、ID前缀出生
  WATER:
    网格y<S -> POSITION_IN_BED拒绝
    S<=网格y<=eta -> DIRECT_W：W+=q，无水包
    网格y>eta -> FLIGHT_J：还核waterFree>0，q整包出生一个water slot，J[c]+=q
      x/y=网格触点，v=(0,-vEmitQ)，origin=本源sourceCommit，birthCommit=本源physicsCommit
  q>0才I_kind+=q/successSeq++/sourceCommit++/physicsCommit++，派生/全审核/重放
  仅candidate完整封存＋尾fence/native接受才通知成功
```

水每请求最多一个包，独立Kw冷profile **64或128（连续DEMO推荐128）**、独立waterId/slotGeneration，不占沙slot，不用沙massPackage拆水。q受到同一当前列保留空间限制，不能先加W再扣成J。触点原始高度只高于eta不足一网格、floor后等于eta时明确走DIRECT_W，不画虚构落程。源成功是**集中出生于当前模拟边界**，不把raw timestamp当已经飞行的年龄；新J后一个或多个真实quantum才到达，近界面在同quantum到达也正常。

Kw不是只看总水量：约100Hz每次一包、首批落程接近0.9模拟秒时，Kw64会在首次到达前满池，真实拒绝应保留分母，不能用假连续水柱掩盖。候选g512/vEmit16/零角/源高220/空床的落程与120Hz包数由模型单列，Kw128给这组条件留余量，**不是对任意offered频率/失败延迟/高度/阻力的无拒绝保证**。Kw64仍可比较资源/较低频负载，切profile必须新run并明确UI能力，不能热扩容、静默降采样或把不同source请求合并；高负载按FIFO/slot真实背压。

waterId正U32单调，MAX_ID可分配；分配它后置新header的`waterIdExhausted=1`且nextWaterId保持MAX_ID，不wrap0，后续新水包拒ID_EXHAUSTED。sandId在M1同样有显式exhausted位，M0既有schema不改；终态V2 flags表示耗尽，不偷填M0保留位。slotGeneration将溢出则该source整root失败；不复用旧ID、旧run/slot不能接迟到结果。full ARRIVE退slot：mass/state/id/来源/位置速度/birth字段清0，generation保留，下一出生checked增1；事件保存退役前全部身份。

M1状态枚举独立冻结：两pool均FREE0/AIR1/CONTACT_WAIT2/SIDE_WAIT3，namespace区分沙床接触和水面接触；waitReason NONE0/BED1/SIDE_CAPACITY2/SIDE_HIGH_BED3/SIDE_BUDGET4/WALL5/SURFACE6。沙的AIR/WET另存medium bit，不当第二沙种/第二质量。释放改AIR/NONE，不能仅清文本原因而保留锁死状态。

## 3. 完整quantum顺序与阶段依赖

```text
GPU QUANTUM candidate，h=1/120，theta/config/root token冻结:
  copy/base entry audit → 可行角/Hq/固体budget初始化
  water head edges：两色各 PROPOSE→PREFIX_EVENT→APPLY，一邻边一次
  freeze etaMedium/wetMask/bed2/head2（仅真实W，不含J）
  GPU sandId排序；waterId排序
  water逐ID空气运动/列容量/墙/沙或积水接触，暂不加W
  sand逐ID AIR/WET分段→CROSS/CONTACT/LAND，读取此前已更新S/P/J
  床三色proposal/有序total grant/并行apply（原M0严格guard，容量含W/J）
  sand末尾contact/fold：床后埋点投影/失去支撑释放，无二次LAND
  water按(column,y,id) GPU排序，低到高单次CONTACT/ARRIVE收尾
  全列P/J/eta/R归约、所有硬门 → 统一event64 replay到spare/逐word比较
  GPU pack+Q20/F32域校验 → completion → 尾fence/native选择完整bank
```

新增到达水在末尾进W，**下一quantum**才参与水头扩散；LAND/床抬水面也下一quantum响应，不能用渲染均摊假装同quantum水平。etaMedium整quantum冻结，当前S仍逐ID/床动态变化；床接触优先于旧介质面。没有逐事件全池audit/归约/CPU通知，没有内部λ重试。

任一quantum致命失败丢整个candidate：W/J/两pool、位置/速度/预算/来源/角/事件有效cursor全部回前态；之前已native接受的source和前几quantum保留。dt0无写，正常F≤4/G≤1及carry/暂停/失败duration合同沿T230；这不是T221 F64可变dt两子步。stop/UP/CANCEL/切kind/后台清未提交旧代请求、pointer与输入时钟，已经成功的J/P/W保留；active时继续运动，后台暂停不按墙钟追赶，reset明确新空run。切源必须新DOWN，ACK/幂等重放不重复反馈成功。

重力接受沿T230/T233 GPU可行角归约/查表，不把T226 CPU `GravityPort` 的aTrial/acceptedAngle算法搬入M1热控制面。host只采/绑定sensor目标与freshness作为输入，GPU产生最终jAccepted；本quantum水头、水包、湿沙、床/guard读同一已冻结j，成功才接受角revision。失效/stale停新源并暂停本tick物理、不伪置0°，恢复首tick0；shake仅诊断。传感器坐标/观测过滤属于输入契约，任何物理角平滑/速率限制若新增也须GPU新policy，不静默回CPU planner。

## 4. 水头通量：半单位精确head2、两色无冲突

接受u=tanQ20，固定x=c+.5。避免半网格舍入，直接存signed `head2[c]=2*(S+W)-(2c+1)*u`, `bed2[c]=2*S-(2c+1)*u`，支持域绝对值<2^31，差值用signed宽域。每边高head为d、低为r，delta2>0，kappaW为显式cold整数[0,960] s^-1：

```text
qReq = floor(kappaW*delta2 / 240)        // kappa*h*Δhead
q = min(qReq, floor(delta2/4), W[d],
        max(0,floor((head2[d]-max(bed2[d],bed2[r]))/2)),
        Cq-S[r]-W[r]-P[r]-J[r])
if q==0: 无材料写；否则 W[d]-=q; W[r]+=q
```

min前核所有中间量；不把负库存/非法free裁0继续。库存/保留已合法才允许余头请求max0。内部两端q相同，独立核ΣW不变、R<=Cq；水不动I_water或固体budget。`floor(delta2/4)`保证两端头差不反转；相等head没有方向，不随机择边。网格可形成<2质量量子的残余头差，记录QUANTIZED_EQUILIBRIUM而不把W均摊/改基准。

两色顺序按成功quantumSerial奇偶轮换；同色边两端互不相交，提案读当前候选，prefix按edge升序唯一分配event ordinal，apply仅写自己两列。三pass之间barrier，下一色读真实更新W。两端实际before值必须等提案，否则整个root失败。没有float atomic、CAS抢收水或total水budget。workspace边提案与后续床提案时分复用，但frozen medium独立保存。

水平平床中央水、kappa>0、两侧可接收、量足够可表示时，有限步左右累计out都应>0，**不要求单步镜像**。倾斜/墙/沙脊/J预留不同，允许单向流；凹槽汇聚/过脊条件由真实S与head决定。不承诺扩散速度/可见面积/100fps已达标。

## 5. 有界整数时间分段：真实AIR/WET变化

### 5.1 定点积分核

每quantum再用**T=65536个整数时间tick**描述事件时间，D=120*T=7,864,320，时间r tick对应r/D秒；T不是65536次dispatch/固定微迭代。常数gQ、vEmitQ、settlingSpeedQ均Q20；速度上限1024Q，`120<=gQ<=1024Q`、`0<=vEmitQ<=1024Q`、`0<settlingSpeedQ<=1024Q`。gQ下限保证支持角域中从静止完整quantum的重力速度增量至少一网格，避免g量化成无重力；它不是已标定观感下限。kAir/kFlight为cold整数[0,960]、kWet为[1,960] s^-1；不静默把旧.01、exp常数取整，旧配置不能自动兼容。候选g=512、vEmit=16、settlingSpeed=8、kAir=kFlight=0、kWet=24仅PENDING_CALIBRATION。

定义RNE为精确有符号宽乘/除后最近偶数（T230 primitive），a=(RNE(gQ*sin/Q),−RNE(gQ*cos/Q))，湿终速vT=(RNE(vSettleQ*sin/Q),−RNE(vSettleQ*cos/Q))：

```text
AIR for remaining r:
  v1 = RNE(D*v0/(D+kAir*r)) + RNE(a*r/D)
WET:
  v1 = RNE((D*v0+kWet*r*vT)/(D+kWet*r))
水包只用AIR，kFlight替kAir；WET没有再叠加一次重力
p1 = p0 + RNE((v0+v1)*r/(2*D))   // signed宽域梯形位置，非旧半隐式图
```

分量独立执行；任何v1越Vmax、宽算术越域FAIL，不尾端clamp。D*v最大约2^53，k*r*vT<2^56，带符号sum<2^63；v0+v1也用宽域，避免两端都Vmax时signed32相加溢出。DDA和ceil时间用U64，需128bit的扩展操作仍用软件limb，不依赖硬件double/int64 atomic。**隐式阻力、梯形位置、Q20和时间格子是新积分器**，不是旧指数阻力的float改写。

### 5.2 事件推进

GPU各pool resolver按ID独占处理slot，r初始T；并行proposal只初始化/验证入口与系数，实际受冲突的轨迹/材料串行在GPU解析。**先切速度零点**：若某分量v0非0、按r预测已达0或换号，以整数二分≤16轮找该分量首次达到0/换号的正tick，取两分量最早者为rSeg；否则rSeg=r。在[1,rSeg]内无未处理的速度反向，用上述梯形位置生成候选直线弦；用整数有理数找最早λ∈[0,1]，同刻优先**当前/目标床→容器墙→列边→水面→速度TURN**。无更早几何面且rSeg<r时记TURN（同tick两个零点合并mask），更新实际速度/位置后继续余时；v0=0不重复生成零点。床读当前S；水面只在frozenWet=1时存在。比值比较宽交叉乘，无float λ/几何EPS。

正λ的实际事件时间 `t=ceil(rSeg*lambdaNum/lambdaDen)`，t∈[1,rSeg]；位置取原候选弦的有理交点RNE并把命中坐标设为准确整数平面，速度以t重算；r-=t。无几何碰撞时t=rSeg、位置/速度为该段p1/v1，若rSeg=r则完成，否则TURN后续。这个零点切分避免“整段末速度已反向，但碰撞重算的早期速度还没反向”造成弦方向反复翻转、使事件上界证明失效。它是明示的弦几何＋向上时间格化近似，每正事件时间偏差<1/(120T)秒；原始有理λ/segmentTicks/前后tick均入事件，不能拿墙钟或UI动画指定到达时刻。

lambda0只允许一次真实拓扑变化（进入新列/介质或接触等待），保留planeTag；同plane重复且无正tick/拓扑进展则TIME_NO_PROGRESS整root失败。不塞epsilon时间。MEDIUM方向：AIR且向下碰eta进WET，WET且向上离eta转AIR；离水向上设y=eta+1质量网格，入水y=eta，防同一格双重归属，明确1网格空间近似。纯列CROSS按新列frozen surface重定medium，写在同一CROSS flags，不额外偷一条未记事件。W=0不生成水面；当前床已高于冻结eta时床优先。

沙的统一介质分类为`frozenWet && y>currentS && (y<etaMedium || y==etaMedium && vy<=0)`。每个已接受运动事件（含TURN/RELEASE）必须把该规则的旧/新medium写在flags/56+附槽；例如停在水面上的TURN从上升变下降可在**同一TURN行**改成WET，不能下轮循环无日志偷偷重分类。water始终AIR直到退役。水CROSS进入相邻较高水体（非床）时允许整包预留成功后在新列立即CONTACT_WAIT，末尾ARRIVE；若目标为高床则旧列侧等待，不瞬移到高床顶。

左右跨/阻挡坐标沿M0整数网格规则；水跨列整包移J，空间不够SIDE_WAIT；沙跨列整包移P且扣T一次/L两端一次。墙/顶、高床、预算/容量阻挡把法向速度置0并记录**CONSTRAINT_WAIT消费全部余tick**，表示这段时间在约束上停留，不是无声丢余时。床接触同样记录余时由支撑约束消费，沙再请求LAND；纯材料q0与CONTACT位置/速度写分开。早ID LAND已埋住晚ID入口点时λ0 CONTACT到S、再LAND，不能跳过负λ交点。下一个quantum重新评估WAIT，不永久锁死；失去床支撑保留原y释放AIR。

CONTACT_WAIT行τAfter=T，而运动到碰撞仅`tMotion=ceil(rSeg*λ)`，`waitTicks=T−τBefore−tMotion`由该行rSeg/λ明确恢复；速度先按tMotion重算再去法向，余时点和处理后速度冻结在约束，不重复加力/拖拽。LAND材料行另记τ=T→T。不能将τAfter−τBefore全当自由飞行去验速度或隐去等待时间。

### 5.3 事件上界与模型限制

固定接受角±30°域，AIR加速度和WET终速的x方向始终与sin同号，y始终向下/非正；正隐式权重与RNE不把已同向速度变反向，且先切TURN避免弦跨过未处理的速度反向，所以每坐标在一quantum内最多一次方向反转。路径长度每轴≤Vmax/120，加有限网格投影误差；保守列平面次数Bcross≤12。每列访问有常数frozen eta，y最多一次反转，故访问内水面穿越≤2；CROSS本身的medium变化并在该行。沙最多12 CROSS+26 MEDIUM+2 TURN+末MOVE+接触/LAND/WAIT/末CONTACT等≤**48行/quantum，含末收尾**；水包出生vy非正且无向上力/反弹，至多一个x TURN，≤12 CROSS+1 TURN+接触/末MOVE/末ARRIVE或释放，≤**16行，含末收尾**。

证明适用条件包括：冻结角/eta、无水平水流速度/随机shake力、无弹性反弹/点间碰撞，网格投影只顺运动方向，碰床/墙/阻挡即约束停留。新增力/反弹/移动水面会破坏此界，必须新版本重证。运行仍逐写前预留，超过48/16、重复zero-plane或证明假设不满足就EVENT_LIMIT/UNSUPPORTED_MOTION_POLICY整root失败；不删行、不继续无日志跑完，不拿平均事件数做保证。记录实际CROSS/MEDIUM/remaining ticks，独立tester须尝试极限/反向/等面反例；本设计模型不是GPU界已实证。

## 6. 水到达、沙沉降和相互占据响应

water空气轨迹碰到frozen eta/床后先CONTACT_WAIT，不提前写W；沙轨迹随后按frozen medium阻力推进到当前床，LAND整数q<=mass/budget/free/guard，P−q/mh＋q，沙ID来源不变、full一次退役。床三色沿M0严格Hq/有序budget，但receiver free必须含W和J；W不随床流丢失/搬运，eta随S实际改变。

沙末CONTACT/fold后，水索引按`(valid,column,y,id,slot)`在GPU稳定bitonic排序；single invocation按column/y/id升序访问**当前**eta=S+W：

```text
if alive.y <= current eta[c]:
    reserve ARRIVE event; assert m>0, J[c]>=m
    W[c]+=m; J[c]-=m; retire exactly this water slot
    arrivalSerial++; physical root成功才对外生效
else if 曾CONTACT_WAIT而当前面下降:
    release AIR，保留原位置，不瞬移到新低面
else 保持AIR/SIDE_WAIT实际位置
```

因为整数同列转换使R保持完全相同，合法入口和正确writer下ARRIVE不再有T221的RN无写/补尾问题，正m必使W变化；如果容量/派生/来源不符，**作为内部不变量错误整root失败**，不能把损坏隐藏成永远WAIT。低到高单扫让低包到达抬面后可接到更高包，结束后每个活包y>当前eta；不能先按ID跳过高包、后低包抬面把它吞进水里仍叫已对齐。source/quantum入口与末审明确核所有活water的vy<=0，已封存quantum末审另核y>eta，防不受证向上水包状态绕过16行上界。来源/ID/event保留真实序号，ARRIVE不增sourceCommit/I_water/成功源序号；physicsCommit按本quantum整体一次。

最后P/J按真实存量精确归约/比较缓存，水沙分别硬核；中途不依赖每事件全pool重建。湿沙只受阻力/终速和床接触，不被水头通量直接平流；点沙飞行不抬eta，落床才排水。砂水相互响应是准静态占据耦合，非孔隙渗透/压力/惯性浪。

## 7. M0扩展布局、来源和统一回放

不覆写T233 manifest，后续architect新增M1 manifest/reader/program，用不同layout/algorithm/numeric/scene rev。长度变化：

| 域 | M1定稿 | 与M0关系 |
| --- | --- | --- |
| sand16 state | 沿16word；aux0=最后frozen medium、aux1=最后motion quantum低word，均V2定义 | medium是冻结面诊断，不要求和ARRIVE后新eta一致；渲染不能据它遮水 |
| **water16 state** | M0 water12的12槽不改次序，再加`sourceId,birthQuantumLo,birthQuantumHi,flags` | 三bank各增加16Kw字节；birthCommit是源成功物理commit，birthQuantum为当时已成功quantum数 |
| column16 | 原库存/派生槽，W/J/P真实启用，L/U0，eta=S+W/envelope=R | 整数sumchecked，R包括P/J但水面不读R |
| header256 V2 | 原0–111布局继承；112–113 settledTotal，114–115 flightTotal，116–117 arrivalSerial，118–119 quantumSerial，120–121 frozenMediumQuantum，122 motionRev，123 Tticks，124 waterRev，125 opticsRev，126/127 sand/waterIdExhausted，128..131 water/medium/arrival/waterEdge实际行数，其余保留0 | 不增字节，M0 reader拒版本，原waterMass字段在M1是W+J |
| completion256 V2 | 原0–137继承；138..145为settled前后/flight前后各U64，146..149 arrivalSerial前后，150..153 quantumSerial/frozenEpoch，154..159 waterRows/mediumRows/arrivals/waterEdges/deliveryMode/acceptedWaterId，160..余保留0 | 仍1024B/root、最多5root/tick；不是扩大正常读回 |
| publication sand16 | 延用原始单沙proof+投影规则，新revision绑定 | 无需借CPU pool |
| **publication water20** | water16原始字段＋`xF32Bits,yF32Bits,packedVisualTag,reserved0` | 每slot80B，正ID顺序有效prefix，退役水不再显示 |
| **publication column16** | `[S,W,eta,J,P,bh,mh,R,Sf32,etaF32,Wf32,Rf32,wet,columnIndex,slopeF32,reserved0]` | 每列64B，wet仅W>0；slope给高光，不改物理 |

command32/dedup64/source event16/terminal32长度沿用，但V2 reader明确kind命名空间、原waterTotal=W+J、deliveryMode和耗尽flags；不把旧F64 event24/40/48或tape混进Q20。source slab下一无效row写入、accepted prefix随native选bank生效，迟到/失败不改此前终态；query精确只读终态，不二次enqueue制造source。

**统一GRS_M1_EVENT64_V2**复用T233 event64的0–55字段，56–63改为`[tauBefore,tauAfter,mediumBefore,mediumAfter,etaFrom,etaTo,planeTag,eventAux]`，按kind used-mask无关槽0；motion的eventAux=segmentTicks（即rSeg），FIELD动作0。不同reader必须拒绝旧rev。已有slot/id/source/origin、old/new mass/xy/v、stockBefore/AfterA/B、预算、λ U64字段保留。namespace在flags bit18..19：SAND0/WATER1/FIELD2/SOURCE3；bits0..3旧state/4..7新state、8/9旧新medium、10..13旧wait/14..17新wait，TURN时20..21为X/Y mask，其它kind为0，22..31保留0。来源是成功sourceCommit，非sourceId；slotGeneration/namespace一起验证。

kind冻结建议：SOURCE_BIRTH1/DIRECT_W2/SAND_MOVE3/WATER_MOVE4/CROSS5/MEDIUM6/CONTACT_WAIT7/LAND8/BED9/WATER_EDGE10/ARRIVE11/RELEASE12/FINAL_CONTACT13/TURN14。field动作无slot/id置0并明示namespace，不能撞真实ID0。stock A/B按kind：WATER_EDGE为donorW/receiverW，water CROSS为fromJ/toJ，ARRIVE为W/J，LAND为mh/P（pool mass另有before/after）；BED为donor mh/receiver mh。M1只允许新空run且不固结，**bh必须全0并由入口audit明确核**，故不遗漏分取bh的日志；非0基岩/分层初态为UNSUPPORTED_INITIAL_STATE，需另版完整stock事件，不用总S掩盖层内转移。

一个root一个global ordinal流，所有water/sand/field按§3阶段追加；并行边先prefix分配唯一行，resolver单writer追加，**没有第二份CPU B40/water48/tape或双账**。正/零/WAIT时间有明确τ，final几何事件τ=T→T。WAIT消费余时为τBefore→T；MEDIUM零时必须改介质/plane，不能用相同state自转。GPU replay从base重放同一流：核old words/来源/namespace/τ/λ、局部capacity/guard/budget/整数积分/最早平面，再应用actual patch；不重新选pair/重排事件或逐行重扫pool。frozen medium从回放到water-head末阶段的state重新派生，不能引用后来已被改动的candidate面。末核两bank全部authority/派生/计数逐word相同。产品replay仍不是独立oracle，tester另持Fraction/整数实现。

## 8. 提交、同token明水渲染和交互

继承T230/T233：current只读，所有physics/审核/重放/发布pack在candidate；尾fence完成＋native身份/代/首错/GL/取消tombstone全核后一次选`bank+revision+receipt`。每root失败只丢本candidate，late不能复活，context loss为OWNER_STATE_LOST，M1无从1024B摘要恢复完整pool。旧CPU owner仍存在不等GPU可恢复，不能把旧clearwater snapshot导入新整数run续账。

工程公共接口延用resident opaque handle而非ClearWaterEngine：冷`configureM1(profile,angleTable,optics)->owner`必须原子建全资源/新run；`submitSource(command32)->PENDING`、`scheduleTick(rawWindow,sensorToken)`只排控制命令；`copyCompletion(rootToken,out256word)`只取有界已封回执；`leasePublication(expectedToken)->GpuM1Lease`不返回pool数组；大event/整state只经显式`freezeTrace/capture`冷入口。所有入口校run/owner/epoch/revision/shape，M0 reader拒M1，失败首错不由清理覆盖；不在这些薄接口里调用CPU水核或逐包查询。

首选同XComponent GL owner/context，M1 pub仍双slot+release lease，满则背压/旧完整STALE。**scene是GRS_GPU_SCENE_Q20_M1_V2**：GPU审核Q20 authority→软件RNE打包F32/原列投影→pack后有限/顺序核，native只核opaque lease/token，不每帧copyCurrentSceneInto CPU池。背景/床/活动沙/真实水体/水面/在途水/前景读同一个accepted publication。新ARRIVE同token中退水glyph并增W，不能两处画同一已转移物质；source成功或physics成功不等CURRENT，只有真实完整swap。旧V3/V4 F64 parser不冒通过。

优先fragment只读一个publication SSBO（四binding compute不限制fragment消费者；新context须核fragment blocks≥1），床/粒子可VBO/vertex读GPU pack。若平台不支持此profile fetch，可另版GPU image/texture pack并核compute image能力/额外bytes/dispatch；**不自动全pool下载CPU再上传**。物理可headless先开发，但没有同帧显示证据不报目标7/8产品PASS。

水体/高光参考T221§7、T222/T226已定视觉数学：W0透明；depth=`clamp(eta-max(y,S),0,W)`，T=exp(−sigma*depth)，线性premult body=`((1−T)*tint,1−T)`；surface实际像素带受[床,eta]裁剪，解析相邻两列面积、Fresnel/高光在同fragment求和，无跨干槽桥、无WATER_FOG/Canvas。shader中的exp/pow仅光学，不回写物理。optics16参数/单位/合法域沿该合同，M1冷存16F32＋host镜像，原F64请求量化到明确float参数位型并绑opticsRev；浅水F32/像素不可见不能放大W骗过效果。

水包glyph采用T221圆片：rPx由真实mass的显式质量比例/冷半径配置求得，coverage与局部高光；位置仅pub.x/y，禁止喷口到水面的固定假水柱/历史拖尾，glyph半径不当碰撞/体积。水中沙的遮挡由像素与当前S/eta判断，不拿最后冻结medium诊断作当前水界。old/source/physics/publication/rendered token分账，geometry/context/size/optics不兼容的LastGood不复用。

## 9. Dispatch、完整工作量与资源

### 9.1 固定stage图

单quantum保留M0的23个非排序stage（proposal/resolver改M1版本），加：水边两色6、freeze medium1、waterId索引init1、water proposal1/resolver1、arrival索引init1/resolver1、water最终fold1，共**13**；另两套Kw排序。`L(K)=hK(hK+1)/2, Kw64→Lw21、Kw128→Lw28`，所以：

`D_M1_quantum=36+L(K)+2Lw`，Kw64档K192 **114**、K512 **123**；推荐Kw128档分别 **128/137**。

源仍用Kpad上的统一free scan（water slot≥Kw标无效），M0同16+2h包络，即32/34；G1+F4 active tick，Kw64档 **488/526 dispatch**，推荐Kw128档 **544/582 dispatch**，均≤5尾fence、正常completion≤5120B。事件分段while全部在resident resolver invocation中，不每plane dispatch/fence；这些是packed SoA+WORK profile下的**设计stage数**，不是实际已执行值。architect若因四binding/真实范围增加拆pass，必须更新完整dispatch manifest/上界，不将增量藏在同名stage里。

4bindings按phase用base/candidate/WORK/event或result，cold coefficients/角表/queue/indices/proposals嵌WORK明确offset；source查dedup与event flush分pass重绑。propose先全读，apply用plan内old/new值且每列唯一写者，不能把同buffer重绑为两个restrict别名后假定无冲突。不同颜色、freeze/轨迹/排序/replay/pack之间按实际consumer barrier，workgroup barrier不能代跨dispatch/全局同步。驱动cold allocation/first compile/CPU提交/每root fence/map/失败no-op stages和完整shader场景都计。

工作界（N=128、eS48/eW16、F≤4）：`O(BankWords+Klog²K+2Kwlog²Kw+(eS*K+eW*Kw)*(1+2*bTurn)*Wwide+N+E_replay*(1+2*bTurn)*Wwide)`/quantum，加source去重D扫描/free scan/birth/copy/重放。其中bTurn<=16是各坐标零点二分次数，Wwide是软件宽乘/除的实际word-loop成本（按实现最多64/128bit迭代另计），不能把新积分/碰撞除法当免费一条硬件float。无反向条件时可只作endpoint判别，不跑16轮，但性能必须保存真实次数。ID序resolver串行深度不可除以64当延迟，water-head各色并行只消去无冲突边。所有offered/accepted/量化拒量/slot拒量/失败duration/实际T ticks同分母；对照分旧CPU F64行为参考、独立CPU整数policy oracle、GPU M1相同输入三列。没有device GPU timer/实际frame/热竞争证据时不编收益/帧率。

### 9.2 全量新模板与差额

先列与T233 Kw64保留容量直接可比的**Kw64档**：N128/Q16/D4096，eS48**含末CONTACT**，eW16**含ARRIVE/末释放**，水边/床边各最多N−1成功行；source至多max(K,1)行。因此

`EframeM1=4*(48K+16Kw+2(N−1))+K=193K+5112`。

K192 **42168行**，K512 **103928行**，一event64 256B。双frame arena不能继续沿M0容量。对T233基础模板增量：

| 增量 | bytes |
| --- | --- |
| 三bank water12→16 | `3*16Kw=3072` |
| 双pub water16→20、column8→16 | `2*(16Kw+32N)=10240` |
| water motion32 proposal | `128Kw=8192` |
| water ID/arrival两套双索引 | `16Kw=1024` |
| 独立freeze medium4与额外audit4 | `16N+16N=4096` |
| cold M1 physics config128word、双optics16F32 | `512+128=640` |
| 双event扩容 | `512*(EframeM1−EframeM0)` |

新增模板总计K192 **8,282,752B**、K512 **18,113,152B**；叠T233基础后 **23,342,736B / 55,111,056B**。最大单event block **10,795,008B / 26,605,568B**。M0先前Kw64预留不表示已足够M1，也不等实际已经分配了这些字节。T231原pbuffer maxBlock1GiB/旧限制不能证明新XComponent context的真实并存内存/速度；不足明确MEMORY_PROFILE_UNSUPPORTED，不删replay/guard/日志冒同policy。

推荐**Kw128档**按同一通式`Eframe=4*(48K+16Kw+2(N−1))+K`重算，不把M0 Kw64基线当Kw128已分配：三bank水增量`3*(64Kw−48*64)`、双pub增量`2*(80Kw−64*64+32N)`，scan另增`4*(Kw−64)`，water motion/两套索引按新Kw计。K192/K512的Eframe **46264/108024**，最大单event block **11,843,584B/27,654,144B**；相对T233总增量 **10,411,904B/20,242,304B**，M1基础总量 **25,471,888B/57,240,208B**。cold配置profile/尺寸/offset/ledger/token必须一起升代；如果只交Kw64，不能宣传Kw128的连续包容量。

这些均为设计payload模板，**actual allocation/driver/RSS/时延 NOT_MEASURED**。另加alignment/程序、shader冷编译、GPU/host view对象、render VBO/纹理/双目标8*w*h、旧场景/LastGood并存、NAPI/IO/capture/可选checkpoint；M1 cold optics/config用新物件时按真实ledger补对象开销。边提案workspace时分复用不重复记，但必须证明其lifetime和frozen面不alias。双frame保留/冻结故障/IO credit沿M0：满则背压，正常不下传大event；长期trace另显式模式计全部搬运。

## 10. 独立测试、效果与工程接力

硬门全部精确：逐相总量、非负、R≤Cq、Hq/实际角原hard、budget、身份/来源、root原子性、迟到/同token。参数效果/延迟/浅水可见厚度/色差/帧率未标定，沿既有目标与独立预注册，不自创“已及格”阈值。designer模型仅核公式/反例/上界与资源，不运行产品或GL；独立tester自持整数/Fraction，不能引用产品helper或本模型作唯一oracle。

| 合同 | 可执行构造/判定 |
| --- | --- |
| WM01 source | 同量AIR/DIRECT/床内、量化0/partial、slot满/ID max/重复/冲突；源成功总水增q一次、AIR当次W不变；实际requested拒量真实 |
| WM02 fall/arrival | 两高度/零角/斜角/墙高床/水面先后，独立弦+T16 ceil时间；ARRIVE W增/J减/R不变/来源不改、无新source成功 |
| WM03 water flux | 平床中央左右累计正流、斜坡允许单流、凹槽/高脊/墙/J占据；head2五界/量子平衡、每对/全域水exact守恒 |
| WM04 wet sand | AIR→WET/出水/同刻床优先、wet终速与隐式drag/无双重重力、跨列medium改变/zero plane/正tick，旧exp仅行为参考 |
| WM05 coupled bed | 沙穿水不排水、LAND抬S/eta且W不变、床流receiver容量含J/W、末CONTACT及面下降释放；不误把P/J画水 |
| WM06 arrival order | ID与y反序、多包抬面、床上升包围/下降、同col多次到达，排序末扫后活包均y>eta；退役slot不重复清/复用新ID |
| WM07 time/event bounds | Vmax极限/反向两弧/列边water等面，eS48/eW16、τ和λ/ceil误差、消费余时WAIT、溢出/同plane失败；超cap整root回滚，不能丢时间/行 |
| WM08 audit/replay | 混water/sand/field namespace、每kind stock映射、翻位/漏行/重行/ordinal/旧medium/来源/used-mask/超arena，spare逐word比较、全相U64大总量 |
| WM09 root/lifecycle | source已成功后quantum失败、后续量子失败前缀、切源stop/后台/reset、late/timeout/context loss、持有pub lease；无每包CPU往返，无从摘要假恢复 |
| WM10 scene | W0/浅深水/断槽/透射/面积高光/在途与退役同token、F32跨列、真实CURRENT与FAULT/STALE；fragment fetch/上下文lease实际验证，无Canvas |
| WM11 cost | K192/512实际bytes/pass/barrier/fence/readback/事件/拒量/失败/root/frame/IO/热UI竞争，旧F64/整数oracle/GPU同offered与accepted负载；不拿少做物理冒提速 |
| WM12 isolation | M0 reader/revision/默认保持，M1新run和feature0b11、M2/M3拒绝；旧T221/T222/T226 F64只兼容参考，不伪GPU效果已落地 |

效测另持同run连续录像/FrameToken+source/ARRIVE/通量/床/总账：空容器先沙成床→切水真实下落/首次水洼；水平两侧扩散/凹槽/斜面方向；停源J继续到达但总水不变；再加沙入水减速/落床抬面；切源/后台无追赶，故障旧完整STALE明确。sourceSuccess/像素颜色不替代真实库存与CURRENT，缺证/未实现记NOT_IMPLEMENTED/NOT_MEASURED。

**交architect的任务书（不使用真机）**：消费本稿及T233/T236实际HEAD，新增`architecture-T237...`或后续编号M1工程/逐word manifest，不覆写在途M0。落四binding/所有write-set、T16除法/plane/medium分类与event64各kind used-mask、water16/scene列16/pub水20/header/completion V2、source双kind/退役/耗尽，**分别**冻结Kw64与推荐Kw128的stage/资源模板，核source与quantum全部rollback域。平台/scene公共hunk一个集成owner，先同context；向既有T232能力探针追加fragment SSBO/profile fetch、新最大块与并存alloc、T16算术和WM事件fixture，勿重复建整套旧平台探针。developer只做GPU M1/编译/离线自测，tester/effector另卡设备/端到端；所有失败先architect，manager登记/路由不加门。

T237交付是算法与测试合同完成，不是M1实现或目标7/8演示通过；CPU-only生产路线不作为本建议，未知平台项已精确化并不阻离线设计，真机串行且GPU最高优先。
