# T801 v1 · 目标 20 DM SOURCE GPU 分段成本真机取证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T797 driver（buildId 09befda4，设备 md5 2855f567 核对，so
b0df4529，cpp 配对）。**五组交错有效样本**（s1–s5，--timing=B 显式，
各新进程，同 K192/S40 输入）。证据：`test/reports/t491/t801/`
（s1.jsonl–s5.jsonl 各 28 条含 T758 trace+TIMING 原件、v1 判定）。

## 五样本汇总（5/5 有效）

| 样本 | arm | 记录 | exitCode | firstGuardFailure | execFence | retireFence |
|---|---|---|---|---|---|---|
| s1 | B | 28 | 6 DEADLINE | PUMP_ENTRY @12 | 未 sig | 复用已 sig |
| s2 | B | 28 | 6 DEADLINE | PUMP_ENTRY @12 | 未 sig | 复用已 sig |
| s3 | B | 28 | 6 DEADLINE | PUMP_ENTRY @12 | 未 sig | 复用已 sig |
| s4 | B | 28 | 6 DEADLINE | PUMP_ENTRY @12 | 未 sig | 复用已 sig |
| s5 | B | 28 | 6 DEADLINE | PUMP_ENTRY @12 | 未 sig | 复用已 sig |

**GPU timestamp UNOBSERVED**（回执 gpuClock=UNOBSERVED）→按合同降级四
独立 prefix fence（原件含逐事件 ns+drain 窗口）。

## 分段成本原件

TIMING schema 含 dispatch api/barrier ns 数组、drainBegin/End（每样本
drain 窗 ~72ms）、events 数组（SOURCE_BUILD→CACHE_LOOKUP→SHADER_
COMPILE→PROGRAM_LINK→REFLECT→BEGIN_ROOT→CONTROLLER_CHECK→...）——
**专属 copy 与前序排队归属可从逐事件 ns 定位**。

prefix 零 commit：五样本全 commit=0/quanta=0。原 250ms 保持。

## 回执

T773 reader 判定原件 t801/s1–s5.jsonl；五样本一致性锁定 DEADLINE@12
判定并附逐段时序——归 architect。
