# 算法原则：P-011 倾斜跟手性/底层运动响应

> 版本：v0.7　日期：2026-10-18　状态：DESIGN_UPDATED_PENDING_APPROVAL  
> 维护人：designer  
> 问题：P-011（倾斜跟手性/底层运动响应）  
> 本文件登记设计原则、技术路线迭代记录与当前具体算法方案；不改工程代码。

---

## 1. 技术路线与报告隔离原则

1. **方案必须声明技术路线**：P-011 的每一次具体算法方案设计，都必须先声明其所属的唯一技术路线（route），并使用稳定、可追溯的技术路线标识。禁止把多个技术路线的机制混在同一份具体方案中而不加说明。
2. **报告按路线独立成文**：客观测试报告与主观效果报告都必须按技术路线独立成文。不同技术路线不得共用一份测试报告或效果报告，也不得把不同路线的数据合并后作为单一结论。
3. **报告命名必须可路由识别**：报告文件名或文档内首部元信息必须能唯一对应问题编号与技术路线，例如 `test-report-P011-<route>.md`、`effect-report-P011-<route>.md`；同一技术路线的迭代报告可追加版本记录。
4. **不做全量 dominance 比较**：不同技术路线之间不做“在所有指标上全面超过另一路线”的全量 dominance 比较，也不要求某一技术路线的方案必须全量超过另一技术路线。
5. **跨路线比较只作参考**：跨技术路线的对比只能作为参考性说明，可用于描述适用场景、代价差异或感知差异，但不得作为 P-011 任一方案的验收依据。
6. **验收只对同路线闭环**：方案验收指标与结论只在其声明的技术路线内判定；当存在多条技术路线时，应分别形成方案、报告与闭环结论。


---

## 2. 候选技术路线与实施建议

### 2.1 bottom-creep-v2.6

- **routeId**：`bottom-creep-v2.6`
- **状态**：REJECTED（2026-10-15）
- **核心思路**：延续既有底层蠕变机制，重点优化底层蠕变响应、倾角映射与输入平滑，降低突变感并提高倾斜操作时的底层运动跟手性。
- **预期改善指标**：倾斜输入到底层峰值位移的响应时延进一步下降；底层运动的视觉突变率下降；主观跟手性评分提升。
- **被否原因**：
  1. 方向语义存在反向缺陷：当 `effectiveTilt > 0` 时，方案未强制左列为 donor、右列为 receiver；当 `effectiveTilt < 0` 时，也未强制右列为 donor、左列为 receiver，导致方向修正依赖实现层解释，存在左右两侧竞争与方向漂移风险。
  2. 底层蠕变与沉降、渗透、滑移、扬起共享运动预算，倾斜操作高峰期时底层运动被其他运动挤占，出现明显掉帧、截断或跟手不足。
  3. 缺少独立的 bottom creep 预算观测与截断指标，无法量化“预算不足”与“方向/映射错误”这两类问题的归因边界。
- **结论**：不得继续以参数调优方式复用 v2.6；同路线下一版必须先修复 donor/receiver 方向语义，并引入独立预算通道。
- **对应报告**：`docs/test-report-P011-bottom-creep-v2.6.md`；`docs/effect-report-P011-bottom-creep-v2.6.md`

### 2.2 gravity-advection-v1

- **routeId**：`gravity-advection-v1`
- **核心思路**：不再以底层蠕变为主驱动，改为按设备倾角生成直接重力驱动项，形成底层或整层对流运动。
- **预期改善指标**：倾斜方向与底层/整层运动方向的一致性提升；大倾角下运动幅度与响应速度提升；主观物理自然感评分提升。具体阈值在下一版方案中量化。
- **主要风险**：机制改动大，稳定性与性能风险更高；整层对流可能带来不可接受的视觉扰动；端侧算力与时延压力需重新评估。
- **对应报告**：`docs/test-report-P011-gravity-advection-v1.md`；`docs/effect-report-P011-gravity-advection-v1.md`

### 2.3 实施建议

**建议立即实施：`bottom-creep-v2.7`。**

1. v2.7 修复 v2.6 的方向语义缺陷，并补齐独立预算通道与观测指标，改动范围仍小于切换整条路线。
2. 若 v2.7 在限定消融后仍未达成 P95 T90 <= 100ms，停止 bottom-creep 调参，转入 `gravity-advection-v1`。
3. `gravity-advection-v1` 保留为独立备选路线，待其机制设计补齐后再评估实施。

> 上述两条路线的客观与主观报告分别独立成文；本建议仅说明实施优先级，不构成全量 dominance 比较。

---

## 3. 当前实施方案：bottom-creep-v2.7

### 3.1 方案概要

- **routeId**：`bottom-creep-v2.7`
- **状态**：DESIGN_UPDATED_PENDING_RETEST（2026-10-17 TC02 复测失败已完成设计归因，等待测试口径修正与复测）
- **核心思路**：在 v2.6 的底层蠕变机制上，修复 donor/receiver 方向语义，新增独立的 `vBottomCreepMax` 预算子通道，并保留全局总运动硬上限。底层蠕变不再与沉降、渗透、滑移、扬起互相挤占预算，但仍不能突破总运动硬上限。
- **对应报告**：`docs/test-report-P011-bottom-creep-v2.7.md`；`docs/effect-report-P011-bottom-creep-v2.7.md`

### 3.2 方向修正规则

1. 设 `effectiveTilt` 为已完成死区、平滑与符号统一后的有效倾角。
2. 当 `effectiveTilt > 0` 时：左列固定为 donor，右列固定为 receiver；质量/位移从左列向右列迁移。
3. 当 `effectiveTilt < 0` 时：右列固定为 donor，左列固定为 receiver；质量/位移从右列向左列迁移。
4. 当 `effectiveTilt == 0`、处于死区或输入无效时，不产生新的 donor/receiver 迁移。
5. donor/receiver 的判定不得由列间当前高度差反转；高度差只允许影响迁移量的限幅或缓释，不允许改变由 `effectiveTilt` 决定的方向语义。

### 3.3 预算通道设计

1. 新增独立预算子通道 `vBottomCreepMax = 10 列/秒`；这是 v2.7 的首轮固定值，同一路线内的参数敏感性验证不得改变该基准配置。
2. `vBottomCreepMax` 只约束 bottom creep 造成的列位移速率，不得被沉降、渗透、滑移、扬起消耗，也不得从这些运动借用预算。
3. 保留总运动硬上限 `totalMotionBudget`：所有运动类型共享该硬上限。bottom creep 即使未达到 `vBottomCreepMax`，若总运动预算耗尽，也必须被总上限截断。
4. 截断顺序建议：先按本帧各运动类型的申请比例归一化，再由 `totalMotionBudget` 统一截断；bottom creep 的独立预算用量必须在其被总上限二次截断后统计，反映真实有效位移。
5. 端侧实现需避免额外全列扫描：可在现有列更新循环内累计预算，不得为预算统计新增二次遍历。

### 3.4 观测指标扩展

`P011_METRIC` 必须新增以下字段：

| 字段 | 含义 | 单位 |
|---|---|---|
| `bottomCreepBudgetUsed` | 统计窗口内 bottom creep 实际消耗的独立预算 | 列 |
| `bottomCreepBudgetLimit` | 统计窗口内 bottom creep 可用的独立预算上限，来源为 `vBottomCreepMax` 换算 | 列 |
| `bottomCreepBudgetClipped` | 因独立预算上限导致的 bottom creep 申请被截断量 | 列 |
| `totalMotionBudgetUsed` | 统计窗口内所有运动共同消耗的总预算 | 列 |
| `totalMotionBudgetClipped` | 因总运动硬上限导致的所有运动申请被截断量 | 列 |

补充约定：

