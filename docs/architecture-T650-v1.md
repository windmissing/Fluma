# T650：V5 等待页面脉冲与控制器创建首次归因

2026-11-19，architect，不使用真机。结论：**报告因果勘误 + 配置生命周期观测缺口 + 已确认FULL准入遗留门**。已完成本卡归因/工程分流；尚不能定位本次设备首失败，不把G13类比或黑屏当root cause。正式表`architecture-T650-contract-v1.json`、保存原件/包源证据`analysis-T650-evidence-v1.json`；有限只读复核用`analysis-T649-T651-v1.py`，未构建/运行产品或真机。

## 1. 原件支持什么

T594签包83486466…的真实输入是bc4c5ec3…，包内libgrsm1.so为92fe79a3…/3169352B，签前后该ELF及modules.abc字节相同。T619开发收据记717e8650…，与这次输入HAP不同；这是同路径后来重建产物，不能把旧收据当本包完整源映射。所列m3b原生七文件与当前/隔离树SHA可匹配，reader不在隔离entry树；这不证明整个generated App bridge、page/公共输入适配、CMake/宏/依赖或最终链接一致。实际运行模块路径/BuildId/进程启动身份也未在这批原件完整建立。

页面原件只保m5Configure的M5_NEW_EMPTY_RUN_REQUESTED（boot254766824872815/diagGeneration2/attempt4）；冷档part记录同boot/generation的m5ColdCapabilities attempt1361返回M5_NOT_READY。后者是**后一次只读API调用**，不是配置attempt4的创建终态。没有同一配置ticket绑定的stage、session原件、脉冲原字/结果或完整status时间序列，不能证明“控制器没建立”“GL没pulse”“surface/EGL坏”或与G13同因。

报告所引app_bridge:923的实际语句恰相反：Configure成功→Scene PreparePrograms成功→实际宽高/PrepareTargets成功→CurrentLocked通过→**m5Controller=next**→发布M5_WAIT_PAGE_PULSE。该文案是发布后的等待状态，不能证明发布前没controller。更不能将旧status字符串与后一次已reset的controller指针无身份拼成一个瞬间。INCOMPLETE原件是能力查询失败，不是ROOT/FULL payload捕获结果；像素统计也不能辨别这些初始化阶段。

## 2. 当前真实调用及两个不同前置

页面start：真实research Config32→setActive(true)→m5Configure→waitSession轮询→实际18word identity/sourceGeneration→attachFrame→合成输入offer→m5Pulse。native Configure在App锁内撤旧controller、选m5/config并Invalidate/wake，成功回REQUESTED只证入请求；owner实际沿旧route/EGL退休、窗口/EGL、创建新ControlIdentityV5、ControllerV5::Configure/Transport::Create、Scene程序/目标、同代发布后才可读session。

`m5Session`仅要求m5Selected和已发布controller，不要求首业务pulse；因此现有“等session再attach frame”不构成源码上的脉冲循环等待。`m5Pulse`也**不是GL owner入队再构造controller**：NAPI复制shared_ptr后直接调用ControllerV5::Pulse→clock_.PulseLocked（controller mutex），随后wake owner；owner稍后从Quota/Advance消费真实已接受控制状态。合成mode2先有合法input/ready再给pulse，OFF则沿其原资格；不把配置阶段成功叫物理Ready。

当前缺的是跨此链的持久配置ticket和逐阶段观测：通用words API attempt会被后续session/cold读递增，不能当初始化attempt；M5_WAIT_PAGE_PULSE也不是有first/last/epoch绑定的诊断。已有status/session/Quota/SourceStats和GPU输入诊断仍有补证价值，但不能覆盖未发布controller内部Configure/Scene的首返回与正在调用阶段。

## 3. 最小产品观测合同（交 developer，不新增 GL/热 IO）

实现文件归属：`m3b/{init_diagnostics.h,init_diagnostics.cpp}`；规范adapter `bridge_fields/create/pump/napi.inc`及实际生成App路径、ControllerV5::Configure/Pulse/TransportV5::Create、Scene准备的**现有返回切点**；ArkTS `GpuResidentM5/GpuInputAttempt`只增加有限阶段/首失败显示和有界启动退出，公共M3/G9行为不变。生成链须覆盖inc/公共helper/宏、native与ABC，不能只复制m3b cpp后复用旧bridge对象。

固定payload **16384B**，不是cold broker信用：control1024、4×512B attempt、32×256B stage、2×2048B log、尾1024保留零，逐段及逐字段见机器表。stage真正sizeof256/attempt512/payload16384由developer编译static_assert/offsetof交付，mutex/helper另列实际sizeof；不拿表式检查冒C++ ABI证据。stage保最初16和最近16，覆盖/attempt轮换/截断计数显式，保每attempt首失败与最后进展；槽失效返回STALE_TICKET，不拿新槽补旧terminal。

配置请求在受原安全锁校验成功、Invalidate/wake之前原子分配单调requestTicket，绑定boot、words configAttempt/diagGeneration、完整原Config32及SHA、cancel generation、epoch/window和源manifest。owner开始后才填实际身份/context/tid，未知scope整零但SCOPE_KNOWN关闭；不是UI猜run。每条stage携该ticket与真实scope。后续只读API可有自己的call attempt，绝不改配置ticket。任何payload变更递增同一snapshotRevision；只读查询不变更，溢出具名DIAGNOSTIC_SEQ_EXHAUSTED/不完整，不wrap。

