# P-011 M1生产热接口契约 v1：定容缓冲与数值代际patch句柄

日期：2026-10-24。责任：architect。任务：6adc91c3…d612，P5。状态：**DESIGN_READY / PENDING_MANAGER_ACCEPTANCE / NOT_IMPLEMENTED / NOT_ZERO_ALLOCATION_VERIFIED**。

本卡按用户明确交办补严格零分配热接口，不采用有界分配降级方案B、不变更D1完成标准。纯设计零代码，不跑真机，不关闭D1、不授权D2。主契约依据 [路线C架构rev2](architecture-P011-dual-field-particle-v1.md) §4.4/§13/§16、[R9裁定](decision-P011-dual-end-refund-adapter-v1.md)、[developer R3提案](proposal-P011-D1-R3-buffer-contract-v1.md)、[R9交付](dev-report-P011-D1-R9-v1.md)与priority.md 19:05验收。预算/guard/质量/路由/endpoint冻结语义均不变；只改变生产调用侧的存储、身份和结果传递方式。

## 1. 现状与热/冷边界

当前`P011ParticleField.ets`已有canonical SoA、scratch池和before/after复用；`snapshot()`仍返回独立副本；prepare每次new独立patch对象；collect建数组/请求对象、sort闭包和JSON身份；apply返回结果数组；prototype有field/budget checkpoint、Set与事件对象。manager复跑全链探针exit=2，报告60处潜在显式分配站点；这不是每步真实分配次数，亦不因普通单元全绿而达标。

新增生产API后，旧对象/字符串API保留为 **COLD_DIAGNOSTIC_COMPAT**：独立snapshot、对象patch、字符串requestKey、JSON导出与抛Error语义保持，供旧单测/诊断使用；它们不能从生产HOT调用图到达。冷wrapper可调用新热核再物化对象；热核不得反调冷wrapper。不是删除旧公开API或用复用同一对象悄改其独立副本语义。

热返回码为独立接口状态，不重编号既有物理errorCode：`OK=0,NOT_CONFIGURED=1,INVALID_INPUT=2,INVALID_BUFFER=3,STALE_HANDLE=4,INVALID_PATCH_STATE=5,PATCH_BUSY=6,PLAN_OVERFLOW=7,MOTION_OVERFLOW=8,JOURNAL_OVERFLOW=9,LINEAGE_OVERFLOW=10,TICKET_OVERFLOW=11,PARTICLE_PLAN_MISMATCH=12,ATOMIC_AUDIT_FAILED=13,TOKEN_EXHAUSTED=14,DUPLICATE_REQUEST=15,IDENTITY_MISMATCH=16,UNKNOWN_PARTICLE=17,STEP_BUSY=18`。prepare的返回域特例是token或0，错误码只写work.lastStatus；其它Hot/Into入口返回上述status，失败不可依赖异常对象表达。上层将status映射到既有失败attempt/error策略，映射表在冷初始化固定。

零分配计量边界：完成`configureHot/warmUpHot`后，从`beginStepHot`开始，含冻结输入、双collect、逐值核对、apply六事务、field/budget/guard/票据/patch/journal、正常结束及可预期失败/rollback，直到`finishStepHot`返回。禁止仅围refund或零授予deposit量测。初始化、显式重配Nmax/容量、脱离该栈的证据物化允许分配并独立计时；不能把每步都要做的计划/身份构造移到marker外就叫“冷操作”。正常step重置与每步输入填充也必须零分配。

本合同“零分配”指完整HOT可达源码没有创建堆对象/数组/闭包/字符串/容器/TypedArray视图的操作，并有长时运行证据；VM内部JIT/GC细节另外披露，不声称靠JS构造器trap即可证明ArkTS全堆零分配。可预期错误走数值状态码，不在HOT内new Error/throw字符串。

## 2. 容量与所有权

所有缓冲只在冷配置时创建。每个executor一个ownerId，canonical池由M1拥有；调用方（D1 harness或D2场）拥有workspace、输入、输出与导出页，初始化后绑定不可替换引用。live与shadow分别拥有完整独立workspace，不能共用一个scratch同时重放。

