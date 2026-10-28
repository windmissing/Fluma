# T214：exact70 派生量数值 V2 工程规格

2026-10-28，architect。原卡 `6ae168e3e4b0f11bdf26a135`「【T214·派生量数值V2工程化】architect：exact70 tuple屏障、publication域与版本开关」。消费 T207 `f2b3e83` 唯一方案 §9、T205 工程与资源/证书域、T202/T208 已到原值证据。本卡不使用真机、不实现业务、不重跑产品/模型测试。

## 1. 交付裁定、启用边界与版本组合

按 manager 裁定，可实施**显式新 run 的隔离 V2**，生产默认关闭。V1 原实现、原 bits、旧 raw 和原错误继续回放；不后台升级旧 live run。V2 的列 cache、dry 水量统计和包络取值可能改变准入、grant、后续轨迹，不能记为 `TX_EXACT_STRUCTURE_V1` 对 V1 的全轨迹等价。

| 维度 | V2 必须完整绑定的值 |
|---|---|
| cache | `P011_LIVE_MASS_EXACT70_V2` |
| physics policy | `P011_DRY_CACHE_EXACT_V2`，medium仍为 `DRY_AIR_V1`，不得只凭medium启用 |
| envelope | `P011_DRY_ENVELOPE_ORDERED_V2`，waterTopMeaning=`DRY_OCCUPIED_ENVELOPE_V2` |
| publication | `P011_DERIVED_PUBLICATION_V2` |
| checkpoint / migration | `P011_DERIVED_CHECKPOINT_V2` / `P011_DERIVED_MIGRATION_V2` |
| TX结构 | 同一numeric policy内才作结构等价；证书/fixture/configId必须带上述policy组合 |
| GPU | 现T137/T171/T191 ABI **不支持此V2**，configure前明确拒绝，不默默用CPU完成GPU请求 |

首版域：C=128，2≤N≤512，capacity=256，phase0/1，live mass有限且0<m≤256，总live≤N。非此域不偷偷扩大limb容量或截断。DRY床四库存必须finite且各≥0，悬沙U_H/U_L数值为0；negative bed、非零U、超capacity不是cache修复对象。V2保留实际DEGRADE/失败分支：若它产生非零U，则严格域失败并按原root范围回滚，**不吞悬沙、不强制LAND、不把它重记外源**。本方案不承诺所有旧合法run都可在V2成功。

当前20数组、24片重组与第57列 `−3×2^-54`、mask128/STALE 已到件。不得再派“补导出接线”；完整旧pool/首writer历史没采，不从cache猜该列为空。若以后专门回放旧故障，才按T207 §6补同token13pool/allocator/源账预算，不将其设本工程前置。

全程仅新XComponent+shader渲染，含前景与背景；禁止Canvas上屏/失败回退。失败为FAULT，或明确STALE上一有效**完整**新管线帧。T214不改renderer EPS/阈值，不夹cache/包络，不放宽材质域。

## 2. 文件、责任与接口

所有路径为拟新增/修改，本卡无业务修改。T211的TX core、T212的GPU桥在途；共享hunk按函数接力，不回退别人修改，不将数值V2混入它们的V1等价提交。

| 开发责任模块 | 拟新增文件 / 修改hunk |
|---|---|
| exact70纯核 | `entry/src/main/ets/physics/derived/P011Exact70.ets`：bit解码、70-limb编码/add/sub/round；无业务、无BigInt、无hot分配 |
| tuple/view | `P011DerivedViewV2.ets`、`P011DerivedTupleBarrierV2.ets`：固定成员计数、before/after、branch/undo/barrier，接T205 opaque handle和地址注册 |
| dry域 | `P011DryDomainV2.ets`：dirty envelope、支持域、dry water统计、最终public验证与证书 |
| commit/publication | `P011DerivedCommitV2.ets`、`P011DerivedCheckpointV2.ets`；`P011PublishedRenderState.ets`增加明确版本元数据；`WaterSandField.ets`按第5/6节接入 |
| 隔离facade/DEMO | `P051DerivedDemoV2.ets`、`P051DerivedFacadeV2.ets`：私有owner、新run开关、V2结果、只读publication；隔离page/build target。现 `P051EmptySandDemo.ets`/`EmptySandDemo.ets`的V1默认构造不变 |
| renderer消费 | `P013SceneController.ets`/`P013ScenePreparation.ets`的输入版本/同token gate；FrameAdapter/MaterialDataPreparation原strict谓词/四pass/F32图保留 |
| 独立验证 | `test/derived-v2/`，tester所有：DD01–12 oracle/trace/故障注入；不能引用生产exact70做独立RN oracle |

拟接口按ArkTS显式class/interface实现，禁止任意动态对象/类型绕过：

