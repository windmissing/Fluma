# T331 · V2 续证工程三件实施交付 v1

日期：2026-10-31。责任：developer。任务卡：滴答 `6ae5462fe4b0e9cb68ef58d1`。
合同依据：`docs/architecture-T323-v2-continuation-disposition-v1.md`（COMPLEXITY_ENGINEERING_ROUTE / COLD_BUDGET_DESIGNED）。一卡三件+附注两件。

## 1. 增量审计证书（合同 §2 第一件）

**实现**：`P011ParticleHot.ets` audit 双模式路由——

- `auditParticleHotBody` 按 source 路由：`SINK_ROW` 且基线有效时走 `auditIncrementalHot`（增量路径）；`APPLY_TAIL`/`FINISH`/`OTHER` 恒全模式（`auditFullBody`，即原逻辑逐字保留）。
- **基线**：`auditFullBody` 成功后重建增量基线（auditBaseMassH/L 列质量快照 + live/free/nextId 结构计数），并重建有序 id 索引（复用本次排序结果）。
- **增量路径**（每 SINK_ROW）：触列一致性谓词（sink 登记的 c 与 target 列的场/池缓存有限值/非负/容量/DRY 检查，与全模式逐条相同）+ journal 重放 Δ 对账（`patchActionsJournal` 环形缓冲，出生/沉积/抬升/跨列/降级/合并六类动作的列质量增量重放，基线+Δ 必须等于当前缓存，1e-9 容差）。
- **K=16 重验**：每 16 个 sink 行或 attempt 尾强制一次全池重验（合同指定 K=16）。
- **回退语义**：任何增量谓词失败→立即全模式重跑，错误码语义与原完全一致（13）；journal 溢出→增量失效，下轮全模式。`auditFallbacks` 计数披露。
- **生命周期**：beginStep 新 attempt 失效基线（首行必全验）；rollbackStep 失效基线+索引（回滚后证书过期）。

**冻结约束全保留**：不改 dt、不跳审计（counts[7] 仍为 R+2）、不降阈值（所有谓词逐条保留，1e-9 容差原样）、错误顺序不变（先结构后列质量后预算后守恒）。

## 2. audit-scope sort 消除（合同 §2 第二件）

**实现**：有序 id 索引 `auditIdIndex` 增量维护——

- **出生**：nextId 单调递增保证尾插有序（O(1)）；非单调时索引失效回退 heapsort。
- **合并/死亡**：二分查找 O(log L) + 移位删除（同列合并、沉积清零、降级清零三处钩子）。
- **K 重验**：`auditFullBody` 中索引有效时用 O(L) 有序性验证（严格递增=有序+无重复）替代 heapsort；索引失效或基线首建时才排序一次。
- `sortActive`（collect 路径）**未动**——合同只限 audit-scope。

**效果**（scaling 实测）：R=320 的 sort 计数从 R+2=322 次降为 **1 次**（仅基线）；转移差合同从 T311 的 `R` 变为 `2R-1-floor((R-1)/16)`（每行省 1 次全池拷贝 + 全验每 16 行一次），五档全对。

## 3. attemptCumulativeMs 预算披露（合同 §2 第三件）

**实现**：`P011HotWorkReceipt` 新增 attempt 级累计墙钟——`next()` 记起点、`seal()`/`emitProgressCold()` 刷新定格；progress 行（T311_STAGE）与 exportCold（revision 升 **T311_HOT_WORK_V2**，rowFields 显式列出 slot13=attemptCumulativeMs；rows[at+13] 落盘）双通道披露。**不新增测量点**（复用 Date.now，按 T325 裁定保留运行时诊断价值）。

## 4. 附注一件：包 SHA 勘误（合同 §1）

`test/reports/t311/delivery.json` 增补 `packageVariantErratum`（叠加式，不改原文）：实测安装包 `366d1ca6…`（final 变体，bind 无 `stageCounts.fill(0)`，T312 实测对象）与卡面引用包 `9e7ac541…`（candidate 变体，含 fill(0)）的取舍说明、语义判定（纯诊断计数，T312 证据不受影响）、处置（final 为交付变体，卡面引用应为 366d1ca6——登记为 developer 交付清单引用错误）。两包均手签（verified-profile 同 uuid），按 T328 移交裁定 1 口径如实登记，不混写。