1. `bottomCreepBudgetClipped` 只统计独立子通道截断，不包含总运动硬上限造成的二次截断。
2. 总上限造成的 bottom creep 二次截断量建议另行记录到调试明细；若只保留一个字段，则必须在报告中写明统计口径。
3. `bottomCreepBudgetUsed / bottomCreepBudgetLimit` 与 `totalMotionBudgetUsed / totalMotionBudgetLimit` 必须在同一时间窗口、同一样本集合内统计，禁止混合帧与秒级口径。

### 3.5 输入与输出数据

- **输入**：`effectiveTilt`、列当前状态、`vBottomCreepMax`、`totalMotionBudget`、本帧沉降/渗透/滑移/扬起的预算申请、帧间隔 `dt`。
- **输出**：每个 donor/receiver 列的有效位移、`bottomCreepBudgetUsed`、`bottomCreepBudgetLimit`、`bottomCreepBudgetClipped`、`totalMotionBudgetUsed`、`totalMotionBudgetClipped`、方向正确性标记、P95 T90。

### 3.6 伪代码

```text
for frame:
    tilt = smoothedEffectiveTilt(rawTilt)
    if tilt == 0:
        record no bottom creep direction
    else:
        if tilt > 0:
            donorColumns = leftColumns
            receiverColumns = rightColumns
        else:
            donorColumns = rightColumns
            receiverColumns = leftColumns

        requested = computeBottomCreep(tilt, donorColumns, receiverColumns, dt)
        requestedAmount = sum(abs(requested column motion))

        bottomLimit = vBottomCreepMax * dt
        bottomGrant = min(requestedAmount, bottomLimit)
        bottomIndependentClip = requestedAmount - bottomGrant
        bottomGrant = distributeScaled(requested, bottomGrant)

        otherRequests = sum(settlement, percolation, slip, rise requests)
        totalLimit = totalMotionBudget * dt
        totalRequested = bottomGrant + otherRequests
        totalGrant = min(totalRequested, totalLimit)
        totalClip = totalRequested - totalGrant
        grants = distributeScaled(all requests, totalGrant)

        apply grants to columns
        record P011_METRIC(
            bottomCreepBudgetUsed = sum(abs(bottom creep grant after total clip)),
            bottomCreepBudgetLimit = bottomLimit,
            bottomCreepBudgetClipped = bottomIndependentClip,
            totalMotionBudgetUsed = totalGrant,
            totalMotionBudgetClipped = totalClip
        )
```

### 3.7 验收指标与测试方法

**硬门槛（必须全部达成）**

1. **方向正确率 = 100%**：在 `effectiveTilt > 0` 的采样中，左列净输出为 donor、右列净输出为 receiver；在 `effectiveTilt < 0` 的采样中，右列净输出为 donor、左列净输出为 receiver。测试需覆盖至少 200 个非死区采样，并包含倾角符号切换。
2. **独立预算隔离 = 100%**：仅触发 bottom creep 时，`bottomCreepBudgetUsed <= bottomCreepBudgetLimit`；同时注入满载沉降/渗透/滑移/扬起时，bottom creep 独立可用预算下降比例不超过 1 帧统计误差或 2%。
3. **总运动硬上限不可突破**：`totalMotionBudgetUsed <= totalMotionBudgetLimit`，违例率为 0；至少 3 个高压用例出现 `totalMotionBudgetClipped > 0` 时，系统不崩溃、不超时。
4. **P95 T90 <= 100ms**：从有效倾角越过死区到首个 bottom creep 有效位移的 T90 分布，P95 不超过 100ms。

**软门槛（及格线）**

1. 主观跟手性评分不低于 6/10；若 v2.6 存在有效同路线主观数据，则 v2.7 相对 v2.6 不下降。
2. 底层视觉突变率不高于 v2.6；若 v2.6 无有效客观数据，则帧间列位移突变率不超过 5%。
3. 高压场景下 `bottomCreepBudgetClipped / bottomCreepBudgetLimit <= 30%`；若超过 30%，必须在同一测试中证明将 `vBottomCreepMax` 从 10 调整到 12 后仍无法降低截断率，才可判定为预算参数上限问题。

**测试方法**

1. 客观测试使用固定种子回放，覆盖小/中/大倾角、左右往复、死区边缘、满载多运动并发、低帧率与高帧率。
2. 每个用例输出逐帧 `P011_METRIC`，统计 P95 T90、预算违例率、独立截断率与总截断率。
3. 主观测试在不知道技术路线版本的条件下评估跟手性、方向一致性、突变感与稳定性，采用 10 分制。
4. v2.7 的闭环基线只能是同路线上一版 `bottom-creep-v2.6` 的报告；`gravity-advection-v1` 只能作为定性选型参考，禁止作为 v2.7 的验收依据。
5. 报告必须独立成文：`docs/test-report-P011-bottom-creep-v2.7.md` 与 `docs/effect-report-P011-bottom-creep-v2.7.md`，不得与 v2.6、`gravity-advection-v1` 或其他路线合并结论。

### 3.8 失败分流

1. 若方向正确率未达 100%，优先判定为实现未按 `effectiveTilt` 固定 donor/receiver 语义执行，反馈 architect 修订实现。
2. 若 P95 T90 > 100ms 且方向、预算与指标实现均正确，先仅在 `vBottomCreepMax = 8/10/12` 三档内做有限消融；若仍无法达标，**立即停止 bottom-creep 路线调参，转入 `gravity-advection-v1`**，不得继续扩大平滑窗口或预算参数搜索。
3. 若验收失败但测试用例本身存在口径错误，上报 manager 决策，不得私自修改用例。

### 3.9 2026-10-16 final signed-HAP first-layer attribution

- **Verdict:** Keep the bottom-creep-v2.7 physical direction semantics and do not start a v2.8 algorithm design. The implementation maps `effectiveTilt > 0` to left-donor/right-receiver and `effectiveTilt < 0` to right-donor/left-receiver, matching the intended physical mass transfer.
- **Observation contract defect:** `bottomCreepDonorEffectiveDisplacement` and `bottomCreepReceiverEffectiveDisplacement` must be signed global left/right displacement, not positive magnitudes. With `directionSign=+1` for left-to-right and `-1` for right-to-left, each effective transfer must record `donor += -directionSign * amount` and `receiver += directionSign * amount`. The current implementation does `+= amount` for both fields, contradicting the architecture example and checker expectation and causing all 559 direction errors.
- **Sign-switch counting defect:** Opposite tilt commands are separated by `0 degrees`, and the checker clears the previous direction whenever `bottomCreepBudgetUsed <= 0`. Therefore adjacent nonzero samples with opposite signs cannot occur. The zero count is an observation blind spot, not an independent physical failure.
- **TC-P011-02 attribution:** The final run uses `+88/0/-88` phases of 700 ms, not the prescribed `0/+20/-20/0` sequence with 2 s/4 s ramps, 1 s holds, and 5 repetitions. Thus `pearsonH=0.3867486705` and `pearsonL=0.1434990221` cannot reject the algorithm. The checker synthetic self-test also fails, showing that its fixture contains activity noise unrelated to bottom creep. Do not change the `r >= 0.70` acceptance threshold yet; rerun the original case or ask manager to decide whether the test caliber must change.
- **Routing:** Send the direction and sign-switch defects to architect to fix observation-field signs and checker sign-switch counting. Send TC-P011-02 to tester to rerun the prescribed sequence and repair the synthetic self-test first. Developer must not tune the physical algorithm based on this result.

### 3.10 2026-10-16 original-sequence rerun first-layer attribution

