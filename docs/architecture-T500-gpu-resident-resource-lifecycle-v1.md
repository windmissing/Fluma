# T500 · GPU驻留资源峰值与长期生命周期工程盘点

2026-11-14，architect，P5，不使用真机。只读盘点T492最终M1/M2生成源、T485 graphics工厂与T475 store，不改容量、性能阈值或现有开发关闭范围。[资源算术/来源索引](analysis-T500-resource-inventory-v1.json)保存四档逐对象账、原件SHA与签包核对；[编译审计](analysis-T500-host-layout-v1.txt)仅取同源host对象布局，不链接或运行GL。

## 1. 来源与计量口径

实际来源是 `.codex/local-test/t492-m1-delivery` 的m1和 `.codex/local-test/t492-m2-delivery` 的m2route，不混共享App/头和冻结cpp。重算签包HAP/包内ELF，均与T257及T492收据一致：M1 HAP `801c2d63…a1b0`，M2 `c6ba4032…0ce8`；overlay manifest的现存native文件逐项匹配。compute/物理按T492 unchangedPhysics记录及冻结实现读账，不拿工作区新实现覆盖旧包身份。

账目分开保留，不合并成一个虚假的“总内存实测”：

- **GPU请求payload**：应用glBufferData/glTexStorage2D所请求的逻辑字节；同一GLuint别名只计一次，API删除不证明驱动物理页立即回收。
- **host拥有的存储**：native定容块、vector容量、cache binary、文件worker副本、JSON/JS冷导出。CPU停投不影响对GPU管线这些必要host资源的盘点。
- **磁盘**：binary header、payload、receipt、原子临时文件和导出档，单独计，不把磁盘128MiB又加成GPU分配。
- **driver/system未知**：program/JIT、GL对象/同步器元数据、对齐与分配粒度、退役存储、swapchain/compositor、allocator缓存、GPU/RSS重叠归属。现有报告没有这些物理峰值，均NOT_MEASURED。UMA上RSS/GPU计数可能重叠，不直接相加判机型资源不足。

## 2. compute常驻对象与四档账

两路线实际App都绑定RootIsolation。设K/Kw为批准四档，pad为256或512：

```text
W = 16K + 16Kw + 2304 words；S = 4W
P = 4(16K + 20Kw + 2304)                 单publication
Es = 256(K+1)                           单旧SOURCE64 arena
E = 256[4(48K+16Kw+254)+K]              单QUANTUM64 arena
SW1 = 4(96+4096×64+W+4096×16)
Q0 = 32+pad+2Kw+127×16+512+K+Kw+61444+4096×16
QW1 = 4(Q0+W+256)                       已扩大的workspace，替换旧store
B1 = 3S+2P+3Es+4096+SW1+QW1+3E
SW2 = SW1+8pad+33280；QW2 = QW1+32768
A128 = 512(2K+1)
B2 = B1+3A128+8pad+33280+32768
```

M1共17个GL buffer：Source两个state bank、两个旧SOURCE event、一个SOURCE scratch、两个publication、两个source result、一个source WORK，共10；Quantum candidate、expanded WORK、两个publication event arena、一个隔离scratch arena、两个full result，共7。`quantum.events`是其中eventArenas/scratch的别名；接受时交换句柄，不产生第四份常驻E。Source结果和full结果是独立对象，不能合并漏计。M2再有SOURCE128双arena及scratch，共20个buffer；旧SOURCE64/event/work保留，不能以新协议不消费为由从实账删除。

| K/Kw | M1常驻B1 | M2常驻B2 | 每份QUANTUM E | M2 SOURCE128三份合计 |
|---|---:|---:|---:|---:|
| 192/64 | 34,551,504 | 35,210,960 | 10,795,008 | 591,360 |
| 192/128 | 37,728,720 | 38,388,176 | 11,843,584 | 591,360 |
| 512/64 | 82,374,608 | 84,019,152 | 26,605,568 | 1,574,400 |
| 512/128 | 85,551,824 | 87,196,368 | 27,654,144 | 1,574,400 |

