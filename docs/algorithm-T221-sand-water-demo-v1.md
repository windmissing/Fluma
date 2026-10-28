# T221：目标8沙水交互 DEMO 演进方案 v1

2026-10-28，designer；原卡 `6ae19fb6e4b0f11bdf2b2ca2`。路线 **`sand-water-demo-flight-v1`**，合同 **`CW_DEMO_FLIGHT_DESIGN_V1`**，状态 **DESIGN_READY_FOR_ARCHITECT / NOT_IMPLEMENTED / NOT_VALIDATED**。目标8/P3排目标1–7之后，GPU compute仍最高调度优先。消费T196/T203、T209开发、T215独立报告、T219当次部分工程稿及当前clearwater源码。输入指纹及本人的算术模型输出见 `design-T221-sand-water-demo-evidence-v1.json`；复盘另文。

不使用真机，不实现业务、不重跑产品测试，不回退共享修改。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改其它行列/队列或滴答同名清单。所有渲染含背景/前景只能走新XComponent+shader，禁止Canvas上屏或失败回退；失败FAULT或显式STALE完整旧有效新管线帧。manager登记交付并推进依赖，不新增验收门。

## 1. 结论、实际能力和版本隔离

推荐**同一明水owner内的有界在途水包 + 到达转W + 既有邻接水头扩散 + 完整新管线场景**。水包不是SPH，不求压力、飞溅或水滴相互作用；在途库存只保存尚未进入W的水，绝不能把同一质量同时记在W和水包。页面不建立第二份仿真库存，不继续运行旧干场owner再叠一个水owner。目标5的交互/空容器体验可复用，其旧WaterSandField/Hot状态不能热接成明水。

| 证据/部件 | 已有范围 | 目标8仍缺什么 |
| --- | --- | --- |
| T209 A/B；T215独立12/12 | 真W、单沙床/活动粒子、源幂等、正dt耦合及有界重放 | 无在途水、无完整页面/切源/FrameToken/可视明水；不是DEMO PASS |
| Source.commitNext水分支 | `c=floor(x); work.W[c]+=q`；y仅校验未进床 | 无高度相关飞行/到达时序，不能画假水柱把即时注水叫落水 |
| T219 C工程稿 | 同owner只读capture、双packet、V3校验、透射、完整STALE、输入生命周期已定 | 水面高光/真实目标角仍有设计缺口；C尚未实施。本文§7/§8给可计算补充，不修改其文件 |
| 目标5当前失败 | T202记录干场微负cache→top>water/code100，旧帧STALE | T216实施→T217独立→T218同DEMO真机尚为真实接力；不以本方案宣告已修 |

**这是有行为改变的新能力版本，不声称A/B V1全轨迹等价。** 目标7原`SINGLE_SAND_CLEAR_WATER_V1`、source row20/56/24、B event40/旧snapshot及独立证据保持其历史含义。目标8新run明确使用：

- `SINGLE_SAND_CLEAR_WATER_FLIGHT_V2` / `CW_SOURCE_FLIGHT_V2` / `CW_FLIGHT_STATE_V1`；
- `CW_B_FLIGHT_STEP_V2` / `CW_FLIGHT_EVENT48_V1` / `CW_REPLAY_TAPE_V1` / `CW_FLIGHT_RESERVATION_V1`；
- `CW_SCENE_FLIGHT_V4` / `CW_WATER_OPTICS16_V2` / `CW_GRAVITY_ACCEPT_V1`。

同一组现存Engine/State/Source职责内扩展，cold配置选择唯一模式，严禁复制一套W引擎或把旧V1请求送到V2解释。V2需新run冷创建；不静默迁移旧run/旧snapshot，restore继续UNSUPPORTED_FEATURE。目标7 C可先按T219已定合同开发，不能因等待在途水把整个C基础接线冻结。本文不是T207干场V2：**W真实非零，不能用DRY_AIR/干场water=0统计或exact-RN替换目标7的ID顺序P。**

## 2. 完整体验及可观测时序

1. 新run空容器，W/沙/在途水均0；默认掉沙，主指DOWN首tick只立时钟、采样量0。后续真实active dt生成源请求；沙沿原phase0活动粒子下落并LAND，床逐步形成。屏幕点到模型坐标使用当前geometry/镜像/box，y向上，不按屏幕颜色猜沙面。
2. 切到掉水立即结束旧手势，需新DOWN。触点在当前沙/水面上方时，成功源只出生一个有真实质量和位置的在途水包，**当次W不增加**。后续成功子步推进位置；到达当前床/积水表面才产生ARRIVE、W增加、在途量减少。触点已在水体内且不在床内时，明确DIRECT_WATER分支即时入W，没有下落动画；触点进床则POSITION_IN_BED拒绝，不把点挪到床顶。
3. 第一次到达后，水位`eta=S+W`可见抬升；本路线先做水头扫描、末尾处理到达，故新到达水**下一子步**开始横向流，不伪称同一子步已扩散。水平平床中央注水，左右边累计真实外流都应出现；奇偶串行顺序允许瞬时不对称，不要求每帧两边相等。
4. 凹槽先就近积水再按水头/越床上界流向相邻列；高于水头的沙脊不能直接穿透。倾斜时`head=eta-(c+.5)*tan(angle)`决定方向，可以单向流；不把单向流误判成“不向两侧扩散”。左右墙封闭，无溢流泄漏。
5. UP/CANCEL停止新出生，已成功出生的水继续下落/到达，W继续内部扩散；总水保持，不能把“停源后W仍增加”判为偷注水，应检查在途水同步下降。全部到达之后W仍可重分布，水面不应因停手消失。
6. 再切掉沙，新DOWN出生沙；沙穿过水体的阻力/沉降沿B，LAND抬S且W不减少，eta抬升并随后重分布。飞行点沙不排水，床排水有既定一子步响应；这是B的模型边界，不宣称浪花/压力求解。
7. 旋转/摇晃改变有效目标角时，在支持±30°域中连续接受重力方向，原始角、限域角、接受角分别记录。线加速度shake强度首版仅诊断；不伪称已有摇晃扬沙/惯性水浪，详§8。