- **Verdict:** Do not start `bottom-creep-v2.8` and do not change the v2.7 algorithm, parameters, or acceptance thresholds. The original-sequence FAIL is not yet valid evidence of a physical-response defect.
- **TC-P011-02:** The low Pearson values cannot be attributed to insufficient algorithm response. The script drives integer-degree targets in a tight `aa start` loop, contrary to the required at-least-one-frame spacing, so `effectiveTilt` moves as sparse 1/2 degree impulses and many overlapping Wants are consumed by last-writer-wins. The checker then expands each impulse to a plus/minus 200 ms window, mixing `A(t)=0` samples with bottom movement driven by the persistent tilt state. In this run there are only 264 nonzero input-activity samples but 4404 samples enter the correlation window. This is a test-driving and observation-caliber mismatch, not a conclusive physical-response failure. `feL=0.3490` and the 86.90% independent clip rate remain response-shape signals to re-evaluate only after a valid continuous-motion fixture exists.
- **bottomDirection:** The signed displacement semantics are now correct. For positive tilt, donor values are negative and receiver values are positive; for negative tilt, donor values are positive and receiver values are negative, exactly matching `donor += -directionSign * amount` and `receiver += directionSign * amount`. The checker nevertheless sums donor plus receiver separately over the left and right spatial halves and requires right-positive/left-negative for both signs. That spatial aggregation has a boundary bias and marks the negative-direction phase wrong. Sample-level reclassification shows positive-direction samples pass while negative-direction samples fail; therefore the 2360/4777 errors are a checker/observation-caliber defect, not a remaining signed-displacement defect.
- **Independent budget isolation and high-pressure evidence:** Both failures are caused by insufficient scenario coverage. The run contains zero `totalMotionBudgetClipped > 0` samples, so `fullLoadSamples=0` and `totalMotionClipRuns=0`; it never injects the required full-load concurrent settlement/percolation/slip/suspension motion or at least three independent high-pressure clipping segments. These MUST-PASS failures do not establish an algorithm defect.
- **Routing:** Report the checker direction aggregation and TC-P011-02 activity-window/input-driving defects to manager for test-caliber decision. Add the required full-load and high-pressure scenarios to the objective suite. Do not dispatch developer to tune the physics; no algorithm-scheme change is warranted in this attribution layer.

### 3.11 2026-10-16 final formal-suite first-layer attribution

- **Verdict:** Keep `bottom-creep-v2.7` and do not change its algorithm, parameters, or acceptance thresholds. This FAIL does not establish a physics, parameter, or budget defect.
- **maxSubstep:** Each pressure log contains exactly one 473-511 ms telemetry discontinuity at an app/scenario restart boundary. The checker treats adjacent `P011_BOTTOM` records as one physical substep without requiring continuity, so it diffs the final state of the previous run against the initial state of the next run. All reported 41-70 unit maxima occur across that boundary. Recomputing only adjacent records with a gap of at most 100 ms gives maxima below 0.27 in every pressure log, with zero values above 5. This is a test-fixture/observation continuity defect, not a `vBottomCreepMax`, `totalMotionBudget`, or bottom-creep algorithm failure.
- **uncommanded-tilt:** The 81.988-degree residual is at the same restart boundary: `targetTilt=88`, the previous `effectiveTilt=88`, and the next run's `effectiveTilt=6.012`, with a 473 ms telemetry gap. Excluding that discontinuity leaves a maximum residual of 0 in all pressure logs. It is not evidence of input-rate or tilt-control misbehavior.
- **bottom-direction:** All pressure logs report `bottomDirectionError=0`. The `bottomDirectionCorrectAll` failures in the full-load and positive-segment logs come only from requiring `bottomDirectionSignSwitches > 0`; those logs intentionally contain one nonzero command sign. The negative and repeat logs each have one sign switch and pass. This is a per-log observation/coverage mismatch, not a donor/receiver direction defect.
- **Architecture contract:** The checker must not compare `P011_BOTTOM` or `P011_METRIC` records across non-contiguous telemetry. Add an explicit continuity/run boundary contract: either emit a monotonically increasing physics sequence/frame/substep ID plus `runId` and a reset marker, or require the checker to split logs on a positive `runId` change/reset marker. `physicsTime` is not sufficient because it is wall-clock based and can remain ordered across app restarts. Max-substep must be computed only within the same run and adjacent logical substeps; scenario resets are excluded from smoothness statistics. A single-sign pressure log must not independently require a direction sign switch; require sign-switch coverage at suite level, not per log.
- **Routing:** Send the continuity/run-boundary and suite-level sign-switch observation contract to architecture, then have tester revise the fixture/checker execution caliber through manager approval. Do not dispatch developer to tune implementation or physics.

### 3.12 2026-10-17 new-schema formal-rerun first-layer attribution

- **Verdict:** Keep `bottom-creep-v2.7` and do not start a v2.8 algorithm design. This rerun has complete identity, parsing, ordering, route, direction, stability, conservation, and budget evidence; the remaining FAILs are objective-suite coverage and aggregation-caliber defects, not physics or budget-behavior defects.
- **total-clip segmentation:** The four pressure runs are independent `runId`/reset/target-phase logs, each driven by one constant high-pressure tilt. A sustained clipped interval is therefore one valid segment. Their observed counts are 1/2/1/1, for five independent suite segments, satisfying the documented suite-level minimum of three; independent logs are valid segment boundaries. Requiring at least three segments inside every single-sign pressure log contradicts the suite contract and turns continuous valid clipping into a failure.
- **sign-switch coverage:** `PressureSegments` intentionally contains one sign per run and does not run TC02. Sign transitions must be observed within one run, not synthesized across independent run boundaries. TC02 contains the prescribed repeated `0 -> +20 -> -20 -> 0` sequence, but the current suite invocation omits it from checker inputs even in `All` mode. The zero suite counts are execution/suite-design coverage failures, not behavior-design failures; direction error is zero in all 8,138 pressure direction samples.
- **Required test corrections:** Run the formal objective suite in `All` mode; include the TC02 log in suite inputs; keep pressure logs as independent high-pressure evidence; and make checker gates suite-level rather than per-log. Aggregate `totalMotionClipRuns` across pressure logs with distinct `runId`/reset boundaries and require at least three suite segments. Aggregate positive-to-negative and negative-to-positive switches only from transitions within the same run/reset segment, with TC02 supplying both directions; do not count transitions across logs or reset boundaries. Per-log results should report counts and direction correctness, while `allLogsPass` must not require every single-sign pressure log to contain three clip segments or any sign switch.
- **Routing:** Send the suite fixture and checker aggregation corrections to manager for tester execution; no developer physics change and no algorithm/parameter/threshold change is warranted.

### 3.13 2026-10-17 TC02 formal rerun design attribution and correction

