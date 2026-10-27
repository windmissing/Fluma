# T155 audit 负载诊断独立测试报告

日期：2026-10-27。任务：6adff62ce4b06ee3574bf340（tester，离线，不使用真机）。被测树：HEAD c066ab3（T162 V2 50ebbff + T166 1f8e16c 已在树）。规格：`docs/architecture-T162-diagnostic-gate-disposition-v2.md` §4/§5 + T116 V2§4。

## 执行器与独立点

`test/p155_audit_load_independent.cjs`（20 用例，不镜像 T154 8/8 开发自测断言）：

1. **排序终止语义直接驱动 Index**（不经 pool 注入路径）：独立构造 P011ParticleSnapshot 直接调用 `P011MergeExactIndex.find`，独立复算堆排序比较上界。
2. **seed/audit 链真实分源**：成功/λ 重试/非法 ID 三臂独立构造，计数器与 ring delta 逐项核对。
3. **非法/拒绝与成功分列**，不重复 T128 287pair 样本冒新负载证明。

## 结果：20/20 PASS（原始 JSON：test/reports/t155/independent.json）

### A. 排序终止与比较计数（6/6）

| 用例 | 结果 |
|---|---|
| 满池（40 live）排序终止，比较 629 < 独立上界 1120 | PASS |
| 稀疏池 0/1 live：false 返回且零排序开销（sortComparisons=0，rebuilds 各 1） | PASS |
| 同键 12 粒子（同 phase/source/sourceId/state/x）终止、invalid=false、id 决序选出对 (11,10) | PASS |
| 非法 ID（pId=0）早退 invalid=true，无排序完成宣称 | PASS |
| 重复 ID → invalid=true | PASS |
| NaN/负半径 → invalid=true | PASS |

### B. seed/audit 链真实分源（3/3）

- **成功 seed**（.35 入 col64，nmax=16）：seedCalls=1、三 audit 位点各 1（sinkRow/applyTail/finish）、sortCollectCalls=1、sortAuditCalls=3、birthRequested=4；SEED ring 行 lambdaAttempt=0、lambda=1、error=0、delta[auditFinish]=1。
- **λ 重试链**（不兼容满池）：8 attempts 每次真实分源（seedCalls=8、indexRebuilds=8），auditFinish=0（rollback 后零假完成），末行 lambdaAttempt=7、lambda=1/128、error=-1、delta[indexRebuilds]=1；rollback 后 pool/field/预算与关诊断臂字节级一致。
- **非法 ID 准入**：早退保留 error，sortCollectCalls=0、auditFinish=0，ring 有非零 error 完成行。

### C. attempt delta 与阶段计时/未捕获标记（3/3）

- 缺时钟：clockStatus=NOT_CAPTURED，运行中行 status=RUNNING、startMs/endMs/delta 全 null（不冒实测零）；endSpan(span,9) 后 error=9 保留。
- 单调时钟：MONOTONIC_INJECTED_MS，SEED 行 startMs=12.5/endMs=37 单调，firstLong 捕获、inputStatus=EXACT_STAGE_POOL_BYTES。
- 反向时钟：CLOCK_INVALID，elapsedMs=null。

### D. 冷 ring 容量/dropped（3/3）

- 诊断关：getPoolProgressDiagnostic()=null（无 ring 导出）。
- 溢出：ring 恒 32、first32 恒 32、dropped=614（8 attempts 后）→1260（再注入后增长）。
- 首超长窗口：只捕获一次（inputPoolHex 13 组、exactFullTransactionReplay=false），pool.reset() 后原 hex/hash 冻结不变。

### E. 诊断开关 field/pool/预算/事件等价（1/1）

诊断开（抛异常时钟）/关两实例注入后 field 十数组+pool 快照+source/预算/commit 全等；3 步 step 结果 deepEqual（physicsTimeMs 除外）；CLOCK_UNAVAILABLE 如实导出。

### F. 适配器反例与探针基线（3/3）

- **反例 A/B 源事实**：beginStepHotBody 中 `work.pool.totals().count` 实参确在 `work.fieldColumnEvidenceV2!==null` guard 内的 captureFailure 调用（含 freeSlotCount/nextParticleId 全实参）；`configurePoolProgressDiagnostic` 全函数体不含 fieldColumnEvidenceV2 赋值——**E 独立于 P 的合同语义在源码层成立**（T162 §2/§3.1 事实核验）。
- **旧 conservative 探针保留**：无参 API 输出 NOT_ZERO_ALLOCATION、remaining=35、legacy 60 项完整——历史对照未被 profile 新图改写。
- **OFF_ALL 主图结论：NOT_RUN**——T168 上下文敏感探针（test/tools/p011_hot_context_graph.cjs）在工作树存在但未提交/未交付，按卡正文如实不宣静态结论；不把 T154 8/8 改名冒独立门禁。

## 边界与未覆盖（如实披露）

- 静态项（OFF_ALL 主图 0 remaining/0 unresolved/60 项完整、§4.1 独立反例全注入矩阵）**NOT_RUN**，待 T168 交付后按 §5 结论条件补验；本报告不解除 T162 门禁缺证。
- 动态等价覆盖注入/λ 重试/step 循环；更广的时序负载（完整 seed/audit 链真实分源次数/λ/失败/rollback 已覆盖；attempt delta 与阶段计时已覆盖）未含真机。
- 本报告不含设备结论，供 T156 消费动态部分；完整无阻断结论须待静态项。

## 结论

T155 动态独立验证 20/20 PASS：排序终止/比较计数（满/稀疏/同键/非法 ID 分列）、seed/audit 链真实分源次数与 λ/失败/rollback、attempt delta 与阶段计时/未捕获标记、冷 ring 容量/dropped、诊断开关物理等价全部成立。静态项 NOT_RUN（T168 未交付）。无新缺陷上报；未发现产品或诊断实现问题。