成功receipt、物理commit、publication、实际CURRENT swap分别回报。可先有source成功而没有任何CURRENT帧；页面不能用成功提示代替完整演示证据。

## 3. 库存、容量与数值合同

### 3.1 单一水总账及在途状态

沿N=128、容器C=256、沙K≤512；水包独立类型/ID命名空间，固定`Kw`，首版容量档**Kw=64、ew=8**，均为工程规模候选，非效果标定结论。每个水请求最多一个包，质量为真实准入q，不借沙massPackage拆水/占沙slot。合法水包：正质量finite、`x∈[0,N), y∈[0,C]`、finite速度、正单调U32 waterId、originSourceCommit、birthPhysicsCommit；状态AIR/INTERFACE_WAIT/SIDE_WAIT，死亡质量0/退槽一次，slot复用分配新ID。来源指成功事务，不是sourceId。water header16固定语义为`[head,tail,freeCount,liveCount,nextWaterId,arrivalSerial,simulationTime,lastArrivalPhysicsCommit,flightRevision,waterArenaSerial,emissionPolicyRev,flightConfigRev,sortPolicyRev,0,0,0]`；首尾空链为-1，未用槽正零，整数/版本/时间域分别校验。

每列`J[c]`为按**waterId递增**归约当前在途质量的RN fold。原P仍按sandId递增归约；S/eta/R原图保留，另：

```text
S[c] = RN(RN(RN(bh[c]+0)+mh[c])+0)
eta[c] = RN(S[c]+W[c])
R[c] = RN(eta[c]+P_ID[c])
R8[c] = RN(R[c]+J_ID[c]) <= C
Wtotal = RN(fold_c_ascending(W) + fold_waterId_ascending(mass))
expectedWater = RN(initialWater + I_water)    // 新run initialJ=0
Sandtotal / expectedSand = 原B定义
```

源正成功出生J或DIRECT_WATER都只把q加入I_water一次、waterSuccessCount/sourceCommit/physicsCommit各按原正成功推进。MOVE/CROSS/ARRIVE/WAIT不是外源，不改I_water、waterSuccessCount/sourceCommit或初始基准。ARRIVE定义`W'=RN(W+m)`、退役同一m；只存W或只减J的半交换绝不提交。源实际总水增量与q、每次内部总量差沿原绝对1e-6核；整步水/沙各自总差≤1e-6，源累计相对drift沿原`abs(actual-expected)/max(expected,1e-9)<.001`。原域/阈值不改，RN导致的实际差值必须保留，不补尾质量。极小q加W无写时不能成功，出生正质量J即使全域fold舍入不显也是真实库存变化，须记录原bits。

V1 `audit()`以ΣW独对I_water，不适用于在途非0。V2须**显式新水审计定义**：保留所有原stock/沙ID/cache/有限性核，水全域换为上述W+在途；新增J/水链/来源/总容量核。不得调用V1审计后忽略失败、在审计前临时篡改I_water、或把J当沙P骗过审计。V1入口维持原审计。

### 3.2 空间保留与局部容量

在途包是稀疏点水，飞行图形不代表已填满整列；为了到达有空间，质量J在**当前列**保留容量。它不抬eta、不参与水头/光学深度，不是体内排水。保留会降低该列其它源/横流水/沙的准入，这是明确的保守模型选择；不是旧B的数值等价优化。

所有V2源准入和water/bed/LAND/sand CROSS/water CROSS都必须考虑J。候选用原70limb辅助核，把每列**实际沙粒质量+在途水包质量**加在同一私有`Gexact`；`E=exact(S)+exact(W)+Gexact`，入口冻结`Bcap=max(exact(C),Eentry)`，局部真实patch不恶化该界。更新用旧/新stored operands分别编码，不能对控制q直接加减；每事件最多两列O(70)，禁止逐事件全池归约。B旧的`Pexact`在V2中明确更名/升策略为Gexact，不能泄漏到V1。

初始RN可容极小exact越界的原态保留方式沿T196，但最终无条件核原R≤C与新R8≤C；没有新容量EPS。正源q先受`maxSourceVolume`、当前`C-S-P-W-J`、fixed资源、token等限制，再真实candidate audit，不能单凭名义free宣布成功。DIRECT与flight各自审实际变化。水跨列整包，目标保留空间不足则SIDE_WAIT，**不把水强塞目标W、不分裂ID、不越墙**。ARRIVE同列把J变W仍可能因RN使exact界或R8失败：保持INTERFACE_WAIT并报告NUMERIC_ARRIVAL_BLOCKED；不缩q、丢余量或钳零。等待在W内部的点水仍在途计账、显示实际位置，不隐藏成已到达。夹具若反复等待/明显不可达，先交architect，不用“小于视觉阈值”抹除。

## 4. 水源事务、身份与交互生命周期

### 4.1 请求与成功

