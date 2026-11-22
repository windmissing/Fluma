# T827 v1 · 目标 9 与 10 PC 差分和镜像配对验收准备

日期：2026-11-22，tester，Windows PC。
产物：`test/reports/t827/t827-pc-mirror-ledger.json`。

## 目标 9（T779/T796）与目标 10（T557/T814）判据源登记

| 项 | 来源 | 判据 | PC 镜像状态 |
|---|---|---|---|
| 目标 9 | T779/T796 交付 | 总 q / 来源 q / 图像三类原件 | 未观测（PC 无实际 GPU 执行） |
| 目标 10 | T557 设计+T814 离线审计 | eligiblePairs 配对差分 | T814 offline：**eligiblePairs=0**（device NOT_RUN） |

## T814 现状（如实）

`T814_G10_MIRROR_OFFLINE_V1`：device=NOT_RUN、product=NOT_OBSERVED、
eligiblePairs=0、observedTotalQ/OriginQ/Image 均 null——镜像配对在 PC
无实际 GPU 原件可判（与 T825 结论一致：PC 真实 GPU 接口未形成）。

## 三类原件判据（准备就绪，待 PC 原件）

1. **总 q**：accepted 时间同窗的总量 q 对账
2. **来源 q**：SOURCE/排放来源 q 分项
3. **图像**：同后端/同配置渲染像素差分

## 复用

相变/重放/冷档 fixture 复用登记（t280 replay 归档+t814 离线审计骨架）。

## 回执

验收准备完成（判据框架+fixture 复用登记）；**缺实际 PC 原件——按合同
保未观测**（与 T825/T826 一致，前置=PC 基线 GPU 接口形成）。
