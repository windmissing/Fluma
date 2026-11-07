# T110：严格态writer操作图、舍入包络与支持域补篇 v1

2026-10-26，designer；原卡 `6adf3613e4b0e6ce82cce07b`。路线 `P011_STRICT_REPRESENTABLE_STATE_V1`，补篇标识 `P011_WRITER_PROOF_V1`。DESIGN_READY_FOR_ARCHITECT；未实施／未验证，不重开T98。本轮不使用真机、不改业务／阈值／原始证据。仅新XComponent+shader渲染，禁止Canvas上屏／回退含背景前景，失败FAULT或显式STALE。

## 1. 交付边界

按 `architecture-P011-T104-strict-state-certificates-v1.md` §5–6，先给settle、top relax与pool LAND的可落地操作图、分支证据及有界包络。**不提供通用epsilon／4ULP补丁；不宣布其余writer已经覆盖，不能据本补篇直接开完整严格route。** 其余入口的支持状态见§9，architect须逐核登记后再实施。

新增严格核版本 `SETTLE_RN_GRAPH_V1 / TOP_SPLIT_RN_GRAPH_V1 / LAND_RN_GRAPH_V1`，包络 `AFFINE_RN_ENVELOPE_V1`。对应源码位置以函数与证据SHA为准：WaterSandField.settleSandV2、splitTopV2、relaxAngle；M2Sink.applyHot的kind2与ParticleHot的DEPOSIT patch。不将collector／preview的deltaS当真实库存结果。

## 2. 固定数值对象和可执行的误差传播规则

### 2.1 状态和算术环境

沿T104：`C_c=RN(RN(RN(bh+bl)+mh)+ml)`，RN为binary64最近偶数舍入；同attempt的uTheta位型复用。predicate精确比较这些存储数的 `C_{e+1}−C_e−u`，H不变。非有限、负库存、失效缓存／身份先拒绝。

物理库存不改格式。T104定容dyadic模块用于比较、方向舍入及固定短和；生产不引入测试端Fraction／任意精度库。RN环境、subnormal存取／运算不受FTZ破坏、无FMA／重结合是kernel前置。环境不满足或范围无法给有限包络，返回UNSUPPORTED，不用宽容差。

### 2.2 每个节点都有来源的舍入界

对给定q域J，每个节点存理想实数表达式z*、实际RN值z、区间域和误差 `z−z*∈E_z=[l_z,u_z]`。输入库存／q是其存储数的精确值，初始E=0；已冻结的中间常量直接以实际位型作为新输入常量，不能又假称其是未舍入的物理总和。

对每个RN节点，先用**向外区间运算**求精确运算值的有限范围Z；其局部舍入误差界 `[-η_Z,+η_Z]`。η_Z取覆盖Z的binary64舍入单元的最大半宽，包含跨binade／次正规单元，向上表示；2^-1075不能用F64保存时以可证明向上的最小正次正规界表示，或由T104定容整数承载。η必须在proof record中列node/range/指数来源，不是“每核统一4ULP”。

最小可实现的向外原语：先RN得v，再用精确比较判断v相对a+b、a−b、a*b的位置；必要时只取一次nextUp/nextDown得到对应方向的界。正分母除法用精确比较v*b和a定方向。不是反复走ULP寻找安全q。输入／中间溢出、零分母均拒绝。

误差递推（所有界运算向外）：

```text
add/sub: E_out = E_a ± E_b + [-η,+η]
mul: E_out = a*·E_b + b*·E_a + E_a·E_b + [-η,+η]
divide by frozen D>0: E_out = E_a/D + [-η,+η]
```

这里的a*、b*以对应理想域区间包住，不能随手把乘积交叉项删掉。min/max选定分支没有新增RN，但分支本身必须被域证书证明；未知分支不能套某条公式。完全相同输入／同表达式重算的乘法结果必须是同一node值／误差符号，不能在守恒证明中当作独立随机噪声抵消。

