# T907 · G13 PC 业务超期归因与裁剪方案 v1

> 2026-11-24 更新：T912 在候选业务根前的 OP_48 建管线长尾按 [T915](architecture-T915-v1.md) 隔离复测，再做本稿要求的配对。
>
> 2026-11-24 更新：T945 复用 T921/T923 的无分化对照。UHD 630 先验证 OP_48 按黏性、压力及其余算子拆分的建管线资格，再独立核 F02 的根成本及业务场证据。 [T945 集显工程合同](architecture-T945-v1.md)

2026-11-24 · architect · 离线复核。T901 B 首拒为 `GRANT_DEADLINE`。同一已提交根的 GPU 时间戳在晚退役后可用。实际求解段 712.151 ms，重放 551.579 ms，`PACK_DONE` 后另有 499.658 ms 待拆解。2 次业务复制的区间约 **13.08 µs**，原报告的 13.08 ms 量纲有误。优先分解完成封印与两个求解 scope，再并行化经量测确认的热点。 [T901 报告](../test/reports/t901/report.md) · [原日志](../test/reports/t901/b-hot-slots.jsonl) · [T899 实现](developer-T899-v1.md)

## 1. 原件与时间分层

- T901 原件提交 `865bd2df`，T899 B 源 `af0f6d9d`，PE SHA256 `d5f0b8bf8b850dc62d6e6992d5474fcee52312f045ad4a7ca63bc0ed5c34d24c`。F02 64×64、`PRIVATE_PROJECT`，Intel UHD 630（`8086:3e98`，驱动 `31.0.101.2140`，原值 `1656924`）。以原件第 176–182 行关联 run/ticket/geometry `1/1/1`、QUANTUM nonce `16375466411550`、device `2625794735208`、queue `2625794906344`、fence `2625794467512`、generation `1`、query pool `2625871014936`。等待期间记录 `FENCE_NOT_SIGNALED`。退役等待仍返回 2，真实 device drain 返回 0，随后同一 fence 确认返回 0，才读取 18 点和槽证据。最终 `businessRejected=true`，`acceptedRoot=null`。
- 写入掩码 `393215` 为 bit 0–16 与 bit 18，只有两枚独立冷复制点缺席。18 点均 `available=true`、`vkResult=0`，按 `((end-start) & ((1<<36)-1))*83.333 ns` 重算。以下是从日志可见 period 算出的值。设备摘要的 root `1770310000 ns` 是有限精度输出。

| Vulkan 标记或覆盖段 | GPU tick 起 → 止 | ms |
|---|---|---:|
| ROOT_BEGIN → UPLOAD_DONE | 65697389489 → 65697389724 | 0.019583 |
| UPLOAD_DONE → ACTUAL_BEGIN | 65697389724 → 65697431804 | 3.506653 |
| ACTUAL_BEGIN → ACTUAL_END | 65697431804 → 65705977646 | 712.150651 |
| ACTUAL_END → REPLAY_BEGIN | 65705977646 → 65705977723 | 0.006417 |
| REPLAY_BEGIN → REPLAY_END | 65705977723 → 65712596692 | 551.578544 |
| REPLAY_END → COMPARE_DONE | 65712596692 → 65712601901 | 0.434082 |
| COMPARE_DONE → PACK_DONE | 65712601901 → 65712637243 | 2.945155 |
| PACK_DONE → BEFORE_TAIL | 65712637243 → 65718633162 | 499.657918 |
| BEFORE_TAIL → BUSINESS_COPIES_DONE | 65718633162 → 65718633319 | 0.013083 |
| **ROOT_BEGIN → BUSINESS_COPIES_DONE** | **65697389489 → 65718633319** | **1770.312085** |
| ACTUAL pressure 首尾 | 65697621925 → 65705976975 | 覆盖区间 |
| ACTUAL viscosity 首尾 | 65697462394 → 65705975641 | 覆盖区间 |
| REPLAY pressure 首尾 | 65706050353 → 65712596126 | 覆盖区间 |
| REPLAY viscosity 首尾 | 65705989255 → 65712595074 | 覆盖区间 |

