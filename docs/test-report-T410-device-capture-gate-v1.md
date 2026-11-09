# T410 · GPU V2 同窗双源与负控真机补证（采集门 CAPTURE_BUSY 阻断，如实）

2026-11-09，tester 真机。包 T409-finalized `b27eb2fa…067d` 本地核验一致并安装。**主目标未达成：`beginEvidenceCapture` 在真机上恒 `REJECTED CAPTURE_BUSY`，全部下游清单（空场 QUANTUM/双源 token 链/六项负控/30fps 报告）未行使。**

## 1. arm 尝试全记录（每次 REJECTED，零副作用）

| 尝试 | 配置 | 结果 |
|---|---|---|
| l4 | SHA 空 | `INVALID_CAPTURE_CONFIG`（config 校验先于 busy，符合代码顺序） |
| l7 | SHA 4×16 注入完整核验 | `CAPTURE_BUSY` |
| l8 | +6s 等待 | `CAPTURE_BUSY` |
| l9/l10 | **force-stop 冷重启**+重注 SHA | `CAPTURE_BUSY`（干净进程！） |
| l11 | 停止按钮（pulse 停） | `CAPTURE_BUSY` |
| l12 | Home 往返重建 session+停止 | `CAPTURE_BUSY` |
| l13 | **40 次连点跨越多个 worker 周期** | `CAPTURE_BUSY` 40/40 |

## 2. 根因分析（源码级）

busy 判定 `captureWorkBusy||captureRootBusy||jobLive`（`evidence_capture_app.inc:45`）：

1. **`captureWorkBusy` 是 RAII `WorkInterval`，包裹 worker 每帧循环体全程**（`app_bridge.cpp:396`）——dispatch/fence/poll 整个周期都 busy，空闲窗仅循环间隙（微秒级）。UI 按钮 arm 经 uitest IPC 调度无法命中（40/40 未中）。
2. **`jobLive`** 在 JobIntent（permit 建立）置 true，仅 RevokeLocked/worker 完成/RUN_RESET 清除。页面显示 STALE（physics 层）不等于 permit 清除——结合 T376/T384/T390/T399/T405 结构性停顿家族（**第 6 次跨包复现**），root 永不封尾 → jobLive 恒真。
3. **实现与合同偏差**：T407 §4.3 合同定义冷边界="worker 无活动 root 且接受/呈现无在途"（等待不持锁）；实现把整个 WorkInterval 当 busy——比合同窄。T409 开发自测通过是因为 HOST_SCRIPTED 在脚本精确 idle 边界 arm；真机 UI 节奏无法到达。

## 3. 未行使项（卡面如实）

空场 QUANTUM、双源真实沙水 SOURCE→ACK→QUANTUM→同 token Draw/Swap、负控（GAP/GPU 250ms/pending4/SOURCE FIFO16/旧代/lateSwap）、30fps 有效新帧/模拟秒/墙钟分列——全部在 arm 门上游被阻断，**NOT_ACHIEVED/NOT_EXERCISED/NOT_MEASURED**。

## 4. 工具链就绪（后续消费零改动）

- T411 T409-schema 检查器：档到件即可核
- T412 配额 oracle：账到件即可核
- T416 token 链判读器：`--summarize` 即出报告（host 档已判：mixed PASS 8/8、late 反例预期 FAIL）

## 5. 移交 architect/developer

二选一：(a) 将 arm 冷边界收窄定义改回合同（循环内 idle-without-root 检查，而非整个 WorkInterval RAII）；(b) 提供脚本化 idle 边界 arm 入口（诊断面板命令）。tester 在现实现下被 `CAPTURE_BUSY` 恒阻断——每次尝试零副作用（合同保证），全部原始证据保留于 `test/reports/t410/`（l0–l13 布局快照+verdicts）。