### 2.3 CanonicalS与外邻边

每个changed column的**四项实际库存**按原顺序再加三次RN节点，得到 `C_actual(q)`。设初始四库存精确和T0，δ0=`C0−T0`（精确固定5项短和）；某个固定分支理想库存和变化为 `A_c q+B_c`，则

`C_actual(q)−C0 = A_c q+B_c+E_sum,c−δ0`。

不可漏掉δ0，也不可直接把 `RN(C0+q)`当成库存writer的结果。将系数A、B和E_sum向外包络，得到每列的上下仿射界。边误差是右列下界−左列上界、右列上界−左列下界；固定u精确比较时不再加一次“tan误差”。对pair，受影响边是 `{d−1,d,r−1,r}`去重的合法索引；相邻donor/receiver时即中央＋两外邻边，不能只证中央。

若系数是desired/R等非dyadic有理数，生产只需向外二进制区间系数，不扩展T104为无限有理数。**不要**为追紧界把全部系数乘积塞入T104未批准的长表达式；用上述有限向外节点传播即可，越出其表达式白名单就UNSUPPORTED。

若工程接口要求相对名义斜率k的常量ρ，而非上下仿射式：在已认证J=[L,U]上，令ρ下界为`min((a_lo−k)*L,(a_lo−k)*U)+b_lo`的向下界、上界为`max((a_hi−k)*L,(a_hi−k)*U)+b_hi`的向上界。于是`r_actual−(r0+k*q)∈[ρ下,ρ上]`。本版优先保留仿射式避免丢失q相关性；ρ是具体图、J、边和冻结输入的产物，不跨writer／branch／snapshot复用。

## 3. settle：真实RN操作图与控制量偏差

冻结apply到达本列时的库存、waterTop、canonical C、params、dt，以及原collector留下的 `R=originalRequested`。先按实际表达式冻结：

```text
depth = max(RN(waterTop−C), EPS)
aH = min(1, RN(RN(heavySettleSpeed*dt)/depth))
aL = min(1, RN(RN(lightSettleSpeed*dt)/depth))
dH = RN(suspH*aH); dL = RN(suspL*aL)
D = max(R,EPS)
scale = RN(q/D)
h = RN(dH*scale); l = RN(dL*scale)
suspH' = RN(suspH−h); mh' = RN(mh+h)
suspL' = RN(suspL−l); ml' = RN(ml+l)
```

这里q是budget/旧guard之后的控制量，不是 `h+l` 的精确代名词。R来自collect，dH/dL在apply重新算；不能假定R恰好等于此时dH+dL，连实数比例也未必1。proof domain：R>EPS、q∈J⊆(0,qGranted]、所有冻结量有限／非负，D>0，实际h/l不超susp库存，原stock/capacity守恒检查通过；不满足就RAW失败／域不支持，禁止先截h/l再说“同kernel”。

分支内理想列增量：`A_c=(dH+dL)/D，B_c=0`，其它列A=B=0。guard若仍用名义k=1，遗漏项 `(A_c−1)q` 必须计入偏差，不能只算浮点rounding。更推荐工程包络直接使用A区间，而不是宣称k=1精确。

局部h/l误差：scale节点 η_s，`E_h=dH*E_s+[-η_h,η_h]`，l同理。再通过四个库存add/sub及CanonicalS三节点传播。沉积边界的ρ包括h/l／库存／CanonicalS与δ0，不包含任意状态放宽。wave/水守恒本核不写waterTop，dry更新包络另记衍生状态节点而非当作水源。

shake来源活跃时还必须按原序记录 `sourceH=RN(RN(h*shakeSourceH)/max(suspH,EPS))`（L同理），source残量max0和landed累计的RN节点；它们不进入坡度ρ，但进入stock/provenance／实际patch审计。不能只证明床数组就跳过来源预算。

## 4. top relax：分支图、三段域与相邻边

