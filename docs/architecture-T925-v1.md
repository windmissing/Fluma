# T925 · PC Demo 后段降速与停止出沙首次归因

2026-11-24 · architect · Windows Vulkan · 离线。用户本次运行的是 **T899 A 普通档**。随活沙增加，实际 QUANTUM 推进从每秒 17–20 根降到不足 1 根，后段曾连续 37.35 秒没有新接受根。最终的一次 `CLOCK_GAP` 发生在已提交的 QUANTUM 上，同一 fence 的退役等待超时，旧逻辑退出续根回调并取消入料许可。最后有效根仍有 **71 / 192 个空沙槽**。 [复算原件](architecture-T925-evidence-v1.json)

## 1. 冻结工件与计量

从用户原始 `.codex/local-test/demo-7b46b51ba59b497a8ac4447f1328229b/demo.jsonl` 冻结前 6,539 行完整字节，共 14,696,965 B，SHA256 `cbc3659f1f4251f19857ddf4d066a35cc9bf42fde0b7232d3ca33d9051e594c3`。本机副本 `.codex/local-test/t925-frozen/demo.jsonl`。运行 PID 87816、创建时刻 14:09:15、owner TID 85120、run `16603889196966`、geometry 2、QPC 10 MHz。用原 PID 的 Win32 进程路径和命令行核为 `t899-a-release.exe`，其 SHA `c09e87fa…da79` 与 T899 A 发件一致，核对三个实际模块 SHA `1912af9e…7162 / b8f0e327…c85c / caf0614c…06ce`。参数为 `--m1-empty-preview --m1-manual-gravity --m1-one-quantum-grant --m1-dual-source`，窗口 784×461。GPU 是 Intel UHD 630 `8086:3e98`，驱动 31.0.101.2140（Vulkan 原值 1656924）。 [原 T899 发件](developer-T899-v1.md) · [普通启动脚本](../tools/start-pc-demo.ps1)

以首个实际 Down 的 QPC `16603947930267` 为 0。SOURCE/Q 分别按 run、rootKind、nonce、成功同根 fence、Completion256、owner 和 acceptedRoot 联接。QUANTUM 另核 physicsCommit/sourceCommit、slot/generation 和同根 WSI 入队。reject 按 `businessStopQpc`，成功按 fence 的宿主观察时标，窗为半开区间。SOURCE 与下一根 Q 常共一条日志。汇总共有 **1,317 个 Q**，量子序号连续，全部有同根 WSI 入队。**218 笔实收 SOURCE** 累加 28,262,477 Q20（26.9532 模型单位），每个已接受 Q 的沙库存与对应 sourceCommit 的实收累计一致，水库存为零。实际 Q 累计消费 10.975 秒物理时间。活沙为零的接受根可将全部沙库存记为床。混态根有总库存、活槽和空槽，缺逐列 S/P、静床质量与逐笔 LAND 原件。 [旧版 Completion 字典](../entry/src/main/cpp/gpu_resident/m1/layout_generated.h) · [T908 相同计量口径](architecture-T908-v1.md)

## 2. 后段降速先于终止出沙

| 首 Down 后，秒 | 接受 Q/s | 仿真秒/墙钟秒 | 实收 SOURCE 笔／模型单位 | 沙总库存，模型单位 | 活沙中位／峰值 | compute submit 返回→fence，p50/p95 ms |
|---|---:|---:|---:|---:|---:|---:|
| 0–20 | 17.55 | 0.1463 | 25 / 3.234 | 0.132→3.234 | 6/24 | 49.88/80.36 |
| 20–40 | 20.40 | 0.1700 | 25 / 3.138 | 3.234→6.245 | 1/32 | 39.27/75.68 |
| 40–60 | 7.85 | 0.0654 | 70 / 8.664 | 6.372→14.800 | 42/61 | 114.75/146.49 |
| 80–100 | 4.25 | 0.0354 | 16 / 2.048 | 22.827→24.875 | 94/98 | 208.12/213.30 |
| 120–140 | 3.15 | 0.0263 | 8 / 0.843 | 25.747→26.590 | 115/118 | 242.11/248.27 |
| 140–160 | 2.10 | 0.0175 | 2 / 0.243 | 26.590→26.833 | 119/120 | 247.68/249.31 |
| 160–180 | 1.30 | 0.0108 | 0 / 0 | 26.833 恒定 | 120/120 | 248.97/249.67 |
| 180–200 | 0 | 0 | 0 / 0 | 无新接受根，沿最后已接受库存 | 无新值 | — |
| 200–220 | 0.20 | 0.0017 | 1 / 0.120 | 26.833→26.953 | 121/121 | 249.47/249.65 |