```text
Exact70.encodeMassBits(lo: number, hi: number, out: Uint16Array, offset: number): DerivedStatus
Exact70.addAt(acc: Uint16Array, bucketOffset: number, encoded: Uint16Array): DerivedStatus
Exact70.subtractAt(acc, bucketOffset, encoded): DerivedStatus
Exact70.roundAt(acc, bucketOffset, bitsScratch: DataView): DerivedStatus  // 成功时结果原bits写入scratch
DerivedViewV2.createCold(profile: DerivedProfileV2, owner: TxOwnerHandle): DerivedViewHandle
DerivedViewV2.fork(parent: DerivedViewHandle, target: TxViewHandle): DerivedViewHandle
DerivedTupleBarrierV2.begin(view, cut: TxCutIdentity, slotA: number, slotB: number): DerivedMutationToken
DerivedTupleBarrierV2.finishActual(view, token, originalStatus: number): DerivedStatus
DerivedViewV2.rollback(view, savepoint: TxSavepoint): void
DryDomainV2.flushBeforeRead(view, readPurpose: DryReadPurpose): DerivedStatus
DryDomainV2.verifyPublicBoundary(view, boundary: DerivedBoundaryIdentity): DerivedCertificateV2
DerivedCommitV2.prepareInactive(view, proposed: DerivedCommitIdentity): DerivedPreparedCommit
DerivedCommitV2.commitPrepared(prepared, originalReceipt: TxOriginalReceipt): void
DerivedFacadeV2.copyPublishedRenderStateInto(out: DerivedPublishedSlotV2): DerivedCopyStatus
DerivedFacadeV2.copyPublishedSceneStateInto(out: DerivedPublishedSlotV2): DerivedCopyStatus
DerivedCheckpointV2.exportCold(owner): DerivedCheckpointV2
DerivedCheckpointV2.restoreCold(candidate, checkpoint): DerivedRunResultV2
```

handle/token私有构造，带owner/view/branchGeneration、root/cut、单调content serial和slot generation；before两个slot必须distinct，unused=-1。finish核对token仍当前、实际slot/ID/generation，无重入writer/外部callback/query；一个token只能finish/abort一次。禁止调用方传名义质量q当after tuple。原检查先按原位置执行；原writer失败时保留其原code/stage/ordinal，derived模块只恢复其自身原域，不用新错误覆盖先发生原错误。

V2 facade私有持有 WaterSandField/Hot/新derived storage，外部只得命令与复制完成的只读slot，不得返回owner raw arrays。现 WaterSandField 有公开waterTop/suspH/L，现DEMO有公开field；**不能仅加readonly注解就宣无alias**。隔离V2 model不得把该field实例暴露给page/plugin；所有可达setter/callback列入writer registry，输入cold深复制。未知writer、未受控alias或调用扩展未登记时拒V2配置/中止candidate，不能下一UI帧扫描修补。调试注入单独faultPlan，不能进入生产默认。

T205原有handle/storage/undo接口优先复用，数值policy新域扩充到该owner；没有T211核心时也可先在隔离whole-bank adapter实现同一合同，真实额外generation/bank内存另记，不能假称已共享省掉。

## 3. RN70可执行操作图

每bucket `(phase*C+column)` 的70个U16为little-index整数A，值为A×2^-1074。不得将整个尾数/指数用JS32位移位表示，不用生产BigInt、字符串、parseFloat或第三方任意精度库。

1. cold预建8B ArrayBuffer+DataView，按显式little-endian读取原mass `lo/hi`。sign=`hi>>>31`，E=`(hi>>>20)&2047`，fractionHi=`hi&0xfffff`。live的负数（含−0）、±0、NaN/Inf、>256或非法state/phase/列均拒；codec数学primitive可对±0给0，但**不能据此接受live zero**。inactive旧mass残值不参与归约，不清理它的原bits。
2. 尾数拆四个U16：`lo&65535, lo>>>16, fractionHi&65535, floor(fractionHi/65536)`；normal在第4字加16表示隐藏bit52。subnormal不加隐藏位，shift=0；normal shift=E−1。encode scratch70字先清零，按 `j=floor(shift/16), r=shift%16` 将四字乘2^r并进位写入最多五字。临时数用精确整数Number与floor/mod65536，不用有符号溢出移位；超70字失败，不截高字。
3. add逐limb `t=old+encoded+carry`，t≤131071，写t%65536、carry=floor(t/65536)；末carry不为0报OVERFLOW。subtract逐limb显式borrow，t<0加65536并borrow=1，最后borrow报NEGATIVE。都只改私有active patch，错误按undo原word恢复，不能留下half减旧。
4. round先从高字找bitLength b。A=0输出原bits全0的+0；0<A<2^52直接组subnormal fraction。否则s=max(0,b−53)，抽取M=A>>s的53bit整数；s=0无丢弃位。s>0读取guard bit(s−1)与其下所有sticky；guard且(sticky或M奇数)则M++。若M==2^53，则M/=2、s++。E=s+1，fraction=M−2^52，拆lo/hi用DataView组normal。E非法/非finite拒绝，不返回Infinity/max finite。
5. 所有中间整数在本域≤2^53可精确表示；round的M最大舍入临时2^53本身也精确。禁止F32/FTZ替代。cold环境probe需验证raw subnormal读写及原Number RN算术保持；失败是NUMERIC_ENVIRONMENT_UNSUPPORTED detail，不默吞最小次正规。

