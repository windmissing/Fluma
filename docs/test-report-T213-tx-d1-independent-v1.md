# T213 整体事务 D1 与 GPU 桥独立验证报告（v1）

日期：2026-10-28；tester；不使用真机。被测：T211 `edd980c`（D1 与树桥）当前树 + T212 `a27d92c` GPU 桥模板（隔离树 HOST_DOUBLE）。依据 T206 §19 TX-01..12 规格、T205 架构 §4/5/6/9/10、T211 交付声明。执行器 `test/p213_tx_d1_independent.cjs`，原始 JSON `test/reports/t213/independent.json`。

## 结论

**13 PASS / 0 FAIL / 0 NOT_RUN**。基线复跑：T211 开发五套自测（core 1804/0 fail、bank 6/6、fixture N192/512 matched、commands、baseline 6 命令×3 backend 全一致）与 tree-bridge（N192/512 COMMITTED、after 逐字节等）全部复现。

## 分项

| ID | 内容 | 结果 |
|---|---|---|
| D-01 | 8 命令全差分（源成功/重复序/非法时间/NaN dt/queued/configure/reset/再源）OFF/LEGACY/TX 三路回执+原始状态逐字节一致 | PASS |
| D-02 | trace 闭合恒等式 entered=completed+failed+cancelled+active；AUDIT_LOGICAL=EXECUTE+CERT_HIT 且 CERT_HIT=0（数值 fold 恒 execute）；原拒绝路径 scope 仍闭合 | PASS |
| D-03 | cut(2) SOURCE_IDENTITY / cut(19) SOURCE_PUBLISH / cut(12) SINK_RESERVE_GUARD 钩子可达 | PASS |
| D-04 | 源成功前缀保留：NaN dt 安全 no-op（stepped=false）不改床；下一有效 step 在已有源上执行 | PASS |
| D-05 | AVL 配对 vs 独立 O(n²) oracle（-0 / r 边界 / MAX_ID / 跨 group / slot 复用）| PASS |
| D-06 | BranchView：fork/mark/commitChild（parent bank 不动）/rollback 含 child 成功还原/新 mark 后旧 point STALE/read lease 在 rollback 后失效（即使恢复字节相同，T205 §4.1 合同）/foreign view 拒绝 | PASS |
| D-07 | Storage：scalar save/restore、深度 8 层成功第 9 层 UNCERTIFIED、enterRoot 深度守卫 | PASS |
| D-08 | ID 树 retire/reuse/no-stale-hit；重复 ID 在 PairAdj 层 certified=false（ID 层同值取最小 slot） | PASS |
| D-09 | backend 白名单：armed route 下 GPU_PAIR_V1 拒 TX_D1_BACKEND（TX12 走隔离桥不经 D1 attach）；selectPair foreign owner TX_FOREIGN_OWNER | PASS |
| D-10 | wall inclusive/exclusive 按 cut 聚合、负时长标 INCOMPLETE 不冒 monotonic service、detail 默认 DISABLED | PASS |
| D-11 | GPU 桥树适配（隔离树 TxGpuTreeAdapter，HOST_DOUBLE）：N192/512 LEGACY vs tree 物理 after 逐字节一致 | PASS |
| D-12 | resource-ledger 诚实标记 NOT_TOTALLED/PARTIAL_BOUND/UNBOUNDED_EXTERNAL 在档 | PASS |
| D-13 | trace 字典 20 cut 与 T205 manifest 顺序+身份一致、273 fn scopes | PASS |

## 边界与残余

- HOST_DOUBLE 不是真机 GPU；树 backend 真机联调归设备卡（T211 交付声明同口径）。
- V2 数值 policy / D2 证书 / 异步准入 owner 未实现，未派预测试假缺陷（TX-04 证书复用、TX-10 异步背压留待对应实现）。
- trace 默认 detail DISABLED；详细 32 列逐 scope 差分需 ENABLED 专项（开发已留开关，未在本卡开启，不计未验项）。
- 1804 开发 oracle 仅作开发自测证据引用，本执行器全部独立实现（oracle/结构 walk/恒等式）。

## 执行器语义备注（实现观察，非缺陷）

- savepoint 重复 rollback 同一 point 为实现语义（幂等恢复同一 bank），STALE 在新 mark 后触发；T205 未承诺重复 rollback 拒绝。
- 原拒绝路径（如非法 source、NaN dt）为 return 非 exception：trace failed 不增、entered 增。
- NaN dt step 原语义为安全 no-op（ok=true, stepped=false）。