- **Verdict:** `design-updated`. Keep `bottom-creep-v2.7`; do not start `bottom-creep-v2.8`, tune physics, change `vBottomCreepMax`, or change donor/receiver semantics. The five-log run proves complete telemetry identity, 100% direction accuracy, zero budget violations, and suite-level high-pressure clip coverage. The remaining decisive failures come from suite applicability and observation state-machine design.
- **independentBudgetIsolation conflict:** The checker evaluates `fullLoadSamples > 0` as a per-log MUST-PASS. The TC02 log is the prescribed variable-tilt behavior case and intentionally has no concurrent settlement/percolation/slip/suspension request, so `fullLoadSamples=0` is expected and must be `NOT_APPLICABLE`, not `FAIL`. The independent-budget hard gate belongs to the designated full-load log at suite level; that log has 2,033 full-load samples, zero isolation violations, and zero budget violations. The defect is scope binding, not insufficient evidence and not a budget-behavior failure.
- **sign-switch design defect:** Direction-switch state must persist across command events within one log/run/reset segment. The current phase key contains each input `sequence`, so every accepted Want starts a new phase and clears `lastNonZeroBottomDirection`. TC02 contains repeated `0 -> +20 -> -20 -> 0` phases and 12,915 correctly directed bottom samples, but this design records zero in-run switches. `0°`, dormant, dead-zone, and zero-budget samples must be skipped without clearing the last nonzero direction; only a new log, `runId`, or reset epoch may clear it. Single-sign pressure logs are expected to report zero switches and must not fail for that reason.
- **TC01/TC02 tiering:** The current checker applies behavioral tier scores to full-load and pressure logs whose scenarios are not designed to exercise the behavior profile, and TC02's constant-rate ramp makes Pearson undefined when activity is measured as frame-to-frame tilt delta. Scores must be computed only on the designated TC02 behavior log. TC01 must detect dead-zone crossings at run/reset scope, and TC02 must correlate signed target-tilt level with signed net bottom creep displacement on non-dead-zone, nonzero-budget samples. The score thresholds remain unchanged: TC01 `P95 T90 <= 100ms`, TC02 `r >= 0.70`, `|lag| <= 66.7ms`, `FE <= 0.15`, and suite minimum `TC01/TC02/TC03 >= 6`.
- **Correction contract:** Classify logs by suite role: TC02 is `behavior`; `full-load` is `budgetIsolation`; the pressure logs are `pressureClip`. Per-log gates evaluate only applicable evidence and return `PASS`, `FAIL`, or `NOT_APPLICABLE`; suite gates aggregate role-specific evidence. `independentBudgetIsolation` passes only when the designated full-load log has `fullLoadSamples > 0`, zero isolation violations, and no bottom/total budget violation. `suiteSignSwitchCoverage` passes only when the designated TC02 log records at least one positive-to-negative and one negative-to-positive switch within the same run/reset segment. High-pressure coverage continues to aggregate pressure logs and require at least three independent clip runs.
- **Why this is not threshold relaxation:** No failed metric is reclassified as PASS without evidence. The same full-load physical hard gate remains mandatory, but its evidence source is corrected to the log that actually injects full load. Sign-switch coverage remains bidirectional and must be observed in one run rather than synthesized across logs. TC01/TC02 minimum score remains 6 and all numerical thresholds remain unchanged; non-behavior logs are marked inapplicable instead of being given artificial scores.
- **Routing:** Manager must approve the checker/test-suite caliber change. The checker owner updates role binding, applicability, and state machines; the test-script owner preserves `-Mode All` and the five-log input contract. No `WaterSandField`, `SandCanvas`, physics parameter, telemetry field, or acceptance threshold change is authorized. Tester reruns the complete suite after checker self-check regressions pass.

---

## 4. 下一迭代设计：bottom-creep-v2.7.3 behavior-stability

> **routeId**：`bottom-creep-v2.7.3`  
> **日期**：2026-10-18  
> **状态**：`REJECTED(subjective HG-TT-04)`；客观 suite PASS 不能覆盖主观硬门槛失败。  
> **基线**：同一 `bottom-creep` 技术族内的 `bottom-creep-v2.7.2 behavior-stability`。本节不与 `gravity-advection-v1` 或其他路线合并数据。

### 4.1 2026-10-18 v2.7.2 复测事实与 v2.7.3 归因

1. `TC01=10`、`TC02=0`、`TC03=10`，硬门槛与 suite 聚合全部 PASS；唯一失败是 `behaviorTierMinimum`。
2. `activitySamples=400`、`correlationSamplesH=400`、`correlationSamplesL=400`，但 `pearsonH=null`、`pearsonL=null`。
3. `FE_H=0.023496788213169695`，`FE_L=0.20549138771581632`；L 超过既有 `0.15` 上限。
4. `direction accuracy=100%`、`direction error=0`，身份、预算、守恒、稳定性、角色隔离与符号切换覆盖均不构成阻塞。
5. Pearson `null` 的直接原因是 checker 仍在 `test/p011_check.py` 中使用 `|ΔeffectiveTilt|` 与逐子步位移；v2.7.2 设计要求的 `A_corr=signed(targetTilt)`、相位内带符号累计净位移与方差预检尚未落入 checker。该口径错误不得转化为算法结论。
6. `FE_L` 超标说明在同一总预算与 donor 组成约束下，light 通道的累计位移分布落后于 light 目标分布；这属于响应形状问题，不能通过 checker 删除样本、平滑或重采样修正。

### 4.2 有效 Pearson 前提与复测口径

复测仍使用 TC02 的 5 轮 `0 -> +20 -> -20 -> 0` 连续目标角序列，保持同一 400 活动样本口径。有效 Pearson 必须同时满足：

1. **样本一致**：H 与 L 使用同一 400 个候选样本；样本来自同一 `routeId`、`runId`、`resetEpoch`，禁止跨 log、跨 run、跨 reset 拼接。
2. **特征非退化**：`A_corr(t)=signed(targetTilt)`；checker 禁止继续使用 `|ΔeffectiveTilt|` 作为 TC02 state-following 输入特征。
3. **响应非退化**：`R_corr,H(t)` 与 `R_corr,L(t)` 均为当前命令相位内、以相位起点为零的带符号累计净底层位移，且各自方差大于 `1e-12`。
4. **有限值**：所有参与样本的 `A_corr`、`R_corr,H`、`R_corr,L` 必须是有限数。
5. **相位覆盖**：正相位与负相位各至少 30 个有效样本，H/L 每通道至少 30 个非零响应样本。
6. **预检失败处理**：若任一方差为 0、样本不足或存在非有限值，复测判定为 `INCOMPLETE(degenerateMeasurement)`，不得给 TC02 计 0 分后继续用无效 Pearson 判定物理算法。

### 4.3 行为与信号设计

1. **状态一致性**：命令相位状态只在 `runId/resetEpoch` 变化或目标符号实际翻转时重置；`0°` hold、dormant no-op 与零预算样本不得清空上一有效方向或相位状态。
2. **时序窗**：响应整形窗口固定为 `100ms`，最大允许滞后仍为 `66.7ms`；窗口内只做物理响应整形，不在 checker 侧平滑验收数据。
3. **熵源归一**：Pearson 使用 `A_corr=signed(targetTilt)` 与当前命令相位内、以相位起点为零的 `R_corr,H/L=signed cumulative net bottom displacement`；FE 使用 `|A_corr|` 与 `|R_corr,H/L|` 的归一化累计序列，保持 H 与 L 在同一能量口径下比较。
4. **命令相位处理**：新增 `commandPhaseId` 与 `commandPhaseKind`，相位只在符号翻转或 run/reset 边界变化；同一相位内的每个 Want 不新开相位，避免把连续 ramp 切碎。
5. **总请求整形**：保留 `kTarget=0.5 列/(°·秒)` 与 `vBottomCreepMax=10`。新增 H/L 双通道残差：`C*_H=C*_L=∫ kTarget*|targetTilt| dt`，`C_H` 与 `C_L` 分别累计本子步 heavy/light bottom creep 实际位移，`e_H=C*_H-C_H`，`e_L=C*_L-C_L`。请求速率为 `clamp(kTarget*|targetTilt| + max(e_H/tauH, e_L/tauL), 0, vBottomCreepMax)`；`tauH=100ms`，`tauL=60ms`。`|targetTilt| <= bottomCreepDeadZone` 时请求为 0，且理想量、实际量与残差只在 run/reset 或目标符号翻转边界清零。
6. **通道内分配**：总请求仍先受 donor 余量、receiver 容量、独立 `vBottomCreepMax` 与 `totalMotionBudget` 截断；截断后的本子步总量按固定顺序先满足 light 残差、再满足 heavy 残差，但每个通道的实际转移都不得突破该 donor 的 heavy/light 存量与 receiver 容量。禁止新增 light-only 预算、绕过总预算、改变 donor/receiver 方向语义，或改变三相组成守恒。
7. **设计目标**：在不改变方向语义、独立预算上限、总预算上限与及格线的前提下，将 `FE_L` 的设计余量压到 `<=0.12`；正式验收仍维持 `<=0.15`。

### 4.4 验收标准

以下标准全部沿用现行分档规则，不做任何放宽：