70×16=1120bits覆盖实数<2^46；本域总mass≤512×256=2^17，远小于该宽度。codec不据“还有空高字”扩大物理支持域。RN(非负exact和)≥0，空bucket=count0且A0才给+0；count>0但A0、count0但A非0均为损坏，不用EPS定义空列。普通更新期间count不得负/超过N，所有bucket count之和对应liveCount。

原pMass/床/位置/velocity表达式继续原F64图。acc准确表达**实际stored masses**，不证明它们每次merge/减余量都逐位守恒；原H/预算/drift/1e-9审核继续执行，不加质量修正尾差。RN70独立用例必须覆盖原设计DD02的normal/subnormal、tie-even、跨binade、−0与非法live输入；T207的3807模型例不是产品codec测试。

## 4. 实际tuple屏障、cache替换和回滚

旧tuple为 `(slot,id,slotGeneration,state,phase,floor(actualX),massLo,massHi)`；新tuple从writer完成后的真实slot再读。state1/2为live；state0不编码。两个旧slot/两个新slot最多四次remove/add，最多四个changed bucket。

```text
mark原rollback域 → 捕获旧tuple并验证成员身份
→ T205 key barrier摘旧节点（如key将变）
→ 原authority writer/F64图（V2禁执行旧cache +=/−=）
→ 读actual新tuple并验证支持域
→ 按旧tuple顺序remove全体，再按新tuple顺序add全体
→ 更新bucketCount，round全部changed buckets到唯一cacheH/L
→ 置dryEnvelope/数值audit/predictor/publication证书dirty
→ 按actual keys修T205树 → 关闭mutation token → 开放下一原读取
```

acc在inMutation锁内原地受undo保护，不要求4套bucket staging；现四个70-limb scratch可复用encode/round工作。不向guard/audit/birth/observer暴露减旧尚未加新的中间状态。scratch是非语义工作区，下一次明确清/覆写；A/counter/cache/dirty/header/generation才是逐bits回滚域。

V2的唯一cache owner在对应DerivedView；canonical pool、patchBefore/After/Scratch、field mirror、frozen/predictor snapshot通过同view版本复制/lease绑定，不各自再增减。发布patch将完整pool+相应derived view一起切换，不能13buffers已新、A仍旧。bulk clone/import用完整库复制或显式一次build，普通publish/rollback不得再从pool fold构造“差不多数值”。

| 实际writer类 | before/after与必须替换处 |
|---|---|
| birth/SPAWN/LIFT | 原free slot旧state0→实际质量/phase/floorX/live；每个真实birth一次。cache列取actualX，不能仅用plan.column；nextId/free/事件顺序不变 |
| partial/full LAND/DEPOSIT | remove旧stored mass、add实际RN remainder或无live；原`cache-=granted`在V2禁用。granted==oldmass时原0/退役规则原样 |
| DEGRADE/remove | remove真实live成员，实际state0；field悬沙写原样，strict dry若非零则候选失败，不能让cache模块清U |
| MERGE | 两旧tuple删除、actual survivor插入；mass=RN(ma+mb)、质心floor变化均真实读取。旧“同列cache不变”注释仅适V1，V2不能沿用 |
| CROSS MOVE/WAIT | MOVE按old/new actual floor双列更新；WAIT若仍同floor/mass/phase，A无变化。不能仅凭kind认定WAIT不变；保持原x边界表达式 |
| ordinary motion | 当前Hot/Field要求不跨column；若相同floor/mass/phase/live则不动A，pY/vx/vy改变仍使content/predictor/publication失效。未来允许跨floor必须先登记同一屏障 |
| phase/state/slot复用 | phase或active membership变化都更新；退役/复用发新slotGeneration。source/ID变化虽不改和，也必须改变身份/cert依赖 |

未知writer不得配置为V2；若仅有明确cold入口，可在修改前取得独占candidate，完整校验后一次build。**不允许先无屏障推进、待renderer取图再rebuild**。

### 4.1 λ、seed、preview、undo

同root连续seed/row继承当前A，不每beginSeed扫描N；λ与preview fork继承父view的A/count/dirty/证书身份。固定8view+一个独立audit bank；实际存活数/深度沿T205图输出并核容量，无hidden第9个私有branch。原成功source是独立已封存根，之后step失败只退stepBefore，不能退到源前。

WORD_UNDO用T205每写前存旧word的nested mark规则，增加width=2的U16地址描述（可用oldLo存低16bit，但恢复时只写该limb，不损坏相邻字）；F64 cache/包络/header按原lo/hi bits保存。child commit记录仍供parent rollback；child/λ失败逆序到对应mark。源账/预算/事件page/field和派生必须同一rollback域。普通undo不调用round/rebuild：数学相同不等于旧原bits。

branch/content控制serial单调递增，不随数据rollback倒退；业务derivedRevision/field/source/原预算按其savepoint恢复。恢复后的新控制serial使迟到证书失效；不会把旧publication证书贴到新branch。WHOLE_BANK首阶段可按原入口复制派生persistent arrays，真实COPY_DERIVED_BYTES/Pcopy计费；不每seed复制8套整bank。