| 容量符号 | 本版规则 | 对应定容存储 |
|---|---|---|
| C | 物理列128，不升级 | 七数组+派生S+列在途H/L+field checkpoint |
| N | 同已校验params.nmax，候选512非冻结 | canonical/frozen/checkpoint/patch前后/scratch SoA、排序索引与motion |
| Q | requestCapacity，冷配置默认4096 | planA/planB请求行、result、票据sidecar、ordinal去重 |
| M | motionCapacity=N | planA/planB运动行，活粒每步最多一条 |
| E | eventCapacity，冷配置默认4096 | 数值事件行与birth/merge/action关联行 |
| G | guardEdgeCapacity=3 | 固定受影响边索引、residual/influence与结果 |
| P | activePatchCapacity=1 | 一份patchBefore、一份patchAfter、scratch池、patch动作区 |

Q/E默认承接原4096日志候选，不把它当可静默增长上限。多相spawn可产生N量级birth、merge；每笔先计所需行数及槽数，再进入写入。若本步所有事件超过E，明确返回JOURNAL_OVERFLOW、回滚并沿现有冻结/失败attempt路径处理，不截断、不覆盖未消费记录、不丢质量、不临时扩容。Q满报PLAN_OVERFLOW；M满报MOTION_OVERFLOW；票据/patch忙报对应数值错误。需要更大容量由冷configure与manager资源复核处理，不在热步重试扩容。

SoA snapshot字段沿主稿§13.3，含pMass/pX/pY/pVx/pVy、phase/source/state、id/sourceId、freeSlots/freeCount、nextId、liveCount、列质量H/L。完整TypedArray载荷=55N+16C字节（5个Float64粒子标量、3个Uint8、3个32位字段含freeSlots），不含wrapper/scalar/VM开销；N=512,C=128时30208B/份。canonical、scratch、frozen、step checkpoint、patch前/后共6份约181248B，**不是整个workspace内存**；还须加计划、事件、field/budget快照与live/shadow第二executor。导出实际byteLength合计，不用这个下界冒充RSS。

### 2.1 定容行格式

所有行使用预创建SoA，`count`只表示有效前缀；不push/slice/subarray/map/filter/Array.from。表内每个同名数组长度固定为该capacity：

| 缓冲 | 类型化列 |
|---|---|
| RequestBuffer(Q) | Uint8 kind/phase/sourceKind；Int32 column/targetColumn；Uint32 particleId/sourceId；Float64 substepId/ordinal/requested/totalWeight/predictedX/Y/Vx/Vy；Int32 mergeStart/mergeCount |
| MotionBuffer(N) | Uint32 particleId；Int32 column；Float64 x/y/vx/vy |
| ResultBuffer(Q) | Uint8 committed/disposition/released；Int32 status/guardIndex/eventBegin/eventCount；Float64 granted/remainder/budgetClipped/guardRejected/capacityClipped/poolClipped/massAfter |
| EventBuffer(E) | Uint8 kind/phase/sourceKind/flag/remainderDisposition；Uint32 particleId/sourceId；Int32 requestOrdinal/column/targetColumn/lineageStart/lineageCount；Float64 requested/granted/remainder/massBefore/massAfter/x/y/vx/vy/deltaS/budgetLocal/total/bottom |
| GuardBuffer(G) | Int32 edgeId；Float64 residual/influence；结果count/accepted/rejected/limitingEdge/allLegalBefore为预创建scalar字段 |
| BudgetBuffer(Q,C) | localUsed/remaining(C)、total标量、每票据source/target、reserved/refunded/closed、逐端sidecar(Q×2)、R9收据固定2端 |

kind/flag等枚举的数值表在冷配置固定，冷导出映射回原字符串（SPAWN/LIFT/LAND/CROSS_COLUMN/MERGE/DEGRADE/TO_SUSP等）；不改变字段语义。`mergeStart/count`引用定容Uint32 lineage arena，容量单列`lineageCapacity`（默认2E），先预留再写，耗尽按journal overflow整步回滚；不能每事件建mergedIds[]。事件行格式是数值物化源，冻结报告要求的guardPreview、bySource、byState等由同commit相关缓冲完整取数，不能省略。

result/guard/receipt等out对象本身在冷时预创建；调用只改scalar/已有数组元素。每次入口先校验count、长度、ArrayBuffer完整性与跨缓冲alias，尤其canonical/冻结/patchBefore/After不得同底层buffer；坏out在改canonical/预算前拒绝。方法返回的是primitive status或token，不返回新结构体。

## 3. 热接口签名与职责

