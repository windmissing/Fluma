# T867 · PC M1 双源首个或续帧 QUANTUM 拒绝

2026-11-23 · architect · P3 · Windows PC · 原件复核与工程分流

**T864 五轮都在第一笔材料已接受、首次松手前退出 QUANTUM fence 等待。** 四轮已出现业务画面，一轮首个 QUANTUM 就失败。最后一次短轮询真实返回 `VK_TIMEOUT`，应用随后提前离开等待，距 250 ms 根期限还剩 49.478–229.302 ms。冻结宿主分支和同代事件将问题定界到 QUANTUM 许可/时钟护栏。实际丢失的是哪条 owner pulse、哪个时钟子条件先触发，原日志没有记录。两类材料分别可接受，现场尚未执行到同一场景下第二类材料的 SOURCE 受理。 [T864 原始证据](../test/reports/t864/report.md) · [复算收据](architecture-T867-evidence-v1.json)

## 五轮复算与勘误

工件为载入 T862 双源实现的 T860 paged PE `b02294dc…`，三份运行时 compute SPIR-V 与 T853/T856 收据匹配。双源图形 SPIR-V 长 9,004 B，SHA `d7d73a46…`，字节在实际 PE 内唯一。T862 独立 PE `9fc0985c…` 另存。逐份重读并核 `3090a5bd` 五份原日志和实际终态，源码按 `d32f564c` 的产品边界。取每轮首条失败记录，重复的退出状态只用于核退役。QPC 频率均为 10 MHz，producer pulse、root nonce、fence 样点、deadline 可在同一进程换算。 [T862 交接](../test/reports/t862/handoff.md) · [T860 工件](../test/reports/t860/handoff.md)

| 原件 | 接受 SOURCE（Q20） | 同根 Present 数 | 首拒行/位置 | 根期限剩余 / 前后 producer 最大 pulse 间隔（ms） |
| --- | ---: | ---: | --- | ---: |
| `run1` | 沙 124,585 | 25 | 178 / 续帧 | 121.979 / 31.203 |
| `run2-water` | 水 129,635 | 11 | 116 / 续帧 | 229.302 / 29.519 |
| `run3-sand-then-water` | 沙 134,668 | 7 | 93 / 续帧 | 49.478 / 30.957 |
| `run4-dual` | 沙 135,076 | 0 | 50 / 首个 QUANTUM | 168.258 / 22.601 |
| `run5-dual` | 沙 126,391 | 1 | 62 / 续帧 | 186.308 / 26.569 |

T864 汇总称 `run2/run3` 正常结束，原件都有 `M1_QUANTUM_CONTINUATION_FAILED`。`run5` 在一帧成功后失败。`run4` 尚未进入 WSI。五轮首拒全部早于首次 Up，失败后仍有同 `geometry=2` 的 Win32 输入。`run2` 的模式选择发生在首个 Down 前，其余首笔受理之后出现的键切换都晚于失败。`run3` 名为 sand-then-water，原日志没有材料切换。切换、待处理 offer 撤销和重新按下有输入侧证据。`M1_OFFER_RETURN` 只是入口回执，`M1_OFFER_TERMINAL(result=2, root≠0)` 和同根 Completion 才证明接受。每轮只见一笔，任何续帧的同场景 `sandStockQ` 与 `waterStockQ` 都未同时大于零。 [五轮哈希、行号与终态](architecture-T867-evidence-v1.json)

## 许可与 fence 判别

