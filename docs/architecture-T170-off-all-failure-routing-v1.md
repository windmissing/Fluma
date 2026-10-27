# T170：OFF_ALL 81 项首次归因、M2 单列 guard 热存储修订

2026-10-27；architect；原卡 `6ae055c1e4b0127832eabca4`。不使用真机、不实现业务/测试、不重跑探针。输入：T168 `6c0c654`、T155 `27b9054/a419f5a` 报告/追验、T168原JSON/压缩图、T162正式V2和M1热接口合同。**首次归因及工程规格交付；T155交付完成但门禁FAIL保留，T156当前不能按零分配前置已齐解锁。**

## 1. 真实结论、作用域与证据身份

只读解压并校验三个profile gzip和解压JSON的SHA，均与gate-summary一致；OFF_ALL原图5,572,362B、1656contexts、24roots、75pruned、15显式分配+66unresolved=81、registry60/60、analysisIssues0。原保守图35另列；81不是运行次数，也不是比35新增46个泄漏。独立反例16/16及动态20/20证明已测工具/诊断等价，不证明完整程序零分配或任意语法soundness。原始证据不覆写。

M1合同§1明确：warmup后begin→双collect→apply、真实field/guard/budget/票据/错误/rollback→finish的全闭包须零显式堆分配；不是只证明原Field/Prototype七文件，也不是只refund或零授予。T162V2要求保留真实接口实现集合，未将M2从接口调用闭包剔除。现图applyParticlePlanInto→applyParticlePlanIntoBody→P011ParticleM2CommitSink.applyInto→WaterSandField.internalParticleGuard路径有真实源码支撑；Prototype另一实现也保留。**这不是删除M2根、限制只用Prototype的scope争议，不需交manager放宽。**

图根清单含Hot导出/Hot getter及旧预算数值入口，共24；M2Sink不是新增独立根，是接口生产实现的可达callee。M2Binding.seed等外层构造入参对象不因此自动进入此root分母，不能把本卡结论升级成整个WaterSandField/页面无分配。NAPI/GPU隔离测试分配与此CPU生产图也分开。接口若允许外部实现，证明范围须列本地closed-world实现和外部unknown边；不能忽略外部边宣普遍零分配。

`internalParticleGuard` 在原guard调用发生时执行updateSandTop，然后 `new Map`，即便requested=0也先构造Map；现代码不受P/I/E诊断开关保护。M2Sink kind2或kind4（非birthOnly）保留实际host guard，而不是仅使用前一个数值guardAdmitInto。因此至少这个Map构造已足以否定真实生产Hot零分配；不能把81全归分析精度，也不能只修工具就期待PASS。

## 2. 15 个allocation逐项与66个unresolved路由

编号严格按 `test/reports/t168/off-all-remaining.json` 数组顺序；行号属于归档源码身份。本轮附 `docs/analysis-T170-gate-routing-v1.json`，逐项保留原owner/file/line/text/kind或reason、完整witness并新增分类/路由；不得将此架构人工分类当探针PASS。

| A# | 原位置/表达式 | 结论与路由 |
| --- | --- | --- |
| 1 | WaterSandField:4539 new Map | **真实生产guard入口显式分配**，M2单列数值存储替换，不能靠profile剪枝 |
| 2 | WaterSandField:4915 requested<=0返回对象 | 真实闭包内条件分支分配；单列fixed out保持原返回数值/早返语义 |
| 3 | WaterSandField:4922 residual回调闭包 | requested>0构造真实closure；单列直接按边读live residual，不传闭包 |
| 4 | P011MaterialGuard:134 new Set | 通用约束构造；M2单列改固定边前缀，通用冷参考保留 |
| 5 | 同:143 [] | 同上，固定residual/influence/out |
| 6 | 同:147约束对象 | 循环条件满足时真实对象，固定槽写入 |
| 7 | 同:56 inactive/非法requested返回对象 | 源级分配表达式，但此具体分支在已知H/有效requested入口下是否可行需值证明；不把静态可能分支冒每次分配。新增fixed out分支等价保留 |
| 8 | 同:71非有限d/k返回对象 | 条件分支，保留拒绝及错误/非有限行为，不删除guard |
| 9 | 同:82 k=0 infeasible返回对象 | 条件分支，真实单列±1不走k=0也不白名单该函数；通用实现/其它入口需保守分析 |
| 10 | 同:105 qLo>qHi返回对象 | 条件分支，fixed out保持区间拒绝 |
| 11 | 同:109最终返回对象 | 已到闭式admit尾时真实分配，fixed out |
| 12 | 同:166 new Set | naturalObservation拒绝证据ON且rejected>0才进入，属于独立观测路径，见下文N开关边界 |
| 13 | 同:175 [] | 同上受影响边数组；M2证据定容写入 |
| 14 | WaterSandField:4945证据对象 | 同上；不能靠诊断OFF名字认定自然观测已关 |
| 15 | PoolProgressDiagnostic:122 new Uint8Array(buffer) | 归档witness经M2 failHot→rollback→copyPoolDiagnosticHot finally→endSpan→checkLong。**P=null正常应短路**，当前unknown escape使事实失效而保守可达；是分析缺口待精化，不证明OFF真实执行。ON时视图仍真实分配 |