以下为待实施的导出接口；`P011ParticleHotWorkspace/PlanBuffer/ResultBuffer/FieldBuffer/BudgetBuffer/EventBuffer`均是按§2一次构造的显式ArkTS类；成员readonly绑定数组，允许数值scalar更新，不使用any/动态属性字典。

```ts
// 冷入口：唯一允许分配workspace/TypedArray/适配器的阶段。
configureHot(config: P011ParticleHotConfig): P011ParticleHotWorkspace;
warmUpHot(work: P011ParticleHotWorkspace): number;

// caller-owned identity/view/out，方法只返回数值Status。
beginStepHot(work: P011ParticleHotWorkspace,
  input: P011ParticleHotStepInput): number;
copyStateToHot(out: P011ParticleSnapshot): number;
checkpointHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number;
collectParticlePlanInto(work: P011ParticleHotWorkspace,
  expectedStepEpoch: number, destination: P011ParticleHotPlanBuffer): number;
compareParticlePlansHot(a: P011ParticleHotPlanBuffer,
  b: P011ParticleHotPlanBuffer): number;
applyParticlePlanInto(work: P011ParticleHotWorkspace,
  expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer, sink: P011ParticleHotCommitSink,
  results: P011ParticleHotResultBuffer): number;
rollbackStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number;
finishStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number;

interface P011ParticleHotCommitSink {
  applyInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer,
    row: number, results: P011ParticleHotResultBuffer): number;
}
// sink由冷配置构造并绑定live或shadow field/budget；不是每步创建的闭包。
```

HotConfig精确包含 `cols,nmax,requestCapacity,motionCapacity,eventCapacity,lineageCapacity` 与校验后的params；nmax必须与池一致、motionCapacity=N、patchCapacity固定1、guardCapacity固定3；不支持热改params对象形状。HotStepInput为预创建scalar对象，字段 `runToken,substepId,attemptToken,dt,effectiveTiltRad,seed,paramsRevision,inputRevision`，以及对caller预分配FieldBuffer的只读绑定。FieldBuffer含waterTop/bh/bl/mh/ml/suspH/suspL/S、cols/capacity、sourceInjectedH/L/sourceEpoch与本次涉及的其它rollback标量；M1在beginStep复制到固定frozenField，不创建P011FrozenParticleView或临时函数闭包。

runToken由冷run登记器对runId字符串分配安全整数，一run一次；runId字符串保留在冷manifest。paramsRevision/inputRevision和dt等只拷scalar，validate不得临时建values[]。新run/参数切换可走冷配置，普通每步填同一input对象；输入对象被调用方以后复写不会改变work保存的冻结值。

PlanBuffer精确持有 `ownerId,runToken,stepEpoch,substepId,attemptToken,paramsRevision,inputRevision,seed,dt,effectiveTiltRad,requestCount,motionCount,totalRequested,valid,consumed`、§2请求/motion列与lineage区、可选数值fingerprint。ResultBuffer持有同一身份与resultCount、status；不以“new数组+返回值”收集结果。所有函数在本executor唯一同步线程执行，禁止重入/worker共享；多executor必须各自work。

### 3.1 Budget/guard热适配

```ts
interface P011ParticleHotBudgetSink {
  reserveInto(expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer, row: number,
    receipt: P011ParticleHotTicketReceipt): number;
  refundInto(ticketToken: number, amount: number,
    receipt: P011ParticleHotRefundReceipt): number;
  closeInto(ticketToken: number, committed: number,
    receipt: P011ParticleHotRefundReceipt): number;
}
guardAdmitInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, column: number,
  deltaSign: number, requested: number, out: P011ParticleHotGuardBuffer): number;
```

票据receipt预创建，仅含数字身份/余额/状态与固定2端数组，requestKey字符串由requestOrdinal关联替代。票据token同§4防重用规则，sidecar使用同一request数值身份；register不能每笔构造P011ParticleBudgetTicket对象或调用会动态存字符串的旧注册入口。旧R9公开冷adapter保留；新增numeric热入口复用R9逐端实际扣额、total仅一次、bottom0、close残差及拒绝零副作用算法。**不**因为API变了改成旧平分退款。

