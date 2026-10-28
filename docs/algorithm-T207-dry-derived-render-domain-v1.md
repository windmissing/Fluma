# T207 干场派生量、live质量归约与严格渲染域 v1

2026-10-28，designer；原卡 `6ae15c23e4b0d25fef5adaf3`。补篇 `P011_DRY_DERIVED_DOMAIN_V2_DESIGN`，**DESIGN_READY_FOR_ARCHITECT，未实施／未产品验证**。消费T202正式归因与两份analysis、T197整体方案及T205当前资源manifest。只设计当前dry双场跨域合同，不借本卡修改目标7明水或wet物理。

不使用真机、不改业务／原raw／阈值、不重跑产品测试，不回退共享修改。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列／队列／滴答同名清单。全部渲染含背景前景仅新XComponent+shader，禁止Canvas上屏／失败回退；失败FAULT或明确STALE上一有效完整新管线帧。manager按交付登记，不新增验收门。

## 1. 已证明的故障与推荐路线

T202第57列原F64：sandTop/bh/bl/mh/ml/suspH/L为+0，waterTop=colParticleMassH=`−3×2^-54`，L cache为+0；renderer source mask128即top>water，发生在F32／GL之前。原24片、49063字符、20数组SHA及STALE回执均有据，不能改称导出缺接线或CURRENT。完整pool/首残差writer历史未采，**不假定该列为空，不断言LAND/CROSS哪次独占首因**。

推荐在**物理owner**引入确定性、精确存储质量之和的非负派生cache：`P011_LIVE_MASS_EXACT70_V2`。每个支持的实际pool mutation按真实before/after质量／列／phase更新定长整数acc，正确RN一次形成cache；dry envelope在原逻辑读／提交边界从同一revision缓存与床重算。归约不再依赖过往“加q减q”的历史残差，空集合严格为+0，正质量集合不可能得到负数。

这不是在renderer或publication把负数max到0，也不改pMass／床／实际水源；renderer严格条件和原first-failure捕获保持。**cache取值／归约方式改变会影响物理决策**，因此作为显式dry数值revision实施，不叫与旧轨迹全量逐位等价。不允许先做publication-only正值拷贝而让物理继续用旧负cache，再冒称同一物理状态合法。

版本组合：

| 领域 | 新明确版本 |
|---|---|
| derived cache | `P011_LIVE_MASS_EXACT70_V2` |
| 支持物理policy | `P011_DRY_CACHE_EXACT_V2`（medium仍DRY_AIR_V1，但必须绑定新policy字段，不能只靠旧字符串） |
| dry包络 | `P011_DRY_ENVELOPE_ORDERED_V2` |
| publication | `P011_DERIVED_PUBLICATION_V2`，新增内容版本／域证书身份 |
| 数值迁移 | `P011_DERIVED_MIGRATION_V2`，仅显式冷迁移或新run，不改旧快照／原bits |

T197的 `TX_EXACT_STRUCTURE_V1` 仍只对原数值policy谈等价；本篇属于新的数值policy维度，T205需登记policy-aware read/write域、证书失效和资源，不能将V2对V1的真实差异隐藏为结构优化通过。

## 2. 权威数据、严格支持域与失败边界

权威仍为live pool每slot `pState/pMass/pX/pPhase/pId/source...`、床四库存、源成功账和原预算。cache `colParticleMassH/L`、dry waterTop及sandTop为**确定函数的派生量**，不是新的沙相或外源。inactive slot的旧pMass残值不参与归约；state非法、live质量≤0／非finite、phase非法、坐标越界、ID/free-list损坏沿真实校验失败，不能过滤掉非法active后宣派生合法。

首版支持C=128、N≤512、capacity=256的现有主域，两个phase0/1。每live mass≤256、总项≤512，exact phase列量≤N*256<2^18；定容70×16bit、scale2^-1074足够。其它容量／更大域需显式范围证明和capacity revision，不能溢出后截断。没有为更大GPU诊断档自动放宽生产域。