8view是**同时存活的数值状态**预算，不是每个旧snapshot对象都各得一套A。冷计划固定槽角色为：当前work、public入口、λ/preview入口、Hot frozen、patchBefore、patchScratch/After、motion/planner入口、journalBefore。source与step入口不并存同一未闭合root；兄弟preview顺序执行。canonical/patchAfter可以在publish时转移同一数值bank所有权，patchBefore/frozen只在构造性相同状态时共享不可变lease；任何后续writer先fork/保留undo，不能边共享边写。原backup.particleState/particleBackup/其它13buffer拷贝必须在复制时绑定对应immutable state lease，不凭名称/短hash猜相同。journal长期持有单独槽直到原journal边界释放。开发输出实际lease引用图与峰值；无法证明8槽的实现须cold增加viewSlots并更新所有内存/深度式，不得漏算第9套、每seed重建或新增公开资源拒绝。外部cold checkpoint自持副本、projection固定副本不占用这8个运行槽，另列内存。

## 5. 逐文件hunk与writer覆盖账

[T214 manifest](analysis-T214-exact70-engineering-manifest-v1.json) 指纹20文件，列245个静态写入/批量边界review行；包含read-call的候选行，**不是自动穷尽alias证明**。开发build须逐项归入SUPPORTED/EXPLICIT_BULK/V1_ONLY/REJECT_V2，新增writer同时登记；缺一不准启该组合。

| 当前文件/函数hunk | V2接法与失败/隔离合同 |
|---|---|
| ParticleHot `editPoolHot/mergePairHot` | 第4节每个真实birth/merge/deposit/degrade/cross包屏障；在原precheck后开始；旧cache加减只V1执行 |
| ParticleHot `preparePatchHot/publishPatchHot/finishPatchHot` | before/scratch/after绑定同policy派生view，failed prepare恢复scratch域；publish/finish错误带derived sidecar，不只copy13buffer |
| ParticleHot `copyPoolHot/copyPoolDiagnosticHot/beginStepHot/rollbackStepHot` | copy/isolation/原binding校验不删；frozen/patch banks派生版本一一配对；rollback恢复A原words，begin不偷偷build每seed |
| ParticleHot `applyParticlePlanIntoBody/auditParticleHot` | motion屏障及content失效；原slot-order fold/预算审核、sink/tail/finish切点保留。新增derived verifier只public最终边界调用 |
| ParticleField `allocate/retainAfterDeposit/remove/mergePair/edit/applyMotions` | 旧非Hot路径同屏障或在V2配置明确不可达；不能仅Hot实现然后legacy fallback无屏障 |
| ParticleField `copySnapshot/restoreSnapshot/restore/publish/transferHotState` | 内部same-policy copies/undo带对应A身份；外部import必须cold candidate校验；不以snapshot原cache为可信V2派生 |
| ParticleField `reset/invert180/bindHotTransactionStorage` | reset实际live/allocator语义保留，A/count置0；底层invert若未来获明确支持须按实际坐标/库存bulk candidate重建；当前上层dry invert仍拒，不借此开放。纯视觉旋转不动authority，事务三bank别名账不重复分配 |
| M2Sink `applyInto` | 原reserve/guard/stock→prepare→publish→SINK_ROW audit→close/event次序；替换直接dry waterTop写为同view resolver，flush后方可下一读；非零U在新strict域报真故障 |
| M2Field `beginDryBatchBody/finishDryBatchBody/seedBody` | scope身份继承public source，Hot finish不是sourceSuccess；page暂存与根提交边界分开。λ失败恢复A/cache/预算/源前态 |
| M2Field `beginCollectBody/apply/rollback/reset` | A/B frozen来源同policy，preview/authoritative分view；保留原consistency，rollback派生也恢复 |
| M2Projection `write/capture/consistent` | 原plan/pool投影之外，V2比较policy、A/count/cache与8个field数组；在相同逻辑cut按列flush后捕获。先各自核branch/lease身份，再比语义内容，不强求preview与authoritative的branch/content控制serial相同；旧pool11N投影不足证明派生相同 |
| WaterSandField `initializeDryParticleDemo/initialize/reset` | V1 API默认V1；V2独立工厂cold preflight C/N/capacity/route/writer registry后建空A/基准水0，全部准备好才新run切换；失败不先破坏旧owner |
| WaterSandField `injectDryGestureSand` | 原λ/prepare admission/backup/finishDryBatch序列后、callback-free source tail前做一次最终V2验证及inactive publication准备；失败不commit admission/epoch，不伪成功 |
| WaterSandField `stepWithTiltTrace/recordEndpointCommit` | queued source在stepBefore前的原边界保留；预计算新commit/ledger identity并完成derived/domain+封存后才可切render authority。endpoint采样/编码异常为capture故障，不回滚已完成物理commit |
| WaterSandField `updateSandTop/dryReceiverCapacity/getWaterVolume/getTotals/getDryDiagnostics` | V2只通过dirty resolver取得same-rev cache/包络；getWaterVolume为dry无真实水语义+0，非残差max；wet分支原样；capacity/guard读前flush |
| WaterSandField `saveBackup/restoreBackup/saveMotionSnapshot/restoreMotionSnapshot/previewChannelPlansAt/planning scalars` | 为每对应pool/field备份配A/count/dirty/identity，preview finally恢复；旧restoreBackup末尾updateSandTop在V2不当作可替代原bits恢复 |
| WaterSandField shadow/planner/correction/field channel写点 | 相同原床F64图，写前登记field dirty；shadow S/envelope均接同policy resolver。原source/eta/water等wet-only路径V2拒mode，不能让其直接waterTop±q |
| WaterSandField `invert180/restoreFromSnapshot/configure*Route` | V2冷candidate，见第8节矩阵；实际invert旧表达式可能越域则明确失败，不夹x。route切wet/clearwater/未登记writer不得在V2 live owner上原地继续 |
| WaterSandField `copyStateTo/copyPublishedRenderStateInto/copyPublishedSceneStateInto` | V2读已sealed bank，不调用会updateSandTop的旧copyStateTo。两个入口同版本gate；旧V1复制实现留原历史语义 |
| `P011PublishedRenderState`/scene controller/preparation | 新policy/domain证书与完整field/pool/token同份；在pack前核受控slot的版本、owner、binding和seal。严格SOURCE/F32/MATERIAL谓词不删 |
| `P051EmptySandDemo`/page | 新隔离V2model/facade，page只命令/完整snapshot，不暴露field/数组alias；现V1默认路径及touch/dt/source身份保留，不默认切生产 |
| T212 GPU桥/config | V2 cache/policy不在旧wire身份/fixture合同内，native configure前显式UNSUPPORTED detail；不只给configId追加字符串冒兼容证明 |

