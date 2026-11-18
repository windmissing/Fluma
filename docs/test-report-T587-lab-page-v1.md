# T587 v1 · T531 lab 排空与原文件分页真机复测第一段（T579-lab 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579-lab-signed.hap（T565/T562 修复消费包），bm clean 后冷启，PID 33264。
路由：首页 → 开启独立 GPU M2 V3 空场 → **独立资源Lab**（T531_RESOURCE_LAB_V1）→ 启动。
证据：`test/reports/t491/t587a/`（lab-record.json 为 UI 导出原件）。

## 结果：lab 序列 FINISHED（约 40 分钟，TRACE_MAX_SOURCE 四档）

`resource-lab-1-1795006999726.json`（UI 导出，sha256=43a9588091568a64990f5cd9c1b0f7440675）：

- **state=FINISHED，outcome=COMPLETED_FIXTURE_NOT_PHYSICS**，records=30，dropped=0
- 四档 TRACE_MAX_SOURCE 全走完：每档 TRACE_BEGIN→REQUESTED→TRACE_FINISH→
  **TRACE_READY**→FIXTURE_SHA256（四档各不同：5e846956…/2edd5384…/f8be0275…/
  2fba9968…）→FIXTURE_RECIPE（i*2654435761u; header T445 V3 synthetic）→
  **PAGE_ACQUIRE=OK → PAGE_WRITER=ENCODED_SYNTHETIC → PAGE_SHA_PREFIX=SHA256**
- **PAGE_WRITER=ENCODED_SYNTHETIC 四档全 OK**——T587 域"原文件分页"核心：writer
  编码-落页链在修复包下四档全部成功，无 CACHE_QUEUE_FULL（对比 T526 v3 的
  CACHE_FAULT FAIL 原件）
- FIXEDTURE_SYNTHETIC 与物理域分列保持（physicsValidated=false 如实）

## 观察与如实记录

- UI records 域在运行期间冻结于 START（拉取窗口内无增量），最终 record 集经
  "导出"按钮全量落盘取回——UI 增量刷新滞后不影响原件完整性
- TRACE_MAX_SOURCE 单档耗时长（总约 40 分钟），屏锁期间运行未中断
  （power-shell timeout -o 600000 保持）
- T587 卡面的"fence/5s 取消/终态修复 case、BUSY、FilesV2 begin/index/page/end
  四对 FILE/SHA 实取回、stat 变化/过期/边界/reader 信用"域：本轮完成 PAGE 层
  四档全 OK 首段；fence/cancel/BUSY 注入 case 与四对 FILE/SHA 实取回 NOT_RUN
  （lab record 为 fixture 合成域，注入 case 需独立 lab job——后续轮续做）

## 判定

- **PAGE 分页/排空链真机复测 PASS**（四档 ENCODED_SYNTHETIC 全 OK、无 QUEUE_FULL、
  COMPLETED_FIXTURE 终态）
- T526 v3 的 CACHE_FAULT（CACHE_QUEUE_FULL）在本包下未复现——修复消费验证首段 PASS
- 剩余 case（fence/5s cancel/终态/BUSY/FilesV2 实取回）留 T587 后续轮
