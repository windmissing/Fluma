# T138 GPU确定配对原型工程 V1

2026-10-26；architect；原卡 `6adf6a09e4b017ab3407f962`。不使用真机，不运行测试，不实现业务。GPU路线当前最高调度优先；T123 CPU索引继续。消费T137全稿、离散模型和证据，T130、T136，以及当前T123/Hot接口。T137证据的11项输入SHA与本轮磁盘文件全部匹配；4916池等计数仅为designer模型结果，不是本轮重跑或设备结果。

**ENGINEERING_READY_FOR_T141**：下文冻结隔离原型协议；T141仍按原任务等T139修订harness及T140独立oracle，不把本卡工程完成冒全部前置齐备。T137已有确定性、唯一写入、原RN64范围及最小pair证明，本次未发现需退designer的算法矛盾。生产route、renderer context及CPU权威事务不迁移。

## 1. 文件与依赖所有权

| 所有者 | 文件/责任 |
| --- | --- |
| T139 developer | 通用compute context/capability、barrier/wait/map错误、资源恢复、冷热与逐轮记录；保留其独立文件，不让T141改写T131历史probe冒修订 |
| T141 developer | `test/p013_compute/pair_wire_v1.h`、`pair_prepare_v1.{h,cpp}`、`pair_executor_v1.{h,cpp}`、`pair_owner_v1.{h,cpp}`；`glsl/pair_build_v1.comp`、`pair_query_v1.comp`、`pair_reduce_v1.comp`；独立`CMakeLists.txt`及离线driver/build脚本 |
| T141 developer | `test/p013_compute/ets/`请求打包/原事务测试适配；`test/p013_compute/tools/generate_hot_test_variant.cjs`构建期生成隔离测试变体；测试专用native binding/包只放此目录；`docs/developer-T141-gpu-pair-prototype-v1.md`回执 |
| T140 tester | 独立oracle/fixtures/预期结果及报告，路径沿其交付；T141只读消费，不复制GPU比较/查询实现冒独立性 |
| T142 tester | 真机同机串行，归档`test/reports/t142/`逐请求/attempt/环境/资源/原始错误/统计；图形竞争使用既有新渲染管线 |

不修改`entry/src/main/ets/physics/P011ParticleHot.ets`、`P011MergeExactIndex.ets`、M2/WaterSandField、生产NAPI或renderer/CMake/shared hunk。原型独立CMake target `p011_gpu_pair_prototype`；通用harness以T139正式公开接口接入，不臆称其未交付代码已可链接。T139接口到件可用薄adapter映射本文接口，不改wire/算法；若同步语义冲突回architect，不自行删错检。既有SignHap密码配置故障保持原码和范围；不得把离线compile/PackageHap当已签设备包。

## 2. 调用接口与CPU精确准备

以下为测试专用API，不是生产endpoint变更；输出容器均冷分配：

```cpp
PrepareStatus prepareExactRanges(const FrozenSnapshotView&, double radius,
    const RequestIdentity&, CpuSlot&, PreparedView& out);
ExecStatus executeIntegerPair(const PreparedView&, ContextSlot&, Deadline,
    ResultWords& out, RequestDiagnostics& diagnostics);
ConsumeStatus verifyAndConsume(const PreparedView&, const ResultWords&,
    CurrentOwnerState&, VerifiedPair& out); // 不写权威物理状态
void cancelOwner(OwnerGeneration); // 逻辑失效；物理回收按§6
```

`FrozenSnapshotView`沿P011ParticleSnapshot字段：nmax/cols/liveCount/freeCount/nextId；5个Float64Array的mass/x/y/vx/vy，3个Uint8的phase/source/state，id/sourceId为Uint32，freeSlots为Int32，2个列质量Float64Array。所有数组长度/offset/byteLength先检验，拒绝越界、错类型/共享可变别名；active由原`pState!=0`决定，不按mass猜active。复用原snapshot校验的质量/状态等约束；active ID互异且1..UINT32_MAX（跨group重复也拒绝），slot与active记录一一对应。非法输入INPUT_INVALID，不混为NO_PAIR。任何支持域缩减须显式拒绝，不静默过滤记录。

