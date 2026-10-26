# T111：严格态基础测试先行 v1（精确 dyadic oracle 与证书边界）

日期：2026-10-26。角色：tester。任务卡：T111。合同：`docs/architecture-P011-T104-strict-state-certificates-v1.md` §2/§7 阶段A。执行器：`test/p111_dyadic_oracle.cjs`（测试端 BigInt 任意精度 oracle；固定对齐 2^-2148）。raw：`test/reports/t111/p111_results.json` + `p111_dyadic_fixtures.json`（冻结夹具供 T112）。**11/11 PASS**。不使用真机；不改阈值/业务。

## 1. 精确 oracle 能力（9 case）

| case | 断言 | 结果 |
|---|---|---|
| OR-01 | **每个有限 binary64（含 subnormal/最大值）在 2^-2148 对齐下精确表示**（最小 sh=2148−1074−52≥0）；去重后序保持 | PASS |
| OR-02 | 残差比较在**存储值**上精确决策；生产现行的「RN(Sright−Sleft)」残差与精确存储差可不同——严格 revision 改比较存储值 | PASS |
| OR-03 | ULP 机械：nextUp(tan30) 有效；平凡输入的 dyadic 乘积与 double 位型一致 | PASS |
| OR-04 | 4ULP 边界：+4ULP 斜率在文档边界内；**T86 实际斜率 tan30+5e-16 ≈ 5 ULP 越界**；两者都超精确 hard max（严格 revision 拒绝字节级重接纳旧非法初态） | PASS |
| OR-05 | 符号零：±0 规范化后比较相等（定向门口径） | PASS |
| OR-06 | subnormal 排序经整数表示精确保序（oracle 无 FTZ 损失） | PASS |
| OR-07 | 容量：max×max 乘积 significand ≤106 bits，对齐单位远低于 4200-bit 上限 | PASS |
| OR-08 | 非有限（NaN/±Inf）被 oracle 解压层拒绝（无符号位偷渡） | PASS |
| OR-09 | 表达式白名单边界：oracle 只暴露固定表达式（和/两输入积/rate 积/定向零）；超白名单生产模块必须报 `ARITHMETIC_EXPRESSION_UNSUPPORTED` | PASS |

## 2. RED 基线（供 T112 阶段A）

`P011ExactDyadic.ets` 与 `P011StateCertificate.ets` **均不存在**；planning delegate 无 `previewCandidate`（V2）→ **NOT_IMPLEMENTED**（预测试 RED，非业务修复失败）。生产 route 未切换。

## 3. 冻结夹具（T112 实现对照）

`p111_dyadic_fixtures.json`：tan30（hex `3fe279a74590331c`）、nextUp（`3fe279a745903320`）、T86 斜率（同 hex，=nextUp）、subnormal 对、maxDouble、白名单表达式清单、COMMON_SHIFT=2148。T112 实现须与本 oracle 逐值一致；非有限/容量/白名单拒绝语义按 T104 §2 原文。

## 4. 执行器自身修正（非判据变更）

- OR-01 首版序检查未去重 ±0（同值不同位型）与重复 subnormal（4.9e-324 与 5e-324 同位型）→ 改按规范化值去重；
- OR-02 首版对 BigInt 误用 Math.abs → 改为有意义的「精确存储残差 vs RN-of-difference 残差」对照断言；
- OR-04 首版把 T86 斜率当 4ULP 内——实测 +5e-16 ≈ **5 ULP**（ulp=1.11e-16），修正为「+4ULP 在边界内、T86 实际 5 ULP 越界」双断言（与 T100 的 4ULP 记录一致）。