1. `pearsonH` 与 `pearsonL` 均必须为有限数值，且各自 `>=0.70`。
2. `|lagH|` 与 `|lagL|` 均必须 `<=66.7ms`。
3. `FE_H` 与 `FE_L` 均必须 `<=0.15`；其中 L 必须从 `0.20549138771581632` 降至不超过 `0.15`，设计目标不超过 `0.12`。
4. `TC01`、`TC02`、`TC03` 分档最低分必须 `>=6`，或按现行分档规则得到不低于及格线的结果。
5. `direction accuracy` 必须保持 `100%`，`direction error=0`。
6. 复测报告必须独立成文为 `docs/test-report-P011-bottom-creep-v2.7.3.md` 与 `docs/effect-report-P011-bottom-creep-v2.7.3.md`，不得与 v2.7、v2.7.1、v2.7.2 或其他 route 合并结论。

### 4.5 本轮边界

- 本文档修订不改业务代码。后续实施时，业务实现只允许升级 `routeId=bottom-creep-v2.7.3`、新增 H/L 本地累计状态与通道内残差分配；`vBottomCreepMax`、`totalMotionBudget`、donor/receiver 语义与遥测字段集合不变。
- checker 允许按 §4.2 与 §4.3 更新特征、相位边界、方差预检、Pearson、lag 与 FE 计算；不得平滑、重采样、删除样本或改变阈值。

---

## 5. 下一迭代设计：bottom-creep-v2.8-heavy-interface

> **routeId**：`bottom-creep-v2.8-heavy-interface`  
> **日期**：2026-10-18  
> **状态**：`DESIGN_PENDING_APPROVAL`；本轮仅更新算法设计文档，不改业务代码、manager 文档或测试报告。  
> **基线**：同一 `bottom-creep` 技术族内的 `bottom-creep-v2.7.3 behavior-stability`。本节不与 `gravity-advection-v1` 或其他路线做 dominance 比较。  
> **结论**：v2.7.3 的客观 PASS 与主观 FAIL 不矛盾：客观指标确认了聚合底层转移，但没有确认用户可见的绿色 heavy 底层移动；v2.7.3 因此 REJECTED，需要行为变更后的新 route。

### 5.1 v2.7.3 失败归因（证据链）

1. **绿色视觉层绑定的是 heavy deposited `bh`，不是聚合底层位移**。渲染代码将 `bh` 画为底部绿色带，将 `bh+bl` 画为上方黄沙带；因此绿色边界的变化只能来自 `bh`。
2. **算法把授予的 bottom creep 优先从 `bl` 取出**。`splitBottomLightFirst` 先消耗 `bl`，仅当 `bl` 不够时才消耗 `bh`。thick 预设下 `bh=72`、`bl=72`，有效倾斜期间 `bl` 未耗尽，所以 `bh` 可以长期保持 72。
3. **客观 suite 自己已经暴露了该事实**：`test/artifacts/p011-formal-prep-20261016/p011-suite.optimized.json` 的 behavior 记录为 `maxSubstepBh=0.0`、`maxSubstepBl=0.029000537833468343`。正式客观样本中 heavy 底层没有发生子步变化，只有 light 底层变化。
4. **主观日志与客观结论一致**：对 `demo/effect/P011-v273/` 的 8 个有效 hilog 逐一统计，所有 run 的 `bh` 范围均为 0；`bl` 的最大高度范围为 `1.72485249182122` 至 `3.47041291657493`。这解释了“水/黄沙有响应、绿色底层完全静止”。
5. **客观 TC02 的 H/L 标签不能区分 heavy 与 light**：checker 将同一个聚合 receiver 位移同时累加为 `response_h` 与 `response_l`，导致 `pearsonH=pearsonL`、`FE_H=FE_L`。它能证明“有底层材料向正确方向转移”，不能证明“绿色 heavy 界面移动”。
6. **TC03 不要求 heavy 必动**：现行 TC03 以 `max(maxSubstepBh, maxSubstepBl)` 判定平滑性；`bh=0`、`bl>0` 仍可得到满分。这不是测试用例错误，而是该门槛的观测对象是跳变/平滑，不是绿色层可见运动。
7. **排除项**：现有证据不支持“viewport/颜色遮罩”“渲染接线错误”“route gating 错误”或“算法完全没有底层运动”。routeId 一致，水/黄沙视觉与日志变化方向一致，真实 bottom flux 存在；问题在于相位分配与客观特征口径。
8. **次要事实**：effect 侧 128 列的 `P011_BOTTOM` 行在 donor/receiver 数组后段被 hilog 截断，但 `bh/bl` 数组完整，且截断不是本次绿色静止的原因。后续必须用紧凑标量遥测避免长数组截断。
9. **证据卫生问题**：客观报告记录 source commit `bb854c21a260e7ace92edc99084fbdac736d81d7`，主观报告与本轮任务记录基线 commit `fdc819045f66bdc666b0f9b1fa8e9d65c3760934`；两份报告的 signed HAP SHA256 相同。该不一致不得作为修改结论的依据，但 v2.8 正式复测必须同时记录 source commit、HAP SHA256 与每个 log SHA256。

### 5.2 假设

- **ASSUMPTION-1**：`demo/effect/P011-v273/` 主观评价所指“green bottom layer”就是渲染中的 `#24571F` heavy deposited 带，即 `bh` 边界。
- **ASSUMPTION-2**：在现有独立/总预算内，将聚合 flux 的至少 35% 分配给 heavy，可在 2 秒级有效倾斜内产生 `>=1.0` model unit 的 `bh` 形状变化；该假设需在 v2.8 实现后的单场景预检中校准，不允许靠提高预算达成。
- **ASSUMPTION-3**：`1.0` model unit 的 `bh` 边界变化在当前 viewport 映射下至少约为 2px。正式验收前 tester 必须用同分辨率 ROI 实测换算，不允许仅凭该假设闭环。

### 5.3 新路线机制：phase-balanced heavy-interface creep

#### 5.3.1 核心思想

v2.7.3 的错误不在 donor/receiver 方向，而在“授予量全部由 light 优先承担”。v2.8 保持方向语义、守恒和预算不变，将每个 donor→receiver 的授予量拆成 heavy/light 双通道，并给 heavy 一个下限份额，使绿色 heavy 界面与 yellow light 界面同步响应倾斜。

#### 5.3.2 数学模型

对每个相邻 donor/receiver 的本子步授予量 `a >= 0`：

1. 计算 heavy 基准份额：
   `wH0 = clamp(bh[donor] / max(bh[donor] + bl[donor], eps), 0.35, 0.65)`。
2. 若 `eH > eL`，将 `wH0` 向上限偏置；若 `eL > eH`，向下限偏置；仍限制在 `[0.35, 0.65]`：
   `wH = clamp(wH0 + kBias * (eH - eL), 0.35, 0.65)`，`kBias` 的实现值由 developer 固化为常量，禁止逐帧调参。
3. 拆分：
   `aH = min(a * wH, bh[donor], a)`；
   `aL = min(a - aH, bl[donor])`；
   `aRemainder = a - aH - aL`；
   若 donor 某相不足，允许 remainder 转移到另一相，但仍受该相存量、receiver 容量、独立 bottom 预算与总 motion 预算约束。
4. 状态更新：
   `bh[donor] -= aH; bl[donor] -= aL`；
   `bh[receiver] += aH; bl[receiver] += aL`。
5. 命令相位内累计：
   `C_H += directionSign * aH`、`C_L += directionSign * aL`；同相位内不因 0° hold、dormant no-op 或零预算清零，仅在 run/reset 或目标符号翻转时清零。

保留 v2.7.3 的双通道残差请求整形：`Cstar_H = Cstar_L = ∫ kTarget * |targetTilt| dt`；`requestRate = clamp(kTarget * |targetTilt| + max(eH/tauH, eL/tauL), 0, vBottomCreepMax)`；`eH=Cstar_H-C_H`，`eL=Cstar_L-C_L`。`kTarget`、`tauH=100ms`、`tauL=60ms`、`vBottomCreepMax=10`、`totalMotionBudget`、dead zone、donor/receiver 语义与现有客观分档阈值均不变。