q从原clip／guard来，relax现有物理分支amount≤EPS不调用split；INWARD必须仍在q>EPS域，否则退ZERO，不改变EPS。冻结donor四库存 `BH,BL,MH,ML`，receiver四库存，`M=RN(MH+ML)`、`D=max(M,EPS)`。

准确操作图（+=/-=都各自RN，括号是原执行顺序）：

```text
f=min(q,M); s=RN(f/D)
h0=RN(MH*s); l0=RN(ML*s)
r1=RN(q−f); bL=min(r1,BL)
l=RN(l0+bL); r2=RN(r1−bL)
bH=min(r2,BH); h=RN(h0+bH)
MHd'=RN(MH−RN(MH*s)); MLd'=RN(ML−RN(ML*s))
BLd'=RN(BL−bL); BHd'=RN(BH−bH)
CanonicalS(donor)                // splitTopV2内部update顺序
MHr'=RN(MHr+h); MLr'=RN(MLr+l)
CanonicalS(donor), CanonicalS(receiver)  // caller更新
```

返回h/l已加bottom分量，donor mixed扣除仍是h0/l0，不能错扣返回h/l；同一RN(MH*s)在返回值与扣除中重复算的位型必须相同。中间update在dry模式有waterTop包络副作用，patch须捕获；纯CanonicalS计算不得调用有副作用的update。

INWARD分支证书对整个J证明以下实际比较，不只看RAW的一个点：

| branch | 实际域 | 理想transfer T*(q)及各相 |
|---|---|---|
| MIX | q≤M | h*=MH*q/D；l*=ML*q/D；T*=(MH+ML)*q/D |
| MIX_LIGHT | q>M，RN(q−M)≤BL | h*=MH*M/D；l*=ML*M/D+(q−M)；T*=q+b |
| MIX_LIGHT_HEAVY | q>M，RN(q−M)>BL，RN(RN(q−M)−BL)≤BH | h*=MH*M/D+(q−M−BL)；l*=ML*M/D+BL；T*=q+b |

其中 `b=(MH+ML)*M/D−M`，MH+ML在理想式是两个存储数的精确和，M仍是实际RN和。不可假定b=0。donor理想ΔS=−T*、receiver=+T*，系数代入§2.3；每段RN节点按图传播，分支常量与q依赖分别处理。

求分支域时，RN(q−M)及其再减BL对q单调，实际域端点可用其RN端点值证明；不能把 `q≤M+BL` 的一个rounded和当作同一条件。q=M归MIX；`r1=BL`归MIX_LIGHT；bH饱和超出库存的段不支持INWARD，不能假设“其斜率还是1”。min/max在边界切换时需重建该段证书，不能复用RAW段的ρ。

本版最小确定域取法：以RAW的实际branch为目标，计算其包含RAW的**已认证闭子域J**；若只能得到退化单点，可RAW认证但不伪造INWARD区间；若区间证书不足，直接INWARD_UNSUPPORTED退ZERO。允许保守J，不要求最大域；必须有构造来源／端点比较证明，不能由developer任选一个邻域。

可实施的J构造：MIX取[0,min(M,qGranted)]；后两段先以(0,qGranted]为候选，用RN减法的逆像界求r1/r2比较所需区间，再相交q>M。RN逆像由binary64舍入单元端点（midpoint及tie-even开闭）按存储位型直接计算；开端点用ceilRep并检查端点实际比较，最多各端一次相邻修正。T104基础模块不含RN逆像时，本版允许只启用MIX的INWARD，**后两段只RAW/ZERO**直到architect补明确端点接口／独立oracle，不由开发盲二分。

## 5. pool LAND：量、ID、CONTACT_WAIT与ZERO区别

