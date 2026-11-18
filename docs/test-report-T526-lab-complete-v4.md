# T526 v4 · T538 lab 8/8 收口（DIAGNOSTIC_BUSY_FAIL 真机首证）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T538-m2-isolated-signed.hap（重装后设备在位）。
证据：`test/reports/t491/t519v2/lab-busy-fail.json`。

## DIAGNOSTIC_BUSY_FAIL — FINISHED（29 阶段，本轮）

- 真gate BUSY 拒（DIAGNOSTIC_GATE OK→GATE_BUSY）→ SINGLE_CAPTURE 一份副本 →
  WRITER ACTUAL_BYTES → 完整档/worst-escape 两原件 FILE+SHA256 → CREDIT COMPLETE×3 →
  WORST_ESCAPE_CLAMP=FALLBACK
- **五类注入出口逐一带原件**：INJECTED_FAILURE = ALLOCATION_FAILED / NAPI_FAILED /
  IO_FAILED / CANCELLED / FINAL_MANIFEST_FAILED，各配 FILE+FILE_SHA256（5 份原件在档）
- END COMPLETED_FIXTURE_NOT_PHYSICS——按 T538 口径：故障注入原件，不称设备
  allocator/系统调用真失败

## T538 lab 全 case 终态（v2+v3+v4 累计）

| case | 状态 |
|---|---|
| TRACE_MAX_SOURCE（四档） | FINISHED（v2） |
| TRACE_MAX_Q（四档） | FINISHED（v2） |
| TWO_SLOT_PIN_REPLACE | FINISHED（v2） |
| TTL_WALL | FINISHED（v2，60.714s 实测） |
| DIAGNOSTIC_CREDIT | FINISHED（v3） |
| TTL_EDGE_INJECTED | FINISHED（v3） |
| CACHE_FAULT | FAIL=CACHE_QUEUE_FULL（v3，六段证据在档） |
| **DIAGNOSTIC_BUSY_FAIL** | **FINISHED（本轮）** |

**8/8 case 全部执行完毕**——T526 卡面 lab 域收口。唯一非 FINISHED 项 CACHE_FAULT 的
FAIL 切点（队列满段设计意图）维持交 developer 确认（v3 已登记）。

## 维持口径

- FIXTURE_SYNTHETIC 与物理域分列；注入/合成内容不冒真实物理/设备失败
- session 目录物理文件 hdc 不可达（app 私有域）——FILE/FILE_SHA256 在档，回传 NOT_MEASURED
- driverAccounting=DRIVER_NOT_MEASURED 等如实字段保持（v2 生产采样域）
