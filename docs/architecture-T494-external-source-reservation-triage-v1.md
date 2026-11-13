# T494 · 外线程SOURCE消费与预约缺证首次归因

2026-11-13，architect，P5，不使用真机。消费T488 D1/D2逐动作、阶段原件、E断言、T478 v2及实际include/链接来源。完成首次工程归因；不执行GL业务测试，不改产品实现或旧报告。证据/hash及MSVC只读编译审计见 [索引](analysis-T494-evidence-v1.json)、[编译审计](analysis-T494-compile-audit-v1.txt)。本稿补充 [T478 v2](architecture-T478-symmetric-driver-lease-v2.md)，不重开T471。

## 1. 裁定及证据保留

**先交tester修测试工程，当前不成立“外线程SOURCE产品饥饿”或“预约失败与SOURCE同根”的结论。** 交付构建存在确定的C++跨翻译单元类布局不一致；D1没有业务里程碑交接/排空；D2没有发送SOURCE，且缺request终态/owner阶段记录。三项都能破坏当前判据，历史运行未保存足够来源和消费记录，不能从静态缺陷唯一倒推某轮实际首错。

T488原件保持原hash，不覆盖旧报告/summary或把FAIL改PASS：最终runId `t488-702877288780900`，summary为26/29、E12/12、D1 off/on accepted=0、captureRows=0。最后一轮逐账观察69 generated = 61 overload + 8 revoked，carry0、clearedCarry120000000、SOURCE/physics/completion均0，两臂数值确实相同；这只是所记录终态账相同，不能证明非空业务无扰动。E的SOURCE/Acquire/重复背压/真实signal同代再租归还/slot1→0→1覆盖旧代STALE/新代/Release **12个通过观察保留**，不要求重做旧T471，不扩展成未signal注入或reset/context负控已过。

新发现来源混编同时涉及E所在TU，因此E旧断言不得外推成任意一致版本的产品ABI证明；后续来源修正若更换被测构建，只核必要lease见证承接，不抹掉既有E原件、不重开整套旧卡。保留历史观察与限制其归属同时成立。

## 2. 实际来源与ABI：首要测试工程阻断

`test/t488/build.cmd`只把部分-I及.cpp指向 `.codex/local-test/t475-handoff-final`。实际driver include链为：

```text
driver_lease_diff.cpp
  → test/t422/t409_host_nomain.cpp
  → test/t398/app_host.cpp
  → test/t324/app_gl.cpp
  → entry/src/main/cpp/gpu_resident/app_bridge.cpp
  → 同共享目录m1/run.h、full_transport.h、scene.h及capture头
链接：t475-handoff-final/.../m1/run.cpp、full_transport.cpp、scene.cpp等
```

MSVC `/Zs /showIncludes`核出上述共享头；原build的-I不会覆盖带相对路径的quoted include。相同宏/编译选项下用`/Zs /d1reportSingleClassLayoutFullM1Transport`分别检查driver TU与冻结full_transport.cpp，均语法通过，但结果如下：

| 同名grs::m1::FullM1Transport | driver TU共享定义 | 冻结implementation TU定义 |
|---|---:|---:|
| sizeof | 216704 | 182640 |
| flushSample偏移 | 33880 | 64 |
| source偏移 | 33944 | 128 |
| quantum偏移 | 125936 | 92128 |

共享定义含copyRing[128]/ledgerPrefix，冻结头无；Scene共享头还含T485新增uniform/准备状态，冻结Scene无。这是ODR/ABI违规，编译链接成功不能保证C++对象正确。build不能被描述为“完全冻结T475 M1源”；冻结目录的app_bridge本身还是生成路由稿，与实际共享App也不同。E直接实例化同一TU中的类，所以也受相同构建边界约束。

本轮审计仅语法/include/类布局，未链接运行新GL程序。历史manifest没有TU include依赖/hash，不能证明12轮均使用当前二进制的同一构建状态，不能把本轮静态审计伪装成历史崩溃现场。最终exe/hash与所有原件入索引，未重签包、未改产品源码。

