# T147 向外算术独立差分（tester 报告 v1）

日期：2026-10-27。角色：tester。不使用真机。目标：T133 e7cd4a1（`P011OutwardArithmetic.ets` / `P011WriterEnvelope.ets` / `P011ExactDyadic.compareAffineBound`）。
执行器：`test/p147_outward_independent.cjs`（固定 seed=20261027，独立 BigInt 有理数 oracle，Python `fractions` 第三方仲裁）。

## 结论

**9/10 PASS；发现生产缺陷 F1（divInto 方向极性反转），D2b 刻意 FAIL 留作缺陷证据，须交 architect 分流。** 开发自测 4/4 与本卡独立 oracle 不互为替代；本卡所有期望值来自独立精确有理数复算，不镜像开发测试。

## 覆盖与结果

| 用例 | 结论 | 要点 |
|---|---|---|
| D1 nextUp/nextDown | PASS | ±0（`nextUp(-MIN)= -0`、`nextDown(0)=-MIN`）、次正规、`1+ulp`、`MAX→+Inf`，11 个固定点对独立位遍历 oracle 全一致 |
| D2a add/sub/mul 端点 | PASS | 225 固定 seed 样本：方向端点与"精确残差符号"逐一吻合；精确值情形区间宽度为 0（`lo===hi===v`），无多余 nextUp/nextDown |
| D2b div 端点 | **FAIL（F1）** | 70/75 非零残差样本方向全反（5 个 matched 均为残差恰 0） |
| D3 拒绝码 | PASS | 溢出（add/mul/sub/商）→`RANGE_UNSUPPORTED`、零除→`DIVIDE_BY_ZERO`、NaN/Inf→`NON_FINITE` |
| D4 区间四角 | PASS | 120 对区间乘法四角组合与独立 corner oracle 全一致；区间加端点精确；跨零除法拒绝 |
| D5 η | PASS | binade [1,2) 半 ULP=2⁻⁵³、[2,4)=2⁻⁵²；次正规 floor=MIN_VALUE（文档化保守上界，非容差） |
| D6 affine | PASS | 200 随机（1 乘积 + 1..7 标量）与精确 oracle 全符；>7 项/NaN 标量→`ARITHMETIC_EXPRESSION_UNSUPPORTED`；共享 result 调用后立即快照纪律验证 |
| D7 LOCAL/FINAL | PASS | 整数格+半格构造（0.5、3.5 均二进制精确，无 binade 舍入陷阱）；改善 patch LOCAL 合法（`ENTRY_H_OR_OLD_RESIDUAL`、`materialOnly=true`）；远边违规不进 LOCAL 视野；FINAL 无入口豁免（after 斜率 >H 即拒，firstEdge=0 符合语义）；同号更小残差接受；恶化/`BEFORE_MISMATCH` 拒绝 |
| D8 NoWrite | PASS | 位级浮点保持区分 ±0 与 NaN 位型；ID 精确；计数器全零；`CONTACT_HOLD` 返 `UNSUPPORTED` 且禁止 verify |
| D9 envelope | PASS | branch/node 容量、`lo>hi`、闭区间开边界、op 域、非有限、前向节点引用全部类型化拒绝 |

## F1 缺陷（须 architect 分流）

**位置**：`P011OutwardArithmetic.divInto`（entry/src/main/ets/physics/P011OutwardArithmetic.ets:74）。

**内容**：`directed(out, v, -sign*(b>0?1:-1))` 中多乘一个负号。数学上 `exact − v = (v·b − a)/b`，其符号应为 `sign(v·b−a)·sign(b)`；生产实现给出其相反数。**对任意 b 符号方向都反**。

**决定性最小反例**（Python fractions 第三方仲裁）：
- `a=181.74490928649902`，`b=3.2622665372628035e-05`，`v=RNC(a/b)=5571123.8554711`
- 精确 `v·b − a = +1.33e-14 > 0` → 精确商在 v 上方 → 正确端点 `[v, nextUp(v)]`
- 生产返回 `[nextDown(v), v]` —— 方向相反，**区间甚至不包含精确商**
- `compareAffineBound(v,b,[-a],1)` 直调返回 `sign=+1`（正确）→ 缺陷仅在 divInto 的极性表达式

**影响面**：仅 `divInto` 及依赖它的 `intervalDivInto` 四角组合（非跨零路径）。add/sub/mul、affine bound、LOCAL/FINAL verifier、NoWrite、envelope 均不受影响（D2a/D4/D6/D7 独立差分全过）。75 样本证据与最小反例已归档 `test/reports/t147/d2b_f1_evidence.json`。

**修复方向（供 architect/developer 参考，本卡未改产品）**：`directed(out, v, sign*(b>0?1:-1))`（去掉外层负号）；或保持现符号约定改为传 `sign(a−v·b)`。修复后 D2b 断言（mismatch===0）即为回归判据。

## 边界与声明

- 测试侧构造教训（记入工程注意点，非产品缺陷）：① `2+H` 类和在粗 binade 舍入——**边界构造用整数格或 2 的半格**；② col1 变更同时影响 edge0→1 与 edge1→2 两条邻边；③ **`verifyLocalPatch` 返回共享 `certificate` 对象、`compareXxx` 返回共享 `result`**——任何消费方跨调用持引用会被覆盖，T125 教训在 T133 新代码同样适用（集成合同注意点，报告 §F1 同步登记）。
- 排查过程中本执行器自身 oracle 曾出现三处单位/调用错误（乘积网格 2^8592 vs 标量 2^4296 未统一、add/sub 误含乘积项、div/mul 漏 −v 项），均已修正并经 Python fractions 第三方仲裁；oracle 修正不构成判据变更。
- 未接入生产 strict 路由、未实现 writer 影子事务——按 T133 交付声明分列，不视为本卡缺口；materialOnly 域不冒预算/pool 全事务证明。
- 无阈值/判据/容差变更；未签发设备 PASS；GPU 线不受影响。

证据：`test/reports/t147/p147_results.json`、`d2a_samples.json`、`d2b_f1_evidence.json`。原 T133 交付文件零改动。
