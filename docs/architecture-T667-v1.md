# T667 · broker 持读释放新观测与矩阵隔离续归因

2026-11-19；architect；不使用真机，不运行harness/GL/构建/产品。消费T627 `3190ce6f`、run2.json/完整26行ndjson、driver_v2、两个adapter、生产715f314d…与T636/T656。证据 `analysis-T667-evidence-v1.json`；精确续作矩阵 `architecture-T667-contract-v1.json`。

裁定 **TEST_FIXTURE_TICKET_READER_AND_CLEANUP_ERROR**。D2“Freed忽略活跃reader”未被实际行使，无需改生产broker/退休合同、无需designer。M5匹配opaque context的退役正控成功保留，M4累计额度仍NOT_RUN。

## 1. 逐票据首个偏离

生产ReadBegin拒不存在票据、该票据builder未结束或全局任何reader已持有；ReadEnd只清匹配ticket的reader；Freed若匹配ticket仍reader则不释放，否则清slot。两个slot允许一个builder和另一个reader并存。按原日志与driver真实调用：

| 边界 | 真实状态及证据 | 报告宽结论的问题 |
|---|---|---|
| seq8–10 / M1 | ticket1 ReadBegin=true，随后B.ReadEnd(1)、A.Freed(1)；M1结束没有reader1 | M2 detail却继续写readerA-active，没有对应重开 |
| seq11–13 / M2 | ticket2 Reserve成功；builder期ReadBegin=false；BuilderDone(2)后ReadBegin(2)=true | 此时没有别的reader，true完全符合合同；不是D1产品语义变化 |
| seq14前后 | A.ReadEnd(tA=1)对已释放1无效；再次ReadBegin(2)=false，因为**reader2正持有** | false使`if(a3)`清理分支跳过，2未ReadEnd/Freed；不是释放全部reader后的失败 |
| seq15 / M3 | ticket3 Reserve成功、builder3未Done；代码调用ReadBegin(3)但**忽略bool** | 它因builder3和reader2两门均不能成功，未建立reader3 |
| seq16 | B.Freed(3)清掉无reader的3；下一fresh0 Reserve成功来自刚空slot | 新票据输出没记且局部变量遗失；按实现可推serial4，不能冒日志已观测4。不是Freed杀了活reader3 |
| seq17–18 | 旧3已不存在，ReadBegin(3)=false；fresh Reserve又被遗失的builder阻挡 | 合法的旧票据失效；不是持读票据提前退休。reader2仍漏着 |
| seq19 / M4 | 入口Reserve=false，累计route/process分支没执行 | 已有builder/two-slot更早拒门，不能归额度或ALL PASS |
| seq20–24 / M5 | GPU域独立；B保活动gB→匹配ctx ContextRetired→A fresh0 Reserve=true | 保该组件正控；不冒真实eglDestroyContext、driverfree或全部错误guard已逐项隔离 |

关键遗漏的void调用必须在新日志逐项列出参数和前后序号，返回栏标VOID；void没有可验证bool返回。上表生命周期有些是源码/生产实现推导，原日志没有读返回或新票据号的地方均明确“不曾观测”，不伪写进历史原件。

生产库文件 `.codex/local-test/t636-harness/libgrs_cold_slots.so` SHA `715f314d43549d4eed991922f25faa350fedcaf6ab8dab1598abc980dd278d03` 与T608 release逐字节一致。run2 LOADED只记录wrapper地址/两个handle，并没有本轮maps/库SHA/全部调用参数；静态现成库身份不能冒新run实际映射资格。但即使暂不凭这份实现推导，**M3首ReadBegin未检查**已足以否定“reader3确认活动后Freed”的报告前提。

另对该现成AArch64 ELF作纯静态反汇编：GrsColdFreed在0x6600读取slot+21的reader标志，0x6604仅在reader为0跳入清slot分支，为1则跳过清除并解锁返回。完整命令/工具SHA/输出在证据JSON内；这不是运行broker或设备测试，但直接排除了“这份现成二进制根本没编reader保护”的说法。

## 2. ABI / 日志声明收窄

八个loader指针现在都做非空检查，优于旧版；实际两DSO都选A_命名exports。Reserve采用bool(U64*,size_t,size_t,U32,U64*,U64*)，ReadBegin为bool(U64)，BuilderDone/ReadEnd/Freed为void(U64)，GpuReserve为bool(U64*,U64,size_t)，GpuFreed/ContextRetired为void(U64)。两个adapter的B_BuilderDone别名返回bool，但本driver调用的是A_BuilderDone/void，不能把未调用别名报成当前ABI故障。

日志仅Reserve做dladdr并记录其实际名，ReadBegin只有地址；没有全部8函数的模块/名字/地址记录，不宣8函数身份均已取齐。26行确实seq0..25连续，原每行只有 **seq/event/detail**，不含报告声称的boot/pid/attempt；文件仍`std::ios::app`，不能说已强制single-attempt create-new。保留本次26行，不去重/改写为合规新日志。

