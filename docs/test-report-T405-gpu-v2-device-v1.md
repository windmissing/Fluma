# T405 · GPU V2 双源与过载真机验证 — V2 机制真机在位并真实推进（325/508 commits）；双源准入与 quantum→Draw/Swap 闭环被结构性 GAP 停顿阻断（V1/V2 同因，非 V2 回归）

- 卡面：T404 签包，按 T401 同机 V1/V2 对照，成功 quantum 与真实沙水准入→同 token Draw/Swap，独立负控（GAP/GPU 超期/旧代/满队列），核模拟秒、有效新帧/墙钟、丢配额。
- 包：T404-complete-signed.hap SHA256 `e72786aa8d621b8b40342639f9cdfcd051f6a26b886e0843c3632f4e1ecac095`（本地核验一致，verifyApp PASS）。
- 路由：`grsm1=true`，pid 54591。证据：`test/reports/t405/`（51 文件，2 次时间线导出，SHA 见 t405_verdicts.json）。

## V2 机制真机在位（功能层证实）

1. **页面冷选生效**：诊断面板「下轮进度时钟：V2」→ `PROGRESS_V2_NEXT_RUN` 回执 → 新空场激活。
2. **V2 真实推进**：V2 run 下 physicsCommit 观测到 `0:325` 与 `0:508`（GPU 接受 quantum 真实发生）；status 出现 V2 专属字段 `pageSession=4`、`quotaCarry`、`quotaBalanced=true`——整数配额账在维护。
3. **pulse 链验证**：PAGE_PULSE 接受直至任何 tick 停顿 >33ms（uitest dumpLayout/系统卡顿）→ `CLOCK_GAP` 撤权 → 页面显示 `STALE_PAGE_SESSION`，需新 session（前台往返）恢复——与 T401「需新 session 或显式新 run 再入，不接收补发旧 pulse」合同一致。

## 负控 CLOCK_GAP 真机证实（功能层）

时间线导出 statusBefore（V1 基线）：`state=PAUSED_CLOCK / stage=CLOCK_GAP / permissionState=PAUSED_CLOCK / ready=false / epoch=47 / readyEpoch=0 / acceptedCount=0 / clockDeltaNs=48613281（48.6ms>33.33ms 窗）/ clockPhase=NEW_RUN_REQUIRED`——真实断档→撤权→清队列→需新 run，T401 合同逐项在位。时间线 dropped=3,663,823（长会话环形溢出，fail-closed 正确保留截断拒判）。

## 主目标未达成（功能闭环被上游阻断）

**双源准入 0 / quantum→同 token Draw/Swap NOT_ACHIEVED（V1 与 V2 相同）**。根因：结构性 GAP 停顿（DispatchExit 后 38–62ms，T376/T384/T390/T399 同族）使每 run 撤权成为常态；页面 `down()` 被 `allowed()`（permissionState==='READY' 且 epoch===readyEpoch）门拒绝——准入窗口从未建立。**这不是 V2 回归**：V1 基线同样 accepted=0。

## 未行使项（如实，阻断在上游）

GPU 250ms 超期独立负控、旧代拒收、满队列丢最新归档、lateSwap：准入门在链更上游即被 GAP 撤权关闭，无法到达。性能判定按卡面「性能与功能分别判断」**不做**（功能链未通，250ms 仅实验）。

## 移交 architect

1. 结构性 GAP 停顿已第 5 次跨包复现（T376→T384→T390→T399→T405），V1/V2 双时钟下均阻断准入——按 T395 §4「任何方案若在真实设备不能稳定交付，按原诊断回流」，建议回到停顿根因（DispatchExit 后封尾/完成等待族）的专项消解，而非继续时钟侧迭代。
2. V2 验证合同中「健康心跳+慢 GPU 可接受」项需准入链打通后才能行使。

## 边界

未改业务；双源真实触摸（命中 XComponent 由 TTHNI 证实）；SHA 4×16 注册；时间线导出 2 次（1 次 PACKAGE_SHA_REQUIRED 失败后补 SHA 重导成功，fail-closed 保留）。系统时间 23:03–23:50，会话内多次前台往返用于 session 重建（V2 合同要求的恢复路径）。