准备保留group `(phase,sourceKind,sourceId,state,floor(x))` 原字段精度，按group、Y数值、无符号ID、slot全序排序；±0数值相等；group字段不拼易碰撞hash，不把floorX截成int32。每group按T137§2的upper_bound(Y<=ya)划两段，再用原`abs(RN64(ya-yb))<=radius`分别二分，得到半开[L,R)。严禁rounded(ya±radius)、epsilon或F32高度。有限输入相减溢出为Infinity时按原谓词false，不能错误拒绝原本合法有限snapshot。

ArkTS原binary64值按位传递；native使用IEEE binary64 RN、保留次正规、禁止fast-math/FTZ/DAZ和改变表达式顺序，构建记录相关选项/环境。环境不满足报ENV_UNSUPPORTED，不用放宽容差。CPU排序、全局ID唯一检查、范围生成和全部封送均计时，不将其藏在冷配置。每a验证`G0<=L<=aRank<R<=G1<=n`。排序/范围不导入T123或T140实现，避免oracle和被测共享缺陷。

Nmax固定于冷配置，K=pow2ceil(max(1,Nmax))，h=log2K，不随active n改变。n=0/1仅CPU合法预检后返回NO_PAIR，执行身份CPU_EMPTY_SHORTCUT，GPU dispatch/fence/readback均0；不混成GPU收益。每个非空请求重新生成全范围和level0。

## 3. Wire：P011_GPU_PAIR_WIRE_V1

冻结为little-endian uint32字，字节偏移=word index×4；std430仅uvec4数组，不依赖C++ struct padding。Header256=64 words，Result128=32 words；C++静态断言长度，ArkTS用DataView显式LE读写，不转Float32或将uint64塞入Number。64位serial用{lo,hi}，冷配置检查增长溢出，溢出拒绝并换owner，不回绕复用身份。主机/GPU不支持约定字节表示时报ENV_UNSUPPORTED。

### Header256逐字段（输入，只读）

| words | 字段 |
| --- | --- |
| 0–3 | magic=0x47505031、wireVersion=1、headerBytes=256、resultBytes=128 |
| 4–7 | algorithmRevision=1、cpuRangeRevision=1、layoutRevision=1、flags（本版仅bit0正确性诊断，其余0） |
| 8–11 | Nmax、n、K、h |
| 12–15 | ownerGeneration lo/hi、contextEpoch lo/hi |
| 16–19 | payloadSerial lo/hi、poolMutationSerial lo/hi |
| 20–23 | attemptSerial lo/hi、substepSerial lo/hi |
| 24–27 | candidateRole、candidateOrdinal、slotIndex(0/1)、W=64 |
| 28–31 | requestToken[4]：ownerGeneration lo/hi、requestSerial lo/hi；owner代在本run不复用，请求serial单调且不回绕，manifest保存两者；跨run还须校验完整run身份 |
| 32–39 | snapshotSha256[8]，每连续4个摘要原始字节按LE组成一word |
| 40–47 | payloadSha256[8]：恰按Leaf16[K]接Query32[K]的上传原字节，不含未写表层 |
| 48–55 | identitySha256[8]：下述规范identityBytes的标准SHA256 |
| 56–63 | reserved=0，收到非0拒绝版本 |

算法字符串对应`GPU_SEGMENTED_ID_PAIR_V1`、`CPU_RN64_MERGE_RANGE_V1`，与route `P011_GPU_PAIR_PROTOTYPE_V1`、wire版本一同存manifest。candidateRole固定1=seed collect、2=λ candidate、3=preview、4=authoritative apply、5=isolated pair fixture；调用方按真实来源选择，不同角色/重试的ordinal独立递增，不以同一token复测冒同request。

