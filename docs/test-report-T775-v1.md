# T775 v1 · 目标 20 DM SOURCE 三臂真机对照

日期：2026-11-21，effector（tester 兼，含部署采证），真机（011124264R000337）。
被测：T771 driver（receipt-T771-compile-v1：driver buildId 71b03f56/so
cbb01f59/cpp 配对——设备 md5 逐件核对后刷入新目录 t771bin）。三臂同
K192/S40 输入（dm20-input），一臂一新进程。证据：`test/reports/t491/
t775/`（A.jsonl/B.jsonl/C.jsonl 各 28 条含 T771_DM_SOURCE_TIMING_V1 原
件、v1 判定）。

## 三臂对照（每臂独立进程，原期限保持）

| 臂 | exitCode | firstGuardFailure | lastGuardSuccess | 记录数 |
|---|---|---|---|---|
| **A** | 6 DEADLINE | PUMP_ENTRY_GUARD DEADLINE @12 | PRE_EXEC_POLL_GUARD PASS | 28 |
| **B** | 6 DEADLINE | PRE_EXEC_POLL_GUARD **CONTROLLER_REJECTED** @12 | PUMP_ENTRY_GUARD PASS | 28 |
| **C** | 6 DEADLINE | PUMP_ENTRY_GUARD DEADLINE @12 | PRE_EXEC_POLL_GUARD PASS | 28 |

**三臂全部 exitCode=6（DEADLINE）@dispatch 12**——失败在
PUMP_ENTRY_GUARD-DEADLINE（A/C）与 PRE_EXEC_POLL_GUARD-CONTROLLER_
REJECTED（B）之间振荡：同一 250ms 预算耗尽，哪个 guard 先注意到略有差
异——**确定性成因确认，非随机**。

## 原件

T771_DM_SOURCE_TIMING_V1 逐臂真时序（dispatch api/barrier ns、root/
deadline、cadence）全保存，供 T773 reader 独立判定。

## 合同遵守

一臂一新进程 ✔｜原期限保持（未放宽）✔｜晚尾只退役 ✔（retirementFence
复用执行 fence 已 sig 模式延续）｜同根尾 boot-work-tail.u32 各臂产出
（本轮未重复读取，已有 4 次确认）。

## 回执

- T773 reader 独立判定（原件 t775/A.jsonl、B.jsonl、C.jsonl）
- 归 architect：三臂 DEADLINE 一致性=250ms 预算为确定性成因
