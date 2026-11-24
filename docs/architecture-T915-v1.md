# T915 · G13 OP_48 建管线停滞与 FULL_M1 持按断流首次归因

2026-11-24 · architect · Windows 集显 Vulkan · 离线。G13 停在候选诊断的 `vkCreateComputePipelines(OP_48)`，业务根未开始。相同模块曾在 T890 用 11.1103 ms 返回，也曾在 T901 用 **1226.696 s** 返回。FULL_M1 的恢复路径在第一处 QUANTUM 拒绝即清了应用内部 hold，新时钟恢复了物理但无法继续同一次按压。两项均转 developer，附可证伪对照。 [T912 报告](../test/reports/t912/report.md) · [复算原件索引](architecture-T915-evidence-v1.json)

## 1. G13 · 管线调用边界与已有实证

### 1.1 原件修正

| 原件 | OP_48 窄口证据 | 到达业务 |
|---|---|---|
| T912 T909 `g13-t909-no-tail`，PID 22196 | 行 97→98 ShaderModule 成功，行 99 pipeline ENTER `16477623440105`。没有 EXIT。 | 已过 BOOT 数值 fence 和前八角色的资格。OP_48 未到 OP66，也无 T877 业务 submit。 |
| T912 T909 `g13-baseline-tail`，PID 24032 | 同样止于 99 行，ENTER `16491160388497`。stderr 空。 | `g13-stall-evidence` 与此文件逐字相同，是同一进程原件。 |
| T912 T890 control，PID 31336 | 行 99→100 `16477223731456→16477223842559`，10 MHz 下 **11.1103 ms**，VkResult=0，随后 OP66 实际成功。报告写的 1.036 s 不能作为该调用的时间。 | 到 F02 私有投影根。原 250 ms `GRANT_DEADLINE` 拒绝，晚退役 18/18 点可读，`acceptedRoot=null`。 |
| T901 T899 B `b-hot-slots`，源 `af0f6d9d` | 同 SHA、同样先用 SNAPSHOT ledger、同 flags/cache 指针。行 99→100 `16362510383013→16374777344578`，**1226.6962 s（20.44 分钟）**后返回 0。 | 后续 OP66 和晚退役诊断到件。仍因原业务截止拒绝。 |

T912 四次预算 240/420/400/870 s 来自执行摘要。仓库可独立重算两个不同失败进程，摘要所述另外两次尚无独立 PID/时标与终止原件。四次观测窗均短于 T901 已发生的返回时间，现据应记为“截至监督截止未返回”。故障定位已到 API 调用，尚未出现驱动簇栈样本或实际 VkResult。T901 的老工件能证实同角色此前已有长尾，T909 特异二进制回归和永久挂起尚未成立。 [T912 开查询原件](../test/reports/t912/runs/g13-baseline-tail.jsonl) · [关拆点原件](../test/reports/t912/runs/g13-t909-no-tail.jsonl) · [T890 对照](../test/reports/t912/runs/g13-control-t890.jsonl) · [T901 长尾](../test/reports/t901/b-hot-slots.jsonl)

### 1.2 参数与构建交集

