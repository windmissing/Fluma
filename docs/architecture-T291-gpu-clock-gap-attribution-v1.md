# T291 GPU CLOCK_GAP 持续阻断首次归因 · 工程修订 v1

2026-10-30，architect；原卡 `6ae3fc72e4b031ea82c40fd9`。消费 T283 正式报告、`test/reports/t283/` 57 份 UI 快照及 `suspend.txt`、T282 修订与当前 `app_bridge.cpp`/`control.cpp`/`run.cpp`；不使用真机，不重跑测试，不改业务代码。此稿接续 T269 §5，限 K192 产品桥接时序与取证；不改 `GRS_FIXED120_TIME_V1` 的 120Hz、F≤4、33,333,333ns、carry、事务与线协议。

## 归因与证据边界

T283 的 `t283e/live_1/s_1/t283y` 分别保存约 205/211/204/157ms 的 `clockDeltaNs`、`advanceCode=6`、`PAUSED_CLOCK`、`ready=false/readyEpoch=0`；`acceptedCount=0`、无成功 root/quantum/lease/CURRENT。20 个 `live_*` 快照的 runLo/epoch/delta/carry 完全相同，是同一次暂停状态的 20 次采样，**不是 20 次独立超窗**。`w_*`、`r_*` 也有相同的状态平台。`suspend.txt` 仅三条 09:59:58.857–.905 的资源调度消息，未与每次 `clockDeltaNs` 的单调时钟、worker TID、帧序号配对。READY 快照本身未捕获；T282 的同锁发布/撤权由离线竞争测试与设备字段一致性支持，但 T283 的“每次 READY 后下一 8ms tick”及“窗口 <0.5s”不是上述采样可直接计时证明的事实。T283 报告的 60+ 尝试可作为执行记录，不能当 60+ 条独立原时序。

源码可证明：`wake.wait_for(8ms)` 是等待上限请求，不保证 8ms 定时唤醒；`last` 在 `Advance(OK)` 之后才置为本轮调用前的 `time`，在随后 `Pump()`、GPU 提交/轮询、scene 绘制、诊断或 worker 被调度出去期间不更新。下一轮 `time-last` 因而合并**前轮工作、等待、调度延迟**，并非纯 `wait_for` 误差，更不是可单独归到 Doze 的“挂起时长”。`Fixed120Clock::Plan` 超 33,333,333ns 会 Pause；bridge 也在调用 `Advance` 前以同阈值 `RevokeLocked(PAUSED_CLOCK)`、`run->Pause()`、`last=0`、`gap=true`，该 run 后续只 Pump quarantine 并要求显式新 run。现有状态只有最后 delta/carry，没有 last 的建立时刻、每次 wake/工作阶段时间或首帧 commit 的时间戳，故**已证的是设备上超窗与撤权，唯一环境根因未证**。`carryBefore≈0.943–0.946e9` 和 `plannedQuanta=1` 是前一推进状态的残留快照，不表示超窗被钳成一个量子；撤权后 `carryAfter=0`。

工程归因：产品桥接缺少可复核的首帧/活跃时间线，且在持续 157–211ms 间隔时必进入 new-run 环，现有签包无法给出 T269 §5.2 所需的“真实调度时间线”。这是**取证与生命周期接线缺口**，先修工程文档再交 developer；不能据此断言 OS 唯一因，也不能绕过时钟上限以获得假 CURRENT。若完成分段取证后证明稳定前台的真实最小工作周期长期超过 33ms，才把严格 33ms 窗口与设备吞吐的设计冲突交 designer；目前没有该判据的原始证据。

## developer 工程修订（不使用真机）

在实验 bridge 加有界、固定容量的单调时钟 trace；记录 `run/epoch/surface/inputGeneration/worker TID`、顺序号与 dropped/incomplete。每轮至少记：初始化开始/CommitReady、first anchor、wait 进入与 wake 返回、`Advance` 前的 now/last/delta/carry/返回码、Pump 进入/退出、submit/poll、scene Draw/swap、pause/撤权、新 run；同时记录 active/window/前后台与第一 source offer 的入队/消费/拒绝时刻。时间戳来自同一 `steady_clock`，冷导出 raw ns，不用 UI 轮询时刻代 worker 时刻；热环有总字节预算，不做逐帧大 JSON/hilog。`status` 只提供窄摘要和 trace 导出句柄/序号。初始化完成后首个正 tick 的锚必须明确：记录 `last==0` 的 first-tick 路径与后续首次 `Advance(OK)` 更新；不得暗中将长墙钟拆步、钳 delta、丢 carry 或把 `PAUSED_CLOCK` 复位成成功。保留 T282 原子 READY、旧代隔离与当前 pause/quarantine；若需要修改公共时钟/状态机，先回 architect 复核守恒/事务边界。

非机定点检查同一个生产 bridge：注入 wake 延迟与 Pump/Draw 延迟分别超过阈值，trace 能区分两者且 `CLOCK_GAP` 原序列不变；首 tick、无源空帧、源在飞、pause/new run、旧 epoch/输入包、trace 环溢出均能给出首因与 dropped，不将 READY 当 source ACK。编译/非机自测/签包即可交开发卡；不把设备稳定 2 秒或 T283 真机结果加作开发门。

## tester 定点补证请求（待工程包，真机串行）

同一签包 SHA/PID/TID/run/epoch 采连续 worker trace，配对显示/窗口/active/电源状态与系统 suspend/resume 时间；分别在亮屏稳定前台与一次明确后台→前台周期取 `CommitReady→首 anchor→前数帧→首次 offer→CLOCK_GAP`。报告每段 `work/wait/scheduling` 原 ns 与 first failure，保留原始 trace、溢出数及 UI 状态；若前台周期也超阈值，展示占时阶段；若仅系统挂起，给出可配对的 suspend/wake 证据。只有 READY 稳定且同一 K192 run 真实 source terminal→成功 quantum/completion256→lease→XComponent+shader 同 token CURRENT/像素，才能标产品最小链 PASS；acceptedCount=0 时保库存与端到端链保持 NOT_RUN。失败只 FAULT 或明确完整旧 token STALE，无 Canvas 回退。