tester接续第一步必须选定**一份完整一致的M1 host源树**：App/Run/transport/Scene/所有传值或成员类型的头和实现、GLSL生成header、cache helper一同冻结。若冻结目录的App为M2生成路由，不能只改宏或拼共享App；选择有完整M1来源的固定commit/生成流程，写明route/actualClock，缺完整来源先明确所需生成输入交相应developer，不自行恢复旧产品逻辑。host App适配仅通过显式测试入口引用同源App；不改生产endpoint。每TU保showIncludes或编译依赖清单及hash、预定义宏、编译器、link输入、exe和GL环境/renderer。所有TU的类定义hash/布局一致是开跑前的构建检查，不能靠调整new对象大小/填padding掩盖。

## 3. D1：固定动作表不等于确定性消费

最终manifest实际 **40 action**，不是报告41；6初始pulse、2 SOURCE、30 QUANTUM pulse、RESERVE和END。SOURCE位于97ms和305ms，却各占一拍、没有同stamp pulse；相邻真实pulse间隔为32ms，虽未超过33,333,333ns，报告“业务都与pulse同拍”仍不符合最终表。RESERVE与下一pulse同161ms，END609ms没有后续有限排空表。应明确保旧script为历史版本，修订另建script revision，禁止悄改旧表追认。

确定的问题：

1. 协调器只等captureCommittedEpoch，随后连续拿App锁推进脚本到609ms，没有等待owner到Wake/指定业务checkpoint或同一root的接受/拒绝里程碑。D1 PumpBegin hook只是`while(controlReserved)`占位，D1中该变量为false，未建立两向交接。即使动作都成功入mailbox，也可能在owner完成任何root前推进大量quota，pending最多4、active最多4而overload累加；不能从overload61推出owner从来没Pump，也不能推出FrontLocked永不放行。
2. 送完立即`a.quit=true; worker.join()`。loop的quit检查会退出，retire销毁Run/transport；这是退出清理，**不是排空送达的SOURCE、plan、未封提交、fence、lease**。终态pending/active为0和revoked8都不能证明曾成功消费；没有quit前快照，不能唯一确定8 revoked发生在哪个具体撤权点。
3. pulse调用用`a.progress.sequence+1`，未使用表的act.seq；日志没有实际session/seq/UI与native接收双域、32U32 payload/hash、quota/Run快照。MakeOffer又用实时identity/input/boundary和硬编码y/m，manifest的参数FNV不证明实际上传字节相同。
4. `deliveryEqual`只比较receipts的actionId，变量未进断言；off的reserve/END noop不进receipts，on reserve进、END也不进。DELIVERED_ACTION_COUNT只比较总数40/40。它不能拒等数量错序/错payload/错seq，也没有保存一次真实漏拍负控执行。保留“总数相同”观察，撤去“实际输入等价已全面证明”的外推。
5. completionCount仍0/0可被Cmp计为一个等值PASS；虽没有额外completion256 PAIR_OK，仍须单列SOURCE>0、QUANTUM>0、completion非空前置。NO_OBSERVATION_DISTURBANCE仅比较8个账，不核逐root/state/event/completion/token或实际deliveryEqual，现有布尔PASS不能作业务无扰动证明。firstError只扫描evidence行，没有ARM时保持NONE，不能据NONE排除原生首错；需同时保存App firstFailure/phase/permission及原始许可记录。

实际生产顺序是：当前身份/Ready检查→清过龄SOURCE→`!run->Tick().active`才FrontLocked→Run::Offer→idle时progress.Claim(queued!=0)→AdvancePlanV2锁定sourcePending→Pump。active期间外部SOURCE等当前有限plan结束，下次idle消费；Run::Pump在接受末QUANTUM后会active=false。FrontLocked本身不按OS线程分支。这个门用于避免在途plan消费身份变更，**不能为修测试直接删除、把SOURCE插入旧plan或绕过mailbox调用Run::Offer**。

真实饥饿须在同源、合法Fresh/Allowed、无撤权且计划已完成的观察窗内证实：mailbox仍有未过龄有效SOURCE，而owner到idle消费点却未取/反复新plan抢先。旧日志没有这些逐点状态，因此此域为NOT_MEASURED，并非已证没有产品问题。

