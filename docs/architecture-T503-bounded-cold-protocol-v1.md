# T503 · 冷trace生命周期与诊断序列化有界合同

2026-11-14，architect，P5，不使用真机。接续T500 R2/R3，以T500所核T492增强delivery的首QUANTUM/faultRoot/call起止helper为基线；handoff M1及m2-handoff-v2较旧helper只作兼容审计，不能丢增强字段后称同源。物理wire仍M1/M2原版。机器合同见 [T503 manifest](architecture-T503-cold-manifest-v1.json)，[来源/静态核对](analysis-T498-T503-contract-checks-v1.json)留证，与M3物理V4无关。本卡只冻结协议/所有权/实现与检查范围，不实现业务，不扩大T492/T485关闭范围。

## 1. 版本与兼容选择

新增独立冷能力`T503_BOUNDED_COLD_V1`，trace响应`T503_M2_COLD_TRACE_V1`，所封存payload明确`T445_M2_TRACE_V3`，保存原completion256、state及SOURCE128/QUANTUM64语义。M1调用trace明确UNSUPPORTED_ROUTE；M1/M2都支持新diagnostic协议。普通SOURCE/QUANTUM/lease/scene、live timeline和T492 immutable snapshot协议不变，snapshot仍允许跨run读旧scope。

T507交付启用新冷能力的构建时，迁移所有在线冷trace调用方到新API；**旧requestM2Trace/readM2Trace与旧executionDiagnostics在线入口明确返回协议升级拒绝**，不能保留另一条无限制分配旁路。返回完整小JSON `state=UNSUPPORTED_COLD_REVISION, required=T503_BOUNDED_COLD_V1`，不返回旧TRACE_READY/旧档结构。旧V3档案和旧reader离线保留；旧reader对新schema必须拒未知，不重写历史档。需要保留旧在线能力的历史构建另标build profile，不宣本有界合同适用于它。此为显式版本迁移，不对旧V3 trace id暗加TTL或冒成功兼容。

新native API（U64均十进制字符串；对象/数字参数按实际NAPI适配）如下：

```text
coldCapabilities() -> revisions/routes/limits
coldTraceBeginV1(expectedScope, replaceId="0") -> requestId/traceId/status
coldTraceStateV1(requestId) -> state + original trace scope/error
coldTracePageV1(traceId,page) -> immutable header/page or explicit terminal
coldTraceEndV1(traceId) -> release/terminal receipt
executionDiagnosticsV1(expectedScope, remainingUtf8Bytes) -> bounded full/envelope
```

scope包含PID、route、实际source声明、epoch/run/context/surface/session/resetSerial/permissionRevision；包/ELF由外部签包收据核，不信自报source当安装证明。expectedScope任何字段不同拒STALE_SCOPE，配置/读者未知revision先拒。page0只描述header，后续每页最多1024U32，UTF8响应≤16,384B；checked words/count/stride/row对齐，不能根据candidate未审计计数分配。标准trace大小上限`Tprofile=stateBytes+max(SOURCE128 arenaBytes,QUANTUM arenaBytes)+1024`，四档最大27,705,344B；ordinal/descriptor/hash与原completion保持。

## 2. trace两slot与一个reader，累计capacity先预留

App一个ColdTraceManager，两个slot，状态FREE/BUILDING/READY/RETIRING；全manager同时最多一个builder、一个正在序列化的reader。**所有已分配slot capacity累计≤55,410,688B**（两倍现四档Tmax），不是两份每次再临时复制的vector。每slot一个精确容量U32 owner block，布局completion→state→events；允许复用较大capacity，free/retiring/building都计入总capacity。不得vector自动扩容、先new后记账，或在GL Capture的旧vector再复制到slot。

申请过程在短控制锁内核scope、单builder、replaceId及reader/slot状态，预留所需capacity信用，锁外allocate/free与owner GL Capture。若需换slot容量，先释放该FREE slot旧block并更新账，再分配；没有足额信用就WOULD_BLOCK。bad_alloc明确TRACE_ALLOCATION_FAILED，归还预约，不撤业务许可、不伪造0字节成功。root/lease忙时保REQUESTED至真实owner安全点，不在UI线程Freeze/map。

生产Capture增加仅冷路径的caller-owned有界span适配，写本slot的state/event区，返回实际counts；复用原GPU freeze/check/copy/unmap/restore顺序和错误，不改变热提交。容量先按合法profile校验，completion声称超容量则TRACE_FORMAT_REJECTED，不能信它扩内存。map失败、unmap失败、Freeze恢复失败保原GL/EGL/阶段；若原产品恢复路径要求FAULT照原合同，不把故障降格成成功导出。

