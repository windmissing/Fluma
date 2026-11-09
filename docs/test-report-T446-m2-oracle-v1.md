# T446 · GPU M2 独立配对与事务反例 oracle 报告

2026-11-09，tester，P5，不使用真机。从 T440 design 冻结（algorithm-T440-gpu-M2-merge-birth-v1.md §1–10、design-T440-gpu-M2-checks-v1.py、design-T440-gpu-M2-fixtures-v1.json 33 cases）建独立枚举/整数 RNE 与事务断言 oracle。先冻结设计层语义，不等工程 wire。

## 交付

- `test/t446/m2_oracle.py` — 独立 oracle（verdict PASS，31 entries 全过 + 5 项 NOT_IMPLEMENTED）
- `test/reports/t446/t446_m2_oracle.json` — 判定原件

## 独立语义实现（不 import designer 代码）

- **完整 group** `G=(namespace,phase,sourceKind,sourceId,originHi,originLo,tag,medium,floor(x/Q))`，origin 独立于 sourceId
- **P(a,b)**：eligible∧同组∧idA<idB∧|yA−yB|≤radiusQ；最小 pair=全体满足 P 的 pair 按无符号 (idA,idB) 字典序
- **RNE**：一次精确有理最近偶数（负数先绝对值按商奇偶 tie），`Fraction` 独立参考
- **停止序**：TARGET_COMPLETE→COLUMN_LIMIT→BIRTH_LIMIT→ID_EXHAUSTED→NO_PAIR 首达判定

## 覆盖（M201–208 逐域 + 33 反例类型映射）

| 域 | 覆盖 | 结果 |
|---|---|---|
| M201 | unsigned MAX_ID 作 idB、重复 ID 不进 pair 逻辑（入口审核域）、同 sourceId 不同 origin 不合并、**60 组随机全枚举==邻接见证**（最小性证明有限验证） | 4/4 |
| M202 | nearest 不是最小 b、radius±1 边界、重包 unary 排除、中间非 eligible 不遮邻 | 5/5 |
| M203 | merge 后重选/旧 disjoint batch 反例、generation 保留语义 | 2/2 |
| M204 | 正/负 tie-even、抵消零唯一编码、极大宽乘 <2^48、结果在两输入间 | 6/6 |
| M205 | 停止序五态首达、offered 不截短 | 6/6 |
| M206 | 跨来源不合并、per-origin 聚合语义冻结 | 2/2 |
| M207/M208 | SOURCE128 快照布局/SOURCE 成功→QUANTUM 失败保留 SOURCE/fatal 整 root 回滚/迟到尾只退休（设计层冻结，精确断言列 NOT_IMPLEMENTED） | 4/4 |
| BOUND | M1 大包 2×131072>244667 必然阻跨、新 package 2×122333≤244666 | 2/2 |

## 过程记录

初轮 3 处 FAIL 全为 fixture 语义错：M201a 的 MAX_ID 粒子 origin 应与配对对象一致；M201b 重复 ID 在 `idA<idB` 约束下本就不进 pair 逻辑（入口审核域，非 pair 判定）；M201d 随机组出现重复 ID（病态输入），加 ID 唯一约束（真实系统由入口审核保证）后 60 组全过。

## NOT_IMPLEMENTED（5 项，依赖 V3 wire）

1. M207 SOURCE128 used-mask 逐 word 断言（需 V3 sourceRecord wire）
2. M208 每 stage 故障的 completion reason23–26 精确码（需 V3 firstError wire）
3. M205 firstId/birthCount 连续区间断言（需 V3 header132–144 wire）
4. M206 per-origin liveMass 聚合审计（需 V3 base 审核路径）
5. M203 mutationSerial U64 checked+1（需 V3 header word141/142）

依赖 V3 工程字段后由后续工程差分卡消费本 oracle；本卡不新建 CPU 仿真，不判产品实现。
