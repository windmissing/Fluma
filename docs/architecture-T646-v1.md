# T646：GPU 创建未发布与原生停点首次归因

2026-11-19，architect，不使用真机。结论 **TEST_FACILITY_REPAIR_AND_PRECISE_EVIDENCE_FIRST**；本卡归因与分流已完成。当前不能确定本轮最后GL调用、program/role或线程等待链，不能据缺证派产品卡顿修复。先由tester续办原T496设施与有界采证；没有证据表明还缺产品只读stage。未运行夹具、构建、产品测试或设备。

## 1. v6 原件勘误及身份

消费T620、aba5938/32228ce、v6报告及保存原件。可复核证据与源码行/SHA见`analysis-T646-evidence-v1.json`，只读复核脚本`analysis-T646-v1.py`。报告attempt62012对应日志实际runId是`t496-1218097213334300`；该run只有START与两条COORDINATOR_WAIT_ITER，stage只有D1_OFF ENTER、arm为off。它实际进入默认D1分支，不是具名C0/C1/C2的CREATE_ONLY。没有可核argv原件，不能推断具体误传了哪个参数；但该分支身份由原件直接确认。

`main`只识别C0/C1/C2，其余参数静默走默认D1；只有`RunCreateOnlyDispatch`调用`t496_sink_initialize`，默认D1没有。因此本轮找不到`gltrace-62012.ndjson`，不是已注入sink后观察到无GL事件；旧v4c三臂有sink和未配对411，**仍只能归旧run/arm/attempt/pid/callId**。本轮最后GL ENTER/EXIT、当前program/source/role、IN_API是否持久及等待栈全部UNKNOWN，不用旧411填空。

| 字段/producer | 真正含义 | 可作何种证据 |
|---|---|---|
| RunD1 `a.hostNow` | steady_clock真实单调ns；Stamp的时间来源 | 该分支clock已改，不能推广到CREATE_ONLY（后者仍固定脚本clock） |
| Emit `scriptNs=g_hostNowNs` | 固定1000000的脚本域 | 不是wall时间，不能测420s/30s心跳间隔 |
| coordinatorWaitIteration | 协调器for循环的0/15000 | 包含sleep/锁与调度，不能据15000×2ms认真实已耗30s |
| ownerProgressSession | 实际取`a.progress.session` | 会话代号，**不是ownerProgressSequence或心跳**；恒1不证明owner不推进 |
| epoch | App撤权/创建代号 | 采样恒1不证明全程未reset或owner不运行 |
| captureCommittedEpoch | App提交发布后置seen，另有归零路径 | 两采样为0只证此时尚无该发布标志；不证所有resource分配/编译均未发生 |
| v6 summary420s/aliveAtKill | tester报告的外部观察 | 本run未见配套watchdog发出/接收/退出和栈原件，不当精确调用持续时长 |

因此撤回v6“owner无推进”“所有资源commit从未发生”“设施各项全部有效”和“已超tester域需要产品日志”的充分性判断。保留真实观察：本run未到后续握手块，产品CREATE_WAIT/READY原因未定；D1/D2差分与32cycle仍NOT_MEASURED，不伪产品PASS。

## 2. 真实 Create→发布链及现有观测

冻结同源树`.codex/local-test/t492-m1-delivery`的App owner：EGL准备→TRANSPORT_CREATE_ENTER→FullM1Transport::Create→TRANSPORT_CREATE_EXIT（真实code/firstGl/allocatedBytes）→Scene::PreparePrograms ENTER/EXIT→构造Run→App mutex内`recovery.Resources`/v2ResourcesReady→CurrentLocked核epoch/window→WAIT_NEW_SESSION/Mark Ready→**committed为真才captureCommittedEpoch=seen**。创建实际对象早于最后标志；等待也可能发生于scene、callback获取App锁、资源发布或撤权退休，不只能归transport内部。

FullM1 Create内部已有source CreateUsing、intermediate quantum Create、final program Build、Resource SubmitScope、Reflect/Prepare、cold warmup/prefix/fence与真实返回值；source/program-cache/submit/tail observer已连App。App `bootProbe.Copy()`是独立短锁冷快照，保存attempt serial/epoch/ownerTid/latest/firstFailure、256点和dropped计数；已有bootNote与submit/scene/Resources切点足够先定位大阶段。snapshot是有界历史、旧点可能覆盖，不能冒完整API trace。`t561_seams.h`中的大部分high-level是poll观察映射，不能把SEAM或超时后补记冒真实ENTER/EXIT。

等待链应区分：owner在GL/driver、sink TraceMu/flush、App mutex、bootProbe mutex、program-cache/文件IO、或协调器/回调交叉等待。D1OwnerHook已不等待permit，但仍会拿App mutex和调度mutex；去掉一处wait不等所有锁都已排除。测试采证在锁外落盘；外部暂停/栈工具会影响调度，其样本单列诊断，不混进性能臂。

