# T435 · GPU 来源前缀独立边界 oracle 报告

2026-11-09，tester，P5，不使用真机。依 T433 §7 A-T 冻结"每 QUANTUM 复制有效 sourceEvent16 前缀"的边界输入与预期，不等 T434 实现。

## 交付

- `test/t435/prefix_oracle.cjs` — 独立 oracle（CLI `node test/t435/prefix_oracle.cjs [exec.log]`，exit 0/1）
- `test/reports/t435/t435_oracle.json` — 判定原件（verdict PASS，23 entries）
- `test/reports/t435/t435_exec.log` — T280 真实 GL host（ANGLE/SwiftShader）复跑基线：465/0，证明执行环境有效

## 独立期望模型（不复制产品算法）

从 T433 §7 合同文本与 wire 布局独立推导：

| 有效前缀 n | 期望 prefix copy | 说明 |
|---|---|---|
| 0 | 0 B | `count&&prefix!=events` 门跳过，毒值区保持 |
| 1 | 256 B | 最小有效前缀 |
| 4095 | 1,048,320 B | 接近满 |
| 4096 | 1,048,576 B | 满前缀，省量 0（T433 §7 明示） |

ledger 固定复制边 262,144 B 本卡不动（与 T433 analysis JSON `sourceLedgerGpuCopyBytesPerQuantum` 逐位核对一致）；perRow 256 B 与 `gpuEventPrefixCopyBytesPerRow` 一致。

## 场景矩阵 17 项（T433 §7 A-T 边界清单逐项冻结）

n=0/1/4095/4096、DIRECT_W（不占 water slot 但入来源账）、部分准入（拒量入前缀）、SOURCE 拒绝不改 ledger 边、同 SOURCE 连续 QUANTUM（续帧 used[a] 路径）、停源在途 J→W（尾拒绝 header/count 不变）、LAND 到达入前缀、capacity/ID 耗尽精准拒绝、旧 run/workspace 身份拒绝、非法 origin 原切点失败、错 n>4096 checked 拒绝、后缀毒值 [n,4096) 保持 0xDEADBEEF。

## 预期 RED 登记（2 项，交 T436 消费）

- **S16_SUFFIX_POISON**：T434 实现后 [n,4096) 毒值必须保持——当前产品整区复制，按预期 RED 登记
- **S17_PREFIX_BYTES_REDUCED**：T434 后 prefix copy 应为 512B（n=2）——当前产品未实现按预期 RED

## 判读结构

E1 基线可复跑（465/0）→ E2/E3 oracle 数学与 T433 JSON 逐位一致 → E4 TRACE 结构有效 → E5 17 场景冻结 → E6 EXPECT_RED 正确登记（不计产品缺陷）→ E7 FAULT 14 模式原切点拒绝前提证据。

## 边界

- 本卡只冻结独立可复跑 oracle 与期望；**实际执行差分归 T436**（消费本 oracle 对比 T434 实现），不把模型 PASS 冒产品通过
- 文件归 test/t435，避开产品实现与 T410 设备窗口
- GPU 计时不在范围（copy bytes 不推算耗时，T433 §7 明示）