#### 5.3.3 伪代码

```text
for each physics substep:
    update tilt and command phase
    if |targetTilt| <= deadZone:
        requestRate = 0
    else:
        idealStep = kTarget * |targetTilt| * dt
        Cstar_H += idealStep
        Cstar_L += idealStep
        requestRate = clamp(kTarget*|targetTilt| + max(eH/tauH, eL/tauL), 0, vBottomCreepMax)

    collect bottom requests under independent and total budget caps
    for each donor->receiver request in direction order:
        a = grantedAmount(request)
        wH = clamp(bh[d]/max(bh[d]+bl[d],eps), 0.35, 0.65)
        wH = clamp(wH + kBias*(eH-eL), 0.35, 0.65)
        aH = min(a*wH, bh[d], a)
        aL = min(a-aH, bl[d])
        move remaining grant to the phase that still has stock
        bh[d] -= aH; bl[d] -= aL
        bh[r] += aH; bl[r] += aL
        C_H += directionSign*aH
        C_L += directionSign*aL
        emit compact per-phase telemetry
```

### 5.4 输入、输出与复杂度

- **输入**：`targetTilt/effectiveTilt`、`bh/bl`、donor/receiver 方向、`vBottomCreepMax`、`totalMotionBudget`、`commandPhaseId`、相位起点 `bh0/bl0`、帧间隔 `dt`。
- **输出**：状态更新后的 `bh/bl`、`bottomFlux`、`heavyFlux`、`lightFlux`、`C_H/C_L`、`eH/eL`、`heavyShapeRms`、`lightShapeRms`、`heavyShapeMaxDelta`、`directionCorrect`、`heavyShareDeficit`。
- **复杂度**：与 v2.7.3 相同，仍为 `O(cols)` 时间、`O(cols)` 状态；仅新增常数个标量，端侧额外内存可忽略。每个相邻对的拆分是 O(1)，不引入排序、全列卷积或额外物理 pass。

### 5.5 验收与判别信号

以下为 v2.8 新增的路线内观测门槛；不放宽、不替代 v2.7.3 已有的 objective tiered minimum、预算、守恒、稳定性或主观 TIERED/HG 门槛。

1. **heavy 非零且方向正确**：每个 ≥2 秒的有效正/负倾斜相位，`heavyFlux` 至少 30 个非零样本；`sign(C_H)` 与 `sign(targetTilt)` 一致，方向错误为 0。
2. **heavy 形状可测**：以相位起点为基准，中央 80% 列的 `heavyShapeRms >= 0.5` model unit，且 `heavyShapeMaxDelta >= 1.0` model unit；`maxSubstepBh` 仍必须满足现行 TC03 平滑阈值。
3. **heavy/light 独立观测**：`heavyFlux` 与 `lightFlux` 分开累计；禁止 checker 再把同一个聚合位移同时复制为 H/L response。`variance(C_H)>1e-12` 且 `variance(C_L)>1e-12`，否则判 `INCOMPLETE(degenerateMeasurement)`，不得给物理行为计分。
4. **客观原有门槛不降**：现有 Pearson、lag、FE、方向、预算、守恒、非有限值、out-of-range、model error、crash/ANR 与 TC01/TC02/TC03 分档继续按现行规则判定，不做阈值放宽。
5. **主观绿色 ROI 成功信号**：EC-TT-01~EC-TT-04 两轮中，绿色 `bh` 边界在 ≥15° 或用例规定的最大有效倾角保持期内，中央 80% ROI 的边界质心位移同向且 `>=2px`；反向倾斜时符号翻转，复位后回到水平趋势。水/黄沙改善不得用于替代绿色 ROI 指标。
6. **失败判别**：
   - 若 `heavyFlux>0` 但 `heavyShapeRms` 不足，归因为幅度/预算分配不足；
   - 若 `heavyShapeRms` 达标但视觉 ROI 静止，归因为渲染/ROI/颜色映射或帧采样问题；
   - 若 `bh` 日志变化而 `bl` 不变，检查 heavy/light 拆分过偏；
   - 若两者均静止但 `bottomFlux>0`，检查 donor/receiver 或遥测状态；
   - 若遥测静止且帧也静止，才归因为算法未生成 bottom motion。

### 5.6 交接约束

#### Developer

1. 只在实现新 route 时将 `routeId` 升级为 `bottom-creep-v2.8-heavy-interface`；禁止沿用 v2.7.3 routeId 发布行为变更。
2. 实现必须新增紧凑标量遥测，禁止把 128 列全数组作为唯一 H/L 证据；保留现有数组仅用于诊断。
3. 不得提高 `vBottomCreepMax`、`totalMotionBudget`、独立预算或任何现有及格线来制造视觉响应。
4. 必须保持 heavy/light/water 总量守恒、receiver 容量约束、donor 存量约束与 donor/receiver 方向语义。
5. `kBias` 固化为常量并记录实现值；禁止在验收运行中调参。

#### Tester