`identityBytes`按固定顺序编码：3个revision与route的UTF8 length-prefixed字节、run/state/config完整UTF8标识、paramsRevision/inputRevision、seed、owner/context/payload/pool/attempt/substep的原64位值、candidateRole/ordinal、slot、Nmax/n/K/h/W/flags、requestToken16B、radius的F64原始8B、snapshot/payload两个32B摘要、构建manifest SHA256。paramsRevision/inputRevision/seed先沿原合同验证，再各用原binary64的8B编码，不作uint32截断；其它非serial整数均u32，serial为两u32。变长字段以u32 LE长度前缀，整数按LE、serial低word在前；全部长度先校验，不使用任意JSON序列化顺序。构建manifest归档完整源码/GLSL/二进制hash与编译选项，不能用旧tinysha。

snapshot摘要覆盖以上snapshot所有字段，标量按固定schema编码（nmax/cols/liveCount/freeCount为u32，nextId为原binary64 8B，其余schema padding置0），13数组各带字段ID/原byteLength再接完整原字节，半径另在identity内。inactive槽、free-list有效与未用区字节均不漏。摘要只作关联；CPU保留完整identity、冻结snapshot及owner生命周期，不能用hash概率代替不可变所有权。

### Result128逐字段（仅最后reduction写）

| words | 字段 |
| --- | --- |
| 0–3 | magic=0x47505231、layoutRevision=1、status、flags=0 |
| 4–9 | idA、idB、slotA、slotB、aRank、bRank |
| 10–11 | completedDispatchCount、errorStage（成功0） |
| 12–15 | requestToken[4]原样回显 |
| 16–21 | contextEpoch、payloadSerial、poolMutationSerial各lo/hi原样回显 |
| 22–29 | identitySha256[8]原样回显 |
| 30–31 | completionMarker=0x444F4E45、reserved=0 |

status：0 NOT_READY（每request清结果全128B，magic/layout可初始化但marker必须0）、1 PAIR、2 NO_PAIR。PAIR要求1<=idA<idB<=UINT32_MAX且slots/ranks合法；NO_PAIR的4–9全0，用status区分合法slot0。GPU/GL/协议错误不伪造NO_PAIR；host ExecStatus另报INPUT_INVALID/CAPACITY_UNSUPPORTED/ENV_UNSUPPORTED/GL_ERROR/TIMEOUT/WAIT_FAILED/MAP_FAILED/UNMAP_FAILED/STALE/CANCELLED/PROTOCOL_ERROR/ORACLE_MISMATCH，保存原始GLenum和阶段，结果均不可消费。

manifest/CPU还须核owner仍活、slot为本请求、当前scratch未变、run/state/config/attempt/substep角色完全对应；回显subset加identity摘要不能单独授权消费。marker不是内存同步替代品。调试计数/分项GL耗时放host diagnostics，不挤128B或增加隐式result版本。

## 4. SSBO绑定、各层与dispatch

每slot六个独立buffer：binding0 Header256只读；1 Table16[K*(h+1)]；2 Query32[K]；3 CandidateA32[K]；4 CandidateB32[K]；5 Result128。std430分别声明uvec4数组；一个shader只声明其使用的block。查询用4 blocks，最终reduce只需header/input/result；配置检查全局binding数>=6及各program实际compute blocks、最大block字节、workgroup X/总invocations>=64、shared>=2048B，不能把T131的16 compute blocks当binding点数。

各buffer整体绑定，不以layer的16K offset直接bindRange绕过GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT。若adapter采用suballocation，须另外对每个range做checked align并上报额外字节；不能沿用本文无padding预算。所有CPU索引/字节乘法用checked宽整数，先验证uint32 shader索引、GLsizeiptr、SSBO及配置容量后创建；热循环不得扩容或降N。

记录/查询布局严格沿T137：Leaf/Table每条`uvec4(id,slot,valid,originalYRank)`；真实前n，padding valid0/rank=i其余0；Query32两向量为`(id,slot,aRank,L)`及`(R,G0,G1,valid)`，padding两向量全0。全序 `(1-valid,id,slot,rank)`，无符号逐字段，不以ID差转signed，不以MAX_ID当empty。