沿同owner唯一FIFO，queue≤64、dedup/event≤4096，本run不驱逐。key仍是`runGeneration,inputGeneration,gestureId,sourceId,requestSeq,kind`（runId/owner另绑定）；config/geometry/clock/raw timestamp/x/y/q/seed和±0位型按payload核。V2请求revision和cold emissionPolicy也是身份绑定，不能以同key把V1直接水重放成V2水包。支持只读终态查询，复用T219 `copySourceReceiptInto`，不得二次enqueue当通用查询。

```text
enqueue -> PENDING ACK (未注入)
显式 source boundary 逐请求:
  检查run/input/geometry/time/版本、整state与事件/去重/水slot/token容量
  freeze sourceBefore; work <- canonical (包括J和水包，五bank无alias)
  SAND: 原沙出生图，但V2容量含J
  WATER and S<=y<=eta: DIRECT_WATER；W+=q，无水包
  WATER and y>eta: FLIGHT；出生1个waterId，m=q,x/y=触点，v=(0,-vEmit)
  重建派生一次；V2源audit；准备receipt/event/suffix/origin
  同步提交stock+两池+源账+完整receipt+事件+dedup终态
失败不动已成功库存/水ID/成功计数；终态失败按既有去重规则留痕
```

不能先按A注入W再扣回给J，这会污染账本/失败次序。若y<沙面拒绝；y==eta显式DIRECT，没有虚构飞行时长。水包slot耗尽只拒本水请求，沙池资源独立，但共享总容量/队列真实检查。正q部分准入有实际PARTIAL回执，不显示requested全成功。

**建议线格式**：继续复用同一FIFO/dedup/源事件分配与key逻辑，V2明确新row revision；保留20/56/24字长的公共字段位置，V2 prefix的globalWater字段明确指Wtotal。按同一row index增补dedup `suffix16`、event `suffix8`，不建第二张身份表。suffix16为`[settledBefore,settledAfter,flightBefore,flightAfter,JcolBefore,JcolAfter,firstWaterId,waterBirthCount,deliveryMode,nextWaterIdBefore,nextWaterIdAfter,arrivalSerialAtBirth,emissionPolicyRev,birthSimulationTime,0,0]`；suffix8为`[waterId,deliveryMode,JcolBefore,JcolAfter,nextWaterIdBefore,nextWaterIdAfter,emissionPolicyRev,birthSimulationTime]`。deliveryMode NONE0/DIRECT1/FLIGHT2；沙相关水字段0。回执带suffix revision及明确两类库存。公共字段不表示V1兼容；V1 reader必须拒绝V2。suffix容量/终态计数与prefix同一提交，不能先发布成功prefix后补suffix。request原始20槽无新热字段，emissionPolicy通过cold config和revision绑定。

水量是该active窗口的**集中出生量**，不是声称连续喷流各微团在窗口内均匀出生。源成功发生在处理时的当前模拟边界，`birthSimulationTime=simulationTime`，随后正dt子步从此边界推进；原始timestamp只证明采样/速率窗口，不倒填成已经飞行过的物理时间。simulationTime只随正dt成功以既定RN顺序增加，失败/暂停不增长，不能用`wallNow-birthTimestamp`直接摆水包位置。source处理墙钟、birth模拟时间、arrival模拟时间、首次CURRENT墙钟分开记录；raw窗口请求不补历史飞行，近表面可在一个子步内到达属正常结果。

### 4.2 用户动作和时钟

双选源仅`SAND/WATER`，不是两条同时发源的手势。主pointer单占；MOVE更新坐标；每active tick最多一个新请求，采样rateKind×**同一raw时钟先作差一次再换秒**。dt>0≤1/30，拆至多两个≤1/60子步，首tick/倒退/NaN/换单位/超窗不补量、不截dt后追赶；沿T219 continuity和clockWindow身份。

| 动作 | 尚未提交请求 | 已成功沙/水包/W | 时钟/显示 |
| --- | --- | --- | --- |
| UP/CANCEL/切源 | stopSourceGeneration，新input代清旧pending | 保留；active时继续物理 | 释放pointer，切源需新DOWN，无补发 |
| 后台/退出/geometry失效 | 先stop/清pending | owner仍在则保留；暂停dt | 返回首tick0，旧已成功水不撤销；需新DOWN |
| reset | 先stop，串行新run空状态 | 明确清空，初始账归新run | 新token/ID域、无旧LastGood跨run CURRENT；不是运行中重置基准 |
| source成功后本step失败 | 已成功源不撤销 | 保留出生包/直接水；本candidate回滚 | capture当前canonical并列failure，source成功≠CURRENT |
| 第二子步失败 | 不重放源 | 保留第一成功子步 | 失败后本tick不重试/补推进 |
| context loss | 停止新源，当前完整packet失效 | 不动owner；不以restore伪恢复 | 新context重建后capture/reupload，失败FAULT/兼容旧完整STALE |

stop/切源只影响未提交输入，不清在途水。正常停止检查`Wtotal`保持而非ΣW立即恒定；后台暂停期间不按墙钟计算落水轨迹。帧排队固定有界，不coalesce有身份的请求；资源不足明确拒绝/背压，不把ACK记成功，不在显示线程一次补齐积压。

## 5. V2完整子步、碰撞和原子重放

### 5.1 有界时序

