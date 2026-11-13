# T478 · 对称驱动器与lease阻断归因及工程合同

2026-11-12，architect，P5，不使用真机。接续T465 D/E，核T471原件、实际线程/判据及真实transport lease。证据索引见 [本轮原件核对](analysis-T484-T478-T480-evidence-v1.json)。本稿替代“worker单线程消费即可证明生产异步采集”的笼统建议，不改产品控制/采集合同。

## 1. 首次归因

T471第一版完整运行的原始summary/双臂日志已经遗失，只有报告转述，不可恢复为PASS。现存 `test/reports/t471/t471-run-symmetric-pairs.txt`有真实分歧，但off/on跳过的pulse不同，不能归为同输入产品差异。报告声称的 `test/reports/t471/pulse-log.ndjson`不在；本地 `.codex/local-test/t471/pulse-log.ndjson`只有**16行**，不是报告的19拍，也没有runId/arm标签可证明它就是所述停滞run。必须按各文件hash保留，不补造失去的旧版结果。

| 实现切点 | 问题与证据边界 |
|---|---|
| `for i++` + try_to_lock失败continue | 未送达也推进i，丢脚本动作；local日志相邻native时间有32ms跳跃。seq又用progress.sequence+1，把丢拍压缩成连续序号，掩盖缺输入 |
| `!progress.open`时continue | 失效后的预定动作也消失，没有拒绝回执，不能验证关闭态旧pulse行为 |
| PumpBegin on臂spin等待g_controlReserved | 只on臂等待，off臂已跑业务；若固定reserveAt拍被try_lock跳掉或先失效，条件永不成立。此路径足以造成无界等待，但历史19拍run无栈/阶段原件，不能唯一归因为该切点 |
| 脚本耗尽直接`a.quit=true` | 没有排空条件，queued/active/lease未完成就退；报告“排空终止”未落实 |
| `cmp(acceptedCount, off.generated,on.generated)` | 比的是generated，不是a.accepted或progress.accepted；接受量不能由产生量代替 |
| completion size==size且允许0 | 空对空也记PAIR_OK，缺非空业务见证；不证明采集无扰动 |
| 全局g_records每臂clear，双臂结束后才写文件 | off日志丢失，只剩最后一臂；没有每动作run/arm/输入哈希与先落盘回执 |
| E守护的leaseDone为内层局部变量，线程detach | 出作用域后线程继续按引用访问，存在悬空引用未定义行为；在E开始前该变量已经销毁。不能把240s文字当可靠阶段时钟证据 |
| “先落盘”代码在E结束之后 | 守护_exit时尚未执行，D/E证据仍会丢。无持续enter/exit，无法确定超时在Create/JIT/fence/Release哪一步 |
| E返回码只记录却passed++；STALE分支失败也再次passed++ | 失败能计通过，必须修真实断言和计数，不把“记录成功”算lease语义成功 |

这是测试工程问题，直接交tester修订工具，不需要产品developer提供“轻量假transport”。T475已交真实工厂缓存，T477还获得实际compute冷热命中，不能继续把没有阶段证据的E超时猜成JIT。真机上屏与独立脚本对拍是不同范围，当前未测项仍NOT_MEASURED。

## 2. D：固定脚本与异步入口必须分别验证

采用两个互补子集，均调用实际生成App/Run/transport及生产 `ReserveEvidenceLocked→TryArmReservedLocked` 接线。禁止在测试直接写evidence.armed、手调ARM成功、修改quota或在恰好空闲时绕过真实预约。

### D1 确定性业务差分

用一个独立协调器管理不可变动作表，记录`actionId/kind/session/seq/uiNs/nativeNs/payloadHash`。动作表包含固定真实SOURCE command及QUANTUM pulse，reserve/END为同一动作点的on副作用，off执行noop；所有业务动作两臂字节及顺序一致。脚本时钟只由协调器推进，不按wall sleep、accepted数量或锁竞争推进。错session/关闭态也必须调用入口并记录拒绝，不跳动作；序号用表中固定值，不从产品sequence生成。

owner在测试编译域的Wake/明确业务checkpoint通过**另一个测试调度mutex/cv**交接，进入barrier前不得持App mutex、GL调用或publication lease临界交接锁。协调器先等owner的预声明checkpoint，释放调度锁后取App mutex，送一条或一个明确动作组，记结果并释放App mutex，再确认动作已处理。动作成功处理或明确拒绝后才能推进nextAction；try_lock可作诊断但失败必须重试同action，不得continue到下一拍。禁止任一线程持测试调度锁等待App mutex，而另一线程持App mutex等测试锁。

