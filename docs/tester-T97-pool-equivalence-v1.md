# T97：池优化独立等价与有界执行 v1

日期：2026-10-26。角色：tester。任务卡：T97。合同：`docs/architecture-T92-pool-freeze-routing-v1.md` §6；被测实现：T96 c0c58c3（对照基线 = 其父 067fd84 的真实源）。执行器：`test/p097_pool_equivalence.cjs`（双版本 A/B 逐值对照）、`test/p097_bounded_replay.cjs`（子进程外部超时重放）；raw：`test/reports/t97/`。不使用真机；不改生产/判据；开发自测（run_t96_pool_hot_units 5/5）不作唯一证据。

## 1. 双版本同池逐值对照（4/4 PASS）

物化两版完整依赖（physics/common/render，各 74 文件，`git show` 逐文件落盘），同一确定性负载（种子 PRNG：随机列/两相 birth、连续 merge、溢出注入、N16 留尾 birth 空位）：

| 工作负载 | 步数 | MERGE 数（NEW） | 对照结果 |
|---|---:|---:|---|
| N16-tail（13 初种+tail birth） | 60 | 3 | **逐步一致**（pool digest/事件/status/free/live/nextId） |
| N16-mix | 60 | 2 | **逐步一致** |
| N512-wide | 40 | 0 | **逐步一致** |

- 每步比较键：`pId/pState/pPhase/pMass/pX/pY/pVx/pVy/pSource/pSourceId/liveCount/nextId/freeCount` 全数组 + 事件流（kind/id/mergedIds/mass）+ step 结果；
- **诊断开/关等价**：NEW 版 40 步 ON/OFF 物理逐值一致；诊断串 ON 时存在（`P011_POOL_PROGRESS_DIAGNOSTIC_V1`，含 mergeCandidateChecks）、OFF 时 null（T97-DIAG）；
- 新 `mergeOldestHot` 的字典序最小合法 `(a,b)` 语义与旧全扫描一致（含重扫 scratch、无匹配回滚）。

## 2. 三臂有界重放（T78）

- **精确重放 = UNPROVEN（如实登记）**：T78 归档（`test/artifacts/t78-dualfield-retest-20261026/`）只有 hilog + 截图；R3 的 505 行 pan_update/pan_end 提供了 localX/localY 触摸序列（已提取归档 `p097_r3_gesture_sequence.json`），但**无初始池/场快照、无 seed、无注入原语记录**——不可精确复现 T78 卡死时的池状态。
- **有界同手势重放**（两版各跑一遍，子进程 + 外部 60s 超时）：NEW 与 OLD 都在 ~2.1–2.3s 内完成 505 触摸事件 × 2 子步 = 1010 步，**均未触发超时/未卡死**（T97-R3-NEW/OLD）。注意这是"同手势序列从新场出发"的有界运行，不是 T78 状态的复演。
- **零源链探针**（gate→clock→request→inject）：当前构建触摸后第 1 步即出现非零源注入、120 步内 pool liveCount=238，全程有界完成（T97-ZERO）——仅记录当前构建链行为；设备零源归因归 T69/T82。

## 3. 结论与解锁边界（按 T92 §6）

- **等价证据**：新旧两版在三类负载下逐步逐值一致 + 诊断开关物理等价 → T96 优化的**离线等价证明到件**；
- **有界执行**：同手势负载下 NEW 有界完成（与 OLD 相同），无 O(N²) 卡死重现；但这是**host 有限负载**，不能当作设备 fps/不冻结的产品证明；
- **T78 精确三臂重放：UNPROVEN**（缺输入/池快照，与开发自认一致）；
- **T94/T57 解锁边界**：等价 + 有界证据已到件，但 T94 的实质前置（同场景设备三臂不冻结、真实准入、CLOSED）仍需**真机串行验证**——本卡不解除，交 manager 按 T92 §6 推进；
- 0 真实失败 → 无 architect 分流项；未改产品/判据；无 Canvas。