1. upload仅header256、table level0的16K、query32K、result reset128，总48K+384B。Table高层与候选不必每次清零，因本次所有K个位置均被确定写；这条覆盖必须由独立证据验证，未完成链不得读旧层。
2. build l=1..h：每次ceil(K/64)组，读取`(l-1)*K+j`，按T137另一半lower_bound，将x唯一写到`l*K+segmentStart+rSelf+rOther`，禁止原地读写层别名。每层后SHADER_STORAGE barrier。层参数用已界定的uint uniform，不改header或重传叶表。
3. query：ceil(K/64)组，每a分解原Y秩[L,R)，在层段ID表二分首个valid且id>idA，选最小b；不能用idA+1。padding invocation写invalid候选。Candidate32为`(valid,idA,idB,slotA)`、`(slotB,aRank,bRank,0)`。写A后SHADER_STORAGE barrier。
4. reduce：W64，每组共享64×32=2048B；输入越界lane装invalid，**所有lane参与组内barrier**，禁止early return。固定全序min，lane0唯一输出；input/output不别名。长度依次K→ceil(K/64)→…→1，A/B轮换。非末轮写候选后SHADER_STORAGE barrier；最后一组lane0直接构造整个Result128（含status/identity/dispatchCount/marker），不另起finalize dispatch。
5. 最后写result后BUFFER_UPDATE barrier，尾fence/flush、有界wait、map128B、复制并核unmap。整request仅1个尾fence/128B结果读回，不每层CPU map/fence。完整表诊断另模式、另预算和计时。

主档：192用K256/h8，8 build+1 query+2 reduce=11 dispatch；512用K512/h9共12。n<=1短路0；其它K≥2按h+1+ceil(log64 K)实际循环计算；K1不启动GPU。CPU记录提交计数与Result回显比对。padding/table层跨group并非错：只能取完整包含在本group范围内的标准段，不能整取跨边界segment。

## 5. T139适配与错误时序

adapter必须提供owner线程绑定的`createContextAndLimits`、冷program/buffer准备、逐阶段GL错误采集、`finishReadback(buffer,bytes,deadline)`及retire/reset，不准用返回bool抹掉原错误。barrier类别与完成等待沿[T136](architecture-T136-compute-arrival-decision-v1.md)及[Khronos GLES3.1 §7.11/同步对象章节](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)。CPU wait计时绝不标GPU runtime，GPU timer不存在或disjoint时NOT_MEASURED。

执行前归档既有GL错误为阶段preexisting并停止本请求，不能先清再当NONE。每个上传/dispatch/barrier/fence/map/unmap取原始错误和阶段。glFenceSync失败不进入读取；glClientWaitSync只对ALREADY_SIGNALED/CONDITION_SATISFIED放行，TIMEOUT_EXPIRED仅在未到deadline时继续有界轮询，到期报TIMEOUT；WAIT_FAILED原样报告。deadline是冷配置记录的执行保护值，不是新增性能门槛；每次实际wait timeout及host轮询次数归档。glMapBufferRange为NULL或unmap FALSE均失败，先复制的数据也禁止消费。program编译/link日志和limit拒绝原码保存。

context失效令epoch递增并使所有prepared/result失效；重建后重新检查limits、编译/分配，归档新epoch资源，不复用旧GL名字/旧fence。恢复后的真实算法差分由T142验证，harness能重建不能冒原型已恢复。GL调用只发生在当前独占owner线程，不借用XComponent context，也不在UI提交后同步等GPU。

## 6. 双slot、不可变所有权与迟到

slot状态：FREE→PREPARED→IN_FLIGHT→COMPLETED→VERIFIED→RETIRED→FREE；错误/取消则IN_FLIGHT→QUARANTINED。每owner同一时刻只有一个非空request执行，另slot是失效隔离储备，不允许两个依赖merge并跑。owner冷配置保存run/代、context epoch、token序列、slot实际GL对象；CPU槽与GPU槽一一配对。

冻结之后直到消费/退休，snapshot、leaf/query/header、manifest与相关scratch不被其它调用写。成功消费后CPU原merge修改scratch，poolMutationSerial递增，所有旧prepared/index/result逻辑失效；任何birth/其它scratch修改同样递增，不能因n未变复用表。新λ/preview/attempt等即使物理值相同也取新请求身份。

