# T526 · GPU 新冷协议资源界真机验证 tester 报告 v1

- 卡：`6afa6b4be4b0711ec49384d3`（tester，P5，真机，设备串行 62T0225B10010020）
- 日期：2026-11-16
- 被测包：**T507-m2-complete** `b702032f6fb34c2818fe…`（本地逐位核验），bm clean 后冷启，PID 60647
- 路由：`--ps grsm1 false` → 首页"开启独立 GPU M2 V3 空场"（真实 bounds [254,1414][1027,1554]）→
  GPU M2 V3·V2 页（T452_M2_EXPERIMENT_V3）
- 证据：`test/reports/t491/t526/`（真机 trace 3 页+manifest、mem 存档、UI dump、截图）

## 冷协议资源界真机实际行为 → PASS（核心）

**导出有界冷 trace（新协议 T503_M2_COLD_TRACE_V1）真机实测**：

| 界 | 合同值 | 真机实测 | 判定 |
|---|---|---|---|
| 单页 ≤16KiB | 16,384B | 909/3,276/2,956…3,102B（9 页全部） | PASS |
| manifest | T503_M2_COLD_ARCHIVE_V1 | `state=COMPLETE, reason=STREAMED_COMPLETE`、9 files、24,786B、7,680 words | PASS |
| scope 身份 | pid/route/source/control | `pid=60647, route=m2, source=6cfdcc5c…+T492-overlay-v1, epoch4/run2/session1` 逐字回显 | PASS |
| rejectedPage | 无拒绝页 | None | PASS |
| 页字段 | page/pages/actualWords/stateWords/eventWords | page=0..8/8、actualWords=7,680、stateWords=7,424、eventWords=0（QUANTUM 空场零事件如实） | PASS |
| SHA | 每 file sha256 | manifest 9 项逐页哈希在档 | PASS |

**新协议在真机可用且各资源界被遵守**——T507 离线合同在设备上成立（首次真机证据）。

## 相位 RSS 采样

| 相位 | VmRSS (kB) |
|---|---|
| P1 首页冷启后 | 103,168 |
| P2 进 M2 V3 页（编译后） | 508,720 |
| P3 新空场后 | 572,820 |
| P4 冷 trace 导出后 | 361,636（导出释放后回落 ~211MB） |

hidumper Total（P4）：286,688 kB。导出后大幅回落符合"finally 释放 snapshot/trace"合同。

## NOT_MEASURED（如实）

- SOURCE/有效 Swap 相位：M2 V3 run 状态 STATE_LOST WAIT_RETIRED（与 T461 v1 同域）——
  满载 trace（大 payload 页逼近 16KiB 上限）/诊断 2MiB/两 slot 累计 55,410,688 的压力域
  在 CURRENT 域未达成，无法在真机施压验证上限行为（仅空场小页通过）。
- 诊断 writer 2MiB 边界、两 slot 并发、TTL 过期：页面无独立触发入口，NOT_MEASURED。
- M1 路由 trace：明确 UNSUPPORTED_ROUTE（T507 合同），未在本轮装 M1 包重复验证。

## 判定

- 冷协议真机基本行为（页界/scope 身份/manifest 完整性/释放回落）：**PASS**
- 压力域（上限逼近/slot 并发/TTL）：**NOT_MEASURED**（CURRENT 域未达成，回 architect 排期）