```text
dt0 -> strict no-write (连队列/预算/诊断成功serial都不改)
正dt、同一个GravityAcceptanceToken:
  原子root资源预检、V2 entry audit；stepBefore/work复制；建立Gexact/Bcap与原固体guard
  1. 原水头核：每邻边一次、奇偶先后沿stepSerial；容量含J
  2. 冻结 etaMedium / wetMask（不含J）
  3. waterId递增一轮：水包空气轨迹/墙/列边/冻结水面接触，最多ew-1行/包
  4. 原sandId一轮：空气/湿运动、CROSS/CONTACT/LAND，容量含当前J
  5. 原床邻边一次、原预算/guard
  6. 沙原末尾contact+IDfold；刷新当前S/eta/P
  7. 水包索引一次固定scratch归并排序(column,y,id)，按此顺序一次接触/ARRIVE收尾
     每个入口包至多1行，ew总上限含此行；不再次推进时间，不做第二轮水头扫描
  8. 一次waterId fold产生J/R8；V2 work audit、全步水沙差/guard/预算核
  9. replay从stepBefore，按完整tape顺序验old bits/应用actual changes，末尾独立fold/audit
  10. authority/两池/预算/完整事件逐位比较，预检全部存储，同步提交+切sealed
```

每个正dt一个candidate，无λ搜索或重试；replay不是第二次轨迹预测。失败回stepBefore，包括在途位置/速度、J、水nextId/链/free、W、沙、预算、sealed/acceptedAngle；已有源及之前成功子步保持。V2 entry/work/replay三次whole audit，各融合一次两池/列扫描，禁止按水事件重新调用完整A audit或source commit。所有copy字节包含两池，不能漏掉source路径五bank扩展。

### 5.2 轨迹与碰撞范围

水是点包，屏幕宽度仅视觉，不作体积/碰撞半径。冷配置明确`gFall>0, dragFlight>=0, vEmit>=0`及finite域，候选待标定；和沙gModel采用同一模型单位及重力角，不得按屏幕像素/s²直接代入。一个空气段h：

```text
vx1 = RN(RN(vx*exp(-dragFlight*h)) + RN(gFall*sin(angle)*h))
vy1 = RN(RN(vy*exp(-dragFlight*h)) - RN(gFall*cos(angle)*h))
x1 = RN(x + RN(vx1*h)); y1 = RN(y + RN(vy1*h))
```

沿B明确的**候选直线弦**找最早λ，不声称真实解析弹道根；事件时间`hEvent=RN(h*λ)`，交点沿同弦，速度按hEvent重算，剩余时间继续。同刻优先：当前/目标可接触的床或水界面→容器墙→列边。界面是当前列冻结etaMedium；目标列较高界面在列边阻挡，不穿高沙脊。床/水面接触只停在交点、转INTERFACE_WAIT，材料转换统一在末尾处理。左右墙零法向速度/余时等待，无出界丢水；顶部初始源在C内，后续向上撞顶同墙处理。接受左右跨列及受阻位置沿B的精确边界/nextDown规则，整包容量Gexact转移、无固体预算扣费，无1e-9几何偏移。

同一个零时刻plane按出射方向/上次plane身份推进拓扑；正时间RN不减少remaining且无拓扑进展则TIME_NO_PROGRESS整步失败。ew−1用完还需运动则EVENT_LIMIT失败，不能静默丢余时。MOVE/WAIT的真实速度/位置变化必须记事件，不以材料q0冒无写。水/沙不做点对点碰撞；水包之间无SPH/融合；只撞容器与沙床/积水的柱状界面。气相水点不排水，但J保留容量。

末尾按(column,y,id)升序扫描，用**当时当前**eta判断到达：`y<=eta[c]`时请求整包ARRIVE，必须真实W写变化、容量/数值/事件全部成功才退役；否则记录明确WAIT。未触及当前界面者保留原位置，之前接触面下降则释放AIR，下步继续，不瞬移到新低面。床上升包围水点是准静态接触，同样请求ARRIVE而不是反射/飞溅。按低到高处理避免晚到达抬面后遗漏更高包；只按ID收尾可能发生“先跳过高包，后低包到达抬面吞没它”的不一致。正常可到达包一次扫描即可；数值/容量受阻的包明确允许INTERFACE_WAIT，不能伪称到达。排序一次O(Kw log Kw)，不是每次ARRIVE排序或反复扫全池。

### 5.3 事件与验证责任

水另用`event48`，不把水ID伪装沙ID/改写原B40。双arena中一行48 F64：`[kind,localOrdinal,slot,id,origin,birthCommit,fromCol,toCol,m0,m1,x0,y0,vx0,vy0,x1,y1,vx1,vy1,W0,W1,Jfrom0,Jfrom1,Jto0,Jto1,state0,state1,wait0,wait1,remaining0,remaining1,hEvent,lambda,planeType,planeCoord,reason,flags,0..0]`，尾36–47正零。kind MOVE1/CROSS2/CONTACT_WAIT3/ARRIVE4；ARRIVE m1=0，W0/1为本列，slot退役一次，其它操作W槽0。J旧/新是**局部exact工作量的RN证据摘要**，不充最终canonical J_ID；逐事件从完整old/new mass/column核Gexact，而最终J只末尾IDfold。接口/plane/used-mask由architect落manifest，reserved不得塞未定义字段。

完整replay tape为U32 `[stream,rowIndex]`：stream0原B40、stream1水48，tape位置+1是全局ordinal，row内部ordinal仍各stream连续，不让B V1 reader猜跨流序号。tape每行引用一次、顺序严格符合阶段，不能漏水行/重复退役；replay不重新积分或排序选到达，只验事件旧bits、合法拓扑、stage、真实数值patch、容量/来源并应用。两套candidate产生器与独立tester的oracle必须分开。还需独立重算碰撞与到达可达性，不能只靠产品replay同源实现当正确证明。

## 6. 水头扩散、积水与预期效果边界

