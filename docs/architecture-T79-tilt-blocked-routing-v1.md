# T79：step73 安息角规划阻断工程归因 v1

日期：2026-10-26。角色：architect。原卡：`6adef380e4b0127832ad40f3`。
状态：**归因分流交付；机制尚缺证据，不是产品通过，也未确认业务修复**。不使用真机。

## 1. 裁定与证据边界

消费 T47 `59e64af` 的 `docs/tester-T47-tilt-blocked-evidence-v1.md`、`test/reports/p047_tilt_blocked_evidence.json`，以及 T46 `docs/decision-P011-fullpool-dynamic-rejection-attribution-v1.md`。本轮只读报告和代码，不重跑 tester 测试。最新失败归因顺序由 architect 先分流，取代两份历史报告的“直接交designer”职责文字，不改其原始证据。

已成立：step index73（第74次调用）code5；容量512仍同失败，排除正常容量拒绝；前后逐值回滚、未永久冻结；71–72 LAND/松弛后的床沿到达 tan30；无新请求仍阻断；同状态请求5°可以 accepted。它们不证明旧角0°在完整粒子＋通道计划下必然可行，5°成功也不构成0°应成功的数学证明。

差值约5e-16仅是候选线索。不能直接加epsilon、改安息角、跳过planner、强制setTilt(5)，也不能从“基线同失败”认定预期正确。现有证据不支持回滚缺陷、容量返修或T20回归。

## 2. 从源码缩小分支（重要补充）

已读取 `59e64af:entry/src/main/ets/physics/P011TiltPlanningPass.ets` 的 createPreview，与本次读取的工作区对应控制流一致：

1. PASS1 的 `plans1===null` **立即返回 passCount=1**。
2. T47 实际是 `passCount=2 / fallbackUsed=true`，因此该次 **PASS1 已返回非null计划，进入了候选区间决策**；不能继续把“PASS1 collect/particle失败”列为同等可能的首因。
3. 此结果仍不能区分 PASS1 区间拒绝、可行但钳角需重算、第二候选失败，以及旧角 fallback 的 collect/particle失败或区间拒绝。passCount不是preview调用次数；一次请求可能发生 REQUESTED、CLAMPED、OLD三个调用。
4. `blockedResult` 的最终 limitingEdge=null 不保留内部失败边，不能据此反推没有限制边。

当前区间函数按每边 `gNext=S0[c+1]-S0[c]+deltaS[c+1]-deltaS[c]` 得 `[gNext-H,gNext+H]`，再做全局交集及角速度/定向角区间交集。必须使用**该次计划的deltaS**，不能只看失败前床沿减H。preexisting检查容差与candidate比较精度不同是可检查候选，尚非已确认根因。

## 3. 观测工程补充合同：P011_TILT_PREVIEW_TRACE_V1

本节是精确补证的工程落点，不修改生产planner判定、endpoint/peak协议或物理账本。manager可派一次只增只读诊断的developer工作，再交tester补采；若tester已有可信的隔离源码诊断工具，可直接用测试专用build补齐，须归档patch与身份，不伪称未改源码。

| 文件/hunk | 只读接口与时序 |
|---|---|
| `physics/P011TiltPlanningPass.ets`：createPreview、p011DecideCandidate | 可选诊断sink，默认为null；为每次preview分配本attempt内callOrdinal与role=REQUESTED/CLAMPED/OLD。记录实际调用及未执行原因，不用passCount猜次数；在返回blocked前封存决策。 |
| `physics/WaterSandField.ets`：previewChannelPlansAt | 返回null前记录阶段COLLECT/PARTICLE及实际collectError.code/column、particleStatus；正常标PLANS_READY。保留原返回类型，诊断不得改变控制流或重新apply。 |
| 同文件：planParticleTilt与子步入口 | 在backup后、首次preview前复制 `substepSandTopBefore`；关联runId/stateEpoch/attempt sequence/substep/configRevision、dt、thetaBefore/target/requested、hardMax及现有EPS。 |
| 可选测试专用trace类型/容器 | attempt内有界：至多3次preview、每次cols-1条edge、各通道cols条delta，另一次S0快照。保留原遍历顺序；非诊断运行不拷贝数组。不得在热路径逐边console。 |

