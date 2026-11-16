# T511 · GPU 资源 ledger 与缓存残留独立差分 tester 报告 v1

- 卡：`6af8605ce4b010f489353c05`（tester，P5，不使用真机）
- 日期：2026-11-16
- 被测：T501（34368a2 资源 ledger/退出回执）+ T502（fc93f4d 缓存临时预算）实际交付档
- 消费原件：`test/reports/t502/delivery.json`、`store-metrics.ndjson`(13行)、`seed/restart-metrics.ndjson`、
  `prepared-m1/m2-cold/warm.json`（T501_OWNER_RESOURCE_LEDGER_V1 四档）、`test/reports/t501/*`
- 方法：不信任 developer 断言，从原始 ndjson/JSON 独立复算（判据先行，结果后对）

## 独立差分结果 11/11 PASS

| ID | 判据 | 独立复算 |
|---|---|---|
| D1 | 128MiB/32key 预算 | **写前拒入门限语义**：payload+temp 超 128MiB 的 2 行（128MiB+512B / +1B）全部 diskRoomRejected>0（新写拒绝生效 7 次）；残留本身如实计账不虚减 |
| D2 | remove 失败残留 | tempBytes 峰 134,217,729（128MiB+1B）/tempFiles=1/cleanupFailures=144、lastCleanupError=13（Permission denied）——残留参与预算且不可 truncate 逃账 ✓ |
| D3 | 高水位语义 | diskPeakObserved ≥ 每次 diskBytesLastReservation 全部成立（0..134,217,729） |
| D4 | 4×32MiB pin | pinnedArtifacts 峰=4 在账 |
| D5 | 启动收敛 | 独立进程 seed temp=147B → restart temp=0（清理收敛）✓ |
| D6 | ledger 账自洽 | **liveBytes==Σ objects(requestedBytes, state=live) 四档全 MATCH**：M1 cold/warm 37,728,720（24 objects）、M2 38,388,176（27 objects）——与 T501 文档请求容量逐字一致 |
| D7 | warm/cold 差分 | creates 差 6=3 program+3 shader（BINARY_LOAD 跳过编译，与 T502 warmCompileCalls=0 一致）；deleteRequests 同差 6；liveBytes 相等（同资源集合） |
| D8 | 对象账 | M1 kind 分布 5 program + 17 buffer + 2 其它；droppedEvents=0、incomplete=0（有界容量未溢出） |
| D9 | 接线一致 | resourceState.cache 与 cacheStorageState **同一实账**：`Stats501(){return Diagnostics502();}`（artifact_store.h:580，同一 Store() 单例）——T502 串行接线 ✓ |
| D10 | 不冒物理回收 | glHandleValidity=`NOT_INFERRED_FROM_ARTIFACT_OR_RECEIPT`（全部行）；driverAccounting=`DRIVER_NOT_MEASURED`；gpu=LOGICAL_REQUEST、host=HOST_ALLOCATED_CAPACITY 分账明确 ✓ |
| D11 | 扫描失败域 | diskScanFailed 全 False（启动扫描失败拒存路径为负控域，本轮未触发——如实标注非隐漏） |

## 语义澄清（初版判据修正，不放宽）

D1 初版把"payload+temp≤128MiB"当硬上限判 FAIL；读 T502 合同后修正：预算门限作用于**新写预留**
（残留计入预算使新写被拒），残留本身是故障注入的真实字节（32MiB 系列产物），合同要求"参与预算、
不可原位 truncate 逃账、写前拒绝"——三项全部实测成立。残留可暂时超出门限值不是违例（无物理
删除能力是故障注入前提），虚减或拒绝失效才是。修正后判定不改判定标准，是读对合同。

## 判定域分列（卡面要求）

- **离线 store/ledger 会计**：PASS（上表 11 项）
- **真实 GL 故障注入 vs 会计域**：T502 graphics 13+3 组（remove 失败/queue 满/pin 满下合法
  program 仍可 use）为 developer 侧执行，本卡从 delivery.json graphics 域复读确认结构在位；
  独立重放需 ANGLE 环境重跑 build_graphics.cjs——与 T504 长测独立，本轮未重跑（记录为复用域）
- **真机/driver/RSS**：NOT_MEASURED（合同明确不由离线计数推断）