冻结合法HOT request身份、phase/source/sourceId、particleId、slot、mass m、column cache、plan.predictedY和params。先reserve ticket，再得到可用qGranted；LAND输入只允许单phase（0→H或1→L），不是一个请求同时两相。原guard host与HOT mirror须使用同一个冻结snapshot和新证书，不能两次独立读取已改变库存。

质量操作图：

```text
h = phaseH ? q : 0; l = phaseL ? q : 0
mh' = RN(mh+h); ml' = RN(ml+l)
remainder = (q===m) ? 0 : RN(m−q)
cache_phase[c]' = RN(cache_phase[c]−q)
mass[slot]' = remainder
if remainder===0: retire same slot; push freeSlot once; liveCount--
else: keep same id/phase/source/sourceId; state=CONTACT_WAIT
      y'=RN(RN(predictedY−contactEps)+contactEps); vy'=0
surface' = CanonicalS(c)
```

pool nextId不因LAND推进；满额释放slot不能再重复写free-list；partial仍是原ID／来源，不给剩余质量换一个birth。pool full、events/lineage空间不足、身份错、audit错误仍整体rollback，不把沙降级到列U绕过。

INWARD域分别为partial `0<q<m` 或full点 `q=m`。full点可以RAW认证，不把它当带宽为零的INWARD区间；partial正q必须有 `RN(m−q)<m`，否则本次转移在pool存储没有生效，不能把bed增加当已守恒。phase相反通道必须保持原库存（即使`+0`数值不变也不必写）。实际pool余量用存储RN值，账本理论m−q和实际余量分列，不假装总是逐位等同。

坡度理想ΔS=q，单活相库存加法误差加CanonicalS误差按§2传播；另将pool减法/cache减法的误差带入原守恒、cache一致性和capacity审计。它们不扩大坡度ρ，却是最终允许LAND的必要检查。水／dry占据包络与provenance亦使用同actual patch。

**ZERO必须直接返回无写材料patch**：不调用splitTopV2、不执行库存±0、不更新dry waterTop、不重新计算缓存、不改变ID/free-list／source／pool质量，不生成成功LAND事件。原HOT DEPOSIT q=0仍可能写state/y/vy，所以不能拿原disposition6自动当作ZERO。

如果首次接触必须把AIRBORNE放到CONTACT_WAIT/停vy，显式分离 `CONTACT_HOLD_MOTION_V1` 运动patch：质量/床/预算转移为0，位置/状态变化原样列证，单独经过pool/几何约束且不冒充LAND／ZERO。本补篇只给其身份边界，**未证明其完整运动准入，严格route启用前须architect另落该支持核**；已经CONTACT_WAIT的ZERO保留其原位型即可。不能为了实现ZERO让粒子穿床，也不能隐改“零量无副作用”的口径。

## 6. RAW / INWARD / ZERO的唯一有界选择

### 6.1 固定non-worsening基准

每个candidate角进入某writer之前，冻结该writer输入库存的CanonicalS和u，不是attempt最早S0，也不是RAW失败后的scratch。对各受影响边精确算r0：若|r0|≤H，bound B_e=H；否则B_e=|r0|。**RAW/INWARD/ZERO三者共用此B_e**，不得失败一次就换新基准累计漂移。下一笔真实被接受writer以它的新输入重新冻结；已有违规边只能non-worsening，最终FINAL仍要求全边≤H。

### 6.2 从包络求可表示q

经§2.3与核图，得到精确r0加上下仿射包络：

`r_actual(q)∈[r0+a_lo q+b_lo, r0+a_hi q+b_hi]`，q≥0，A/B及误差全取向外界。

在该核的已证分支域J内联立：

`r0+a_lo q+b_lo ≥ −B_e`；`r0+a_hi q+b_hi ≤ B_e`。

两条都是关于q的一次约束，分别按系数正/负/0精确分类求端点，再与J、stock和[0,qGranted]相交。除法端点采用向内ceilRep/floorRep，端点以原不等式精确复核；没有代表值就INWARD_NONE。系数是外包的binary64数，最终短和/乘比较按T104白名单分解，不读rounded lo/hi作接受证据。

