# T203：单沙明水 B 完整子步工程 V1

2026-10-28，architect；原卡 `6ae152f0e4b0f11bdf2418e3`。唯一算法依据 T196 `d80f669` / `CW_B_STEP_DESIGN_V1`，消费其§1–12、retro、既存模型证据及T188/T189 A交付。目标7/P3排前六之后，compute目标6仍最高调度优先。**ENGINEERING_READY，B尚未实施/独立验证/设备测试。** 不使用真机、不跑模型或产品测试、不写业务代码。

机器可读逐数组、逐槽位、状态/flags/身份及两个规模预算见 `analysis-T203-clear-water-B-engineering-manifest-v1.json`。该文件是设计分配规格，不冒实际allocation ledger。T107本篇的B补充优先；A源事务布局/语义仍沿现行源码，C及restore保持未实现。

## 1. 固定边界及工程所有权

独立 `physics/clearwater/` owner，不接旧WaterSandField/M2/Hot/GPU/renderer热文件。B仅水邻接流、已有phase0粒子分段运动/跨列/接触/LAND及单沙床流；不birth/merge/lift/U回退、不搜角、不λ重试，物理candidate V=1。replay是证据重放，不是第二次轨迹候选。acceptedAngle单位度，沿±30；N=128、容量C=256、K≤512，主档192/512。成功source在B失败时保留，帧第二子步失败保留第一子步；dt0不消费源队列或改库存/成功ID/预算/事件。

模块/文件与未来开发卡所有权如下，公共hunk由Engine集成owner统一维护，不能两张卡同时覆写同一文件：

| 文件（均以P011ClearWater前缀） | 唯一责任与禁止越界 |
|---|---|
| `BTypes.ets`（新增） | B版本、冷BConfig、StepReason、可复用Out、数值manifest常量；不重编号A枚举 |
| `ExactMass.ets`（新增） | 70×U16整数加/减/比较/编码、两个scratch及8B冷view；不改canonical P定义 |
| `Events.ets`（新增） | 双event40/lineage3/arena header，预留与封存、replay字段校验；不重释source event24 |
| `Guard.ets`（新增） | 固定budget六列数组/32标量、3边guard scratch、实际RN复核；只调用既有数值Into核，不构造Map/Set |
| `Hydraulics.ets`（新增） | 水/床各N−1边一次奇偶序；水不扣固体预算，床stock图按T196 |
| `Particles.ets`（新增） | 递增ID活动链一轮，事件积分/plane tag/等待/LAND；不调用旧seed/选对/全池搜索 |
| `Step.ets`（新增） | 全阶段调度、最终fold/几何收尾、B audit与线性replay、计数scope；不自行source提交 |
| `State.ets`（既有hunk） | 可选B origin/wait/header、copy/reset/allocations；A-only模式布局及行为不变 |
| `Source.ets`（既有hunk） | 正成功birth的origin/wait原子扩展及来源区间更新；保持key/payload/row20/56/24、回执与A算术不变 |
| `Engine.ets`（既有hunk） | busy/五bank所有权、cold attach、advance提交/失败Out、snapshotB与资源/计数；公共A入口集成唯一owner |

先开发BTypes/ExactMass/Events，再Guard/Hydraulics/Particles/Step，最后同一个集成owner完成State/Source/Engine三个hunk；独立测试拥有自己的oracle/报告，不复用被测轨迹/acc实现作参考。不新建运行页面或显示功能。新增模块必须用实际ArkTS编译探针检查，不能以未被页面import为由漏编。

## 2. 冷配置、入口与错误接口

保持A `ClearWaterRunConfig`、`enqueueSourceInto`、`commitQueuedSourcesInto`、`stopSourceGeneration`、`advanceSubstepInto(dt,acceptedAngle,out)`和snapshot V1签名/原义。**旧ClearWaterStatus实际0..22共23个值**（T196/卡片“旧22个”按此勘误），全部保留，不插号或改PARTIAL_CAPACITY=22。

新增可选冷接口，建议精确签名：