沿原B：`u=tan(angle)`, `head=RN(eta-(c+.5)*u)`, `z=RN(S-(c+.5)*u)`；每邻边一次，q受`kappaW*dt*delta, delta/2, donorW, max(0,hd-max(zd,zr)), 接收真实free`五界。V2的free/Gexact审核包含J，实际W差两端/全域审计保持；水不动固体300/s、28/s账。无全列均摊/贴图spread/重置initialWater；凹槽/沙脊/墙边界从真实S/W决定。

平床中央单列W>0、两邻可接收、kappaW>0、足够可表示质量时，有限步中左右外流都应为正；原奇偶扫描不能保证首步相等。更远列因一次奇偶序至多有限传播，不得跨全域瞬时水平；记录边流q/实际差、累计左右signed flux和首次到达时间。沙面倾斜、两侧容量不等、J占据、侧墙、凹槽缺口都有不同预期，测试必须分场景。B既有高头流/床流能力复用，但加入J会改变准入与轨迹，必须独立V2回归。

能得到的是：可观察的水下落、沙面真实积水、渐进扩散/汇聚、透明水层下仍可见沙、再加沙抬水位；**不承诺**水滴互撞、湍流/飞溅、惯性波浪、孔隙渗透、摇晃扬沙或全角度翻转。前述未实现现象若有后续目标另开路线，不以旧雾或假动画顶替。

## 7. 新管线同帧水体、水面和在途水（含T219 D1补充）

### 7.1 可直接复用/需要新版本的部分

复用T219同owner只读capture/双ETS与native packet/原x64证明/F32投影/完整场景swap/FAULT-STALE原则，**不可每帧snapshotB**。V4 profile每列10 F64 `[S,W,eta,P,bh,mh,R,J,R8,wet]`，wet仅`W>0`；waterProof每活包10 F64 `[id,m,x,y,vx,vy,origin,birthCommit,state,wait]`按waterId升序，xy32/meta32另预分配。native重折P/J、核R8/总账绑定及水来源/状态/原坐标，不能以F32列反推质量。

native V4与V3独立parser/scene revision。水面、水体、沙床、活动沙、水包、背景前景均读同packet/FrameToken；token增加flightPolicy/config/flightState/arrivalSerial/emissionPolicy和waterProof摘要，精确结构字段比较。水source后B失败时当前packet须包含出生J；sealed旧B不能冒当前water proof。CURRENT只在完整新scene成功swap后，失败不能当前沙+旧水混拼。V4 LastGood不能和V3/V1、旧run/context/geometry互换；显式STALE回旧token，不虚构本源已显示。

层序建议`BACKGROUND → SAND_BED → SAND_PARTICLES → WATER_BODY → WATER_SURFACE → FLIGHT_WATER → FOREGROUND`。新到达包在同一提交中退役、水体W增加，不能同token同时画活水包和已入W的同一质量。水包水下WAIT仍按其实际坐标绘制，并保留原因诊断；没有从喷口连到水面的固定假水柱/拖尾。包只用真实点/质量确定非体积glyph，不能用于推导物理体积。

水包glyph也给独立可计算首版：顶点用proof的当前xy32，实例化屏幕圆片，`rPx=clamp(radiusScalePx*sqrt(m/mRef),rMinPx,rMaxPx)`；片元距中心d，`coverage=clamp(rPx+.5-d,0,1)`，`alpha=opacity*coverage`；局部高光`h=clamp(1-distance(pixel,center+(-.3*rPx,.3*rPx))/(.4*rPx),0,1)^4`，预乘RGB=`alpha*((1-h)*tintLinear+h*lightRGB)`，按容器box裁剪。无全屏假流或历史拖尾。冷`FlightVisualConfig`含版本、mRef、radiusScalePx/rMin/rMax/opacity，mRef∈[2^-20,256]、0<rMin≤rMax≤8px、scale>0≤8px、opacity∈[0,1]，保证本质量域比值finite；候选mRef=.125、scale=2、rMin=.5、rMax=4、opacity=.6，PENDING_CALIBRATION。glyph含可见最小半径仅属点显示，不抬eta或改碰撞/库存；独立测试以proof而非像素面积对水账。CPU oracle直接同几何公式核coverage/premult，效果合格仍等真实影像。

### 7.2 透射和水面精确公式

水体继续T219线性预乘公式：`W==0 || y>eta -> alpha=0`；其它`path=clamp(eta-max(y,S),0,W)`, `T=exp(-sigma*path)`, `(rgb,a)=((1-T)*tintLinear,1-T)`。不用WATER_FOG/INTERFACE_FOG，不采读正在写的FBO，不把云雾染蓝当明水。sigma0只表示内部透射恒等，水面仍可反光。

补齐T219 **CW-C-D1**：新`optics16=[sigma,tintR,tintG,tintB,surfaceGain,opticsRev,profileRev,reserved0,widthPx,lightR,lightG,lightB,F0,specPower,lightX,lightY]`；sigma≥0，tint/lightRGB/gain/F0各[0,1]，widthPx>0≤8，specPower∈[1,64]，light向量finite非零；profileRev与V3/V4分别绑定，不偷填旧8槽。规范化light一次冷做，shader/CPU用同一记录。参考候选`widthPx=1.5,gain=.35,lightRGB=(1,1,1),F0=.02,p=32,L=(-.4,1)`，**PENDING_CALIBRATION，不是已通过默认/新阈值**；sigma/tint沿显式配置，不凭空改旧颜色。

