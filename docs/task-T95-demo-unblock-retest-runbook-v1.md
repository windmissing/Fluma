# T95 DEMO 物理阻断后复测执行案

日期：2026-10-27。责任：effector。任务卡：6adf1e5ee4b017ab3402929f。  
基线：worktree @ `a419f5a`（含 T166 修复 `1f8e16c` + T167 独立 46/46），树指纹 `aee7374caa8ff7cb`，HAP s150 `f28ec28e…`（已重装主工程包，覆盖 GPU 隔离测试包）。  
状态：**待执行**。

## 0. 目标（任务卡三项缺口）

1. **J2 设备两臂**：T82 触发序列（拖动注入+倾斜沉降）在 T166 修复版上 **code5 不再复现**（对照 T82 的 FAIL 与 T103/T119 的机制定位）。
2. **LAND 长窗**：粒子准入后 **≥30s 静置**，depositedMass 时间序列收敛（T82 时 3s 窗全 airborne 未落床）。
3. **真实手持重力**：手持倾斜方向正确性（T82 R4 未完成项；主观 0/1 记录，不做强度标定）。

## 1. 场景矩阵

| 臂 | 操作 | 时长 | 判定 |
|---|---|---|---|
| R0 | 冷启冒烟 | 15s | 页面活 |
| R1 | J2 复现臂：4×swipe 拖动注入（T82 R2 同款）+ 45s 静置 | ~90s | **code5 零复现** + LAND 序列（deposited 收敛）+ 截图×5 |
| R2 | J2 加压臂：6×swipe（更大注入量）+ 45s 静置 | ~100s | 同上（更大库存下的 code5 零复现） |
| R3 | 手持重力（手动窗口） | ~60s | 方向 0/1 + 倾角诊断记录 |
| R4 | finish → CLOSED → 双流拉取 → manifest | 收尾 | 完整落盘 |

每臂 PID 隔离判读（T124 勘误纪律）；freeze 关键词全文搜索；动作与截图交错。

## 2. 判据

| # | 判据 | 通过 |
|---|---|---|
| J2-1 | code5 复现 | `physics-failed`/`getLastError=5`/first_code5_trace 零出现（R1/R2 全程） |
| J2-2 | code5 未复现时物理正常 | dry.request/receipt 正常、守恒 true、commit 单调 |
| L1 | 落床序列 | depositedMass 随时间非降；末段 ≥10s 内增量 < 总注入 5%（收敛） |
| L2 | LAND 语义 | 粒子 fromState FLYING→CONTACT_WAIT→准入 mh/ml（ledger/particle events） |
| H1 | 手持方向 | 沙向低端流动（0/1 + 截图/照片） |
| S1 | 稳定性 | 无 THREAD_BLOCK/appfreeze（PID 隔离） |
| D1 | code5 若现 | 按 T151 机制记 invalid 交 T146 线（不应出现——修复已对齐口径） |

## 3. 工具与交付

`tools/t95_demo_unblock_retest.ps1` + `tools/analyze_t95.py`；产物 `test/artifacts/t95-demo-unblock-retest-20261027/`；回执 `docs/effect-report-P051-T95-demo-unblock-retest-v1.md`（对 T82 NOT PASS 对比）；滴答卡评论直发。
