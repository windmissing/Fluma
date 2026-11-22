# T816 v1 · 目标 20 RX SOURCE 根内分段诊断真机验证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T816-rx（T810 bootSource 80ac0d6e 回据核验一致→signed 52bc48e1，
T810_RX_SOURCE_SEGMENTS_V1 writer 交付）。证据：
`test/reports/t491/t816/`（sidecar-terminal.json 25829B +
sidecar-retirement.json 34337B、v1 判定）。

## 执行（单次 Configure，pid 58229，~5.5 分钟无中断）

完整链 → PUMP_FAILED code=5。**sidecar 两 JSON 再次完整采获**（T798
writer 修复稳定）。

## T810_RX_SOURCE_SEGMENTS_V1 分段原件（本轮核心）

时钟：wall CLOCK_MONOTONIC + cpu CLOCK_THREAD_CPUTIME_ID + **gpu null**
（按合同无同步查询）。

- **dispatch8to11**：4 笔 dispatch，每笔含 guard=PRE_DISPATCH、
  pulseSeq、guardNs、cursor a/b（原 opcode 8–11 顺序）✔
- **dispatchBarriers8to11**：4 笔 ✔
- **copies**：arena copy cursor 0 **bytes=591872**（guard PRE_*）✔
- flush/create/wait 顺序在 trace ✔

## firstReject（结构化）

**PUMP_ENTRY / COMMAND_EXPIRED / code=5**：
- deadlineNs 86279627241875、nowNs 86279626444219（差 28.2ms 未到）
- pulseNs 86279621864375 → **pulse age ≈4.6ms**（now−pulse）
- receiptNs 86279375512813 / beginNs 86279377241875

## 合同遵守

250ms 保持；oracle 未动；wall/thread CPU 双时钟逐事件行（T810 固定槽）；
prefix 零提交域不涉（本轮为根内分段）。

## 回执

T765/T785 reader 判定原件 t816/sidecar-*.json；T807 原顺序对照的 T810
分段原件**齐备**。