```ts
attachBInto(config: ClearWaterBConfig, out: ClearWaterBAttachResult): ClearWaterStatus
copyLastBEventsInto(out: ClearWaterBEvidenceBuffers): ClearWaterStatus
copyBFrameCountersInto(out: ClearWaterBFrameCounters): ClearWaterStatus
snapshotB(): ClearWaterSnapshotB                 // 冷独立副本
restoreBInto(snapshot: ClearWaterSnapshotB): ClearWaterStatus // 始终UNSUPPORTED_FEATURE
```

attach只允许同一个活A owner空闲且尚未装B，cold全预检/分配/来源检查完才安装B runtime；不是从外部snapshot恢复。失败保留A状态/队列/时钟/源序号，无半装B；已有B重配需新run/明确cold生命周期，不热改变e/caps。构造时要启B可在reset成功后调用同一attach逻辑，不强改A原构造者调用。BConfig包含revision/configRev、step/event/lineage/priority/contact/acc版本、显式gModel/dragAir/maxAvalancheFlux、e/eventCapacity/lineageCapacity、calibrationStatus=PENDING_CALIBRATION；dragWet/settlingSpeed/kappaW/massPackage沿A读取。所有数值finite/nonnegative，e整数≥2；配置资源至少E=eK+2(N−1)、lineage≥2E；主档e8，变化须新版manifest/真实资源预算，不用临时加大绕过失败。

扩展ClearWaterStepResult只新增`stepRevision,stepReason,stage,ordinal,lastBStepSerial,sourceCommitAtEntry,acceptedAngleDeg,arenaSerial,eventCount,lineageCount,maxResidual,allEdgesWithinH`（预建class字段），原status/stepped/lastAcceptedCommit/failedAttempt/implementation仍可读。B开启后implementation=CW_B_STEP_V1；未装B的正dt仍保持A NOT_IMPLEMENTED_B_COUPLING/UNSUPPORTED_FEATURE原行为。dt0只填调用者Out，不改内部计数/serial或清旧arena。

`CW_B_STEP_REASON_V1`独立于A status，逐号在manifest：非法dt/角→INVALID_VALUE；无B→UNSUPPORTED_FEATURE；重入→REENTRANT；token耗尽→TOKEN_EXHAUSTED；arena/lineage/每粒event cap→EVENT_FULL＋精确reason；有限域/进位/减法/时间无进展→NUMERIC_UNREPRESENTABLE或输入非法；最终audit/guard/replay不一致→AUDIT_FAILED。NUMERIC_NO_EFFECT/CAPACITY_NO_GRANT/BUDGET_NO_GRANT为合法局部ZERO，status仍OK；不要仅见一个reason就判整步失败。

所有失败都保留raw coarse status/独立reason/阶段/首次ordinal及attempt；不拿最后一次cleanup覆盖首错。attempt可在正dt实际开始后增长，成功stepSerial/physicsCommit仅提交时增长；angle身份记录run/config/inputGeneration/geometry/clockWindow/attempt与传入角原值，不在B内部读sensor或替caller决定角。busy覆盖整个candidate/replay/提交，所有公开写入口共用同一busy。

## 3. 五bank、源成功与提交协议

五份不alias state：既有canonical/sourceBefore/stepBefore/work，加独立replay；每份的pool/七场/auditMass独立，B启用时各加`originSourceCommit F64[K]`、`waitReason U8[K]`和16槽B header。只有canonical为权威；scratch/auditMass不作为可恢复authority。字段/形状/length/byteOffset/backing身份在cold绑定，热入口检测detached/替换/alias，拒绝不继续写。

sourceBefore与stepBefore不能复用：源边界Q≤64逐源事务提交；stepBefore必须在最近成功source之后复制。source复制必须携带已有B metadata，不能每次生新work清掉旧origin/wait。只读snapshot V1可省B字段，但复制到B已开启bank时若来源缺B metadata须拒绝，不猜0补齐；reset是单独清零路径。

### 3.1 source birth origin和cold attach