- 活沙 0、32–63、64–95、96–127 的同一运行 host fence 区间中位为 **36.95、123.58、206.64、239.92 ms**。每次固定 127 个派发，宿主 record 中位约 0.19–0.22 ms，graphics submit→fence 中位从约 0.72–1.01 增至 6.65 ms。后段 120 活沙、无新增库存仍贴近截止。相同 PE 的 T908 原件另有沙床库存约 25–38 单位而活包为零时、Q 恢复到每秒 21–26 的正对照。优先检查有效活动量和各阶段放大。用户原件缺设备 query，host 区间包含排队、GPU 执行、同步和 fence 观察。无法从这些数分出单个 shader 的耗时，也无法排除同设备其他进程负载或温态影响。
- 稳态恶化共有 **149 ROOT_DEADLINE、65 LATE_FENCE_SIGNAL、1 ACCEPT_DEADLINE、7 已提交 CLOCK_GAP** 的独立可恢复拒绝根，另有 1 次没有本次根 nonce 的 CLOCK_GAP 恢复。末次故障是第 8 个已提交 CLOCK_GAP，两个打印行只计一次。149 次 ROOT_DEADLINE 中有 **127 次**首等预算至少 245 ms，保单量子 grant 后仍超期。上一有效 Q 为 218.262788 s。
- 最后一次实收 SOURCE 在 **211.434352 s**、原行 6198，hold 4408、commit 218，实收 125,561 Q20。之后仍有 3 个接受 Q。冻结窗内所有 218 笔 SOURCE 实收不为零，准入码与拒绝量为 0。另 9 笔已提交 SOURCE 是 `SOURCE_HOLD_REVOKED`，均在 134.468 s 前，已经退役。到最后一次接受 Q 时活沙峰值 121、空槽 71，终止前最近的实际 SOURCE 也有 71 个空槽。没有到全局槽上限的收据。局部列、阶段容量和未观察的投递仍需实际判据。
- 160 s 到最后一次实收前，有 5 次 Down 获得 hold，26 次有权尝试处看见的 SOURCE boundary 全为 `offer=0`。最后一次实收至终止期间另 11 次 Down 获得 hold，3 次被 `NOT_READY` 拒绝，未出现新的 SOURCE 提交。旧 PE 只有 owner 实际轮到该分支时的 `takeFresh` 采样，缺生产端预约全账。持续降速期间可确认的是 **可用入料机会和实收同步下降**，生产端未产生、过期或撤销要用携带授权代的预约收据进一步区分。

## 3. 终止根和恢复门

| 原行 | 同 run 的实际边沿 |
|---|---|
| 6388 | session 239，nonce `16606128063635`，最后真正接受并排入 WSI 的 QUANTUM。 |
| 6396–6408 | session 239、240 各一次 LATE，session 241 一次 DEADLINE。各自拒绝、同 fence 已退役、新真实 baseline。 |
| 6410–6411 | session 242、plan 1540 的 CLOCK_GAP 先停业务，随后独立 retire 返回 0、新 baseline 243。 |
| 6412–6413 | 同一个 session 243、plan 1541、nonce `16606142484360`、fence `1787706299240`。submit=0，第一次 wait 预算 249.696 ms。真实相邻 callback 约 38.6804 ms，超过 33.333333 ms 门。业务以 CLOCK_GAP 在 grant 后 35.044 ms 拒绝，业务 wait=2。进入独立同 fence 等候，258.130 ms 后仍返回 `VK_TIMEOUT=2`、`fencePending=true`。第二行额外有 `m1PublicationReleased=true`、WSI retire=0。物理 fence 仍缺成功结果。 |
| 6414–6539 | 同一窗口继续收到输入。5 次位于原客户区内的 Down 全为 `hold=0, inputReady=false, reject=NOT_READY`，第一笔在失败后约 76 秒。没有真实新 READY、SOURCE、Q 或新物理图。 |