dry有效提交域：bh/bl/mh/ml有限且各≥0，suspH/L为0（原dry合同），live质量各>0；cache finite/nonnegative、精确空列为+0；床CanonicalS按原左结合；包络严格top≤occupiedEnvelope≤capacity。旧物理EPS容许的微负**床权威**不在本修复范围，仍明确拒绝域证书；不能改床／吞负库存／对负床做同种“修复”。原H/drift/预算EPS作用域不变，新增strict dry可发布域是显式支持约束。

错误域独立命名 `DERIVED_INPUT_INVALID / DERIVED_ACC_NEGATIVE / DERIVED_ACC_OVERFLOW / DERIVED_CERT_STALE / DRY_DOMAIN_INFEASIBLE / PUBLICATION_DOMAIN_MISMATCH`，由architect映射原code/stage身份，不冒新错误都是原100。原V1原码／诊断仍原样保留；V2失败不发新CURRENT，也不篡改已成功source的epoch或账。

## 3. exact70表示、更新与正确舍入

### 3.1 表示及正确性

每列、每phase一个70-limb unsigned little-index整数A，实际值A×2^-1074。对非负binary64质量m，读取原位型：

```text
exponentField==0: encode(m)=fractionBits
else: encode(m)=(2^52+fractionBits) << (exponentField-1)
A[c,phase] = sum encode(actualStoredMass of matching live slots)
```

1120bits覆盖到实数<2^46，远大于本支持域<2^18。运算只做16bit limb加／减／比较；每加进位临时≤131071，每减显式borrow，使用精确整数Number／U16存储，不用会溢出的32bit移位表示整个52bit尾数。cold固定DataView解析bit，运行不得临时new TypedArray/BigInt/任意精度库；测试端任意精度只作独立参考。

初始化／冷迁移从校验过的完整pool扫描一次建A。对每actual mutation，记≤2个slot的完整旧tuple `(live,phase,column,mass,slotGeneration)`，运行原材料/运动F64图，再读真实新tuple，先从对应A减去旧质量编码，再加新质量编码。只更新真实changed buckets，不用名义q代替actual remainder或merge结果。

```text
for each old tuple if live: A_oldBucket -= encode(oldStoredMass)
for each new tuple if live: A_newBucket += encode(newStoredMass)
validate no borrow/overflow, live count bucket/identity bookkeeping
for each changed bucket: cache = roundNearestEven(A*2^-1074)
mark corresponding dry envelope/physical numeric reads/certificates dirty
```

中间A只是private patch，完整mutation准备完才替换可读派生cache；不让减旧后尚未加新的一半结果被guard/audit/下一birth读到。材料失败恢复其原rollback域的A/cache/dirty/rev，不逆算负q猜旧值。MERGE虽同group也必须读实际新mass和实际floor(x)；CROSS源与目标，DEPOSIT remainder、DEGRADE退役、full/partial LAND都走tuple写屏障。纯velocity改动不动A；位置改变但floor不变不动列量，但仍使particle publication内容revision变化。

V2分支的原`cache+=amount/cache-=amount`必须被该派生输出**替换**，不能与exact更新双执行；pool canonical、patchScratch／field mirror各副本只能从同一derived结果同步，禁止各自加减再靠EPS比对。MERGE旧实现即使没有显式cache写也会改变实际存储总量（`RN(ma+mb)`不一定等两旧质量精确和），所以它同样要走两个旧tuple删除、一个实际幸存tuple插入；不能仅以“同列同phase”免更新。

归纳：初态A是非负项之和，删旧tuple只删实际仍包含项，再插实际正质量；因此合法mutation后A精确等于当前live集合的存储质量和、无历史误差。A=0当且仅当该bucket无live项（质量必须>0）；不使用EPS定义空列。

### 3.2 RN转换的可实现规则