超时/owner取消后GPU可能仍跑：逻辑丢结果，slot不能立即reset/upload给新请求。完成可确认后安全删除sync和退休；不能确认则停该context后续工作并按T139销毁/重建隔离，严禁无限new context/slot绕过。新owner不可使用旧context未排空的slot；迟到只记STALE，不写新snapshot。进程清理/驱动挂死也不能冒recovery成功。GL对象live/peak数和隔离期间CPU/GPU占用实际归档。

## 7. CPU权威事务与完整attempt接入

生产`mergeOldestHot`现由`work.mergeIndex.find`选slot再调用私有`mergePairHot`。最小原型不改其生产函数；测试构建期生成隔离Hot变体，仅将**选pair的来源**替换为测试native selector，保留原mergePairHot/preparePatch/publish/restore/discard/finish、原M2调用顺序和算术。生成器归档源SHA、替换位置/规范化diff、生成文件SHA，强制拒绝白名单之外的语句变更。依赖闭包的import仅重定向到测试变体，以免完整attempt偷偷仍走生产CPU selector；不在真实生产构建启用测试宏。输出只进测试构建目录，禁止覆盖原源码。

ArkTS测试driver在独占Worker/等价测试owner内执行整事务，native测试binding在同owner线程维护独立compute context，单次同步调用覆盖准备/封送/链尾读回；UI只送有界测试输入并收不可变报告。若当前测试包/运行时无法承载这条原事务闭包，先报明确工程阻塞，不用C++重写merge替代原算术后宣称全事务等价。原型package/ability与生产路由隔离；部署/签名由设备任务记录实际结果。

每个原merge调用：冻结当前patchScratch→准备范围/上传→GPU建议→身份/epoch/slot/rank/原group/原F64谓词重检→原mergePairHot恰一次→mutationSerial++。PAIR输出须同原扫描/T123最小pair一致；合法性重检不等最小性证明。NO_PAIR沿原`P011_HOT_INVALID_PATCH_STATE`/原调用方失败路径，不变成成功empty提交。GPU错误沿失败诊断中止测试事务，走原restore/discard/audit，不提交部分权威pool/预算/nextId/ledger/lineage/free-list或preview。无自动CPU重算冒GPU成功。

完整attempt固定同before/config/input/seed，分别运行旧扫描、T123精确索引、GPU三个测试变体；各有独立原事务存储/相同输入，比较每次pair/no-pair/status及最终所有pool字节、ID/free-list、预算/退款、事件/lineage、rollback与publication。GPU不执行任何物理merge算术。批量birth、tail、seed collect/apply、各λ失败/重试、多preview、authoritative apply按原顺序每调用重新请求，不能合成一次batch替代；记录真实M，不从容量推合并次数。

## 8. 固定资源预算与峰值记账

T137 shader payload每slot：16K(h+1)+32K+64K+384；两候选buffer仍各32K，不擅自压缩造成预算/实现版本不符。CPU每slot冷分配：冻结snapshot数组55Nmax+16cols B；标量编码预留64B；8份uint32工作索引共32Nmax B（group/Y排序、ID排序、排序临时、slotToRank、G0/G1/L/R）；Leaf+Query staging为48K，header/result staging384B。CPU已知载荷合计87Nmax+48K+448+16cols，不含语言对象/allocator padding和原事务其它副本。

| Nmax（cols=128） | GPU每slot/双slot B | CPU每slot/双slot B | 两者已知双slot小计 B |
| --- | --- | --- | --- |
| 192，K256/h8 | 61824 / 123648 | 31488 / 62976 | 186624 |
| 512，K512/h9 | 131456 / 262912 | 71616 / 143232 | 406144 |

这不是总RSS或driver显存。T123索引额外12Nmax+8K B，主档4352/10240B；原before/after/scratch每副数组55Nmax+16cols及其plan/actions/journal/ledger必须按实际活跃副本另计，三路对照顺序跑不强制同时常驻。sha计算须流式复用冷staging，不每request拼接new巨型buffer；冷manifest、输入队列、构建后程序binary、日志/逐轮时序容器均列固定上限/实际峰值/失败策略。热区不new Map/Set、不创建program/buffer或每merge扩容日志。

