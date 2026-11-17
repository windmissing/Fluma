# T518 同源离线 Create 长等待首次归因 v1

> 2026-11-17 T547补充生效：`architecture-T547-create-and-empty-export-triage-v1.md` / 对应机器表修订后段精确sink、匹配环境seed与900s runner。T496 v2原件实际C0 378行、C2 660行且继承完整C0前缀；C2自身无compile/link，不能采报告的环境miss重编译断言。CREATE_WAIT非owner心跳，非空sizeof门仍未修。本文旧历史证据不冒新核验；新请求按T547执行，CPU/期限/渲染边界不变。

2026-11-16，architect，不使用真机，不运行GL长测。结论 **首次归因与设施修订合同已交付；具体未返回GL调用仍需精确补证**。不能从CPU活跃、等待iter或T475另一fixture成功推断JIT/驱动死锁。输入原件及SHA见 `analysis-T518-host-create-evidence-v1.json`，其中保全现存小日志/布局文本，避免忽略目录中的原件丢失。

## 1. 原件能证明到哪

读取T496原卡、全部现存JSONL、源码/build、双TU布局、T475工厂、T492实际Create/scene/recovery。现存 `t496_log.ndjson` 共29行、8个runId，stage文件10行；不是三轮完整可比执行。最新run `t496-823516366862300` 只有START和15条CREATE_WAIT，最后iter210000、scriptNs始终1000000，无真实wall时间、ownerTid、GL环境或内部调用enter/exit。

首个可定位的未配对切点是 **D1_OFF ENTER**（最新run；arm还为空），不是 TRANSPORT_CREATE/SHADER/SCENE 的原始enter。`CREATE_WAIT`只观察 `captureCommittedEpoch` 尚未匹配，覆盖EGL、transport、scene、资源commit全部路径；不能据它定位compute或graphics。`>52min/CPU活跃`保留为tester观察，日志本身无法独立复算52分钟。iter×2ms只是sleep下界，锁等待/调度会延长。另一run `820269492331700`有GUARD_TIMEOUT和空PRE_QUIT，stage还硬写OFF EXIT OK、ON ENTER，EXIT OK并不证明业务成功。

## 2. 已确定的设施缺陷及边界

| 项 | 静态证据与影响 | 分流 |
|---|---|---|
| ABI门空比较 | MSVC原件写 `size(182640)`，build却findstr `sizeof`；两个sizeof文件均0B，fc空文件必相等。双原始布局现存成员/offset确实一致，含source128；不能据此声称这轮一定混编，也不能再称该自动门有效。无/showIncludes归档与exe→源码hash收据，历史执行来源还有限制。 | tester修门，非产品ABI缺陷 |
| 暖种子根本未接入 | T496 main忽略argc/argv，未调用Store.Configure，未设置packageSource；另一个build_app脚本的种子参数不会影响此exe。该脚本实际是 `process.argv[5]`，CLI project/out/route/seed（第4个位置实参），不是随便给T496一个目录。 | tester接实际host缓存 |
| READY交接断层 | RunD1在资源创建前直接progress.BeginSession；创建后资源进入WAIT_NEW_SESSION，T496无RecoverySessionLocked/首PagePulse/有效UI交接；协调器却先等PumpBegin。actual owner在WAIT_NEW_SESSION且!Allowed时continue，可能永远达不到该hook。captureCommittedEpoch只是资源commit，不是READY。 | tester走生产交接，不改产品门 |
| 排空/退出锁 | drain持有unique_lock(g_schedMu)，随后同线程再次lock_guard(g_schedMu)，非递归mutex自锁；退出只设a.quit却不设g_scriptDone/通知，owner可能仍卡在hook等待permit，join不返回。两臂也未重置全部调度状态。 | tester修有限终止 |
| 业务屏障和D2不足 | milestone位置目前只有空App锁；runActive/hasFlight是由accepted/runQueue推算，不能代替真实Tick/HasFlight。文件声称D2 Flush暂停，但实际无独立D2线程/切点/终态，只有on.snap.evidenceFrozen断言。 | tester补完整T494合同 |
| 假通过路径 | QUANTUM_NONEMPTY仅核off，bothEmpty可让parity/量子断言true；delivery未实际置true/比较完整动作和源bytes；阶段EXIT总写OK、main总return0。 | tester区分PASS/FAIL/NOT_MEASURED，失败exit非0 |

上述均可按代码条件直接复核，**不宣称它们就是最新Create_WAIT对应的内部卡点**。当前两种根源必须区分：driver/API调用未返回；资源已返回但恢复/测试握手未前进。现有日志不能排除后者。冻结脚本时钟只适用离线调度等价性，TailObserver/programcache仍走真实steady clock；不能用脚本根deadline结论冒真实250ms性能证据，也不得为迁就host耗时修改业务期限。

## 3. 最小可执行暖种子与定位矩阵

已有合法候选seed：`.codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1`，正式收据副本 `test/reports/t492/cache-m1`。六role收据含三compute与三graphics；environmentSHA `2aabb8c88eabed25386ede5af0b3fd16cfe498a5c53ba42f5239900f2258a5b0`。本次只核磁盘T475B001包头/长度/key/payloadSHA、收据及文件hash，不调用glProgramBinary，不宣称当前环境一定可载入。

