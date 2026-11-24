# T898 · PC 持续落沙与自主续帧首次归因

> 2026-11-24 修订：[T915](architecture-T915-v1.md) 处理普通 FULL_M1 在已接受 BASE 上恢复 CLOCK_GAP 后的同次持按。保真实物理按压只读身份，临时撤销旧 SOURCE，按新会话的真实相邻拍重新授权。沿用本稿的真实退役与输入取消门。

> 2026-11-24 修订：T899 A 在用户复试中的负载、实收 SOURCE/累计 LAND、重力限角和回正沿 [T908](architecture-T908-v1.md)。T898 的退役/许可和同根可见性合同继续适用，LAND 取证使用实际 kind 8 行。

2026-11-23 · architect · P5 · Windows Vulkan · 用户现场日志离线复核

**连续入料曾经发生，停住后的自动恢复仍缺一段。** 现场一个 630 ms 的长按实际受理了三笔沙源，松手到下一次按下前还有八个自主 QUANTUM。主要中断是 `ROOT_DEADLINE`。T894 A 能安全退役并恢复输入 READY，但把仿真恢复放在 `awaitSource(true)` 后面，必须再按一下并成功入料才继续。固定四步共享 grant 加剧超期，尾部也有首个量子得到近完整 250 ms 仍未完成的原件，需分别解决。 [机器复算](architecture-T898-evidence-v1.json)

## 用户现场原件

使用卡面 `.codex/local-test/demo-recovery-20261123-202032-832.jsonl` 的关闭后完整快照，2,247,877 B、2,110 行、SHA256 `219a6a7c286f3a6ce2e23e4c6595cfc502065abbbbff9a55486e1aa9918bd11e`，末行实际 WSI `RETIRED`，返回 0、publication 已释放。日志是 `DRY_M1_V2` 和 `EMPTY_PRESENT_QUEUED` 的实际交互路径，命名 T894 A 的本地 PE 实物 SHA 与交付匹配。源码钉 `8787f323`，现场日志不含 PE 自证行。QPC 为 10 MHz，单 run `15963073238641`、geometry 2。以 Completion 的 LE 原字、owner 实际返回和 WSI 根身份联接，不按日志行数冒计根。现场没有 `M1_RESERVATION_RETURN/TERMINAL`，未被领用的端点原因仍待完整 trace。70 行有重复诊断键，核验统计使用唯一原根及字段，READY 另以真实基线和 Win32 Down 的 `readyQpc` 关联。 [T894 A 工件](../test/reports/t894/delivery.md)

| 实际问题 | 原件复算 | 本轮覆盖 |
| --- | --- | --- |
| 一次按住是否会继续 SOURCE | 56 次 Down 的原 hold 均有效。按唯一 SOURCE nonce，3 次无接受、48 次恰一笔、4 次两笔、1 次三笔。共 59 笔非零，6,878,928 Q20。hold 83 的原行 118、189、193 分别受理 129,602、127,272、131,757 Q20，共 388,631。 | 同一个 hold 可在新帧边界连续发出 SOURCE。命令和生产者摘要均从原字复算，`requested=8×实际相邻拍Δns/10^9`，Q20 的 admitted+rejected 与实到量相符。任意一笔偏小先查实际相邻拍。例如 hold 852 只受理 6,809 Q20，原 Δ=811,700 ns。长按总墙时不可直接乘为实际入料。 |
| 松手是否会停掉物理 | 共 53 个新 QUANTUM 的 nonce 在实际 Up 之后、下一 Down 之前，相应停源窗无新 SOURCE 受理。hold 83 的八个根在原行 197–203 和 205，activeSand 恒为 5。 | 已证健康阶段无需再点即可推进。该窗口短，未观测完成落床。137 个排队呈现的 QUANTUM 的 liveSand 范围为 1–65，`bedEventCount=0`，还缺逐笔 LAND 原件。 |
| 什么时候变成“再点一下才动” | 52 个唯一 QUANTUM 进入 `M1_STALE_WAIT_NEW_INPUT`，43 个 `ROOT_DEADLINE`、4 个 `LATE_FENCE_SIGNAL`、5 个 `CLOCK_GAP`。另有 7 次等待输入时的 CLOCK_GAP 收据。52 次恢复都核到真实退役结果 0、`fencePending=false` 及额度／carry 平衡。 | 首笔 hold 3 在第 36 行退役后停住，实际 Up 后到下一次 Down 之间无新根。hold 246 按 1.724 s 仅两笔，第 460 行退役，随后留在同一按压约 1.158 s 仍无新根。末尾 3.997 s 和 6.416 s 的两次长按各有一笔 SOURCE 被接受，但无业务 QUANTUM 新图，接受账到了 sourceCommit 59，最后一次呈现只到 57。 |

