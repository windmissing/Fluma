# T162：T154 诊断 OFF 可达性与静态门禁接入 V2

2026-10-27；architect；原卡 `6ae00c6de4b017ab340e9c6a`。纯工程设计，不使用真机、不执行测试、不改 entry/ 或 test/。本版取代同名 V1 草案与原卡正文中嵌入的旧回复；旧材料原样保留，不作为放行依据。状态：**归因与实施规格交付；扩展探针尚未实施，原门禁 FAIL 保留。**

## 1. 裁定与证据身份

34 个 unresolved 的直接原因确实是原探针只装载七个文件，缺 PoolProgressDiagnostic、FieldColumnEvidenceV2、MergeExactIndex；同时原分析器按方法名连接所有同名方法、扫描所有分支，不传播 null/布尔值。补文件本身不足以证明 OFF 零分配，反而会发现 ON 分配和新间接调用。选择题 **(a)：扩展主探针，做有限的上下文敏感 OFF 可达性分析**；诊断 ON 单列计数/分配报告，不代替主门禁。

- 冻结输入为 T154 `7983e20d9ebb6ec730926821ab44477d36acac44` 的代码与 `test/reports/t154/static-gate.json`。后者工作树与该提交内容一致；工作树原文件 SHA256 为 `aaf3e4b3d0f46762fbd872a2035fce279fb12a4db5c165295aca29e4a0a4448e`。
- 原文件 **baselineCommit 是 b21d0e582ef20e5d37a71089ce5eb0fe456d22eb**，不是卡片/报告所写的 6ee7e19。只读 git 对照两版本的 Hot、MergeExactIndex 内容 SHA 相同，均尚无 PoolProgressDiagnostic；因此这两处既有调用的归属结论不受影响。不能把两个 commit 字符串当相同提交，后续报告记录实际对象与源 hash。
- `7983e20` Git blob 原文本 SHA256：Hot `f0721b418b99fa2bc3f9ba64a64ca31cb709d9e1cbb9a24e11f3deed36ba594e`；MergeExactIndex `c84d2085edb7d0f8dd50af0b885d5b04544ea519ef9821565aeafe5461ac63e2`；PoolProgressDiagnostic `6004c8ff2a8afcfe343a290d21cdfba99469253ddb9f729b2232c18569dee8b0`。Git blob 与工作树换行 hash 分列，不能混用。
- 本轮阅读时 Hot 已叠加 T166 `1f8e16c`：waterTop EPS 常量及谓词使表中位置整体后移四行；诊断守卫与调用未变。表用**原报告行号**定位，不伪称 current HEAD 的同一行号；后续探针对实际待测树重新产 source manifest。

原报告 `remainingSiteCount=35` 是 34 unresolved + 1 totals 对象字面量；它不是“35 次运行时分配”，也不是零分配证据。

## 2. OFF 的两个独立开关与范围

定义 P=poolProgress，I=mergeIndex.diagnostic，E=fieldColumnEvidenceV2；G=work.diagnosticEnabled。

`WaterSandField.configurePoolProgressDiagnostic` 在关闭进度诊断时设置 `G=false、P=null、I=null`，**不会设置 E**。`setTiltPreviewTraceSink(null)` 才将 E 置 null；非空 sink 在冷边界创建 E。route 创建/恢复也分别重绑这两个诊断对象。Workspace/Index 默认 P/I/E 均 null。直接 Hot 调用者有可写 workspace，不能由一个 UI 开关名替它假定全部关闭。

| profile | 入口事实 | 可作的结论 |
| --- | --- | --- |
| OFF_ALL | P=null、I=null、E=null、G=false；操作中不开关/重绑 | 本次目标主门禁 profile；所有成功、拒绝和 rollback 分支都分析，不假定输入永远合法 |
| PROGRESS_OFF_EVIDENCE_ON | P/I=null、G=false、E非空 | 32 处进度调用短路；FIELD_COLUMN 路径可达真实分配，不能套 OFF_ALL 的 ZERO_ALLOCATION |
| PROGRESS_ON | P/I非空；E按实际记录 | 进度诊断进入计时/阈值捕获，分配和时钟回调独立披露；不承诺零分配 |
| UNKNOWN / 不一致绑定 | 任一前置未证或外部别名可改写 | 保守合并分支；不能获得 OFF_ALL PASS |