## 4. D2与Wake对照必须独立纠偏

D2本身只有pulse线程、control预约线程和owner，**没有SOURCE动作/Offer调用**，不能把ARM失败直接归为D1 SOURCE未消费。PumpBegin暂停点在Pump调用前，不是报告中所谓实际GL Flush卡住；WAIT_ROOT是生产预约返回的blocker观察，保留这一子集，但它不证明已在真实Flush/fence忙区预约。

`g_d2ArmedSeen`仅在Wake采样evidence.armed，缺ReadCaptureRequest终态、safePointSerial、检查点/所阻root/HasFlight/transport idle、deadline及firstFailure；如果ARM后又冻结才到采样也不能据false判从未ARM。D2阶段EXIT硬写OK，即使D2_ARM_AT_CHECKPOINT断言FAIL；join无独立测试wall终止机制，g_d2Quit只设false未设true。当前无法区分未到safe point、请求已timeout、scope改变、root/呈现未退或采样漏见，须补精确状态，不猜产品预约缺陷。

T459当前runArm替换为t459Hook，仅观察/退出，没有调用offerWords（定义未用），也未运行T422 actualCadence的SOURCE hook。旧acceptedCount=8与QUANTUM账一致，未见该8条含SOURCE的证据；T422 actualCadence另有Wake内OfferLocked，但payload/control/时钟/触发条件与T488 OfferUiLocked不同。故“T459 Wake塞SOURCE成功、T488只差线程”不成立。

允许tester做**新的同源定位对照**：同一M1构建、同一初态/32U32 command/identity/session、同表两域stamp、同Ready/许可和owner切点，一臂由owner Wake测试hook提交，另一臂由独立协调器在同一锁外交接点调用同一个OfferUiLocked，再沿相同有限业务里程碑与排空。只改变调用线程，不改变入口、SOURCE时刻或消费顺序；两臂输入及前置不能匹配则DRIVER_INPUT_MISMATCH。该定位实验不替代外线程正控，不能改成Wake塞源后宣布原D通过。所有GL仍在owner线程。

## 5. 最小tester修订合同与精确证据

tester，P5，不使用真机。按本稿修订T488的新版本/接续测试卡，改动仅host测试目录、构建适配/有界日志，不改产品行为或阈值；先完成来源一致，再D1/D2，E已观察子集保留。不以另造假transport代替真实实现。

### 5.1 固定时钟及交接

每轮/每臂独立不可覆盖目录，业务baseline在Create/合法session/真实pulse与Ready完成之后。动作表含固定pulseSeq（只给pulse独立序列，不从live字段生成）、SOURCE实际字节或可验证的确定性绑定规则、reserve/noop、END和有限尾部pulse。两臂动作id/seq/payload/两域stamp一致；业务同stamp动作顺序明示，每个pulse只发一次，不对相同stamp重复发heartbeat。

使用独立调度mutex/cv：owner在预声明锁外测试hook发布checkpoint/ownerTurn后等待permit；协调器等同checkpoint，放调度锁后拿App锁，设脚本stamp/送当前动作组/立刻落盘，放App锁再放行owner。owner仅推进到预先固定、capture无关的业务里程碑。可在同一脚本stamp多次Pump/Poll等待真实fence，不调用pulse、不隐式推进脚本时间；wall只为父进程有界守护。busy期间仍须有外线程SOURCE送达案例，不把全部动作退化为等idle才送。

预声明每动作组最大ownerTurn及wall守护预算，未达里程碑则保存当前root/阶段/返回码为NOT_MEASURED，不依据on是否ARM动态放行更多业务；宿主慢/JIT如超守护也保最后ENTER，不解释成设备250ms超期。空闲里程碑要核Run.active/HasFlight/Submitting/Queued、App mailbox、transport pending/retire、permit与accepted序号；这些Run/GL字段仅owner读并复制定长快照，control不可在owner未暂停时跨锁直接读Run造成数据竞争。

### 5.2 最小观察槽

只记录首SOURCE及前几个plan/root的有限轨迹，全部有run/arm/actionId、线程、脚本native/UI callback/host域标签。必需点：

