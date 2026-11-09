# T417 · GPU 采集预约与 owner 安全点

2026-11-09，architect，P5。静态核对 T410 原档、T409 签包与对应源码。不使用真机。本文修订 T407 §4.3 的采集启动协议。

## 归因与证据范围

T410 的确遇到采集启动拒绝。T409 把「请求采集」与「当场开启窗口」合成一个同步操作。页面点击必须碰上 owner 安全点，失败后请求直接丢弃。这个接口缺少让 owner 在下一合法边界消费请求的机制。该工程缺陷足以解释健康运行时为何也可能一直抢不到采集窗口。现有设备档无法区分这次究竟是抢不到安全点，还是当时一直没有安全点。

签包实测 SHA256 为 `b27eb2fab5fd89a65cdc6cd9e4bf3873d0c8c0bfe47b71c6e0e0b9127024067d`，与 T409/T410 记录一致。T410 的 14 份布局与 verdicts 清单 hash 一致。签包来源 `t409-handoff` 中的 `app_bridge.cpp`、两个采集 `.inc`、`evidence_capture.h` 和 `m1/run.cpp` 与本次核对的工作区内容一致。

| 原档 | 可核验的事实 |
|---|---|
| `l0`–`l13` | 14 份布局的实际 `progressClockRevision` 全为 **V1**。`l7/l8` 显示 run3/epoch4/session2 `CLOCK_GAP`。冷启后 `l10/l11` 为 run1/epoch2/session1 `CLOCK_GAP`，`l12/l13` 为 run1/epoch2/session2 `RESUMING`。这些是页面快照，提交计数另有自己的身份。 |
| `l4`、`l7/l8/l10/l11/l12/l13` | `l4` 空 SHA 得 `INVALID_CAPTURE_CONFIG`。其余快照显示 `REJECTED CAPTURE_BUSY`。`l13` 只保存末态。40 次连点来自 tester 的操作记录摘要，缺逐请求回执和 TID/时间链。 |
| T410 verdicts 和原卡 | 无已开启采集档、无三个布尔位的原子值、无安全点序号、无同 root 的调用/排空证据。无法判定拒绝时对应哪个门，也无法量化安全窗口宽度。 |
| 源码与 host 用例 | `controlUnits` 手工拉高/清除 `captureWorkBusy`。实际 ANGLE 用例在 `Wake` hook 内直接提交，hook 位于 `WorkInterval` 构造之前。它验证了已知安全边界启动及后续采集，覆盖范围不含生产 NAPI 到持续运行 owner 的交接。 |

停止按钮只调用 `model.stop('STOP')` 停源，前台 FrameCallback 仍调用 `pagePulse`。这一步不可当作停心跳负控。`CLOCK_GAP` 屏显属于当时 V1 的控制判据，不能替代 V2 真实页面心跳或 GPU 250ms 期限证据。三位 busy 值、根是否最终返回和 V2 性能均未知。T410 的空场 QUANTUM、双源链、六项负控和 30fps 结论继续保留为未行使或未测。

## 三个门的真实含义

| 门 | 源码行为 | 裁定 |
|---|---|---|
| `captureWorkBusy` | `app_bridge.cpp::loop` 的 `WorkInterval` 从 wait/wake 区块之后覆盖几乎整个循环体。包括 EGL 初始化/退役、控制检查、未领到配额的 `continue`、Pump、Draw/Swap 和发布状态。进入下一轮 wait 前解除。 | 粒度比 root/接受/呈现生命周期宽。不可把每轮都解释成 GPU 活跃，也不可根据布局断言可用区间只有几微秒。等待分支可能释放锁睡眠，实际路径需原子观测。 |
| `captureRootBusy` | 普通 `Pump` 返回后缓存 `run->HasFlight()`，run reset 时清零。`HasFlight` 只含 `submitting/inflight`，而 `gap` 分支、撤权清理、drain 的若干路径并不走同一缓存更新。 | 只作现有状态提示，不能决定采集安全。实际 transport 仍可持有半提交、fence、被隔离的旧 root 或清理状态。 |
| `jobLive` | V2 创建 JobIntent 时置位，原 `RevokeLocked`、reset、跳绘或呈现终态清除。V1 permit 分支不置此位。逻辑撤权可以早于物理清理。 | T410 布局所见实际 V1 不支持「V2 的 `jobLive` 恒真」或「第六次同一根因」。V2 中也不能用该位替代真实 drain/Swap 状态。 |

