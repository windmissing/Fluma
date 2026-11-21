# T754 · DM SOURCE 根超期与工件身份归因

2026-11-21 · G20 · P5 · architect · 离线交付。

**T751 的正式复测使用 T748。实际失败为 `Result::DEADLINE=6` 和 `ControlResult::DEADLINE=9`，窗口为第 12 次 dispatch/barrier 已提交、执行 fence 尚未创建。** 250 ms 门按现有实现触发。精确的超期检查点、耗时调用、GPU 实际完成和退役结果还需原始时点。本轮把测量与部署收据修订写入 T736，交 developer 实施诊断并编译。 [T736 修订](architecture-T736-v1.md) · [T754 联核原件](architecture-T754-evidence-v1.json)

## 两次原日志和实际 DSO

| 原件 | 同次身份与实际结果 |
| --- | --- |
| `dm21-out`，25 行 | 旧 T745 `BOOT_ONLY`。PID **4219**、startTick **5850766**、ticket 1、run 7/context 11。driver BuildId `a2f4a69e…`、DSO `9b6cc802…`。BOOT fence 37146、WORK 8 B `[0,0x641647]`，`CREATE_EXIT=1638`，`rootsAuthorized=false`。保原报告所记传输未覆盖旧 driver 及重发过程。 |
| `dm21-out2`，26 行 | 正式 T748 `BOOT_SOURCE`。PID **6695**、startTick **5882046**、ticket 1、run=context **58820616386719**、owner 1。driver `0a496a947e090be7d462e3951ffe25d71ac8faf1`，DSO `63b69fe3f7e825b81ab730c67fd02b08c1d42172`。BOOT 单独有 fence 37146、成功 map/unmap 和同一原尾。随后 `CONFIGURE_EXIT=0`、baseline/offer/begin 全为 0，再到本次 SOURCE 根拒绝。 |

T751 摘要中所列 PID 4219、T745 BuildId 和 `CREATE_EXIT=1638` 属首次旧次。正式次的 BOOT 与 SOURCE 原始记录均可在 PID 6695、ticket 1、同 run/owner/context 下联据。两轮的 BOOT shader 字节相同（22,353 B），但 BOOT roleSet 分属各自变体。正式次 BOOT roleSet `0ae2aa44…`，整体 BOOT+SOURCE roleSet `8a0efe36…`，SOURCE 单独 source SHA `fcb0fe5c…`、41,802 B。`CONFIGURE_EXIT=0` 经实际代码可知 SOURCE build 和反射已成功。BOOT Program 被删除后才建 SOURCE，原件两次都返回 GLuint 2，身份以角色、源与 program 生命周期绑定。 [旧次原日志](../test/reports/t491/t738/v7/dm21.jsonl) · [正式原日志](../test/reports/t491/t738/v7/dm21-v2-final.jsonl) · [程序切换](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L408)

MD5 差异来自比较了两个字节版本。复算正式 driver 的 MD5 为 `0804440281343f2b4fc460b71b97401c`。T748 交付目录的 **1,353,304 B** 剥离 DSO 与 HAP 内 payload 逐字相同，MD5 为 `b99227cc35c2762d6321aed414e37de5`、SHA-256 `fb8d734f…`，与 T751 报告的设备 MD5 前缀一致。**5,436,792 B** 未剥离链接 DSO 的 MD5 恰为 `780cc7c1e3462d3b85966df1fa8fcd56`、SHA-256 `e0fd1ec2…`，两个本地 ELF 的 BuildId 相同。正式次映射 BuildId、设备 MD5 前缀和运行角色均支持 T748 打包版。现存编译回执 `signed=false`，正式次为独立 ARM64 驱动实际运行。设备端整文件 SHA 和签包工序以原有回执出现时核验。后续部署比较 `driver` 和 `pairedDsoFromHap`，保 `linkedDso` 作构建追溯；给新目录保实际路径、完整 SHA-256 及进程中的两个映射 BuildId，再开始计数。 [T748 编译回执](../test/reports/t748/compile.json) · [T745 编译回执](../test/reports/t745/compile.json) · [T751 现场转录](test-report-T751-v1.md)

