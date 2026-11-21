# T768 · DM SOURCE 250 ms 根内超期与提交修订

2026-11-21 · G20 · P5 · architect · 离线归因和实施合同。

**保 250 ms 根期限和 33,333,333 ns pulse freshness。先降低提交调用和忙轮询的成本，再用同根 fence、1024 B 原尾和 typed proof 验证。** T762 的第一笔大 copy 和末批首次 flush 合计占根内 **163.011 ms**。截止前最后一次真实 fence 轮询仍未就绪。现有原件足够定位主要可见耗时和最后拒绝边，GPU 排队、驱动内部等待与 CPU 被抢占的份额仍需量测。 [T762 报告](test-report-T762-v1.md) · [T768 复核](architecture-T768-evidence-v1.json)

## T762 同根原件

PID/TID **54691/54691**，process start tick **9752360**，ticket 1，run/context **97523765513750**，owner 1，nonce/plan 1/1。实际 driver 映射 BuildId `652b030a…`，BOOT 真正加载的 DSO BuildId `88b8e1b3…`，与 T758 本地三件套匹配。报告的 run 7/context 11 属旧次口径。T762 报告记录重发后三件套设备 MD5 对齐，已归档原件缺设备端完整 SHA 和 DSO 映射路径，按报告与本地打包联核分别登记。BOOT 的 fence 37146 及 map/unmap 8 B `[0,6559303]` 同票。 [部署/编译收据](receipt-T758-compile-v1.json) · [逐阶段原件](../test/reports/t491/t738/v8/dm22-final.jsonl)

| 时间域 | 复算 |
| --- | --- |
| 根前冷路径 | BOOT_AUDIT_OK→SOURCE 根开始 **317.306 s**。本次 `cacheConfigured=false`。SOURCE 的真实 compile **82.820177500 s**、link **234.473359375 s**、reflect **0.092656 ms** 都发生在 claim 前。SOURCE build 返回并完成反射后尚有约 **2.252 ms** 才起根。 |
| 根开始到前十次提交 | 根时间 `97841893238281`，deadline `97842143238281 ns`。首次 program 使用调用 **1.563 μs**，第一个 Pump 在根后 **0.248 ms** 进入。pass 9 / SOURCE_COMPARE 后，第一笔 12,767,232 B GPU buffer copy（GL 对象 7→8）从 **+0.975625 ms** 到 **+86.526719 ms**，API 墙时 **85.551094 ms**。同体积第二笔 7→9 仅 **0.028906 ms**。第一批 Pump 仍按 8 ms/10 pass 让出，单次阻塞调用可跨软预算。 |
| 末两次提交和 fence 建立 | pass 11 / SOURCE_SEAL 的 dispatch 在 **+86.658438 ms** 返回。末批第一次 `SUBMIT_FLUSH` **+86.662813→+164.122344 ms**，API 墙时 **77.459531 ms**。执行 fence 创建返回 **+164.158438 ms**，其后一次 `EXEC_FLUSH` **0.469 μs**。12 dispatch、12 dispatch barrier、3 copy/barrier 和 cursor 12 均成立。 |
| 等待及拒绝 | 第一次执行 poll 在 **+164.162031 ms** 左右开始。最后一次 timeout 记录 `97842143235937 ns`，距期限 **2,344 ns**。最后 `PUMP_ENTRY_GUARD PASS` 为 `97842143237812`，早于期限 **469 ns**。下一 `PRE_EXEC_POLL_GUARD DEADLINE` 为 `97842143239531`，晚 **1,250 ns**，两者差 **1,719 ns**。后者在真正调用 wait 前返回。最后一次 dispatch 到拒绝约 **163.343 ms**，guard 的字段 49 是 pulse 序号。 |
| 退役 | 执行 fence handle 1 轮询 **13,069** 次，末返回 **37147 / GL_TIMEOUT_EXPIRED**。退役复用同一个 handle、同一 generation/根，62 次后在 `97842224779844` 取得 **37146 / GL_CONDITION_SATISFIED**，距根期限 **81.541563 ms**。实际 GPU 完成时点至多能按前一条 timeout 和此条 signal 返回夹逼，轮询记录的是观察时点。SOURCE 没 map，typed proof、209715 Q20、commit+1 均未到达。退役收据有 **commit=0、quanta=0、FIFO=1**，根侧旧 `queueAfter=0` 是失败未填值。 |