- T890/T909 PE 实物 SHA 分别 `1e49f2a6…671c` / `f742cf6a…c484`。从两个 PE 内实际定位一份完整 `PC_OP_48.spv`，均为 261,304 B，SHA256 `f01959cc0f2fdecff46c953cf6c6073498f53d91bec0ae7d900b35c4c18aceb`。设备行选中同 Intel UHD 630 `8086:3e98`、驱动 `1656924/31.0.101.2140`、family 0。deviceUUID、driverUUID、pipelineCacheUUID、报告 Vulkan loader/instance API、无设备扩展、layout hash `b9390359…cab9` 在可比日志中一致。OP_48 调用 `flags=0,pipelineCache=null`，从冻结代码核同一个 compute stage `main`、无 specialization、无 base pipeline、allocator=null、四 storage binding 加一 uniform、24 B push。设备使用一条 queue、priority 1，`pNext/pEnabledFeatures` 默认空。SDK、MSVC 14.51.36231、CMake Ninja Release 和 `/O2 /Ob2 /DNDEBUG` 本地构建记录一致。PE 大小及嵌入角色集合有变化，不能单凭编译器版本定位到具体对象。layout SHA 是程序记录的合同指纹，完整 runtime create-info 比对仍需原收据。 [管线与设备入口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp) · [T890 构建](../test/reports/t890/delivery.md) · [T909 构建](developer-T909-v1.md)
- 已找到实际混杂变量。T890 源码和 PE 都没有 `--g13-slot-snapshot`，行 79 用 `OP17_PRIVATE_LEDGER`（`cf3d9add…`）。T909 两份失败及 T901 长尾则在同一位置用 `OP17_PRIVATE_LEDGER_SNAPSHOT`（`bdd47fcb…`）。T909 从候选入口启用该旗标时，在 BOOT 时就为 WORK 增加 14×42×4=2352 B，原 descriptor range 取整块实际 buffer；同时改变前置 role 创建和 OP66 提交。两项均发生在 OP_48 之前，OP_48 调用自身的 module/pipeline layout 仍相同。T890 对照因此只能证实同模块和设备有快速正例。T909 的 tail 拆点与 seal 候选位于后续；OP_48 前的已见角色序列在 T909 有无 tail 拆点时相同。在源码中 query pool 也在角色资格后才建。下轮应先把 snapshot 这一组控制住。 [资格顺序及替换角色](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc)
- `VkPipelineCache=null` 只说明应用未传 Vulkan 缓存对象，cache UUID 同值也只识别兼容边界。驱动内部持久缓存、文件锁、线程等待、CPU 工时、实际 loader/ICD/compiler 文件与温冷状态均未封存。T912 已改变窗口显示、stdout 直写和 tail 旗标，原点仍停，足以降低日志排水的嫌疑。进程级采样前保留编译、阻塞与缓存三个分类，避免把缺 EXIT 写成驱动死锁结论。
- 该臂使用 `--g13-candidate`，走 BOOT 数值后直接进入 F02 `PRIVATE_PROJECT`，且没有 WSI 的首个有效业务画面。它阻断 G13 候选配对和首个候选业务根，在本证据中是 P4 诊断/资格问题。若后续在真实产品窗口 trace 证明同一调用位于首个 CURRENT/Present 的必经路径，再单列首帧阻断。停滞发生前未到 815–819 根尾和 seal 差分，T907 的性能结论继续按已提交根适用。 [候选与窗口分支](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp) · [T907 根预算](architecture-T907-g13-v1.md)

### 1.3 最小可证伪 A/B 与接手

1. **同路由 PE 对照。** 冻结实际 UHD 630 和驱动文件、F02 CONFIG/STATE SHA（T912 `295c075e…/e1c6b49e…`）、CWD、等价输出路径、同进程镜像名/目录、日志方式和允许枚举的 Vulkan/驱动环境变量。T890 与 T909 均运行 `--g13-candidate OUT --g13-fixture F02 CFG STATE --g13-diagnostics hot`，两边均关 snapshot/tail/替换 seal，按 ABBA 交错、使用新进程。再只用同一个已冻结 T909 PE 切 snapshot（两边 tail/替换 seal 仍关）。并行报告进程冷启动、重复启动、已证驱动缓存范围。没有可核的独立驱动缓存空间时在报告中标未知。T899 B 可作为已知会进入同前置角色序的桥接工件。一次完成或超时均保原件。
2. **只在分支有差时继续单变量检验。** 若 snapshot 同 PE 分出差异，developer 以同生成输入构建两个独立开关，构成正常/扩容 WORK × 原/快照 ledger 的可执行组合。只有绑定范围合法、含全部实际结果的组合可提交，SDK 和源/模块不随开关重出，OP_48 SHA 不变。若关 snapshot 仍随 PE 分化，则给两个源码快照同一个生成输入和工具链，用当前真实前缀逐提交二分。单角色 `--qualify-steady-role OP_48` 对照完整 F02 前缀；仅诊断允许已有 `--disable-optimization`，此时 flags 差异独立记录。单角色快、完整前缀慢将缩小到角色次序、对象代/分配和缓存。单角色也慢则优先取驱动调用栈、加载身份与 pipeline 包络。
3. **监督与原始输出。** 先以同一短限时取 ENTER/EXIT 或 `CENSORED`。未返回时按预定两个间隔采 PID、进程创建时间、owner TID、CPU 增量及非破坏性线程栈/小转储，记录实际 Vulkan loader/ICD/Intel 编译器路径和版本/SHA、driver cache 的隔离证据及应用缓存是否为空。用已验证的 T854 收据方法，必要时补 stderr、超时监督原值和进程结果。可在另有实际采样价值的臂延长编译观测，仍与 250 ms root deadline 区分。新增诊断只记录指针指向的实际结构字段和 SHA，不跨实验比较裸地址。只有回到两路角色资格均完成才继续 T907 的 815–819、独立完整 replay/守恒和业务截止，两路 F02 只确认私有资格/时标。晚完成仅诊断，真实业务 gate 和 GPU 驻留继续适用。 [既有 T854 栈取证](architecture-T854-v1.md)