在容器投影的**实际屏幕像素**定义水面带，不旋转一个脱离真实eta的假平面。模型y经geometry映射到向上屏幕坐标`Y`，每列床/面为`Ys/Ye`；带`[max(Ys,Ye-widthPx),Ye]`。对像素footprint方形I，覆盖率`A=area(I ∩ 当前列屏幕矩形 ∩ 带)/area(I)`；0≤A≤1，镜像/box/像素中心由同geometry确定。本首版支持轴对齐容器box且每模型列投影宽度≥1px（box宽≥N），绑定时明确检查；过小窗口返回UNSUPPORTED_GEOMETRY，不让“每像素最多相邻两列”假设失效后偷偷全列扫描。支持域内每像素最多与相邻两列求交并加覆盖，列内nearest profile，不跨干列桥接。W0无带，浅水厚度<widthPx时带被真实水体裁剪，不强行画最少一像素。抗锯齿来自解析面积，不用模型epsilon或人为扩水mask。

法线取当前湿列及直接相连湿邻的eta：两邻均湿`s=(etaR-etaL)/2`，仅右/左湿取相应单边差，都干取0；`n=normalize((-s,1))`, `v=(0,1)`, `F=F0+(1-F0)*(1-clamp(dot(n,v),0,1))^5`, `G=clamp(dot(n,L),0,1)^specPower`，`aSurface=surfaceGain*A*(F+(1-F)*G)`，预乘RGB=`aSurface*lightRGB`。dot/覆盖率的[0,1]裁剪仅是光学合法域，绝不改物理库存；覆盖率浮点求和后同样限制到[0,1]。邻列仅用于光照法线，不平滑W/S/eta/边界，不跨沙脊连出水体。连续干湿边没有水的像素a=0；相邻两孤立水槽不会被高光横连。gain0整个水面层恒等；W→0覆盖连续趋0；更改分辨率保持widthPx单位但实际受水厚度限制。

线性合成`GL_ONE/GL_ONE_MINUS_SRC_ALPHA`，目标沿T219的sRGB framebuffer合同；同时测水下床/粒子可见性和前景遮挡。CPU oracle直接算footprint交面积、n/F/G和RGBA；GPU F32/色空间误差阈值由独立tester按现有渲染容差及实际格式预注册，**这里不给未标定像素误差/主观分数判PASS**。该公式闭合缺口不等其视觉已验证；高光弱/水色不合格应按实测回architect分流。

以下为可直接翻译进新水面fragment的纯核（参考GLSL，未编译/运行，不是业务代码修改）；调用者沿上述最多两列交面积得到A，W0直接返回vec4(0)：

```glsl
float overlap1D(float a, float b, float lo, float hi) {
  return max(0.0, min(b, hi) - max(a, lo));
}
// Pixel center p in actual upward screen pixels; column bounds x0/x1.
float bandArea(vec2 p, float x0, float x1, float Ys, float Ye, float widthPx) {
  return overlap1D(p.x-.5, p.x+.5, x0, x1)
       * overlap1D(p.y-.5, p.y+.5, max(Ys, Ye-widthPx), Ye);
}
vec4 surfacePremult(float A, float slope, float gain, float F0,
                    float specPower, vec2 normalizedLight, vec3 lightLinear) {
  vec2 n = normalize(vec2(-slope, 1.0));
  float F = F0 + (1.0-F0)*pow(1.0-clamp(n.y, 0.0, 1.0), 5.0);
  float G = pow(clamp(dot(n, normalizedLight), 0.0, 1.0), specPower);
  float a = gain*clamp(A, 0.0, 1.0)*(F+(1.0-F)*G);
  return vec4(a*lightLinear, a);
}
```

有不同slope的两列共享一个像素时分别以各自bandArea求surfacePremult后**面积加权相加（A已含面积，不再乘第二遍）**，最后作为一个水面fragment输出；不要两次over把不重叠subpixel面积当全像素层反复遮挡。CPU模型`area/surface_alpha`给单列纯核，正式tester须再补两列/镜像的完整footprint oracle。

## 8. 真实重力接受策略（T219 D2补充）

首版**明确选“姿态改变重力方向，线加速度shake只诊断”**。这保留拿动/摇晃设备造成姿态变化的重力响应，区别于未实现的纯平移摇晃激振；不能在产品文案宣称摇晃扬沙/惯性水浪。原T50 adapter保留其低通/±89°target/shake数值，使用原source='sensor'的身份；合成角仅离线/显式测试模式，绝不冒真传感器证据。

新`CW_GRAVITY_ACCEPT_V1`是独立于旧WaterSandField planner的明确支持域映射，**不是默默clamp后声称追随原角**：

```text
每active tick开始一次冻结最新adapter样本与raw sampleTime/adapter revision
valid && target finite && sample freshness通过原250ms候选 && 非synthetic生产冒用
tBound = min(30,max(-30,target)); saturated=(tBound!=target)
每实际子步h，从上次成功a0计算
deltaFiltered = (tBound-a0)*(1-exp(-h/tau))
delta = min(omega*h,max(-omega*h,deltaFiltered))
aTrial = a0+delta; 核finite/[-30,30] (非法失败，不尾端钳aTrial)
以同一tick冻结样本，但各子步自己的aTrial/token进B/水轨迹/水头/guard
仅该子步成功才接受aTrial和angleRev；失败保持a0、停止本tick后续尝试
```

tau>0、omega>0有限，候选tau=.12s、omega=90°/s，标定未完成。初始`a0=0,angleValid=0`；首个正dt成功后angleValid=1，不让首传感器样本瞬时把物理变到30°。输入adapter滤波状态是“观测状态”，可以先更新；acceptedAngle状态必须与物理commit一起提交。source-only帧带最后接受角及其commit，不能把新raw target用来转水面；几何eta不由UI倾角伪造。两个子步只冻结一次传感器样本，第二aTrial从第一成功角计算；第二失败保留第一角/commit。