设A=0返回derived +0；0<A<2^52为可精确表示次正规，直接编码fraction=A。其它取b=bitLength(A)、s=max(0,b−53)、M=A>>s，保留的M为53bit有效位。根据丢弃位与2^(s−1)比较做nearest-even：大于half向上、等于half仅M为奇数向上；M进位成2^53时右移一位并s++。normal exponentField=s+1，fraction=M−2^52，组装原F64位型；超有限域明确overflow，不返回Infinity或max finite。s=0没有丢弃位，不执行负shift。

bitLength／guard／round／sticky由70-limb扫描求，界O(70)固定；不得转字符串再parseFloat或靠宿主`Number(BigInt)`在生产实现。该RN结果只依赖当前集合，不依赖插入/删除顺序或AVL树形；若m>0则A至少一个2^-1074单位，RN结果也>0，最小次正规不会被当作0。平台FTZ／位型写入不保持subnormal时拒数值环境，不能以“设备很小看不见”吞掉。

由非负exact sum得cache≥0；binary64 RN在非负域单调，0输入转+0。**不证明存储质量本身由merge/LAND逐位守恒**：原RN mass图的偏差继续由原审计／源账判定，acc只是准确反映实际stored masses，不能给质量修正尾差。

## 4. 固定阶段重建、审计与整体复杂度

### 4.1 不把O(N)扫描放到每事件／UI

每mutation只触≤4个old/new bucket，编码／加减／RN各O(70)，fixed microtuple和scratch无全pool scan。所有pool writer、public reset/restore/seed/batch/merge/side-fallback和preview branch必须登记写屏障，不允许只修DEPOSIT留下其它cache增减路径。

每public root入口若从已有**同V2**合法root继承且read/write证书有效，不重新建A；seed/row是同root内savepoint，继承当前A／undo。λ／preview回滚选择正确branch A根或恢复undo，不能把旧A贴新mutation号。与T197一致，bulk import/未知alias不得用旧证书；需要cold重建只能在明确入口做一次并计入Bbuild，不能每个merge/seed触发全量build。

每个成功public source或成功substep，在最终接受边界做一次独立从完整live pool建auditAcc、比较A及rounded cache的逐位一致，成本O(70N+140C)；可融入原已有末尾full audit的scan，**不新增每SINK_ROW一遍pool扫描**。失败early-exit的已消耗工作也计；如果因后续原检查失败撤回，此边界检查结果不代表commit成功。数字“每root一次”指新增derived重建验证，不删除原Hot row/tail/finish审核。

原Hot audit保留slot-order质量fold、field-before/after／预算／容差比较，不改成exactsum重排；其已有slot fold与V2 cache的比较仍沿1e-9，原conservation/drift等不放宽。V2最终derived证书额外要求cache精确等RN(A)，不能用旧EPS代替。支持域合法phase列量T<257，slot RN求和与RN(exact)的标准相对误差界≤(γ_512+u)T，u=2^-53；保守值<1.5×10^-11，小于旧1e-9，但最终总审计仍原样实际执行，不以此误差界跳验证。subnormal非负和用位型oracle单列验证，禁止FTZ。

### 4.2 与T197/T205的聚合公式

设L=70、U_mass为实际pool membership/mass/column/phase mutation数量，Bbuild为初始/import／失效重建次数，Jroot为实际public成功边界尝试检查数，Pcopy为acc整bank复制次数，Dflush为实际dirty-column刷新次数：

`W_207 = O(L*U_mass + (Bbuild+Jroot)*L*(N+2C) + Pcopy*2CL + Dflush)`。

整帧`W_totalV2 = T197原完整执行集合之W + W_207`，原source R≤8、V≤3/S≤2、DIRECT帧外调用仍计，不把Jroot误写成每UI帧1。U_mass包含每次未提交λ、discard preview、实际birth/merge、partial LAND，不只最终committed事件。不能T197原audit和“融入同一次扫描”重复计算同读取，但新增70-limb操作真实计入。诊断捕获／SHA／publication copy／IO照T197独立列项，zero allocation不等zero CPU。