工厂key由实际完整shader source/layout/environment/link config决定；compute key前三项分别以1028e02a/9062993a/545a3f5f开头，完整值在证据JSON。native `.bin`带144B封装，不能把payload裸拷贝当artifact，不能仅改文件名。graphics descriptors仍需当前源码/接口反射和generation核验。任何miss/拒绝必须保原route/cacheRawGL/log，允许原工厂合法冷编译，不能把冷fallback误记暖命中。

tester先单独做CREATE_ONLY，不启动D1脚本：

1. 冻结所选完整树，记录exe、全部输入头源/generated头、宏/include、link库/DLL绝对路径+hash和环境字符串；/showIncludes归档，解析完整类layout及关键offset，断言两个记录均非空且一致。不要比较空文本。源/头不一致拒开跑。
2. 每个arm用新进程/新输出目录：C0空cache、C1仅三compute bin、C2全部六role bin。每个只允许一次有界attempt，不能先OFF跑完再把同进程内存cache命中冒成ON与OFF等初态。Store.Configure(本arm files目录)在owner启动前完成并验证，记录packageSource为该来源收据；预置bin经原loader校验。
3. 启动前安装测试侧真实GL/EGL转发stage sink：每次ENTER先append+flush，再原调用、EXIT原样记录返回。字段含run/arm、wallNs与scriptNs分别列、tid/context、callId/parentCallId、role/key、stage/原GL/EGL。使用正确函数签名、同owner context，不多读glGetError吞原错误；通过既有observer结果或保存并转交原错误。sink不拿App/调度锁、不调用GPU、不阻塞GL owner。
4. 必须覆盖EGL创建/makeCurrent、SOURCE Build、INTERMEDIATE Build、FINAL Build、reflect、buffer分配、Prepare dispatch/flush/fence/wait、twoBatches prefix、SCENE三个Build/反射/VAO、资源commit、WAIT_NEW_SESSION、session handoff、首pulse baseline、READY。实际FullM1Transport先source.CreateUsing，再quantum.Create，再替换FINAL，再prepare/warmup；scene.PreparePrograms在transport返回之后、READY之前。不能只写泛称Create enter。
5. 若现有boot/diagnostics只能冷读completed阶段，独立watcher每秒读取固定快照并写文件；它不调用NAPI唤醒owner。要定位真正卡住的GL调用，最低仍须测试转发层ENTER落盘；不要依赖“等EXIT才形成收据”的program factory。仅用已存在的测试hook/转发包装，不改产品shader/业务控制；若该包装无法覆盖具体函数，回architect给明确函数/接口缺口再转developer。
6. 本轮设施观察上限900s wall/进程（沿T496已声明值），约每30s输出一次watcher状态；外部进程watchdog负责落超时收据并终止本次测试子进程，不能在卡住GL调用中强拆资源或无限join。900s是测试cold观察界，不替换500ms既有cold-prefix预算、33ms GAP或250ms物理root期限。超时不再加长，记录首个ENTER无EXIT及原文件，交architect判进一步方向。

C1能过compute且卡scene只能把范围缩到scene，仍须具体调用；C2 BINARY_LOAD仍卡prepare/flush则不再重复归因compile。所有C0/C1/C2都能到资源commit而D1不能READY则归测试恢复握手。只有出现真实shader/link失败日志才转相应developer工程修复；环境/厂商未知继续保unknown，不猜ANGLE后端或机型。

CREATE_ONLY过资源commit后按生产RecoverySessionLocked(serial,epoch,run,page)取得新session，在当前有效UI/manual输入下发首pulse基线并核READY，再开始D1。锁外checkpoint按代号握手，里程碑由owner的真实Run/Tick/HasFlight/FIFO固定快照决定，不能由账反推。修drain为单次锁操作、所有退出都设scriptDone+permit并notify，所有等待有wall上界，两个arm重置调度状态。D2仍须真实Flush/Wait切点独立控制线程保reservation→terminal，不可用D1采集成功替代。空账保持NOT_MEASURED，两个arm均须sourceCommit>0且physicsCommit>0。

## 4. T504 可独立推进范围

T504静态10项已交报告保留；它不需要等待T496跑通才能做所有后续：可独立完成四档对象/bytes公式与分配/退休路径核对、T501生命周期源/错误退出静态审计、已有T475/T492成功创建fixture的Create→Release有限GL子序列（先核同源及本合同观察界）、T502缓存文件预算/TTL/取消/ENOSPC等不需物理正控的文件边界测试、已有合法原件的代号/首次故障检查。每项输出自己的PASS/NOT_RUN/证据，不套D1结果。

真正依赖非空SOURCE/QUANTUM正控的32cycle混合序列仍等修好的T496动作设施；可复用独立已存在GL fixture先做不依赖D1的Create/Release子集，不能把它冒完整32cycle通过。已有适配若接口未齐就记录准确接口前置，不无限等同一个长跑、不重开CPU方案。

## 5. 回执与后续角色

本卡完成首次归因、工程/测试设施合同及精确证据请求。manager可将§2–3交tester在T496续做；产品GL/API未返回根因待该精确原件再回architect，当前不默认派kernel拆分。T504按§4并行可做范围推进。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它项。

不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人状态有限例外；不改其它行列/滴答同名清单。不使用真机，CPU停投。前景背景全部XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。此次不改产品、不替tester执行、不延长业务期限、不冒产品PASS。