单位B。M1约32.95–81.59MiB，M2约33.58–83.16MiB。M1与T433算术一致；M2与最终`m2/layout_generated.h::kProfiles.payloadBytes`逐档一致，不以单对象`GL_MAX_SHADER_STORAGE_BLOCK_SIZE`能力检查代替整组同时分配成功。

compute缓存覆盖SOURCE、中间QUANTUM、最终QUANTUM三个role，**稳态只有两个compute program**：Quantum::Create先建中间program，再被Full::Create删除并替换为最终program；Source program保留。加三graphics，稳态五个program、两个scene VAO。替换/编译期间新旧program/shader可能暂存或被GL延迟删除；其GPU/JIT字节未知，不能用binaryBytes代GL program内存。

## 3. 合法峰值须按同时存活阶段计算

### 3.1 冷准备、FBO与resize

`ColdPrepareOptions.twoBatches`走PrepareColdPrefix，临时再分配**一整个E**，不是借用常驻隔离scratch；成功/失败/候选失效均在原owner清理coldFence/eventScratch并恢复quantum.events。普通pass0预热没有这份额外arena。source Create的host cold/workspace vector为`S+SW`，返回后销毁，不能同时算成所有四档常驻副本。quantum.workspace扩大时再次glBufferData，旧store被替换，应用可见一个buffer新payload；驱动可能短时保旧store，另作retired backing未知，不能把统计器的增量误当两份永久WORK。

Scene的FullFrameTargets有两张单层SRGB8_ALPHA8 texture和两个FBO，逻辑payload为 **F=8wh**，无本类depth/stencil或mipmap副本。Ensure在尺寸改变时先Reset再allocate；当前代码**没有应用拥有的完整旧双FBO＋新双FBO事务副本**。同context delete后仍被命令引用的旧store、默认EGL surface的颜色缓冲均属driver/system待测；不把旧新共存从账里无条件删掉，也不将其伪称已知应用8wh双倍。分配半途失败Reset清本次两句柄，不把残缺candidate发布CURRENT。

冷额外arena只在transport Create内，先于Scene program准备/READY/首次Draw建立FBO，当前正常调用路径两者不同时活跃。因此一个context的**已知对象payload高水位**为：

```text
普通路径：B + F
twoBatches路径：max(B + E, B + F)
另加program/GL元数据/driver退役/默认surface未知；不是 B+E+F 的实测峰值。
```

1280×1609仅作可复算例，F=16,476,160B（约15.71MiB），不是绑定设备最大尺寸：

| K/Kw | M2运行B2+F | M2 twoBatches与运行两阶段max |
|---|---:|---:|
| 192/64 | 51,687,120 | 51,687,120 |
| 192/128 | 54,864,336 | 54,864,336 |
| 512/64 | 100,495,312 | 110,624,720 |
| 512/128 | 103,672,528 | 114,850,512（约109.53MiB） |

resize若将来改为保旧完整帧直到新双FBO成功，须另修工程合同：峰值至少B+8woho+8wnhn，并保失败FAULT/真实STALE语义。本轮不授权该改造、不改纹理分辨率或池容量。

### 3.2 reset、失效与旧context

T492 App retire先记录root/lease墓碑，销毁Run，给GPU ContextLost并销毁host对象，Scene::Reset(false)忘记graphics记录，再在GL owner上unbind、destroy surface、destroy context、terminate，最后释放held window引用；**retire全部成功才创建新context**。任何EGL步骤失败置retirePending，下一owner循环重试；不能在旧context未退休时再启动一个完整新B。这是正常reset/窗口更代下“一份应用context资源＋退休状态”的责任边界，不是“两份B常驻”的默认假设。