renderer消费同commit两次重绘不扫描pool／不建A，不改authority、source或physics commit；只复用owner已封存的域证书／immutable profile，O(C+N)的正常packet/particle pack成本仍按实际frame次数计，不因为“不做acc扫描”说整个UI为O(1)。

## 5. dry envelope和publication严格域证明

沿原四床库存的实际RN序列：

```text
bottom = bh
mix = RN(bh+bl)
top = RN(RN(RN(bh+bl)+mh)+ml)
P_H = RN(exact current phase-H live sum)
P_L = RN(exact current phase-L live sum)
occupiedEnvelope = RN(RN(RN(top+suspH)+suspL)+RN(P_H+P_L))
dry requires suspH=suspL=0
```

该式刻意保留原`updateSandTop`左结合和`particleMassAt=H+L`分组，不改成RN(四床＋所有粒子一次精确和)。由全部库存和cache非负、RN单调，严格bottom≤mix≤top≤occupiedEnvelope成立；零床、零live空列给+0包络，不会出现T202的top0>负water。全部值必须finite且occupiedEnvelope≤capacity，否则V2候选／publication拒绝，不用renderer修容量。

在dry内，旧字段waterTop明示语义为**占据包络，不是明水库存／自由面**；元数据`waterTopMeaning=DRY_OCCUPIED_ENVELOPE_V2`，明水目标7仍用独立W/eta，wet旧水位也不被此版本覆盖。renderer dry水雾继续禁用，不把包络画成水。

V2 dry的真实水量按medium定义为+0，`getWaterVolume/getTotals/freeWaterTotal`在此policy分支直接返回dry无水语义，不再用`RN(RN(envelope−top)−RN(P_H+P_L))`反解：即使所有输入非负，加法舍入也可让这个反解出现微正／微负“幽灵水”。这是**明确数值语义修订**，不是对真实水库存钳零；dry根本没有独立W，湿场／目标7有真实水时禁止走此分支。dry初始/expected water基准与源类型一致为0，原drift比较函数／门槛不变；迁移先核没有真实水来源／湿库存，不能用dry标签吞掉旧wet水量。occupancy与水量两个统计分开，不把体积预留叫液体。

owner在每个原write之后、下一读之前按dirty column重算相同envelope：包括pool mass/cross/birth/retire更新、床变化和dry零U验证。所有直接写waterTop路径（M2Sink、WaterSandField、projection/shadow／restore）改为统一owner resolver或经同图验证的接口，不存在两套分别±q的包络维护。来源是同一个actual pool/field revision，flush必须在audit/capacity／guard需要它、以及commit之前完成；不在renderer临时改snapshot包络。

### 5.1 publication阶段和身份

新publication必须带run/stateEpoch/physicsCommitId/poolMutationSerial/fieldRevision/derivedRevision/cachePolicy/envelopePolicy/domainCertificateId，source ledger key／预算版本及原FrameToken绑定也保留。derivedRevision只在同owner私有Work封存成功后对应physics commit生效，不每UI copy乱增。诊断同时标AUTHORITATIVE_POOL、DERIVED_CACHE、DRY_ENVELOPE，原array bytes仍直接捕获，禁止捕获前调用refresh“修首错”。

commit过程：完成原事务写／原审核→完整derived验证／strict dry域→stage所有field+pool+cache+域证书到inactive publication bank→一次切authority/metadata/publication handle（或单owner保证在切换前不可消费）→成功源／physics回执。任何失败恢复原逻辑rollback范围，不能先sourceSuccess然后因未封存的snapshot让调用者读半新材料。已成功source在后续step失败仍保留其已封存源publication／ledger。

current copy接口必须只复制一个已封存commit，不执行修复／重建；out pool和out field不得来自不同commit，consumer错误版本／token／mixed domain必须拒。旧V1 snapshot不带V2域证书不得冒V2；`copyPublishedSceneStateInto`和`copyPublishedRenderStateInto`需一致接受规则，不能DEMO旁路只改其中一个。独立renderer拒绝旧负cache是预期，不能把支持V2变成默认接受所有旧material。