- SOURCE API enter/return：实际32U32 bytes/SHA256、Allowed/identity/input/boundary、session/open/Ready、sourceEnqueued、mailbox head/count、回执；ENQUEUED不当ACK。
- owner消费前后：Tick.active/complete、HasFlight/Submitting/Queued、pending/activeRemaining/planId，FrontLocked是否调用及失败类、Run::Offer原码、Pop与expiry/cancel计数。expiry必须有stamp/enqueued和原阈值，不从终态source0猜过期。
- Claim/Advance/Pump/accept：grant/nonce/kind及state/event/completion/publication/permit身份；首错与成功前缀独立保存。scope/真实GAP/250ms撤权及SOURCE变更保留原语义。
- END前、排空每个里程碑及quit前：未完成具体queue/root/fence/lease、quota守恒。固定尾表耗尽仍未空就报未完项，不quit清理后追认DRAINED；capture冻结与业务排空各有终态。
- D2：真正Flush/Wait/accept前或Draw/Swap切点在App锁外暂停，记录OS tid、callEnter/未返回和root；control用生产Reserve/Read接口，保完整requestId/requestNs/armByNs、blocker变迁/safePointSerial/ARM→BEGIN→END/FROZEN及首错。正常路径保证预定有限业务终点，超时路径父进程保最后阶段；请求timeout后晚checkpoint不可ARM沿T422已交S4复用明确范围，需同源适配才补必要回归。

这些观测优先现有Mark/trace/request冷接口；需要host hook仅测试编译域，不能在非host加入线上业务endpoint/全量读回，不能假写Ready/armed、放宽条件或在control调用GL。

### 5.3 持久化与断言

当前Emit只push g_rows，最后E结束才写；detail直接拼接并截断JSON，最终run的RESERVE及CONTROL_RESERVE两行均非合法JSON。全文件4590行、24行非法，含12个run；断言没有run/arm，且`for(g_diff) log<<rows.str()`重复整组17次。stageLog还把D1/D2的arm固定写成e，D2 EXIT OK与失败断言冲突。修复应使用正确JSON序列化、结构化完整有界request而非substr，超限明确截断flag和原bytes/hash；action/阶段结束当时append+flush，关键边界同步持久化。D先落盘且独立子进程，不能等E结束；每断言唯一ID只写一次，带runId/arm及原件链接。构建脚本/父runner要实际具备外部timeout并留退出码；现build.cmd本身无timeout，不能只靠注释声称已有。

必要检查：同数量错序/漏拍/多拍/seq或payload单字变更都被拒；源树/类布局不一致开跑前拒；dump中含引号/换行仍合法，崩溃截断仅末行；D产物先于E可读且旧run不覆盖。正控要求真实外线程SOURCE接受、非空QUANTUM、完整逐root差分和capture生命周期，账相同但业务全空必须NOT_MEASURED。D2无源码观测不能把ARM false定同因；若同源受控正控仍不消费或不ARM，带第5.2节首个未进展切点回architect，再判相应developer，不要求tester自行修产品。

## 6. 执行约束及本卡完成边界

各执行prompt：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它可做部分。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用既有oracle。背景前景仅XComponent+shader，禁止Canvas/软件GL产品回退；host PC_PBUFFER/软件GL只作显式HOST_SCRIPTED_OFFLINE测试，不冒设备上屏或产品回退。产品故障FAULT或标STALE上一完整有效帧。33,333,333ns GAP、250,000,000ns root deadline、120Hz quota保留，显示目标30fps。成功回原卡且本人行结束待验收；失败/关键缺证写未完项、复现原错、原件、尝试、不能自行继续原因、所需前置与接手角色，本人行看评论。

本卡完成原件/来源/布局/消费与预约路径归因及可执行工程修订，确定先交tester。缺失的历史owner进展、完整request及实际两臂字节送达不能补造，已给精确补证；不把未知产品域记PASS，不直接派删除消费门或Wake塞源的产品修复。本轮只读编译审计与原件解析，不运行GL/真机，不改旧测试或他人任务。