projection新增固定存储：每个scratch+8 projection slot补 `2*C*70 U16 + 2*C U32 + C U8 + 2*C F64 cache + 8*C F64 field + 32 F64 header`，field为waterTop/bh/bl/mh/ml/suspH/suspL/surface。A/count/cache/field逐word比原bits；dirty在匹配cut flush后应全清。header保留各自身份并按branch映射验证，branch/content serial不作为跨branch必须相等的数据。共 `9*(4CL+89C+256)` B，C128/L70为427392B。它们**额外于**exact70的8view+audit，不能声称旧projection自动含A；仅替换为T205真实immutable view lease并证明其lifetime时可免此副本，build ledger标明实际选择。原projection比较plan/pool的工作仍在。

## 6. 包络、public验证与同commit封存

### 6.1 读屏障与严格域

同view的原左结合图固定为：

```text
bottom = bh
mix = RN(bh+bl)
top = RN(RN(RN(bh+bl)+mh)+ml)
P = RN(cacheH+cacheL)
envelope = RN(RN(RN(top+suspH)+suspL)+P)
```

V2只从同一pool/field revision派生；床改动和tuple改动都标列dirty。下一capacity/guard/audit/whole-field/观察读取前按原列序flush；可控局部读取只刷新依赖列，全场读刷新所有dirty。raw publication/failure capture是读取原值，不触发修复flush。每写field都登记dirty；未知writer不能沿用old envelope cert。

flush核床各finite非负、U==0、cache与当前A round的已维护证书有效、envelope有限且≤capacity。由非负及RN单调得bottom≤mix≤top≤envelope；不能把失败变成`max(top,water)`或EPS判断。普通RN cache更新不重扫pool。dry的water统计返回medium定义无水+0，initial/expected/freeWaterTotal同policy；occupiedVolume另统计包络。wet/clearwater具有真实水库存，不得走此函数。

### 6.2 Public derived verifier

每成功public source/成功substep的最终接受candidate边界，从完整**authority pool**在独立audit bank建A/count；核结构/allocator/ID/phase/domain，再逐limb/count比当前view，再逐cache比RN(A)原bits，再校所有列严格包络/床/源预算身份。一次 `O(70N+140C)` 新验证，不放每SINK_ROW/事件/UI；原Hot各logical audit仍原slot fold及原阈值。可共用原末尾scan的读取，但新增70-limb操作实计。

boundary ID含root/attempt/substep/source kind；同一内容/同一接受边界不重复新增verify。若校验后又有authority写入，证书失效：该候选尚未最终完成，需在其真正最后边界验证并计实际次数，不能强凑“每frame1次”。失败λ/discard preview也统计实际已耗工作，不伪造为successful verify。cold build/import一次与public verify是不同scope，不拿build结果替代最终读回。

证书至少带owner/view/branch、run/stateEpoch/proposed physicsCommitId、pool content serial、fieldRevision/derivedRevision、policy/envelope/publication、writer/buffer registry/算法图版本、sourceEpoch+ledger key、budget版本、validation boundary和certificateId。T205原audit/predictor/结构证书read-set增加policy/A/count/strict-domain/envelope，任何tuple/床/U/params/route/restore写立即失效；同rev异内容/外部alias不因revision整数相同而复用。

### 6.3 准备与原子切换

当前 `publishRenderState`只增render commit counter、复制接口在读时调用copyStateTo；这不足V2。增加**同步双publication bank**，与T205 async owner/有限lease行为分开：

