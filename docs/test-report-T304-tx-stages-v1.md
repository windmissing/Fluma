# T304 事务阶段锚点真机续证（真机，结束待验收）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T303-signed.hap，SHA256 `46b6ebea96f7ade86dad170873ab84582a0280709d65ea0c46b5fb0a33c7927c`（实测一致）。阶段观测 txStage=true（fsync 成本入真实分母）。

## 一、T297 "AVL/GPU hang" 勘误（重大）

T297 报告的 `TX_ADJ_AVL`/`GPU_PAIR_V1` hang **实为同进程复用 Want 未触发新导出**（T297 六组 suite 循环共用进程、仅首批 run 落文件），**非 backend 死锁**。T303 包冷启复测：

| run | backend | steps | 结果 |
|---|---|---|---|
| t304avl1 | TX_ADJ_AVL | 8 | **SEALED / COMMITTED**，10 roots，physicalMs=1445 |
| t304avl32b | TX_ADJ_AVL | 32 | **SEALED / COMMITTED**，34 roots，physicalMs=6520 |
| t304gpu1 | GPU_PAIR_V1 | 32 | **SEALED / COMMITTED**，34 roots，physicalMs=3420 |

同进程二次 Want（t304avl32 无文件无日志）复现 T297 现象——每 run 必须 force-stop 冷启。

## 二、阶段文件独立解析（worker-stages.ndjson，T303_RUNNER_STAGES_V1）

- IDENTITY 行：runId/packageSha/pid/TID/backend/arm/fixture/suite/faultPlan 完整（worker TID 43999 ≠ launcher PID 43489，TID 分列达成）。
- **AVL32b 74 行全部解码**：RECEIVED→34 × (ROOT_ENTER→ROOT_EXIT，每 root 195–372ms 递增)→ARCHIVED→ENC_ENTER→WRITE_FSYNC→ENC_EXIT，**零 firstError、零 observerError**——每个 root 均有 ENTER/EXIT 成对终态，编码/写盘/fsync 阶段全部闭合，无悬挂阶段。
- GPU_PAIR_V1 同构 77 行（34 roots + ARCHIVED/ENC/FSYNC 闭合）。
- fsync 阶段成本：avl32b WRITE_FSYNC 步长 10–29ms，如实计入 physicalMs 分母。

## 三、缺档定向补齐（按 T299 缺口，不重复 1–19 已命中）

| 补档 | 结果 |
|---|---|
| cut20/root3 | SEALED / physicalStatus=**FAULT** / hit=True / error=`ENDPOINT_COMMIT_KEY_MISMATCH`（观察提交保留语义如期） |
| cut5B/root3/ord2 | SEALED / **FAULT** / hit=True / error=`TX_GPU_PHYSICAL_FAILED:5` |
| freeze ON + cut9/root2 | SEALED / **FAULT** / hit=True / error=`TX_GPU_ADMISSION_FAILED`（admission 门在 freeze ON 臂生效，2 roots 后拒） |

## 四、结论

T304 **结束待验收**：三 backend 冷启全 SEALED/COMMITTED（AVL/GPU hang 勘误为 Want 复用问题）、阶段文件 TID 分列且 34 root ENTER/EXIT 全闭合、三项缺档补齐 hit=True。原码/夹具提示：同进程二次 Want 无任何日志与文件（launcher 未重跑），建议 architect 将"每 run 冷启"写入 runner 使用约定。

证据：`test/reports/t304/`（avl1/avl32b/gpu1/cut20/cut5B/frzON 归档 + 3 份 worker-stages + 2 份 launcher-stages）。