若内域非空，唯一选**内域上端的最大可表示数**，只对这个保守内域主张最大，不主张原始真实可行域的最大。表达式/算术容量超界、分支未证或非有限直接UNSUPPORTED；不逐ULP搜索，不把实际split的总predicate当单调函数二分。

### 6.3 三候选执行及证明

1. RAW：使用qGranted在独立shadow执行真实图，局部证书/stock/capacity/原守恒/预算/provenance全部通过则选它。RAW不需要ρ来证明已观察的实际patch，但必须完整捕获真实图及支持域，不能调用未登记的旧writer。
2. INWARD：RAW失败时，从**相同输入**恢复，按该图的固定域J求一个q，重新真实执行，再核branch／包络／所有审计；超出已证域不得继续收缩。即使包络保守证明通过，仍做实际证书，不省掉库存图。
3. ZERO：INWARD缺失／失败时，从相同输入返回无写patch，检查冻结r0相对B_e与原stock；ZERO合法不代表整个candidate最终合法。若为真正新角造成全局不可行，仍需OLD或blocked，不能靠ZERO绕FINAL。

最多两次非零真实patch＋一次无写ZERO，无无限尝试。正确包络保证INWARD边残差位于原bound；实际证书是独立防线，发现包络被违反应报ENVELOPE_CERTIFICATE_MISMATCH，不作为普通碰运气失败继续调q。

## 7. grant、分相实量、预算与守恒不换口径

每笔并列记录：qGranted、qControl（所选q）、各实际h/l函数值、各库存前后位型／精确差、pool质量前后、phaseSum=RN(h+l)、实际debit/refund。**qControl不声称等于精确h+l或实际S变化**；settle重算比例及top的M舍入可使其不同。原guard/预算控制量口径保留，拒绝部分退款只取实际已debit的qGranted−qControl，不能额外根据相差再造一次退款。

原事件字段如granted按控制量，则继续该义并增加明确phase／stored-change诊断字段；若已有审计要求其与phaseSum在冻结容差内，仍原样执行，不因新证书放宽。H/L审计使用实际库存＋pool，并与外源成功账比较；不把h/l改成满足控制量之和的假数、不改最后一相“补尾差”来过证书。若实际存储不闭合到原门槛，RAW/INWARD必须拒绝。

pair在理想图中每相从donor取出的h/l正好进入receiver；RN层的损失／增益由操作图和原审计约束，不能口头保证“同q加减必逐位守恒”。LAND相同原则，cache是派生计数仍须与实际pool重算一致。refund及ticket close按实际原合同一次执行；同snapshot三候选只对shadow结算，最终选定结果才原子落权威库存／事件／预算。

即使qControl=0，禁止先reserve→±0写→close造成不可逆身份或预算舍入；可在shadow准备ticket、选ZERO后abort／恢复ticket，不推进成功序号。motion contact若独立授权，预算仍0且独立事件身份，不叫成功转移。

## 8. 最小反例与独立验证要求

本轮推导与源码SHA放 `design-T110-writer-proof-evidence-v1.json`，不是产品测试。至少保留这些反例：

- S0=[24,24+H的最近存储值,24]，实际超H四ULP：q=0无写仍不合法，不能靠ZERO认证成FINAL。
- settle的R取collect数、dH/dL取apply重新算值，A=(dH+dL)/R可非1；只对名义ΔS=q做ρ会漏掉确定性比例偏差。
- top库存MH=.1、ML=.2，M=RN(.1+.2)不同于两存储数的精确和；MIX的A与后两段b不能假定1／0。q=M、RN(q−M)=BL两侧相邻数需要不同域证据。
- donor/receiver中央边看似安全而外邻边在H上，真实phase加法／CanonicalS可先越外邻边；必须三边认证。
- pool full LAND、partial LAND、q==m、q<m但RN(m−q)==m的通用数值域、同phase反通道±0、q0仍改CONTACT_WAIT位置等；不能用只有床量的测试替代ID/free list/provenance。