1. 私有candidate完成原事务写和原必需审核，预计算next commit/stateEpoch/原ledger key与source成功增量，检查安全整数及输出容量。
2. 完成最终derived verifier/strict域，在inactive bank复制完整field+pool+cache+域header与proposed源账，所有fallible检查在此完成。inactive不会被consumer读。
3. 单owner、无callback/await的tail提交原admission/预算/源epoch或step成功身份，然后一次切active bank/authority及元数据。后续observer只能读这一份。切换前错误回原candidate mark；先前已成功source不受后续step失败影响。
4. endpoint sampler/日志/编码/IO若在物理commit后失败，报告capture INCOMPLETE，保留物理成功。native draw/swap成功才有CURRENT，sourceSuccess不代表CURRENT。

source inactive bank里的proposed source state是准备值，直到原commit tail才公开，不能把prepare admission当成功。当前P011ExternalAdmissions仅保留O(1)summary/lastRecord，raw同步外发；V2复制该实际summary及其完整身份，不新建历史队列，不额外要求所有历史raw齐才可恢复一个本已可校验的冷checkpoint。rawHistoryStatus/exportFailures仍真实保留，不将summary冒完整逐笔raw。STEP的原commit key必须与最终endpoint键相同，不先发一个ledgerKey=null的V2完整证书再事后补身份。RESET/RESTORE/INVERT/ROUTE_CHANGE分别保留原kind/stateEpoch规则及新数值policy。

双bank不是向外借出的owner lease：V2 facade把active sealed内容复制到调用方预分配slot，单线程复制期间无callback/reentry；slot与内部bank不alias，完成后sealed身份写入最后。旧字段布局可保留schema4的array payload，但**V2仅在带`P011_DERIVED_PUBLICATION_V2`的外层slot中有效**，不能把裸payload当V1可回放记录。slot原始arrays只在受控render adapter内部可达，UI/plugin不获得可写引用；导出给外部是独立cold checkpoint/复制，不沿用live证书。

两个copy入口都先核slot publication revision、容量/array identity、同一个active bank generation、run/commit/policy、源与预算身份，再完整复制，最后核generation未变。copy不做round/build/updateSandTop，不增加physics/derived/source序号；FieldOnly不能从另一个live field补数组。renderer重复重绘同sealed slot不扫authority pool、不建A/修cache，正常C+N packet打包成本仍实计。

对于untrusted导入、直接bit/pointer注入或外部可写alias，不能仅凭旧证书放行：在明确cold import/最终public验证重算/比较；已seal的render slot若其受控所有权破坏，拒该slot而非修复。独立测试必须绕过barrier注入并证明该路径不能获得可用seal/证书；不能用readonly类型掩盖实际逃逸。内部bank无写入口且consumer复制无重入是证书成立的前提，不是防任意内存篡改的安全声明。

## 7. 错误合同与原bits冻结

新增namespace `P011_DERIVED_FAILURE_V2`，V2 status分配1001 INPUT_INVALID、1002 ACC_NEGATIVE、1003 ACC_OVERFLOW、1004 CERT_STALE、1005 DRY_DOMAIN_INFEASIBLE、1006 PUBLICATION_DOMAIN_MISMATCH；这些不是旧WaterSandErrorCode，也不是renderer code100。V1枚举和旧错误不变。

`DerivedFaultV2`字段：namespace/code/reason/detail、原root/cut/stage/ordinal、owner/view/branch/policy、slot/bucket、raw old/new tuple bits、limb/borrow/carry、originalHotCode/originalFieldCode（只有原检查真实失败才非null）、outerTransportStatus、rollbackDomain/rolledBack、firstFaultFrozen。Hot/M2可用V2内部非零abort status沿原fail/rollback路径传播，V2 coordinator必须读sidecar；外层M2_STEP_FAILED/TILT_PLANNING_BLOCKED运输状态不能冒原首因。

V2 facade返回 `DerivedStepResultV2{ok, originalError, derivedFault, physicalCommit, captureStatus}`；源回执保留request/actual admitted、sourceEpoch和原transactionId，新失败原因明确namespace。不得为兼容旧bool UI而把所有derived fault硬记code5/100/99。renderer本身原SOURCE/F32/native故障仍按原真实码记录，与owner domain失败分开。

错误优先序：原输入/身份/绑定等检查到原位置执行；若已失败，保持其first code；受控writer开始后出现新的V2非法tuple/借位/溢出，立即停止该candidate并回原rollback域，不再运行下一数值读来“争一个原错误”；末尾derived/domain失败阻止candidate成功；publication版本故障不运行renderer；物理commit后render失败不逆改源账。原V1首错完全不被V2校验重写。

首次V2故障在冷预算内一次冻结**未刷新**field/pool/cache/A/header/相关tuple与父scope；失败路径禁止调copyStateTo/updateSandTop“整理后再抓”。详细完整raw导出受预先预算限制，truncation/未采字段标INCOMPLETE，不能称完整pool可回放。原T202 20项bits保持原档；新增故障证据不覆盖旧run。

## 8. initialize / invert / restore / route / 迁移矩阵