实际峰值=上述已知载荷+原事务/基线活跃载荷+alignment/桥复制+program/context/driver开销+诊断buffer+线程栈/日志；驱动不可测部分标UNMEASURED，不能以表内小计冒整机峰值。shared2048B是每在途workgroup，不擅自当每slot一次。每slot6buffer，双slot12buffer；3program可按同context共享；单在途尾fence正常live<=1，timer query/诊断对象若开另计。销毁后检查本context对象live归零，资源失效与pool不提交同等记录。

## 9. 计时、独立验证与开发任务书

冷配置记录context创建/能力查询、编译link、buffer首次、测试包/构建身份；warm定义资源持久且不重建。每request原始单调时钟区间涵盖freeze、CPU验证/排序/range、hash/pack、跨语言桥、upload、11/12 dispatch及barrier提交、尾wait、map/copy/unmap、身份/谓词重检。CPU等待与提交可能重叠GPU执行，GPU timer有效时只作旁证，不重复加进墙钟总计。外层attempt从排队/owner领取前约定起点到原提交/失败清理完成，明确同时给service与含queue时延，包含CPU merge/账本/全部失败λ/preview，不能将分项p50相加冒attempt p50。

正确性模式开T140独立oracle与逐层回读诊断，额外读回/复制计数单列；性能模式可关闭仅用于证明的全扫描/全层dump，但身份、原谓词、错误处理、正常SHA准备、实际桥/CPU排序/上传/等待不得静默删掉。T123基线同样标明计数/日志开关；三路线同设备/构建/完整输入，以192/512各早配对/晚配对/无配对、稀疏/满、多group、连续merge、批量birth/λ/preview为主，4096只诊断。冻结真实负载未交则注明synthetic，不冒T94精确重演。

T140提供独立原扫描/逐层排序与区间集合、边界fixtures，不共用GPU key/range实现。验点沿T137 GP01–GP12：MAX_ID/signed边界、±0/ULP/次正规/相减溢出、rounded端点反例、重复ID非法、padding/唯一写、跨group段不误取、连续质心/slot退役、身份注错/迟到/timeout/恢复。缺原型时预期RED登记NOT_IMPLEMENTED，不当产品失败或通过。T141完成开发/编译及不涉及真机自测即可交付；设备端到端不作为开发卡关闭前置。

T142须验证真实整链barrier/reduction/结果回读、同设备三路逐pair及全事务、原始冷暖每轮身份/失败/outlier/分位算法、总p50/p95/最坏/错误率、UI/输入延迟与渲染竞争、资源/context恢复。先做定点正确性再收益，不重索已交T131版本/map。只有完整attempt相对T123改善且批次误差支持、最坏/UI/原资源硬门不破才可建议采用；无新百分比/容差门，仍须另立生产工程。原型不采用也是有效结果，不能换F32、batch merge或减少preview去赢。

**给T141的最小可执行顺序**：读本wire并做布局/端序自检→接T139冷context/错误/retire接口→做CPU精确prepare及T140离线fixture比对→实现三shader与最后reduce直写结果→做slot/epoch/identity注错和不消费验证→隔离测试Hot变体接原事务与T123/旧扫描三路→离线编译/自测、归档生成diff/源码/GLSL/构建与manifest、报告尚未执行的设备项。禁止绕T140复制designer模型当独立oracle；算法不符精确回designer，工程不符先回architect修文档。

所有下游prompt必须包含：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己状态有限例外）；不更新滴答同名清单，不回退他人修改。设计/开发/离线oracle明确“不使用真机”；设备任务打真机并串行。所有渲染含背景前景只走新XComponent+shader，禁止Canvas上屏或失败回退；失败明确FAULT或保留标明STALE的上一有效新管线帧，不伪装当前成功。

本轮只交工程合同与最小任务书，无生产/GPU实现、无设备测试或性能结论；manager登记推进，不新增manager验收门。