按 T899 A 源 `1bc13152`，`RunM1Quantum` 拒绝先关输入、`RetireM1Root` 对同 fence 等一轮，超时让 `inFlight` 留真。物理 owner 仍有拒绝墓碑，`CanRearmM1` 要求 `Idle && !inFlight && !pinned`，因此 `advanceQuantum` 返回失败。发布回调先打印失败，随后 WSI 释放旧 published lease 并再返回同一个失败。UI 的 `disarmM1Input` 清 `pointerReady/held/permission`，只有真正开放新会话的 READY 才允许后续 Down。析构另有 `DeviceWaitIdle` 清理，但该 M1 末根没有可用的同 fence 晚完成收据，析构也没有重新建立入料会话。源代码与现场一致，已定位终止阻断在 **拒绝后的物理退役等待与自动续根**。末根实际最终何时信号、是否有另一设备状态变化，旧日志无从确认。 [当前退役入口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [恢复谓词](../pc/vulkan/m1_recovery.h) · [T915 旧按压丢失分支](architecture-T915-v1.md)

T916 已实测 **成功退役** 的 CLOCK_GAP 后在同一次 Down 恢复，T918 紧凑版继承 T916。两版中 `RetireM1Root` 对超时仍仅做该轮等待，`CanRearmM1` 仍拒绝 pending 的旧根。T920 短窗用独立 T918 PE `f5aff05d…622d`、624×431，4 秒实收 9.73 单位、2.43 单位/秒，约为 2.50 必要吞吐的 97%。报告在更晚的取样根见约 4.7 px，2–4 秒和 6 px 的原门仍缺通过证据。已采 post-UP gap 与成功退役的恢复，与 T925 的终末超时分列。 [T916 修复和界限](developer-T916-v1.md) · [紧凑版报告](../test/reports/t920/report.md)

T920 的新增长程对照使用 **T909 r2 普通档**（PE `d37e45b9…a124`、`t830-m1-bounded`、同样单量子 grant，12 次持按后还有 6 次末段试探）。原件 7,323 行 SHA `51c3a2a6…a1cd`，195.215 s 内 987 个接受 Q、210 笔 SOURCE，实收 25.5741 模型单位。末段六次仍各有 **36 / 23 / 5 / 6 / 2 / 1** 笔 SOURCE。11 次 DEADLINE、10 次 LATE、1 次 CLOCK_GAP 的物理 fence 均成功退役，原件以实际退出并释放 WSI 收尾。活沙峰值 95，现场故障前为 120–121。本臂证实较高库存和长时重复下仍可入料，以及末段实收下降。更高活动量、首次退役超时后回联、紧凑档长程与逐列/像素同根仍要实测。 [启动收据及原件复算](architecture-T925-evidence-v1.json)

## 4. 工程修订及复测

### A. 退役等待和资格恢复先独立修

1. **Developer** 将普通交互的物理拒绝拆为“业务拒绝已终结”“物理退役待确认”“同代完成恢复”三段。第一段立即撤旧入料许可、取消旧 grant 和未消费额度。保最后真实 accepted bank/CURRENT 和 WSI lease，记录两者 nonce、版本和范围，UI 显示退役中或具名故障。保独立持按身份及真实 Up/Cancel、失捕、焦点、换料、geometry 换代。原 250 ms 与真实时钟 freshness 保持。拒绝 candidate 永远无 publish 权。
2. 第一次退役 `VK_TIMEOUT` 时让同一 owner 持有原 submit 的 device/queue/fence/generation、墓碑和资源租约，给 UI 事件让路。通过现有 owner 循环或独立监督线程通知 owner，按有界批次等待同一 fence 并记原 VkResult/QPC。下一次 `VK_SUCCESS` 后先确认原代，完成 tombstone Poll，核 owner Idle、无冷读/图形 pin 冲突，才在有效场景下建立新真实 baseline。遇到设备丢失、对象代失配、窗口撤销或超过产品诊断预算，进入明确故障/重启路径并让资源继续由原 owner 安全收尾。晚退役只推进资源账。
3. 新会话复用 T916 对仍有效捕获和实际 Down 的重新核验，以及首拍建锚、后续相邻真实拍重新入料。超期和旧 offer 原 terminal 要逐一闭合。UP、失捕或换料期间待退役的负控可重启物理，但须等新合法 Down 入料。不延长旧期限或销掉配额亏账。前台 READY 与来源能力使用真实状态，按原键记录 `BUSINESS_REJECT → RETIRE_PENDING → TOMBSTONE_RETIRED → NEW_BASELINE → HOLD_REVALIDATED / WAIT_NEW_DOWN`。每根输出一次物理终局，WSI 可有独立收据。
4. 最小可证伪对照复用 T916 的 Win32 输入与 T917 的独立判据。先在 host 检 validator 的超时→同一 fence 成功、错 fence/错代、设备丢失、持续超时、等待中 Up/失捕等，禁止提交后提前重绑定或拿 WSI 退役抵物理退役。实际 PC 再复用同一 PE/模块和同一 BASE 做“即时退役”“首次超时、随后真实退役”对照。若只采到正常恢复，把 pending 窗记为未触发。采到同源真实迟退役后，新 Q 必须以旧 accepted BASE 继续。仍有效按压或新合法 Down 必须取到非零 SOURCE，保全账与实际同根着色输出。

### B. 把高负载和画面耗时从恢复判据中拆开

1. **Developer** 先在同一模块和 seed 里补早/中/末 Q 的 GPU 段时标与实际工作计数。对照纯 Q/同 grant 的 SOURCE+Q、无 WSI 的诊断/同代真实着色、旧库存与 inventory-v1。按活沙 0/32/64/96/120、固定重床零活沙及同库存不同活比例配对，同时固定姿态、候选/独立 replay 及事件设备范围。量库存和角域、排序、运动、床/水、完整 replay、审计/pack、graphics。分别记录 host 录制、GPU tick、WSI、排队、实际 grant、退役和事件短缺。用成功与迟退役时标覆盖截断偏差。失败根只用于诊断。保存 GPU/驱动/缓存、实际工件与进程负载范围。
2. 先上有等价证据的活索引/每列归约与排序优化，沿 T908 尽量用一次池扫描生成稳定索引，将库存固定扫成本从 `O(N·K+A·K)` 降到 `O(N+K+A)` 加确定排序，保完整零区合法、逐字首错、事件序、原守恒与独立 ACTUAL/REPLAY。复算等价后，再处理随真实冲突链和事件数放大的运动/独立 replay，最后验证显示的同代粒包分桶和 shader 绘制。真实时间账固定每 Q 为 1/120 s，Q 与 30 fps 呈现节奏分开计。用 ring/credit 限制在飞，不补墙钟丢失配额。接口由现有 owner 及 GPU 发布合同维持。开发集显实测正确性和诊断，用户目标独显做实际持续性能。
3. **Effector** 按 T920 已追加范围，旧 T899、本轮修复候选、T918 紧凑对照分别记实际命令行和模块。首次见堆后再连续运行至少 60 秒并反复持按。固定各段 Down、恢复输入、喷嘴和材料，量头/中/末 20 秒的实际 Q/s、实收 SOURCE、活槽/空槽、同根 S/P+ROI、每类拒因和闭合链。达到预设高负载后取空槽、零 SOURCE 及合法容量负控组。末段若停止，记录 UI 意图→授权→合规相邻拍→offer→claim→GPU→接受→呈现的首个缺边。只用实际同根画面和物理账判定。测试器补少量 READY/续根与无迟到接受负控，可与长程采证并行。

**分流。** 确认的终止问题交 developer 修同 fence 异步退役及显式入料状态。阶段热点按工程计量和等价优化接 T908。真实接纳仍失败时，按首个缺边分配到授权、Vulkan、typed gate 或场景测试，并保配对工件、独立 replay 与守恒。确认同输入和原任务边界后若仍出现运动或守恒反例，再交 designer。保全 GPU 驻留与 shader 渲染，禁止 Canvas 回退。
