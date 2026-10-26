# T92 双场主线程卡死工程归因 v1

日期：2026-10-26。architect。原卡 `6adf1e5ee4b03273ad83857d`。不使用真机。状态：**确定同步粒子池路径存在高成本工程问题，唯一耗时构成/零源机制待精确补证；不是已修复**。

## 1. 证据与结论边界

消费T78 `40b26d5`报告、114a7ee/s144设备证据及两份原始appfreeze：`test/artifacts/t78-dualfield-retest-20261026/R1-appfreeze-162023.log`（PID49147）、`R2-R3-appfreeze-162128.log`（PID49813，实际是R2进程，文件名不证明R3也有独立堆栈）。R1不同采样分别位于collectParticlePlanInto和mergeOldestHot，R2有merge→seed→lift链；两份APP_INPUT_BLOCK原文成立。R3进程50631的死亡/日志中断与报告相符，但不把PID49813的栈挪给R3。

三臂产品卡死FAIL保留；栈定位物理step内同步pool工作，非已证GPU/worker收尾卡死。**相同栈不等于死循环**。源码中mergeOldestHot是有限i/j扫描；births经safeInteger和剩余journal容量检查；每次成功merge liveCount--、freeCount++，下一出生消耗槽；collect的phase/j、heapsort的parent递增均有进度。正常池不变量下没有可由这些循环直接证明的无终止分支。仍需池状态证据排除损坏/异常规模，但不能把“加循环上限”当根因修复。

## 2. 已确认工程热点与复杂度

`P011ParticleHot.ets:mergeOldestHot`对每个active i遍历所有N个j，即使已经找到最终按ID最小的可合并对仍继续扫描；满池时每个birth都可重复。单次最坏N²候选检查；默认N=512，满占用单次262144个内层位置，B次满池出生为O(BN²)。massPackage=.1，tail也产生birth；不能只按总质量猜birth数。

`P011ParticleM2Field.ets:seed`为单笔出生先beginStep冻结/备份，然后两次collect全池，再把plan改成单行birthOnly请求；lift对各列H/L逐笔调用seed。每列可能多出生，且planner preview/authoritative阶段亦会重复相关路径。因此成本还含每seed的全池copy、排序、双collect、patch/sink/校验/事件导出，不能只修merge便签全链性能通过。具体B、seed数、pool占用及单段耗时，T78未记录，**静态复杂度不是该设备每段实测耗时**。

工程归因成立的是同步重复计算/全对扫描风险与设备栈吻合，足以给最小工程优化任务；不能断言已排除所有其它物理热点或已找到唯一根因。不修改容量、massPackage、速率/预算、MERGE匹配/选对规则、守恒或既有性能门槛。

## 3. shake=0与零源的收窄

liftByShake已有 `requestEnergy<=EPS` 返回，requestEnergy取更新前shakeEnergy；setShakeInput(0)不会把已有能量清零，而按既有衰减；invert180也可把能量抬至至少.8。页面thick且无explicitShakeControl时另有倾角派生shake输入。因此“shake参数0”不证明该次lift能量0，**不得加shakeInput==0直接return破坏衰减合同**。T78 R3是否受残余/派生输入、脚本覆盖或其它路径影响仍未知，须记录实际控制和入口，不凭R1/R2栈给R3定因。

pan accepted=true只表示gestureSandGate通过，标记设置在rateClock.sample之前；dt<=0、captureFinished、field为空/预检null均可在收据之前退出。stageGestureSandAndFeed确有同步inject调用，但0 sink和sourceEpoch0不足以证明每个accepted均已调用inject，也不足以证明与卡死唯一同因。零源作为同卡伴随问题分别追链，不绕开真实准入，不伪造NO_WATER拒因。

## 4. 工程合同：P011_POOL_PROGRESS_DIAGNOSTIC_V1

新增仅诊断的有界attempt摘要，由manager派developer落以下hunk，独立tester执行：