独立tester用有理数／精确位型oracle逐node验证包络包含真实RN结果：每分支普通点、端点、相邻ULP、EPS与M阈、subnormal及最大支持库存域；随机样本只是辅助，**不能代替节点包络与branch域证明**。量化rho/动作裁剪损失，预演与commit同序逐位、三候选无泄漏、错误rollback/non-frozen、H/L及预算原线都保留。

## 9. 其余writer和publication入口支持矩阵

| 路径 | 本补篇状态／处理 |
|---|---|
| settle、top relax的MIX、单phase LAND | 图／包络规则已定义，工程落逐node和独立oracle后可实施，未标运行已支持 |
| top MIX_LIGHT/HEAVY | RAW图定义；INWARD需RN逆像分支端点接口，不具备则只RAW/ZERO，不假装生产全覆盖 |
| CONTACT_HOLD_MOTION | 与材料ZERO分开，运动核证明尚缺；首次contact相应严格路径UNSUPPORTED |
| percolate／同列phase重排 | 理想S不变不够，RN会改变CanonicalS；未给真实图，不支持，禁止静默执行旧核 |
| bottom two-stage/phase-balanced、deep creep、correction、lift的seed/merge/湿fallback、advection／crossing／merge | 分支／provenance/预算不同，本版未证明；每一实际可达非零路径需registry命中，否则STRICT_WRITER_UNSUPPORTED。配置关闭且确实不调用／不改状态可跳过，但不能只因grant打印0就认无副作用 |
| initialize | cold candidate全数组／pool／预算／源基准组装后，CanonicalS、角门和原不变量FINAL通过才新run；非法场景明确INIT_STRICT_INFEASIBLE，不抹平初态 |
| restore | 输入version/全部状态位型和FINAL验证通过后原子替换；旧非法／缺revision快照拒迁移，保留原件，不能自动reset |
| invert | 当前实现转换多数组/角/池，未给逐node图；本版不支持主动invert。未来单纯visual mirror不改物理且不发新物理状态可另登记，但不能把现函数当纯显示 |
| dry／wet外源、runtime add/drainWater、legacy inject、route/config change | 外源完整事务／水包络与发布身份未在本篇证明；严格route不得直通。全零初始化支持不推出运行注入支持；legacy inject还重置基准，禁止拿它修旧态 |
| 非状态改动的同token重绘 | 不产生新物理publication，沿原token／STALE边界；不把renderer成功当物理认证 |

这张矩阵是**明确支持域而非完整启用清单**。因此T104基础模块／oracle可继续；三个核及尚缺writer、contact/外源publication全部闭合前，不能开启完整严格产品route或声称step73/设备已修。本轮回architect逐核工程落地；若要更宽支持需后续设计补篇，不把UNSUPPORTED改成旧实现降级。

## 10. 交接与资源

不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容，仅自身状态有限例外；不改滴答同名清单，不回退共享修改。不使用真机、仅新shader、无Canvas回退。architect维护T104工程接口／registry／node manifests再交developer；designer不实施业务。前五目标优先，T106独立P3不争用这些文件。

每核固定node数、changedColumn≤2与受影响边≤3；包络／branch证书固定缓冲，RAW与INWARD各一份scratch可复用、ZERO不分配。CanonicalS全场终结O(N)，局部O(固定图节点)；pool身份查找沿已批准固定索引／有界路径，不为数值证书新增全池全对扫描。若新分支图超定容上限显式CAPACITY_UNSUPPORTED，禁止热扩容。

本篇给安全推导与支持边界，不把未证明范围写成已完成产品能力；不改变质量容差／EPS／H／物理预算、旧失败或设备根因。测试失败仍先回architect，manager只登记派工，不增验收门。