## 5. 附注二件：冷采分段化（合同 §3）

**实现**：`T284Cold.ets` `enqueue` 在锚点窗口只做**原子交接**——payload 引用存储+字节/分片算术记账；digest 计算移到 `drain()`（1Hz 冷 timer 窗）首触时惰性计算并回填 index。锚点记录 {anchorId, bufferValid(经 rawBytes/parts), captureMs}。**4ms 预算与 INCOMPLETE 规则原样保留**——超预算仍如实 INCOMPLETE。

**验证**：延迟 digest 与分片逐位一致（digest-check.txt：part digest===index hash、全部分片同 digest、reassembly payload 还原）；t284 selftest 15/15 + reassembly 4/4 在分段版树全绿。

## 6. 非机回归矩阵（设备 NOT_RUN）

| 套件 | 结果 | 说明 |
|---|---|---|
| t311 units | 9/9 | 五档 N/R 对照（原/新权威逐位）+ 收据行为 |
| t311 scaling | 7/7 | **物理结果逐位相同**；转移差断言更新为新合同 `2R-1-floor((R-1)/16)`（复杂度赢余的合同化，非隐式） |
| **T331 死亡复现族** | D1-D4 全 PASS | R=320 单 attempt：apply 64ms、attempt 累计 68ms、单审计事件 ≤1ms（D1 ≤600ms ✓）、sort 1 次、全验 22 次（=1+floor(319/16)+2 ✓）、fallbacks 0 |
| m2-transactions | 14/14 | 事务回归 |
| gesture | 18/18 | 手势注入回归 |
| hot-sink | 17/17 | 沉积/跨列/抬升回归 |
| t284 selftest | 15/15 | 分段化后冷采链 |
| t284 reassembly | 4/4 | 分片重组 |
| attemptCumulativeMs 单元 | 行为全对 | open 刷新/seal 定格/row slot13/export V2 |
| 生产 hvigor | CompileArkTS+PackageHap 绿 | SignHap 00303116 已知如实保留 |

**对照 T312 设备实测**：R=261 时 audit 1165ms/sort 777ms 累计、单事件 1449ms、两 run 死于 THREAD_BLOCK_6S → 增量证书后 R=320 单 attempt 总 68ms（**约 20× 余量对 6s watchdog，超出合同要求的 10×**）。

## 7. 红线遵守与语义对位说明

- 不改 dt/不跳审计/不降阈值：全部保留（counts[7]=R+2 合同不变、1e-9 容差不变、谓词逐条保留）。
- "只移热点"检查：转移差合同**显式更新**并注明来源（每行省一次全池拷贝是增量证书的直接结果，非热点搬迁）。
- scaling.cjs 断言修改属**合同演进**：T311 断言编码的是当时的复杂度合同；T331 改变复杂度合同本身，断言同步并在注释中写明推导。
- T284Cold 分段化：4ms 预算与 INCOMPLETE 规则未动；captureMs 超预算仍 INCOMPLETE。
- 冷采在设备端的实测（captureMs 回 4ms 内）归后续真机卡；本卡非机验证逻辑等价性。

## 8. 包签名状态

本卡全部非机验证，**无签包**（SignHap 00303116 已知失败如实保留，未手签）。设备端验证（watchdog 死亡族设备复核、冷采 captureMs 实测）归后续真机卡，其包由执行 tester 按 T151 流程补签（T328 移交裁定 1）。

## 9. 证据清单（test/reports/t331/）

units.json / scaling.json / death-family.{txt,json} / m2-transactions.txt / gesture.txt / hot-sink.txt / cold-selftest.txt / digest-check.txt / production-build.txt；隔离生成树 `.codex/local-test/t331-final2/`（含全部三件+分段化）。

## 10. 变更记录

- 2026-10-31 v1：初版。三件实施（增量证书 K=16+回退语义、audit-scope sort 增量索引消除、attemptCumulativeMs）+附注两件（包 SHA 勘误、冷采分段化）；非机回归 9/9+7/7+死亡复现族 D1-D4+14/14+18/18+17/17+15/15+4/4 全绿；R=320 attempt 68ms（20× 余量）。