现有 `BeginEvidenceLocked` 在 busy 检查之后才比较配置身份，因此 BUSY 也没有证明 epoch/run/session 匹配。它先运行 `CheckEvidenceLocked`，已有窗口可能按自己的到期或换代规则冻结，原档不能证明操作级零副作用。NAPI 对每次通过解析的尝试先构造约 2.8 MB 的 storage，连点存在额外分配成本，设备上的实际开销未测。

## 修订合同

派给 developer 的最小范围为 `app_bridge.cpp`、`evidence_capture_app.inc`、`evidence_capture_napi.inc`、现有 capture 类型与隔离 `GpuResidentM1.ets/GpuEvidenceCaptureCold.ets`。必要时在 `m1/Run` 和 `FullM1Transport` 暴露 owner 专用纯只读快照。只在现有隔离宏入口启用。不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止，局部受阻先做其他项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，除本人行状态有限例外。不改滴答同名清单。前后景仅 XComponent+shader，失败沿 FAULT 或完整旧 token 的 STALE 处理，禁止 Canvas 上屏和回退。

### 预约接口与身份

| 接口 | 约束 |
|---|---|
| `beginEvidenceCapture(configJson:string):string` | 保留 T409 九字段、严格 U64、SHA 与预算校验。先在短锁下验证当前 activeRevision/run/epoch/session、前台和窗口绑定，再占唯一预约槽，生成单调非零 `requestId`，回 `RESERVED`。捕获 owner/context/surface/geometry/config 作为内部身份。未提交有效 run/窗口时回具体 `CAPTURE_SCOPE_UNAVAILABLE`，身份不匹配回 `CAPTURE_IDENTITY_MISMATCH`。 |
| `evidenceCaptureRequest(requestIdDecimal:string):string` | 只读返回该预约的 `RESERVED/ARMED/FROZEN/CANCELLED/TIMED_OUT` 与具体 reason。保存 requestId、requestNs、armByNs、实际 scope、最新 blocker 和序号。ARMED 后返回绑定的 captureId 与 startNs。旧 requestId 返回 `STALE_CAPTURE_REQUEST`。 |
| `cancelEvidenceCaptureRequest(requestIdDecimal:string):string` | 只取消同一仍 RESERVED 的请求。重复取消回同一终态，旧 ID 拒绝。已 ARMED 则返回当前已启动状态，由原 end 接口显式冻结。 |
| `endEvidenceCapture` / `evidenceCapturePage` | 继续使用 captureId，非 requestId。原幂等 end 与冻结分页语义保持。新预约在真正 ARMED 之前保留旧冻结档，取消或超时后仍可导出。导出期间页面禁止发起下一预约。替换成功后旧 ID 明确拒绝，导出器据此拒绝不完整档。 |

新控制回执标 `T417_CAPTURE_REQUEST_V1`。同身份同配置重复预约返回同一未完成请求，不重新编号、分配或延长等待。异配置在已有预约或活动采集时显式拒绝。锁内重查身份和预算后才提交。失败或过期预约不生成 captureId、BEGIN、伪造的空档或丢配额事件。现有 T409 冻结档字典和 sourceCommand32/completion256 不改，新增预约收据独立归档，必须以 requestId→captureId 与真实 scope 配对。