### 5.2 renderer继续严格验证

保留当前P013FrameAdapter的finite、0≤bottom≤mix≤top≤water≤capacity及MATERIAL_VALUE对mh/ml非负检查，无新EPS、max(water,top)、cache钳零或跳source验证。V2证书是owner证明来源，不代替consumer核长度／位型／token／严格域；坏packet仍拒绝。

同一非负单调平滑算子依原四pass、相同边缘复制／F64 scratch和除3应用于四个有序profile，逐个运算RN单调使profile次序保持；正值同权核也保持序。F64→F32正确RN单调，finite域转换不改变次序；若生成非finite或capacity转换相关异常，仍在原F32验证拒，不“修F32”。这证明的是受支持非负输入的次序闭包，**不证明所有后续材质／GL都成功**。shader/FBO/swap任一失败仍FAULT或完整STALE，不能CURRENT标志越过native实际新帧成功。

## 6. 数值版本差异、源账预算与迁移

| 项 | V2承诺与V1比较 |
|---|---|
| 权威pMass／位置／velocity／床writer表达式 | 在相同request/grant/branch输入下原F64操作图不变，不修质量／补尾相 |
| pair最小选择／ID/free-list/来源 | 相同pool位型和支持域下沿原/已批准结构选对；派生sum本身不重排pair；真实后续pool若分支变化则不能声称仍同轨迹 |
| phase列cache | 真实变化：旧历史RN增减改为RN(当前存储质量精确和)，不保旧负残差／旧slot-fold位型；derived-only数值升版 |
| dry water统计 | 真实变化：旧包络减法反解的浮点余量改为medium定义无水+0，不改wet/真实W，不靠残差给DRY增加水源；旧V1余量仅留历史诊断 |
| capacity／wet/dry检测／guard输入 | 真实变化：凡读P或包络的路径可能在边界给不同grant/reject／后续事件，须新policy输入结果差分，不用1e-9藏差 |
| source账 | 记真实成功准入及原initial+source公式、正成功身份规则不变；V2可能改变哪些请求成功／部分量，绝不先照V1写源成功再改cache |
| 物理预算 | 300/28、scale/debit/refund、闭式guard规则及H/EPS/drift门槛不变；由于实际grant或accepted分支变化，used数值／事件可能真实不同，逐笔自洽验证，非全轨迹逐位等价 |
| rollback/preview | branch原子恢复包含A/cache/dirty/rev/证书；preview丢弃不污染authority；原源成功与step失败边界保持 |
| 诊断／错误 | 旧T202证据/旧revision原错保留；新policy不能伪造旧帧已过或将域故障都改0；输出明确版本与真实当前/STALE |

实施必须先由architect列实际read/write接口与差异；本设计明确选择数值V2，不能混入“TX_EXACT_STRUCTURE_V1全轨迹无差”验收。后续tester分为**条件操作图等价**与**V2新域正确性／真实轨迹差异**两个表，不要求不可能的全状态一致，也不将所有差异随便归为允许。

新run是推荐启用边界：V1运行与旧raw继续可回放原版本，V2 fresh initialize校验pool并建acc，旧非法权威或strict dry无解明确失败，不原地清理。显式冷迁移V1 live run到V2，仅当完整authority/pool/来源/预算/field合法并可校验时，在独立candidate重建derived并通过所有V2域检查，以`DERIVED_MIGRATION_V2`记录before/after cache bits、同源累计及新stateEpoch／policy，再原子替换。它可能改变未来准入，必须显式请求；不是后台copy snapshot时无声升级。

T202原包poolReplayComplete=false不能执行上述迁移、不能从负cache反推“空池”。本方案不需要再取首writer历史才能设计；如后续要证明该**旧现场**在V2重算结果，才最小补齐同token13 pool buffers＋live/free/nextId、field/source/预算身份；若要定位首残差writer，还需一次相邻mutation前后tuple／cache原bits。只在首目标失败冷预算冻结一次、不全帧dump／不调用update改变首错，不把缺现场回放当本设计阻塞。

