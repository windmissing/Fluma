# 算法原则：P-011 倾斜跟手性/底层运动响应

> 版本：v0.6　日期：2026-10-18　状态：DESIGN_UPDATED_PENDING_RETEST  
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
> **状态**：`DESIGN_PENDING_APPROVAL`；本轮只更新设计文档，不修改业务代码。  
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
