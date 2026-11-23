# T879 目标13 同机 full 与 hot 实际 GPU 分段对照

- 任务：T879 目标13 同机 full/hot 实际 GPU 分段对照
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t877-release.exe`（SHA256 `ce824652d3d24408b50a55f0de1522fd6eefd63cbaa1fa5f603f35451c66e7f1`）
- 机器：Intel UHD Graphics 630（deviceUUID `8680983e-…`，驱动 31.0.101.2140）——与 T868 基线同一 iGPU，未混入独显（沿 T878）
- 冻结输入：T868 的 F02/F03/F05 config/state 原件（同 SHA）
- 六臂交错：F02/F03/F05 × full/hot；250ms 与原数值门全不放宽；GPU 驻留、无 Canvas 回退

## 六臂结果总表

| 臂 | op65 | 首退出原因 | 业务等待 | 退役 | 设备排空 | SOURCE | GPU 时标 |
|---|---|---|---|---|---|---|---|
| f02-full | QUALIFIED | **GRANT_DEADLINE** | 245.0ms/16 轮 | vk=2 | 1207ms | — | QUANTUM 未信号 |
| f02-hot | QUALIFIED | GRANT_DEADLINE | 249.8ms/16 轮 | vk=2 | 1182ms | — | QUANTUM 未信号 |
| f03-full | QUALIFIED | GRANT_DEADLINE | 252.1ms/16 轮 | vk=2 | 1016ms | — | QUANTUM 未信号 |
| f03-hot | QUALIFIED | GRANT_DEADLINE | 253.9ms/16 轮 | vk=2 | 1298ms | — | QUANTUM 未信号 |
| f05-full | QUALIFIED | GRANT_DEADLINE | 249.2ms/16 轮 | vk=2 | 1590ms | **受理+V2 档 1792768B** | SOURCE **READ_AFTER_FENCE** + QUANTUM 未信号 |
| f05-hot | QUALIFIED | GRANT_DEADLINE | 255.0ms/16 轮 | vk=2 | 1589ms | **受理+V2 档** | SOURCE **READ_AFTER_FENCE** + QUANTUM 未信号 |

## 核心发现

### 1. T873 归因问题已回答

六臂一致：**首退出原因均为 GRANT_DEADLINE**（非脉冲/窗口撤权）。业务等待以 16 次 2ms 有界短轮询耗尽 250ms 全额；退役等待 250ms 仍 vk=2；随后的 `vkDeviceWaitIdle`（T877_DEVICE_DRAIN）耗时 **1016–1590ms**——这是 850 派发 QUANTUM 根在此 iGPU 上的真实 GPU 执行时间。**根是 GPU 执行受限，超预算 4–6 倍**；脉冲全程健康（age 1.5–29.5ms，最大 callback gap 30.8ms < 33ms 限）。

### 2. SOURCE 与 QUANTUM 的 GPU 时间对照（F05 独有）

F05 两臂的 SOURCE 根（16 派发）GPU 时标 **READ_AFTER_FENCE**：实际 GPU 时间 **51.7ms（full）/ 58.1ms（hot）**——在 250ms 预算内，fence 正常信号、V2 档（`PC_G13_T833_RAW_V2`，1792768 B，七段含 PACK0/PACK1）正常落盘。QUANTUM 根（850 派发）所有臂 FENCE_NOT_SIGNALED。**预算对 SOURCE 级根充足，唯 850 派发根超限**。

### 3. full 与 hot 对照

recordMs：full 0.65–5.87ms / hot 0.60–0.67ms（f02-full 首次路径 5.87ms）。业务等待、首退出、退役、排空在两模式统计无差（drain：1207/1182、1016/1298、1590/1589ms）。**hot 模式缩减 fence 内取证（3376 B）不改变 GPU 执行界**——T873 建议 3 的证据搬移优化在此 iGPU 上不是瓶颈，compute 派发本身主导。

### 4. 可达域

- F02/F03/F05 准入（op65）六臂全 QUALIFIED
- F05：SOURCE 受理+V2 档+GPU 时标可读（10/10 点位含 UPLOAD/ACTUAL/REPLAY/COMPARE/PACK/BEFORE_TAIL）
- F02（私有投影）/F03（周期剪切）：量子首路由到达提交，无数值域结果（iGPU 250ms 界）
- GPU 时标能力：36 valid bits、83.333ns period（查询能力在位）

## 证据

- `test/reports/t879/`：六臂 JSONL + F05 V2 source 档 ×2 + summary.json
