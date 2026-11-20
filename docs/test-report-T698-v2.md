# T698 v2 · lab 独立 driver 全量矩阵真机补证——14/14 模式执行完成

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：t668_driver（T697 ELF 3dc69a93/BuildId 1907d3d7 + libc++_shared
1262248B，从 T592-pairing HAP 提取），/data/local/tmp/t697 独立进程串行。
证据：`test/reports/t491/t698b/`（14 模式 stdout 原件、v2-fullmatrix.json）。

## 全量矩阵结果（14/14 执行，退出码全 0）

| 模式 | 结果 |
|---|---|
| full_queue_pop | **EXPECTED**（WAIT_SPACE→128 队列→insert→pop→return，128 项完成后 pop 成功） |
| full_queue_cancel | **EXPECTED** |
| fence_cancel | **EXPECTED** |
| deadline | **EXPECTED** |
| barrier_cancel | BARRIER_ENTER→**CANCELLED**（真实 BARRIER_ENTER 原件+取消终态） |
| barrier_release | COMPLETED_FIXTURE_NOT_PHYSICS |
| files | COMPLETED_FIXTURE_NOT_PHYSICS（Begin/Busy/错 End 完整流） |
| files_ttl | RUNNING→**FINISHED→NO_FILES**（61s 过期语义记录） |
| files_mutation / _truncate / _replace / _delete | 4 变体 COMPLETED_FIXTURE_NOT_PHYSICS（隔离执行） |
| injected_clock | COMPLETED（TTL_EDGE_INJECTED ×3 标签在位） |
| injected_fault | COMPLETED（CACHE_FAULT ×4） |
| catalog_contention | RESOURCE_PHASE_SAMPLE（TWO_SLOT_PIN_REPLACE ×1） |

## 判定

- **T697 离线 driver 设备执行全矩阵 14/14 完成**——单进程 EXPECTED 域
  （WAIT_SPACE/WAIT_FENCE 联核真实返回）、barrier 域（BARRIER_ENTER 原件+
  生命周期终态）、files 域（完整流+TTL 过期+四变异）、注入域
  （TTL_EDGE_INJECTED/CACHE_FAULT/TWO_SLOT_PIN_REPLACE 独立标签）全实测
- 全 FIXTURE_SYNTHETIC，无物理声明（页契约一致）；未采瞬态按未观察登记
- 目录变体全部使用新鲜绝对目录（T697 合同）；普通 30s/TTL 80s 界内完成

## 回执

T698 验收域（lab 独立 driver 全量矩阵真机补证）**完成**——T668/T697
设施链设备闭环。