ContextLost只使句柄/许可失效，不调用GL删除。此时GPU资源由旧EGL context负责，不能见host对象被free就减成物理0。旧root/lease/fence不再接受或Swap；正常显式transport Release则必须同owner且无在途root、无未退release fence/lease，BUSY不能被当清理完成。驱动在eglDestroy返回后的内部退役是否仍占页，需真实长序列测量，不凭API返回猜无限堆积或立即归零。

一个实际缺口：loop最终退出只调用一次`retire()`且忽略false，shutdown EGL清理失败没有后续owner循环重试；控制侧没有可复核的最终资源墓碑。静态App通常在进程结束时退出，这不证明正常reset泄漏，但host多App/故障退出须单独验证。不能在UI线程补GL清理或“报成功归零”。

## 4. host常驻、诊断副本与M2冷trace

### 4.1 同源host布局与不能重复相加的项

MSVC x64 `/Zs`检查T492完整生成controls TU，原选项/头链，无GL执行：

| 项目 | host sizeof B | 计量边界 |
|---|---:|---|
| App M1 / M2 | 2,365,824 / 2,365,872 | 已含live timeline及1,404,072B diagnostics，不能再加一遍 |
| 独立TimelineSnapshot | 754,200 | 含固定payload753,664＋metadata536；App只持unique_ptr |
| EvidenceCapture每块 | 2,818,864 | 4096定容event/token；不是maxBytes=1MiB的序列化限额 |
| AppTransport M1 / M2 | 182,640 / 183,672 | host对象，不含它拥有的GPU store/vector动态容量 |
| M1 Run | 3,320 | M2及设备ABI另测，不用共享类拼接 |

`capturePrepared/evidence/captureRetired`并非默认三份：初始一块prepared；首次ARM移入evidence，supervisor再预备一块，长期通常两块。下一次ARM是旧evidence移retired、prepared移evidence，worker在锁外**先destroy retired再分配新prepared**，保持两块生命周期上界；超时/取消未ARM请求只占固定request，不新建一块capture。临时存储缺失返回STORAGE_NOT_READY。需要把移出App的worker局部指针也计入，否则会误报释放后又分配的瞬时空洞。

M1的已知host对象项`App+snapshot+transport+Run+2×EvidenceCapture`为 **8,943,712B**，未计C++动态字符串/vector/cache/全局create诊断、allocator和线程栈；初始一块时为6,124,848B。它们是host布局参考，**不是ARM64设备RSS或native完整上限**。固定snapshot End/60s TTL仅撤使用权，不free这754,200B；reset后旧snapshot保原scope，不能把同一payload又按每个run累计。

`executionDiagnostics()`每次另make_unique复制1,404,072B，再通过ostringstream、out.str、NAPI/JS string转换形成瞬时副本。reader的2MiB diagnostic/总4MiB是导出接受预算，且在native完整序列化/JS字符串已创建后才检查，**不是native或JS分配前硬上限**；不得声称“一份snapshot意味着所有冷导出只占753664B”。页副本只含4行及4对token（约9KiB＋metadata/字符串），旧live前缀另在App内，算过一次。

T492 diagnostics仅四scope、首root/首QUANTUM各768槽、首错尾64、UI96/packet64/reset32/stage128，满后计dropped而不扩容。超过四个reset后新scope细节不足不等于没有故障；长序列必须有独立资源ledger，不能只查firstCause空而报PASS，也不能自动清掉历史首错维持“完整”。

### 4.2 M2ColdTrace是真实额外host大项

M2 App有`shared_ptr<const M2ColdTrace>`，在owner合法cold checkpoint Freeze/Capture后保留完整state/events及completion。成功替换前旧trace仍在，**没有End/TTL，reset或新request的scope失效也未清已有trace**。这是跨run保留的冷档，不是GPU arena复制，也不是TimelineSnapshot。