开发接 OP48 一致开关与窄日志，tester 独立核参数、样本以及角色原字，effector 复采实际 PC。最小缺证是两次额外停点的独立收据、实际 argv/ICD 镜像和同旗标成对结果。当前离线已复算已有实物，驱动内部的具体执行或等待位置仍未定位。

## 2. FULL_M1 · 物理回到新会话，应用已撤掉按压

### 2.1 唯一根与代码反证

T909 PE `f742cf6a…c484` 在 T912 `2a-sand-observe` 使用 FullM1、单量子 grant、观察 120 帧。run `16449942729366`、geometry 2、QPC 10 MHz。原行 31 只有一次 Down（hold=1，QPC `16449943372318`），原行 510 Up（+8.00048 s）和 511 CAPTURE_LOST。中途无 Up、Move、换材料或新 Down。按同 run/nonce、Completion rootKind 1 和真实 SOURCE fence 对上 **16 份**真实 SOURCE 档（sourceCommit 1–16），质量 **2,064,374 Q20**。31 来自携带同一 SOURCE 结果的中间日志数。最后一个实收 SOURCE 为 `16449969799237`，+2.67452 s，且仍是拒绝当时有效 BASE。修正数量不改变 LAND 与质量结论。 [T912 沙线原件](../test/reports/t912/runs/2a-sand-observe/demo.jsonl) · [M1 同根档 reader](../pc/vulkan/read_t909_observation.py)

| 事件 | 现场关联 |
|---|---|
| 行 202，QUANTUM `16449970250208` 拒绝 | session 1，producer/consumed seq 178/177，callback gap 39.1816 ms、receipt gap 39.1817 ms，超过 33.333333 ms。实际 stop 在 +2.74123 s，真实旧 fence `1422238522408` 退役返回 0，在 +2.77733 s，配额平衡。 |
| 行 203，`M1_READY_PHYSICS` | 新 session 2，真实 baseline seq 1、callback QPC `16449971158055`，在 +2.77857 s。旧物理根保持，随后新 QUANTUM 与 shader 窗口继续。 |
| 从行 204 到 Up | **51** 个不同的成功 QUANTUM（含同根 Present）。后续 50 个有 `m1SourceBoundary` 的帧均 `offer=0`，全部 sourceCommit=16，旧会话实际入料无重复。 |