| 入口 | 可执行合同 |
|---|---|
| fresh V2 initialize/reset | 新config明确numeric policy+isolated flag+新run；域/writer/资源先cold校验，再空pool建A、dry水基准0、原源账重置语义。当前V1构造不默启 |
| ordinary同V2 rollback | 原word还原，不rebuild；同policy/owner/branch校验后恢复cache/A/count/dirty，重新发控制serial使旧证书失效 |
| 同V2 checkpoint restore | `P011_DERIVED_CHECKPOINT_V2`外层：完整schema4 authority payload、policy/config、A/count/cache原bits、源/预算/身份与可回放标志。仅静止的root边界导出/导入，未闭合attempt/ticket/patch不序列化成可续跑成功态。独立candidate校验全pool+重建audit逐word匹配，源账/summary身份及当前守恒可校验后原子RESTORE，新stateEpoch；失败旧owner不变 |
| 裸V1/raw/schema3/4 restore | 保持V1回放，不带V2证书、不默认重算/升级。源/预算/pool缺失的旧现场不能执行live数值迁移 |
| 显式V1→V2冷迁移 | 本稿只交candidate接口/资源/差异记录，**不启迁移开关**。以后显式请求且完整authority+真实无水/源预算证明齐备，才单独candidate build+严格校验、记before/after cache/envelope bits、policy/stateEpoch与原源累计，不覆盖原checkpoint；不要求再索已到的20项 |
| physical invert | 当前WaterSandField dry入口已拒 `DRY_INVERT_UNSUPPORTED`，V2首版继续拒，不借数值revision开放新物理操作。未来另行明确支持后，才可在candidate执行原F64坐标/床图、按actual new floor一次bulk build；越界/strict不可行则拒，不夹坐标/床。纯视觉旋转无authority写，不动A/commit |
| route OFF/fieldOnly | 不沿粒子证书。只有显式创建V2 zero-pool owner、证明无live且床/U/source域齐备才支持其empty derived；否则保持独立V1 FieldOnly。不能route OFF时隐藏live沙再发V2 |
| wet / clearwater B | V2组合拒绝，在新的正确policy owner/run上调用各自原路线。真水W/eta不得解释为dry occupied envelope；目标7实现不归本卡 |
| GPU backend | configure前检查numeric policy，不支持V2即显式失败；新GPU算法/ABI将来升版后独立验证，不借V1真机PASS |
| unknown route/writer/外部alias | cold明确拒V2。无法证明write-set的hook不提供“自动rebuild下一帧”模式 |

checkpoint外层版本是数值回放合同，旧endpoint/commit/elapsed/particles字段协议不因此改名。V2隔离capture manifest登记numeric policy/checkpoint revision；旧endpoint checker不认识新增回放件就标unsupported，不能删除policy伪装V1。禁止对旧raw原地补V2元数据。新run隔离研发授权不等于生产发布或旧run迁移授权。

## 9. T205资源与工作包络扩展

本稿与 [T214 manifest](analysis-T214-exact70-engineering-manifest-v1.json) **extends** T205，保留其原manifest不改指纹，供T211/T212在途消费。manifest逐项列26个新数组模板，以下是payload而非RSS：

| 增量 | 字节/规则 |
|---|---|
| exact70基础/view | acc2C×70U16=35840；count2C U32=1024；dirty C U8=128；4×70U16 scratch=560；bits8B=8；合37560B，DataView不再计backing |
| 8view+独立audit | 300480+37560=338040B，**仅基础**，不是完整owner峰值 |
| tuple/身份/地址 | 每view四行before/after的slot/id/generation/state/phase/column/massLo/Hi=120B；9bank header32F64+registry16U32；双pub header32F64。此分项4352B，基础加它342392B |
| slotGeneration | 独立V2需每view F64[N]，N192/512分别12288/32768B（8view）。实际同T205同view registry共享且证明identity一致时才免重复计，不凭同名扣掉 |
| projection完整比较 | 第5节whole-copy方案额外427392B（含实际cache和8个field数组）；若immutable lease方案证明完备则按实际bank/lease账替换，不能两个都不计 |
| word undo | 原24B/record，新增derived最多4次70-limb增减、4count/4cache/4dirty/32header/2generation的宽松326条/mutation（不是已测值）；nested depth保守乘。cert.valid/原field/树/源/预算日志由T205另计 |
| 双publication | 同步V2原子封存新增两份**完整**field/pool/schema metadata，含pool13每份55N+16C，field每个实际数组+源账/预算版本、header与旧current/context恢复重叠；T205实际已有同bank才可alias不重复计 |
| cold checkpoint/migration | 持有完整旧/新owner、旧/新publication和checkpoint，before/after两phase cache4C F64、envelope2C F64、header256共6400B只是迁移记录；新owner完整资源和caller持有copies额外累计 |
| runtime/native/捕获 | interned版本字符串/cold class/header/error字段、诊断封存/UTF8最坏转义、native/FBO/STALE/恢复重叠与T205归档全部保留；未测容量不填0 |