有效QUANTUM trace最多`T=S+E+1024`B，SOURCE128 trace通常更小；K512/Kw128时T=27,705,344B。当前单生产者且无并发旧reader时，新旧替换可达2T=55,410,688B逻辑vector有效数据，vector容量/allocator另计。readM2Trace在锁内取得shared_ptr、锁外序列化，读者可继续pin旧trace；若允许并发native查询，通式为`T_current+T_candidate+ΣT_pinned_old`，当前没有显式并发读者/退休byte预算，不能宣称任意调用下硬上界2T。原id/epoch仍标旧scope，不冒当前结果；旧trace保留本身不证明泄漏，但会抬高长时RSS基线。

## 5. program cache：预算、真实样本、pin及写临时

T475 Store为进程/模块内单worker共享，T485 graphics不新建另一份32条/128MiB缓存。每条binary payload≤32MiB、entries计数≤32、entries payload合≤128MiB；receipt每key≤128KiB、提交JSON≤60000B、队列≤128。Store::Get/Receipt/WRITE_PENDING持shared_ptr会pin同一Artifact：**pin不复制payload，也不能把每个receipt都加32MiB**。Reserve只淘汰use_count==1且非WRITE_PENDING项；全部被pin时Put返回CACHE_NOT_STORED或写失败，合法GPU program仍保留，不能为满足缓存强制丢物理/渲染。

实际T492离线cache收据的六role binary样本：M1合 **792,671B**，M2合 **859,015B**，共享三个graphics role合64,671B；这些是HOST环境binary样本，不证明设备binary大小或GPU program内存。32MiB只是允许的单条上限，不能按六role固定实占192MiB，也不能把两路线共九个不同key默认视为同进程同时加载。

需要分阶段累加的临时项：

| 阶段 | 同时存活的额外host payload | 回收者 |
|---|---|---|
| GL导出/重复key Put | 一份实际binary向量b≤32MiB；move入Artifact通常不复制buffer，拒存/旧key返回时临时销毁 | GL caller/Put |
| 文件worker写binary | entries中的b仍pin，另wire=b+144B，再原子.tmp写到磁盘 | worker lambda返回 |
| 冷预载ReadArtifact | wire=b+144B＋待Reserve的a.bytes=b；在Reserve前可能与原entries共存 | worker函数/Reserve |
| 排队receipt | 128×60000B约7.32MiB文本payload上界，外加key/function/allocator；同Artifact共享pin | serial worker |
| receipt历史重写 | history≤128KiB＋当前text，再wire≤128KiB及临时拼接副本 | 单worker |

例如steady写入与下一GL导出并发，保守请求payload可达128+32+32MiB＋144B；正常启动预载也可在Reserve前出现128+32+32MiB＋144B。**192MiB是这些假设下的上界算例，不是实占或全进程硬上限**。若配置失败后重试预载与GL导出并发，需再计该owner导出向量；并发API/元数据/JSON/容量及allocator未纳入。不能以128MiB store限制证明所有native峰值≤128MiB。

磁盘DiskRoom把已提交.bin/.jsonl、header及incoming临时长度放在同一128MiB/32key预算中；同key覆盖旧档保留到Atomic成功。worker串行，正常一次一份.tmp；不再另给32条各一份并发写临时预算。Configure清理上次.tmp，Atomic失败会尝试删除本次.tmp。但删除失败被忽略，后续DiskRoom不统计残留.tmp，**故障路径的磁盘总量还需补证/修订**，不能拿正常路径预算作故障后全目录硬界。任意外来文件/文件系统journal也不在该逻辑计数内。

graphicsRecords上限32且Scene::Reset(false)也ForgetGraphics；compute pending/qualified各上限32，但Source/Quantum::ContextLost不走Forget，可能保留旧context收据到被替换/计数淘汰。它们不是GPU句柄有效性证明或无界binary泄漏，长序列要按“有效对象0＋有限历史收据”分别判，不要求cache/历史map/RSS回到进程启动零值。进程Store退出会join并排空worker，慢I/O可延迟退出，不能用根250ms deadline替代文件清理期限。