origin指成功sourceCommit，不是sourceId、requestSeq或water成功序号。冷attach从同owner原source event24抽取沙行 `{firstParticleId,particleCount,transactionId}`；跳过water，验证正U32 ID连续区间、无重叠/倒序、transactionId安全正整数；16D字节固定区间表顺序写，最多4096行。每个live ID二分查区间，O(D+KlogD)，缺项/歧义拒attach，不逐粒扫完整D，不接用户修改过的snapshot。表cold分配后保留本run复用，reset清有效count，不每步重建。

后续source正成功：在work（尚未publish）按本事务新ID区间一次填写origin=newSourceCommit、wait=NONE，并在剩余区间槽stage本事务范围；可在现有一次rebuild遍历融合，或一次有界K扫描，不能B次全池找slot。metadata/source event/区间表容量与token均在publish前检查；与原source候选同audit边界成功才发布state、dedup/receipt/event24及区间count。water与幂等重放不新填origin、不追加沙区间。源失败保持此前canonical metadata/有效区间count；不用新回调在权威写到一半时再抛错。

full LAND清该slot的origin/wait供空槽复用，但事件/lineage保留退役前原origin；pId旧非活值沿A规则，不改nextId。partial保持同origin、ID、slot。reset成功新run将五份B metadata、arena有效代、serial/预算/计数和来源表有效区间一起清理，A初始库存/queue/dedup规则照旧；stopSourceGeneration只清原待提交/时钟屏障，不抹已成功物理/来源历史。

### 3.2 正dt唯一candidate时序

1. 校验dt/角/owner/配置/caps/token，标记busy；canonical原A audit一次，失败不写库存。复制到stepBefore与work，预算与事件候选有效count清0，生成attempt元数据。
2. 从stepBefore按完整live链建立P精确acc和固定Bcap；冻结入口guard d/bound；水边各一次，成功后冻结etaMedium/wetMask。
3. 轨迹按递增ID活动链一轮，处理前存next；全LAND O(1)摘链/退slot。每粒主循环最多e−1条，收尾预留1条；原时间不能推进且无合法拓扑变化立即STEP_TIME_NO_PROGRESS。
4. 床边各一次；在既有末尾ID fold中融合一次几何收尾CONTACT，重建P/colParticleMassH同序及S/eta/R，B几何/来源/预算/全局guard及原A audit核一次。
5. replay从stepBefore复制一次，按全局event ordinal核旧bits/应用actual新值，重新预算/容量/局部guard与ID链结果；最后按同序fold和原A audit一次。比较authority、pool、free/prev/next/head/tail/nextId、origin/wait、预算/事件与work逐位一致。
6. 最后预检commit存储/源序号未变/封存容量且不再回调。复制work→canonical、更新成功physicsCommit/stepSerial、切封存arena及header可见代；把这整个无回调同步区间作为逻辑原子提交。若实现不能保证预检后copy不抛，需明确恢复stepBefore及旧封存头的保护，不能半写后返回失败。不得恢复到sourceBefore撤销已成功源。

异常/失败：丢弃本candidate；canonical/已封存arena/成功计数不变；Out保留失败身份与lastAcceptedCommit；finally解除busy。私有scratch可被试写，不能把其事件外发或冒恢复后的权威。三次whole audit是入口/work-final/replay-final的上界（成功时3，早退更少），不得在每event内部调用whole audit/rebuild/copy。

## 4. 核接口、数值顺序与热资源

各纯核统一返回coarse status并填预建Out/原因槽，不返回新对象/数组。关键接口责任如下：