| hunk | 必须接口/时序 |
|---|---|
| `P011ParticleHot.ets` merge/collect/edit | 在既有循环累计candidateChecks、activeCount、mergeCalls/success、birthRequested/completed、journal/lineage使用；同事务记录free/live/nextId前后及首异常。计数安全整数，不逐pair打印、不复制无界历史。 |
| `P011ParticleM2Field.ets` seed与M2Sink | attempt内seed次数、copy/collectA/B/patch/apply/finish各段耗时与状态、birthOnly及来源列/phase/sourceId；trace异常不得改变物理成败。 |
| `WaterSandField.ets` lift/规划/step | run/attempt/substep/config、preview或authoritative、shakeInput、更新前后energy、dt、requestEnergy、各列请求/实际lift及fallback；只在真实执行值处旁观，不第二遍seed。 |
| `SandCanvas.ets` routeGestureSand/stageGestureSandAndFeed | gate之后clock active/sampleSeconds及原因、captureFinished、请求sequence/H/L、inject enter/return(null或真实receipt)、sourceEpoch前后及返回耗时。事件时戳保留单位，不为了诊断重算源量。 |

热路径只写预分配摘要/计数，安全边界flush；溢出标TRACE_INCOMPLETE。开关诊断同输入，池/场/预算/账本/事件/身份与提交序列应一致。卡死不能等正常return才留下全部证据，隔离host进程超时需由外部runner取最后阶段/计数，不能让UI靠日志watchdog强制半提交。

## 5. 最小developer任务书（工程优化，分段证明）

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身行状态有限例外，不改滴答同名清单；共享代码不回退他人修改。任何渲染只走XComponent+shader，含背景/前景；无Canvas上屏/回退，失败FAULT或标明STALE上一有效新管线帧。

先按§4采离线热点计数，固定114a7ee全依赖和补丁身份。**不要先改物理策略**。可直接实施的最小等价优化：merge选择改为按粒子ID递增的确定次序查找，找到最小a及最小合法b即可返回，匹配谓词与加权更新计算顺序完全保留；使用warmup定容索引，按scratch当时状态重建/失效，不复用frozenPool或先前merge前索引。原函数按全扫描选字典序最小ID对，优化必须逐次证明选对与返回status一致。无可合并时仍准确返回原错误，不无限重试。

这个短路只减少可合并场景检查，**最坏无匹配仍O(N²)，不能作为完整性能保证**。developer先对T78离线轨迹及对抗池报告实际计数/耗时，若仍不能满足既有预算/性能合同，交architect再分流designer做精确索引/批次算法，不通过降低N或截断birth取得通过。

seed双collect若确认为热点，另一个小hunk在`P011ParticleM2Field.seed`及必要Hot初始化：只为birthOnly准备与原collect完全同值的owner/epoch/plan头、单行计划及guard预算，不运行随后必被覆盖的运动请求收集。该优化须证明collect并无仍被消费的派生副作用，保持planA/B、sink、rollback/journal/lineage及finish状态；不能未经证明直接删两行collect。与普通motion collect分开入口；证据不足先交部分优化，不扩成重写pool。

失败维持原原子回滚、错误与wet LIFT既有fallback，dry无merge/fallback语义不动；不能在主线程异步切开一次事务、跳过merge或静默降级成功。source/lift审计累加顺序和事件ID不改。优化属于工程实现revision，原物理协议/阈值不升不改。

## 6. 离线证据、复测与解锁

developer先交开发/编译/离线等价证据；tester独立覆盖N=16/512、空/满/无匹配/多匹配/不同列源相态、tail births、journal/lineage不足、连续merge造成质心变化、两phase、rollback、preview/commit、诊断开关等价。新旧实现对同池逐次pair/质量/ID/freeSlots/事件/status逐值对照；计数证据比纯墙钟更易跨环境核对，墙钟只作host性能身份，不替设备fps。

T78三臂cold run参数/输入时序离线复现，runner在子进程执行并有外部超时（超时记FAIL并保留阶段，不将杀进程当修复），报告总seed/birth/merge/collect次数与最坏单attempt运行；无复现或无池快照明确UNPROVEN。R3重点记录input/energy/派生输入及首次lift，零源记录gate→clock→request→return全链；按run/PID去混臂，不修改raw。

T94及T57的实质前置：具体修复开发交付与同场景离线有界执行/等价证据到件；manager据交付推进，无新增批准门。若优化仅部分有效，仍列实质未齐，不能因“代码有循环上界”就释放整卡。随后设备T94串行验证三臂不freeze、真实准入、完整scene可见沙、前台/Want finish和两流严格CLOSED/manifest；T57完整G5保留原颜色/性能门。无需先由manager重跑测试。T66收尾当前UNPROVEN，不能因到不了finish而直接返修store。

本轮仅归因和工程文档，无业务修改/测试重跑/真机。原卡报告交付及未证项，自己的行结束待验收，后续卡/队列由manager派工。
