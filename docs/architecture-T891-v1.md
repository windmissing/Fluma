# T891 · PC 跨材料 SOURCE 与 D1 动作证据归因

2026-11-23 · architect · P4 · Windows Vulkan · 离线复核

**A 的跨材料入口在第二次 Down 前已经解除武装。** 三轮都先发生共享 grant 到期，旧根真实退役，随后切料状态确实刷新，但没有新的有效 hold。**B 的 owner 漏项发生在首帧后的脉冲会话重置。** 五臂的 drain 有实证，52 条脚本记录尚未闭合全部 owner 终态，两轮 OFF 的实际时域及输入量也已有原值差异。 [原件复算](architecture-T891-evidence-v1.json)

## A · 跨材料实际中断位置

逐条复核 `565d3578` 的三轮，A PE SHA 匹配，源码钉 `d6395752`。所有有效预约均为旧 `hold=1` 的相邻真实脉冲。两笔 SOURCE 在同一 run 中分别用 session 1 和 2 接受，第二笔仍由首次长按触发。两份正序与一份反序合计三轮。第二次 Down 坐标都在实际 784×461 客户区内，第二 Down 的 `hold=0`，window/geometry 无变化，之后没有第二材料的 reservation。 [T887 原件](../test/reports/t887/report.md) · [T882 工件](../test/reports/t882/delivery.md)

| 轮次 | 同一首次 hold 接受量 Q20 | 最后合法根（全轮 queued 数） | 首次失败 / 原同 fence 退役 | 最后撤销 → 失败 → 模式 → 第二 Down（原行） |
| --- | --- | --- | --- | --- |
| `a-forward` | 沙 134,242 + 134,039 → 268,281 | frame 4 / ordinal 2（4） | 业务 wait=2，退役=0，第二 Down 晚于退役 4.282 s | 118 → 119 → 123（水）→124，hold 0 |
| `a-forward2` | 沙 137,194 + 128,932 → 266,126 | frame 4 / ordinal 3（6） | 业务 wait=2，退役=0，第二 Down 晚于退役 5.097 s | 118 → 119 → 123（水）→124，hold 0 |
| `a-reverse` | 水 139,845 + 136,165 → 276,010 | frame 4 / ordinal 3（5） | 业务 wait=2，退役=0，第二 Down 晚于退役 3.258 s | 108 → 109 → 113（沙）→114，hold 0 |

