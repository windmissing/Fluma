# T175 M2 与主门禁修订独立验证报告

日期：2026-10-27。任务：6ae06c77e4b06ee35757a51b（tester，离线）。被测：T174 c797ea9（M2 定容改造）+ T168 0fd4a2e（工具续作 v2），冻结参考 961d837。

## 执行器与独立点

`test/p175_m2_gate_independent.cjs`（11 用例）。独立性：冻结参考 SHA 从 git 对象独立重算并与 T174 归档交叉核；M2 全管线对比直接从 961d837 git archive 提取整树加载（@ohos stubs），不经 T174 自测加载器；探针反例注入位置/标记独立设计。

## 结果：11/11 PASS（原始 JSON：test/reports/t175/independent.json）

### A. 冻结参考身份（1/1）

三文件（P011MaterialGuard/WaterSandField/P011NaturalObservation）git-object SHA256 独立重算，与 T174 selftest 归档 frozenSourceSha 一致（6c2133ff…/8adf4a57…/2f2cab36…）。

### B. M2 全事务六类独立对比（1/1，最重核）

961d837 冻结整树 vs 当前树，同一 M2 全管线场景（可准入注入×2 + 满池拒绝注入触发 λ/rollback + 32 步）：

- after 状态（field 十数组 hex/pool 快照/src/预算/commit）逐字节相等；
- stepFails、source 账、预算三元组、注入事件账（admitted/rejectedReason）相等。

叠加 T174 自测的 11520 组数值核（sign±1/边界列/H±EPS/ULP/NaN/Inf/qLo>qHi/tie/RN 逐位）与 256 组实际 host 调用（-0/非整数列/非有限值）——**旧 Map/Set 冷核 → 定容数值核的逐位等价在独立加载路径下成立**。

### C. 探针新增能力独立反例（6/6）

| 反例 | 结果 |
|---|---|
| 固定数组元素存储：数组字面量仍记分配（无静默豁免） | PASS |
| super 静态 base 派发（allocation 依赖运行时值→保守可达） | PASS |
| 闭包构造即分配；调用才执行 body | PASS |
| for-of break：body 分配记录、IteratorClose 保守路径 | PASS |
| 字符串 toString/padStart 记结果效应（内建不静默豁免） | PASS |
| 伪 builtin set 执行其体（分配可达，非按名豁免） | PASS |

### D. 主图独立复现与归档完整（3/3）

- OFF_ALL：ZERO_ALLOCATION、remaining=0、24 根、60/60 registry、1032 contexts——与 rev2 gate-summary 逐字段一致；
- 旧无参保守图 35 remaining 原样保留；
- v1 的 81 remaining 归档在 `test/reports/t168-rev2/original-81/` 未被覆写（目录在）。

### 基线复跑

T168 开发自测 25 项 + rev2 效应夹具（合计 74）独立复跑全 PASS（本会话 IMiQvE 任务输出）；T174 自测 9/9（11520 numericCases）复跑 PASS。

## 边界与结论

- 主图 ZERO_ALLOCATION 为源级有限闭包断言（OFF_ALL），不含运行时 GC/设备/卡死因果（归设备卡）；N ON 真实闭包内 probe 保留，未以关闭 N 取零（源码核）。
- T174 工具级等价 + T168 v2 工具反例均独立复核通过；**无阻断，T156 可按前置接续**；失败上报：无。
