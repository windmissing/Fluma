# T642 v1 · 目标 9 位移拨动 profile 独立公式与反例基准（T637 设计消费）

日期：2026-11-19，tester，不使用真机，无 designer checker import，无 CPU 物理
stepper。
依据：`algorithm-T637-v1.md`（G9_DM_NATIVE_RECEIPT_SPATIAL_R1，DESIGN_FEASIBLE，
NOT_IMPLEMENTED）+ `design-T637-fixtures-v1.json`（52 fixtures）。
证据：`test/t642/`（independent-exec.json 全夹具独立执行 + independent-derivations.json
八项独立推导）。

## 独立实现与执行（30/30 可执行 case 全 PASS）

自行实现 T637 §4/§5 全部公式（Fraction/Decimal 精确算术、RNE ties-even、
checked 饱和），不 import designer 检查器：

| 组 | 覆盖 | 结果 |
|---|---|---|
| R01–R06 receiptDuration 资格 | ±1ns 边界（8,333,333/8,333,334、1e9/1e9+1ns） | 6/6 ✓ |
| A01–A05 age 门 | -1/0/249,999,999/250,000,000（等号合法）/+1ns | 5/5 ✓ |
| V01–V04 callbackValidate | 0/8ms/8ms+1ns/-1 | 4/4 ✓ |
| P01–P12 空间配方 | deadzone 双侧、符号（纯下滑 vy=0 斜下滑有 vx）、-0 规范、NaN/129/257 拒、饱和 SAT_X\|Y=3、精确 RNE 值逐 word 对照 | 12/12 ✓ |
| D01/D02 延迟反例 | 100ms 手势 DOWN 延迟 99ms→receipt 1ms 拒（C-timed 会放大 100 倍）；整体晚 10s→receipt 不变 eligible 且 sampleAgeUnknown | 2/2 ✓ |
| D03 空间不变性 | 同位移 20ms/200ms 接收时长 J/cap 全同（vxQ=vyQ=2097152） | 1/1 ✓ |
| S01–S11 状态机 | 11 序列独立枚举（ONE_COMMAND×3 / NO_COMMAND×8） | 11/11 ✓ |

N01–N11（11 项独立交接域：FIFO 满/过期头/去重/BLOCKED 不消费/2^53 不转换/GPU
坏值/冷 reader/成本）按 fixtures 原样记 **NOT_RUN**——需产品实现，与 T637
11 项 NOT_RUN 精确映射。

## 独立推导补充（八项，X01–X08）

- **量化误差界自推**：trunc <1/Q；RNE 速度 <2.5/Q per axis；J 误差 =2.5/16=0.15625
  精确值
- **四个硬界独立复算精确吻合**：|Jx|max=1.5Q²、Jymax=2Q²、向量 2.5Q²、
  cap=800Q³·2^24<2^120
- deadzone 不连续性：等号走 ACTIVE，阈值两侧 ±1Q 探针列为设备域必测项
- 延迟不可辨识性与空间不变性的两条不对称反例（D01/D02/D03）独立复算

## 判定与映射

- 独立基准就绪：后续 T609 型设备验证可直接逐 case 对照（含 ±1ns/等号/饱和/反例）
- 时钟/队列/GL 真实产品 NOT_IMPLEMENTED/NOT_RUN 不冒 PASS；T637 profile 默认 OFF
  维持；未擅改任何阈值（T641 若细化字段按正式版本消费）
- fixture 输入 SHA 绑定：执行脚本直接消费 design-T637-fixtures-v1.json 原字节，
  期望值逐字段对照记录于 independent-exec.json（无字段增删）