等待上限定为 **1 秒单调墙时**，从原生接受预约起算，是诊断响应预算。`now >= armByNs` 只能到 `TIMED_OUT/CAPTURE_SAFE_POINT_TIMEOUT`，优先于同刻消费。开始之后再按 `startNs + durationNs` 独立计算原 2–5 秒采集窗口。加法溢出显式拒绝。复用原 native 监督线程检查等待期限，允许 owner 卡在 GL 调用时由监督线程终结预约。查询不得刷新等待时间、GPU 期限或 pulse。终态同时保留真实检测时刻和延迟，避免把 1 秒预算描述成操作系统准时保证。

预约、取消、超时、换代和 owner 消费在同一控制锁线性化。epoch/run/session、实际 revision、窗口、owner/context/surface/geometry/config 或前台换代使待处理预约取消。传感器过期、真实 GAP 与 GPU 超期仍由原路径处理，预约不能触发恢复、重置、释放资源或撤销生产工作。已稳定但暂停的同身份 run 可采诊断窗口，开始收据保留既有 `phase/permission/firstFailure`，既有故障不伪记为窗内新故障。身份中的 input 仍按原每个 root permit/事件记录，不能因正常输入更新把诊断预约误判为换代。

### owner 消费与安全快照

由 GL owner 在实际事务边界执行 `TryArmReservedLocked(ownerSnapshot, nowNs)`。入口位于本轮控制/身份校验之后，下一次 offer 消费、PlanGrant、JobIntent、Pump 或 Draw 之前。优先在循环顶和 wait 唤醒后检查，覆盖初始化提交完毕、普通 root 完成、cadence 跳绘、呈现完成、已撤权排空和静置等待。必要时在正常现有 `continue` 后下一轮统一进入，不为采集重排业务。停在同步 EGL/GL 调用的 owner 无法跑到安全点，监督线程只记录超时。

安全快照在 owner 线程读取，禁止 UI/监督线程直接读取 worker 私有对象。要求全部满足以下条件。

- 本代 run/window 已提交，非初始化、退役或资源换代中。Run 无提交/在途/隔离轮询，transport 无活动 root、半提交或未处理的 root fence，`drainingRevoked` 已完成。`Run::HasFlight()` 和 `SubmitPhase::IDLE` 任一单独成立都不充分，尤其在 `DiscardForControl` 清 run 位但 transport 仍排空时。
- 本 root 的接受事务、Draw/Swap、发布及 scene lease 释放交接都已结束。禁止在 Swap 回调内、接受已完成但待 Draw、或 Draw 返回前启动。正常已交给 transport 管理的 release fence 继续按原 lease 复用门处理，无须为只读采集增加 GPU 等待。释放失败沿原故障/排空，不能视为正常完成。
- 无仍可执行或重试的旧 JobIntent。`BUSY` 尚未消费 nonce 时也不能偷换许可。若 `jobLive` 留有已终态的旧元数据，依据该 root 的真实完成/撤销与清理结果识别，不能由采集入口清位。存在排队的源、pending quanta 或剩余计划但无活动 root，可以在两 root 之间启动，保持原队列和许可次序。

owner 到点后在同一短锁内再次核身份/时限，切换预备 storage、记录 startNs/首账和 BEGIN，已有 Ready 才记录 READY。随后本次正常工作继续，下一条 JobIntent 可被捕获。owner 给出单调 `safePointSerial`、`workerPhase`、阶段进入 ns、root kind/nonce/permit start/deadline、是否 drain/Draw/Swap 和最近完成序号的定长诊断快照。只在状态切换更新，不逐轮写文本或逐次打印。超时回执保存预约与最后观察的时间和 blocker，如 `WAIT_ROOT`、`WAIT_REVOKED_DRAIN`、`WAIT_PRESENT` 或 `NO_OWNER_CHECKPOINT`。最后一次采样不足以定位未返回的具体驱动调用时，保留未知，不从缓存状态推断调用耗时。

