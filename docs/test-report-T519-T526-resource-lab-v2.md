# T519/T526 · 资源账与缓存基线 + 新冷协议资源界（T538 lab 签包域）v2

日期：2026-11-17，tester，真机（011124264R000337），设备串行。
被测：T538-m2-isolated-signed.hap（签包收据 test/reports/t257/T538-m2-isolated.json）。
依据：T531 纠偏（544d263）+ T538 开发交接（T531_RESOURCE_LAB_V1）。
证据：`test/reports/t491/t519v2/`。

## T519 域（生产只读快照，纠正身份后首次真机原件）

- **`T531_RESOURCE_SAMPLE_V1` COMPLETE**（此前 T519 只有 SHA 误声明/无原件——本轮为
  **首个真机 COMPLETE 采样原件**）：
  - resource.json：liveBytes=0、retiringBytes=0、logicalPeak=0、hostPeak=6,328,864、
    gpuAccounting=LOGICAL_REQUEST、hostAccounting=HOST_ALLOCATED_CAPACITY
  - cache.json：entries=0、pinned=0、payloadSize=0、diskPeakObserved=3,551（空场缓存域）
  - manifest：driverAccounting=**DRIVER_NOT_MEASURED**（如实）、packageReceipt/elfReceipt=
    NOT_MEASURED（安装/ELF 证据未取）、nativeBeginRaw 带 pid/route/scope/STEADY_NS 原文
  - RSS（/proc VmRSS）：296,452 kB 系统分账在档（不冒 driver 释放——T531 纠偏口径）
- 不重跑已证空场同证（T531 口径）；生产只读通道 WORKS 在真机成立。

## T526 域（T538 lab 三 case 真机首证）

### TRACE_MAX_SOURCE（四 profile 全 FINISHED）

- 四档 {192,64}/{192,128}/{512,64}/{512,128} trace 全部 TRACE_READY：
  223,744 / 227,840 / 571,904 / 576,000 B（真实槽 span 原位填、真实 Layout/Finish/Page/Writer）
- 逐档 FIXTURE_SHA256 + recipe（i*2654435761u; header T445 V3 synthetic）+ PAGE_SHA_PREFIX 在档
- **合成内容明确不宣真实物理 trace 通过**（T538 交接原句，如实标注 FIXTURE_NOT_PHYSICS）

### TWO_SLOT_PIN_REPLACE（FINISHED）

- 失败候选（INJECTED_BUILD_FAILED）**保旧 published=TRACE_READY** ✓
- READER_BARRIER OK ✓；第二替换 **WOULD_BLOCK 拒** ✓
- END_PINNED=RETIRED_READER_STILL_OWNS → 显式释放屏障 → READER_RELEASE=**FREE_RETAINS_CAPACITY** ✓
- 全链（失败保旧/pin/拒/退休/释放保留容量）逐段 record 在档（resource-lab-two-slot-final.json）

### TTL_WALL（真实 60s 墙钟，FINISHED）

- TRACE_FINISH→TTL_EXPIRED = **60.714 s**（60s TTL 原始发现延迟实测）
- 61 次 TTL_POLL_NO_RENEW 全 OK（Acquire/Release 不续期验证）→ 最后 EXPIRED ✓
- PAGE_ACQUIRE EXPIRED、END COMPLETED_FIXTURE_NOT_PHYSICS

## NOT_MEASURED / 边界

- DRIVER_NOT_MEASURED：卡面明确 driver 永远不冒——保持
- TTL_EDGE_INJECTED/DIAGNOSTIC_CREDIT/DIAGNOSTIC_BUSY_FAIL/CACHE_FAULT：本轮未跑
  （设备时间预算内完成三主力 case；注入/边界类 case 独立轮次，同 lab 通道无前置）
- 压力域（多 session 并发）：待 T538 压力 lab 复测（T531 口径，不重复空场同证）

## 判定

- T519：真机 COMPLETE 采样原件首次取得（纠正身份后）→ 看评论待验收判定
- T526：新冷协议资源界合成域三 case 首证（四档 trace 尺寸/双槽替换/60s TTL 全链在档）
  → 看评论待验收判定
- 原件故障无改写 PASS；FIXTURE_SYNTHETIC 与物理域分列全保持