A1–14是实际业务闭包中的堆构造表达式，条件分支未全求可满足性，不称14处每次执行；A1的明确可达性已独立足以否定零分配。A15不能宣产品新泄漏，也不能手工从剩余计数删掉。

| U#（覆盖全部66项） | 原owner/位置/类型 | 初次归因与续作 |
| --- | --- | --- |
| 1 | beginStepHotBody:558 fieldBinding[i] | 预分配ArrayBuffer[8]原地覆盖引用，不是TypedArray但也不是new；工具仅认可typed storage过窄。需固定数组owner/长度/索引0..7/不扩容/无逃逸证明，不能将任意computed write放行 |
| 2–7 | BudgetAdapter.rollbackHot:95/97的totalRemaining/totalUsed/ticketCount/openCount/denominator/epoch | 原代码均原始标量赋值；M2派生类super.rollbackHot绑定this丢失/别名合并精度不足。补super静态分派+原动态receiver身份，不改业务退款 |
| 8–9 | WaterSandField:4539 delta.set | 同一调用的UNRESOLVED与ESCAPE两记录；实际Map方法/可能容量分配不是纯数值intrinsic。M2改固定单列存储；工具做有副作用摘要，不能新加pure白名单 |
| 10 | WaterSandField:4922闭包体 | CLOSURE_BODY_NOT_MODELED，工具未实现；闭包构造已另记A3，调用体还需解析真实captured-this，不重复算运行次数 |
| 11、18、26、33 | MaterialGuard:135/144/167/176 for-of | 四处UNSUPPORTED ForOf；缺失迭代与异常/early-return/IteratorClose语义。Map/Set迭代器自身可能分配，不能只消除unresolved就计零 |
| 12–13、27–28 | MaterialGuard:135/167 deltaColumns.keys | Map迭代器effect/escape摘要；真实调用不是pure；M2迁出后仍用于通用参考/其它可达入口分析 |
| 14–17、29–32 | MaterialGuard:137/140/169/172 affected.add | Set写入/可能扩容，有界业务替换与工具effect摘要分开 |
| 19–22 | MaterialGuard:145/146 deltaColumns.get | number→number Map读取可证明只读但要证明builtin与键/接收者，不能对同名get豁免 |
| 23 | MaterialGuard:148 residualOf(edge) | 本地闭包回调目标/捕获集解析；未知/外部回调保持失败，不把方法名残差当pure |
| 24–25、34–35 | MaterialGuard:147 constraints.push /177 edges.push | 数组可增长/元素对象别名，真实潜在分配效果；不能纯函数化push |
| 36–37 | NaturalObservation:877 materialGuardEvidence.push | 自然观测证据独立ON路径及可增长数组；M2证据热写定容/冷物化，工具仍验证callee效果 |
| 38–39 | PoolProgress.now:70 this.clock() | OFF路径由unknown扩大后进入；ON外部clock未知effect必须保留；不将SDK时钟纯函数化 |
| 40–43 | PoolProgress.input:75四字段 | UNKNOWN_ASSIGNMENT_RECEIVER，接收者/逃逸精度续作；不构成new证据 |
| 44–47 | PoolProgress.now:70–72时钟状态/lastTime | 同上；ON真实callback影响仍独立披露 |
| 48–53 | PoolProgress.checkLong:110/116/121/125各锁存/状态赋值 | 同上；标量写入不是分配，不能直接减计数仍冒自动证明 |
| 54 | checkLong:125 toString(16) | OFF误可达链续作；ON是真实字符串构造效果，padStart连链也需遍历，不可漏扫callee求值 |
| 55–56 | checkLong:125/126 rawHash/inputStatus | 接收者精度，与54字符串分配不可混同 |
| 57–60 | PoolProgress.record:82–84 write/count/dropped/firstCount | 接收者精度；固定ring原地写需证明真实冷容量 |
| 61–66 | PoolProgress.cursor:101–102 root/end/parent/child/loopCount/ticks | 接收者精度，P=null路径应按事实剪；不指认为循环或诊断执行缺陷 |