使用短临界区和现有 wake 通道唤醒 idle owner。线程在 wait 前检查新预约，通知只消费一次，不以待处理预约为永久真谓词使等待自旋。没有安全点时保持原 worker/poll 节奏。复用固定容量 storage，最多一份现有采集和一份已准备的后继块。隔离包冷初始化时准备首块，后续回收与准备在控制锁、GL 提交路径和真实 FrameCallback 外完成。存储未准备好回 `CAPTURE_STORAGE_NOT_READY`，页面显示准备状态，等待 ready 后方可发起新预约。开始和取消只转移已准备块的所有权，不在短锁内清零整块内存。页面复用既有每回调单快照，在 pulse 之后读轻量的预约字段，不增加 status 轮询或阻塞等待。UI 区分预约成功、窗口已开启、冻结、取消和超时。测试者只在实际 ARMED 确认后发健康窗口的业务输入。

物理仍按 120Hz 子步配额。V2 真实控制 GAP 仍为 `>33,333,333ns`，root 从原意图起算的绝对 250ms 期限继续逐关核对。有效新内容帧目标为 30fps，节流只作用 Draw/Swap。预约不会改变 source/ledger/lease/epoch、伪造或延迟 pulse、扩大旧许可、暂停 GPU 或强制等待 GPU。预约后的真实控制故障与采集门故障要分别报告。

## 检查与后续路由

**先 developer 修订并签包，再 tester 真机续办 T410，结果回 architect。** 不再靠连点命中旧入口。离线先覆盖以下生产路径，卡内编译与自测即可交付开发。

1. 从普通 NAPI/控制调用在 owner 连续工作时提交一次，在下一真实安全点自动 ARMED，验证首账、BEGIN 先于后续 root 事件。覆盖已有安全点但队列/计划仍非空、owner 正在 wait、cadence 跳绘、V1 GAP 后排空、V2 真实暂停等路径。host hook 只注入控制时钟或 GPU 延迟，不能替页面把预约塞进 Wake 空档。
2. 在半提交、fence、撤权后未排空、接受后待 Draw、Swap 内及释放交接时预约。确认这些阶段只返回 RESERVED。持续卡住时监督可独立超时，保存实际 blocker，之后 owner 晚到也不能开启旧预约。验证时间等号、cancel 对 arm、换 run/session 对 arm、重复和旧 ID，超时及身份拒绝保留上一冻结档。
3. 同输入比较未启用采集和预约/取消/超时路径的许可/账本/源边界、接受前缀和 lease。验证真实 GAP、250ms+1ns、旧代、lateSwap、归档预算及原 T411/T412/T416 消费接口。检查冷分配和 UI 额外开销。完成编译、开发报告和新签包 SHA 后移交，无需等待真机结果来关闭开发卡。
4. tester 标真机、GPU 优先串行。先在同设备各作实际 V1 与 V2 的单次预约门验证。V2 必须冷选后新 run，归档 `actualRevision`、run/session/epoch、配置回执、原子预约/安全点快照、ARMED/FROZEN 或超时终态。健康窗关闭 `dumpLayout` 与外部轮询，命令起止与装包证据单列。若超时而 owner 仍在推进，查漏掉的安全点。若最后真实 cut 无返回或 drain 未结束，带该 cut 的证据回 architect，区分采集饥饿和 GPU 工作停顿。
5. V2 预约与同窗取档成立后，按 T407 §5 复用既有 T411/T412/T416 工具，完成空场 QUANTUM、真实沙水 ACK→后继 QUANTUM→同 token Draw/Swap、独立负控及 30fps 有效新帧/模拟秒报告。测真实 GAP 需专门停止真实回调的隔离控制，停止源按钮不替代。原卡保留设备结果，失败或缺证不升级为产品通过。

2026-11-09，T417 完成签包、归档、真实版本、三个门与 owner 生命周期的首次核对。设备上哪个门导致每次 BUSY、是否存在新一轮 GPU 停顿及实际 30fps 仍待修订包取证。