undo term保守 `depth*(326*U_mass + 2C*wholeEnvelopeFlushes + 非tuple额外header写 + bulkAccPersistentWordWrites)`，原框架域另外累计，不能将同一个cache/field地址日志双收或漏收。semantic派生复制最小persistent payload=36992B/view（acc/count/dirty），再加实际选择header/generation；非语义scratch不必克隆，COPY_DERIVED_BYTES仍按实记。whole-bank/cold迁移每次copy计Pcopy，不能只报新AVL字节数。

完整峰值为 `T205所有unique live backing + 本增量 + projection实际选择 + 未在原undo预留中的新增日志 + 未已有双publication + cold checkpoint/迁移重叠 + runtime/native/driver/caller`。同alias只算一次。fixed undo/output/work配置未选、native/runtime未测时NOT_TOTALLED；外部调用者无限保留cold snapshot仍为UNBOUNDED_EXTERNAL，不能顺手启T205的有限lease/新准入API。

新增计数：DERIVED_BUILD_SLOT、TUPLE_REMOVE/ADD、LIMB_VISIT、ROUND_GUARD_STICKY、CACHE_WRITE、DIRTY_ENVELOPE_FLUSH、DOMAIN_CHECK_COLUMN、PUBLIC_DERIVED_VERIFY、DERIVED_CERT_HIT/MISS、COPY_DERIVED_BYTES、DERIVED_UNDO_WORDS/RESTORES、MIGRATION，挂原source/λ/seed/row/candidate/public/frame父scope。

`W_V2 = W_T205(policyV2) + O(70*U_mass + (Bbuild+Jverify)*70*(N+2C) + Pcopy*2C*70 + Dflush) + 实际domain/header/capture/publication工作`。所有未提交λ/preview mutation计U_mass，public source与substep各计最终边界，不是UI帧一次；原所有audit及field C²残余保留。UI不加derived full-pool verifier，正常已sealed copy/particle pack O(C+N)按实次数计。服务率/峰值native容量/整链耗时PENDING_DEVICE_CALIBRATION，不能拿模型毫秒宣UI或稳定性达标。

## 10. 开发、独立DD01–12和同DEMO设备接力

这是manager后续派工范围，本卡不越权建/关远端任务；纯工程交付无需新增manager验收门。

| 接力 | 输入/具体交付 |
|---|---|
| developer | 本稿+T207；独立exact70纯核/tuple/view+全部writer矩阵、V2 facade与双publication、explicit新run构建开关，完整cold allocation/undo/work账；保持T211/T212 V1合同，编译/不涉机自测后可交付，不将真机做开发关卡 |
| tester离线独立 | DD01保原T202/T208证据；DD02独立任意精度encode/RN oracle；DD03历史无关集合；DD04全tuple writer；DD05条件F64图等价与真实轨迹差异分表；DD06嵌套rollback/源成功前缀；DD07strict床/U/包络域；DD08同commit/版本/alias；DD09严格renderer+F32/GL失败；DD10生命周期/route/migration隔离；DD11实际计数/分配上界；DD12设备执行规格 |
| tester真机 | 对应实现/独立无阻断后，同EmptySandDemo场景的明确V2新包/source+shader/native SHA、policy/run身份；一个静置控制+真实已准入负载，记录同commit/同FrameToken的场床与live粒子两态可见性、源账/物理持续性和CURRENT/FAULT/STALE、N192/512与真实service/峰值；打真机标签，串行设备 |
| effector真机 | 在该可复现V2包上独立观感：手持流向、触摸准入/跟手/停止沉积、两态沙同token、持续完整shader场景；产品结果按真实证据，不把源账/数值模型当可见性/稳定演示PASS |

DD05明确比较两类：相同request/grant/branch输入下原pMass/床/ID/预算writer的F64图条件等价；V2因cache/水统计/严格域改变而实际准入或trajectory不同的数值policy结果。第二类也要逐笔源/预算自洽、守恒/原阈值和明确允许差异，不能把任意缺陷都归允许差异。

故障注入固定在：旧tuple捕获后、删旧/加新之间、末limb borrow/carry、缓存publish前、原pool publish后、nested child提交后parent失败、最终verifier后再写、inactive完整copy前后、源成功后step失败、restore/route异policy、scene pack/F32/native swap。先于当前错误的原first code不得被派生覆盖，failure capture不得调用refresh。所有注入只在隔离faultPlan，新生产默认关闭。

新模块/新域未实施的预测试RED登记NOT_IMPLEMENTED，不是产品缺陷；实施后真实测试失败先architect归因。工程问题先改此稿再developer；算法矛盾交designer，用例争议交manager，缺证提出精确补证。manager读报告登记，不重跑测试、不另加人工门。

## 11. 本轮文档自查与交付范围

交付物为本稿及资源/源码writer指纹manifest；冻结RN70、tuple/branch/undo、严格域和原成功边界、版本/不可达路径、完整增量成本和接力范围。源指纹与T207设计输入一致，原20数组不再索补。仅做只读源码/原报告检查、文档算术及diff核对，未实现/编译/跑产品/用真机，不记产品PASS。只更新自己T214状态并回原卡，不维护清单其它行/队列或滴答同名清单。
