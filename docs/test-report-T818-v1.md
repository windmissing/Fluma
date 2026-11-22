# T818 v1 · 目标 20 DM replay 与 compare 前序排队真机取证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T797 driver（09befda4，设备 md5 2855f567 核对；输入
dm20-input/config.u32 设备 md5 2549e70e）。证据：
`test/reports/t491/t818/`（y-op8/y-op9/y-B.jsonl、v1 判定）。

## replay 与 compare 三臂

| 臂 | 记录 | exitCode | cursor | retire fence | polls |
|---|---|---|---|---|---|
| **replay OP8** | 29 | 0 | **9**/12 | signal | 159 |
| **replay OP9** | 29 | 0 | **10**/12 | signal | 189 |
| **compare B** | 28 | **6 DEADLINE** | 12/12 | signal | 67 |

- replay 臂 = 独立 prefix 运行（GPU clock UNOBSERVED→合同降级 prefix
  fence），cursor 与 T804/T809 基线逐匹
- compare B = 公共提交臂（root→fence wall span 由逐事件 ns 原件可算），
  DEADLINE @12 一致
- **原 wait 值与区间**：execution/retirement fence 对象完整在原件；
  wall/thread CPU 双时钟 ✔

## 合同遵守

- 250ms 保持（B DEADLINE @12 一致；prefix 零 commit）
- 原 oracle 未动
- **不以 poll 数推成本**——只报原始 wait/区间
- 复制专用补组未跑（按卡面"暂不跑复制专用补组"）

## 回执

T773 reader 判定原件 t818/y-*.jsonl；完整 argv 在原件（deviceExecution
域）。