1. 客观与主观报告独立成文：`docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 与 `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md`。
2. 每份报告必须记录 source commit、signed HAP SHA256、device、OS/API、每个原始 log 的 SHA256、routeId、runId 与 resetEpoch。
3. checker 必须分别解析 `heavyFlux/lightFlux/C_H/C_L/heavyShapeRms`，不得复制聚合 response 冒充 H/L。
4. 需要新增一条最小预检：±15° 各保持 2 秒，输出 green ROI 质心位移、`heavyShapeRms`、`heavyShapeMaxDelta` 与 `heavyFlux`。该预检只判定观测链路，不替代完整 suite。
5. 不得因水/黄沙响应良好而修改 HG-TT-04 或主观用例；v2.7.3 的主观 FAIL 保持原判。

#### Effector / 评审闭环

1. 评审截图/视频必须同时标出 water、yellow light、green heavy 三个 ROI，并给出绿色边界的时间序列或逐帧 diff。
2. 若只提交连续视频，需至少提供 0°、正倾保持、0°、负倾保持、0° 五个关键帧及 frame timestamp 与最近 `P011_METRIC` sequence 对齐表。
3. 若 hilog 仍出现 128 列数组截断，不得据此判定 H/L 位移；必须使用 compact scalar telemetry。
4. 闭环仍按三条判定：现行硬门槛必须 PASS、现行软门槛不低于及格线、P-011 用例有可见改善；未解决问题用例可失败，但不允许重解释 HG-TT-04。
5. 本路线不得与 `gravity-advection-v1` 做全量 dominance 比较；跨路线只能作为选型参考。

### 5.7 2026-10-18 TC02 正式复测归因与观测协议修订

> **状态**：`DESIGN_UPDATED_PENDING_RETEST`  
> **判定**：本轮 TC02 FAIL 不是 `bottom-creep-v2.8-heavy-interface` 物理/预算机制的失败，而是 test-mode 输入观测状态机把非 Want 帧写成 `source=script` 遥测的调度/观测语义缺陷。物理机制、预算、方向语义与既有阈值均不修订。

#### 5.7.1 证据与归因

1. 正式复测解析到 546 条 script 事件，期望 531 条；最小 accepted gap 为 17ms，accepted gap 违规 10 处。证据为 `test/artifacts/p011-v28-heavy-interface/p011-tc02-formal.tc02-validation.json`。
2. 事件流自第 256 条开始偏离 schedule：第 256 条 `targetTilt=0`，而 schedule 第 256 行应为 `20`。第 256 条距前一条 acceptedTime 仅 17ms，随后一条又被计为 83ms gap。
3. 事件 target 多集相对 schedule 多出 `0×9`、`+15×3`、`-15×3` 共 15 条。这些目标值不是新增 schedule 命令，而是输入状态机在无新鲜 Want 帧内对 `tiltX` 衰减/复位后产生的中间值。
4. 代码路径显示，`inputSource` 默认就是 `script`；test-mode 下无新鲜 Want 时仍可能改变 `tiltX`，随后因 `tiltChanged` 进入事件记录分支。该分支把衰减/复位帧与真实 `aa start --ps wstilt` 命令混在同一 `P011_EVENT input/source=script` 语义中。
5. 因此 546/531 与 17ms gap 是观测语义污染，不是算法生成多余命令，也不是调度要求应从 100ms 放宽到 17ms。

#### 5.7.2 修订设计

1. **命令唯一来源**：TC02 中只有新消费到的 `wstilt` Want 才能产生 `P011_EVENT input` 且 `source=script`。输入恢复、保持、平滑、force-invert 退场或零复位帧不得复用该语义。
2. **保持语义显式化**：有效 script 命令之间必须精确保持上一次 `wstilt` 目标；禁止隐式向 0 衰减。若后续确实需要衰减/复位观测，必须使用独立显式 schedule 命令，不能由帧循环合成。
3. **遥测隔离**：帧循环状态恢复或诊断帧可记录 `P011_EVENT input_state`，checker 不把其计入 script 事件、Pearson、lag、FE 或 accepted-gap 指标。禁止把 `input_state` 改名为 `script` 后继续混入。
4. **1:1 映射**：531 条 schedule 行必须对应 531 条 `source=script` 事件；同一 run 内 `sequence` 连续且不重复。出现第二个 `runId`、进程重启或事件缺失时，直接判 TC02 schedule precheck FAIL，不允许跨 run 拼接物理解。
5. ** accepted gap 不变**：相邻真实 script 命令的 `acceptedTime` gap 仍必须满足 `>=100ms`，违规数为 0；不允许为吸收合成帧而放宽该阈值。
6. **复测闸门顺序**：先完成 schedule precheck；只有上述 identity/count/gap/target 全部通过，才允许进入 TC02 物理评分、压力段或主观复测。TC02 物理评分与压力段沿用现行门槛。

#### 5.7.3 可验证验收指标

TC02 schedule precheck 必须在同一份原始 log 上输出并全部满足：

```text
scriptEventCount == 531
expectedEventCount == 531
scriptRunIdCount == 1
scriptSequence == [1..531] 连续且无重复
targetSequence == schedule.targetTilt 精确一致
acceptedTimeGapViolations == 0
minAcceptedTimeGapMs >= 100
```

同时新增结构检查：`P011_EVENT input/source=script` 只能由新鲜 `wstilt` Want 命令产生；`P011_EVENT input_state` 不进入上述指标。任一项失败即判 schedule precheck FAIL，不评估 TC02 物理分数。

#### 5.7.4 2026-10-18 第二次正式复测失败归因与采集冻结闸门

第二次复测出现独立 precheck `531/531` 通过而全量套件捕获 `984/531`、双 `runId`、`sequence invalid`、最小间隔 `23ms` 的不一致。该差异不能解释为物理调度失败：完整套件日志仍是可增长采集流时被读取，注入 shell 未完成/采集未冻结时得到的样本数量、相邻间隔和 run 边界都不是终态。此后等待同一采集生命周期自然结束再校验，冻结后的全量套件日志为 `531/531`、单 `runId`、sequence 完整、`minAcceptedTimeGapMs=116`、0 violations。因此第二次失败归因为采集生命周期与校验时序语义差异，不是阈值不足。

为消除这一差异，schedule precheck 增加采集冻结闸门；该闸门只约束证据生命周期，不修改或放宽既有 `531/sequence/100ms/target` 阈值：

1. **单一路径**：全量套件 TC02 与独立 schedule precheck 必须使用同一个 schedule、同一种 `aa start` Want 注入命令、同一段 shell 间隔和同一 hilog 采集参数。禁止 precheck 用短脚本、全量套件改由另一个采集/注入路径替代。
2. **先等待注入完成**：`hdc shell sh <device script>` 必须在采集进程存活期间同步执行并返回；校验前必须确认 531 条注入命令已全部结束。
3. **先冻结再校验**：采集停止后，必须先冻结证据：记录 log 绝对路径、大小与修改时间；间隔至少 1 秒复查一次，大小和修改时间不变才允许解析。校验读取前后再次比较大小、修改时间与 SHA256；任一变化即丢弃结果并判 `FAIL(captureNotFrozen)`。
4. **禁止活动残留采集**：同一目标/输出路径存在未结束的历史 hilog 子进程或前一次采集句柄时，不得开始新 formal run；必须先显式终止并清空目标，再启动采集。
5. **终态 run 边界**：解析必须以冻结后 log 为准，禁止在运行中逐行读取后缓存事件，也禁止把清理前、注入前或另一 run 的 `input/source=script` 事件计入本 run。
6. **可审计证据**：冻结前后的 log size/mtime/SHA256 必须进入 report 或 validation JSON；缺少这些字段时 schedule precheck 不能 PASS。

第二次复测的评分闸门顺序仍按 §5.7.3：冻结与采集审计先于 identity/count/sequence/gap/target 判定。若采集冻结闸门失败，即使事件数恰好为 531，也不得进入 TC02 物理评分。

### 5.8 2026-10-18 用户授权的双输入模式补充设计

> **技术路线**：`bottom-creep-v2.8-heavy-interface`，输入协议修订 `dual-input-v1`；不是新物理路线。  
> **状态**：`READY_FOR_ARCHITECT`，待实现与双报告验证，不能标记 VALIDATED。  
> **原因与授权**：用户明确要求“测试模式（自己构建出手机倾斜状态并测试），体验模式（通过传感器获取手机状态）”。本轮用程序构造姿态替代先前要求用户配合手持的测试输入；不再把人工手持作为程序测试前置。物理预算、方向、HG-TT-04、主观 ≥6/10 及格线与 §5.7 fresh-Want/canonical/frozen 闸门全部保留。模拟输入证明模拟场景下的视觉与物理行为，不能冒充真人传感器体验已获验证。

#### 5.8.1 现状、缺口与候选取舍

现有 `test/p011_tc02_schedule.py` 已能确定性生成 TC02 倾角 schedule，canonical runner 将其作为 `aa start ... --ps testmode true --ps wstilt <angle>` Want 注入；`parseP011TiltWant` 已校验 `rendermode=4`、测试模式、非 force-invert 及 `[-89,+89]` 有限角度。现有 `!testMode` 分支已有真实加速度传感器映射与滤波。两条能力可复用。

实际缺口是隔离与切换：传感器 callback 在测试中仍更新滤波；面板、长按、热 Want 重复切换而不完整清理；水沙消费端未校验当前模式；拖动在体验中仍关闭传感器并保持 2.5 秒；按钮只显示“测试开/关”，无法直接说明真实输入来源。

| 候选 | 结论 | 理由 |
|---|---|---|
| A. 复用外部 deterministic schedule/Want，统一输入模式转换及入口隔离 | **SELECTED** | 已有完整可审计输入协议，修改边界小，可覆盖重复运行和两方向切换 |
| B. 新增 App 内置脚本场景引擎及每帧 script 输入事件 | REJECTED | 重复现有 harness，扩大维护面；每帧合成事件还会破坏仅 fresh Want 计数及 531 identity |
| C. 沿用两个布尔标志，仅修改按钮文案 | REJECTED | 仍存在旧 target、pending Want、滤波与手势接管残留，不能实现源隔离 |

#### 5.8.2 输入选择与数学语义

逻辑上仅有 `TEST` 与 `EXPERIENCE` 两模式；可继续以 `testMode` 表示，不要求全项目重构。输入所有权由模式唯一决定：

```text
mode == TEST       => accept script/测试手势/测试 forceInvert; reject sensor samples
mode == EXPERIENCE => accept sensor samples; reject script/倾角手势/forceInvert
```

TEST 的正式自动输入是既有 schedule 行 `u_k=(sequence_k, targetTilt_k, commandTime_k)`；有效新鲜命令被消费一次后，`targetTilt(t)=targetTilt_k` 保持至下一条命令，不自动衰减。只有该消费动作可写 `input/source=script`；保持、复位与切换不产生 script 命令。测试手势可保留作手工调试，但不得覆盖正在保持的正式 script 目标；需要手势调试时应显式结束 script 控制，其记录仍为 gesture/input_state，不能混入 TC02。

EXPERIENCE 继续使用既有加速度映射 `sx=-filteredAx, sy=filteredAy`，当平面模长 `sqrt(sx²+sy²)>0.3` 时，`target=atan2(sx,sy)*180/pi`，继续走既有滤波/角速度限制与物理 `effectiveTilt` 管线，不改参数或方向约定。没有有效传感器样本时保持初始化安全姿态 0°；不得回退使用上次模拟角度。退出 TEST 后下一个新鲜传感器 callback 即重新获得输入权，无需手势结束或等待旧 hold 到期。

#### 5.8.3 统一转换、清理边界与首命令顺序

面板、长按、启动初始化与热 Want 必须走同一转换语义。仅当模式确实变化时清理；TC02 中每条 Want 都带 `testmode=true`，同模式重复声明不得重置 target、滤波/输入序列、runId 或物理累计。

一次模式变化应在下一次输入消费/physics step 前完成以下操作：清除 `p011ScriptTargetTilt`、旧 pending `wstilt`、`forceInvert/wasForceInvert`、gesture hold/接管状态、`tiltX/lastTiltX` 与输入滤波 `sensorGx/sensorGy`、旧输入时间戳；按目标模式设置 `useSensor=!testMode`，同步 UI 与 telemetry。水沙内部若有输入平滑状态，也须通过最小既有接口清理该输入缓存；不能直接改 `bh/bl/water`、重置材料场或把模式转换伪装成物理位移。跨切换边界的输入延迟/跳变统计应标注边界，不作为连续 TC02 样本拼接。

**实现顺序陷阱**：当前 `EntryAbility.applyWant` 先发布 `p011TiltWantCommand`，再发布 `liquidWantCommand`。若转换时无条件丢弃 AppStorage 最新 sequence，会吞掉“从体验进入测试且同一 Want 携带首条 wstilt”的合法命令。architect 应选择以下最小方案之一并写明：

1. 将当前 Want 的模式变更与可选 wstilt 作为同一转换 envelope 处理，先清旧状态，再发布/消费 envelope 的合法新命令；或
2. 携带可审计的当前 Want 命令身份/sequence，转换时只丢弃旧 pending，保留当前转换 Want 的合法新命令。

不得仅靠 commandTime 大小或与帧时间相近猜测身份。面板/长按没有新命令 envelope，应推进已消费 watermark 至当时 pending seq 以废弃旧命令。EXPERIENCE 消费端必须再次拒绝/废弃 pending，即使生产端曾允许其进入缓存；重新进入 TEST 也不能重放它。同模式重复 Want 仍须保留一命令一事件的原协议。

```text
onModeRequest(nextMode, optional currentWantEnvelope):
    if nextMode != mode:
        clear input-only state and discard pending commands from previous mode
        mode = nextMode
        useSensor = (mode == EXPERIENCE)
        sync UI and telemetry
    apply only this envelope's valid controls for the selected mode