OFF_ALL 是既有冷配置可达状态的显式描述，不修改 T154 开关语义，也不关闭 T149 诊断去掩盖其真实成本。主证明范围保留原 probe 的 Hot 根函数、Hot getter、60 项 registry，不扩写为“整个 WaterSandField/M2/页面零分配”。例如 M2 seed 的入参对象及页面查询 totals 不在此范围，另行报告作用域；不能删根或缩减 registry。

## 3. 与 static-gate.current.unresolved 的 34 项逐条映射

下表序号为 JSON 数组下标+1，文件均为 P011ParticleHot.ets。Q=接收者确证 null 的可选链，连同实参均不求值；B=确证 null 的 `if(d!==null)` 整块跳过，含 try/finally 对应块；T=条件表达式只取可达臂。所有“跳过”均依 §2 指定事实，不以方法名或文件身份豁免。

| # | owner / 原行号 | 调用 | OFF_ALL 判定及来源 |
| --- | --- | --- | --- |
| 1 | beginStepHotBody:525 | E.captureFailure(...) | E 的非空 guard 为假，整个调用及 totals 实参跳过；E独立，只有 P OFF 不足 |
| 2 | beforeInOrder:664 | d?.increment(6) | Q；d 从 sortOrderHot 经 siftDown 实参传入 P=null |
| 3 | siftDown:672 | d.increment(7) | B；同一 d=null |
| 4 | siftDown:672 | d.cursor(...) | B；同一 d=null |
| 5 | sortOrderHot:697 | d.input(pool) | B；const d=P |
| 6 | sortOrderHot:697 | d.cursor(...) | B；同上 |
| 7 | sortOrderHot:698 | d.begin(...) | B；同上 |
| 8 | sortOrderHot:698 | d.increment(...) | B；同上 |
| 9 | sortOrderHot:701 | d?.cursor(...) | Q；同上 |
| 10 | sortOrderHot:704 | d?.cursor(...) | Q；同上 |
| 11 | sortOrderHot:707 | d?.endSpan(...) | Q；finally 仍访问但调用短路 |
| 12 | mergeOldestHot:949 | work.mergeIndex.find(...) | **常开生产边，不剪枝**；解析真实 Index.find→findBody→sort/sift/tree/range |
| 13 | editPoolHot:969 | P?.increment(19,births) | Q；P=null |
| 14 | auditParticleHot:1106 | d.begin(...) | B；const d=P |
| 15 | auditParticleHot:1108 | d.increment(...) | B；同上 |
| 16 | auditParticleHot:1109 | d?.endSpan(...) | Q；同上 |
| 17 | beginStepHot:1213 | d.begin('BEGIN_HOT') | B；const d=P |
| 18 | beginStepHot:1213 | d.increment(15) | B；同上 |
| 19 | beginStepHot:1215 | d.endSpan(...) | B；finally 内独立同值 guard |
| 20 | collectParticlePlanInto:1221 | d.begin('COLLECT') | B；const d=P |
| 21 | collectParticlePlanInto:1221 | d.increment(16) | B；同上 |
| 22 | collectParticlePlanInto:1223 | d.endSpan(...) | B；finally 内同值 guard |
| 23 | finishStepHot:1229 | d.begin('FINISH') | B；const d=P |
| 24 | finishStepHot:1229 | d.increment(18) | B；同上 |
| 25 | finishStepHot:1231 | d.endSpan(...) | B；finally 内同值 guard |
| 26 | copyPoolDiagnosticHot:1236 | d.begin(...) | B；const d=P |
| 27 | copyPoolDiagnosticHot:1236 | d.increment(8) | B；同上 |
| 28 | copyPoolDiagnosticHot:1236 | d.increment(21) | B；同上 |
| 29 | copyPoolDiagnosticHot:1237 | d?.input(out) | Q；restore 值不影响 null 短路 |
| 30 | copyPoolDiagnosticHot:1238 | d?.input(out) | Q；code/restore 值不影响 null 短路 |
| 31 | copyPoolDiagnosticHot:1239 | d?.endSpan(...) | Q；finally |
| 32 | applyParticlePlanInto:1245 | d.begin('APPLY') | B；const d=P |
| 33 | applyParticlePlanInto:1245 | d.increment(17) | B；同上 |
| 34 | applyParticlePlanInto:1247 | d?.endSpan(...) | Q；finally |