reserve仍 `l=min(r,localSource,localTarget), a=min(l,l*scale,totalRemaining)`，分母r仅在collect记一次；T3/T4计、T2仅rise、T1/T5/T6为0。guard用固定edgeId/residual/influence有效前缀，T2/T3单列±1影响两侧边、T4不改S不调guard；不得热造Map/constraint对象。可新增粒子专用`guardAdmitInto`，与现p011GuardConstraintsForDelta/p011GuardAdmit逐式同源、同受影响边/边界/截断/non-worsening/limitingEdge；旧函数本体与旧通道不改，Node冷参考对照证明所有有效边界输入一致。

## 4. 数值代际令牌：禁止复用对象句柄的ABA

### 4.1 唯一令牌与生命周期

热接口使用primitive **number令牌**，0为INVALID。模块级`nextLeaseToken`冷初始化为1，每次发放严格递增，最大`Number.MAX_SAFE_INTEGER`；无位运算打包、不使用Uint32截断、不取模、不循环复用。令牌跨本模块所有executor全局唯一；本owner只接收自己的当前令牌，错误owner因全局不重号而被拒。计数器是控制身份，**永远不随rollback/reset/snapshot恢复**；不同于物理pId/nextId，它们仍随事务回滚恢复。

```ts
preparePatchHot(work: P011ParticleHotWorkspace,
  plan: P011ParticleHotPlanBuffer, row: number,
  grantedHeavy: number, grantedLight: number, dispositionCode: number): number;
// 返回>0数值token，返回0失败；失败码写work.lastStatus，不new句柄/异常。
publishPatchHot(work: P011ParticleHotWorkspace, token: number): number;
restorePatchHot(work: P011ParticleHotWorkspace, token: number): number;
discardPatchHot(work: P011ParticleHotWorkspace, token: number): number;
finishPatchHot(work: P011ParticleHotWorkspace, token: number): number;
```

work内部保存 `activePatchToken,activePatchStepEpoch,activePatchRequestOrdinal,patchState`，全部为scalar。所有操作先要求token为有限正安全整数，等于当前activePatchToken且owner/stepEpoch吻合；旧令牌不能凭相同requestOrdinal或相同substepId重新获准。prepare必须先验证容量/输入，完成独立scratch及patchBefore/After后才发token；失败输出0、不修改canonical且不复用旧token。最多一个active patch，忙时prepare返回0+PATCH_BUSY，不覆盖其before/after。

step状态也显式检查：IDLE→begin成功为OPEN→finish成功或rollback完成为IDLE；OPEN时再次begin返回STEP_BUSY，不覆盖checkpoint或旧令牌。finish失败仍保持可rollback的OPEN态，不能先清checkpoint。rollback结束就使旧expectedStepEpoch失效，即使下一次begin尚未到来；epoch有效性由OPEN标志和数值共同判断。令牌仅当前进程模块生命周期有效，证据文件中的token不得恢复为可用凭据。

| 操作 | 前态 | 后态与令牌处理 |
|---|---|---|
| prepare | EMPTY | PREPARED，数值新token，before/after独立有效 |
| publish | PREPARED | PUBLISHED，只把after复制到canonical一次 |
| restore | PUBLISHED | 复制before到canonical后立即EMPTY、token作废 |
| discard | PREPARED | EMPTY、token作废，canonical未改 |
| finishPatch | PUBLISHED且字段/预算/journal原子提交已过审计 | EMPTY、token作废；不再次写canonical |
| rollbackStep/reset/reconfigure | 任意 | 废弃全部patch/票据/plan lease，token永不再有效 |

重复publish/restore/discard/finish、跨步调用、跨owner、已回滚句柄一律STALE_HANDLE或INVALID_PATCH_STATE，拒绝前后所有canonical/预算/日志/out均不变。restore后若需要重试，重新prepare取得新token，不能拿旧token再publish。

典型ABA反例：prepare得t=41，discard；再prepare得42；旧变量数值41仍是41，不会像同一返回对象被复写后变成42，故publish(41)必须拒绝。rollback后恢复同一物理substep重试也发43，并提高stepEpoch；允许原语义request重试，但不恢复旧patch/ticket资格。近MAX_SAFE_INTEGER时，发完最后合法值后进入TOKEN_EXHAUSTED，后续prepare在改写前拒绝并要求停止/重启受控运行；不wrap、不开“清零后重用”的例外。

这是防误用/陈旧引用的生命周期令牌，不是对不可信调用方的密码学授权。所有数值counter均有越界拒绝；runToken/stepEpoch/attemptToken不可用JSON或32位hash冒充。

### 4.2 计划、票据与out的别名规则

