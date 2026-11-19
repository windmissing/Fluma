# T593 v3 · 正式 M1 包（T597-fixed）续证——GPU_WORK_PENDING/DRAW_PENDING 振荡原件

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T593-m1-signed.hap（**T664 合同指定工件**：既有正式 M1 T597-fixed，
inputSha 7d01da02→signed 798747a0，sign=PASS；不再误用 T591 M2 采集包）。
证据：`test/reports/t491/t593c/`（l1–l6 状态原件、hilog-t593c.txt）。

## T664 归因修正采纳

v2 报告的两个错误已按 T664 判定修正：
1. **错用工件**：v2 用 T591-collector（M2 采集包）——本 v3 换正式 M1 T597-fixed
2. **错误推断**：v2 的"sample 递增= pulse 活"实为 status 读计数（++a.sampleSerial），
   不证 pulse；本 v3 不再以此推断

## 执行链（正式恢复链路径）

| 步骤 | 状态域 | 观测 |
|---|---|---|
| 冷启 | STALE / run 0:1 / CLOCK_V2 | 正式 V2 时钟域 |
| 新空场（正式 reset） | run 0:2 / **epoch 4 / session 2** / GPU_WORK_PENDING | 恢复链 reset 生效（epoch/serial 前进） |
| 选沙（setSource） | GPU_WORK_PENDING，**物理 0:318→0:536 递增** | GPU 物理线程活 |
| 真实触摸 Down 220ms | 收据恒 #0（零 SOURCE 入队） | down 未达 READY 门 |
| 等 20s | **DRAW_PENDING** | 状态迁移发生 |
| 等 25s | **GPU_WORK_PENDING**（回迁） | 振荡 |

## 判定（按 T664 分类学）

- 正式恢复链的 reset/epoch/session 全部正确前进；物理线程活；但状态在
  **GPU_WORK_PENDING↔DRAW_PENDING 振荡**——创建臂未达 terminal（与 T591 v3
  LINK 40s in-call、T496 崩溃同族：创建/链接域未闭合，READY 永不发布）
- SOURCE 链（typed enqueue→Claim）在 READY 门之前 NOT_REACHED；收据 #0 为
  UI 零读数（T664 已纠正其证据地位）
- 非 STATE_LOST（v2 的旧域判定不适用于正式包）；GPU_WORK_PENDING/DRAW_PENDING
  振荡为正式 M1 的真实终态域

## 回执

按 T664 capture-first 清单已采：包/ELF 身份（签名回执）、状态域序列、epoch/
session/run 迁移、物理计数、触摸组合、收据读数。missingBuildField：恢复链
recoveryState 无独立端点暴露（该域 NOT_AVAILABLE_IN_THIS_BUILD，如实记录非零
值冒充）。回 architect：GPU_WORK_PENDING↔DRAW_PENDING 振荡原件 + T591 v3
LINK 40s + T496 崩溃三证据同域汇聚——创建臂 terminal 缺失为共同根因候选。
