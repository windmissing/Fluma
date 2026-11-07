# P-011 满池动态 code 5 归因与修复路由（T46）

日期：2026-10-25（Asia/Shanghai）  
责任：designer  
任务：6addc406e4b017ab33eb18dc  
设备：未使用真机  
结论：**已复现的现象是 `TILT_PLANNING_BLOCKED` 规划阻断，不是容量拒绝；现有证据不足以判定为预期阻断还是算法/实现缺陷。没有证据支持“质量丢失”或“T20 改动引入回归”。需 tester 补采 planner 与逐步状态证据后，按本文规则分流。**

## 1. 证据盘点

| 证据 | 观察 | 能说明什么 / 不能说明什么 |
|---|---|---|
| `test/probe_p011_full_pool_contact_failure.cjs` 与 `test/reports/t20-dual/known-fullpool-baseline.json`、`known-fullpool-current.json` | nmax=16；128列、平床；注入同列 H=1.6；seed=1；1/60s步进。baseline `2ef21f9` 和当前工作树都在 step index 73 返回 `ok=false, stepped=false, substeps=0, errorCode=5, errorColumn=-1`。两份报告的池总质量均为 H=1.0226497308103744、L=0，计数11且全为 resting；报告的 water/H/L drift 为约2.05e-12/0/0。 | 同一输入在基线和当前版本同样失败，故不能归因于 T20 当前差异。总量守恒观测不支持质量丢失。报告未保存逐列前后快照，不能证明每个 cell/粒子 ID 的状态不变。step 73 失败时池中只有11个粒子，且未记最大占用，因此不能证明失败时仍处于满池。 |
| `WaterSandField.ets` enum 与规划错误路径（代码行约299、2104–2114、5633–5642） | `errorCode=5` 是 `TILT_PLANNING_BLOCKED`。该错误会令本步 `ok=false` 并走 `restoreBackup()`；特别豁免 `frozen=true`，表示 field 不永久冻结。`planParticleTilt` 在 planner `status='blocked'` 时保存 `blockedReason` 并返回该错误。 | 这不是 `CAPACITY_OVERFLOW` 或 particle allocation 拒绝。§20.4 的语义是本步/本run无效并回滚，但允许调用方决定后续无效延续；“不冻住field”不等于该次run通过或这次失败可忽略。 |
| `docs/task-T31-subjective-particle-e2e-runbook-v1.md` §5.3 | 明确 `TILT_PLANNING_BLOCKED` 按设计不冻结，但未把错误码本身作为产品效果通过。 | 支持区分错误码语义与指标达成；不说明本探针为何在此状态触发。 |
| `docs/developer-T20-dualfield-basic.md`；T02/T30事务与T20双场测试报告 | T20称该长跑复现于 baseline 和当前版本；另有静态满池打包、容量不变式、MERGE释放槽、DEGRADE只扣一次、拒绝不改状态及双场包全量压实等通过用例。 | 这些测试分别验证局部事务和显示包，不证明第73步规划请求为何不可行，也不推翻此长跑失败。 |

## 2. 三种情况的裁决口径

- **正常容量/准入拒绝：** 请求分配没有可用槽时按容量语义拒绝，且粒子池、床层、水量和已提交账本不变；满池时通过合法 MERGE/DEGRADE 获得槽位仍须分别满足质量守恒。这类拒绝不能被 `TILT_PLANNING_BLOCKED` 代称。
- **预期规划阻断：** planner 明确返回 `blocked`，有可复核的 `blockedReason`、输入角度/限制边或预算状态；该步保持原物理状态、不提交子步，field 不进入永久 frozen。此结果仍使本次步/run无效，不能计为完整动态场景通过。若确实处于契约允许的不可行输入，则分类为预期 fail-closed，而非容量错误。
- **算法或实现缺陷：** 在冻结契约定义的可行状态下 planner 仍持续阻断，或其理由与输入/账本不符，指向算法/边界契约；planner 声称可行而实现产生 code5、回滚漏写/多写、粒子或H/L质量/ID破坏、或 field 被错误冻结，指向实现缺陷。逐列/逐ID不变量与提交/回滚记录必须支持该归因。