测试工件对应的 `b4478d3e` 源码在 `RunM1Quantum` 的 reject lambda **先**无条件调用 `attempt.disarmM1Input()`，之后才 `RetireM1Root()`。`Window` 回调在 mutex 内同时做 `pointerReady=false; held={}; scope.permission=0; ingress.Authorize(scope,0)`。`recover()` 入口再次采取同样动作。新 READY 回调只把 `pointerReady` 打开。正常 `MOVE` 只能更新非零 hold，只有正式 Down 可以从零创建 hold，因而恢复后不能单凭仍按住继续 SOURCE。另一个代码反证是 `Window::Pulse()` 在 held 非零且真实 pulse 合法时，无论 `AdjacentSourcePulses` 是否通过，都执行 `sourceAnchor=pulse`。如果真的保住 hold，旧 anchor 最多挡第一次跨 session 配对，下一次同 session 可重新配对。本卡修复重点是分离物理按压和 SOURCE 授权，清 anchor 是必要配套，单独清 anchor 无法解除已清零的 hold。 [QUANTUM 拒绝](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc) · [recover/new baseline](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [Win32 hold/生产脉冲](../pc/vulkan/main.cpp) · [相邻拍与 ingress](../pc/vulkan/m1_ingress.h)

### 2.2 修订合同

1. **第一处就区分临时暂停和真实取消。** 普通 FullM1 已有 accepted BASE 的可恢复 `CLOCK_GAP`，在 QUANTUM reject 的首个调用处关闭 SOURCE 授权及入队，记录暂停原因、原 run/geometry/hold/down/capture epoch 和材料。把 UI 跟踪的物理接触保留为独立状态。recover 入口只能重复相同的暂停动作。用真实 Up、Cancel、越界、capture/focus lost、换源、resize/new geometry、关闭或设备丢失立即清物理状态。无当前已接受 BASE、D1 或不可恢复条件使用原拒绝路径。重新开放只在原同 fence 真退役、owner idle、许可/窗口仍有效、新真实物理 baseline 后发生，沿 T898 的 QUANTUM 自主恢复。
2. **新会话只重新绑定仍有效的真实按压。** 在既有 Window mutex 下重验同 down/capture/geometry/material、坐标在有效客户区、未见 UP/取消和本代 UI capture 证据。需要额外采证时由 UI 线程检查实际捕获 owner，保原时间与结果。Win32 静止持按用当时仍有效的真实坐标即可，避免要求补发 Move。恢复期间如果发生 UP、换源或失捕，允许物理继续，SOURCE 保持禁用。将现有 `held` 样本保为 UI 身份，用独立的 SOURCE 授权 epoch 与新 clock session 约束 ingress、Claim、submit 和 commit。暂停时对旧 waiting/claimed、watermark 与 sourceAnchor 原子撤权，保全部配额回执、旧终态和去重账。重新绑定不复用旧 permit、offer sequence 或旧 offer，迟来的跨线程旧 offer 因 epoch 不符被拒。任何已经接受的 SOURCE 保原 inventory/commit。
3. **真实新拍下的第一对间隔。** 原子登记 rearm 边界，清 anchor 后首个满足同 scope、同新 session 且晚于边界的 producer callback/receipt 只建锚。随后的真实相邻拍且两种间隔均 `0<Δ≤33,333,333 ns`，端点由 owner 实际消费、原 freshness/标签/配额仍通过，才按原 `rate×Δns` 请求。配额和 SOURCE intent 只计新真实区间，任何旧 offer、旧 carry、暂停时长或未被消费的次数不补发。保存 `SUSPEND→RETIRED→NEW_BASELINE→HOLD_REVALIDATED→OFFER→TERMINAL`，逐笔写 hold、授权 epoch、时钟会话、callback/receipt、permit/nonce/实际质量。SOURCE 仍为零物理时长，保持 250 ms 原根期限、原 typed 库存继承与 WSI lease。超期/拒绝前景按上一完整 shader 帧 STALE 或实际故障。
4. **独立证伪。** host 夹具固定 Down→静止持续按，插一次大于 33.333333 ms 的真实时钟间隔。旧实现预期在新 session 有真 QUANTUM 但无 SOURCE，新实现必须观察到旧 offer 撤销、首拍零量、第二合法拍以后同一次 Down 有新 SOURCE，保 Q20 原账、真实 Q 时钟及无重复。加入连续两次 gap、实际 retire pending/late、恢复中 UP、越界、失捕、换材质、跨 geometry、错 epoch/旧 endpoint 延迟投递、无新 baseline、刚参加 pairing 又失效、满池合法 q0、D1 原覆盖，以及观察开/关两臂。PC 用同一程序/窗口脚本、同一物理输入和真实 gap 采根与 QPC，持续到原 8 秒或真实 Up。4 秒见堆仍使用此前独立门，记录新获准量及 WALL/SIM。T912 的无观察 1b 只有约 4 s 且未遇 gap，可作基础正控；同窗观察与否、是否真实触发 gap 的单因素对照仍需采。 [T898 退役与正负控](architecture-T898-v1.md) · [T908 可见门](architecture-T908-v1.md)

developer 实施暂停/重新绑定并交可构建工件，tester 先做真实时钟边界/旧账拒绝负控，effector 核一次 Down 跨 gap 后恢复实际 SOURCE。源状态问题无需等待 G13 编译。全程 GPU 驻留和 shader 呈现，禁止 Canvas 回退。