## 3. tester 续办合同（不使用真机，原T496，不新增开发门）

1. 先修入口：仅接受预声明mode与arm/attempt，未知参数非零退出，禁止静默转D1；默认D1若保留也必须在启动owner前初始化同一sink，明确sourceCommit/fixtureSHA/PE SHA/实际加载ANGLE模块/argv/cwd/pid/ownerTid/run/arm/attempt及新建文件成功。当前C0/C1/C2需同样接T620真实clock/握手，不把只改RunD1说成已修CREATE_ONLY。原件失败即停，不继续空等。
2. 修runner现有实际错误：`%ATTEMPTID%`应为真实已声明的attempt变量；while条件应等待**该child存活且未到deadline**，当前`-not(Get-Process...)`会反向提前结束；PID生命周期连创建时间，禁按进程名杀其它会话。用Start-Process则WindowStyle Hidden。保存launch/kill-request/observed-exit时间与退出码；join不受控时外部watchdog负责本child总上界，不能worker.join无限延长。
3. 单一具名修正臂，整体≤420s（资源≤240s、握手≤120s、诊断/清理≤60s，全部在总额内），不延长等待、不先跑三臂。采样独立wallNs，progress.session原名保存，另以实际checkpoint事件累计ownerProgressSequence及lastOwnerStage；不得由iter或epoch派生。当前GL sink还将两次WallNs分别标wallNs/scriptNs，须修为明确wall clock profile与真实脚本producer（非脚本臂标absent），不能再混名。冷采现有bootProbe.Copy、v2ResourcesReady、captureCommittedEpoch、RecoveryJson真实serial/epoch/run/page/phase，复制后解锁再序列化。不能从未到新阶段推driver排除。
4. GL sink使用原API的参数/返回值维护固定容量shadow表：create/delete program代、glShaderSource实际拼接字节SHA（含length规则）、Attach/Link/UseProgram与角色映射、dispatch XYZ；记录调用thread ID、同一identity的callId与ActivePhase。profile/完整原源与缓存key另绑定。只保存CPU已见参数/返回值，不新增glGet*/glFinish/glGetError、不强制flush或延迟，不改变原错消费。未知角色诚实UNKNOWN；固定表满/截断具名EVIDENCE_INCOMPLETE，不冒身份匹配。观察调用本身无时延免费承诺，不能据诊断运行宣性能。
5. 未推进时在本attempt外部有界采样2–3次ActiveCall、boot最新stage、owner与协调线程栈/等待对象、watchdog时刻；工具/权限不可用保原错。把最后API ENTER与对应EXIT按完整键关联，区分ENTER_EMIT/IN_API/RETURNED/EXIT_EMIT；有ENTER无EXIT不等永久driver死锁，flush/日志锁本身也列等待候选。
6. **资源发布真实已达且scope仍有效**才调用RecoverySessionLocked，再PagePulseLocked使用同一真实clock域的当次stamp，记录真实返回码和Fresh/Ready/Allowed；当前T620握手块在循环耗尽也无条件Emit RESOURCE_COMMITTED，且首pulse仍用脚本1000000，必须修为具名TIMEOUT或真实guard。不能拿旧脚本stamp给新real hostNow做freshness，不能手改Allowed/Ready布尔。

先完成可离线修的入口/runner/identity/快照与role记录，再做一个有界窗口；签名/部署/脚本/采证归tester。原D1/D2/32cycle只有真实正控非空SOURCE/QUANTUM及其原件后按原合同续测，不能把本次诊断窗口替作业务通过。

## 4. 回 architect 的精确条件与角色

当前交tester，不派泛化产品日志或资源重构。若同身份新原件证明某真实入口已ENTER且现有Boot/Submit/GL shadow不能区分内部不可观察等待，需提供该调用/源SHA/最后阶段/缺失字段与栈原错；architect再决定最小产品stage。产品后继只允许固定容量只读CreateStage/sequence/attempt/epoch/role/sourceSHA、已有API返回值与现有错误读取点的raw code；不新插GL查询/强同步、不在owner IO、不改调度/容量/物理/GAP。不凭现在的缺采证先授开发范围。

T620本稿所修是设施未按合同落地及原件解读，非设计数学问题、非用例阈值争议。产品卡顿根因仍待精确证据；这不阻断本卡首次归因完成。完整执行边界：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其它可做项。失败回执含未完/复现原错/证据/尝试/不能自行继续原因/所需前置与接手角色，原卡评论并本人行看评论；成功结束待验收。不允许修改D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不更新他行/其它列/队列或滴答同名清单。GPU驻留、CPU停止投入、前景背景仅XComponent+shader，FAULT或明确STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12、旧湿M1暂停。developer只开发编译，tester/effector签名部署执行采证，无manager新验收门。
