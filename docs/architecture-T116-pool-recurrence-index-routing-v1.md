# T116 池优化设备复发、精确索引与计算路线 v1

日期：2026-10-26。architect。原卡 `6adf3c1ce4b017ab3404b7c5`。不使用真机。消费T94 009c43d原始日志/两栈/ledger、T92/T96/T97与52a0700实现。**部分交付：复发工程热点及可等价索引方案已明确，真实单attempt负载/唯一首因仍缺关键计数；本卡回执看评论**。

## 1. 复发成立与R3证据纠正

R1-appfreeze-192032.log PID48057、R2R3-appfreeze-192151.log PID48742均为APP_INPUT_BLOCK，样本落在T96版mergeOldestHot→seed→**liftByShake**→applyMotionChannels。与T78属于相同函数链热点，但源码行/实现已变，不能称逐字节同栈即同一循环进度。两臂足以确认T96等价短路没有解决真实设备硬故障，不重开T96/T97或推翻其限定离线等价结果。

R3实际PID49203；其R3-freeze.txt和R3-full.txt的19:22:51 THREAD_BLOCK记录均指**上一臂48742**。执行器Check-Alive只全局grep关键词未过滤PID/run，故“R3也同栈卡死/三臂全复发”没有独立R3首因证据。R3新日志有49203的pan后续记录；不将上一进程fault混入它。现有R1/R2栈是lift来源，**不能据此把手势inject→seed直接触发认作已证**。

本轮流式读取R3-store.ndjson：51,982,038 bytes、264行，单run `p011-run-1793013742752-3`，context.sourceEpoch全0、shakeEnergy全0、particles.count全0；最后sequence/substep264、frame132、commitStatus=committed。它仅是**已写入窗口**，未含卡死时满池或merge调用，不能用它构造真实冻结池状态。

同run/PID49203的FENCE/CLOSED原时间为**19:22:29.068**，sent/written=264/0，requestId=ui:1；R3接受的pan从19:22:36之后出现。R4脚本19:23:23检索到了历史CLOSED，不是“进程死后才新发CLOSED”。页面finish设置captureFinished，frame停止物理、stageGestureSandAndFeed会跳过inject；accepted只说明前级gate，因此**R3零源与提前收尾一致，是需核采集时序的候选原因，不是已证被seed吞掉**。为什么ui:1提前触发需命中/UI动作证据，不能凭坐标猜系统或产品错误。

FENCE和已拉ledger存在是正证据，但没有合法双文件/最终manifest归档可签整套收尾PASS。脚本原pull 0B、fence.json不存在的manifest错误保留；空admissions只有在路径存在/真实bytes/hash及same fence核对后才等同合法零记录流。不得绕权限取得成功标记，不重测或修改T66来掩盖采集问题。

## 2. 计算量、复杂度与尚缺实测

T96每次merge按ID升序找a，再扫N找最小合法b；无匹配可N轮。每轮还扫N找a，**诊断mergeCandidateChecks只计b内层，不含a选择扫描/其它开销**。最坏单merge计数N²：N192=36864、N512=262144；T96报告N512=2097152是其8次λ重试累计，不能说单次N512有209万，亦不能把2.1s整体runner当纯merge耗时。

精确总负载应报告：`总检查=Σ实际子步 Σ实际preview/commit Σseed Σλ尝试 Σ满池birth merge检查`。birth按各phase的floor(mass/pkg)+tail，跨列seed总数已含列数时不能再乘128；REQUESTED/CLAMPED/OLD至多3次preview另有authoritative apply，真实次数由trace计；不同候选可能不同pool和birth。每seed还含冻结/copy、一次birth collect、patch/apply/audit/导出，不能只统计merge。

T94未开启/归档完整POOL_PROGRESS与入口状态，当前栈不提供N、birth、seed、重试次数或阶段wall time。容量192只是报告中的假定，本次ledger明记capacityLimit512，冻结时实际N仍需trace。现有事实支持同步昂贵路径阻塞UI的强工程候选，**不证明纯计算量是唯一首因、某次确实执行最坏扫描或设备比host慢10倍**。有限循环仍可能因状态/调用放大异常耗时，需计数与入口/出口进度区分，禁止盲加速度系数或直接提高阈值。

