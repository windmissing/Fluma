# T951 · F02 迟到根与 G16 微步预算归因

2026-11-24 · architect · 离线复核 Intel UHD 630 `8086:3e98`，驱动 `31.0.101.2140`（`1656924`）。两条路径分别处理。原 250 ms 绝对期限、计算和发布的同代身份、GPU 驻留、shader 渲染继续生效，禁止 Canvas 回退。 [T949 测试报告](../test/reports/t949/report.md) · [本轮证据索引](architecture-T951-evidence-v1.json)

## 1. 目标 13 · F02 已走到哪个阶段

T949 的 `f02-split.jsonl` 第 186–192 行是同一 F02 私有 QUANTUM，nonce `16834270194456`，Vulkan device `1559528016328`、queue `1559528197112`、fence `1559527445048` 第 1 代、query pool `1559752882296`。实发 820 逻辑槽、850 dispatch/compute barrier。业务 submit 为 0，16 次业务等待均为 2，首停 `GRANT_DEADLINE`。独立退役等待也返回 2，device drain 后原 fence 确认返回 0。随后从原查询池取得 34 个可用点，14 槽晚读与原 nonce/代一致。最后可定位的 GPU 点为 `BUSINESS_COPIES_DONE`。815 最终场审计、818 打包审计和 819 封存的区间均已走过。GPU 原时标证明执行进度，字段原字、审计结果和业务接受另需各自的证据。报告里的“818 未到达”只适用于业务路径未能获取审计结果，迟到 GPU 路径已经越过该点。 [当轮日志](../test/reports/t949/runs/goal13/f02-split.jsonl:186) · [退役与读回实现](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:650)

**闭包资格也需更正。** 34 条 `STEADY_PIPELINE` 为 17 个角色的两条进度记录。实建、实等 fence、双轮 OP66 零 mismatch 的新闭包是黏性 `812db31f…`（约 551 ms 建管线）和压力 `ca603a59…`（约 7.515 s）。F02 QUANTUM 只需旧组里的 opcode 48/49，当前工件里的 `OP48_REMAINDER` 模块 `64101778…` 只核到了原件摘要，未见该臂实际建管线或 OP66。T949 调用的 standalone 资格入口拒绝新角色名，旧 OP_48 的调用未返回可用于编译归因。若后续路径实际需要余闭包，复用 split 构建/OP66 探针，给指定角色开放资格入口或用真实消费其 opcode 的冻结计划。在该角色有实建收据前保留单独状态。 [拆分角色选择](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:189) · [编译资格汇总](../test/reports/t949/runs/goal13/goal13-result.json)

### 1.1 同一原夹具的历史对照

T901/T949 均为 Intel UHD 630、64×64 F02 `PRIVATE_PROJECT`，log 内 Config 字段 `6e7b33e6…` 是 Config128 内嵌摘要，文件 SHA 为 `295c075e…`，STATE `e1c6b49e…`。两次都只在 ACTUAL/REPLAY 的 J1/micro0 槽 10/415 看见 OP17 active；其余十二个 OP17 槽无实际入口，其他算子的执行次数仍以各自 trace 为准。T901 和 T949 使用不同 PE、角色和时标点位，现有各一个样本可对照量级，不能分离拆分本身的净影响。 [T901 原日志](../test/reports/t901/b-hot-slots.jsonl:176) · [T907](architecture-T907-g13-v1.md) · [T945](architecture-T945-v1.md)

| Host 10 MHz QPC，ms | T901 | T949 split |
| --- | ---: | ---: |
| 录制 / grant 到 submit 返回 | 9.862 / 9.904 | 1.340 / 1.399 |
| submit 返回到业务停 / grant 到业务停 | 241.016 / 250.921 | 249.507 / 250.906 |
| 末次业务等待，申请 2 ms | 14.998 | 16.216 |
| 独立退役等待 / device drain | 251.913 / 1273.138 | 263.137 / 1389.325 |
| grant 到原 fence 晚确认返回 | 1780.646 | 1905.325 |

