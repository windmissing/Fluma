# T224 整体 D1 完整切点补验报告（v1）

日期：2026-10-28；tester；不使用真机。被测：T211 `edd980c` 树（T213 之后）。与 T213 已过范围去重：T213 核了 3 个 cut 钩子 + detail DISABLED 恒等式；本卡启用 detail、覆盖故障首码面、聚合界与 unknown-writer 回退。执行器 `test/p224_tx_cuts_independent.cjs`，原始 JSON `test/reports/t224/independent.json`。

## 结论

**7 PASS / 0 FAIL**。

## 分项

| ID | 内容 | 结果 |
|---|---|---|
| T224-1 | **detail=true 详细 trace 行 schema**：kind∈{1,2,3}、leave 行 incl/excl finite 且 excl≤incl、cut/context 字段捕获、root 单调；**ring 容量满 → incomplete=true**（显式，不热扩容不缺尾当零） | PASS |
| T224-2 | SOURCE 侧故障面 4 组：非法形状（长度 64≠128）拒绝且无半写（bh 原始切片不变）；**负质量注入拒绝**（null）；重复 sequence 原语义接受（无新拒绝，T211 合同）；全零注入 null | PASS |
| T224-3 | step 故障面：NaN dt 安全 no-op 保持 sourceEpoch；大质量源（200→可能部分准入/回退）后 trace 恒等式仍闭合（entered=completed+failed+cancelled+active；AUDIT_LOGICAL=EXECUTE+CERT_HIT） | PASS |
| T224-4 | 6 源 + 10 步全 run：**无 scope 泄漏**（active=0、entered 全闭合）；queue>64 背压路径经 TxQueuedSourceCommand（T211 commands 已核），直接注入无独立 queue 面（如实） | PASS |
| T224-5 | 聚合界：entered==completed、源 attempt 计数（counter 40）记录、copyBytes/archive 路径分列 | PASS |
| T224-6 | unknown-writer LEGACY_UNCERTIFIED 回退计数（counter 37）在生产注入/step 流可达；正路径全 certified 时为 0 如实记录 | PASS |
| T224-7 | 真机 fixture 身份：t211-final STRICT_O2 unsigned HAP SHA + source manifest 交接（离线 PASS 不等于设备 PASS；20-cut 故障矩阵真机复现用同源包） | PASS |

## 与 T213 的去重边界

- T213：OFF/LEGACY/TX 三路差分、AVL/branch/storage/ID 独立结构核、GPU 桥等价、detail DISABLED 恒等式。
- T224（本卡）：detail ENABLED 行 schema 与 INCOMPLETE、SOURCE/step 故障注入首码面、聚合界、unknown-writer 计数、fixture 交接。
- 未宣称：**20 个切点逐一独立故障注入的完整矩阵未全部达成**——生产代码侧显式 `.cut()` 标记仅 3 处（SOURCE_CAPACITY_ETA=2、SINK_RESERVE_GUARD=12、SOURCE_PUBLISH=19），其余 17 切点为原函数 scope 边界（enter id），其故障首码由原检查块权威（T211 保留原 audit/guard/错误块），本卡经 facade 可达路径覆盖了 SOURCE/step 两族；Hot 内部故障注入（beginFailure/collectFailure）需 diagnosticEnabled + tiltTraceSink 组合，属开发 faultPlan（T216 口径），独立矩阵归后续设备卡/专项卡，不在本卡虚报全矩阵完成。

## 边界

- 离线 PASS 非设备 PASS；真机复现 fixture（t211-final STRICT_O2 unsigned HAP + source manifest SHA）已在 T224-7 绑定。
- detail trace 的 UTC wall 时钟语义（负值→INCOMPLETE、不冒 monotonic service）T213 D-10 已核，本卡未重复。
- V2 数值 policy / D2 证书 / 异步准入未实现，未派假缺陷。