缺样/非法/stale/时间倒退时停止新源、暂停该tick物理且保持旧acceptedAngle；不换0°或猜上一dt补步，页面明确GRAVITY_STALE。已有W/J/沙保留，恢复先时钟0并需新DOWN；已有在途包下一个有效正dt继续。raw target超30°仅标SATURATED_SUPPORTED_RANGE，不称±89/翻转已支持。shakeInput/magnitude和stale原样诊断，不改变g/drag/预算/床流；如要纯平移摇晃驱动，另版给有效加速度、输入频谱、守恒/守卫和独立oracle，不在本次暗加。

`GravityAcceptanceToken`至少含run/owner/input/config/adapterRev、tick/sampleTime/sampleSeq、rawTarget/boundedTarget/saturated/shake、dt/attempt/substepIndex、aBefore/aTrial、angleRevCandidate、committedPhysicsCommit或failure。token绑定整个子步/重放/scene，计时未测不填0。T219可直接消费此明确算法后由architect补其两个hunk；T221不抢写工程文件。

## 9. 完整frame工作量和固定资源

设G为本tick实际源边界（页面最多1）、Q为边界处理请求（≤64，含历史已入队但未提交项）、D≤4096、K沙池上限、Kw水池上限、F≤2正dt实际attempt，Ls≤eK+2(N−1)、Lw≤ewKw、Lt≤Ls+Lw。逐步simulationTime RN加法须finite且正dt使其确实前进，否则TIME_NO_PROGRESS失败，不累积丢失时间。**不能只报飞行核复杂度/只报一个成功子步。**

```text
frame = 全enqueue扫描 ΣO(D+Q) + G*sourceDrain(Q)
 sourceDrain = Σ请求[前置/拒绝的真实成本 +
   若构造candidate: O(N+K+Kw) copies/rebuild/audit + O(本次沙births或1水birth)]
 stepAttempt = O(70*(N+K+Kw))入口精确核
   + O(N)水/床 + O(70*(Ls+Lw))局部patch
   + O(Kw log Kw)一次接触排序 + O(N+K+Kw)末fold/审核
   + O(Ls+Lw)重放/来源/tape + O(N+K+Kw)末独立审核/比较
   + 完整before/work/replay/commit拷贝、故障提前退出实际行
 frame后端 = O(N+K+Kw)只读capture+hash+native proof/F32/上传
   + 全场景实际像素/层工作+submit/fence/present+诊断/IO
```

水跨列多事件只O(70)，不逐事件扫任一pool；排序每attempt至多一次，最多2attempt/frame，重放不再积分/排序。源逐请求candidate/copy乘Q不能删，冷attach/constructor/reset/恢复重传、source查询D扫描、完整IO/fsync/failed-candidate计数亦独立记录。加入T197整体frame分母时此为**clearwater独立route分支**，不把旧Hot的λ/merge乘数直接套成0成本，也不把GPU整数选对默认当本水核加速。UI owner/背压沿有界执行者、无半帧，运行设备排队让位GPU；没有设备时不承诺帧率/硬实时。

以下为**在T209 A+B、T219 C模板之外的新增固定payload设计预算**，非实际RSS，Kw64、ew8、N128、D4096；详模型JSON逐项公式：

| 新增部分 | 公式/责任 |
| --- | --- |
| 五state的water pool/J/header | `5*(74Kw+8N+128)`；每pool 7F64、idU32、state/wait各U8、prev/next各I32、freeU32；J F64[N]、header16 |
| 双水event48/header64/summary16 | `2*(48*8*ewKw+64*8+16*8*Kw)` |
| 双全局tape | `2*2*4*(eK+2(N−1)+ewKw)` |
| 排序双索引/marks/flight scratch | `9Kw+256`；不热new/slice/Map/字符串 |
| source suffix | `16*8*D+8*8*D+16*8`；按原表row index共享计数 |
| 四份ETS/native水proof/xy/meta | `384Kw`；两端各双packet |
| V4 profile/双GPU texture增量 | `64N+32N`；由8→10槽，GPU两行→三行，尾纹理槽明确0 |
| 双水GL VBO/额外header/optics | `32Kw+1024+256`；optics8→16四份增量 |

新增小计：K192 **1,304,480B**，K512 **1,345,440B**。相加T209 owner 4,230,656/6,312,256与T219 C模板207,112/331,272，得设计基础总量 **5,742,248/7,988,968B**；这是数组/指定上传模板的和，不是完整峰值。sourceBefore/stepBefore/work/replay/authority五bank扩展全计；不另建全水池Exact70，复用V2 Gexact合同，但实际实现若新增独立audit bank/undo/COW须加账。

另加T219的双全帧目标`8*widthPx*heightPx`（若不能复用既存目标，还要旧/新同存峰值），driver/纹理对齐、程序/VAO、对象/view headers、冷配置/字符串、NAPI临时复制、截图/IO队列、设备进程以及可选snapshot。V2完整snapshot尚未设计可恢复入口，禁止热导出/偷偷保留副本。实际编译实现后必须以allocation ledger替换模板，不以此小计冒内存PASS。事件/tape/队列先预检定容，耗尽前显式失败/拒绝，不能热扩容/丢事件/借下一帧预算。

## 10. 独立测试与效果合同

tester/effector另卡建正式用例/报告，不修改既有用例/阈值。本方案仅给可执行oracle/记录要求；下面拓扑/零写/身份为精确判定，原质量阈值沿B，fps/时长/像素/主观分档均**PENDING_CALIBRATION**。离线设计模型不等产品测试；预测试未实现登记NOT_IMPLEMENTED，不派假缺陷。