| GPU 时间戳，ms | T901 | T949 split |
| --- | ---: | ---: |
| ACTUAL | 712.151 | 742.381 |
| REPLAY | 551.579 | 602.177 |
| REPLAY 结束到 COMPARE_DONE | 0.434 | 0.453 |
| COMPARE_DONE 到 PACK_DONE | 2.945 | 4.957 |
| PACK_DONE 到 BEFORE_TAIL | 499.658 | 549.214 |
| 819 SEAL_BEGIN 到 SEAL_END | 无细点 | 549.210 |
| BEFORE_TAIL 到 BUSINESS_COPIES_DONE | 0.013 | 0.026 |
| **ROOT_BEGIN 到 BUSINESS_COPIES_DONE** | **1770.312** | **1902.949** |

T949 的 815 最终场审计约 0.438 ms，816 时间准备约 0.020 ms，817 视图约 3.213 ms，818 打包审计约 1.286 ms。J1/micro0 首个压力 cycle 的 ACTUAL/REPLAY 区间分别约 105.923/54.996 ms，尚缺完整 cycles、收敛位和各 scope 的实际执行账。首个黏性 block 区间为 0.017/0.010 ms。F02 的 permit 选择 `PRIVATE_PROJECT`（`c[762]=1`），shader 明确将 opcode 48 做诊断空体，尽管 Config 的 ν 为 1/64，这两点也只覆盖空体调度。测值按 36 位回卷和日志 period 83.333 ns 重算，包含相应 Vulkan 时间戳作用域与屏障的进度。各 scope 的 pressure/viscosity 首尾覆盖包含后续预排槽，区间重叠。ACTUAL、REPLAY、封存三个区间约占整根 39.0%、31.6%、28.9%。即使封存完全省去，按单样本量级其余仍约 1354 ms，当前差距需要同时改善多个执行段。 [同 fence 34 点](../test/reports/t949/runs/goal13/f02-split.jsonl:190) · [私有路由](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:184) · [空体分支](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp:134)

### 1.2 首个整体增量与补证

1. **先跑已有可逆对照，把 OP819 分清。** 复用 T946 同一 PE `bc056740…`、同一 F02 CONFIG/STATE、`--g13-split-op48 --g13-diagnostics hot --g13-tail-timing --g13-slot-snapshot`。A 用原 OP819，B 只追加 `--g13-seal-v1`（`8561de20…`），作 ABBA 并记录预热、pipeline/cache 状态。T909 只并行清 256 字与复制两组 255 字，串行 `gpuSha(2,uL[4])` 仍在。首轮同时核 819 整段、同根 host grant→业务停以及迟到后 fence 与原字。若 819 仍为主要成本，developer 在诊断变体中把准备、完整 state SHA、两 scope 复制/封印分段；GPU 仍按 canonical 场原字和标准 SHA-256 block 链做摘要。可用已锁定同一字节串的摘要，前提是能从全状态最后写入到 seal 证明对应 buffer/range/generation 完全未变，并在同根做逐字与独立 SHA 对照。非同一输入的摘要不得复用。 [T909 现有变体](developer-T909-v1.md) · [封存及 SHA](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp:73)
2. **求解和重放同版取证，先压力后决定黏性。** 同一 PE 的窄诊断每臂仅选一个范围。F02 采 ACTUAL/REPLAY 的 J1/micro0 各压力 cycle 入口、出口、`tracePressure` active/noop/converged 位和 ordinal，另记 J2/J4 预排空段起止。F02 黏性仅记录空体写集，实际黏性在 F03 的非零黏度周期剪切臂以相同方式采 `traceVisc`，按范围独立留下成功或迟到收据。现有早退路径仍会写 trace，任何槽裁剪需等价消费这些证据并保首错。点位和原字必须绑定已提交业务 fence 的句柄/代、nonce、pool/reset serial。若业务过期，先锁住未退役资源，等原 fence 后读取查询和从原工作区限量冷拷 `Completion256`（含 error/audit）、两个 scope trace 以及可重算 F02 的 u/v/π 和 RHS；冷拷自用 generation，保原根已拒身份。每组实报原字段 SHA、实际完成位、可用标记和首差。按测值选一个实际高成本算子阶段，先做分块并行和定序归并，或已收敛的等价轻量槽。两套 scope 分别计算和核证，禁止拿 ACTUAL 数值填 REPLAY。重放计量保所有实际 J 和失败前缀。 [T948 独立判据](../test/reports/t948/handoff.md) · [压力和黏性 trace 字典](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl)
3. **放行。** 目前有证据安排 819 单因素对照和 ACTUAL/REPLAY 压力分段。815/818 先保原审计，黏性等 F03 有实际迭代成本后再排数值内核优化。developer 用既有候选接入离线可核的诊断变体，tester 独立核 SHA 和场，effector 只在本机采实际 Vulkan 根。F02 即使原字段数值验证通过也仍是私有算子证据（`acceptedRoot=null`）。目标 13 仍按 T828 验实际及时根、同代速度/压力/黏性原字、真实散度与证书。黏性 RMS/max `2⁻¹⁶/J`、`2⁻¹⁴/J`，散度 RMS/max `2⁻¹⁴`、`2⁻¹²`，压力证书 `2⁻¹⁶`、投影速度证书 `2⁻¹⁶/J`，并保 T631 对各用例更紧的判据。目标 14 的 GridView 仍等实际可信同代场。 [T828 验收与交接](architecture-T828-v1.md)