## 7. mutation／branch／资源合同

所有支持入口：initialize/reset、新source dry batch/seed、birth／merge／full/partial LAND／DEGRADE／cross／motion floor改变、INVERT、RESTORE、ROUTE_CHANGE、preview/shadow clone、λ rollback、公用snapshot导入。route OFF或wet仍用原policy，不偷换物理水语义。invert若为纯视觉不动pool无需A变；原物理invert改列则是bulk重建／显式delta迁移，不能只镜像cache数组后沿旧证书。

全部branch的A与actual Work绑定，不可与canonical共写；沿T197/T205固定view/保存点。缓存证书依赖pool live/state/mass/phase/floorX及content bits、field/params/owner/schema；即使mutation号没变但pointer/alias／内容变仍失效。unknown writer拒V2配置或在明确cold入口重建，不在下一UI帧偷偷补救。

每view的主体：2*C*70个U16=35,840B（C128）；bucket liveCount两个phase用2C U32=1024B（身份／零集审计）；dirty mark C U8=128B；固定4个limb scratch＋tuple8B位视图=568B，合计**37,560B/view**。现cache H/L数组已在原pool13buffer内，不重复计；若scratch由owner共享需明确无重入／view lease，不靠alias节省未定义内存。

T205提案8view若各一套为300,480B。另public边界独立auditAcc/counter bank和scratch一套37,560B，则新增基本payload **338,040B**；old/new bytes undo/COW、证书metadata、完整域header、双publication新增字段、cold迁移旧/新峰值由architect分项增加，不能把338040当全程序内存。冷资源不足让GPU优先，不热扩容／缩数据／丢事件／驱逐source账。

undo每个受影响bucket至多写70个limb＋counter/cache和dirty/header，必须按原nested savepoint保first-old-value；若记录每次limb写则在T205 undoWordMax中加入`O(70*U_mass*depth)`上界。整view复制35KB不是免费，对每次λ/preview／savepoint实施究竟copy或undo须在Pcopy计数明确；**不能seed内部每次copy8view全套**。old state回滚恢复acc原words与cache原bits，非重新fold得一个数学等价值冒逐字恢复。

工作计数新增 `DERIVED_BUILD_SLOT / TUPLE_REMOVE/ADD / LIMB_VISIT / ROUND_GUARD_STICKY / CACHE_WRITE / DIRTY_ENVELOPE_FLUSH / DOMAIN_CHECK_COLUMN / PUBLIC_DERIVED_VERIFY / DERIVED_CERT_HIT/MISS / MIGRATION`，并挂原T197 source/λ/candidate/seed/public/frame父scope。原audit数值fold与renderer pack不被隐藏。性能门沿既有整frame/事务合同，服务率／CPU常数PENDING_DEVICE_CALIBRATION，不以模型毫秒或单个acc操作通过外推设备稳定。

## 8. 候选取舍及独立可测合同

候选A“renderer EPS／max(top,water)／负cache钳零”拒绝；它改显示输入并掩盖物理派生错误。候选B“每事件从pool重fold非负cache”可在逻辑上消残差，但O(U*N)，拒作目标实现。候选C“每UI帧publication自行重建正包络、物理仍旧cache”拒作修复，混不同域和同token来源。候选D“固定slot二叉sum-tree”可确定非负且O(logN)更新，但C×phase×N表及树形RN另语义、可能更大内存；留备选。候选E“本篇exact70增量成员表＋边界独立重建”推荐：数值集合定义精确、固定字宽、更新不全池扫描，成本与差异明确。候选F“保留旧轨迹但只做渲染投影容忍”不满足本卡严格消费合同。