| 模块接口 | 输入/输出约束 |
|---|---|
| ExactMass.encodeInto(value, target, limbOffset) | 非负finite binary64（−0作为精确0但不改权威原bits）；写70×U16，失败不截高位 |
| ExactMass.add/subtract/compare(array, offset, operand, operandOffset) | 原地fixed70字宽；减法负值/进位溢出FAIL；compare从高limb到低，结果−1/0/1 |
| ExactMass.buildFromStateInto / fitsPatchInto | 按stored mass前后分别encode后加减，不先用RN差分代替精确差；pair预检≤2列、fixedBcap，拒绝不能污染acc有效值 |
| Guard.beginStepInto / admitPatchInto / replayBudgetInto | total=RN(300dt)、local=RN(28dt)，total一次、两端local各一次；水/纯运动0debit，局部ZERO无材料写；guard实际RN边校验 |
| Hydraulics.advanceWaterInto / advanceBedInto | 同accepted angle；水头公式/五界/隔床界按T196§4，床top-first RN stock图按§7，不调顺序或加循环 |
| Particles.advanceInto / reconcileGeometryAndFoldInto | T196§6弦段积分、床/墙/列边/水面优先；当前床/冻结介质面分开；无ID查找全池扫描、无无限零时间plane循环 |
| Events.reserveInto / commitRowInto / replayInto | 写前容量检查；row/lineage/局部patch验证完才推进有效count；不持有可被后续candidate改写的借出view |

encode按2^-1074单位：次正规fraction，正规mantissa53bit左移exponentField−1。不能用JS 32位位运算承载53/1120bit整数，也不要求ArkTS BigInt；把lo/hi拆16bit limb并按16bit位移/进位，单次加法最大131071，保持Number精确整数。仅本辅助核用整数，不改变原RN64库存运算。主域E≤(K+2)C<2^18，70limb可容<2^46；异常/更大域明确拒绝，不静默截断。两行scratch在encode/capacity阶段复用，跨列patch可先逐列非写入预检，再统一更新acc，不额外申请每event临时bank。

Bcap[c]=max(exact(C),exact(S0)+exact(W0)+exactPoolSum0)，固定整个candidate；其作用是保守预检/原态不恶化，最终仍执行A严格 `R=RN(RN(S+W)+P_IDfold)<=C`。最终P不能取exact accumulator四舍五入值，也不能用每event的P+=/−=质量替代末尾IDfold。只重建派生缓存，不修/钳权威库存。

现A.rebuild会按ID链求P，但不会同步pool.colParticleMassH；B最终fold需要**同一逐ID加法结果同时写P和colParticleMassH**（L始终0），再原A audit独立归约。初次、work末、replay末按各自scope核；普通source.rebuild保持A原图，不因B hunk改A-only缓存结果。源A正沙born的O(B)缓存加法与rebuild的最终同序结果保持原行为。

guard只用最多3边固定numeric Into核。cold从256B budgetScalars切出residual/influence/edgeId/bound各3个F64 view（offset128/152/176/200），不额外backing、不热subarray；其它16标量与4保留槽见manifest。writer入口实际d的max(H,abs(d))与全步入口bound分开；先名义闭式q，再按真实bh/mh/CanonicalS复核，不以k*q顶替actual边，不二分/逐ULP找“好q”。NO_EFFECT与WAIT需写的CONTACT运动不同，不能借ZERO偷改state/y/vy。

空气/湿速度、弦段λ、剩余时间、同刻tie、右受阻nextDown(c+1)/左受阻c、接受右c+1/左nextDown(c)全部按T196执行。nextDown用cold8B bit view，边界严格，无1e−9几何魔数。床后CONTACT收尾只校正当前S几何、不二次LAND/重积分，计入每粒e和arena。候选参数只显式配置PENDING_CALIBRATION，不把9.8/.01/24写成默认合格值。

## 5. 事件40、lineage3与封存读取

布局逐槽见manifest，kind冻结MOVE1/MEDIUM2/CROSS3/CONTACT4/LAND5/BED6/WATER7。槽19..26在前四kind为remainingTime0/1,hEvent,lambda,planeType,planeCoordinate,medium0/1；LAND/BED为床前后库存。BED槽9/10为fromM/fromB，其它粒子行是massBefore/After；water保留无关槽0，不伪造粒子。BED/WATER slot=-1、particleId/origin=0；真实ID必须正U32，origin安全正整数。