PlanBuffer为caller-owned持久对象，**其引用不是授权句柄**。调用方在begin成功后立即保存primitive `stepEpoch`，作为后续接口显式 `expectedStepEpoch` 参数；入口先要求expected值等于work当前epoch，再核对plan中的身份。旧plan缓冲即使被填新内容，也不能让保存的旧expected epoch授权新操作。禁止wrapper每次从已复写的plan/work重新读取epoch代替调用方保存值来“通过校验”。`beginStepHot`递增不回滚的stepEpoch；整步rollback再次begin须新epoch。票据同理：返回primitive ticketToken，receipt对象只是可变观测out，不能拿receipt引用里的后来ticketId冒充原票据。

同一缓冲的borrowed只读视图仅在当前lease内有效，不能跨下一次begin/finish或跨线程保存；需要持久证据时在§7冷边界物化独立副本。禁止用shared out对象或 `patchId`属性实现热patch凭据。冷对象patch API保留原独立对象语义，但不参与热token识别，冷热句柄不得互换。

## 5. 确定性双collect与数值request身份

request语义不改：`(runToken,substepId,kind,particleId或sourceId,column,targetColumn,ordinal)`；attemptToken/stepEpoch是执行lease，失败重试可有同一语义request但必须是新lease。ordinal在本次计划1..Q，按现deposit→crossing、各自particleId升序，再其它既定事务顺序赋值，不能因换缓冲改变交易顺序。原始计划物理pId/seed仍按原规则。

生产请求无需字符串hash。排序用预分配Int32 order[N]与显式原地迭代heapsort（比较pId，必要时slot固定打破平局），不用sort回调/临时数组；原本pId唯一仍要验证，不凭槽顺序当id顺序。双collect使用同frozenField/frozenPool、相同scalar身份，两份不同PlanBuffer；不能第二跑读取第一跑修改过的live数据。

去重采用ordinal索引的固定`seenEpoch[Q+1]`及对应数值身份列（Float64代际、安全整数），遇同ordinal重复立即报DUPLICATE_REQUEST；同ordinal不同内容报IDENTITY_MISMATCH。epoch不wrap；epoch冲突不能清表后接受旧lease。requestCount/motionCount/所有有效字段/lineage顺序逐值比较，NaN一律无效，±0按既有JSON比较相同的数值语义归一；可有定宽数值fingerprint作早拒，**hash相同不能替代全字段逐值核对**。不在HOT调用JSON.stringify、字符串拼接或.toString。

collect只写目的plan/scratch及请求统计，不扣真实预算、不调用真实CommitSink、不改变canonical、ID/空槽栈、shakeEnergy或随机状态。第二collect不二次加真实分母。apply只消费plan A已核对的有效前缀；全部结果写预分配ResultBuffer、guard字段指向固定out索引，不push新结果对象。不从grant后状态生成漏进分母的新请求。

## 6. checkpoint、事务发布、事件与失败

workspace在begin复制一次stepCheckpoint：完整池、七数组/派生S与源账本、预算local/total和R9 sidecar、已消费ordinal状态、所有数值计数与journal尾。每笔prepare复用patchBefore/After和scratch池，不new粒子池/snapshot。field/budget提交前用固定小区保存受影响列与票据的before-image；整步checkpoint是最终兜底，不能靠每笔new state。full snapshot与逐列delta两种均只可选在已分配存储内执行，接口不允许临时扩容。

发布过程沿主稿：预检全部容量→reserve→guard/退款→pool prepare→字段stage→pool publish/field写入→审计→事件复制→close票据→finish patch。粒子池、字段与日志同一同步栈，不在半状态调用外部observer。正质量尾数、分相来源、T3只减granted、T4整粒WAIT、T5只计一次、合并一阶矩均不变。

Birth/Merge/RepresentationAction不返回新对象：当前patch的定容action arena用count/start/index组织；合并链引用定容lineage。事务前算出需要的行数/槽数，准备阶段越界即拒绝；审计通过前事件仅在scratch页。消费者不可在publish前读取新事件或在rollback后保留借用视图。所有事件/结果记录包含numeric requestOrdinal、particleId/sourceId、stepEpoch/attemptToken，冷导出可完整恢复原事件字段。