> 松手窗口按实际 QPC 判断。hold 83 的第 196 行 nonce 在 Up 前，Present 退出在 Up 后。第 205 行输出在下一次 Down 之后，其 nonce 仍早于该次 Down。机器收据分别保存根起点及 Present 退出时刻。

## 两个独立根因

1. **共享 grant 的剩余时间耗尽，加上少数单根长尾。** `ProgressClockV2::Claim` 将最多四个 `pending` 一次冻结到 `activeRemaining`。Win32 `RunM1Quantum` 使用同一个 `permit.grantedNs+250ms`，每个根还串行经过实际 Present。43 个 `ROOT_DEADLINE` 里有 41 个在同 grant 已有 1–3 个接受 QUANTUM，前面的 compute/fence/WSI 已消耗这份墙时。第 36 行第四个 QUANTUM 开始首次 fence wait 时只余 **7.631 ms**。43 个 deadline 首次 wait 的剩余预算中位数 **69.462 ms**。全部已接受根的 nonce→业务 fence `VK_SUCCESS` 宿主间隔中位数 **75.237 ms**、P95 **158.348 ms**，包含录制、提交与等待。末尾第 2101/2106 行是纯 QUANTUM 新 grant 的首步，各有 **249.297 / 249.792 ms** 首次 wait 余量仍超期，相应 fence 在期限后 **22.991 / 51.568 ms** 的退役边沿可确认。拆小 grant 有明确作用域，无法覆盖这两次单根长尾。原日志没有实际 GPU timestamp 区间，不能定为某个 shader 算子或精确 GPU 耗时。 [共享配额](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h) · [QUANTUM 原截止](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc)
2. **回收完成后，物理被新入料挡住。** T894 A `recover()` 先关旧 hold，核同根退役并撤销旧 session 的配额，发布 `STALE_WAIT_NEW_INPUT`，建立新基线后发 READY。接着调用 `awaitSource(true)`，设置 `clock.ready/baseline` 和调用 `RunM1Quantum` 之前必须取得一个 SOURCE 并接受。Win32 即使已有新拍、`inputReady=true`，旧按压的 MOVE 也不会自行恢复 hold，用户松手后更不会产生新 offer。故障前的 `nextPublication` 本来支持无源 QUANTUM，故障后的 `recover()` 才使继续演进依赖再点击。同一根中有过 `SOURCE_HOLD_REVOKED` 两次，旧 hold 在 Up 时被正确撤回，该边沿本身不授予新源。 [T894 恢复位置](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [输入授权](../pc/vulkan/main.cpp) · [T891 原安全边界](architecture-T891-v1.md)

## Developer 修订交接

### 1. 把 SOURCE 授权与继续物理分开

- 普通交互窗口已有 accepted BASE 时，沿 T894 先关旧 hold、隔离拒绝根、核原实际同 fence、tombstone Poll 及 owner Idle、无 in-flight、无阻止复用的 pin。按原事务关 pending/active/carry 并保余额证明。用新的真实 callback/native receipt 重建基线，从**最近已接受 bank**恢复，独立保存 `lastAcceptedRoot` 和 `lastPresentedRoot`。GAP 时只有实际新拍可恢复，未退役、换代或设备失效不开放旧代。
- 基线建立后进入 `READY_PHYSICS`，下一个真实间隔正常积累 120 Hz 配额，直接从原账领纯 QUANTUM，不调用 `awaitSource`。指针侧可同时发输入 READY，但只有其后合法的新 Down 才有新的 SOURCE hold。旧 hold、旧端点、被拒绝根和已清额度都不补发。SOURCE 被 Up/切换撤销且物理许可仍健康时，只结清 SOURCE 终态，保合法 QUANTUM。首个 SOURCE 以前无已接受业务态时沿原空箱/等待路径，以免虚造根。
- 在同代有效上一帧上保 STALE 空档，第一张新版 CURRENT 必须是新接受的真实 QUANTUM 和同 lease shader Present。普通单源/双源沿该路径。D1、诊断单次、专用冷档保原终态及 owner 覆盖合同。没有真实 pulse 时记录原 fault、回到有界等待，无伪造帧或时间。保持 SOURCE 零物理时长、根原子性、沙账、发布与安全退役。 [T894 实际退役](../test/reports/t894/delivery.md)

### 2. 缩短单份 grant 的实际工作队列，保原 deadline

- **第一版隔离候选用普通 PC 交互每次最多一份 QUANTUM。** 在 `Claim` 之前决定限制，grant 只取 `min(pending,cap)`，未领的真实额度留 `pending`，仍受原四份缓存和 overload 规则，用同一 `Balanced()` 等式复核。保实际 `generated/accepted/pending/active/overload/revoked`、carry 和固定 120 Hz 步长。新 grant 只从当前真实 fresh pulse 授权，截止仍为实际领用后 250 ms，绝不续期、重领旧 grant 或丢掉已授予配额来制造边界。较长批在实测能覆盖 fence/WSI 及 SOURCE 成本时再按适配器测定。只有实际 QUANTUM 接受才消费一份。新的 frameSerial、ordinal、eventStart 和同帧 SOURCE 前缀由原规则增加及独立核对，源码接口给原单次和 D1 保持现形。
- **持续 hold 在合法边界继续有入料机会。** 保 T882/T894 的最新有效相邻拍预约，先看同一 hold 且端点仍在原 freshness 内，已认领不可替换。可用预算内优先一次 SOURCE 后留合法物理步骤，预算不足的边界允许先做 QUANTUM 并记录 SOURCE defer，避免空等旧 offer。公平判据应按持续 fresh 的真实入料请求和接受的物理进度验证，比较每段可用、接受、过龄、替换及真实累计入量；不得把被丢弃的拍合成一笔，不能拿保留的老画面冒充活跃落沙。若保守限额下 SOURCE+首 QUANTUM 仍常不能在同预算闭合，按既有零物理 SOURCE 与下一帧新的真实 QUANTUM grant 接续，重验 NEXT_FRAME、事件 arena 和两个原根授权。任何改动不得延长在途根期限。
- **单 QUANTUM 有近完整预算仍超期，独立定位。** 新收据记实际 grant、record begin/end、submit enter/exit、短 wait、业务止因、退役、graphics 与 Present QPC。关联实际设备、pipeline、库存/活粒数、正常与晚完成的 same-fence 原结果。对 127 dispatch 单步做相同初态、低负载与场景累积后的成对采样。支持时在真实 fence 后取实际 GPU timestamp，包括 T889 的失败退役后诊断方式，并以 query 开关做有界对照。PC 实时 pulse 另报 callback/native 间隔、Fresh age 和 trace loss。只有证实 timer/日志造成真实缺拍时才分离 Win32 producer/trace 热路径，错开同步写出，禁止补采成虚构的历史拍。保 33,333,333 ns、250 ms、原 GLSL 生成上限、物理和 GPU 守恒，明确分阶段测出结果。 [T889 分时方法](architecture-T889-v1.md)

### 3. 目标 1 / 3 的独立验收

| 场景 | 必要原件与判据 |
| --- | --- |
| 单次长按 | 固定实际 Windows PC、PE、三模块、配置/初态和输入位置，按下后持续一段预登记长窗，既测不移动也测真实移动。单个 Down/hold 必须连续跨多个合法边界受理非零 SOURCE，至少三笔作为功能正例。持续过程取逐 offer 真终态、端点/命令 SHA、实际 Q20 与 sourceCommit、同根几何/发布、拒因及时间停顿，不以多次点击凑数。代表性重复和实测进展率另登记。 |
| 无新输入落床 | 用真实权威根证明 Up 时仍有活 P，Up 后不发 Down/源。在同 run/geometry 继续实测接受 QUANTUM，sourceCommit 和总沙量固定。至少针对一个真实 `LAND` 用有效事件段的原字、前后 P→S 和来源守恒核验，保后续沙床存在和同代 shader 帧。T892 活粒松手后归零及原首根冷档可给夹具，现场单底边探针与 `bedEventCount=0` 不能替代 LAND。 |
| 期限恢复 | 一次真实超期或受控故障夹具，分别覆盖仍按住及已松手。原拒绝与实际退役保全，等待真实新 baseline；此后在没有新 SOURCE 或点击时看到新的物理 commit 和实际 CURRENT 帧。另测晚 fence 不接受、退役 pending/设备丢失/旧代拒绝。独立保原 D1 和同材料预约回归。 |

已离线重跑 T894 A 的退役／scope／pin／配额自测、T848 的 T892 冷档关联（`NEXT_FRAME`，94,408 B），以及 T852 四组合成正控和十五项负控。本次交付为首次归因与工程合同，等待 developer 修订后由 tester／effector 分别核独立判据和新 PC 实物。全程 GPU 驻留、shader 渲染，禁止 Canvas 回退。 [T892 停源原件](../test/reports/t892/report.md) · [T848 归档](../test/t848/check.py)