初始化/JIT完全在脚本业务baseline之前，两臂用相同初始状态和配置，在Create完成、合法page session交接和真实首pulse后开始同一business表。App supervisor仍按脚本nativeNow检查控制；创建期间宿主wall耗时记录为HOST_ONLY，不冒业务deadline。业务阶段GAP/250ms值不改，不在poll里隐式推进时间。允许多次零时长poll观察同一固定时刻，实际GPU fence完成才消费后续里程碑；wall只决定外部守护，不改变业务输入。

每个动作组之后owner只能推进到事先指定的**业务**里程碑，如“当前root完成接受/拒绝”“当前无在途根”，不能以capture是否armed决定off臂跑多少工作。里程碑在两臂相同，超预算/条件不可达就输出具体当前状态终止为NOT_MEASURED，不能改动作表追认差异。留至少一组在root尚忙时送pulse/offer并复核mailbox路径；不能整卡退化成只对理想空闲态调用private helper。

终止分两阶段：固定表耗尽→停止新增业务并按既定END动作冻结capture→排空此前已送的queue、activeRemaining、未封提交、fence/迟到尾和lease。脚本后续控制动作若排空仍需pulse，须预先列入尾部有限表并在两臂相同执行；不得临时“保持新鲜”无限补pulse。最终条件及原账落盘后才quit。尾表不足则报具体未排空项，不写PASS。

### D2 独立控制线程预约，必须保留

单线程顺序消费能证明规定输入下的业务等价，**不能证明控制线程在owner阻塞时可预约**。D2至少两种真实生产接线路径，使用与D1同一有界原件格式：

1. 在已开始root的GL Flush/等待/accept前或Draw/Swap前测试切点暂停owner，暂停点必须在App mutex外；两臂使用相同切点和脚本时间。独立control线程在busy期间调用生产预约入口，得到RESERVED及真实WAIT_ROOT/WAIT_PRESENT等blocker。它只保留请求，不直接ARM。
2. on/off都完成同一有限pulse动作序列后解除暂停，owner从原控制流继续，在实际合法checkpoint ARM；核请求id/期限不重置，产生BEGIN/首offer/accept/END/FROZEN。再做一次owner不归还、请求ARM超时的负控，supervisor必须能独立完成请求终态，晚checkpoint不能ARM；另做真实GAP/250ms撤权和迟到尾负控。

D2的控制调用、owner barrier及supervisor都记录OS线程id与phase enter/exit，证明真实异步入口被行使。调度等待不占App锁，不放GL到control线程，不以“只是HOST”跳过撤权。这个子集不能由D1或T422旧自测通过替代；T422用例和原件可复用其明确范围，不重复全套五场景。

## 3. D判据与原件

每臂独立新目录，执行前写不可变script/expected/来源manifest及hash；每个action与阶段结束立即append+flush，关键边界同步持久化。读侧流式JSONL每行含runId/arm/actionId/phase/tid/native脚本stamp/hostStamp及输入输出hash，U64用十进制字符串。BEGIN、callback原UI域和native接收域分别保存，不跨域相减。崩溃仅最后不完整行判截断，不能覆盖另一臂或上一轮。D/E分别子进程，D产物不等E结束才写。

比对按action与业务root身份，至少独立核：a.accepted实际总接受、progress.accepted实际QUANTUM接受、SOURCE/physics commit、generated、pending、active、overload、revoked、carry、clearedCarry和守恒式；逐root接受顺序、state/event/completion、publication/root token、首错及成功前缀。captureId/诊断serial可以单独比较，不可把业务root身份不等也一起“规范化掉”。要求至少一个真实SOURCE接受及非空QUANTUM接受，completion不能为空；没有合法业务接受就NOT_MEASURED。首错取FIRST_FAILURE或真实业务错误，正常EXPLICIT_END不当错误。

每条断言有唯一id、expected/actual/原件链接，passed只由bool结果计一次，记录条目不算assert。相同script hash是必要条件，还要核**实际送达**的actionId/seq/两域stamp/payload回执逐条一致，漏送/多送判DRIVER_INPUT_MISMATCH，先回architect，不能算产品差异或“环境噪声”。相同送达后首个真实产品分歧保留原数据，回architect归因，不在test里改expected。

## 4. E：真实transport一次创建复用与正确lease序列

每个测试子进程在真实GL owner/context创建一次实际M1 FullM1Transport，复用该对象跑所有能独立恢复的正常lease周期。使用T475生成M1的ProgramFactory三program路径、独立私有cache目录和真实source/layout/环境key，保留cold/miss与load收据；不得只include工厂头却仍调用未改造的共享Create。缓存不命中也必须输出Build各阶段，不能预先假定可在240s内完成。需要ContextLost的终结性负控放最后；独立新context试验才另创建，理由写清。