## 3. 可等价工程改造：每次merge重建范围最小ID索引

不改变匹配谓词或选对规则，新增定容 `P011MergeExactIndex`（可在Hot同文件或纯逻辑模块）。输入仅**当前patchScratch**，N冷配置固定。维持字典序最小(pId[a],pId[b])、aID<bID；group完整键为 `(phase,sourceKind,sourceId,state,floor(x))`，不可漏state/sourceId，也不能把mergeColRadius误加进现有同列条件。

一次merge的确定步骤：

1. 冷分配并复用两个N长slot索引和排序scratch；扫描active槽，校验已有有限/ID唯一不变量。按group tuple、pY数值、pId、slot全序做原地heapsort（或定容归并，明确额外scratch），得到group连续、各group内Y升序；另一个索引按全局pId升序。
2. 建立以Y排序位置为叶的定容线段树，叶保存当前粒子ID及slot，inactive为sentinel。只作整数最小ID查询，绝不改变质量或坐标；group边界及slot→position直接数组。使用id=0为EMPTY且查询专门处理，不能与Uint32最大合法ID混淆。
3. 依全局ID升序枚举a。先从树移除a；此前所有更小ID已移除，树恰包含ID>a的未访问粒子。仅在a所在group查Y区间。区间不能盲用rounded `y±radius`，用生产原谓词 `Math.abs(y_b−y_a)<=mergeHeightRadius`：在Y≤y_a半区二分首个满足点，在Y≥y_a半区二分最后满足点；有限同域Y下差/绝对值随排序单调，保留原RN求差、±0及等高含端点语义。NaN/非法radius沿原配置/状态拒绝。
4. 对该连续区间做range-min-ID查询，得到最小合法b；第一个存在b的a即全局字典序最小pair。调用**原mergePairHot**，加权更新顺序/质量/ID/freeSlots/journal/lineage不变。无pair返回原INVALID_PATCH_STATE，不新加retry/fallback。

正确性：group精确编码原等值谓词；Y范围精确编码原唯一高度条件；树只留ID>a且range-min给最小b；全局a升序给最小a。没有近似bucket量化、概率hash匹配、批量同时merge或基于冻结旧质心的假等价。group排序只影响查找，不影响任何浮点材料计算。

每次merge从scratch重建，时间O(N log N)、空间O(N)，最坏无匹配也O(N log N)。**不是整步O(N log N)**：B次merge仍O(BN log N)且有λ/seed/preview放大；端侧是否够快仍须测。初版不跨merge缓存，避免成功merge的质心/质量/state、spawn/free-slot/ID改变后索引失效；后续增量树若要持久化，必须对所有mutation/rollback定义generation与undo，另版工程证明，不能为了性能沿用旧索引。

资源显式计数：排序slot/ID顺序/slotPosition/groupBeginEnd各定容O(N) Int32/Uint32，树叶容量pow2ceil(N)、节点<4N；总字节按实际数组列出，warmup后不new Map/Set/对象/无界容器。索引完全派生于scratch，失败/rollback丢弃索引内容即可，权威pool/预算/账本仍原回滚；不因为查找失败偷偷合并不同来源或丢弃birth。

此为严格等价选对的工程方案，可先developer实现＋独立差分，不需要先改物理设计。若具体实现不能证明原谓词范围/ID全序、或需修改merge语义/批次分配，则明确退designer，不能打“等价优化”旗号扩行为。

## 4. 三条计算路线的成本与选择