## 2. G16 · B13 的余时拒绝和 B52 的期限失败

**B13 static J128。** “2” 是请求根数，实际只发第一根。业务 fence 第 1 代及时返回（grant 到回执 20.8149 ms），第 2 代为诊断冷拷。Completion256 `words 40..44 = 2,11,9,15,1`，即微步 15 的第 2 个事件段后仍有余时；`word58=17` 即前 15 步各一段加本步两段，`word59=32` 即两个 16 轮，`word60=276`，实际 130 dispatch/129 compute barrier。基底逐字等于 seed、private commit 仍为 0、未发布。冷档有两组各九盘（1–9）、同一静止底壁（wall 0）的真碰撞，journal flags `9`（admitted+collision），没有截断。 [B13 日志](../test/reports/t949/runs/g16/b13-static-2.jsonl:7) · [微步实现](../pc/vulkan/g16_multi_v1.comp:677)

| 微步 15 的账 | 按 GPU 原字重算 |
| --- | ---: |
| 名义步长 `1/(120×128)` | 65.104167 µs |
| 段 0 碰撞时刻（全局），9 条同组 | 1009.732950 µs |
| 段 1 开始（witness[6]）/ 进入余时（witness[7]） | 1009.732950 / 31.933687 µs |
| 段 1 碰撞时刻（全局），9 条同组 | 1033.599605 µs |
| 段 1 消费 / 实际剩余（由末段入口与 journal 反推） | 23.866654 / **8.067032 µs** |

两组接触 gap 原字均约 `+2.794 nm`，底排静壁解后候选法向速度接近 0。本配置重力下，从该正间隙再次落到底壁的单壁估算为 23.866635 µs，与两组间隔吻合。硬碰撞响应后又以重力自由漂移的底排重复撞壁耗尽两段，余时约占该微步 12.4%，远大于该时间尺度的 FP32 舍入。单壁估算只解释底排现象，候选计数和两组真实夹根仍无法排除余时内其他盘对或壁对。旧 `reason 10/stage 4` 在该臂首因已被越过，后续全域仍由原判据核实。 [碰撞解和微步推进](../pc/vulkan/g16_multi_v1.comp:473) · [当前两事件段物理上界](algorithm-T906-planar-avalanche-v1.md)