flags以F64存确切U32整数：old/new pState各2位、old/new wait各3位、old/new medium各2位、reason8位，22..31位保留0；field位置见manifest。wait=NONE0/BED1/SIDE_CAPACITY2/SIDE_BUDGET3/SIDE_HIGH_BED4；medium AIR0/WET1。纯接触/等待状态变化另记CONTACT；材料q0或NO_EFFECT不产生成功LAND/WATER/BED。event ordinal从1起连续，lineageStart从0起，写入有效prefix；未用字段0，不把缺测诊断误填0。

每kind的usedSlots/unusedSlots及40-bit mask已列manifest；LAND单列床接收库存用23..26（19..22清0），BED用全19..26。planeType固定NONE0/BED1/WALL2/COLUMN3/WATER_SURFACE4。有限域中间量必须在min/max之前检查，不能让min的有限结果吞掉上游乘法溢出。

lineage3每条为particleId/originSourceCommit/edgeKind；CONTINUE1、TO_BED2、TERMINATE3。MOVE/MEDIUM/CROSS/CONTACT一条CONTINUE；partial LAND为CONTINUE+TO_BED，full LAND为TO_BED+TERMINATE；BED/WATER0条，至多2/event。不创建新source/birth/merge身份。

双particleSummary各K×16槽按T196：eventStartOrdinal为首ordinal、eventCount为总数，**不等所有事件连续**；床后收尾CONTACT可在其它粒子/床事件之后。replay按全局ordinal解码，禁止用start/count切连续slice替代完整事件。

双arena一candidate一sealed，失败不切换有效sealedSerial；后续candidate可能复用旧inactive存储。外部只允许copyLastBEventsInto写到调用者已分配且不alias的目标，带expected owner/run/arenaSerial校验；不得返回裸view伪不可变快照。source在B之后另成功时，canonical.physicsCommit可前进而lastB sealed仍指旧B commit；输出同时区分current source/physics与sealed B commit，不能伪造同帧一致。C尚未实现，不生成FrameToken或渲染成功回执。

## 6. 资源清单、snapshot与计量

manifest每个数组列name/type/length/bytes/lifetime/owner，cold view另列0额外backing。五份state header16F64=640B、两arena header64F64=1024B、全链counter48F64=384B，合为T196预留2048B；不得再暗增不入账的每粒临时对象、JS可增长数组或Map/Set。私有8B bit scratch和140个U16 scratch均已计。

| N128/e8 | K192 | K512 |
|---|---:|---:|
| A既有payload（按K折算） | 2,721,824 | 2,802,784 |
| E / 每bank lineage cap | 1790 / 3580 | 4350 / 8700 |
| 新增B热payload | 1,443,296 | 3,443,936 |
| B含保留cold origin表峰值 | 1,508,832 | 3,509,472 |
| A+B含cold表 | 4,230,656 | 6,312,256 |
| 一份snapshotB额外payload | 3,319,456 | 4,284,576 |

K512持一份snapshotB时payload合计10,596,832B，仍不含对象/字串/view header/对齐/栈/driver/C publication/native/GL。这是分配工程预算，不是实际RSS PASS；调用者多份snapshot逐份加。snapshotB从空闲owner复制A原snapshot的全authority/queue/dedup/event24/时钟和代，再增加canonical origin/wait、16槽Bheader、一份sealed arena/header/event40/lineage3、sealed local remain/used及entry d/bound四N向量、budgetScalars与BConfig。snapshot V1仍只A视图，装B时明确B字段被省略，不能作为B恢复包；A-only snapshot行为不变。

restoreBInto始终UNSUPPORTED_FEATURE，不接旧wet无W schema，不从sourceId猜origin，不恢复半份数组。未来完整restore另做版本/长度/alias/位型/来源/链/预算guard核验及全体原子替换；本轮cold attach不是restore。snapshotB仅冷端读取，不在每event或UI每帧生成，也不把其成本藏出计时。

