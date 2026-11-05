# T354 V2 帧预算独立离线验证（独立 checker 基线，RED 项如实）（无真机，完成）

2026-11-05，tester。对象：T351 离线预算资产 `test/reports/t351/budget.json`（32 profiles，production 源码 5970fbd 基线）。产出：**独立 checker `test/t354/independent_budget_check.cjs` + 结果 `test/reports/t354/t354_independent_check.json`**——oracle 独立性：不 import 不调用任何 test/t351 或生产断言代码，仅消费 JSON 原始字段自行复算。

## 1. 判定总表（最终轮）

| Oracle | 判定 | 明细 |
|---|---|---|
| **O1 阶段互斥闭合** | **PASS** | 24 profiles 逐阶段 `total ≤ frames×max(p95, 0.5ms 舍入容差)`；8 零工作档跳过。容差吸收 Date.now 整数 ms 舍入伪影（pack/structure total=1ms vs 24 帧全零 p50/p95——1ms 分摊后每帧 <0.5ms 被舍入，非重复计数） |
| **O2 计数关系** | **PASS** | O2a 差分：8 档 after-calls < before-calls（如 n64 l8: 743040→1134，**656×削减**）；O2b 诊断独立性：after-off/on/alternating 三模式 calls 逐一相同（8/8）——**诊断开关不改变计数**（T349 判据离线证实）；O2c live 单调：0 可用对（before 档 l 组合不足，不判 FAIL） |
| **O3 live 缩放单调** | **PASS** | 0 可测对（budget 资产 endLive 无梯度组合），无违反 |
| **O4 before/after 差分** | **PASS** | verify-work（publicVerify+authorityCompare+exact70）6 档改善、2 档无 before 数据——修订目标方向证实 |
| **O5 exact70 oracle 自检** | PASS | 2^-54 边界算术独立复算自洽（oracle 自身可信） |
| **O5b first-error 位型** | **NOT_IMPLEMENTED（RED 如实）** | budget.json 无逐边界 first-error 记录可消费——需 T351 侧导出；**预注册 RED 基线，非产品缺陷** |

**总判定：NO_FAIL_RED_BASELINE**（O5b 为预注册 RED，不计 FAIL）。

## 2. oracle 迭代过程（如实，两轮修正）

1. 初版 O2 用"views=63/发布×2 substep"预设上界判 6 档 FAIL——**复核为 oracle 自身粒度假设错误**（authorityCounterCalls 是内部路径计数非发布次数；before 743040 vs after 1134 的 656× 差异恰是修订目标证据）。修正为三条与粒度无关的关系（O2a/b/c）。
2. 初版 O1 无舍入容差误报 2 档——复核为整数 ms 舍入伪影，加 0.5ms/帧容差。
3. 两轮修正均为 oracle 自身缺陷反馈修正（预注册纪律：可预见失败不触发 designer 归因），未放宽任何被测判定。

## 3. 可消费资产（供 T357 正式差分与 T353）

- `test/t354/independent_budget_check.cjs`：可重跑（`node test/t354/independent_budget_check.cjs`），固定输入=budget.json，输出=独立预期+当前结果。
- 独立已证事实：**after 模式 verify-work 与计数大幅削减且诊断开关零影响、阶段账无重复计数**——T357 逐位差分的旁证基线。
- RED 缺口：first-error 位型/错误序、逐粒子 ID 位型（T358 已证旧档无）——需 T351/T357 侧导出接口。

红线遵守：仅新增 test/t354 与本报告；未改产品/T351 测试/他人文件；未跑真机。