两类覆盖标记取各自首尾出现位置，区间重叠并跨过预排的后续 J。Vulkan timestamp 反映相应同步作用域的进度，无法从这些区间反推纯 kernel 执行时间或单列 barrier 等待。原逻辑槽 814 为 COMPARE，815 为 FINAL_FIELD_AUDIT（物理拆三段），816 为 TIME_PREPARE，817/818 是 PACK_VIEW/AUDIT，819 是 COMPLETION_SEAL。基线 `PACK_DONE` 记在 818 后，下一枚 `BEFORE_TAIL` 记在 819 和相关 barrier 之后。499.658 ms 的首个排查点是完成封印及其前后同步。 [原绑定](../entry/src/main/cpp/gpu_resident/g13flow/root_binding.h) · [计时录制](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc) · [Vulkan timestamp 作用域](https://docs.vulkan.org/spec/latest/chapters/queries.html#queries-timestamps)

| Host/QPC 证据 | 复算与结论 |
|---|---|
| grant `1637546641154900` 至 deadline `1637546891154900`（ns） | 原截止为 250 ms，包含录制、submit、有效 fence 和接受校验。QPC 为 10 MHz，录制 9.8621 ms，submit 调用 0.0181 ms。 |
| submit 退出 `16375466510590` 至 stop `16375468920754` | 241.0164 ms，16 次 poll 均返回 `VK_TIMEOUT=2`。末次请求等待 2 ms，实际 14.998 ms。stop 越过绝对截止 0.9205 ms，停止原因仍为截止。pulse age 14.6859 ms，最大 callback/receipt gap 均 29.1631 ms，低于现有 33.333333 ms 门。 |
| retire `16375468920754→16375471439885` 与 drain `16375471485825→16375484217206` | 分别 251.9131 ms 和 1273.1381 ms。两者以 host 钟记录，其中等待与 GPU root 区间重合的长度未知。另存 fence 确认和退役资源代。 |

原件后缀含 14 槽×42 字（32 latch + 10 CTRL，LE U32 SHA256 `c5b1ea40c29457ef592be06877d3ebad4784f1d1102ae3ff4f56c07711b20943`）。每槽用 OP17、nonce 两字、scope/J/micro/ordinal、active 原字核对记录位置。实际槽序 `10,70,126,186,242,298,354`，重放 `415,475,531,591,647,703,759`。仅 10 和 415 的 OP17 latch 为 active，分别对应各 scope 的 J1/micro0。其余 OP17 的 NOOP 不能替代其他阶段的执行证据或 J 的业务接受证书。热路径录制 820 逻辑槽、850 派发、850 compute barrier、160 pipeline bind、6 descriptor bind，业务两次复制 3376 B。29 是 T899 前一版 hot 次数。不同 PE 或模式的 T879/T897 原件单列，继续保留以做历史比对。 [T897](../test/reports/t897/handoff.md)

## 2. 成本模型与验证顺序

固定调度为 `2×(1+2+4)×56 + 2×3×4 + 12 = 820`，拆 OP7/OP17 后实际为 850。56 包含 32 个固定 pass、8 个黏性块和 16 个压力 V-cycle；单块最多 16 次 Chebyshev，单 V-cycle 约 `8L+28` 轮内层扫描、串行层间依赖。令 F 为实际 MAC 面规模、N_l 为第 l 层格数。分开统计两个 scope 的有效微步、黏性已执行块 `b≤8`、压力已执行 cycle `v≤16`、实际格/面访问、宽算术以及首证冻结后的空派发。对应主项为每有效微步的 `O(F+N₀) + 16b·O(F) + v·Σ_l c_l·O(N_l)`，加 trial copy/reset、所有逻辑控制与证据写入、外层归约与封印、各屏障及传输。每级 `c_l` 包含平滑、限制/延拓、残差、兼容与归一，粗层迭代更多。现有主要压力/黏性实现每次以单个 64-lane workgroup 跨步扫面或格。小网格也可能串行等待在该组的全局写入与组内同步上，现有原件尚不能分配该成本。 [固定调度](architecture-T607-g13-dispatch-v1.json) · [工程数学](architecture-T607-g13-engineering-v1.md)

| 顺序 | developer 实施与可证伪对照 |
|---|---|
| 1 · 量测与基线 | 同一台 PC、同一 PE/profile 和 F02 CONFIG/STATE 摘要使用 `--g13-fixture F02 CONFIG STATE --g13-diagnostics hot --g13-slot-snapshot` 复核原点。增加可选同队列查询，分清 815/816 与 817/818，以及 819 入、出、最终 transfer barrier 后。再以单独诊断臂在 active J/micro 的黏性块与压力 cycle 两侧记录前后点、真实执行计数及已冻结位。每次只开一个细分窗口，交替开关标记，同热度至少三次，保每次原件和缺点原因。若关闭细点改变时序分布，就按臂分别报告。先辨认 499 ms 稳定落在哪个作用域，并查 completion 的实际 lane/遍历范围。 |
| 2 · 等价裁剪 | 从已测得大头落一项对照。完成段若为串行 scan，改为固定块的局部归约和冻结顺序合并，保字节范围、checksum、错误顺序。空 trial/cycle 占比高才试 GPU 权威 latch 驱动的间接派发或轻量语义槽。保 trial gate/reset、首错、证书、trace/seal 和全部需要的失败前缀。间接参数由同根 GPU 写，具备 shader-write → indirect-read 依赖，仍用一枚根业务 fence。关闭 latch 优化的旧 850 路径作为对照，覆盖 J2/J4、首次通过、拒绝、撤权与 late。无安全的判据时继续原槽。836 候选只少 14 派发，仍须过 T854 资格。 |
| 3 · 并行求解 | 若 active ACTUAL/REPLAY 仍超过业务余量，把最重 stencil/宽归约分为多组 tile、双缓冲、固定子块顺序归并，使用派发间 Vulkan shader 写→读适当依赖同步组间，逐阶段验证恒等式和有效位。先做可单测的一个压力阶段或一个黏性阶段，一步一收据，再进入整根。保 MAC 拓扑、墙法向、Q32 tie/RNE、S128 checked、原有 FP32 运算和固定收敛/冻结次序。若需要更换迭代法、误差预算、舍入或改变实际物理字段，保第一差和成本，转 designer 判算法，独立保留原路径作参考。 |
| 4 · 重返业务资格 | 先用 F02 定位并复算原判据，然后 F03 新鲜 grant 强制 J1、至少 30 个真实合格根，按原步制做细化，并以 F05 查物理动态压力。最后按 T866/T874 对 F01、F06/F07、失败和诊断臂及实际 SOURCE→QUANTUM 对接原资格矩阵。每臂保 GPU/驱动/工件 SHA、输入、逐根实际时长、原场、实际及 replay 前缀、原 fence/owner/lease。实际 CURRENT 帧才参加产品判断。 [业务取证](../test/reports/t866/handoff.md) · [独立判定](../test/reports/t874/handoff.md) |

即使假设 551.579 ms 重放和整个 499.658 ms 尾间隔消失，实测 actual 仍需 712.151 ms。单做复制、调 host 等待或把 250 ms 改长都无法达到当前合同。首轮工程量测需为真实录制、提交、校验留余量，按同根 grant 到接受封存计时。保 250 ms、33,333,333 ns、根的 1/120 s 物理时长、J 选择、完整 actual/replay、同代输入和 accepted token，同根真实收支/散度/压力/黏性证书沿 T721/T631 及各用例较紧界。晚完成只作诊断，继续保上一有效帧的 STALE 身份。T901 的 F02 是私有投影根，目标 14 仍需已接受、可独立复算的真流场和 GridView。GPU 驻留与 shader 呈现，禁止 Canvas 回退。 [T828 PC 合同](architecture-T828-v1.md)

tester 独立解码原字、量纲、首差和用例判据。effector 采集实际 PC 窗口和写入同根证据。新编译单元按展开后的完整 shader 输入逐变体核对现行 49,152 B 限额，报 SPIR-V 大小、实际建管线时间和缓存状态。集显上验证算法及功能，目标独显使用同模块与同场景实测持续性能。本交付完成首归因和实施合同，实机裁剪收益待数据。