## 6. 工程缺口与分流合同

以下是**独立后续范围**，不扩T475/T485/T492已交开发卡，不改变现有业务endpoint、shader、容量、时间或物理语义。manager按本稿建接力卡；architect本卡不实现业务代码。

| 项目 | 已知事实与最小接续 | 角色 |
|---|---|---|
| R1 资源观测不完整 | UiState::knownGpuPeak保留App生命周期内已观察buffers＋FBO最大值，NewRun只清当前bufferBytes/frameBytes、不清max；它不含冷暂存/退役/host账，Stats也不是删除后的live账。增加有界owner资源ledger，按object+contextGeneration记create/replace/delete/tombstone/retired；host按ownership类别记size/capacity，外部driver仍UNKNOWN | developer单独工程卡；tester先可用host GL转发ledger补证 |
| R2 M2冷trace保留及读pin | 给独立冷trace生命周期修订：清晰的release/失效scope、旧reader pin上限、替换失败保旧或拒新策略、累计vector capacity；旧V3冷trace兼容/新revision拒判明示，不静默改旧id语义，不改SOURCE/QUANTUM endpoint | architect合同后developer；tester先记录当前两代及pin状态 |
| R3 cold诊断瞬时副本 | 在序列化前限制并发复制、bounded writer在构建中检查预算，超限返回显式INCOMPLETE且保首因/原错及snapshot finally释放；旧reader不将截断JSON当完整。不能以增大2/4MiB预算或删除首错绕过 | developer另卡，版本化冷诊断契约；tester边界补证 |
| R4 shutdown/失败清理回执 | owner退出保存未退EGL stage/句柄/原码及资源墓碑；正常reset先退旧再建新保持。错误清理仍只owner，最终失败报告未回收，不能host free即归零；不得无限join或在control强行销毁context | developer，精确EGL失败注入由tester |
| R5 cache故障临时与元数据 | 补.tmp清理失败的byte计数/启动收敛及满队列/全pin拒存回执；历史compute记录按context失效与binary缓存分开。保现有32/128MiB限制与合法GPU program，不能把缓存故障升级为物理数据丢失 | developer/tester独立；先复用已交store fault原件 |

R2/R3的详细新冷协议由对应architect接力稿冻结后再实施，不在本次盘点中暗改旧协议。R1需输出固定schema：scope/ownerTid/phase/objectKind/objectGeneration/requestedBytes/liveBytes/retiringBytes/logicalPeak、alloc/dealloc失败与原错、host类别/容量/高水位、cache entries/pin/queue/tempBytes、trace generations/readPins、snapshot active/payload、capture块ownership。字段必须标LOGICAL_REQUEST/HOST_ALLOCATED_CAPACITY/DRIVER_NOT_MEASURED；计数在实际成功/失败切点更新，不靠状态字符串推导。不能在hot路径做文件写、JSON或全粒子读回，不能以诊断采样跳过原GAP。

## 7. 独立长序列测试prompt

tester，P5，不使用真机；使用T492完整一致的M1/m2route source及实际GL，host ANGLE仅离线设施。先核HAP/ELF或host exe/逐TU来源、四档K/Kw、shader与cache helper；复用T494来源约束，不混共享头。新增测试目录与每run独立原件，不覆盖旧卡。