onSensor(sample):
    if mode != EXPERIENCE: return
    update existing filter and fresh sensor timestamp

onFrame():
    if mode == EXPERIENCE:
        discard any script pending; derive tilt only from fresh sensor state
    else:
        consume fresh valid wstilt once; otherwise hold script target
        emit input/source=script only for this actual consumption
    feed unchanged physical/render interface

onPan/update/end():
    if mode != TEST: return
    follow test gesture ownership policy; never enable sensor while TEST
```

按钮应直接显示当前模式及输入来源，例如“测试模式（模拟姿态）”与“体验模式（传感器）”；保留既有可用启动参数：`rendermode=4,testmode=true,wstilt=<angle>` 为测试，`rendermode=4,testmode=false` 为体验，且后者禁用/忽略 `forceinvert` 与模拟倾角。UI/启动命名是同一模式的两个入口，不能有不同清理规则。

#### 5.8.4 输入、输出、复杂度与假设

- **输入**：模式请求、来源（面板/长按/Want/启动）、可选当前 Want 身份、有效 wstilt 命令、传感器样本、手势事件、既有帧时间。
- **输出**：唯一生效输入源、目标倾角与既有物理输入、清理后的输入缓存、准确 UI 文案、原协议 input/input_state 事件。正式运行元信息记录 `inputMode=TEST` 与 schedule/capture 身份；不需新增每帧 mode 日志。
- **复杂度**：每次 callback/模式转换/帧输入选择均 `O(1)` 时间与 `O(1)` 额外状态；保持既有一条传感器订阅即可，callback 早退，避免反复注册带来的异步竞态。外部 schedule 的 `O(K)` 存储和生成在 host，不增加端侧物理 pass。
- **ASSUMPTION-4**：同一 UI/Ability 输入状态按现有事件循环串行处理；若存在跨线程写入，architect 必须用现有串行分发确保“清旧后应用当前 envelope”原子可见。
- **ASSUMPTION-5**：传感器可用的设备会持续产生新样本。本轮无需用户手持来触发该能力；若设备不提供样本，记录设备限制，纯 helper/回调测试只能证明隔离和映射，不能冒充真传感器端到端通过。

#### 5.8.5 可量化验收与测试方法

下列是输入功能新增验收，不替代/放宽现有客观、主观阈值。tester 在实现前补同 route 用例与 RED 基线；只对新增 helper 做实际状态机测试，不能仅以源码匹配冒充行为测试。

| 项 | 方法 | 必须满足 |
|---|---|---|
| TEST 传感器隔离 | 同一测试状态分别注入 0 次与 100 次不同加速度 callback，执行相同 script/帧序列 | 模拟目标和生效输入逐步精确相等；测试中的滤波/传感器时间戳更新次数均为 0 |
| EXPERIENCE 模拟隔离 | 已有有效传感器状态下注入 wstilt、Pan update/end、forceInvert；含旧 pending | 生效倾角与只传感器对照逐步精确相等，script 消费/事件数均为 0，forceInvert 始终 false |
| 切换无残留 | 两方向各 10 次，预置非零 target/tilt/filter、pending 与 forceInvert/gesture 状态；覆盖面板、长按、启动与热 Want | 每个边界清理完整；无旧 pending 重放，TEST 永不恢复 sensor，EXPERIENCE 不等待 2.5 秒/24 小时 |
| 首命令与幂等声明 | 体验→测试的同一个 Want 携带 wstilt；接着 3 条 testmode=true Want；另测冷启动首条 | 合法新命令各消费恰好一次，target 序列精确一致；没有首条被清空，也没有同模式清理 |
| 传感器恢复 | TEST 先保持 +20°，退出后注入/观察首个新鲜传感器 callback | 下一次输入消费即选 sensor；无 script 残留、无 gesture hold 阻断；既有映射/滤波策略不变 |
| 可重复程序构造 | 同参数两次生成 canonical schedule 与注入脚本；端到端采集运行仍串行 | schedule/script SHA256 分别一致；保留 531/531、单 run、连续 sequence、目标精确一致、min accepted gap ≥100ms、0 gap violation；完整 capturePath/captureFrozen/captureAudit PASS |
| UI/入口一致 | 打开面板，切两次并对照两种启动入口 | 文案与真实模式一致，每次均清同一组输入状态；体验入口不允许 forceInvert 生效 |

主观自动复测可用程序生成 EC-TT-01~04 既有角度/速度/保持序列，并保留连续帧、时间对齐、green ROI 与原评分标准；输入元信息明确为模拟姿态。若既有用例含人工输入措辞，由 manager 将本次用户授权同步给 tester/effector 后追加变更记录，不可暗改历史结果。本节未完成实现与双报告，因此只作交接设计，不宣布 P-011 闭环。