允许保留固定多页导出队列，但每页必须有数值pageLease、有效count与owner；未消费页不能覆盖。满页按原journalOverflow义务报告失败，不静默丢账、不返回截断计划；不能为减少分配把事件降采样。事件负载与队列是有限内存，不意味着物理质量可被丢弃。端到端发生容量错误须整步rollback，沿主稿保留failed attempt并按现有错误策略冻结/交上层；不能后台扩容然后掩盖失败。

`rollbackStepHot`恢复canonical/field/source/budget/refund sidecar、请求消费状态与事件尾、result有效计数/成功标记；废弃本epoch所有patch/ticket/output lease。已耗费token/stepEpoch/owner控制counter不恢复，物理nextId/freeSlots/随机物理状态恢复。失败attempt使用独立预分配scalar槽记录错误码/序号/stage，不new Error；导出后才能清槽。rollback是内存复制/标量恢复，不重新register字符串票据、不new Set(appliedKeys)。

若失败发生在out/参数校验之前，必须零副作用；若已publish后才失败，则先恢复所有已改状态再返回错误。finishStep必须确认无active patch/未close票据、计划与result/事件对账通过才发布commit；双finish和借旧epoch提交拒绝。warmup/reset不是“重新创建work再算通过”的热路径逃口。

## 7. 冷导出、兼容与对接边界

冷导出接口 `exportParticleEvidence(work, committedPageToken)` 可分配独立对象/数组/字符串；只读已封存页，不能回读下一子步live数组；导出前后pageLease验证防stale。字符串requestKey按主稿原tuple格式生成（runToken查冷manifest的runId），原JSON字段、事件/endpoint命名、source身份、schema v7与失败attempt含义保持。热路径字符串替换成numeric内部身份，不等于改变冻结对外字段。

每步必须执行的证据捕获是HOT写数值arena，不以“以后导出”省略逐笔数据；冷物化/序列化明确计 `evidenceExportMs/exportAllocatedBytes/pageBacklog`，总帧时也须披露，不能只展示marker内耗时来隐藏用户可见GC。正式实时设备采集须证明导出带宽/队列容量够用；不足如实BLOCKED，不跳过超限事件。

旧API与新HOT对照用同输入/seed/参数，逐值比较候选请求、grant/clipping、三桶质量、逐粒ID/来源/运动、guard与四账、事件次序/字段、失败后恢复状态；不是只比总质量。对象身份不同是允许的API物化差异，物理与证据含义不变。兼容旧单测不代表生产调用图没有分配，两个结论分别报告。

## 8. 文件责任与完整分配审计表

| 文件/责任 | 需收口的HOT可达项 | 禁止的“局部通过”推断 |
|---|---|---|
| `P011ParticleField.ets`（M1，或新增同目录Hot模块） | beginStep/copy/checkpoint/collect/record/finish/compare/apply/prepare/edit/merge/publish/restore/rollback/事件填充 | 仅copyStateTo或warmup后TypedArray=0不等于全链0 |
| `P011ParticlePrototype.ets`（D1桩） | 固定field view、field/budget checkpoint、reserve/guard/apply/executePlan与结果/事件 | 不能保持prototype热new对象却只审M1一个类 |
| `P011ParticleRefundAdapter.ets` | numeric票据入口、sidecar、fixed out；R9公式/拒绝不变 | adapter局部10,000次证据不能代替全M1 |
| guard专用热适配模块 | 固定边数组/精确闭式out，不动旧guard/退款函数 | 不以新近似guard绕过旧参考等价 |
| Node/ArkTS测试（developer/tester） | 全可达图审计、HOT状态/句柄/容量/回滚/兼容证明 | 不删旧CONS/BUD/LIFE/SRC或缩窄探针后宣称达标 |
| `WaterSandField.ets`真实生产接线 | **仍归D2**，把上述热view/预算/field绑定接现有存储、live/shadow同源 | D1不能越界把真实route/λ/η/页面写入本轮 |

显式分配审计至少包含：new及构造调用、对象/数组字面量、spread/destructure副本、push增长、slice/subarray/map/filter/concat/Array.from、sort回调/捕获闭包、临时Map/Set与其clear后重新new、JSON/string拼接/template/toString、每步Error构造、动态属性shape新增、返回新对象、getter/适配器的间接分配。白名单只能列冷构造/冷导出并给不可达证据，不按函数名笼统排除。固定TypedArray.set/fill和标量赋值允许；不创建新view。计数按整调用图报告，原“60处”逐项注明已迁冷/已替代/仍残留及证据路径，不能把审计未涵盖节点从分母删掉。