## 最后已见阶段与超期窗口

- **根前冷段。** 正式 BOOT 编译 **188.274 ms**、链接 **515.847 ms**，BOOT_AUDIT_OK 为 `58821451366875 ns`。到 SOURCE `rootBeginNs=59137486619531` 的原时差为 **316.035 s**，包含之后的 SOURCE build/reflect、配置收尾和 admission。该记录 `cacheConfigured=false`，缺 SOURCE 自己的 compile/link/reflect 原时点，不能把 316 s 全部归给任何一个调用。这段与随后根的 250 ms 计时分别登记。 [正式原日志](../test/reports/t491/t738/v7/dm21-v2-final.jsonl#L22)
- **根内提交边。** `deadlineNs=59137736619531`，与开始恰差 **250,000,000 ns**。实际 `kind=1/ordinal=0/nonce=1/plan=1`，提交侧有 12 dispatch、12 dispatch barrier 与 3 copy barrier，最后 pass 11 `SOURCE_SEAL`。其 nine-tuples 按 T736 全部匹配，最后 B/C/W/E 为 GL 对象 **2/3/6/7**，BASE/CANDIDATE 为 1/3。按原源码，第 12 次计数先于末尾 `Allowed()`，而 `Pump` 只在 Dispatch 成功返回后自增 `next_`。所以 `dispatches=12` 仍允许内部最后一个 `next_` 没推进。 [提交边](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L685) · [T736 pass 表](architecture-T736-source-contract-v1.json)
- **拒绝路径。** `Transport::Denied()` 用实读时间 `now>deadline` 返回 DEADLINE。结合 12 次提交和 `fenceCreated=false/fencePolls=0`，原码可定位在最后 Dispatch 的 post-guard、Pump 循环结束到 `glFlush` 前、或该 flush 后的 pre-fence guard。Controller 收到 `Transport::Pump` 的非 OK 即撤权并原样返回 6，记录 controlFault 9。`Controller` 的映射后精度检查、typed proof、零配额提案和 `CommitAccepted` 均在其后。`ALLOWED` 首失败谓词、最后 Pump 的确切 `next_`、最慢调用及是否有 owner/heartbeat 调度间隙尚未采。`heartbeatResult=0` 可能只是初始值或最后一次返回，下一轮需取逐 pulse 或等价的有界统计。 [Allowed/Denied](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L105) · [Pump/fence](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L741) · [Controller 回退](../entry/src/main/cpp/gpu_resident/g9dm/controller.cpp#L81)
- **原尾与输入。** `workObject=0/workReadBytes=0/mapReturned=false` 来自未进入 WORK map，dispatch 仍可见 W=6。`admittedQ20=0/outcome=0/expectedSHA=0…` 来自未填充回执，SOURCE 原 1024 B、真实 admittedQ 和 typed gate 均未到达。Config 和按真实 run/context 重打标的种子 SHA 已用原模板复算。现场文件清单列出 `source-command/input/config.u32`，当前归档未含这些字节。输入使用本次声明的 SHA，`209,715 Q20` 继续作为本族 oracle 预期。 [正式 SOURCE 事件](../test/reports/t491/t738/v7/dm21-v2-final.jsonl#L26) · [复算与绑定](architecture-T754-evidence-v1.json)
- **撤权和退役。** `RevokeLocked` 清 active grant、pending、job/rootLive、ready/open，并保持已提交状态。现 `queueAfter` 只在成功路径采，失败默认 0，不能当 FIFO 已消费。Scope destructor 在 `SOURCE_ROOT` 行之后先解绑 trace，再 `Stop→Drain`。若 GPU 仍忙且无执行 fence，`RetireRejected` 会创建自己的退役 fence、flush、轮询，成功才解除 busy；超时沿现无复用、同进程上下文收尾。原 SOURCE 行的 `fenceCreated=false` 仅覆盖执行 fence。退役 fence、poll、成功/超时和原始退出码还缺各自具名终态。 [原撤权与 Drain](../entry/src/main/cpp/gpu_resident/g9dm/controller.cpp#L136) · [退役 fence](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L897) · [执行器 Scope](../test/t670/dm_source_run.h#L15)

## 工程修订和最小补证

| 接手 | 交付边界 |
| --- | --- |
| developer · 真实接线与编译 | 在研究变体做有界内存 trace。保原 DM Transport/Controller 顺序，用同一 `CLOCK_MONOTONIC` 记录实际 SOURCE cache/retrieve/build、driver `glCompileShader`、`glLinkProgram`、反射/首使用的 ENTER/RETURNED/EXIT。根内记 Begin、upload、每次 Pump 的 enter/exit/next、12 组实际 dispatch/barrier、3 copy、`glFlush` 前后、各 guard 检查点的实际 `now` 与 first-fail、执行/退役 fence 的原返回与最后 poll。带 last pulse sequence/ns、实际 native gap、permit/scope/owner 状态。可用定长缓冲与溢出计数，避免在热路径打印或同步 I/O。Timed return 先记录再检查预算，分别导出 `submitted`, `advancedCursor` 及 `fenceAttempted/Created/Signaled`，未发生值用 null/未采掩码。保测试器 `Scope` 退役前后归同根但另名，并在其后导 terminal；负控覆盖真正晚尾与有/无执行 fence。本项只开发和编译。 |
| effector · 同工件最短复测 | 在原 935F 实际环境，使用带停止/隔离回执的新 ABI 工件和原 K192/S40 fixture，部署比对两个工件的完整 SHA 与独立映射。单次全新 native scope，T748 原 command，按原零配额 admission 只 claim 一次。先保 BOOT 8 B，然后保 SOURCE phase 日志、实发根日志和独立退役原件，归档实际 `source-command/input/config.u32`。若又超期，补足最后 true guard 到 first false 原时点、API 边、heartbeat、实际 GL/EGL 返回和真实进程退出状态。若调用截尾，记录最后 ENTER、原观察窗口和上下文退役。若实际达到 map，保同 fence WORK 1024 B、unmap、typed proof 原字和原 commit/quanta/FIFO。 |
| tester · 离线 reader/oracle | 复用 T736/T647 固定阈值与字段，独立分辨执行 fence 与清理 fence、未观测字段与真实零。覆盖坏包/同 BuildId 不同 payload 版本、时限与 freshness 独立失败、错 scope 迟到 no-commit、有效同根 `admittedQ20=209715`。GPU 尾未到达的尝试保失败原件及准确阶段。 |

两条时间问题按原边推进。若冷时长集中在 SOURCE compile/link/首次使用，沿 T727 单独做真实角色闭包缩小、缓存与冷暖对照。成功持久化仍须经实际 SOURCE 接受和准备，暖臂用同 key 的真实加载回执。若根内主要是 driver 提交/flush、GPU 工作或 CPU owner/heartbeat 调度，按实际失败点决定减提交成本、缩等价闭包或纠正时序，并核 T736 原 12 pass、3 copies、屏障及 oracle。保 **250 ms 根期限**、**33,333,333 ns freshness**、原零配额退役顺序和拒绝事务。禁止用延长期限、重置 permit、合成 pulse 或已撤权的尾推进物理。pbuffer 的 SOURCE 正例可证 real GPU/Controller，真实有效 Swap 自带 XComponent+shader 场景和同一有效 token。GPU 驻留优先，CPU 停投，禁止 Canvas 回退。 [T727 冷暖量测](architecture-T727-v1.md) · [T736 实施合同](architecture-T736-v1.md) · [原 PrepareSourceCache 条件](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L930)

离线复核命令 `python tools/architecture/analyze_t754.py`。本卡未使用真机。**G20 仍为 4/12，约 33%**。DM SOURCE 取得实际配置与提交证据，执行 fence、原尾、首个有效 Swap 仍待证。 [T741 计分](architecture-T741-v1.md#g20-完成度口径)
