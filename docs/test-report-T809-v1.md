# T809 v1 · 目标 20 DM 五组交错前缀成本真机补证

日期：2026-11-22，effector，真机（011124264R000337）。
被测：T797 driver（buildId 09befda4，设备在位 md5 已核）。**五组交错**
（B/OP8/OP9/COPIES/OP11 各一新进程，--timing=B 显式，同 K192/S40 输
入）。证据：`test/reports/t491/t809/`（x-B/x-op8/x-op9/x-copies/x-op11
.jsonl 各 28/29 条、v1 判定）。

## 五组交错汇总（有效分母 = 5）

| 臂 | 记录 | exitCode | cursor | retire fence polls |
|---|---|---|---|---|
| **B** | 28 | 6 DEADLINE | 12/12 | 78 |
| **OP8** | 29 | 0 | **9**/12 | 159 |
| **OP9** | 29 | 0 | **10**/12 | 189 |
| **COPIES** | 29 | 0 | **10**/12 | **201** |
| **OP11** | 29 | 0 | **12**/12 | 137 |

- cursor 与 T804/T788 基线逐一吻合——**五臂全有效**
- B 臂 DEADLINE @12 一致（250ms 保持）；prefix 臂在 deadline 前退出
- prefix 零 commit（commit=0/quanta=0 全部）

## 保守下界

COPIES−OP9 交错重测：poll deltas 201 vs 189；逐事件 ns 数组+drain 窗口
在原件（每臂独立）。**成本归属保持保守**——按卡面"不以 poll 数推成本"，
未做外推；原始 fence wait 值与区间全部在原件供 T773 reader 精算。

## 回执

- 有效分母=5（合同"至少五组"达成）
- T773 reader 判定原件 t809/x-*.jsonl
- 归 architect：五臂交错一致性+保守下界依据齐备