1. 八组合（两路线×四档）各32个固定cycle。每cycle同源状态/有限动作表，至少SOURCE接受＋非空QUANTUM与真实publication使用。八个确定cycle分别在未封batch、fence待signal、已接受待Draw、Swap前后、持publication lease、冷准备、cache I/O、导出中执行reset/窗口失效；正常cycle做有效resize与同尺寸Ensure复用。两次首warm稳定后比较相同checkpoint的实际活跃句柄/请求容量，不以总分配次数永远累加判泄漏。
2. 首次Create分别覆盖普通与twoBatches，断言活跃17/20 compute buffer与对应payload；冷暂存+1，不与本代FBO混算。resize至少A→B→A及非法尺寸/第二张FBO失败，核无残留candidate、无错误CURRENT、在途publication正确退休。App更代旧context未退时不得出现新context分配；模拟每一EGL退役步骤失败再恢复，shutdown失败保最后stage。GL删除挂起的物理store只作driver未知，不用name计数冒回收证明。
3. snapshot Begin/BUSY、跨reset旧scope、End/TTL、取消/hide/IO失败/坏页，32cycle中反复执行；payload地址/数量恒定1，ID不复用，End后存储仍常驻是预期。capture预约重复/timeout/取消/ARM/冻结/替换与supervisor预备块交接，跟踪App与worker局部，最多两块EvidenceCapture；不能只查prepared/evidence字段而漏retired局部。
4. M2在SOURCE及大有效QUANTUM后各取trace，旧trace跨reset保留、在替换/失败/取消/慢reader时核identity及shared ownership；当前协议记录保留，不把应保留旧档判泄漏。若并发读取没有硬界，输出具体pin数/byte及NOT_BOUNDED_BY_CURRENT_CONTRACT回architect，不自行扩大预算。新版本R2完成后再按其明确上限验证。
5. cache先复用T475已有重启/损坏/原子四故障、40key淘汰、receipt80轮、4×32MiB pin及unpin替换；T485复用跨context Forget/候选失败清理。补并发慢worker至128队列、同key重复Put/Reject、满pin拒存后合法program仍可用、临时wire高水位、清理.tmp失败再启动恢复。binary尺寸使用实际值和注入store容量两类分报，不把合成32MiB条目当设备program。不同目录/新进程隔离真实缓存原件，明确KEEP_CACHE和新context规则。
6. D1/D2及capture四scope溢出后，用独立host资源ledger连续观测，不要求旧定容诊断无限完整；未知/丢失/未退项必须明确。检查冷JSON最坏满槽序列化、副本并发和cancel finally：超预算可诊断交付但不能PASS完整，不改数据补造缺页。外部父进程做每阶段wall守护，ENTER/EXIT/资源差额先落盘，杀子进程保现场、不在损坏context续跑。

通过条件为：有真实业务正控；同代资源有可解释有界高水位；每种失败后已知活跃/退休账平衡或明确未完成，旧代无接受/Swap；cache/快照/capture数量及合法持久基线守界。32cycle是本次独立测试量，不修改产品容量/期限。RSS曲线保进程/线程/采样时点，允许allocator/cache热身平台；持续增长须给对象/ownership或driver证据，不凭曲线拟合宣布泄漏/机型不足。纯计数不能证明设备物理峰值。

后续若要设备长期资源证据，另建带“真机”标签tester卡，同设备串行，消费准确T492或后续已签包；按上述相位采实际进程RSS/PSS与可用系统GPU/driver计数，缺工具明确NOT_MEASURED，不代跑CPU oracle、不以离线软件GL上屏。复用T495已有root/reset现场，本轮不重跑其业务、不把新长测作为旧开发关闭门。

## 8. 本卡交付与共同边界

所有接力prompt须明确：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用既有oracle。背景前景全部XComponent+shader，禁止Canvas/软件GL产品回退，故障FAULT或标STALE上一完整有效帧。33,333,333ns GAP、250,000,000ns root deadline、120Hz quota保持，显示30fps目标不变。成功原卡回执并本人行结束待验收；失败/关键缺证原卡写未完项、复现原错、证据、尝试、不能自行继续原因、前置与接手角色，本人行看评论。

本轮完成实际分配/保留/释放链、四档账、同时存活峰值与回收责任、已有覆盖复用及独立长序列prompt。设备/driver物理峰值和长期是否收敛仍NOT_MEASURED，不能凭预算猜资源不足，也不把静态缺口当真机泄漏已复现。只进行原件/签包/公式及host语法布局检查，未改业务或测试源码、未跑GL/设备。