U表并集恰为1..66，每项原文及完整路径见JSON。预期未实现ForOf/closure等记 **NOT_IMPLEMENTED_ANALYZER** 并结束本轮这些项的产品归因，续作交工具实现；不派物理缺陷修复。无法证明的别名/receiver项记PROOF_GAP，不能因此声称真实泄漏或已排除。

### 2.1 unknown为何带入诊断；N不在OFF_ALL事实中

T168 `invalidate` 遇带REF未知调用会将heap槽全置UNKNOWN并标!escaped，后续字段读取不再套profile；局部先捕获的const仍可保持null，这一点T155独立反例已验证。首次delta.set接收者是本地new Map，参数column/sign是primitive，通常没有指向workspace的路径；但目前全局污染导致后续rollback中新读P变UNKNOWN。应证明局部对象可达集后限域havoc，而不是永不失效P：若未知代码确实拿到workspace/alias，仍必须使相关字段UNKNOWN；不能信任已知方法名称强制不逃逸。

此外 N=naturalObservationProbe 是独立于P/I/E/G的开关，当前OFF_ALL只约定后四项，不包括N=null。N ON且rejected>0时A12–14/U36–37**实际可达**；源码默认N=null不等外部冷配置从未开启。因此不修改OFF_ALL含义来逃避证据路径；若manager将来要另列NO_OBSERVER profile，须版本化独立前置并验证真实使用状态，不能替换当前profile/删自然观测模块。当前修订仍覆盖观测ON的M2调用成本。

## 3. 最小业务工程：只替换M2单列guard存储，保持计算/次数

不改外部 `P011ParticleM2Host.internalParticleGuard(column,sign,requested):number` 签名及返回量。WaterSandField新增一个冷初始化的ParticleColumnGuardWorkspace（固定edgeId/residual/influence容量3，count<=2，固定out标量accepted/rejected/allLegalBefore/limitingConstraint/physicalEdge）；private identity/buffer绑定校验沿现有定容模式，非逐次new。新增internal Into纯核可置 `P011MaterialGuard.ets`，**通用Map/Set旧函数保留作为冷参考和非M2调用者，不全局重写算法**。

建议内部接口 `p011GuardAdmitNumericInto(requested,residual,influence,count,hard,out):number`，读取既有F64数组有效前缀、只写out，返回既有数值状态；不建结果对象/临时tuple/闭包。不由调用方自由增长count，不保留借用数组到事务外。WaterSandField入口顺序：

1. **仍先同一次updateSandTop**；不提前删掉或变成缓存。单列受影响边严格按旧Set插入序：column-1在范围内先入，再column在范围内；sign在左边系数+sign，右边系数0-sign。边界列0/cols-1只一条，不引入排序改变tie。
2. residual仍逐边调用原 `materialGuardResidual(edge)`，即sandTopScratch差减 `Math.tan(effectiveTilt*Math.PI/180)`；H仍原 `Math.tan(30*Math.PI/180)` 常量。不能直接用M2前一个guardAdmitInto的值代替hostguard：前者用work.effectiveTiltRad和sand(work)及不同out含义，未证明bit等价。**两个既有guard调用都保留，audit次数不动。**
3. requested<=0保留旧wrapper的早返accepted=0/rejected=0/allLegal=true/limiting=null，不生成拒绝证据；其它分支完整搬运旧p011GuardAdmit的isFinite/armed、qLo/qHi、legal/non-worsening、k=0、lo/hi swap、先lo再hi更新limiting、qLo>qHi拒绝、最终min/max。保持RN运算次序、NaN/±0/相等端点和tie更新规则；limiting先存constraints位置，再按相同edge前缀映射物理edge。此为存储工程，不重新设计区间/容差。
4. internalParticleGuard仍返回accepted原标量，M2Sink依原顺序覆盖其guard.accepted/rejected、按实际debit退款、prepare/publish/close/audit；不提前复用前一次结果、不缓存跨mutation证书，不省任何guard/事务审核。不改预算300/s、H、质量、票据、ID/slot或public endpoint协议。

