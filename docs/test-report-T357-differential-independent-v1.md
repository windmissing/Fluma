# T357 V2 性能修订独立差分（新旧逐位状态与阶段账闭合）（离线，完成——差分稳定零 FAIL，缺口如实）

2026-11-06，tester 离线。对象：T351 实际交付（生产源 5970fbd 基线 vs 修订后）差分与阶段账。消费 T354 独立 oracle 资产（`test/reports/t354/t354_independent_check.json`）。**oracle 独立性说明**：`test/t351/units.cjs` 本身是独立差分实现（`baseline.cjs` 从 git `5970fbd` 独立重建旧树，与当前树逐位对照），本卡复跑不等于"开发自测断言复用"；T354 checker 为另一独立 oracle 交叉验证。

## 1. R1 差分复跑 vs 归档逐项对比：**PASS（稳定）**

- 独立复跑 `units.cjs`（基线树 `.codex/local-test/t351-baseline` 在位）：**23/23 PASS / 0 FAIL**。
- 与归档 `units.json`（T351 交付）对比：**case 名集合与顺序完全一致、状态零 mismatch**——差分结果跨时间稳定。
- 覆盖：N=2/16/64/192/512 × Q0/1/2 × A0/1 的 bitwise/checkpoint/source/commit/counter 逐位对拍；N512 L128/L512 上沿 payload/acc/cache；contact R/N；**首错冻结证据/顺序 parity ×8 类**（structure/mass/cache/duplicateId/binding/field/externalWriter/rollback）；detach/alias/replacement 精确保留。

## 2. R2 T354 独立 oracle 交叉：**一致（零矛盾）**

T354 checker 判定 `NO_FAIL_RED_BASELINE`、FAIL 清单为空——与差分复跑结论方向一致，双 oracle 无矛盾。

## 3. R3 互斥帧段与 inclusive stage 分列：**结构 PASS**

budget.json 每 stage 条目独立携带 `p50/p95/total` 三列（exclusive 计数与 inclusive 分布分列）——T354 O1 已独立核互斥闭合（`total ≤ frames×max(p95,0.5ms)`）。

## 4. R4 诊断 ON/OFF 权威不变：**PASS**

units case `trace ON/OFF bitwise parity and exclusive sum = full frame wall; actual Q/A/bytes/requests` PASS——追踪开启不改变逐位状态与互斥闭合（T354 O2b 计数独立性交叉一致）。

## 5. R5 预绑定视图与非法引用拒绝：**PASS**

`normal publication creates zero new byte views; every compared byte retained` + `byte comparison mismatch/detach/alias/replacement preserves exact decisions and counted prefix` 双 case PASS——63 预绑定视图身份保持、非法引用决策精确。

## 6. 缺口（如实，不冒全绿）

- **fillCopy 实际字节：NOT_EXERCISED**——离线 harness 不走 native 绘制路径，32 档 fillCopy p50/total 全 0/null；实际字节闭合归设备层（T352 已判 NOT_MEASURABLE）。
- **逐边界首错位型**：T354 O5b RED 未解（budget/units 资产无逐边界记录导出）——T357 亦无法补，需 T351 侧导出接口（交 manager）。
- **PC 时间非设备性能**：本卡全部离线，设备性能结论以 T352 为准（perf FAIL 在案）。

## 7. 结论

**DIFFERENTIAL_STABLE_NO_FAIL**：T351 修订在新旧逐位差分（23 类含首错冻结×8）稳定零 FAIL、双 oracle（units 独立差分 + T354 checker）交叉一致、诊断开关与非法引用语义保持。修订安全性进一步坐实；性能与设备层缺口归 T352/T349 线。功能 UP 冻结按修订范围如实（T353 归因中，本卡不假设修复）。

证据：`test/reports/t357/rerun-units.json`（复跑原文）+ 本报告对比数据；`test/reports/t354/t354_independent_check.json`。
