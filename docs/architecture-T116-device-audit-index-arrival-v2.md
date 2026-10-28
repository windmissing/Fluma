# T116 T129到件：audit/索引负载与收尾证据增量 V2

2026-10-28 接续：T156 到件的首次核证、R3 独立 run/提前 CLOSED 勘误、同分母复杂度及 T194 整体设计交接见 `architecture-T116-T156-whole-transaction-v3.md`；本 V2 为历史到件范围。

2026-10-27；architect；原卡`6adf3c1ce4b017ab3404b7c5`。不使用真机，不重跑测试。消费T129 `d4842c3`报告、原始两份appfreeze/五臂hilog/console/脚本，T124/T123/T128及现行Hot/M2/WaterSandField。T124旧PID/提前CLOSED纠正、T123开发、T128离线287样本已消费，不重复索取。

**部分交付／真实调用乘数与精确冻结输入仍关键缺证，原卡看评论。** 设备长同步任务真实复发，源级有界排序不等设备已稳定；本轮先落诊断/取证工程，不授权跳过audit或改物理批次。T57继续等必要修复和独立稳定性；GPU目标6最高优先T141/T142并行接力，不被本卡诊断前置阻止。

## 1. 原始栈比报告更窄，也包含被漏掉的新索引链

| 臂/原文件 | 可成立的观察 |
| --- | --- |
| R1-appfreeze-072151.log，PID31390 | APP_INPUT_BLOCK真实，样本含siftDown→sortOrderHot→auditParticleHot→finishStepHot→seed→liftByShake→applyMotionChannels |
| R2-appfreeze-072309.log，PID32663 | APP_INPUT_BLOCK真实；一个ETS样本为auditParticleHot→M2Sink.applyInto→applyParticlePlanInto→seed；同文件TS栈明确出现**P011MergeExactIndex.sift/sort/find→mergeOldestHot→editPoolHot→…seed→liftByShake** |

因此不能采用T129“两个原栈均无merge、都迁移到finish audit”的结论：R1支持该采样位置，R2明确同时含新索引和sink内audit样本；也没有sortActive返回-1导致finish死循环的证据。TS/ETS映射与采样时刻不同须保留，不把任意一份首栈当完整执行历史。新索引确有执行栈正证据，但不等端侧全部复杂度/性能已通过或旧热点已彻底消除。

HAP s148 hash `4c4aa66bee8e310ea774f9c33bb56983eb4fc275afe824b61566ca977bb8917e`、源基线a2e47d0/entry树f8832885f6ed7696是报告声明；当前仓库查找未见s148包用于独立hash核验。新索引真实栈可确认该路径已运行，不能据声明独立证明整个签包源码身份。后续只需补包/构建对应清单，勿重跑已有host差分。

R1/R2主线程故障记录均在脚本swipe之前可见，支持“本次两臂无需已发出swipe即可复发”；不能由有限臂证明shake是必要条件或所有freeze都是它引起。shake输入0不等requestEnergy0，仍按既有能量前后和初始化/衰减合同判。

## 2. 新增的实际墙钟证据与不能推的量

原WS_FRAME在R1仅2条：07:21:37.828 physicsTimeMs1693、07:21:50.772为12909；R2同样2条：07:22:55.944为1678、07:23:08.916为12938。这是真实帧级物理区间，非单次audit/sort/merge独占时间；保留原单位/计时位置，不按栈占比拆分或推出host10倍。

两臂最后metric的sequence/substep为4，sourceEpoch0、particleMass≈0.05306700511283798，随后各自ui:1 CLOSED为records4/admissions0；说明仍有少量物理进度/粒子，不是“采栈后必已死”。不能从这些汇总反推冻结时nmax/active、每列birth或λ数。原始日志没有POOL_PROGRESS记录；新计数器代码存在不等这轮已启用/归档。N=192的报告估算不采，旧T124另一run的512也不能顶替本次实际N。

`siftDown`在合法定容输入下parent每次交换变为2*parent+1或+2，严格向下；不交换直接return。heapify root循环递减，extract end递减；ID相等仍用slot破同序，不会因重复键失去终止性，重复活动ID另有拒绝。因此源级不支持“同ID/同Y令heapsort变无限循环”这个猜测；非法count/数组别名等需实际反证，不能凭state=R定死循环或排除全部锁/调度成本。

一次audit包含pool transfer、free-list/活动槽扫描、ID排序、重复ID检查、逐列材料/cache/预算审计；池/场部分单次O(Nmax+L log L+cols)，另加真实预算审计成本，L为实际live。单merge索引仍O(Nmax+L log L)，整attempt不是这个界：

`W≈Σaudit(Nmax+Li log Li+cols)+Σmerge(Nmax+Lj log Lj)+copy/collect/birth/预算/事件成本`。

完整seed正常一request路径有sink内部audit、applyParticlePlanInto尾audit、带budget的finishStepHot再audit；前提成功且走到该位置时单seed可有3次，失败会提前退出，不能把源码调用点数当本轮实测次数。lift逐列/分相seed、λ失败重试、preview与authoritative叠加，排序/复制虽有界仍可累积长UI任务；真实M/A/B/λ必须计数，不能拿容量当乘数。T128并未覆盖这条完整audit/账本链。