### 3.1 M2拒绝证据也须固定热写、冷物化

M2原host guard内recordMaterialGuardEvidence含数组/对象/push，不能只迁A1–11便宣全图零分配。为**M2调用来源**增加数值Into记录：固定SoA保存channel数值码、实际affected edges（最多2）、limitingPhysical、requested/accepted/rejected/clipped、原顺序身份；不构造对象或Array/Map。其它writer继续旧记录API。naturalObservation新增窄数值写入口，旧返回/导出字段语义不改。

容量按一个Hot span最多plan.requestCapacity次host guard预分配（当前Q=2048；每行2边）并与work/route cold绑定；每个独立apply结束后在Hot栈外、下一writer前按原顺序物化/封页。**不是把guard本身移到marker外**：只有已冻结观测副本的对象化允许在外。每行只记录原本N非空且rejected>0的调用，不新增记录/重置已有观测；失败/rollback是否保留原观测，逐路径与旧实现比对，不能默认丢失败证据。正常/异常/早返均明确flush责任，热路径禁止回调物化函数。若不能由原Q与一个span的调用关系证明容量/无重入/消费时序，报告该部分BLOCKED给architect，不擅自新增物理拒绝、截断证据或降低guard次数。

只做M2窄接线及cold workspace/数值纯核/观测Into。旧通用guard、其它writer的预览guard、计算舍入、证据字段协议不变；不触及T135支持纯核/strict route。精确等价不成立若涉及算法假设，再交designer；本轮没有算法修改需求。

## 4. T168剩余工具能力工程续作（不改业务来迎合探针）

工具owner负责 `test/tools/p011_hot_context_graph.cjs`、符号建模依赖、reporter/反例；原probe无参图、24根、60身份/配置写入证明不缩减。实施可与§3并行，当前81仍按冻结版本保留。

- **固定普通数组写**：证明cold owner+capacity、整数边界和被写元素effect；ArrayBuffer[8]替换引用与未知push/越界扩容区别。未知索引/长度变更/setter/proxy继续FAIL，不能因Array类型就豁免。
- **super及this**：super.method指向确定base实现，但callee的this沿用实际derived对象，继承字段同一对象存储；内部虚方法仍按动态实现集合。branch join保留对象来源，字段原始赋值不应被误记constructor；未知receiver仍fail。
- **有限points-to/effect摘要**：局部new Map/Set/array/closure有对象身份与指向边，未知调用只havoc可能可达/共享/全局逃逸集；真正workspace alias传入仍失效。精度改进须输出为何保留P=null的证明，不能对M2/诊断文件特别豁免。闭包捕获this/参数的读写/返回effect，未执行闭包体不臆造运行，但构造closure始终记allocation。
- **本地callback/for-of/内建effects**：解析本地residualOf到具体闭包体与捕获对象，未知外部回调FAIL。for-of实现iterator创建/next/done/break/return/throw/IteratorClose及循环不动点；Map.keys/Set迭代器、Map.set/Set.add、Array.push都有可能分配/增长效果，不能列pure。Map.get等可读摘要须以真实builtin receiver、参数/原型完整性为依据；同名伪方法必须执行其体或unresolved。未支持就NOT_IMPLEMENTED，不凑零。
- **链式求值**：例如toString(...).padStart(...)，先分析receiver表达式/调用及实参，再外层方法；字符串产生仍记allocation effect，不能因外层未知漏扫内层。ON报告真实clock未知不免；OFF新读P只有在实际别名证明后才剪。
- 源manifest/配置control-flow hash在生产窄修订后按真实批准新版本更新，只核已解释的冷存储/数值接线hunk；不能“更新所有fingerprint”绕过反向重绑注入测试。结果分层`SOURCE_ALLOCATIONS`、`ALLOCATION_EFFECTS`、`PROOF_GAPS`、`NOT_IMPLEMENTED_ANALYZER`，所有阻断项仍计FAIL，原81身份对账另列，不靠手工分类给ZERO。