替换策略：已有READY时Begin必须携带其准确replaceId，否则TRACE_BUSY；只有一个空闲slot才允许候选。旧READY在候选构建期间仍可读；候选任何错误/取消/过期不换published指针，旧档只要原scope/TTL仍有效就保留。成功发布在锁内核原scope/许可和request后原子换published；旧slot无reader则FREE，有reader则RETIRING，不能提前复用。第二个替换若两slot占用或已有builder返回WOULD_BLOCK，不新增第三份存储。

Page在锁内取得全局reader许可及slot引用，锁外编码一页；许可和slot引用不能逃逸NAPI函数，所有异常finally/RAII释放。另一个Page/trace读者或同call重入返回COLD_READER_BUSY，不排无界等待队列。caller只得到值拷贝；上一页JS字符串由caller持有不pin native slot。scope变化/End/TTL若发生在读期间，slot转RETIRING，回包前重核scope有效性，失效返回完整STALE_SCOPE/ENDED/EXPIRED envelope，不把旧页标当前。先生成的小页缓冲仍受16KiB限制，无Use-After-Free。

## 3. End、TTL、reset、失败终态

requestId/traceId用各自单调U64，非0、不复用；溢出明确ID_EXHAUSTED。TTL为**request创建起60,000,000,000ns**，REQUESTED/BUILDING/READY均受限，读取不续期。单调时钟倒退作本冷请求CLOCK_REVERSED，不重置业务clock。TTL检查在每次cold入口及既有supervisor冷维护点执行，不做GL；BUILDING过期只设cancel，owner从当前GL返回后完成unmap/restore并丢候选，不能从控制线程杀GL调用。声明过期检测可能迟于60s，但过期后不得成功发布。

Begin通过准入后同时分配requestId和预留traceId，失败也不复用；尚未发布的Page返回TRACE_NOT_READY。End(id)撤该trace可读资格；对REQUESTED/BUILDING的预留id即取消该候选，已发布旧replaceId不受影响，owner归来只清候选。READY→FREE，读pin未归还则RETIRING；保最多32条定长terminal回执，使近期重复End幂等，不影响新trace。已被覆盖/不在terminal表的旧id返回STALE_TRACE_ID。End不改CURRENT、root/lease、物理账、quota或业务source，不唤起成功root。未结束FREE block可保留复用，但仍计capacity；明确trim时仅free无reader/FREE块，不能虚报payload已free。

reset/run/context/session或permissionRevision改变，撤旧manager scope的read/publish资格，pending请求取消、READY转FREE/RETIRING，builder归来只能丢弃；归还reader前不得复用slot。已保存的文件仍带旧scope，可离线读；这是新冷协议的scope失效规则，**不用于T492 snapshot**。hide/cancel由collector结束自己的trace/request，不能释放其它id。取消/失败request保原码、first failed stage和原scope，不能沿用上一READY id冒新请求成功。

## 4. diagnostics复制前门与构建中预算

独立DiagnosticExportGate每App最多一个活跃export，**在复制1,404,072B Capture之前**检查expectedScope/单export/预算。trace Page的一个reader许可与此gate分开记，允许各一个，但不能产生多个完整Capture副本。调用无等待队列，重入/并发返回DIAGNOSTIC_BUSY。并发校验状态在App短锁；分配/JSON/NAPI在锁外；状态重核、异常、NAPI失败均RAII归还gate。

remainingUtf8Bytes由collector传入`min(2,097,152,4,194,304−trace已成功落盘UTF8字节)`，不足16,384先返回预算拒绝，不先copy大块。native不相信大于2MiB的值，回显实际limit。只有一份Capture副本＋一个**capacity恰等于limit**的BoundedUtf8Writer，典型最大3,501,224B（副本1,404,072＋writer2,097,152），另有固定小控制结构；各目标编译期核sizeof(Capture)≤1,404,072，变化超界先修合同，不默扩预算。无ostringstream无限增长、out.str再复制或native隐式多份JSON。不把这个请求payload界冒全进程/JS/driver峰值。

writer每次append前做checked UTF8字节信用，处理JSON escape的最坏6倍、U64十进制、原始UI字节十六进制、逗号/括号/终结；所有escape完整原子写，不能半个码点/半个数字/破JSON。输入是定容Capture的原始point/firstCause，不从前次JSON拼字符串。正常完整输出须`actualUtf8Bytes≤limit`，记录完整计数/overflow/dropped，不能因预算不足省字段仍写COMPLETE。

预算耗尽：清当前构建长度，利用预留的**16,384B错误envelope空间**（包含在limit之内）重新构建一个完整JSON，`state=INCOMPLETE, reason=DIAGNOSTIC_BUDGET, dataComplete=false`；保四scope已有firstCause的reason、scope/root/permit及原始GL/EGL/stamp、faultRoot身份、各固定计数/截断事实。不得返回看似合法的“成功前半段”或制造替代首因。工程helper编译期核firstCause fallback最坏UTF8长度≤16KiB，实际最坏值用真实helper覆盖；变更结构不满足则修合同，不提高2/4MiB额度。