每个实际候选记录：collect/particle结果、每通道deltaSandTop和按生产顺序相加的deltaS；每边原始S0差、delta差、gNext、lo/hi、累计lo/hi及取界边；转换后的角区间、maxOmega×dt区间、directedLo/Hi、最终loAngle/hiAngle、feasible/thetaClamped、与候选角的差及接受比较结果。数值保留可往返Float64，关键边提供IEEE754位型或hex辅助，不截到小数6位。同时记录preexisting残差和实际容差，避免把两个判据混称一项。

诊断只旁观已经执行的值；不得调用会递增身份的getter、改变自然观测probe绑定或触发第二遍preview。sink异常只标TRACE_INCOMPLETE，不吞物理错误、不让trace异常变成物理成功；trace在finally恢复完成后交付，复制出的证据不能引用随后复用的scratch数组。开/关诊断两臂须证明物理、budget、ID/freeSlots、sliding状态及提交序列完全相同。不得借诊断把失败状态改为accepted。

## 4. 精确tester补证任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改滴答同名清单。共享代码不得回退他人修改。若涉及画面，仅新XComponent+shader，包括背景/前景；禁止Canvas上屏/回退，失败FAULT或标明STALE的上一有效新管线帧。

消费T47已有四臂，不重复做整体归因。固定59e64af或明确归档全依赖快照与诊断patch/source SHA；复现原128列/nmax16/H1.6/seed1/dt1/60，从step71到73及失败重复调用采§3。保留原raw JSON不覆盖，新增不可变目录。对照原失败状态θ0和θ5，并在复制状态上做诊断开/关等价；不改原状态以“修复”复现。

先报告首个失败阶段和每次实际preview序列，尤其核实PASS1确非null；同时提供S0全数组、per-channel/per-edge值、原错误/回滚/不冻结结果。缺任何值标MISSING，不用0补。只需围绕同一最小反例补证，不扩展设备或大范围参数扫；补证回architect，不要求tester裁定算法根因。

## 5. 到件后的分流表与开发边界

| 可证结果 | 路由与最小范围 |
|---|---|
| preview恢复/缓存/计划输入偏离冻结S0，或错误原因漏传造成观测错误 | 工程问题。architect先补本文件v2的具体差异，再developer仅修WaterSandField preview/save/restore或service入参hunk；不改边界公式。 |
| 真正粒子/collect拒绝 | 追该实际status及事务输入，不能再以“安息角浮点”概括；若对应已实施合同被违反，再给精确工程返修。 |
| S0和计划正确，现行边界/角交集仍排除正常松弛产物 | 设计候选成立，交designer定义安息角合法态、容差一致性与0目标定向区间；保留旧合同证据，明确版本、可行性证明与最小反例。不能由architect先定epsilon。 |
| 输入/计划确属现合同不可行且fail-closed正确 | 登记预期阻断；本run仍invalid，不记动态产品PASS；必要产品可用性要求交designer，不盲派缺陷修复。 |
| 探针混快照/错误读取/用例主张与合同冲突 | 收集明确反例交manager裁决用例，原阈值不私改。 |
| 某项本来尚未实现、只是预期RED | 标NOT_IMPLEMENTED结束该项归因，不把本次已实施planner的真实阻断直接归入此类。 |

若后续工程修复获派，离线回归覆盖原最小轨迹、无注入/容量512/停输/θ5四臂、纯preview不泄漏、preview/commit一致、回滚/非冻结、预算与守恒及原planning单测。原FAIL保留，修复后结果独立归档。不新增manager验收前置。

## 6. 交付结论

T79完成首次工程归因和精确观测工程合同；**业务根因待补证**。后续实质依赖是preview内部证据，不是等待manager验收。本轮不实现业务、不运行测试、不改变用例/指标；三项执行卡中只更新本行交付状态并回原卡。