因此 32 个 **P 调用位点** 在 OFF_ALL/PROGRESS_OFF_EVIDENCE_ON 均不执行；第1项只有 E OFF 才能同判，第12项始终需要真实解析。不能把34项归成“33项由同一开关保护”。短路调用本身不等 callee 零分配。

第12项展开后的 Index 内部诊断也须全部覆盖：compare.increment；sift.increment/cursor；sort.cursor/begin/increment/循环cursor/finally.endSpan；remove.increment；rangeMin.increment（入口及两臂）；findBody.increment、TREE_BUILD/RANGE 的三元 begin、cursor/endSpan/increment；find.input/begin/finally.endSpan。I=null，经 const d=I 及 Q/B/T 短路。`minimum.fill` 对已有 Int32Array 存储操作，constructor 的 Int32Array 与 coldBuffers 返回数组**按从根是否可达判断**，不能按函数名排除。Index.find 仍执行排序/比较/树查询，非诊断模块，不能移出生产分母。

### 3.1 totals 与诊断 ON 的真实分配

`beginStepHotBody` 的 captureFailure 参数 `work.pool.totals().count` 调用会到 Field.totals:367 的对象字面量。它在 E=null 的 guard 内不可达；E开启且输入 FIELD_COLUMN 不合法时，在调用 captureFailure **之前就分配**。即使 captureFailure 内已锁存后立即 return，实参也已求值，不能声称“只分配一次”。M2.liveCount getter 是另一作用域的调用点，不能用它覆盖这条真实 Hot 失败边。

PoolProgressDiagnostic.checkLong:120 的 `new Uint8Array(buffer)` 是**视图对象的显式构造**，共享 buffer 不等没有分配；同段 `toString(16).padStart(...)` 产生字符串。每次首次阈值捕获可循环13个池buffer构造视图，并非“一次视图”。冷预分配的 raw 目标没有消除这些源视图。ON→cursor/endSpan→checkLong 是 Hot 可达链，不能称纯冷导出。它符合 T116 固定首超长窗口的诊断要求，但要独立计入开销，不冒零分配。

E.captureFailure 还包含数组、字符串、对象和视图；arrays 的首次懒绑定也会构造 DataView/Uint8Array。rowJson/exportJson/exportEvidence、构造器等同理：从 Hot 根无调用才能归冷，不允许方法名白名单。now() 注入时钟是外部回调，ON 图未知时记录未解析；SDK/运行时隐式分配本卡没有测量。

T166 修复一个已发现的浮点残差拒绝原因，不使全部 FIELD_COLUMN 拒绝逻辑消失；NaN、其它负分量和溢出仍合法地触发失败。不能以修复链代替 E 路径的门禁证明。

## 4. 主探针实施规格（选择 a，不写业务代码）

实施文件：`test/probe_p011_particle_hot_graph.cjs` 及独立 tester 的探针回归夹具/结果。维护原七模块、全部根、原60项身份计数与分配分类器。沿 import/真实接收者解析到新增三个模块及实际依赖闭包；不能只按同名方法全连，也不能把 index.find 记“已路由”就从 unresolved 减掉。