49 个有效 pulse 无丢样，接收间隔最大 **5.750469 ms**，最后通过 guard 的 pulse age **0.819687 ms**。T762 的 pbuffer 驱动用独立 5 ms heartbeat 和 owner 的 `yield()` 繁忙循环。原件保住了根的提交前缀和独立 terminal 字段，但 2,048 条事件填满并丢 **102,821** 条。前缀的 233 次执行 poll，进入间隔中位数约 **7.031 μs**，没有毫秒级长唤醒；余下区间未经逐边记录。该结果不能外推到产品 XComponent owner 的调度，也还不能将大 copy、flush 和 fence 等待时间拆成确切 GPU 工作量。T751 的旧根死在 fence 创建前；T762 进一步到了有执行 fence 的等待阶段，两者都是原固定期限内未交付有效尾。归档 `dm22-wait.txt` 的文件清单与终态可互核，文件中的独立 `0` 缺命令关联，进程退出码以 terminal 声明 6 和后续真实 shell/wait 取证分别登记。 [T754 历史归因](architecture-T754-v1.md) · [T758 诊断结构](developer-T758-v1.md) · [驱动原循环](../test/t670/dm_source_run.h#L329)

## 工程修订

1. **先替换忙轮询，单独量测。** 只在同代 SOURCE 的根已提交、执行 fence 在途且上次零超时 poll 返回 TIMEOUT 后，使用 owner 的绝对单调时钟 wake 定时器或条件变量。第一臂以 **250 μs** 为目标 cadence，遇真实 lifecycle/cancel 事件立即唤醒，唤醒后重取当前时钟、身份、期限和新鲜 pulse，再由原 owner 走 `glClientWaitSync(fence,0,0)`。下一唤醒不晚于 `min(lastReturn+250 μs,deadline)` 的计划时刻，调度迟到如实计。抵达时限仍未完成则按原 guard 收口，时钟恰等于 deadline 时原 guard 允许，后续每道 guard 仍取实钟。发 pass 待续、等待 fence、故障退役分开调度，能发时不引入 250 μs 等待；任何 GPU 操作和 owner 等待不持 controller/bridge 互斥锁。heartbeat 保原真实来源和间隔，产品采用真实 PagePulse，不能把诊断 heartbeat 接到产品。保持 8 ms/10 pass 提交预算以及原取消入口。首轮以能减少实际 poll 次数、CPU thread time 和 trace 工作量为目标，是否改善根完成看实测。
2. **合并 SOURCE 最后一批提交的 flush。** 做第二个独立可切换研究臂，保中间 batch 到 10 pass/8 ms 的 `SUBMIT_FLUSH` 和所有原 copy/barrier。只有 `kind==SOURCE && next==12`、同 owner/context、原 pre-fence guard 成立且已通过这批 GL 错误检查时，先插入覆盖所有原 dispatch/copy/barrier 的执行 fence，随后在原 owner 做**一次** `glFlush`。记录 fence create 和 flush 各自原出口、首 GL/EGL 错、实读时限与实际 signal。若此前已有提交却发生拒绝或 null fence，把已提交 prefix 交原退役，不以错误同步对象当完成。flush 返回后需新 guard，跨期限即撤权、转原退役。旧 `SUBMIT_FLUSH` 的 77.46 ms 可能移到 fence、最终 flush 或等待段，用根起时→实际 fence signal→typed commit 的总墙时判断，不以某一调用变短记收益。 [Khronos OpenGL ES 3.1 §2.2、§4.1 同一命令流及 fence](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)
3. **追大 copy，不改三笔账。** 固定真实 copy 路由、偏移、字节、屏障及 compare→seal 的依赖，第一笔大 copy 之前有 state/compare 写入与 barrier，单次墙时只能确定调用里等待。对三笔分别保调用墙时、owner thread CPU time、实际 binding、flags；同时采两批 submit/flush、bind/fence、poll enter/return。先在无其它 GPU 客户竞争的同配置对照，另记实际系统竞争和调度。若设备支持可用非阻塞 GPU timestamp 或系统 GPU/调度 trace，带 clock domain、disjoint 与可用状态。缺支持则另建不计 PASS 的 prefix 隔离臂，在 compare 之前、compare 后第一笔大 copy 前和 seal 后分别用独立同队列 sentinel fence，隔离每次实验，禁止把额外等待结果并入真实根时长。确认是队列反压还是搬运之前，不改 copy 范围、不以 CPU 搬运、不删比较副本、不移用上一代的数据。
4. **把不可丢的阶段从热事件数组移出。** 复用 T758/T761 schema 的身份与终态语义，新增固定槽保全部 12 dispatch、3 copy、每批首末 flush、执行 fence 创建、第一/最后/最长 poll、每阶段首拒和废弃时刻。每种 poll 分类计数，另保固定长度近尾 ring；先冻结失败原 snapshot，原 clock 的首拒优先级不能被下一次 `Denied()` 采样覆盖。区分计划唤醒、实际唤醒、进 owner、lock enter/acquired/exit、poll 入口/返回，以及最后真实 pulse 序号/receipt，算实际 owner 间隔。无同步打印或逐 poll IO。采样与溢出计数分开，记录实际被采区间和 wall/thread CPU 的计时来源。真实处理时间不足仍走原撤权与退役。

## 验证和交接

- **developer** 先做有界诊断及两个独立开关，默认保原产品和原研究基线。按同一 K192/S40、同 manifest/SHA、同布局、同设备条件提供 A 原 baseline、B 仅 paced poll、C B 加末批单 flush 的研究对照，构建路径、owner 与 CPU 负载可追溯，研究后端限定于本轮 GLES。不得因修订改变根 claim/nonce、预算起点、队列消费、shader 或原 12 pass/3 copy。如 Vulkan 后续被安排，以相同事务边界和实际队列 fence 另行采证。
- **tester** 做合成时序和独立 reader。覆盖 `deadline-1 / deadline / deadline+1`、正常 pulse 与超窗口、取消/重配置、poll 提前或迟到、旧 wake 不碰新根、null fence、WAIT_FAILED、提交后 flush 过线、迟到 signal 只退休、未映射计数和三组 copy/bound ranges。验证设备原件能分出近尾轮询间隔、真实 CPU 调度时长和 GPU 缺证。T761 reader 按真实 WORK map 才取 1024 B。
- **effector** 用每臂真实已核验的 driver、HAP 剥离 DSO 和 libc++，各进程新身份。每臂先跑一个同输入 SOURCE，诊断齐全再交错复测确认 85.55/77.46 ms 是否重复、A/B 的 CPU 开销、B/C 的全链收益。保终态正确的失败样本，禁止当场试调期限。若 C 仍超期，用同 Proc/GL 时间原边和可用 GPU 时间把 copy/flush/fence 分段后再回 architect。实际通过须是截止内同根执行 signal、map/unmap、typed 及 atomic commit，Q20=209715，commit+1、quanta 不增、FIFO 恰好消费一次、原 barrier 和 cold proof。任何晚 signal 仅允许退役。程序冷编译/链接、SOURCE 通过后持久化及同身份新进程加载单列。

当前结论只含 DM 研究臂，真实有效帧仍需 XComponent+shader 的实际 Swap 和 30 FPS 证据，GPU 驻留、CPU 停投、禁止 Canvas 回退。离线复核 `python tools/architecture/analyze_t768.py`。**G20 约 33%（4/12）**，此次补足 DM 研究 SOURCE 的冷编译与提交时间，产品优化路线的暖加载和首个有效 Swap 仍待取证。 [T741 计分口径](architecture-T741-v1.md#g20-完成度口径)