切点至少包含REQUEST_QUEUED、WAIT_WINDOW/WAIT_RETIRED、EGL_READY、CONTROLLER_CONFIGURE ENTER/EXIT、三个真实program role、SCENE_PROGRAM/TARGETS、PUBLISH_CONTROLLER、SESSION_READ、FRAME_ATTACH、INPUT_OFFER、PULSE_DIRECT、OWNER_OBSERVE_PULSE/PLAN_ADVANCE及退休；实际未发生阶段不补ENTER/EXIT。Pulse记录原seq/uiNs/native receipt/完整scope和真实ControlResult，owner仅在实际读到该状态时记lastOwnerSeenPulseSeq，不能因wake就记消费。Config失败、scene失败或撤权均保首次原返回；没有原GL/EGL采样的字段为-1+known bit关闭，不额外查询错误、Compile状态、fence或上下文。

stage[208,240)为互斥payload：普通program阶段存32B sourceSHA；PULSE_DIRECT/OWNER_OBSERVE_PULSE设PULSE_PAYLOAD、清SOURCE_SHA，改存seq/uiCallbackNs/实际传入Pulse的native receipt/owner已观察accepted seq四个U64，不另扩对象。FRAME_ATTACH/回调注册是ArkTS客户端原件，沿现有有界输入回执单独保存，不能从UI自报造native阶段；只读诊断endpoint无写模式或授权布尔。control256、attempt128和stage256字段/已知掩码均完整登记在表，未取得真实模块SHA/process start的值为未知而非声明值冒证。

诊断用独立短mutex；不持诊断锁调用GL/NAPI/文件/等待，不反向取App/controller锁。先在原持有的安全锁内复制固定原值，释放后emit；必要同步序号保线性化，不把旧ticket阶段归新配置。program/source SHA来自实际传入源码及现有program-cache记录，日志只接现成原字节（每槽64B头+1984B raw、reported/copied/truncated分列），不新增shader log查询。没有新增GPU dispatch/map/fence/glFinish/错误消费或owner落盘；观测仍有CPU成本，不承诺绝对零扰动。

新增只读`m5InitDiagnostics(ticket:string,page:number):string`：8×2048B raw/base64，每包≤4096B，直接复制诊断Store、不排GL owner。page回ticket/boot/page/offset/rawBytes/snapshotRevision/采样时间，跨revision不得拼完整档；有限重试仍变则INCOMPLETE逐页保留。boot/loaded module/source身份分别分名，未能实际核ELF的字段不得用声明包SHA填成已加载SHA。它是初始化观测，不是物理审计或冷COMPLETE。

生命周期修复范围有界：新配置/窗口变代撤旧ticket、仅当前ticket+epoch/window/config代完整创建成功才发布controller/session；等待脉冲状态只在已发布后出现。现有同代检查继续，初始化本来就不应依赖业务pulse，不能套G13另改其时钟。页面waitSession增加有限启动终态/首失败回显与取消代检查，不无限33ms无原件轮询；超时报告观察结束，不能假定native已失败或强造Ready。hide/Stop取消旧回调、未知scope不发源/输入；不得为诊断放宽Fresh/GAP、制造第一pulse或调用GPU操作。

## 4. 确认的 FULL guard 遗留（独立于本次 NOT_READY）

隔离build bridge及当前正式m3b/napi.inc仍有`Semantic(words[15]==0,"PREFIX_IDENTITY_CONTRACT_REQUIRED")`，而T619已交完整cursor/token/capsule合同并要求消除此临时前置。故未来FULL flag1仍会在native拒绝，此为明确静态实现/接线缺口，**不是此次flag0创建失败的已证根因**。

developer须按T619完整FULL writer/reader、真实fullBranchTrace/arena分配与冷预约/上限身份对齐后，将准入改为原正式0/1语义并绑定实际配置/capsule，不只删除门或把未实现FULL报COMPLETE。旧容量/两slot/shared broker/真实退休退信用不变，不拿初始化诊断预算补冷payload；只开发编译，实际FULL执行/reader由T594负责。此项可与最小观测一同修，不能等设备先过才结束开发。

## 5. tester 精确续办与产品指标状态

现成可做：固定bc4c→8348签前后ELF/ABC和实际模块/源manifest关系，读取同一新配置请求前后完整status、m5Session回包、M3B input诊断first/last及实际原Config32；记录setActive返回、session成功/拒绝、frame attach与首input/pulse原字/结果和epoch变化。只发一次声明的配置，不重复点合成或冷导出污染attempt；用现有字段先归当前已发布/未发布域。新观测交付后由同角色取同ticket首阻断，不能把它新增为开发设备门。

如果仍无controller，保真实Configure/Scene/发布stage与原返回再回architect分工程/设计；已controller但input/pulse被拒，保真实scope/clock字段按原门分流；有accepted状态未present另查联合lease/Swap。不能给泛hilog、延长空等或相同INCOMPLETE重复档任务。T594 ROOT/FULL四档、真实BASE+SOURCE/paired61段、cursor64/2920B token、Meta2/FULL CONTROL2、双slot/独立reader仍NOT_RUN；补观测不能替代这些执行。

执行必须完成本卡范围和本角色必要检查或遇确实无法继续的具体问题才停，不能做一点就停；局部受阻先完成其余范围。失败原卡列全部未完、复现/原错/证据/尝试、不能自行继续原因、真实前置及接手角色；成功结束待验收、失败看评论。不允许修改D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不改他行/其它列/队列或滴答同名清单。CPU停投、新仿真GPU驻留，全部前景背景仅XComponent+shader，故障FAULT/明确STALE，禁Canvas/软件GL回退；目标10暂停，水线13→14→12、旧湿M1暂缓。tester/effector签名部署执行采证，真机串行，不增manager或开发设备验收门。
