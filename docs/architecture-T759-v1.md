# T759 · RX SOURCE pulse 与 owner 时钟接线

2026-11-21 · G20 · P5 · architect · 离线归因。

**T755 已通过首次 SOURCE claim 和 `Transport::Begin`，随后在 `Transport::Pump` 返回 STALE。** 源码可复现 owner 时间和已发布 pulse 取样顺序错误，也可复现旧 owner 时间把刚入队命令误判过期。正式原件尚缺首次失效谓词及 Pump 游标，不能确定其中哪一处触发了 T755。编译链接全部发生在 `APPLIED` 前，沿用原 **33,333,333 ns** pulse freshness 与 **250 ms** 根期限。 [T759 原件联核](architecture-T759-evidence-v1.json) · [T736 接线修订](architecture-T736-v1.md)

## T755 同票时序

签前 HAP `6cdcfc7d…`、签后 `7f8a2900…`，ARM64 ELF 字节一致、实际映射 BuildId `7bd1cae40ac3e385074b123882290ee18cacfee8`。T752 BOOT 22,318 B 与 SOURCE 41,767 B 原样内嵌。正式日志为 PID 31536 / owner TID 31696、ticket 1、run/owner/context=1/1/1。BOOT roleSet `e33bdfca…` 与整体 BOOT+SOURCE roleSet `b718fe04…` 分别对齐，SOURCE SHA `ee66f5f2…`。BOOT 有实际 fence 37146 和成功 map/unmap 后的 `[0,0x641647]`。本轮没有归档该 PID 的 start tick 原值，T749 的旧 PID/start 仅作前置历史。 [T755 验签](../test/reports/t257/T755-rx.json) · [T752 构建收据](../test/reports/t752/compile.json) · [同票 BOOT](../test/reports/t491/t755/h755.txt#L1466)

| 同设备单调时钟原边 | 原始时点或状态 |
| --- | --- |
| Configure ENQUEUED；BOOT_AUDIT_OK | `84100636318594`、`84101330326094 ns` |
| SOURCE 编译 ENTER→EXIT | `84101339652969 → 84186663625156 ns`，**85.323972187 s** |
| SOURCE 链接 ENTER→EXIT | `84186663671875 → 84422027915312 ns`，**235.364243437 s** |
| SOURCE reflect；CONFIGURE_RESEARCH_EXIT；APPLIED | program 4、40 uniforms、qualified=1。`84422030339219 → 84422103143594 ns`，两次 result=0。配置入队到 APPLIED **321.466825 s**。 |
| UI 终态 | `l4.json` 只显示 `PUMP_FAILED code=5`。系统栏为约 15:39 的截图时间。实际失败 `ns`、第几次 Pump 与原 guard 快照均未采。 |

T752 页面在同票 `APPLIED` 且原 owner 确认实际表面尺寸后才 `SetActive`、读取有效 session、`g9RxSource` 入队、发 22-word 真实 FrameCallback pulse。`PulseLocked` 首次可设 `baseline/hasPulse`，并将 NAPI 入口的 `CLOCK_MONOTONIC` 接收时点发布。在该 first pulse 之前，没有该页面的 SOURCE root deadline。`BeginRoot` 的 `SourceClaimGate` 需要 baseline、fresh、零 pending 和合法队首。之后冻结 nonce/plan/rootBegin/deadline，`Transport::Begin` 校验、上传并置 `busy_`。所以原 UI 终态和源码共同支持 **已有合法队首、已接受 baseline/pulse、已 Claim、已 Begin**。它不能证明 `dispatches=0` 或第几次 Pump，也不能给出 INPUT128 原字和 209,715 Q20 实际准入量。T755 摘要的「首 pulse NOT_REACHED」改按未采原时点理解。 [编译及 APPLIED 原边](../test/reports/t491/t755/h755.txt#L1493) · [UI 原件](../test/reports/t491/t755/l4.json) · [原页面入口](../entry/src/main/ets/pages/GpuResidentG9Rx.ets#L49) · [原 owner Claim](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L532)

### STALE 能定到的边界

`PUMP_FAILED` 只在 `Controller::PumpBounded()` 收到 `Transport::Pump()` 非 OK 后发布，位置早于读取 typed proof。Pump 先要求 `busy_`，成功 Begin 后才有 live flight。`Allowed()` 经 `Check` 校验 owner active/rootLive、lifecycle、permit、now/deadline、pulse 时间先后和 age、冻结命令 receipt age。`Denied()` 再读时钟，`now>deadline` 给 6（DEADLINE），context 失效或时钟错误也有各自返回。原终态 5 支持 live Pump 中的 guard 拒绝，不能凭 UI 文本从 `now<pulse`、pulse gap、owner 留活、撤权/代号或命令过期中选一个。可能是在首轮进入、copy/dispatch 或其后、flush/fence/pre-map guard。读回后被拒的阶段另名 `TYPED_REJECTED`、`PERMIT_REVOKED`、`COMMIT_REJECTED` 等。当前没有证据宣称实际第 0 pass 已提交或执行 fence 已建立。 [Allowed/Denied](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L96) · [Pump 路径](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L716) · [PUMP_FAILED 状态发布](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L582)

DM T754 是另一个阶段。其 12 个 pass 与 barrier 有请求原件，执行 fence 尚未创建，`Denied` 时实读跨过 250 ms，返回 `6/9`。该轮也有 BOOT→root **316.035 s** 的独立根前区间，缺 SOURCE 编译/链接原边。RX 的编译为根前实测慢点。RX 的前置门在 T755 已被通过，确切为何在 Pump 失效待原证。两族分别用同域原时点判断，G13 的 FTZ 则有自己的原位资格门。 [DM T754 结论](architecture-T754-v1.md)

## 修正已知接线问题

1. **同一时间线上的真实 pulse 快照。** 现在 `Transport::Allowed()` 先读 `now`，随后 `Controller::Check()` 才 load `ownerPulseNs`。若 UI 在两者之间发布更新且合法的 pulse，`now < pulse` 会立刻误判。同理，旧 owner `now` 先传入 `BeginRoot`，之后在 mutex 内 `ExpireLocked` 读取更新的 FIFO receipt，`CurrentAge` 也会返回 false 并错误走 `DropHeadLocked`。两种交错可离线稳定构造，均可能在原 freshness 内发生。用原 9×U64 scope 和 receipt 绑定，先在已有 protocol 锁内取得 pulse/队首/permit/代号，再在同一临界区取真实 owner 当前单调时钟。`BeginRoot` 在 `PollSceneFence` 返回后才这样做，并且在任何 Drop/Claim 前拒绝异常时序。`receipt > now` 单列为时钟/快照先后错误，既有“已过期取消”仅对 `now >= receipt` 且 age 超门生效。首个正控仍以 22-word PagePulse 的真实入口 receipt 为新鲜度基准，发布时点另采。禁止 `max(now,pulseNs)`、手工改时间或合成 pulse。 [现有时钟选择与队首过期](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L65) · [Expiry 路径](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L315) · [离线交错向量](architecture-T759-evidence-v1.json)
2. **owner 检查和原提交锁分别接线。** 对研究变体实现明确的 guard 返回，带首个拒绝谓词、该次 `now/pulse/receipt`、active/rootLive、generation、nonce/plan 和 pending 快照。未持 protocol 锁的 Begin/Pump/Proof 调用可短锁采样，取样后立即放锁，驱动调用、等待、IO 均在锁外。`Controller::PumpBounded` 的提交路径已经持该非递归锁，`CommitAccepted` 还会回调 `Check`。这里必须使用明确的持锁校验入口或同一锁内直接提交校验，避免 Check 再锁同一 mutex。保 `ownerActive` 及 lifecycle 的即时撤权和最新 program/context。取样后若遇真实撤权，前后原 guard 至少一次拒绝，迟到结果只退役。`Target(0)` 目前也写 `ownerPulseNs`，新研究 guard 的 freshness 只消费 `PulseLocked` 成功发布的 PagePulse，target/一般 UI 活动独立记。 [原提交调用](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L628) · [原 Transport 提交](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L827)
3. **保现有 post-compile claim。** 同票 `APPLIED` 和实际 EGL extent 成立后保持页面 `SetActive→读激活后的 scope→真实 source 入队→真实首 pulse`。owner 用该 ticket/geometry/command/first pulse 唤醒，在不打断 cold pin/scene fence 或旧 scope 的前提下优先执行合法一次 Claim，不把编译、数据导出或页面等待计入根。未 fresh 时记录原因；已入 pending 时保原拒绝，不回拨配额。Claim 后的真实连续 pulse 可维持该根，后续 pending 可正常记账但这个研究臂不消费 QUANTUM。`rootBeginNs` 一次冻结，deadline 不因 Pump、重唤醒或 pulse 改写。SetActive 或换窗口只有在真实 retirement 和新 scope 成立才恢复。首根成功仍要 12 pass、12 dispatch barrier、3 copies、同根 fence 和前后 guard、WORK 1,024 B、typed proof、outcome1 / 209,715 Q20、commit+1 / quanta 不变、只消费一次 FIFO 和真实冷档。

同一份实现保退役 fence 与执行 fence 的身份，失败 guard 的原始快照在 `Revoke/Stop/Drain` 前冻结。诊断序列不能被晚来的 lifecycle 或反复 UI 轮询覆盖。验收可复用 T758 的定长事件模式和 T761 reader 判据，沿同一次 RX 的 pid/start、ticket、role，保持既有 controller 端点与 completion ABI。 [T758 有界时序](developer-T758-v1.md) · [T761 独立判定](test-report-T761-v1.md)

## 补证、复测和放行

| 接手 | 可执行交接 |
| --- | --- |
| developer | 在 T752 实际 page/bridge/Controller/Transport 冻结上述时钟顺序并编译。扩有界 trace，独立保存首拒/最近成功/最后游标与退役终态。闭环记录 `APPLIED/extent/SetActive/QUEUE/FIRST_PULSE/OWNER_WAKE/CLAIM/FREEZE/BEGIN/PUMP/FENCE/MAP/COMMIT/RETIRE/SWAP` 中实际到达的事件。pulse 行保 UI callback 原 ns、进入 NAPI/发布的 native ns、序号、结果与 native/UI gap。guard 行保 test/phase + rootNonce、deadline、sampledNow、pulse sequence/receive、冻结 receipt age、scope 预期/实际、拒绝原码及首次谓词。分报真实 dispatch/推进游标、flush/fence/GL/EGL、map 原返回。调用跨预算时保存实际 EXIT 到 guard 的顺序。保已见阶段并在 Stop 前复制到有界诊断，避免每帧热 IO。 |
| effector | 若可取 T755 同进程的完整 `g9RxStatus`，先归档 `source` 和 `configure` 对象及同票 pid/start，能恢复 nonce/plan/cursor 类字段，历史首次拒绝时点仍需新 trace。用已签修订工件按设备队列跑一轮真实 XComponent 输入，保存同票互证的 HAP/映射/BOOT 原尾、SOURCE 编译时间、原 pulse/guard/retire 与真实失败边。若有 typed/commit，归档冻结 INPUT、原尾和 cold，并验真实同代 Swap。未走到的阶段标未观测。 |
| tester | 离线交错控制覆盖 `T=20,000,000 ns` 的 owner 旧采样，随后 pulse `T+1 ns` 才发布，两者在合格窗口，修订后同锁采新 `T+2 ns` 应通过。再测接收时点正好 33,333,333 ns 与 +1 ns、250 ms 与 +1 ns、跨 scope 更新、命令新但 owner 旧、UI Gap/真正停止、第二 pulse 先导致 pending、Target 假刷新、迟到 fence、类型拒绝及 no-commit。必须覆盖持锁提交入口不自锁，负控按真实失效理由及原事务收口。 |

**放行按原证。** 闭合一次 `APPLIED→实际首 pulse→同 scope Claim/Begin/Pump` 后才能归因 freshness。发生真正失鲜仍按原拒绝和退役。完整 SOURCE 成功以原 GPU/oracle/FIFO 判据验收，产品表现以真实 XComponent+shader 同 token Swap 和后续有效 30 FPS 单独登记。321 s 冷链沿 T727 排查 SOURCE 编译、链接和合格后的缓存持久化，保每新增 shader 49,152 B 门。建立冷暖实证仍需真实独立进程、cache identity 和实际加载。仅 GPU 驻留，CPU 停投，禁止 Canvas 回退。 [T727 冷暖量测](architecture-T727-v1.md) · [T736 原序和 oracle](architecture-T736-v1.md)

离线复核 `python tools/architecture/analyze_t759.py`。本卡未使用真机、未修改产品实现。**G20 约 33%（4/12）**。RX 已取得 BOOT+SOURCE 编译、链接和 APPLIED，DM/G13 后续链与三条优化路线的首个有效 Swap 仍待证。 [T741 计分口径](architecture-T741-v1.md#g20-完成度口径)