正确序列如下，所有返回码必须断言：

1. 真实SOURCE成功，取得slot S/generation G及接受前缀；Acquire(S,G)=OK，重复Acquire=BACKPRESSURE。
2. 创建release fence并ReleasePublication(S,G,f)=OK，检查同一releasePending的Retire。**真实fence可能在下一条API前已signal**，不能把glFlush后的即时Acquire无条件判背压。记录真实wait返回：若TIMEOUT，要求BACKPRESSURE；若已signal且Retire完成，允许同G再次OK，但必须归还该新租约。要确定性覆盖未signal分支，可在仅测试GL转发层对指定release fence把wait结果暂设TIMEOUT，标RELEASE_WAIT_INJECTED，不混作真实未signal设备证据；解除后仍由实际fence完成退休。
3. signal/Retire后，publication尚未覆盖时Acquire(S,G)仍可OK，不应STALE；随后真实Release并等待该次租约也退完。
4. 接受新root并**覆盖同slot S**为G2，再核Acquire(S,G)=STALE和Acquire(S,G2)=OK；只在另一个slot生成新publication不能证明S的旧代失效。用递增frame/nonce/control，eventArena必须等frame.lo&1，不能沿T471固定arena=1去提交frame2。
5. 保G2租约时reset/失效，核不得提前复用/释放，晚fence只退休，不跨context接受/Swap；按原失败语义清理。Release/BUSY和context owner约束都记录。

T471当前E中acq2若恰好OK未归还，会污染后续lease；glFinish后直接期待旧G STALE也违反产品合同。两个“passed++无条件增加”必须删除。这些是用例错误，不授权更改publication Release/Acquire语义。

外部父进程做wall守护，启动前建立目录，子进程在EGL/Factory Build/Create/SOURCE submit/poll/Acquire/Release/Retire/替换/清理前写ENTER，返回后写EXIT及原码。父进程超时保存最后阶段、心跳/退出码和未完项为NOT_MEASURED；若使用内部守护，其状态必须比守护线程活得长、正常路径join，不detach引用局部变量。不能强杀GL线程后继续同context；超时结束测试子进程，留原件，下一独立用例新进程。240s是宿主设施守护，不替代250ms业务期限，不从最后ENTER缺EXIT直接推驱动/JIT内部根因。

## 5. 直接可派tester修订任务

P5，tester，不使用真机。按本稿重建D1确定性业务脚本+D2独立控制线程预约+E一次真实transport复用及lease序列，改动仅新测试目录/现有测试GL转发与编译适配，不改产品行为、线上接口、时间阈值或私有字段强行READY/ARM。优先产物持久化、真实输入等价和守护生命周期，再执行实际GL链；复用已交T475/T476原件与T422明确覆盖，不重开T471。当前shared bridge和新T467/T475生成源不同，必须固定被测生成来源/ELF/shader/cache helper hash；若需新测试hook，只放host编译域并核非测试构建无新增入口，不要求另造产品transport。

至少执行漏拍负控能拒、输入一致且有SOURCE/QUANTUM接受正控、busy异步RESERVED→实际安全点ARM、请求超时晚到不ARM、真实fence退休/指定wait注入/同slot替换、守护超时阶段保存。局部GL长期阻塞，先完成静态/离线原件判读与其余工具正负控并报告具体未测域。独立补证结果先回architect，manager不重跑、不增开发关闭门。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它可做部分。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用既有oracle。前景背景只XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE完整旧有效帧。成功回原卡且本人行结束待验收；失败/关键缺证写未完项、复现原错、证据、尝试、为何不能自行继续、所需前置及接手角色，本人行看评论。

本轮完成源码、遗留日志与归档边界核对和可执行工具合同。T471历史停滞的具体线程栈与E首阻塞调用仍缺原件，明确保持未知；本卡已完成对此缺证的精确分流，没有运行产品/真机或追认旧PASS。

## 2026-11-13 T488接续归因补充

[T494工程裁定](architecture-T494-external-source-reservation-triage-v1.md)确认T488交付build混用共享App/类头与冻结implementation，MSVC同名FullM1Transport布局216704/182640字节不一致；先统一完整M1源树。D1仍缺锁外业务里程碑交接及真实排空，40/40仅总数相同、不足以证明实际送达等价；D2未送SOURCE，不能与D1消费缺证定同因。E12项通过原件保留并标来源边界，不冒任意一致产品构建已证。先交tester修来源/调度/结构化持久化及精确观测，再回architect分流，禁止删除消费门或改Wake塞源追认。