48项counter包括enqueue/源边界/Q/birth、子步成功/失败、候选、三类audit、活动链/fold/replay、七kind/lineage、70limb核、copy字节/ID摘链/整cache重建及阻挡、merge/index/λ监测；最后三类B固定0作为禁止路径探针，不代表其它旧路线未调用。N/K/F/e/实际L/Q/B/V、首次失败stage/ordinal随同scope输出。可加只读begin/end frame计量边界，**不得隐式替调用者消耗source队列/推进dt**；未绑定完整frame scope时标PARTIAL_SCOPE，不把单step计数冒frame aggregate。

arena header保留16个命名计时/clock状态槽，单调时钟由cold owner注入；未知/抛异常/不测为NaN，冷JSON转null＋原因，不填0。wholeSubstep含内部各阶段，replay含其自身audit/copy；报告分项inclusive/exclusive，不相加重叠p50。source enqueue/源边界、两子步/失败及cold export显式归账；多次源边界G按实际和累加，不以单次Q≤64掩盖整帧G。

B时间上界沿T196：source边界O(Q(N+K)+Σbirth)且显式计enqueue O(D+Q+N+K)，step O(N+K+70*(N+A+L+2(N−1)))，e固定时O(N+K)，V=1，F≤2。replay/fold/audit固定全遍历照计，不用“活动链一轮”隐藏它们；禁止每event whole audit/回到链头找ID/逐birth重建/copy。T196模型仅设计算术，设备速度/不卡死仍未测，不发明性能百分比门。

## 7. 可执行接力（本会话不建卡/改他人状态）

**实现developer，不使用真机**：按§1文件所有权顺序落模块与三个A集成hunk；保持原A未启B路径字节/回执/key/−0 payload/源序号/幂等/queue64/dedup4096无驱逐，source event24不重释，B事件另arena。实际allocation ledger逐数组核manifest，ArkTS显式编译全部模块，离线自测覆盖失败注点/定容/热图。开发以实现/编译/离线自测交付，不等真机/manager审批；无merge、V=1、原预算顺序或A语义做不到，先回designer，不自行替换算法。

**独立tester，不使用真机**：B-01..B-14逐项来自T196§11，不用model/同被测ExactMass或轨迹核当独立oracle。B01原A15项/key/payload/源账/dt0；B02 source成功后各阶段故障和第二子步失败原子性；B03水五界/actualRN/NO_EFFECT/总水；B04空气湿/出水/同刻tie/剩余时间；B05整包跨列/预算量子/高床与侧等待；B06 CONTACT材料ZERO/收尾预留/严格几何；B07 partial/full LAND/ID/free/source/W不变；B08床stock/实际guard/预算；B09 adversarial质量顺序与双cache位型；B10独立精确整数oracle验70limb/次正规/进退位/Bcap；B11 origin区间、水沙穿插/arena/ID/旧bits篡改；B12整frame调用乘数和失败成本；B13 A/B snapshot隔离/reset/restore未实现；B14 C/页面/切源/透明同帧未实施。checker独立重放不能只调用产品replay；同时核五bank非alias、未用field清零/flags保留位和同token原始F64。未实现预测试RED记NOT_IMPLEMENTED，失败先architect。

**后续设备effector/tester，真机标签/串行**：等实际B实现与独立无阻断，再按N128/K192/512的浅/深/干/满、入出水、倾角、源成功后失败及事件上限做窄功能/完整事务成本；包/source/配置/原输入/计数/收益身份归档，原始库存水/沙都要审核。不把A15/15或本模型当设备结论。可见入水/床抬面/水向低头流与跟手效果只有C实际实现及新XComponent+shader渲染/控制器齐后才派，不让B卡等待C才关闭，不用Canvas演示补缺。

所有任务prompt必须包含：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人状态有限例外），不改其它行/队列或滴答同名清单，不回退共享修改；不使用真机或真机标签/串行真实前置；所有前景背景仅新XComponent+shader，禁Canvas上屏/回退，故障FAULT或显式STALE上一有效新管线帧；成功结束待验收，FAIL/BLOCKED/关键缺证看评论并回原卡，manager不新增验收门。

本轮只交工程/manifest，B/C/restore没有借文档变成已实现；未用设备、未重跑测试、未宣产品PASS。本人行结束待验收。