独立tester新增反例：固定数组合法覆盖与越界push/改length/别名逃逸；super派生实例字段和base内虚调用/伪同名类；本地Map仅primitive参数不污染workspace与Map包含workspace再传未知函数必须污染；closure只捕获标量/捕获workspace写槽/外部callback未知；for-of零/多元素、break/return/throw及IteratorClose分配；Map/Set真实增容与同名伪get/set；嵌套toString+padStart及finally；P关闭E/N独立ON。继续复用原25开发/16独立反例作为回归，新增精确位点断言而非只assert总图FAIL。工具本身测过16项不能证明新增模型无漏放。

## 5. 独立验证与T156前置裁定

两条实施线独立交付并能编译/离线自测即报告，manager直接登记、另派独立tester，不把设备端到端变开发关闭前置。

所有权明确分开：业务developer负责 `entry/src/main/ets/physics/WaterSandField.ets`、`P011MaterialGuard.ets`、`P011NaturalObservation.ets` 的单列workspace/Into及物化接线；仅必要时按hunk协调M2Sink，保留外部host签名、两个guard及audit调用。工具developer负责 `test/tools/p011_hot_context_graph.cjs` 及其符号建模依赖/report生成，不改业务语义；独立tester负责新测试/反例/正式报告及原始结果，不与业务实现者共用预期值计算核。共享树各自保留他人修改；任何源hash变化按实际hunk和manifest记录。工具事实适配等待窄业务源码定稿，但工具能力和独立夹具准备可并行，不拿零碎准备替代整卡实质前置。

1. **业务窄修订独立测试**：以原Map/Set冷参考（冻结源hash）比较single-column sign±1、列0/中/末、requested0/有限正量/非法入口、合法/违法边、等H/ULP、k符号、limiting同界、qLo>qHi及non-worsening；核accepted/rejected/edge顺序和原RN结果。验证先updateSandTop、两个guard及audit调用次数未变；M2六类事务、实际refund每端/total、失败/rollback、before/after原池/source/预算/event等价。观测N ON/OFF及拒绝/错误/flush/容量证明，禁止只测birthOnly绕开guard。
2. **工具独立测试与正式图**：新模型反例及原回归；旧保守图保留；实际待测树OFF_ALL、其原P/I/E/G事实、24根/60项完整，主图remaining=0/unresolved=0才静态ZERO。没有完整模型先如实保留FAIL，不把“真实Map已修”冒整图通过。ON真实分配/未知时钟另列，不要求其零分配来抹诊断功能。
3. T155动态20/20与独立反例16/16已交付，不重开“manager验收”；生产guard与工具变动后只补受影响的窄独立验证和最终图，不无理由重跑T128全部287pair或完整旧矩阵。当前没有实测运行分配次数/GC暂停/设备性能，不索求manager自己跑测试。

**T156当前受真实实现合同缺口与工具证明缺口共同阻断**：至少A1是真实Hot堆构造，因此不能称“仅工具缺口、可直接静态放行”。T156原任务依T155无阻断结论和有效包，保持真实前置；§3窄修订+独立等价、§4工具补证及当前图结论到件后按原依赖推进，不加manager批准门。若manager希望先对现有分配版采集卡死，需另明确“定位取证/非零分配验证”的版本/目的并登记不能作为合同PASS；本卡不自行修改T156前置。

这不是卡死唯一根因：没有设备内分配频率/GC/冻结因果证据，不能用源码new Map推断13秒都来自它。T116仍等待真实N/live、seed/λ/audit/index/copy乘数与首超长时钟；本卡只处理本次门禁首次失败，不重复整体冻结归因。GPU目标6最高优先、隔离合成收益链可并行，不因本CPU门禁无限等待。

各下游prompt必须写：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人状态有限例外），不更新滴答同名清单、不回退他人修改；离线明确不使用真机，设备标真机且串行。所有前景/背景渲染仅新XComponent+shader，禁止Canvas上屏/失败回退；故障FAULT或显式STALE旧有效新管线帧。冻结guard/守恒/预算/审核次数、estimand与现有错误语义不变。manager登记交付与调度，不新增验收门。

本轮逐项读取81条及witness，核对源码与合同并只读校验归档hash；未运行产品/探针测试、未用机、未改业务或测试代码。本归因工程任务成功交付，不把被归因的门禁FAIL改写为产品PASS。