**B13 首次安全增量。** Developer 先补一个版本化的失败尾段记录 `{micro,segment,microStart,dt,remainingIn,hit,remainingOut,eventKeySet,roundedTOI,reason}`，均用实际 FP32 原字及原 fence。诊断重扫最后余时，以第二次冲量后的候选状态及实际姿态，复用全域 swept pairs 和有符号 gap 夹根，保一个事件见证或全域无新接触证书。诊断可在原 fence 后对隔离工作区做，继续返回 11/9 并保持原 bank。由 designer 决定如何按 T906 表达持续承托，或是否批准“两次碰撞 + 经证明无新接触的一次余时漂移”作为预算修订。后者只有在盘盘及四壁都能证明无新事件、累计移动和末端审计通过时才可版本化进入原根，余时从相同姿态推进完毕，保总时间、功账和原两次真实冲量；有碰撞、不确定夹根或数值错保原拒绝。不得把任意 `remaining` 填零、增加末次夹根宽度、跳过残差或把第三次实际碰撞当成漂移。tester 用当前 B13 原字重放，并加余时内确有第三碰、夹根不明和旋转反例。 [G16 接触判据](algorithm-G16-planar-wall-contact-v2.md)

**B52 J256。** Static 首根实际为 **258.6888 ms**（超 8.6888 ms），fast 为 **251.9185 ms**（超 1.9185 ms，请求 480 根只发首根）。两臂都实发 `J+2=258` 个 dispatch、257 个 compute barrier，seed/module 相同，姿态输入不同。submit 为 0、业务 wait/retire 为 2，`fresh=false`、`identityOK=false`、Completion 文件 0 字节。宿主在 result 非成功时保零数组，因此日志 `B=0,C=0,Ic=0,firstReason=0` 皆为未读出值。晚 drain 确认原 fence 成功但没有迟到 Completion 或分段时标。无法判断 GPU 何时实际完成、有效执行了多少步、是否曾触发旧 `reason 14/stage 8`，也无法从 8.6888/1.9185 ms 的 host 超额推出 shader 须省的精确时间。 [B52 static 原日志](../test/reports/t949/runs/g16/b52-static-1.jsonl:7) · [B52 fast 原日志](../test/reports/t949/runs/g16/b52-fast-480.jsonl:7) · [宿主收据读取](../pc/vulkan/sand/g16_multi.cpp:548)

**B52 首次安全预算增量。** Developer 在 `sand/g16_multi.cpp` 复用私有诊断退役接缝，补 record/submit/wait-exit/实际 fence 晚确认 QPC、按 FREEZE、连续微步块（起步先每 32 步）、SEAL/热拷 GPU 点。原 fence 归还后单独采 Completion、first reason、实际微步/段数、候选/迭代与 journal 有效前缀，记录原 fence/代和冷拷代，严禁推进 private commit 或 scene。以 B52 static 和 fast 的同 PE 固定种子各取得一次可解释的晚尾；若 host 等待粒度为主，用同一绝对截止复查等待与可用时标；若 GPU active 成本为主，选实际最高的一段做等价优化。第一可逆候选是桶 clear/prefix 的精确并行化（B52 每进入一段都扫 800 桶，single-lane 先清 count 再扫 prefix）。同一个 GPU workgroup 并行整型 count/scan，保持桶索引顺序、逐盘稳定 scatter、原 sweep/候选/CSR 键和首故障顺序。用原执行器离线对照每段的 bucket heads、pair/CSR、TOI、候选/审计与首错；收益看同一业务窗口整根时延。若时标将大头落在接触而非分桶，先做实际边/盘同快照并行，保每盘原邻接求和顺序和 16 轮上限。B13 与 B52 两档、静止及两转速按 T906/T928 验残差、穿透、连续姿态、守恒和同根呈现。旧报 reason10/14 作为回归输入复用，G16-C 继续依赖 B 的真实正例。 [原桶实现](../pc/vulkan/g16_multi_v1.comp:237) · [G16 接口和验收](architecture-T928-v1.md)

本轮完成原件关联、归因及增量合同。F02 缺原字段及逐 cycle/块的实际执行账，B52 缺晚尾和 GPU 分段，B13 需要持续承托或认证余时的明确算法裁定；这些前置之外已有可执行的计量工作。所有新增完整 shader 输入逐角色/变体按 49,152 B 边界检查，记录源码与 SPIR-V 实际字节及 SHA。
