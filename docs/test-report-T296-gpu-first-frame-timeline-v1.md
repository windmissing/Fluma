# T296 GPU 首帧真实时序采证（真机 BLOCKED + 部分 PASS）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T295-signed.hap，SHA256 `133cb07ea25426f267bfac1a717a1185eb1a274d2ae16dc06b8b7c5c4a49cec6`（实测一致）。pid 38330。

## 一、BLOCKED：timeline 1024 行冷导出缺 JS 消费通道（T295 交付缺口）

`libgrsm0.timeline(cursor)` napi API **已正确导出**（so 内 `grs::BridgeTimeline::Export` 符号 + napi 属性注册 `timeline` 可证；`timelineApi:"timeline(cursor),4rows,page"` 在 status 摘要可见），环形缓冲在**真实记录**（timelineSerial 51→77→80→106→115→119 单调增长，dropped=0，1024 容量未满）。但：

- 生产页面 `GpuResidentM0.ets` 全文**无任何 `m0.timeline(...)` 调用点**（仅 status() 摘要消费）；
- status() 摘要只含 count/dropped/serial 三个标量，不含行数据；
- 无其它页面/Ability/测试通道消费该 napi 导出。

结果：worker 时刻 trace 数据本体在设备上**无法取出**（无 JSON/hilog 热路径是设计内，但冷导出通道未接线）。T296 要求的"CommitReady→anchor→首帧→offer→CLOCK_GAP 连续 trace + work/wait/scheduling 原 ns + firstFailure"分账**无法产出**。这是 T295 交付缺口（NAPI 导出完成、页面接线未交付），回 architect/developer 补一条页面消费通道即可解锁（改动极小）。

## 二、已采证据（status 时间序列，JS timer 100ms 快照）

### 前台稳定 12s（40 快照 @0.3s，fg_1..40）

- `clockPhase` 40/40 = `NEW_RUN_REQUIRED`；`clockDeltaNs` **恒定 199246563**（199ms）不随时间变化——delta 只在 advance 时更新，恒定值 = worker 无新 advance（NEW_RUN_REQUIRED 空转直接证据）。
- `timelineSerial` 51→77（buffer 继续记录 invalidate/offer 等），dropped=0。
- acceptedCount 全 0。

### 一次明确后台→前台（Home→aa start 重进，ret_1..20 @0.25s）

- 前 14 帧：STATE_LOST / ser 80 / delta 199246563（旧快照）。
- 第 15 帧起：**PAUSED_CLOCK / ser 106 / delta 180675469**——rebuild→READY→CLOCK_GAP 真实发生且被记录（T282 原子权限语义在此包上生效：PAUSED_CLOCK=READY 曾提交后被撤权）。
- delta=180ms 是挂起-恢复后首个 wall delta 原值（未钳），≥33.3ms 阈值 → Pause/NEW_RUN_REQUIRED 显式。

### K192 闭环尝试（三轮 newRun+spawn）

- 三轮均 `acceptedCount=0`、`lastOfferCode=NONE`——spawn 需要 identity（READY 门），READY 窗口 <0.5s 未捕获。**source terminal→quantum→completion→lease→同 token 像素 NOT_RUN**（不冒 PASS）。

## 三、结论

- T296 核心目标（timeline 分账）**BLOCKED**于 T295 缺口（JS 消费通道）。
- 附带真机确证：CLOCK_GAP 循环因子与 T283/T273 一致；delta 原值 180/199ms 记录在案；无 FAULT、无旧 token 复用、dropped=0。
- 不猜 Doze 唯一因（dumpLayout 干扰与挂起恢复均可能贡献首 delta）。

证据：`test/reports/t296/`（77 项：fg/ret/k/w 快照 + t296a/b 布局）。
