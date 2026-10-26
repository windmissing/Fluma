# T132：向外算术测试先行（定向舍入、affine 白名单与局部证书）v1

日期：2026-10-26。角色：tester。任务卡 `6adf48d1e4b0127832c37ab9`。合同：T127 §2/§4/§6。执行器：`test/p132_outward_oracle.cjs`（独立精确有理数 BigInt oracle；**11/11 PASS**，其中 RED-01 为预期 NOT_IMPLEMENTED）。raw：`test/reports/t132/p132_results.json` + `p132_outward_fixtures.json`（供 T133）。复用 T125 基础 14/14 结论不重做全套；不使用真机；不改阈值。

## 1. RED 基线（供 T133 阶段1）

`P011OutwardArithmetic.ets` 与 `P011WriterEnvelope.ets` **均不存在**（fs 分类区分文件缺失与加载失败，沿 T125 纪律）→ **NOT_IMPLEMENTED**（预测试 RED，非业务缺陷）。无生产 route 启用。

## 2. Outward 算术原语 oracle（T127 §2）

| case | 内容 | 结果 |
|---|---|---|
| OA-01 | nextUp/nextDown 全边界：±0（nextUp(−0)=+minsub、nextDown(+0)=−minsub）、次正规步进、MAX_VALUE→+Inf、负域方向反转；**位操作实现合同**（禁止 v+EPS*v） | PASS |
| OA-02 | add/sub 精确语义：结果 = RN(精确和) 及至多一个相邻向外值；±0/subnormal 精确保留（5e-324+(−5e-324)=0）；**溢出检测**：1e308+1e308 精确=2e308 > MAX_VALUE → 无有限表示（RANGE_UNSUPPORTED 预期） | PASS |
| OA-03 | divide：先 RN(a/b)，再用 exact(v·b−a) 判方向、负 b 符号反转；b=0/除数区间跨 0 拒绝合同冻结（不产假 0） | PASS |
| OA-04 | 区间乘：恰四次端点组合的向外 min/max（构造 [0.5,1.5]×[−2,3] 四角精确比较）；add/sub 取相应端 | PASS |
| OA-06 | η 语义：binade [1,2) half-ULP=2⁻⁵³ 精确（ulp=2⁻⁵² sanity）；次正规 half-ULP 不可表示 → **MIN_VALUE 作有证上界（文档化，非容差）** | PASS |
| OA-07 | 共享 result 纪律：确认 T112 模块返回共享 result 对象（T125 §4 教训）；**T133 合同必须在下次 compare 前快照 {code,sign}** | PASS |

## 3. affine 白名单（≤1 积 + ≤7 标量，≤8 项，对齐 2⁻²¹⁴⁸）

| case | 内容 | 结果 |
|---|---|---|
| OA-05 | B=H 场景：r0+a·q+b 与 ±H 精确比较——**T86 真实残差（存储值 24.577350269189626−24，+4ULP）超出 H**（1 积+5 标量足够）；B=\|r0\| 场景：代数消去同 r0 → 2·r0+a·q+b 仍 ≤1 积+7 标量（r0 重复两次，无第二积）；>8 项合同必须拒绝（不变形通用表达式解释器） | PASS |

夹具同时冻结 T86 恒等式：S64=`403893cd3a2c8199`、S63=`4038000000000000`、超出 4ULP。

## 4. 局部证书与 ZERO 合同（T127 §4）

| case | 内容 | 结果 |
|---|---|---|
| OA-08 | LOCAL non-worsening vs FINAL：8 列链在列 5-6 预置 2H 违规，FINAL 全边验证在边 5 报 STRICT_EDGE 失败；**LOCAL 只核 changed+相邻边则不受远处既有违规影响**——boundMode/identity 必须分立字段，不得调全场 verifySandTop 伪称局部实现 | PASS |
| OA-09 | ZERO 无写合同冻结：同位型保存、无 ±0 写、无 waterTop/cache 重算、无 reserve/close 成功 ticket、无成功 LAND 事件、不改 pool state/y/vy/ID/free-list；CONTACT_HOLD 需求返回 UNSUPPORTED 而非包装原 disposition | PASS（合同夹具） |

## 5. 供 T133 的夹具（p132_outward_fixtures.json）

- nextUp(0) 位型 `0000000000000001`；nextUp(MAX)=Inf。
- 溢出反例：1e308+1e308 → 不可表示。
- divide 精确有理数与 RN 值（1/3）。
- affine 项数预算：B=H（1积+5标量）、B=|r0|（2·r0+a·q+b ≤8项）。
- η：2⁻⁵³ / 次正规 MIN_VALUE 上界。
- T86 残差恒等式（+4ULP，3320 位型族）。

## 6. 边界

- oracle 全部期望由本文件独立 BigInt 有理数派生；T125 基础结论复用未重做。
- RED 是预测试登记；模块缺失不是业务缺陷，T133 实现后按本夹具对齐。
- 无阈值/判据变更；结果回 architect，T119 等设备线不受影响。