同时修正T656历史简写：**GPU** Reserve要求`*ticket==0`；host Reserve允许已知同route、无reader/全局无builder的现存ticket合法重建。新隔离探针统一fresh0以避免混因，不把此测试选择改成host生产API绝对只准0，也不删其合法复用能力。

## 3. 原T627 tester完整修正矩阵

tester拥有driver/adapter/manifest/部署；复用已交8函数生产库，不等待developer新endpoint/快照。每个阶段从已明确清空的独立组件状态开始，成功取得的每个票据保存在固定ledger内；任何预期拒绝意外成功，都先登记输出新票据并结束/清理该阶段，不能把局部变量丢掉后继续测下一个域。禁止ResetAll/猜别进程票据；若清理无法证实则封原件退出独立测试进程，另unique attempt重开。

| 阶段 | 唯一隔离条件与正控 | 清理 |
|---|---|---|
| M1 单builder/跨DSO | A fresh0 Reserve；B各fresh0同/异route拒。B.BuilderDone(A)，B.ReadBegin(A)成功 | 检查bool后B.ReadEnd(A)、A.Freed(A)逐项记录 |
| M2a 只有自身builder | 无别的reader；B Reserve后ReadBegin(B)拒；BuilderDone后同票ReadBegin准 | End(B)、Freed(B) |
| M2b 全局reader / 跨slot并存 | A Reserve→Done→ReadBegin(A)确认为true并保持；B另route Reserve准、Done；ReadBegin(B)拒；只End(A)后ReadBegin(B)准 | End(B)；Freed(A/B)，各结果与调用原样记 |
| M3 持读Freed | A Done→ReadBegin(A)必须true；B Reserve→Done且无reader，占满第二slot、无builder；B.Freed(A)；第三fresh0 Reserve应拒；End(A)后**ReadBegin(A)应准**证明原票仍在；再End(A)、B.Freed(A)，第三fresh0准且新票 | 原票复读正控不可省；保新票并Done/Freed，最后Freed(B) |
| M4 route额度 | 独立清空，仅A charge4096/budget8192/route1，Done；B charge4097应拒，去掉1字节以4096应准=8192 | 新成功B须Done后Freed，再Freed(A) |
| M4 process额度 | 再独立清空，A route2 charge(cap−4096)/budgetcap，Done；B route3 charge4097拒、4096准，总cap=524375552 | 所有成功票据Done/Freed；不实际申请数百MiB |
| M5 GPU票据 | fresh0保gA活动；wrong-ticket Freed与wrong-context Retired**分别**探针拒；B正确Freed(A)后fresh0 gB准，保持gB活动→匹配ctx Retired→fresh0 gC准 | 仅持有票据实际Freed；错context对照与正确退休分列 |

M3的单次第三Reserve=false有two-slot门，**不能单独证明持读Freed**；必须同时靠End后原票ReadBegin成功、再真Freed后新票准这一组证据隔离。不要在一个只有一个slot占用的场景要求第三Reserve必拒——另一空slot成功并不代表旧reader被清。新的M4中4096+4096==8192应成功，driver_v2现写expect0也须改正，不改变正式阈值。

## 4. 设施与预算

独立进程create-new唯一attempt路径，manifest及每条原件包含boot/pid-start/attempt、seq、monoNs producer；8函数均记录实际symbol/address/module归属。结构化记录op/wrapper/ticket/context/bytes/budget/route、所有指针before/after、bool/VOID、实际调用的清理步骤；字符串严格JSON转义。报告expected与observed分栏，不让detail里的“reader-active”充当实际reader状态。

总≤30s、≤128条、每条UTF8（含换行）≤512B、日志≤64KiB、manifest≤16KiB；所有探索/清理都计数，容量不足明确CAPACITY而非DONE。固定≤16项本进程票据ledger属于tester设施，不新增产品记账。若阶段失败先完成可独立的M5等范围，再给每域真实NOT_RUN/FAIL，禁止无条件DONE→整矩阵PASS。

这是BROKER_COMPONENT_CREDIT域，组件成功不证明driver物理free/真实context销毁或生产route接线。只有修正矩阵后**确有成功ReadBegin匹配票据且仍活跃**、全参数/加载身份齐的生产矛盾才回architect修改工程；本次不改broker715f314d、接口/额度/退休语义，不新增manager门。

执行prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不得更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。architect不使用真机；原T627 tester设备串行，脚本/签名部署采证归tester。GPU优先、CPU停止、10暂停、水13→14→12；产品所有背景前景XComponent+shader，FAULT/明确STALE，无Canvas/软件GL回退；不以设备测试增developer关闭门。
