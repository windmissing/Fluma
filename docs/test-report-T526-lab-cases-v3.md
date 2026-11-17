# T526 v3 · 新冷协议资源界剩余 case 真机补证（T538 lab）

日期：2026-11-17，tester，真机（011124264R000337）。
被测：T538-m2-isolated-signed.hap（重装后设备 so 逐位核验）。
证据：`test/reports/t491/t519v2/lab-credit.json`、`lab-cache-fault.json`、`lab-ttl-edge.json`。

## 本轮补证（接 v2 三 case，lab 通道同源）

### DIAGNOSTIC_CREDIT — FINISHED

- DIAGNOSTIC_GATE OK → BUSY（真 gate 的 BUSY 拒）→ COPY SINGLE_CAPTURE（一份副本）
- WRITER ACTUAL_BYTES；两份原件（diagnostic-complete / diagnostic-worst-escape）FILE+SHA256 在档
- CREDIT COMPLETE×3（完整档/少1字节/2MiB clamp 域）→ WORST_ESCAPE_CLAMP=FALLBACK（最坏 escape fallback）
- session 目录物理文件回传 NOT_MEASURED（app 私有域 hdc 不可达；FILE/FILE_SHA256 在档，
  按 T538 "不能只凭 manifest 自证"的边界如实记录——取回通道交 developer/effector 评估）

### CACHE_FAULT — FAIL=CACHE_QUEUE_FULL（预期注入域走完六段）

- ENOSPC WRITE_FAILED → TMP SYNC_REMOVE_INJECTED → RECOVER DRAINED →
  PIN_BOUNDARY PIN_REJECTED → SCAN DIRECTORY_UNAVAILABLE → SCAN_RESTORE RESTORED →
  QUEUE FULL → **FAIL 退出（无 END）**
- 按 T538 口径"错误退出非零、旧原件保留"如实判读：六段注入证据在档（FILE+SHA256×6），
  队列满段 FAIL=终止点（第 7 段是否预期完成域交 developer 确认 record 设计）

### TTL_EDGE_INJECTED — FINISHED

- INJECTED_CLOCK PRE_EDGE / AT_EDGE（注入时钟边界两态）✓
- ID_LIMIT=ID_EXHAUSTED（ID 耗尽显名拒绝）✓
- CLOCK_REVERSE=CLOCK_REVERSED（倒序时钟拒绝）✓
- 第二 fixture TRACE_READY → END COMPLETED_FIXTURE_NOT_PHYSICS

## 累计状态（v2+v3）

| case | 状态 |
|---|---|
| TRACE_MAX_SOURCE / TRACE_MAX_Q（四档） | FINISHED（v2） |
| TWO_SLOT_PIN_REPLACE | FINISHED（v2） |
| TTL_WALL | FINISHED（v2，60.714s 实测） |
| DIAGNOSTIC_CREDIT | FINISHED（本轮） |
| TTL_EDGE_INJECTED | FINISHED（本轮） |
| CACHE_FAULT | FAIL=CACHE_QUEUE_FULL（本轮，六段证据在档） |
| DIAGNOSTIC_BUSY_FAIL | NOT_RUN（本轮时间预算外） |

FIXTURE_SYNTHETIC 与物理域分列保持；原件故障无改写 PASS。