1. **输入与身份**：`audit({profile,sourceRoot})` 是建议测试接口，CLI 运行当前树；profile 是待证明配置的入口断言，不是可信豁免。读取 Workspace/Index 初始值、WaterSandField 冷创建/禁用/恢复/重绑对相关字段的实际写入，核对 OFF_ALL 可达且字段关系成立；报告这些源区间与hash。新增写入、未知别名或操作中可修改配置均失效，不沿用旧断言。原无参 conservative 输出/exit=2 保留为历史对照；新增 OFF_ALL 图独立给 verdict，不能修改旧 JSON 冒旧图 PASS。
2. **抽象值**：有限域 NULL/NONNULL/TRUE/FALSE/UNKNOWN，加对象/参数来源身份。支持 const 别名、已知槽读取、参数绑定、返回后控制流；传播 P→d→siftDown参数→beforeInOrder，I→d。声明为 nullable 或变量名叫 d 不能证明 null。关联不到确切 workspace 的对象一律 UNKNOWN。
3. **AST 控制流**：实现 if/else、提前 return、?:、&&/||/??、可选链的 JS 求值顺序与 try/catch/finally。NULL 可选调用跳过 receiver 后续访问和全部实参；未知条件两路合并。finally 不得因 try return 漏扫。赋值/别名修改使事实重新求值；循环求不动点，无法收敛有限界则 widen UNKNOWN，禁止截断后给 PASS。语法错误/不支持节点不能忽略。
4. **过程间**：工作项键含模块、类、方法、参数抽象状态及相关槽状态；对每个真实候选分派扫描，未知接收者/外部回调为 unresolved。到方法名相同的其它类不误连；用源码符号/receiver 类型或显式接口实现集合证明候选，不能新加 pure 名字或“诊断模块可信”豁免。配置改变的未知调用必须使逃逸槽事实失效。原 typed-array set/fill 存储规则保留，不能只靠同名属性冒 TypedArray。
5. **分配/registry**：在可达上下文保留所有现行 new/object/array/closure/template/string-concat/throw 分类，额外不认识的分配帮助调用仍 unresolved。new TypedArray(existingBuffer) 仍 constructor。registry 的60项必须逐条有 owner/位点身份和可达证据，不能删、重归文件或按 owner 白名单“冷点化”。同一 site 多上下文取可达并集；可达 allocation site 与 unresolved均输出完整根→调用→分支见证链。
6. **输出**：新增 `analysisRevision`、scope、profileFacts/proof、sourceManifest（commit/dirty/fileSHA/换行口径）、rootManifest、legacyRegistry、reachableContexts、prunedBranches（guard/抽象值/来源/实参被跳过）、allocationSites、unresolvedCalls、remainingSiteCount、registryComplete、status。仍须 **registry完整且 remaining=0 才 ZERO_ALLOCATION，否则 NOT_ZERO_ALLOCATION/exit2**；未知就是失败，不能以 ROUTED/NOT_IMPLEMENTED 填零。保守旧图与profile新图分字段，禁止用 profile PASS 改写旧图 verdict。
7. **ON 旁表**：同一源码列 PROGRESS_ON/EVIDENCE_ON 可达分配位点、触发界、冷初始化/热阈值/热失败/导出阶段与 unresolved 外部时钟；静态位点数不冒运行次数/字节/RSS/耗时。ON 不是主零分配profile，不要求强制 ZERO；副作用等价由 T155 动态测试证明。

建议合并报告只表达已有证据，例：`{legacyConservative:{status:"NOT_ZERO_ALLOCATION",remaining:35},offAll:{status:"NOT_RUN",remaining:null},diagnosticOn:{status:"NOT_RUN"},runtimeEquivalence:{status:"NOT_RUN"},productStability:"NOT_RUN"}`。实施后逐字段填实际值，主图 FAIL 时 `productionHotPathZeroAllocation` 不得为 true。分析器零显式可达分配的源级命题，不推广成设备运行时无GC/无隐式分配。

### 4.1 误报/漏报与必须的适配器反例

保守 UNKNOWN、别名逃逸、未知接口会误报；保持 FAIL 并扩展可解释的分析能力，不放开方法名。最危险漏报是只匹配 `if(d!==null)` 文字却不知道 d 的值、忽略调用实参、漏 finally/异常、跨上下文复用 NULL、同名方法错配、关闭 P 却假定 E 关闭。