| 编号 | 独立客观构造与判定 |
| --- | --- |
| SW01 库存/源 | 同q分别直接/空中注水：源成功总水增q一次；FLIGHT当次W不变；沙总量不变，J有正质量及唯一来源；pending ACK无库存成功 |
| SW02 落下/到达 | 两个高度，固定零角/速度配置，用自持弦段碰撞oracle核事件时间/位置和先后；到达前W不增；ARRIVE W增加/J减少且源成功序号不增。时间测量分模拟时间/墙钟 |
| SW03 容量/侧壁 | 满列/保留J/砂包占据/墙/高床/整包跨列，独立exact bits核容量和Bcap，不拿产品ExactMass/replay作oracle；拒绝不漏水/不分裂 |
| SW04 水沙独立 | 空水、仅沙、混合、沙落床抬eta、W恒量、P与J不同ID顺序fold；反例避免把P或J算作W光学深度 |
| SW05 双侧 | 平床中央可表示的水、kappaW>0，两邻空余；累计左/右真实外流均>0，不强求单步镜像。对照斜床允许单流/墙边仅内侧；每条WATER核五界及实际差 |
| SW06 汇聚/止源 | 凹槽/高脊阻挡；停源input代推进后无新成功，已有J到达引W增但Wtotal保持；J0后停源水仍保持/重分布，禁止initial基准变化 |
| SW07 幂等/切源 | exact key replay原终态、不同payload冲突、切源需新DOWN、旧代pending清、每tick≤1源；一份原成功反馈计数，不按重复receipt再动画计量 |
| SW08 生命周期 | CANCEL/后台/geometry/context/reset，raw首差单位/超窗/倒退/恢复不追赶；保留已成功J/W，reset确为新run；无restore假成功 |
| SW09 原子性 | 在water move/arrival/work audit/replay/commit预检各处故障：当前root全回，source成功和前一子步保留；拒绝保留首错/token，tape/两arena无半封存 |
| SW10 排序/有界 | 构造ID与y反序、多水包同列抬面、床后接触、次正规/舍入无写、ew/arena/tape耗尽；一个排序/attempt，无逐事件全池审计，实际Q/F/L/copyBytes完整scope |
| SW11 optics | W0/浅水/深水/两湿槽夹干列/gain0/sigma0/镜像与DPI；CPU独立面积/Fresnel/预乘核；原x64归约、F32跨列投影、不同commit禁止混层 |
| SW12 帧/故障 | desired/captured/rendered token、source-only后B失败、第二子步失败、GL/config/swap/context故障；只完整CURRENT/完整兼容STALE/FAULT，没有Canvas或旧雾路径 |
| SW13 重力 | ±89→显式saturated±30、接受滤波/速率、同tick两子步、失败角回滚、stale暂停不归0、shake仅诊断/合成输入身份；独立算token及角序列 |
| SW14 V1隔离 | 旧A/B源行/snapshot/status0..22/ID归约/轨迹在未启V2时保留；V2专用审计/水总账/reader显式拒错版本；禁止误套T207 DRY数值策略 |

效测至少四段连续同run录像/逐FrameToken回执：E-SW1空容器掉沙成床后切水、可辨下落→首次积水；E-SW2水平床左右扩散与凹槽汇聚/高脊阻挡；E-SW3停源仍留水、再加沙穿水/抬面、手持方向改变；E-SW4透明水下床/粒子可读、高光不跨干槽、切源/后台无追赶、故障STALE标识。必须同时给source/arrival/WATER/床库存和实际CURRENT截图，不能用成功计数代替可见积水。画面可见厚度/光学参数、扩散速度/可见落程/延迟/评分由effector预注册同路线候选矩阵与原有标准，不新造“已合格分数”；缺证据如实NOT_MEASURED。

## 11. 精确工程接力与完成边界

交architect（不使用真机）的任务书：

1. 以本文route/revision做唯一V2差异manifest，逐字段审计State/Source/Engine/Step/Hydraulics/ExactMass/Events的写路径，五bank含水链/J/origin/ID/预算，逐一列出V1隔离、V2总水审计/容量策略，不能仅在页面延时调用A直接注水。
2. 定版source suffix/receipt查询与固定资源；water pool、48行used-mask/plane tag、global tape、old/new bit检查和commit恢复；把首次错误、Q/F/ew/实际copies与IO加入全frame计数。冷构造资源不足需原子拒绝，不抢GPU资源。
3. 对T219只补D1 optics16/面积高光及D2明确重力策略的工程hunk，保留其它已定publication/透射/输入工作；V4水proof/profile/token在其基础另manifest，公共Engine/native bridge/CMake/UI一个集成owner，串行合入，不覆盖在途T219文件。
4. 页面作为目标5空容器体验的后继集成，运行唯一clearwater owner；当前目标5缺陷按T216/T217/T218修复证据接力，不拉旧干场库存进V2、不把goal7 B离线PASS作DEMO完成。物理V2、C基础、V4/page、独立/设备分成有真实前置的实施单元，manager据此建卡；本文不越权创建或改其它卡。
5. developer只负责编码、真实ArkTS/native编译和非真机自测；独立tester补SW01–14与NOT_IMPLEMENTED基线、effector补E-SW1–4，产品/设备验证另卡，设备打真机标签并串行，GPU最高优先。阈值未标定保持未标定，失败先architect分流。

设计交付可完成，目标8产品仍未实现/验证。后续完整实现需目标7 C基础实际交付、V2物理/帧管线与页面交付、目标5必要修复和独立/设备证据；不新增manager验收门，不重新开启已关闭设计卡。
