# T593 v4 · T676-B 正式 M1 采集——**READY 已发布**（run 2/epoch 4 原件）

日期：2026-11-19，effector（tester 兼），真机（011124264R000337）。
被测：T593-probe-signed.hap（t597-fixed 正式 M1 + tester T593Probe 设施页，
inputSha 7463e8a0→signed 133f1257，sign=PASS）+ 体验页原生导出。
证据：`test/reports/t491/t593c/`（t676b2.ndjson probe 基线、t676b-boot2.json
POST_CREATE boot 完整原件、p1–p3 布局）。

## T678 §B 采集清单执行

| 端点 | 结果 |
|---|---|
| status（probe 页基线） | WAIT_WINDOW / ready=false / epoch 1（无窗口基线，作对照） |
| recoveryState | FAULTED_REVOKED / WAIT_EXPLICIT_RESET / resetSerial=0 |
| bootDiagnostics | pid 36404 attempts=[]（基线空） |
| timelineSnapshotBegin | **2287 行**（T492_IMMUTABLE_TIMELINE_V1，copyNs=143750，dropped=0） |
| executionDiagnosticsV1 | THROW:SyntaxError（参数格式待 T671 对齐，如实记） |
| **体验页原生 boot 导出**（POST_CREATE） | **见下** |

## POST_CREATE 原件核心（t676b-boot2.json，pid 38336）

**recovery 状态机：phase=READY / reason=READY / resetSerial=2 / epoch=4 /
session=2 / runLo=2 / contextLo=4 / resources=true / inCall=false**

**attempt 4（正式 run2）**：terminal=true / **publishedRun=2 / readyEpoch=4** /
K192 / 1256×1406 / firstFailure 全零（无失败标记）。

## 判定

- **READY 已正式发布**（run 2 / epoch 4 / resources=true / publishedRun=2）——
  T678"已进入运行期"判定被本原件**直接证实**；T593 v3 的"READY 永不发布"
  推断作废
- 振荡期状态（GPU_WORK_PENDING/DRAW_PENDING）= 运行期合法忙态（新 root
  冻结旧帧 + 接受回调等呈现），**Down 可过 READY 门**—— SOURCE 链可达性
  恢复，同页真实 Down+typed enqueue/Claim 采证解锁
- inCall=false（无未返回 GL 调用）；dropped=13879（timeline 环形丢弃计数，
  与 rows 4096 容量一致，如实记录）
- probe 页无 XComponent=WAIT_WINDOW 基线；状态采集需体验页原生路径（本报告
  已走通）

## 剩余采证（下轮）

同页真实 Down 的 allowed 十字段/handler/changedTouches + typed enqueue 同 seq
claim + SOURCE 终态 witness + ROOT 接受/present 计数（T678 §B.4/.5）——
端点与预算合同已就绪。