1. 每轮首拒均是 session 2 / plan 4 的 `ROOT_DEADLINE`。250 ms 从第二笔 SOURCE 的 grant 起算，失败 QUANTUM 在首次同 fence wait 进入时仅余 **46.491 / 32.738 / 15.586 ms**。当时三个窗口仍有效，clock 仍开，owner 观察到的最大原 callback/native 间隔均小于 33.333333 ms。`WAIT_PAGE_PULSE` 是原 clock fault 标签残值，实际 `reason` 为 deadline。退役 fence 成功、`fencePending=false`，退役结束分别在 deadline 后 14.693 / 14.364 / 34.047 ms；这是宿主边沿。失败 QUANTUM 的单次 GPU 用时和独显可达性尚无原件。两个重复失败／WSI 边沿同 nonce，只计一个拒绝根。 [原 wait/retire](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [原 QUANTUM 守门](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc)
2. QUANTUM 拒绝时 `disarmM1Input()` 令 `pointerReady=false`、清 held，再经 `Authorize(scope,0)` 给旧预约 `AUTHORITY_CHANGED`；拒绝根只经 tombstone 退役。`advanceQuantum` 只允许关闭时钟后的 `CLOCK_GAP` / `PULSE_QUEUE_OVERFLOW` 恢复。当前 `ROOT_DEADLINE` 被排除，`nextPublication()` 因此终止 WSI 循环。切料发生在旧 offer 撤销和 WSI 退出之后。日志的旧材料只出现在首次 hold 的原件，反序旧材料是水 `1`，并无新 hold 仍使用旧材料的实证。`SelectMaterial` 已在同一 Win32 短锁内更新选材并清 hold/许可，新合法 Down 才会快照新材料。 [Win32 输入与切料](../pc/vulkan/main.cpp) · [预约与撤权](../pc/vulkan/m1_ingress.h)

### A 实施与复采

- **在普通 M1 交互增加明确的同代超期恢复。** 沿已证的 T872 恢复链，锁存拒因与失败 nonce，关旧 hold，核真实 submit 或确定未提交、实际同 fence signal、tombstone 的 owner Poll 退到 Idle、无 in-flight 或冷读 pin 冲突。成功才进入等待新输入。失去设备、退役未闭合、资源或 generation 失配沿原失败/换代处理。首次尚无业务图像用 FAULT_WAIT，已有图像时保实际上一同代呈现为 STALE。若拒的是后续 QUANTUM，GPU 最新已接受 bank 可能包含尚未呈现的 SOURCE，继续从真实接受 bank 开始，单独记录 `lastAcceptedRoot` 和 `lastPresentedRoot`。不能把新提交当作恢复成功。
- **恢复只发新会话和新输入。** 撤销旧 grant 剩余配额及 pending，按 `ProgressClockV2` 对 revoked 和 carry 销账，保已接受 QUANTUM、source/physics commit、run/owner/geometry、bank、来源库存和已接受事件前缀。恢复后用真实新拍建 baseline，再明确发本代 READY，接受晚于 READY 的新 Down。处于等待时可以更新菜单材料，只有新合法 Down 快照材料，已认领旧 offer 仍按原授权与 tombstone 判，不给失败帧或旧 hold 补量、补发事件或延期。原 33,333,333 ns、250 ms、120 Hz、四份配额及零时长 SOURCE 继续沿用。仅普通交互走这条恢复，D1、首根冷档和单次模式保持各自的边界。 [T872 恢复基线](../test/reports/t872/handoff.md) · [时钟与销账](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h)
- **只补切料边沿与可核诊断。** 保 T882 的同材料最新预约和不可变 claim。Win32 owner 入口记录 `MODE_CHANGED` 的实际 QPC/事件序号、旧／新选择及撤回 hold，Down 回执带 inputReady、选择/绑定材料、scope、非法原因。验收保 `GPU_TIMEOUT → OLD_RETIRED → NEW_SESSION/READY → MODE/DOWN → 相邻拍 → SOURCE terminal → 同代 QUANTUM/Present` 的实际 QPC，也单测仍健康时 `Up→切料→新 Down` 及切料中旧 claim 失效，区分恢复和传播。正、反序都要同进程/同 run 同 geometry，第二材料非零接受，随后同一个接受根内两种库存非零且保守恒、原着色点和确切事件。分别测超期时早到 Down 仍无效、晚 fence 不接受、设备失效不 reopen，以及 T882 原同材料正例。用 T852 的真实业务判据，固定 probe 只作信息读数。测试不得延长 grant 或依靠重启成两个场景。 [T880 已证预约](architecture-T880-v1.md) · [T852 双源判据](../test/reports/t852/handoff.md)

## B · D1 投递、owner、实际时域和 drain

B PE 及源码钉 `b683d90a`，复核 T887 五臂严格原 JSON 和 T888 两轮 OFF。每臂有 52 个唯一脚本动作，所有 `DELIVERED` 的 native seq 均非零，两个 SOURCE 都有实际 offer 终态。`AWAIT_PULSE` 是 producer 中间阶段，完整动作需要同 actionId 的真实 SOURCE terminal。`DELIVERED` 是 native 队列回执，另须 owner 对同 scope/session/seq 的唯一终态。原条目的 `traceLoss=0` 只覆盖 writer 丢失，队列清理仍可无痕。 [T888 独立适配](../test/reports/t888/handoff.md) · [T496 原比较](../test/t496/check_pc_d1.py) · [T865 严格门](../test/t865/check.py)

| 运行 | native DELIVERED | owner 接收 / 显式撤销 / 缺终态动作（全部 session 1） | 业务退役与独立归档 |
| --- | ---: | --- | --- |
| OFF1 | 46 | 41 / 1 / **22–25** | 已退役，52 条 producer 行，51–52 在入口已撤权 |
| OFF2 | 48 | 45 / 0 / **15–17** | 已退役，51–52 有实际投递 |
| ON | 46 | 43 / 1 / **24–25** | 已退役，`RESERVED→ENDED` 有热页 |
| REPLAY_OFF | 48 | 45 / 0 / **15–17** | 已退役，9 个同根原档 |
| REPLAY_ON | 37 | 34 / 1 / **16–17** | 已退役，6 个同根原档，`NOT_BUSY` |

**缺项来自首帧后的队列替换。** 五臂的缺项都在 session 1 最后一次已消费拍与首个 `PresentExit` 之间，紧接着 session 2 从 seq=1 开始。OFF1 原行 69/71–74/75/77 依次为最后旧 owner、四条已投递、首个 WSI、首个新 owner，OFF2 原行为 56/58–60/61/63。源码 `nextPublication` 在 cold Present 后执行 `beginClock(true)`，`M1PulseQueue::Begin` 在锁内把旧 `size_` 清零。此路径未为被清掉的旧批次调用 owner observer。`RevokeQueued` 仅在最后 drain 汇报当前 scope，无法找回旧批。生产修订点是 pulse queue 跨会话终态，checker 的 `OWNER_PULSE_COVERAGE` 正确保留。其余 resize、Close、overflow 和故障早退也会走清理相关路径，需覆盖控制用例。 [冷 Present 重锚](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [实际脉冲队列](../pc/vulkan/m1_pulse_queue.h)

**OFF 两轮实际输入不同。** 首个动作相对各自 READY 就为 30,857,400 / 30,246,000 ns。SOURCE 7 取实际 pulse 6→8 的 15,729,600 / 14,961,400 ns，精确得到 `floor(8×Δns×2^20/10^9)=131,949 / 125,505`，差 6,444 Q20，原 command SHA 也各自保留。SOURCE 29 都是 `STALE_ENDPOINT`，实际年龄 44.8592 / 133.6309 ms，另三臂两次 `STALE_ENDPOINT`、一次 REPLAY_ON 真实撤回。原 OFF1 在后段 `LATE_FENCE_SIGNAL`，51–52 入口被撤权，OFF2 有投递。此为在 OFF 之间已出现的变化，原件不足以归因 capture。平均/最大时差只能描述试次抖动，当前时域门与原字 gate 继续留差。 [T888 OFF 原收据](../test/reports/t888/off-pair.json) · [T881 时域与退役](architecture-T881-v1.md)

**退役证据保留。** 五臂 `END_INTAKE→M1_D1_DRAIN→CLOSE` 同代、原 WSI 成功、资源与配额账闭合。OFF1/OFF2/ON/REPLAY_ON 有相应拒绝根的同 fence 退役信号。REPLAY_OFF 普通接受根的第 148 行 `m1QuantumAuthority.lastWait=[15887859244078,15887859323660,2000000,0]`、`fencePending=false`、`m1QuantumOwnerCode=0`，与第 149 行 drain 的同 nonce/fence/等待时标一致，`retireVkResult=1` 是普通成功路径没有运行额外的拒绝后 retire wait。测试接缝应显式区分 `BUSINESS_SIGNALED`、`TOMBSTONE_RETIRED` 和 `COLD_COPY_SIGNALED`，以根和等待原行验证。T888 当前 `d1_drain` 的旧 fence 分支只接受 tombstone 形状，直接用于这个接受根会报 `DRAIN_PRIOR_FENCE`。本卡独立联实际正常 wait，保 15 份重放档在各自同根的范围；不拿退役和归档替代四臂比较。 [T882 B 退役链](../test/reports/t882/delivery.md)

### B 工程修订与最小复采

1. **给每条真实入队拍恰好一个 owner 终态。** 队列在 `Begin/Close` 及故障退出时，在原短锁内带旧 scope 抽取尚未消费的批次，切换到新 scope 时登记 reason 和 QPC。锁外用有界 trace 输出旧 action/session/seq/actual callback/receipt 与 `REVOKED_REANCHOR`、`REVOKED_CLOSED`、`REVOKED_OVERFLOW` 等真实 terminal。owner 已取出的 batch 也要逐项闭合，禁止同一拍二次终态。`Receive` 返回明确 `QUEUED` 或实际未入队原因，只有 queued 发 `DELIVERED`，未入队应保 producer 拒绝、零 owner seq。将已投递且已终结的边界／数量按各 session 写到版本化 drain 覆盖收据，独立核 `queued=accepted+ownerRejected+ownerRevoked`、scope 与唯一键。仍沿原一秒退役清理和真实 fence/WSI 账，区分资源已退役与动作证据未闭合。
2. **先走无冷重锚干扰的 OFF×2。** D1 独立准备实际 Vulkan graphics pipeline/swapchain 及真正 READY，能把首帧冷工作移到动作开始前时再启动原 52 动作，仍由实际 owner 接收原生时标。重锚路径可单独留作用域撤销负控。冻结同一新版 PE、三 SPIR-V、配置、初态、动作表与输入来源。保实际 `declared/producer capture/native receipt/owner consume or terminal/root grant→accept` 原 QPC 和来源，禁止用相对声明 ns 回填物理间隔。严格重跑 OFF×2，先逐一封闭 48 pulse、2 SOURCE、2 capture 动作（动作在入口拒绝也需真原因），退役后比较真实相邻 Δ、原 command 中的实际量、逐 pulse quota/carry/顺序以及实际根／接受范围。T888 原门的实际时域不等仍报首差。只需一对可定位的新原件，不能反复运行挑容差内样本或把 `REVOKED_REANCHOR` 算作 owner 接受。
3. **时域仍不等时交独立等输入诊断。** Windows 同一次真实外部输入采样可封存源参数和成对真实 QPC/接收时序，版本化为只读 `input bytes + clock trace`，供各 arm 重放同一测得的 SOURCE 原量、物理脉冲间隔及配额输入。最好将同一输入捕获点与各 arm 的送达／owner 时标明确区分。该受控诊断仍走真实 GPU、enqueue、owner、提交和退役，实际 freshness、window/session 和 250 ms watchdog 继续用真正墙钟，超龄如实拒绝，记录并对照同一来源 SHA、每根 actual receipt 与时钟账。固定逻辑输入的结果可定位哪些原字仍有差异。若要改变正式 D1 的 authority/归一规则，须单独冻结版本并由独立 checker 核，旧 T865/T888 严格门继续按实际时域解释。两条原时域的因果及实测时标缺口仍独立登记。
4. **放行正式四臂。** 完整 owner 覆盖、原 D1 动作合法终态、同实际输入和时域证据、完成退役均具备后，才做 OFF/ON/REPLAY_OFF/REPLAY_ON 原 SOURCE/Completion、实际接受根顺序以及 bank/event 独立重放的严格比较。ON 忙点没命中保实况，有等待窗口可按同条件补相应热页样本。T496/T850 核各自真实证据，保实际 `SOURCE 29` 过期年龄、首错和 1024 B 热尾，禁止用额外全池读回干预主臂。延期、迟到接受、伪造时标或跳过撤销均不放行。GPU 驻留、shader 渲染，禁止 Canvas 回退。 [T858 D1 证据](architecture-T858-v1.md) · [T703 并行时标](architecture-T703-v1.md)

离线重跑 T888 自测和实际 OFF 对照，严格门复现 `OWNER_PULSE_COVERAGE`。用 T496 reader 重核 15 个重放档与各自同进程原根、事件范围、实际 backing、copy fence/invalidate，均通过。T882 A/B C++、T852 和 T865 离线自测通过。A 的首次归因及 B 的队列和时域修订分开交 developer，tester 与 effector 按上述真实前置接各自离线和 PC 复测。此卡未执行 GPU 或新 PC 产品实验。