| ID | 独立tester断言／原值保留 |
|---|---|
| DD-01 | 原T202 24片/20数组hex/SHA、第57列−3×2^-54及mask128/STALE逐值核，旧renderer仍拒，原raw不改；不因V2模型空列合成例假造旧pool |
| DD-02 | exact70编码/RN与独立任意精度oracle：0/−0、min subnormal、max subnormal、min normal、tie-even/跨binade/多项和；负/NaN/Inf/live0明确拒，不过滤 |
| DD-03 | 同一live集合不同合法birth/cross/retire历史的derived bits一致；空列+0、非空>0；旧incremental产生负数的合成例与新集合同步 |
| DD-04 | partial/full LAND、DEGRADE、MERGE质量RN、cross双列/phase、floor改变、退役slot复用：remove旧tuple/add实际新tuple，不用control q猜剩余量 |
| DD-05 | 原同request/grant输入的质量/床/ID/预算writer条件等价；V1/V2 cache不同导致准入/trajectory变化独立列明，源成功只记实际结果 |
| DD-06 | 原source成功→step失败、λ／preview discard、嵌套seed失败、token/arena/acc超限：恢复全部acc/cache/header/源账正确边界，旧成功前缀保留 |
| DD-07 | bh/bl/mh/ml非负但极不平衡，四床原RN图、H/L两cache、strict包络；负床／U非零／超capacity拒，不能用本方案把其它非法态改合法 |
| DD-08 | public同commit field/pool/cache/domain身份，mixed revision/pointer/bit变／旧V1包错标V2均拒；重绘不扫描pool，不改变physics/derived/source序号 |
| DD-09 | renderer SOURCE_VALIDATION、四pass平滑及F32严格有序／finite，所有旧坏packet拒绝；GPU upload/swap失败仍FAULT/完整STALE，不能source成功冒CURRENT |
| DD-10 | initialize/reset/invert/restore/route/change/cold迁移支持矩阵；旧无pool证据只能不可重放，不能从cache猜authority；wet/明水route隔离 |
| DD-11 | N192/512、C128、source/λ/preview/大量mutation矩阵：LIMB及build/copy/flush计数满足§4，public verifier≤每public最终边界一次；无每事件/每UI全池新增扫描，无热new |
| DD-12 | full pipeline离线后同EmptySandDemo一个静置控制＋真实已准入负载：新包/policy/run/token、持续物理、双态沙同FrameToken可见性、CURRENT/FAULT/STALE正确；设备观感在实际修复与独立无阻断后另卡 |

本轮的设计模型核对不是上述tester独立验证。可逻辑证明非负／次序，不等实际source/guard/preview全链已实现。目标2基础XComponent管线已完成的既有裁定不重开；目标3/4/5本故障及双态稳定／观感仍待实施与真实验证，不提前派PASS。

## 9. architect实施任务书

先补对应dry派生／pool cache／publication／renderer输入工程文档及T205资源/证书域，明确数值V2可执行开关／新run迁移和差异表，再转developer；designer本卡不改业务。仅新增exact70纯模块、pool tuple写屏障、view acc lifecycle／dirty-envelope resolver、commit-domain证书及publication字段；原Hot/M2/Field/preview实际写点逐个列hunk，不能仅改最后updateSandTop或pack。

工程须冻结RN70 encode/round操作图、error优先序、每kind写屏障次序与回滚、old cache镜像／pool13buffer所有者一致性、active count与guard支持域、渲染copy双入口及FieldOnly边界。数值cache是V2 policy而非T197精确优化默改，GPU payload/SHA/fixtures／source identity需带policy，旧GPU协议不认识V2则明确不支持，不自动生产迁移。

开发关闭边界为实现／编译／离线自测；独立tester按DD合同和原阈值复测，全链实际支持后才派同DEMO窄设备，之后effector观感/完整G5。若实测需要旧首writer因果，按§6最小单次冻结补证；不重复索要已到的20项bits、不再派“补接线”空修。不允许修改清单其它内容或滴答同名清单，失败原卡看评论、首归因回architect，manager不加核查门。