**当前分类：** 证据能确定错误类型是 planner 阻断，不能证明它是“正常满池拒绝”；也不能从 `code=5` 单独推出算法错误。相同失败出现在 2ef21f9 基线，只排除当前 T20 差异导致的新回归，不代表历史算法行为符合预期。

## 3. 已知事实与未知项

已知：探针是固定种子的离线长跑；失败在第74次调用（零基 step index 73）；失败结果没有提交子步；错误列为 -1；报告中的全局 H/L/水漂移很小；失败后11个粒子均为 resting；同一现象在 baseline/current 均出现。

未知：发生失败时的 `targetTilt/effectiveTilt/effectiveTiltRate`；`getPlanningTelemetry()` 的 `status/blockedReason/passCount/fallbackUsed/thetaRequested/limitingEdge/configRevision`；planner 被判不可行的具体计划和边界；池当步及此前最大占用是否等于16；失败前后逐列床层、水量、悬沙、粒子 ID/质量/状态是否逐值一致；当时是否已完成静置收敛、粒子接触/合并/降级的事件顺序；调用方是否按 run invalid 规则处理该次 code5。

因此当前不能写“已确认算法 bug”，也不能因 runbook 对 code5 的非冻结约定将该轨迹算作正常通过。

## 4. 建议的 tester 补证 prompt（独立补测，未建卡）

> 不使用真机。新增独立诊断报告，不改冻结测试判据、不改业务代码。以 `test/probe_p011_full_pool_contact_failure.cjs` 的固定种子场景复现：128列、平床、nmax=16、同列 heavy 注入1.6、seed=1、dt=1/60、最多360步；同时逐字节记录失败前后 field/pool 状态。每步记录 step result、`getPlanningTelemetry()` 全字段、effective/target tilt与速率、lastError、frozen、提交/回滚标志、池 count/maxCount及逐粒子 ID/phase/state/mass/x/y/vx/vy、逐列 bh/bl/mh/ml/suspH/suspL/waterTop、全局H/L/水账本与事件 MERGE/DEGRADE/LAND/CONTACT_WAIT。复现后添加独立对照：无动态注入平床；相同注入但 nmax=512；同一时刻停止输入并静置；另选一组已有证据证明可行的安全倾角。每组记录首个差异步与至少前后各10步，分别报告容量准入拒绝与 planner 阻断，不能混成一个 code5 统计。结论按本文第2节分流：planner blocked 且理由/输入确属不可行→预期 fail-closed；可行态被错误阻断→designer复核规划算法/契约；planner输出与结果不符或 rollback/invariant/生命周期异常→developer最小返修；采集字段缺失或用例未复现目标状态→tester完善观测/场景。输出原始JSON、环境/源码身份、逐场景 PASS/FAIL/BLOCKED 和明确未证项。

## 5. 后续责任与边界

1. **当前不给 developer 业务修复任务。** 现有证据尚未确定根因；两版本同失败亦不足以裁定需改算法。
2. **优先 tester 补证。** 将独立 prompt 交 manager 建卡；若现有T30后续测试资产足以容纳，则在独立补测项中落地，避免重建已有场景或改原冻结判据。
3. 若补证判为规划算法/契约问题，回 designer 提出带输入域与不可行边界的最小 revision；若实现偏离契约，manager 另派 developer 只修具体差异；如果只属观测不足，tester补齐证据，不派业务修复。
4. T20 静态容量保护、MERGE/DEGRADE单事务及renderer packet结果保持原结论；不将其推广成满池动态长跑通过。T31主观设备场景不能用此离线探针代替。

## 6. 范围与刷新

依据：T46 原卡；T31 执行案；T20 双场开发报告及 full-pool baseline/current JSON；T02/T30/T20 事务、planning 与 packet 测试现有报告；`WaterSandField.ets` 和 `P011TiltPlanningPass.ets` 的只读代码路径。本轮仅分析现存报告/代码，未运行测试、未改产品或测试判据、未操作真机。独立归因文档完成日期：2026-10-25。
