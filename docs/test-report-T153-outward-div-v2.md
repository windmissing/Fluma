# T153 向外除法 oracle 纠正复测（tester 报告 V2）

日期：2026-10-27。角色：tester。不使用真机。消费：T148 §1–2 裁决与 `docs/analysis-T148-division-sign-v1.json`、T147 V1 执行器/报告（保留原 FAIL）、T133 生产实现。
V2 执行器：`test/p153_outward_div_v2.cjs`（新目录 `test/reports/t153/`，seed 初始 20261027 / 最终 2534401740 分列）。**原 V1（`test/p147_outward_independent.cjs` + `docs/test-report-T147-outward-independent-v1.md` 的 9/10 与 D2b FAIL）原样保留，不篡改、不改写为 10/10。**

## 结论

**4/4 PASS。T148 裁决成立：生产 `divInto` 负号正确，V1 报告的 F1 是 tester oracle 符号解释错误。修订后的独立 oracle 下，全部有效样本的生产区间都包含精确商，无真反例；产品零改动。**

## 核心修订（T148 §1）

V1 把 `exact−v` 误写为 `(v·b−a)/b`；正确关系 `q−v = a/b−v = −(v·b−a)/b`，故定向参数 `t = −sRaw·sign(b)`（`sRaw = sign(v·b−a)` 由独立有理数计算保留）。V2 同时用**独立有理数交叉乘**断言 `lo ≤ q ≤ hi`（全部统一在 2⁻⁸⁵⁹² 公共网格：`du(x)·du(y)=xy·2⁸⁵⁹²`、`du(x)·GRID=x·2⁸⁵⁹²`），并逐样本归档原 bits。

## 实测结果

**D2b div 端点（400 随机 + 固定矩阵）**：attempted=400、skippedZero=15、rejectedOverflow=13、**acceptedExact=12（残差恰零，端点零宽）、acceptedInexact=360、mismatch=0**。每条 accepted 样本：位级端点与 `t=−sRaw·sign(b)` 期望一致，且生产区间含精确商（有理数交叉乘）。固定矩阵覆盖 ±1/3、±1/10（分子/分母双向）、正负分母、精确商（4/2→[2,2]）、下溢（5e-324/2 → ±0 定向端点）、溢出/零除拒绝。

**D2c T148 决定性案例改正**：`a=181.74490928649902, b=3.2622665372628035e-05, v=5571123.8554711`——`v·b−a>0 → q−v<0`，正确端点 **[nextDown(v), v]（即生产返回）**；V1 报告主张的 [v, nextUp(v)] **不含精确商**。位级端点与包含断言均确认，与 T148 精确分数核算（`v·b−a=503444847/37778931862957161709568>0`）一致。**"现实现正确、原 oracle 错误"。**

**D4 非跨零 intervalDiv 四角**：6 组符号矩阵（含全负分母 `[[-2,-1],[-4,-2]]` 与倒数符号组合）四角精确商逐角包含 + 全区间端点一致；跨零、端点零分母均 `DIVIDE_BY_ZERO` 拒绝。V1 未覆盖的区间除法独立定点补齐——不能从区间乘 PASS 外推，现已独立成立。

**D5 共享 result 快照**：div 路径 affine 调用结果快照在下一次 compare 后存活。

## 边界与声明

- Python fractions 独立复核过 T148 决定案例与本轮包含断言网格修正（V2 执行器自身曾混用 2⁻⁴²⁹⁶/2⁻⁸⁵⁹² 网格导致首轮 3 FAIL，修正后 4 PASS——oracle 修正记录于执行器注释，不构成判据变更）。
- 不冒 10/10：T147 V1 的 9/10 与 D2b FAIL 为历史事实保留；本 V2 是修订后的独立执行。
- add/sub/mul 与 V1 其它 PASS 范围不变（本卡未重跑，V1 结果按原覆盖消费）。
- 生产 `P011OutwardArithmetic.ets` / `P011ExactDyadic.ets` **零改动**（含 divInto 负号——正确，禁止删除）；无阈值/判据变更。
- seed 初始/最终分列归档；全部有效样本原 bits 落盘 `d2b_v2_samples.json`。

交 manager 验收。