1. `RunM1Quantum` 在实际 submit 前检查 `current()` 和根 deadline。提交后在 `now<deadline && current()` 成立时才调用 `vkWaitForFences`，每次实参为 `min(deadline-now,2,000,000)` ns。末次返回 `VK_TIMEOUT` 且记录的 `now<deadline`，随后提前退出，说明下一轮许可检查失败。该码只说明本次短等待未取到 signal。最终 GPU 是否会在原根期限内完成这五次失败根没有实测；`run2/3/5` 后续只证明真实窗口退役返回 0。应分别保存实际每次 `VkResult`、许可退出原因和退役收据。 [实际等待循环](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc#L228) · [Vulkan 等待语义](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForFences.html)
2. `attempt.current()` 只有 `!closing && generation==previous`。五轮随后仍在同代记录 Up，排除该范围内更代或关闭导致首拒。在同一 owner 的这次等待中，`Claim`、session 重建、`AcceptQuantum` 均没有执行。前置认可的 plan/session 在该等待期间只会被 `refresh()` 内 `ProgressClockV2::Pulse/Fresh` 的撤销失效。材料切换与 Up 清的是 SOURCE hold/ingress 权，QUANTUM 使用已有 plan 和窗口代。`run4` 失败边上的 `m1PulseFault:null` 来自之前 SOURCE 的成功 fence 收据，QUANTUM 分支未记录自己的 clock fault。 [Win32 scope](../pc/vulkan/main.cpp) · [刷新与接受](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L230) · [原时钟护栏](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h)
3. 原档在失败根 nonce 至末次 fence 样点前后，第一 hold 的 producer offer 序号连续，五轮 producer pulse 最大间隔都小于 33.333333 ms，最后样点前 4.584–14.653 ms 还有真实 producer pulse。它们只能说明生产者按时发出。owner 目前从单个 `atomic latestPulse` 取样，同时把本次 owner 观察时间当 `ProgressClockV2::Pulse` 的 native 接收时刻。中间脉冲可以折叠，GPU/WSI/API 及同步输出也能拉开观察间隔。离线用原 `progress_clock_v2.h` 复现，producer 每 16 ms 给一拍，latest-only 在 owner 35 ms 才观察新值时产生 `CLOCK_GAP`。当时 grant 还剩 216 ms。将同样一串真实接收原时标按序交给时钟后许可保持有效。真实缺拍 36 ms 仍撤权，三个对照配额都平衡。这是已复现的代码机制，现场原档不足以唯一选出 `maxNativeGap`、`maxUiGap`、`Fresh` 判 stale 或时钟反序分支。 [离线时钟负控与生产者时序](architecture-T867-evidence-v1.json)

一次续帧失败后，`nextPublication` 发 `M1_QUANTUM_CONTINUATION_FAILED` 并停在 `while(attempt.current()) Sleep(16)`。该代之后仍可收到键/按下和 enqueue，owner 已停止消费。首个 QUANTUM 失败则在调用 WSI 前提前返回。实际链路的中断位置已在首拒阶段锁定，两种路径都应保留已接受前缀并给出确定的退役/恢复状态。T856/T863 的首根单呈现或冷档用于身份正控。它们的 PE、运行模式和观察窗口与 T864 不同，不能证明持续单源在相同条件下成功，也不能从五轮结果归到水算法或双相负载。 [续帧处理](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L474) · [T856 首帧](../test/reports/t856/report.md) · [T863 冷档](../test/reports/t863/report.md)

## Developer 修订合同

1. **保有界、可区分的原始失败边沿。** SOURCE 与 QUANTUM 各记实际 scope、root、grant/session/id、grant/deadline、`queueSubmit`、等待次数及请求 timeout、最后一轮 wait ENTER/EXIT QPC 与真实 `VkResult`。按真实发生顺序锁存第一个退出原因，记录窗口是否当前、plan/session 是否匹配、`clock.open/ready`、`clock.fault`、当前 producer seq/tick 和最近已接受的 seq/UI/native receipt、`maxNativeGap/maxUiGap`、pending/active/carry、最后接受根。把短轮询 `VK_TIMEOUT`、整根期限、clock/窗口撤权、晚 signal、尾拒绝和异常 VkResult 分开。`DEVICE_LOST` 保失效与资源终态。持续 trace 在热路用定容项，移出 UI 定时回调和 owner 临界区的逐行 `std::endl`/stdout 等待，将日志写入移到有界 writer。溢出留缺口标记。
2. **分清生产、实际接收和 owner 消费。** 以 Win32 同一会话的真实脉冲序号和 callback QPC 为输入，在有界 PC native 接收点登记实际收达 QPC，交 owner 按序消费。参考共享 `PagePulseLocked` 已有的“回调时间与原生接收时间同时传入”语义。PC 的 pulse 接收通道独立于 4 槽 SOURCE offer、鼠标按住和材料选择，绑定 run/window/geometry/session。owner 按序看到每个仍有效的 pulse，`now` 单独用于 `Fresh`、前后 accept 复核和 root deadline。记录不完整、顺序错误、真实 >33,333,333 ns gap 或新的 scope 立即按原失效规则处理。冷 Present 或重新计时后只采实际新 baseline，旧 pulse/已撤销计划不得补发或回填走过的时间。不能在等待 fence 的路径造 pulse，也不能靠扩大 33.333 ms 或 250 ms 门限换取通过。原 120 Hz 物理比例、四份配额上限、原子接受、GPU 重放和 1,024 B 热尾保持。 [共享接收点](../entry/src/main/cpp/gpu_resident/app_bridge.cpp#L87) · [原 PC 定时/trace](../pc/vulkan/main.cpp) · [Windows 定时消息语义](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-timer)
3. **提交过的拒绝根先隔离和退役，再受理新的会话。** 从 `root.Stop()` 保 tombstone，经实际旧 fence / 有效失去设备证据观察退役。未提交的命令取消，已提交结果晚到仅可用于诊断。队列、descriptor、bank、event arena、publication 均须在有效退役前保旧代。窗口仍有效、资源仍健康且真实时钟故障允许恢复时，终态回执→旧根退役→真实新 pulse baseline 和新按下需有独立顺序。只有已接受的库存和场景根继续留在同 owner/geometry，旧计划/旧 hold 已废弃。无有效上一帧发明确 FAULT，有同代上一完整 shader 帧可作为 STALE。失去设备或换窗口则沿原新代创建流程。终结分支同时关待消费入口，活跃状态与 offer 回执一致，避免只有 UI 持续入队。此次修订覆盖首个 QUANTUM 和续帧两个失败出口。 [原 owner 停止与释放](../entry/src/main/cpp/gpu_resident/m1/source_owner.cpp#L59) · [T858 命令与退役](architecture-T858-v1.md)

## 同一场景复测与交接

- **先收许可/时钟对照。** 固定 PE、三 compute SPIR-V、机器、驱动、窗体、材料和真实交互节奏，在 T864 工件上比较 trace OFF/ON，分别取首根和续帧。新版重跑一次真实 producer 缺拍和一次 owner 延迟观察但 producer 连续的对照，留真实 wait 粒度和第一个退出原因。测试按同样 33.333 ms/250 ms 判，提交后撤销动作须有同代 fence、已接受前缀不变和拒绝发布。只有实际收据能够判断观察延迟究竟来自 API、日志或调度。 [T860 离线设施](../test/t860/ingress.cpp) · [T865 D1/D2 夹具](../test/reports/t865/handoff.md)
- **再取跨源正控。** 同一 PE、run、owner、device、config 与 geometry 内，从空场景收首种非零 SOURCE 和相应业务帧。Up→键切换→重新 Down，确认新 hold、带材料的实际 SOURCE command32/Completion256、第二种材料非零接受，以及之后一帧的沙/水库存均非零。反序同样取证。另在 hold 尚未结束时切换，旧 offer 拒绝、新 hold 独立；已授权 QUANTUM 按原窗口/时钟继续，不重复源账。保守恒、零时长 SOURCE、四段身份、实际 lease 和同根 shader Present，另取沙落床、水飞包/到达及有明确空间坐标的像素。固定信息像素和两个单源 run 只沿各自证据域。T852 四个合成 fixture、15 项负控已离线重跑，可直接接真实原件。 [T852 正反源序](../test/reports/t852/handoff.md) · [目标 7/8 物理及画面](architecture-T832-v1.md)

Developer 按上述宿主接缝实施和编译。Tester 复用原句柄、时钟和两相判据，effector 在 Windows PC 取真实运行原件。此卡完成首次归因和复测合同，现有沙、水分别接受继续入账。同场景双源、水扩散和目标环境性能分别等各自真实结果。全程 GPU 驻留、shader 渲染，禁止 Canvas 回退。