独立 tester 至少注入并断言：guard 外 `[]`、传入非空 d、d 被重赋、两种 workspace、E ON 的 totals/captureFailure、未知时钟回调、同名不同类方法、finally 中 new、未知分支 new、index.find 内 new、配置反向重绑。都必须在相应可达 profile 失败。反向确认 `d=null;d?.m(new X())` 的实参不可达，但 `d?.m();new X()` 仍失败；if/ternary/早返三种等价写法结果一致；新增未解析依赖和registry缺一项必失败。用独立夹具验证探针本身，不用当前报告的“预期0”循环证明分析器正确。

## 5. baseline 路由及 T155 的开工/结论边界

| 项目 | 分流 |
| --- | --- |
| T149 captureFailure + totals | 纳入本次 profile/控制流适配器，保留 E ON 的真实失败分配。不是新生产泄漏修复，不等待 T146/T166 假定谓词消失；T149/T150 原有证据不重开 |
| T123 mergeIndex.find | 同次扩展解析完整生产调用图与内部 I=null 传播；不回退T123、不误派T135 writer纯核返工。T135报告并非这条索引首次实现归属依据 |
| T154 32处P调用 | 工程归因为静态分析覆盖/上下文缺口；OFF_ALL 局部短路证据成立，整图零分配仍待适配器实测 |

**T155 可以按 T154 已交付诊断和本版规格进入离线独立验证/适配器工作**；不要求先得到“待它自己执行的合并报告”才能开工，不新增manager批准或验收门。由 manager 按职责分配探针工程实施（developer或具备测试工具所有权的tester）；独立断言/反例和正式验证归tester，本卡不实施。若分开执行，适配器与原独立功能用例可并行，尚无适配器时如实标静态项 NOT_IMPLEMENTED/NOT_RUN，不宣全绿。

T155 报告能解除本次门禁缺证须同时给：实际待测树 OFF_ALL 主图 ZERO_ALLOCATION、0 unresolved/0 remaining且60项完整；适配器独立反例无漏放；诊断开关 field/pool/预算/事件等价与 T116规定的满/稀疏/同键/非法ID终止、真实seed/audit/λ/rollback乘数、attempt delta/计时/缺测、ring/dropped等独立证据。**不能把T154 8/8开发自测原样改名成独立测试，不能削减T155原卡内容。** 任一失败按真实结论回 architect，不预设必须代码修复。

T156 真机接续仍依有效包与T155无阻断结论，只等待设备窗口不算blocked；取得真实超长负载后回T116续判，不能从本卡门禁分流推“卡死已修复”。T161新增的GPU compile info log常驻采集需求应沿GPU原测试/修复链（T158/T164及实际接续卡）保留，与本CPU静态AST门禁分开记录，不将一次编译成功冒日志字段齐备。

所有后续执行prompt：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己状态有限例外），不改滴答同名清单；不回退共享修改。离线卡写“不使用真机”；设备卡保留真机标签和串行前置。渲染含背景前景仅新XComponent+shader，禁止Canvas上屏/失败回退；失败FAULT或明确STALE旧有效新管线帧。不得改变T154诊断语义、audit/事务次数、冻结阈值或测试estimand；manager登记交付与调度，不增加验收门。

## 6. 本轮核验边界与修订

只读原报告、JSON、源码与Git对象，逐条核对34项、两个独立开关、真实分配点和源版本；未重跑probe/业务测试，未编译、用机或改业务/测试代码。没有给当前图/产品指标PASS。

V1中“视图构造不算分配”“主门禁FAIL+路由就等于生产零分配已证”“方法名冷白名单”“T146修复后FIELD_COLUMN不可达”“T155不再核零分配/要先产本卡验证报告才能开工”均撤回，以本版证据与规格为准。本版与T116§4有界诊断合同一致：诊断ON允许披露额外开销，OFF按实际null控制流证明；不改变物理或把诊断开销抹掉。