copy/writer分配失败先从现存锁存结构复制小的firstCause/fault envelope到定容栈缓冲，返回ALLOCATION_FAILED完整小JSON；如果NAPI连小字符串也分配失败，传播原异常且释放gate，不声称已落盘或已返回COMPLETE。NAPI直接从writer.data/length创建JS值，原字符串生命周期到调用返回；JS/NAPI内部副本和caller保留历史档不受该native gate约束，需在长测分列。

scope在copy与回包间变化时，允许回一个明确`scopeStale=true`的旧scope诊断档，不能贴新scope；collector标INCOMPLETE_SCOPE_CHANGED。firstCause在copy时原样锁存，查询不能重置/覆写它。物理firstCause与“诊断导出失败原因”是两个字段，不以DIAGNOSTIC_BUDGET替换GPU_TIMEOUT/GAP。

## 5. collector、文件与旧reader边界

迁移T507两页面实际SnapshotPort及coldtrace调用方，先读coldCapabilities；遇升级拒绝终止该冷导出并保存完整reason，不重试旧无限制API。T492 timelineSnapshotBegin/Page/End仍原协议，collect finally始终End；diagnostic BUSY/超限/IO/cancel不得漏掉snapshot释放。

每页≤16KiB、trace＋diagnostic≤4MiB、diagnostic≤2MiB、整个timeline导出30s沿原值，以**实际UTF8落盘字节**计，不能只用UTF16.length。budget不足保先前文件/拒页原字节及SHA，最后manifest写INCOMPLETE；没有足额空间保原拒页时明确缺证与原因，不覆写旧文件。新M2全state/event trace独立cold导出：原始有效数据≤Tprofile、页数≤ceil(Tprofile/4096)＋1，累计JSON文件字节≤该页数×16,384，仍受request起60s TTL且不续期；不能把原始U32字节数冒JSON字节预算。它不把几十MiB trace混进原4MiB timeline导出，两个manifest/revision/byte账不得混同。coldtrace逐页保存，不在JS聚合完整trace数组再序列化。

new revision的header核payloadRevision、root kind/stride、scope、actualWords/rows、completion身份及byte预算；V3离线reader只读V3旧档，新reader拒未知。文件done/COMPLETE只表示导出完整及校验通过，不等于数值、显示或设备性能PASS。

## 6. developer实现范围与必要检查

T507，developer，不使用真机。新增定容ColdTraceManager/DiagnosticExportGate/BoundedUtf8Writer及机器常量生成，接入实际T492 M1/m2route生成器、NAPI cold能力和两页面collector；M2 span Capture只在冷路径。禁止只改共享App却遗漏实际生成源；按T494逐TU统一、签包/ELF/源hash登记。旧在线入口明确拒绝、旧离线reader保留。compile/必要非机检查/签包即可交付，独立真机不是关闭门。

实际helper检查，不仅mock envelope：两slot所有状态转换；read pin中replace/End/reset/TTL、两替换失败保旧、容量新申请前拒且无第三份；request/trace U64耗尽、相等TTL、倒序clock、wrong scope/id/page/stride；最大四档counts与溢出completion拒；Map/Unmap/恢复失败保原错；writer正好limit/超1字节、全部escape/最大U64/四firstCause、fallback完整解析与首因逐字段相同；分配失败和NAPI异常释放gate；并发/reentrant返回BUSY且复制计数不增；无旧API无限制旁路；collector字节界/IO/cancel/hide/unknown/最后manifest失败仍释放snapshot/trace。

独立tester长测接续复用T500八组合×32cycle中的冷档子集，跟踪**capacity、所有slot及读者/owner局部**，不是只查published指针。断言trace累计≤55,410,688B、builder≤1、reader≤1、diagnostic copy≤1、native writer≤2MiB，End/TTL可保FREE容量但不再可读；跨scope旧尾不发布。保allocator/RSS/driver未知，不以计数证明物理立刻回收。旧T493/T492控制/reader覆盖按来源复用，新schema测试独立，不重开旧卡。

共用prompt：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用旧oracle。背景前景只XComponent+shader，禁止Canvas/软件GL产品回退；失败FAULT或标STALE上一完整有效帧。250,000,000ns/GAP33,333,333ns/120Hz quota/30fps目标不变。成功原卡回执且本人行结束待验收；失败/关键缺证写未完项、复现原错、原件、尝试、不能继续原因、前置及接手角色，本人行看评论。

本卡协议/所有权/预算/兼容与接续冻结完成，未实现或运行新helper/GL/真机，不声称内存长测通过。