## 3. R3不是注入后稳定性通过；R4不是新收尾

R3 run=`p011-run-1793057021611-3`、PID32755。原日志：

- WS_FRAME共132条，最后07:23:47.939；23条WS_METRIC均sourceEpoch0/particleMass0。
- 07:23:47.942真实ui:1 CLOSE_REQUESTED，47.952 FENCE CLOSED，47.953 CLOSED records264/admissions0；lastSequence264/lastSourceEpoch0，与132帧×2子步对应。
- 首pan_update为07:23:55.477，晚于CLOSED约7.5秒；共494条、accepted=true420条，结束到07:24:03.471，但CLOSED后无新物理帧。accepted是手势层收据，不能代替事务注入；captureFinished会跳过后续注入。
- console R3 started为07:23:49，已经晚于CLOSED；启动阶段click(240,620)的页面收尾时序须修执行器。R4在07:24:48检索到同一run早先CLOSED，并非R4新触发且覆盖手势后的双流。

报告“1460 ws行”不能当ledger行或物理帧数；264来自真实先前CLOSED，未见本地ledger/admissions文件供逐行核对。报告“Get-Item时序缺陷导致0B”未证：脚本hdc recv结果/退出码被丢弃，又把缺失/空Length打印成0B；console另明确manifest缺`fence.json`，且脚本未生成该文件就调用manifest程序。同步hdc返回不能无证据归为“未等待”。权限/路径/进程存活/传输退出码各自取原码，不绕访问限制，不声明磁盘上实际空文件。

另外Check-AlivePID在臂结束才pidof；console R1/R2 PID为空却打印alive，空PID不能当健康。原appfreeze明确31390/32663，可独立保留FAIL；执行器须在启动时冻结PID/run，进程消失单列EXITED/UNKNOWN，不用空正则或旧臂记录冒本臂通过。

## 4. 先修工程文档后的最小派工

### developer：有界audit/索引进度诊断，不改物理

不使用真机。沿既有POOL_PROGRESS版本化增补：实际nmax/live/free、run/state/config/substep/attempt/candidate/λ、seed sourceKind/column/phase/B、energy前后；auditCalls按sink-row/apply-tail/finish分源，sortOrderCalls/比较/sift迭代与poolCopies、index sort/tree/range计数各自区分。sortActive用于collect、sortOrder用于audit，不混名字。每attempt/seed保存入/出计数delta、单调时钟阶段起止与错误；全局累计不能冒单attempt乘数。

在循环前和阶段切换记录进度（root/end/parent/child/count/epoch），固定冷ring和首超长窗口，禁止逐比较日志/全量逐帧池dump；各阶段计数不改变排序/审核次数。只导出固定预算首32条进度及一次阈值标记，真实未导出计数标NOT_CAPTURED，UI被卡不得填0冒计完。可合法取得的冻结pool含完整原F64/ID/state/group/free-list/config/seed哈希身份；只能归属当次真实输入，不把合成池称精确回放。

目前不删重复audit、不缓存旧证书跨mutation、不批量seed改账本、不把UI事务拆跨帧。若后续证明可复用同一不可变pool/field/budget的audit，先另列完整revision/write-set/失效与错误等价工程，不凭“看起来三次”直接去两次。若要改选对/事务算法交designer。native完整物理owner及GPU隔离原型沿T130/T138既有路线，另测端到端成本，不把搬线程当计算量已消失。

### tester/effector：最小窄补证与执行器纠正

tester不使用真机：独立核有界排序在满/稀疏/同键/非法ID的终止与比较计数，完整seed/audit链调用放大、错误/rollback及诊断开关无副作用；不要重做T128同一287样本冒新增证据。补诊断开发交付后，effector真机标签同机串行，按实际run启动立即锁PID，**pan与实准入先于唯一显式finish**，验证CLOSED前持续帧/commit/source变化。先一条shake复发与一个真正进行物理的对照，不盲重跑旧宽矩阵。

同一首超长attempt需实际N/L/B/seed/λ/preview/audit/index/copy计数及阶段耗时；若仍只有采样栈，继续标负载未证。存活run收尾保存requestId/run/generation/最后commit/source、原始FENCE/CLOSED；持证合法拉文件，记录hdc退出码/stderr/字节/hash，先实际生成fence.json再manifest逐行核身份。失败原因未知就报告未知，不将拉取错误定产品缺陷，也不将事件级CLOSED冒完整双流文件PASS。

完成这些精确证据后回本T116续判工程/设计/采集问题，不新建重复整体归因。T57依必要实际修复/独立稳定性；T95与T146 FIELD_COLUMN线独立。所有下游prompt：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己状态有限例外），不改滴答同名清单、不回退他人修改。无设备卡明确不使用真机；设备卡真机标签、串行。所有渲染含背景前景仅新XComponent+shader，禁止Canvas上屏/失败回退；故障FAULT或显式STALE上一有效新管线帧。不改判据、H或容差，manager登记不验收。

本轮只读原栈/日志/脚本及源码，未用真机/重跑测试/实施业务；T129设备硬失败保留，机制与实际负载仍未全证，T116看评论。