| 路线 | 解决什么 | 工程成本/证明边界 |
|---|---|---|
| CPU精确索引（推荐先做） | 降无匹配搜索复杂度，保留原权威库存/单事务顺序 | 定容排序/范围最小值、差分pair/status/rollback证明；仍测全seed及调用放大，非自动设备PASS。 |
| native C++＋独立物理线程 | 可减少解释/桥开销并避免UI同步长任务 | 单次NAPI整体输入/结果，独占物理owner；一个事务在工作线程完整执行、双缓冲不可变publication再交UI，不能跨帧拆半；触摸/传感器/config按既有消费身份排序，有界队列及取消/epoch策略；需跨语言RN/FMA/ID/预算/日志等价。native本身不消O(N²)，仅搬线程可能造成延迟/积压。另案设计/工程，非本卡默认实施。 |
| GPU compute | 适合足够大、可并行且同步成本可摊销的负载 | 需要确定的pair reduction/冲突、sequential merge后的重新配对、ID分配、预算退款/失败rollback、CPU权威ledger与GPU状态同步、SSBO/barrier/fence/读回开销；不能把N²循环原封搬shader冒修复。当前N512未证明GPU收益。 |

依据[Khronos GLES3.1规范第17章](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)，compute有独立dispatch/资源/同步合同。当前代码gl3.h与major3创建请求不能证明真实设备支持ES3.1 compute；本卡不操作真机，不虚报能力。若manager选择研究GPU，独立设备卡先查询真实GL版本/compute与SSBO上限、可编译的最小kernel和同步代价，带真机标签串行。GPU线程中原F64/严格数值合同与CPU权威事务须显式设计，不启用未授权另一套数学。

## 5. 最小developer任务书与真实计数补证

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身状态有限例外；不改滴答同名清单，共享代码不回退。所有画面仅新XComponent+shader含背景前景，无Canvas上屏/回退，失败FAULT或显式STALE旧有效新管线帧。失败/关键证据不足在原卡评论并自己行看评论，成功结束待验收。

文件/hunk：Hot merge查找与新纯索引模块、冷configure/warmup/storage检查/资源计数、现有POOL_PROGRESS有界扩展；M2.seed/WaterSandField.lift仅接已执行阶段计数不改物理；页面gate→clock→request→inject及captureFinished跳过原因沿T96诊断复用。分别标记origin=GESTURE_SPAWN/SHAKE_LIFT、parent attempt/candidate/λ序号、实际N/active/B/seed、sort/tree query/访问计数、copy/collect/apply/merge耗时、energy前后/派生控制、错误码/首进度。不要将原全局计数重置/聚合后误称某一attempt。

独立tester以52a0700旧搜索为oracle，覆盖随机池/对抗无匹配、两phase/source/state/sourceId/列、高度边界相邻ULP、相等Y、稀疏active/最大合法ID、连续merge质心移动、tail births、λ重试、journal/lineage溢出、失败回滚。逐次pair及后续pool/事件/预算/ID/free-list/status完全相同；索引计数证明每merge上界，外部子进程timeout保留，host秒数不推设备。不能只用ID1/2易配对例证明无匹配优化。

真实冻结池仍需精确取证：先用既有T94日志按PID/run重分析，列R1/R2缺的最后pool snapshot/实际seed/B/λ；无输入不假造精确重放。设备补采由manager另派窄卡，首超长attempt仅导出固定计数/身份与合法可得快照，诊断不能使UI多打一份无界数组；不绕权限，不自动GPU重写。

## 6. 后续与交付状态

2026-10-27到件增补：T124/T123/T128已交不重索；T129原始R2还含新索引merge栈，R3提前CLOSED污染对照，最新边界及窄补证合同见 [T116 V2](architecture-T116-device-audit-index-arrival-v2.md)。下述为旧交付时状态，不据历史等待重复派工。

已确认R1/R2优化后复发＋已交可等价O(NlogN)查找工程方案；R3独立冻结、各阶段真实乘数/耗时与唯一首因**关键证据不足**，本task按新规则看评论。T57仍等新修复交付和独立稳定性；不能用同函数栈或新host绿灯签设备通过。完成新索引后由manager派对应设备复测，按实际就绪分类，不加manager验收门。

本轮只读报告/流式ledger与源码，未重跑测试、未用真机、未改业务/测试/阈值。原T94 FAIL/日志保留，新归因限制不替用户/manager改旧卡判定。