## 9. 验收要求与遥测（本稿未执行）

1. **静态完整性**：HOT根及所有可达方法列表/调用边/显式分配点清单，含sink/guard/rollback/失败分支；不只扫描两个源文件顶层。原探针的60处逐项可追溯；尚有可达显式分配则NOT_ZERO_ALLOCATION、不得把exit=2改成PASS。
2. **动态固定身份**：至少覆盖现600s/36,000子步全事务沙盒及双collect/完整apply；构造/容器/序列化trap+数组buffer引用/长度身份前后快照都检查。spawn尾数、部分LAND、pool满merge/degrade、跨界、guard退款、合法多次refund、计划重试和journal溢出都实际走到。trap捕不到对象字面量/引擎分配，必须与AST及堆证据合用。
3. **句柄反例**：旧token数值保留；discard/finish/restore后再prepare；rollback同substep重试；reset/reconfigure；跨owner；重复publish/finish；busy prepare；MAX_SAFE_INTEGER耗尽；每项拒绝后状态逐值不变。显式保留旧out引用并复用out，证明旧primitive token不会被其后来字段改写。
4. **物理/日志**：热/冷路径同输入逐笔等价，全部现33+11例与其它既有回归保留；CONS-03正尾数/质量零才释放，预算/guard/来源/容量/一阶矩和R9负对照不变。Q/E/lineage/ticket各容量边界和发布后审计失败均rollback全量；失败不伪造成功journal。
5. **性能诚实性**：只在上述功能/分配条件满足后报告CPU P50/P95/GC/内存；Node证据不代真机ArkTS帧时。生产零分配不能仅由RSS平稳推断，GC后平稳也可能有大量短命分配。既有ohosTest175处错误独立披露，不能称全测试HAP通过。

新增只读数值遥测 `hotContractVersion=1,ownerId,stepEpoch,attemptToken,activePatchToken,activePatchState,requestCount,motionCount,resultCount,eventCount,lineageCount,requestCapacity,eventCapacity,hotStatus,errorStageCode,rollbackCount,staleHandleRejectCount,patchBusyCount,overflowCount,tokenExhausted,bufferIdentityChanged,legacyColdCallCount,pageBacklog`。HOT不构造日志字符串；性能/导出字段另在冷汇总。bufferIdentityChanged和HOT内legacyColdCallCount必须0；错误计数不通过反复回滚清零隐去。

## 10. 交接索引与生效边界

| 主稿旧接口/义务 | 本附件生产替代 | 保留的语义 |
|---|---|---|
| collectParticlePlan返回对象计划 | collectParticlePlanInto+caller plan A/B | 冻结输入、确定性双跑与分母身份 |
| applyParticlePlan返回结果数组 | applyParticlePlanInto+固定ResultBuffer | 事务全序、grant/拒绝分解、逐笔事件 |
| prepare返回独立对象patch | preparePatchHot返回唯一number token | 单active patch、旧句柄拒绝、原子publish/restore |
| checkpoint/snapshot返回独立副本 | checkpointHot/copyStateToHot写预分配区 | 完整池/字段/预算/源/journal回滚 |
| 字符串requestKey与Set去重 | 数值tuple+ordinal/epoch固定表，冷导出恢复字符串 | request与attempt身份不串、重放拒绝 |
| reserve/refund/close对象票据 | numeric热sink与R9 fixed out | 双端各退实际扣额、total一次、bottom0 |
| 运行期零分配 | 完整HOT可达图+长期固定buffer+错误/ABA验证 | 不采用方案B，不豁免其它D1出口 |

本附件通过manager验收后，developer可据此改D1热接口，不再自行把§13对象接口替换为另一套未登记签名；旧接口保留冷兼容。D1 R3是否通过仍等实现、全链证据与manager验收；文档消除的是接口设计依赖，不是宣告零分配已实现。若完整收口需要修改冻结物理算法/额度/guard或降低标准，先反馈manager/designer，不在本合同里暗授权限。

- 2026-10-24，v1（6adc91c3…d612）：按严格零分配热接口任务补调用方定容plan/request/motion/result/field/budget/guard/event缓冲、numeric request与全局不回卷的patch令牌、checkpoint/rollback与导出边界、完整可达图分配审计及验收要求。纯文书零代码，阈值/estimand/R9语义不变。
