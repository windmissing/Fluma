# T234 GPU驻留独立规格与数值基线报告（v1）

日期：2026-10-28；tester；不使用真机。消费 T230 `algorithm-T230-gpu-resident-step-v1.md` §3/§5/§11（GR01–12）与 T231 平台边界稿；与 T233 并行不互设循环。被测：当前树（GRS/gpu_resident 零实现）。执行器 `test/p234_grs_independent.cjs`，原始 JSON `test/reports/t234/{independent,gr-spec}.json`。

## 结论

**7 PASS / 0 FAIL**——全部 GR01–12 预测试 RED（M0 零实现，9/10 GRS 符号缺），本卡交付独立 oracle（自检通过）+ 机器可读规格。

## 分项

| ID | 内容 | 结果 |
|---|---|---|
| GR-00 | 实现面符号核查：GRS_Q20_V1/GRS_FIXED120_TIME_V1/mulDivRNE/GRS_COMPLETION256_WORD_V1/NUMERIC_RESOLUTION_REJECTED/SIDE_HIGH_BED/BUDGET_QUANTUM_BLOCKED/Hq/Hfloor/gpu_resident **9/10 缺**（仅 Hq 字面巧合命中）→ GR01–12 全 RED 归工程 | PASS（缺口如实） |
| GR-01 | **独立数值 oracle 自检**：BigInt RNE ties-even（r×2 vs den 精确比较，含奇分母）、floorDiv/ceilDiv 符号组合、Hq 保守界 (Hq+1)/Qm ≤ tan30 验证、U128 溢出（0xffffffff²=0xfffffffe00000001） | PASS |
| GR-01b | Q20 输入量化基线：floor(requested×Qm)；<1 量子（1e-7、半量子）→ floor=0 → NUMERIC_RESOLUTION_REJECTED 语义建立 | PASS |
| GR-03 | 有理 DDA oracle 自检：U64 交叉乘比较（λ_bed=50 vs λ_edge=0.25 列边先）、同刻 tie 判定、Vmax/120<8.534 界、eMotion=32 行余量 | PASS |
| GR-04 | 整数 guard 区间穷举：k∈[-2,2] × d_e 近 ±Hq 采样全组合，闭式解（ceil/floorDiv 交集）与约束逐点核验，q0 可行性 | PASS |
| GR-09 | 120Hz carry oracle：a=carry+120·deltaNs、F≤4、carry<1e9、5 组精确值（含 1/30s→F=3+carry 999999960）、无墙钟追赶 | PASS |
| GR-SPEC | **GR01–12 机器可读规格登记**（oracle/fixture/status 逐行 + M0 vs M1/2/3 支持边界 + 旧 F64 差异显式），存 `test/reports/t234/gr-spec.json` | PASS |

## 独立性声明

- 全部 oracle 自持 BigInt（RNE 用 r×2n 与 den 精确比较避免 BigInt 除法 floor 陷阱；位型构造独立），**不调用产品核、不引用 design-T230-gpu-resident-model-v1.py**。
- T230 designer Python 模型只验证推导，不是产品 oracle；旧 CPU F64 只作行为基线，Q20 逐 word 正确性待产品核到件后用本 oracle 对拍。

## 边界

- GR02/05/06/07/08/10/11/12 的产品核零实现：source/dedup64/源表 slab、mod3 颜色三 dispatch、root 原子提交/tombstone、dispatch 包络、scene 同 token、M1 水、M2 pair、M3 全通道——全部 NOT_IMPLEMENTED 分列，不派预测试假缺陷。
- rSoft/drag/g/massPackage/吞吐收益 PENDING_CALIBRATION；M0 平台成功（T232）非 GRS 产品收益；当前模型/旧 pair 成功非 GRS 产品 PASS。
- 固定 120Hz（GRS_FIXED120_TIME_V1）与旧可变 dt 最多 2 子步是**版本差异**，不互相比对耗时；每笔量化残差<1 量子是上界，不入基准。
