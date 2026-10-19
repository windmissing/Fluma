# 算法原则：P-011 倾斜跟手性/底层运动响应

> 版本：v0.11.0　日期：2026-10-19　状态：BLOCKED（slip-overshoot-v1 存在 484 次 tan30° residual 越界；30°±2°/5.1s 与现有相邻通量额度冲突；v2.12 双角阈值、v2.14经验标定仍待观测）  
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

## 6. v2.9 方案：bottom-creep-v2.9-central-dune-heavy-interface

> **routeId**：`bottom-creep-v2.9-central-dune-heavy-interface`；输入协议继续使用 `dual-input-v1`。  
> **日期**：2026-10-19。  
> **状态**：`DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE`。§6.3 原四列构造已被 §6.11 的 angle 证明否决；新平滑构造满足 angle 和理想 heavy 形状，却严格超出现有 bottom 预算。`shapeResidual` 的释放规则已定义为待验证契约，尚未证明可在真实 ETS 中稳定达成。无 v2.9 实际 ETS/HAP/设备 green 或正式双报告，不批准交 architect 实施。  
> **基线**：`bottom-creep-v2.8-heavy-interface` + `dual-input-v1`。正式包 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap` 的 SHA256 为 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`；review commit `130a52c74e8964fb024677b672cad1965f5cf1db`，实际构建为 `fdc8190` 加 48 个 dirty 文件的 source snapshot，不能把 review commit 单独称为可复现构建。正式物理证据为 `test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json` 与同目录 `green-new-hap/`。历史 10 列/capacity 1000/vSandTopMax 30 PASS 只作参考，不外推默认 128 列/capacity 192/vSandTopMax 28。

### 6.1 v2.8 默认场景失败归因

**结论：已证实输入链路工作、heavy 有非零输运而默认中央 80% 的 `bh` 形状几乎不变；均匀界面请求的内部相消由源码与 host 探针共同支持。总预算竞争会加重衰减，但不是单独解释全部形状失败的因素。真实视觉映射与候选新路由仍未验证。**

1. **输入已生效**：`P011_EVENT input/source=script` 为单 run、sequence 1..5、目标 `0/+15/0/-15/0`；两个非零相位 wall-clock 保持约 5.1s。`heavyFluxNonZeroSamples=124/154`，方向 correct，说明 script 输送与 route 均已激活。
2. **几何不是直接根因**：默认 thick 初始化为每列 `bh=72, bl=72`，heavy/light total 均 `9216`，water total `4864`（每列 38）。`capacity=192` 下无 `CAPACITY_OVERFLOW/model error`；历史 capacity 1000 只代表更大的绝对余量，不能解释中央 80% 全静止。`vSandTopMax=28` 只影响顶部子步预算，bottom 的独立上限来自 `vBottomCreepMax=10`，两者不是同一预算。
3. **均匀逐界面输送在平直场上内部抵消**：v2.8 对 `cols-1=127` 个相邻界面按同一 `maxFlux` 请求，再用独立预算均分。每个内部列既是 donor 又是 receiver，同子步净变化接近 0；两端积累净变化。既有 host probe 的原始 ETS 平直场，正相位 full-array `max |bh-baseline|≈0.02565`，中央 80% RMS `≈3.48e-7`；正式设备截图窗口的正/负 `heavyShapeRms=1.0555e-7/5.3672e-7`、max `1.7731e-7/1.6499e-6`，均远低于 0.5/1.0。host、全 log 峰值与截图窗口须分别标注，不混称同一统计窗口。
4. **总预算竞争进一步压低 bottom**：正式正相位 `sum(bottomCreepBudgetLimit)≈27.22`、`sum(bottomFlux)≈6.469`，折合 bottom `≈2.38/s`；host 每帧 shake 压力样本里的原 v2.8 预算计算仅 `≈0.936/s`。这两个数来自不同输入负载，不能互换。纯静态 host 样本的 bottom 已达 `10/s`，中央仍几乎相消，说明只改优先级不能解决形状。
5. **宽形状预算成本**：在保持 `vBottomCreepMax=10`、不提高总预算的约束下，相邻界面输运成本随支撑宽度增长。对两侧各 `k` 列、幅度 `A` 的守恒 dipole，所需 signed cumulative heavy flux 的绝对值总和为 `A*k²`。24 列支撑即 `k=12,A=1`，成本为 144 model units，远超约 2.7 秒、heavy share 0.5 时独立 bottom cap 可供的约 13.5 heavy units。因此不把预算摊到宽形状。
6. **light 响应不能替代 heavy**：正式全 log 正相位 `lightShapeRms` 峰值约 `1.719`，但硬门槛要求 heavy `RMS≥0.5/max≥1.0`；正式 green mask 质心仅约 `+0.0061/+0.0012px`。v2.8 物理判 `FAIL(heavyObservation)`，后续 canonical/主观正式评估 `BLOCKED`；dual-input 产品真实测试 `39/39 PASS` 以及冷热首 Want、fresh sensor、面板双向切换通过，属于另一条已完成证据链。

### 6.2 候选取舍

| 候选 | 结论 | 理由 |
|---|---|---|
| A. 继续均匀逐界面输送，只调 heavy share | REJECTED | 平直 128 列场的内部净变化仍抵消；仅调 share 无法把边界变化变成中央 80% 形状。 |
| B. 提高 `vBottomCreepMax`、`totalMotionBudget`、capacity 或放宽 RMS/max | REJECTED | 违反“不改预算、不放宽阈值”的验收约束。 |
| C. 改用跨列长距离搬移 | REJECTED | 破坏相邻 donor/receiver 语义，且引入 `O(cols²)` 端侧成本。 |
| D. 中央 4 列 dipole + bottom-first 预算优先 | **ANGLE 否决** | 理想 heavy 形状过线，但中心 heavy 阶差 `2A=5.4`，若 H/L 同向按 0.5 share 合成则 total sandTop 阶差 `10.8`；均远超 ±15° 的允许值。详见 §6.11。 |
| E. 104 列平滑半波 dipole | **BUDGET 否决** | 理想角度、RMS/max 可过，但相邻 heavy 输运约 1893 model units；保留作为约束矛盾的可复核构造，不是上线候选。详见 §6.11。 |

### 6.3 数学模型与参数

本节保留被否决的四列假设以便复盘；它**不是**新 `targetDelta` 或实现规格。沿用 v2.8 的方向、守恒、receiver 容量、donor 存量、独立 bottom 预算、总 motion 预算、heavy share `[0.35,0.65]`、`dual-input-v1`、默认几何与全部阈值。以下旧数值仅用于计算反例：

```text
P011_DUNE_HALF_SUPPORT_COLUMNS = 2
P011_DUNE_TARGET_AMPLITUDE    = 2.7
P011_DUNE_RESIDUAL_EPS        = 1e-4
```

以中央 80% ROI 的 `start=floor(cols*0.10)`、`endExclusive=cols-start`、`center=floor((start+endExclusive)/2)` 为基准。`cols=128` 时该 ROI 有 104 列。对 `sign s=+1`，目标 heavy 增量为：

```text
d[c] = -A, c ∈ {center-2, center-1}
d[c] = +A, c ∈ {center,   center+1}
d[c] =  0, otherwise
```

对 `s=-1` 取反。设接口 `j` 表示列 `j -> j+1` 的 signed heavy cumulative flux `X_j`。到达上式的最小相邻输运目标为：

```text
F[j] = -sum_{c<=j} d[c]
```

对 `A=2.7, K=2`，正方向只在中央三个相邻界面产生非零目标 cumulative flux：

```text
F[center-2] = +A
F[center-1] = +2A
F[center]   = +A
```

其相邻输运总成本为 `4A=10.8` model unit。若有完整 `2.722s` 可用物理时间、bottom 每秒实际拿满 10 且 heavy share 恰为 0.5，上界才是 `13.61` heavy model units。它只说明孤立正相位的预算必要条件可能满足；没有证明真实 profile 的容量、split、平滑与竞争后仍达目标，更不能推断负相位。按 104 列 ROI 理想形状为：

```text
heavyShapeRms    = A * sqrt(4 / 104) ≈ 0.5295
heavyShapeMaxDelta = A = 2.7
```

此处仅是**被否决假设的理想模型数组值**，实际 ETS 未执行。`0.5295` 距 `0.5` 仅约 5.9%；更直接的否决理由是 angle。正相位的 heavy horizontal first moment 为 `4A=10.8 model-unit·column`；以中央 104 列均匀 `bh=72`、画面宽 1280px 的连续质量质心近似，位移约 `10*10.8/(104*72)=0.0144px`。若要使**同一整块质量质心**变化 2px，需约 `1497.6 model-unit·column` 的一向 heavy 相邻通量，比正相位 10.8 大约 139 倍。实际 `green_centroid()` 是 RGB 阈值后的二值像素 mask 质心，不等于连续质量质心；因此这个计算是可达性警讯，不能冒充像素测量或无条件的像素不可能性证明。局部边界特征与整块 mask 质心需分开看。

还存在**反向相位基线矛盾**：若 + 相位在列材料上留下 `+d`，短暂 `0°` 只暂停而不消除材料，则从当前材料再做 `-d`，相对原始截图最多返回近中性；相位内 `heavyShapeRms` 可能达标，但负相位截图对原始 baseline 的方向位移不成立。若要求在同一次运行的原始基线两侧形成 `+d/-d`，从正峰到负峰至少要 `8A=21.6` heavy 相邻通量；在固定 0.5 share 下需至少 `4.32s` 有效 bottom 满额物理时间，在允许的最乐观 0.65 share 下也需 `≈3.323s`。这未计其他通道、容量和耗散。正式相位约 5.1s wall-clock 不能等同可用 physics time。须在实际双向连续轨迹上验证；不可分别重置场景并拼出两方向通过。

#### Bottom-first 预算调度假设

既有 host probe 经 bundled TypeScript 转译执行原 `WaterSandField.ets` 的 128/192 thick `+15°`、300×1/60s collect/restore/replay 路径。候选预算分配只在探针侧按原请求计算，没有改 ETS apply 顺序。若将来评估 bottom-first，单次记账契约必须是：

1. collect 可以记录各通道原始请求（含 bottom）及原始 `totalMotionBudgetRequested`，只作诊断/需求估计；**不**把这些数当 grant，也不从 `totalMotionBudgetRemaining` 预扣或 `reserve`。collect 的临时材料/目标状态必须如既有 snapshot 语义恢复，route 自有 `F/X/residual` 的 collect 临时变化也必须恢复或仅在 apply 提交一次。
2. apply 从 `totalMotionBudgetRemaining=300*dt`、`bottomCreepBudgetLimit=10*dt`、scale 1 起步。先对三个相邻接口根据**当前材料**求 donor heavy/light、receiver capacity 和可行 split，再给实际成功的 bottom 转移调用一次共享 budget clip；`bottomCreepBudgetUsed` 与 `totalMotionBudgetUsed` 同时按同一实际 grant 增加一次。接口阻塞时不计 grant。
3. 然后重放其他通道，按每笔局部约束和剩余共享预算裁剪。改变 apply 顺序会改变它们看到的 stock/capacity，因此原 collect 请求未必仍可原样执行；必须在 replay 用当前状态重新校验，不得用旧请求强搬、遗漏通道或复用已扣的预算。要求 `0<=bottomUsed<=10*dt`、`0<=totalUsed<=300*dt`，且 `totalUsed=sum(各通道实际成功 grant)`；独立裁剪与 total 裁剪诊断分开。
4. 不得先对 collect 的 `totalMotionBudgetRequested` 扣 bottom 后又在 apply 扣一次；也不得把 collect 统计误称为一次真正 reserve。失败 rollback 要恢复材料、预算、相位目标和 `X/residual` 的同一 substep 状态。

每帧 `setShakeInput(1)` 的 host 竞争样本中，原 v2.8 grant 折合 bottom `≈0.9355/s`；按上述优先顺序**离线推算** bottom `10/s`、其他 `289.03/s`，总 `299.03/s<=300/s`。纯静态样本两者都约 `10/s`。原 ETS replay 的守恒漂移最大约 `7.28e-12`。这些仅支持预算算术与原产品路径可观测性；未验证新顺序下其他通道的真实轨迹、能否取得同额 grant、视觉或性能。`bottomFirstTotalClip` 属假设性统计，不能当 ETS 实测。

#### Residual 与命令相位生命周期

- 正/负相位开始于**有效非零目标的符号变化**，以当前 `bh/bl` 为该相位形状遥测 baseline，重建该相位 `F`，将本相位执行通量 `X` 清零；另保留用于同一次运行视觉方向比较的原始图像 baseline 身份，二者不得混淆。`commandPhaseId`、遥测 baseline 与 dune tracker 必须在同一边界变化。
- 每个已提交的相邻 heavy transfer `qH` 只更新对应有向边的 `X[j]`；`duneResidual=sum(abs(F-X))`。若 `F` 是 signed，必须统一使用全局左→右坐标与 `X[j]+=directionSign*qH`；按缺口选边时用方向化剩余量 `max(0,directionSign*(F[j]-X[j]))`，不得把负相位 `F` 再乘错号。达到目标后停止；任何耗散引起的后续偏离须明示目标重建/容差规则，不能靠重复 Wants 偷偷补量。
- 同号每 100ms 连续 Want 沿用目标与 baseline，不重新累计 `4A`。短暂 `0°` 暂停新 bottom 输运并保留目标、`X` 与残差；同号恢复继续。反号非零命令从**当前材料**重建相位遥测 baseline、终止旧 residual；不能把旧符号 `F/X` 混入新 phase。此规则保持输入语义，却带来上述原始截图两侧可达性疑问；不能靠相位重基线掩盖。
- TEST/EXPERIENCE 切换必须按既有 `resetInputState()` 输入边界清除 route 自有 pending residual、target、baseline 缓存和相位符号，下一条合法输入才从当前材料开新相位；保留材料、runId、simulation/physics time，不改变 `dual-input-v1` 的首 Want 与 fresh sensor 顺序。当前 `resetInputState()` 只清 tilt/direction，没有清 `commandPhaseSign` 或形状 baseline；若直接增加清理，需要同时处理 `setTilt(0)` 的旧相位、遥测与 runId 不变之间的一致性，尚无实际验证。
- donor stock 与 receiver capacity 在**apply 时的实际状态**重算；每个接口约束 `qH <= donor bh`、`qH+qL <= receiver free capacity`，并同时受独立 bottom 与 total budget 限制。若一接口被阻塞，不得把缺口静默重分配到其他接口以伪造 dipole；必须记录阻塞原因、剩余 residual、stock、capacity。

### 6.4 已否决四列方案的调度伪代码（仅解释阻断，不授权实现）

```text
on accepted nonzero sign transition:
    close previous residual; capture current bh/bl phase baseline
    construct signed F on three central adjacent interfaces; X = 0
on repeated same-sign Want: keep F, X and phase baseline
on zero tilt: pause bottom request; retain F, X and phase baseline
on input-mode reset: clear F, X, pending target and cached baseline only

on physics substep collect:
    collect candidate motion requests; count each request once
    preserve/restore any speculative route state with material snapshot
    do not debit total budget or increment committed X
on physics substep apply, if target active:
    start with totalRemaining = 300*dt and bottomRemaining = 10*dt
    for the three adjacent interfaces in direction-aware order:
        deficitH = max(0, sign * (F[j] - X[j]))
        recompute donor heavy/light and receiver free capacity
        choose feasible coupled heavy/light amount preserving existing split bounds
        limit heavy by deficitH; limit total by bottomRemaining and totalRemaining
        commit one adjacent transfer; debit each budget once; update X once
        if blocked: emit interface, donor stock, capacity and residual
    replay other channels under remaining total budget and current material limits
    audit material, budgets and signed target; publish compact diagnostics
```

这段只说明预算/生命周期的正确事务语义；目标 profile 已因 angle 被否决，**不得按此伪代码实现四列目标**。当前 `splitBottomPhaseBalanced()` 也不能精确保证所需 `qH`：它按 donor 组成和相位残差动态选择 heavy share，现有 `creepBottomRequests()` 在其他 motion 通道之后执行。任何新路线须另证新顺序与 split、局部 stock/capacity、回滚和 `maxSubstepBh` 的可行性；不跨列补量，不把 light 计作 heavy。

### 6.5 已否决四列假设的输入、输出与复杂度记录

- **历史假设输入**：v2.8 全部输入；曾拟新增 `targetDelta/F/X/duneResidual` 相位状态。§6.11 已否决此四列目标，以下不构成产品接口要求。
- **拟议诊断输出**：v2.8 全部输出；若路线解除疑点，可新增 `duneTargetAmplitude`、`duneHalfSupport`、`duneResidual`、`duneBlockedInterface`、`bottomActualGrant` 与 `bottomTotalDebit`。后两者应指同一已提交量，不命名为 `reserved`，以免把 collect 请求误作已扣预算。
- **历史假设复杂度**：四列 profile 仅需中央 3 个接口，但已被 angle 否决；不据此推断后续可行路线的 CPU/内存。任何新方案仍不得引入跨列搬运或违背现行性能目标。
- **未验证条件 1**：设备 ±15° 的有效物理时间、实际 bottom grant 与 heavy share 足以完成正相位至少 `4A=10.8` heavy 通量；双向同次运行若需越过原始图像 baseline，还须覆盖反相位至少 `8A=21.6` 的反向 heavy 通量。不能把 5.1s wall-clock 或 host 300×1/60s 直接当设备可用 physics time。
- **未验证条件 2**：真实 ETS 的局部 profile 经其他通道与平滑后，两方向的模型 RMS/max 都通过，`maxSubstepBh` 不越原 TC03，预算与守恒保持。
- **未验证条件 3**：真实渲染的绿色 ROI 同向可见，并能按旧主观信号评估。连续质量质心估算约 `0.0144px`，不能宣称 2px 已可达。若二值像素 mask 与连续质量近似不同，必须由同分辨率实际截图/录像解释差异。

### 6.6 原门槛、证据层级与待决问题

**当前结论为未获批准的实验候选。**原有 Pearson、lag、FE、方向、TC01/TC02/TC03、预算、守恒、非有限值、out-of-range、model error、crash/ANR、`dual-input-v1`、531/sequence/100ms、capture frozen/canonical audit、主观 HG 与 ≥6/10 全部保持。下表区分客观硬门槛与主观效果信号，不据此新建或放宽测试用例。

| 项 | 测试方法 | 必须满足 |
|---|---|---|
| 默认几何一致 | 解析 `WS_EVENT init` 与 manifest | `cols=128`、`capacity=192`、`vSandTopMax=28`，preset thick，无参数覆盖；不换 10 列场景替代 |
| 输入前缀 | 解析 frozen hilog 的 script 事件 | 单 run、`0/+15/0/-15/0` 精确一致、sequence 连续、首命令不丢 |
| heavy 活跃 | 每个 ≥2s 非零相位统计 `heavyFlux` | 非零样本 ≥30；`sign(C_H)=sign(targetTilt)`；方向错误 0 |
| heavy 硬形状 | checker 与原始 `P011_BOTTOM` 双算 central 80% | `heavyShapeRms≥0.5` 且 `heavyShapeMaxDelta≥1.0`；两方向都通过 |
| 新目标语义（仅未来可行路线） | compact telemetry + `P011_BOTTOM` | 按获批准 `targetDelta` 重算实际 `bh` shapeResidual，区分 `F-X`；阻塞须显式 FAIL，不用已否决的四列/半波目标冒充通过 |
| 预算不放宽 | 解析每 substep metric | `bottomCreepBudgetLimit=10*dt`、`totalMotionBudgetLimit=300*dt`；无 bottom/total budget violation；不得通过提高常量制造运动 |
| 守恒与稳定性 | 既有 audit | heavy/light/water drift、receiver capacity、donor stock、`maxSubstepBh` 现行 TC03 阈值全部 PASS |
| 客观 green 预检 | 同分辨率截图/原 TC-P011-V28-00 | ROI 三张图可识别绿色，且正负 heavy 非零/RMS/max 达现行硬门槛；`greenCentroidDeltaPx` 是诊断输出，现行脚本**没有**对 2px 判硬通过 |
| 主观 green 效果信号 | EC-TT-01~04 同分辨率连续录像及原 §5.5 | 绿色重沙边界按原成功信号检查同向、反向与 `≥2px`；不得拿模型 RMS、light 或单张二值 mask 质心替代；若原信号指局部边界而脚本输出整块 mask 质心，须分别报告，不能同名互换 |
| 报告隔离 | 路线独立成文 | `docs/test-report-P011-bottom-creep-v2.9-central-dune-heavy-interface.md` 与 `docs/effect-report-P011-bottom-creep-v2.9-central-dune-heavy-interface.md`；记录 source commit、HAP SHA256、log SHA256、runId/resetEpoch |

**决策点**：angle 与 bottom 预算的数学复核见 §6.11。用户已于 2026-10-19 将 2px 定为局部 heavy 边界位移，whole green mask 质心仅作诊断；这不改变现行预算、几何或阈值。当前四列目标 angle 失败；新平滑目标 angle/理想形状通过但预算失败；固定 share 同向 profile 在平坦 baseline 下有 `B_total≥72.8>51.22` 的下界，现无可变 share/其他通道补偿的完整可行路线。因此不能把本方案标为设计已解；角度/预算/释放的数学归因仍独立成立。

### 6.7 历史实现边界草案（§6.11 已否决目标，当前不启动）

1. 本节不授权实现四列或半波目标。只有未来另一个同时满足 §16 angle/release、预算、形状与视觉口径的目标经过复核，才可讨论 `routeId`；不得沿用 v2.8 routeId 发布行为变更。
2. 保留 `splitBottomPhaseBalanced` 的守恒、容量、donor/receiver 与 heavy share 语义；新 profile 和预算顺序必须先有可行性证明，不能把 `a*heavyShare` 当成固定实际 heavy grant。
3. collect 只记请求；apply 阶段成功提交的 bottom 实际 grant 各从独立 bottom 与共享 total 扣一次，其他通道仅用剩余额度。须显式处理 apply 顺序改变后的其他通道状态、rollback 与诊断口径。
4. `K=2/A=2.7/EPS=1e-4` 属已否决四列假设，半波 `a=1.1` 也因预算失败；均不得作为产品常量或验收运行可调参数。任何未来参数需另有完整证明和默认场景真实证据，不改现行预算/门槛。
5. 若未来路线可行，compact telemetry 至少区分 `F-X` 与按实际 `bh` 定义的 `shapeResidualRms/Max`、稳定窗口、崩塌状态和逐子步实际 `sandTop` angle；128 列数组不可作唯一证据，`P011_BOTTOM` 仅诊断。
6. 若目标无法达成，必须输出 `duneBlockedInterface`、剩余 residual、当时 donor stock/receiver capacity，不得静默回落到 v2.8 行为。
7. `dual-input-v1`、UI 文案、首 Want 顺序、模式隔离保持 architecture §15；只在既有输入 reset 边界清掉 route 私有残差/基线，不重置材料/runId/物理时间。

### 6.8 将来恢复流水线时的验证需求（本轮不启动 tester）

1. 解除 §6.6 决策点并获得实际实现后，才准备同 route RED/验证；不使用 v2.8 FAIL 反例冒充 v2.9 通过。
2. 正式默认场景执行 `0/+15/0/-15/0`，frozen/canonical 采集；先过 schedule precheck，再评物理。
3. 每个非零相位按现行客观门槛核对 `heavyFluxNonZeroSamples>=30`、`heavyShapeRms>=0.5`、`heavyShapeMaxDelta>=1.0`、方向错误 0；`greenCentroidDeltaPx` 单列诊断，**不**加入 TC-P011-V28-00 的 hard gate。另核对 phase-start 模型形状与同次运行原始截图方向，避免反向重基线造成假通过。
4. 用独立脚本从 `P011_BOTTOM` 重算 central 80% RMS/max，并与 metric 差异小于浮点容差；防止 checker 复制聚合 response。
5. 核对最终获批准的 v2.9 参数与证据一致、total/bottom limit 不变、守恒与 budget violation 为 0；`maxSubstepBh` 满足现行 TC03。
6. 保留 10 列/capacity1000 压力回归作为兼容性检查，但其 PASS 不得替代默认 128 列判定。

### 6.9 将来恢复主观评估时的证据需求

1. 使用原生连续录像，完成 EC-TT-01~04 两轮与 EC-TT-05 回放；关键帧 timestamp 与最近 `P011_METRIC sequence` 对齐。
2. 评审图须标出 water、yellow light、green heavy 三个 ROI，并输出 green **局部边界**时间序列或逐帧 diff；同时独立报告现行 RGB mask 全域质心。两种量的定义、方向与 baseline 不能混用。
3. HG-TT-01~06 全部按现行硬门槛判定；绿色四列 dipole 允许局部位移，但不得出现 teleport、撕裂、持续冻结或 persistent jitter。
4. HG-TT-04 必须看到 green heavy 同向可见运动；水/黄沙运动或 light shape 达标不可替代。
5. 主观 TIERED ≥6/10 及格线不变；两份正式报告齐备且 designer 复核后，才允许通知 manager 闭环。

### 6.10 证据索引与尚未完成的判别

本轮只读复核以下既有文件，未运行代码、实验、测试或设备操作：

| 证据 | 已能支持的结论 | 不能支持的结论 |
|---|---|---|
| `test/artifacts/p011-v28-dual-input-20261018/DELIVERY.md`、`units.json`、签名 HAP/source manifest | dual-input 原路线 39/39、签名包身份、冷热首 Want 和静止设备 sensor/面板 smoke | v2.9 物理、动态手持主观效果 |
| `test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json`、`green-new-hap/preflight.json`、`green-new-hap/hilog.txt`、现有双报告 | 默认 128/192/28 正负 heavy 物理 FAIL、ROI 存在、后续正式门槛 BLOCKED | 10 列旧绿样本外推、v2.9 实现通过 |
| `test/artifacts/p011-v29-design-20261019/host_budget_probe.json`、`host_budget_probe-shake.json`、`host_budget_probe.cjs` | 原 `WaterSandField.ets` collect/restore/replay 的真实请求、旧 grant、假设预算算术、原场守恒 | 新 apply 顺序、dipole、HIL/raster/性能通过 |
| `entry/src/main/ets/physics/WaterSandField.ets` 的 `step()`、`applyMotionChannels()`、`creepBottomRequests()`、`splitBottomPhaseBalanced()`、`resetInputState()`；`test/p011_v28_preflight_check.py` | 当前顺序、双预算记录点、相位 baseline 与客观预检实际条件 | 未写出的 route 生命周期或 2px 客观硬门槛 |

文档复核可只读使用 `rg -n "applyMotionChannels|creepBottomRequests|resetInputState|heavyShapeRms" entry/src/main/ets/physics/WaterSandField.ets`，并直接查阅上表 JSON/报告；本轮没有执行 host probe 命令。待条件允许时，最先需要的判别证据是：默认几何真实 ETS 双向连续轨迹中的每 substep `F/X`、实际 H/L grant、两个预算、相位 baseline、`maxSubstepBh`，以及同分辨率连续 green ROI 的局部边界和全域 mask 质心两个不同测量。此前保持 `DESIGN_PENDING_HIL`，不标 `READY_FOR_ARCHITECT` 或物理 PASS。

### 6.11 2026-10-19 架构 §16 阻断复核：角度、预算与实际形状释放

本节取代 §6.3 的四列目标作为本轮最终数学结论。**用户指定的形式角度验收**要求每个有效子步的实际总沙高 `S[c]=sandTop[c]` 满足 `|S[c+1]-S[c]-tan(effectiveTilt)|<=tan(30°)≈0.57735`；`BLOCKED-V29-RELEASE` 要求释放由实际 `bh` 形状决定，不能只看执行通量 `F-X`。架构 §16 也写了这个不等式，但把 `angleDeg=30` 误述为正式 thick 的运行参数：`SandCanvas.ets` 在 thick configure 前覆盖为 `angleDeg=4`。运行时 `relaxAngle()` 的触发阈值其实是 `tan4°`；触发是请求崩塌，**不是**自动保证任何一个角度不等式的硬不变量。以下 tan30 推导只检验用户指定形式约束；实际 thick 崩塌与其他通道须另行逐子步审查，见 §6.15。

**旧四列目标严格失败。** §6.3 的中心相邻 heavy 阶差为 `(+2.7)-(-2.7)=5.4` model units；即便 light 完全不动，正向 +15° 的总沙高正斜率上界也仅 `tan15°+tan30°≈0.84530`。若 bottom 仍按 0.5 heavy share 同向携带 light，相同 profile 的总阶差达 `5.4/0.5=10.8`。反向 -15° 的对应负斜率下界为 `-0.84530`。因此四列 `A=2.7,K=2` 在现行 angle 门槛下不能成为 v2.9；预算优先无法修复这个几何冲突。

**新的显式角度安全目标，用于证伪预算。** 令中央 ROI 起点 `r=floor(128*0.10)=12`、长度 `N=104`，`n=c-r`，相位符号 `s=±1`，振幅 `a=1.1`。定义每列 heavy 目标相对本相位 `bh0` 的增量：

```text
targetDelta_s[c] = 0,                           c outside [r,r+103]
targetDelta_s[r+n] = -s*a*sin(pi*n/52),          0 <= n <= 51
targetDelta_s[r+52+n] = +s*a*sin(pi*n/52),       0 <= n <= 51
```

两半波正负质量严格相等，`sum(targetDelta)=0`；对 `s=+1`，左半部 donor、右半部 receiver，仅需逐界面向右的非负相邻 cumulative heavy flux `F_H[j]=-sum_{c<=j}targetDelta_+[c]`；`s=-1` 反向同理。104 列正好两个半波，`sum(targetDelta²)=52a²`，因此理想 `heavyShapeRms=a/sqrt(2)≈0.77782≥0.5`、`heavyShapeMaxDelta=a=1.1≥1.0`。这只证明数组目标过线，不证明实际材料能到达。

在最有利于保持现有同向 split 语义的固定 heavy share `h=0.5` 情形，配套 light 增量为 `targetDelta_L=((1-h)/h)*targetDelta_H=targetDelta_H`，故目标总沙高增量为 `targetDelta_S=targetDelta_H/h=2*targetDelta_H`。对每条边界 `c→c+1`，理想终态逐边差为 `ΔS_target[c]=(S0[c+1]-S0[c])+2*(targetDelta_H[c+1]-targetDelta_H[c])`；在本例平坦 `S0` 时绝对值至多 `2a*sin(pi/52)≈0.13284`。因此 +15° 时每条边界的 `ΔS_target` 位于 `[-0.13284,+0.13284]⊂[tan15°-tan30°,tan15°+tan30°]≈[-0.30940,+0.84530]`；-15° 镜像时也位于该小区间，且处于 `[-0.84530,+0.30940]`。两方向最坏偏差均 `tan15°+0.13284≈0.40079<tan30°`。在理想同步路径 `S(λ)=S0+λ*targetDelta_S, 0<=λ<=1` 中，每个中间子步也满足同一不等式。

未来路线的真实 apply 必须在每个子步、每条边界按实际材料计算 `ΔS_actual[c]=(bh[c+1]-bh[c])+(bl[c+1]-bl[c])+(mh[c+1]-mh[c])+(ml[c+1]-ml[c])`，然后核对**用户指定的** `|ΔS_actual[c]-tan(effectiveTilt)|<=tan30°`。split 随 donor/residual 改变、其他通道叠加或接口不同比例提交时，便不再受上述理想路径证明保护；拟议裁剪/拒绝越界 grant 的逻辑尚未证明能与既有 thick `tan4°` 崩塌事务和库存守恒兼容，不能把最终目标的证明冒充运行时 angle PASS。

**相邻输运成本使该新目标不可执行。** 左右半波相同而相隔 52 列，正向 heavy 的水平一阶矩及相邻 flux L1 精确为 `B_H=sum_j F_H[j]=52*a*sum_{n=0}^{51}sin(pi*n/52)=52*a*cot(pi/104)≈1893` model units；固定 `h=0.5` 时 light `B_L=B_H≈1893`、bottom 合计 `B_H+B_L≈3786`。这是单个 + 相位从平坦材料到 `+d` 的成本；若随后要从 `+d` 到同次原始基线另一侧的 `-d`，负相位还需约 `2B_H≈3786` heavy 与 `2(B_H+B_L)≈7572` bottom 相邻通量，不能只用 phase-start 的 `-d` 成本宣称全局反向。`vBottomCreepMax=10/s` 使正式 +15° 约 `5.122s` wall-clock 即使乐观假设积分物理时间等于壁钟，bottom 累计也至多 `51.22`；实际 v2.8 约 `2.722s` physics time 的独立上限约 `27.22`。每子步限制仍是 `bottomGrant<=10*dt`、`totalGrant<=300*dt`；把 bottom 置前最多让它趋近 10/s，不能解除较小的 bottom 独立上限。此平滑目标同时满足理想 angle/RMS/max，仍超最乐观 bottom 上限约 74 倍，故被预算否决。

还有一个**对任意固定 share、单向相邻输运 profile 的下界**，不依赖上述半波形状。令相位内 heavy 列增量 `d_c`、右向 heavy cumulative flux `F_j>=0`、`d_c=F_{c-1}-F_c`，平坦 phase baseline、各边界同一固定同向 heavy share `h<=0.65`，light 与 heavy 在相同边界成比例且无反向补偿，其他通道不改 phase 的 `S` 斜率，于是 `S-S0=d/h`。离散分部求和给出 `sum_c d_c²=sum_j F_j(d_{j+1}-d_j)`。在 +15° 的 angle 门槛下 `(d_{j+1}-d_j)<=h*(tan15°+tan30°)<=0.65*0.84530`。现行中央 104 列 RMS≥0.5 要求全场 `sum d²>=104*0.5²=26`，所以 `B_H=sum F_j>=26/(h*0.84530)`，而 bottom 总输运 `B_total=B_H/h>=26/(h²*0.84530)>=72.8`（最有利 `h=0.65`）。这已大于约 5.122s wall-clock 所能提供的乐观 `51.22`，更大于实测 physics-time 的 `27.22`；竞争、容量与崩塌只会进一步减少可用 grant。反向按镜像坐标同证。**证明适用范围**止于上述平坦 baseline、固定 share、同边界同向 H/L 和无其他通道补偿；现有 split 的 share 可在 `[0.35,0.65]` 动态变化，其他通道或 light 逆向抵消也可能改变总坡度，所以下界不能冒称覆盖所有可能 profile。后者尚无满足 angle、两个预算、方向与 RMS 的构造或证明；也不能因未证明无解就批准实现。

**2px 口径已定为局部 heavy 边界位移。** 2026-10-19 用户确认：原主观 `≥2px` 信号测量指定列/局部 ROI 的 `bh` 边界像素随倾角同向移动，不使用 whole green mask 质心作为同项判定。whole green mask 质心仍可作为诊断值，但不得替换或并列满足该 2px 判定。半波理想峰值 `1.1 model unit` 需实际局部比例至少约 `1.82px/model unit` 才可能达到 2px，现无同分辨率 raster 标定，且半波预算已否决。此决定不解除 angle、预算或 release 任一阻断，也不放宽任何阈值。

**实际形状释放契约。** 对任何将来的新目标（包括上面预算否决的半波），每个相位用 `bh0[c]` 作一次 baseline，定义 `e[c]=bh_actual[c]-bh0[c]-targetDelta_s[c]`、`shapeResidualRms=sqrt(sum_ROI e[c]²/104)`、`shapeResidualMax=max_ROI |e[c]|`。`F-X` 仅解释还欠多少有向输运，不能充当释放条件，因为其他通道可使已到位形状塌回。以半波目标 `a=1.1` 为例，若设 `shapeResidualRms<=0.10` 且 `shapeResidualMax<=0.05`，三角不等式才保证实测 heavy RMS 至少 `0.77782-0.10=0.67782`、max 至少 `1.1-0.05=1.05`；同时须连续至少 12 个有效子步、累计至少 `0.20s` physics time 均满足实际总沙高 angle、既有 `maxSubstepBh`、容量、守恒、两个预算和方向门槛，方可从 `TRACKING` 到 `STABLE`。这些 0.10/0.05/12/0.20 是**设计判别参数**，没有被执行或定版，不能写成现有测试阈值。若稳定后连续 2 个有效子步**形状残差**超界，则回 `REPAIR`，按当前实际材料与原相位目标重算剩余邻边需求，仍逐笔受原预算与容量限制；不重置 baseline、不重新计一次目标。angle、容量、预算或守恒任一硬门槛的候选 grant 越界必须在**该子步**裁剪/拒绝并记录，绝不等待两个子步。若相位结束、接口阻塞或剩余有效预算不足以恢复，输出 `BLOCKED(shapeCollapse|budget|capacity)` 与实际残差，不得把 `F-X=0` 或阶段曾短暂稳定当通过。对上述半波，预算证明已使 `STABLE` 不可达，不能配置成放行。

**状态生命周期。** 场景/run 的完整 reset 由既有初始化拥有，材料与 route 私有 `bh0/F/X/e`、phase sign、stable counter 一起重置并产生既有新 run 身份；同号 100ms Want 只更新输入时刻，不重建目标/baseline 或重复扣预算；短暂 `0°` 暂停新输运且保留相位和残差；反号有效 Want 先终止旧 residual，再从**当前材料**建新相位 baseline，等待原 direction hold 允许后才发同号输运，旧方向不能借新 phase 计 grant。若验收要求相对同次原始截图两侧反向，须另计从当前形状到全局反向目标的真实成本，不用相位内 `-d` 假装达标。TEST/EXPERIENCE 输入模式切换只在现有 `resetInputState()` 边界清 route 私有目标、残差、baseline、phase sign 和稳定计数，保留材料、runId、physics/simulation time，并让下一条合法 Want 或 fresh sensor 从当前材料建相位；collect/restore/replay 只在 apply 成功后提交 `X/e` 与预算一次。当前 ETS 尚未实现这些 route 状态规则。

**§16 判定**：`BLOCKED-V29-ANGLE` 对旧四列方案已严格确认；新半波只在理想合成路径解除 angle，实际 ETS 仍缺逐子步证明，且预算明确失败。`BLOCKED-V29-RELEASE` 现有设计给出以实际 `bh` 为准的残差、容差、稳定窗和崩塌回正契约，但没有运行证据，也无法使预算失败的半波进入稳定态。因此两个阻断项均**未解除**，状态仍 `DESIGN_PENDING_HIL / BLOCKED`，不得交 architect。下一设计只能在不改预算、几何、相邻输运、原质量门槛和 `dual-input-v1` 的前提下，给出可变 share/其他通道耦合的完整 angle/成本证明与真实轨迹；若做不到，应由 manager/用户决定是否放弃此路线或调整项目约束，不能伪造 v2.9 过线。

### 6.12 实际动态 split 的覆盖边界：条件性不可行证明已完成，完整类别仍未解

§6.11 的 `B_total≥72.8` **只适用于每条边固定同一 share**，不能直接套到原 `splitBottomPhaseBalanced()`。当前函数按 donor 组成 `bh/(bh+bl)` 与全局 H/L 相位残差偏置求 share，再将**目标 share**裁剪到 `[0.35,0.65]`；若 donor 某相库存不足，后续补分逻辑可能使**实际** H/L 比例偏离此区间。下述代数只覆盖 stock 未耗尽且每笔**实际** share `p_j∈[0.35,0.65]` 的普通路径；stock fallback 要另做守恒、容量和现行 heavy-share 门槛核对，不能默认它提供免费的可行补偿。对任意正方向相邻 bottom grant，定义该相位累计总边 flux `G_j=H_j+L_j≥0`、heavy flux `H_j=p_jG_j`（`G_j=0` 时 share 无须定义），并令 `r_j=p_j-0.5∈[-0.15,0.15]`。从平坦 phase baseline 出发、暂只计 bottom 通道时，列增量满足

```text
totalDeltaS[c] = G[c-1] - G[c]
heavyDelta[c]  = H[c-1] - H[c]
               = 0.5*totalDeltaS[c] + r[c-1]*G[c-1] - r[c]*G[c]
```

这里 `G` 是**已提交**的相邻输运，受每子步 `sum grants<=10*dt` 及共享 `<=300*dt` 约束；collect request 不是 `G`。angle 只限制 `totalDeltaS` 与 `effectiveTilt`，不能单独限制组成调制项 `D(rG)`。例如仅在一段内部边上令 `G_j=g` 常数而 share 在相邻边交替 0.35/0.65，内部 `totalDeltaS=0`，heavy 却可有 `±0.30g` 的列变化；这只是展示**固定 share 下界不可直接推广**的代数反例，段首尾仍要满足 angle、stock/capacity、实际 split 状态和总预算，不能当可行 v2.9 profile。

可得到一个必要条件，但仍不足以作全类别否证。记 `s=totalDeltaS`、`d=heavyDelta`、`B=sum_j G_j`；flat baseline、单向 `G_j≥0`、+15° angle 上界 `U=tan15°+tan30°≈0.84530` 时，离散分部求和给 `sum_c s[c]²=sum_j G_j(s[j+1]-s[j])<=U*B`。若 heavy ROI RMS≥0.5，则 `||d||₂≥sqrt(104*0.5²)=sqrt26≈5.099`；上式分解要求

```text
||D(rG)||₂ >= sqrt26 - 0.5*sqrt(U*B)
```

以乐观 5.122s wall-clock 预算 `B<=51.22` 算，动态组成项必须贡献至少约 `1.81` 的 L2 范数；若沿用正式 v2.8 正相位 `sum(bottomLimit)≈27.22`，必要值增为约 `2.70`。`|r|<=0.15` 又给 `||D(rG)||₂<=0.30*||G||₂`，所以还必须有 `||G||₂` 至少约 `6.0/9.0`（对应上述两个预算情景）。这些只是**必要条件**，并未矛盾；share 范围本身不足以证明所有动态 split profile 不可达。其他 motion 通道若改变 `S` 或 light 位置，需把其逐列增量加入真实 angle 和成本，不能用 `G` 的公式代替完整产品轨迹。

要真正解除 `BLOCKED-V29-ANGLE`，必须给出一个具体动态 `targetDelta_H/L[c]` 与有序边 flux，使每个子步的 donor stock、receiver capacity、现有 `splitBottomPhaseBalanced()` 产生的 `p_j`、合成后**实际** `S` 斜率、bottom/total 逐笔 grant 同时满足约束，并证明正负相位都在正式有效时间内达到 heavy RMS/max；纯 `[0.35,0.65]` 的独立选边自由度不是现有函数的保证。还需从真实 `bh` 残差按 §6.11 的稳定窗口释放，崩塌时保持 angle 硬不变量、按现预算 `REPAIR`，预算或时间不足则 `BLOCKED`。半波的 `0.10/0.05` 与 `12` 子步/`0.20s` 只是**未实现的候选判别参数**，不能将成本失败的目标配置成成功。

因此本轮设计阶段已达到**固定比例同向类别的条件性不可行证明终点**，但没有对实际动态 split 完整类别作绝对不可行证明，也没有找到满足全部条件的新目标。剩余需要的精确计算是：由真实 split 公式及相位残差/stock 演化约束 `p_j` 的空间与时间变化，联立上述 `G/H/S` 关系求可行 flux 或更强的通用下界；任何结果还必须纳入其他通道的实际总沙高、逐子步 angle 和 release 稳定性。视觉 2px 已由用户定为局部 heavy 边界位移，不再待选；whole green mask 质心仅作诊断，不阻断这部分数学结论。当前记录为“**动态约束类未覆盖／无已证设计解**”，两个 §16 阻断继续有效，不交 architect，也不让 tester 或设备进入本轮。

### 6.13 真实 `splitBottomPhaseBalanced()` 的递推约束与最小缺失状态

只读源码确认常量 `P011_HEAVY_SHARE_BIAS=0.10`、目标 share 范围 `[0.35,0.65]`。每个有效非零相位的 `phaseHeavyIdeal` 和 `phaseLightIdeal` 同量增长，故二者在该相位任意时刻相等；share 偏置实际由两种**带方向符号**的累计量之差决定。对某次已通过 bottom/total 裁剪的相邻实际 grant `q>0`，令 `b=clamp(bh_donor/(bh_donor+bl_donor),0.35,0.65)`、`u=b-0.5`、`D=(phaseHeavyIdeal-phaseHeavyActual)-(phaseLightIdeal-phaseLightActual)=phaseLightActual-phaseHeavyActual`。在 donor H/L 库存均足、fallback 不触发时，实际 heavy 比例才等于

```text
p = clamp(0.5 + u + 0.10*D, 0.35, 0.65)
qH = p*q; qL = (1-p)*q
D_next = D + directionSign*(qL-qH)
       = D - directionSign*2*q*(p-0.5)
```

应用顺序是既有方向逆序边遍历；`D` 在**每条边**成功 split 后更新，故不能把所有边看作同一帧一个任意可选的 `p_j`。`q` 先受独立 bottom 与共享 total 剪裁；每子步所有有效 `q` 的和必须 `<=10*dt`，共享所有通道 `<=300*dt`。若 donor 一相库存不够，`extraHeavy/extraLight` 会改变实际 `qH/qL`，上式的 `p` 等于实际 share 的前提失效；这种 fallback 要以真实库存和既有守恒/质量门槛另列，不能称作 `[0.35,0.65]` 普通路径的免费调制。

**正相位的可证条件约束。** `directionSign=+1`，且 unclamped 普通路径时 `D_next=(1-0.20q)D-2uq`。若相位从 `D=0` 起，所有已执行边在 split 当下的 donor 组成均满足 `|u|<=δ<=0.075`，并且单笔 `q<=10dt<=1/6`，则归纳得 `|D|<=10δ`、`|p-0.5|<=2δ`：当 `|D|<=10δ` 时，`|(1-0.2q)D-2uq|<=10δ(1-0.2q)+2δq=10δ`，且 `|u+0.1D|<=2δ<=0.15` 保证夹取不激活。特别地，若初始 thick `bh=bl=72` 且只发生这种成对 bottom 输运，没有其他通道打破 H/L 配比，则 `u=D=0` 在每次 split 后保持，真实 split **严格为 0.5**，不能凭“允许动态 share”自发形成组成波。其他通道、已有材料历史或 stock fallback 可能破坏该不变量。

**再限定其他通道在该相位不贡献净 `bh` 形状或总沙高斜率**，才能把上界带回 §6.12 的 bottom-only `d=0.5s+D(rG)`：若单相位 bottom 已提交总量 `B=sum G`，`|r_j|<=2δ` 则 `||D(rG)||₂<=2||rG||₂<=4δ||G||₂<=4δB`。heavy RMS≥0.5 要求 `||d||₂≥sqrt26`，angle 给 `||s||₂≤sqrt(0.84530B)`，故在这一明确的 bottom-only 条件下，正相位的**必要 donor 组成偏差**为

```text
δ >= [sqrt26 - 0.5*sqrt(0.84530*B)] / (4B),
provided numerator > 0 and all above ordinary-path assumptions hold.
```

在最乐观的 `B=10*5.122=51.22` 时右侧约 `0.0088`；在已见正式正相位 `sum(bottomLimit)≈27.22` 的预算量级时约 `0.0248`。所以若将来能从**拟议新路线的每个实际 donor、每笔 split 前**证明 `δ<0.0088`，并同时证明其他通道的净形状/斜率贡献为零或有可并入的严格上界，即便乐观使用全部 wall-clock bottom 预算，仍可判该正相位不可能同时满足 RMS/angle/预算；若只保证 `B<=27.22`，对应更强的 `δ<0.0248` 条件。现有 v2.8 `lightShapeRms` 或最终截图不是新路线所有 donor 的 `max|b-0.5|`，不能代入证明。反过来，即使观测 δ 超过这些必要值，也**不**构成可行性证明，因为它没有给出可达的空间 share pattern、角度安全的边顺序或足够稳定的实际 `bh`。

**反相位不能镜像套正相位稳定界。** 当前代码把 `phaseHeavyActual/phaseLightActual` 乘 `directionSign` 累加；`directionSign=-1` 时，同一 `D` 递推变为 `D_next=(1+0.20q)D+2uq`（未夹取且库存充分）。从 `D=0,u=0` 起仍保持 0，但已有 donor 组成扰动会被此符号约定放大而非正相位的负反馈。该代数描述的是**现有实现**，不是对设计意图的推断；一旦 share 达夹取边界或 fallback 生效须分段重算。反向前必须按 §6.11 从当前材料重建 phase baseline 并清旧 `D/F/X`，不能拿正相位的条件界冒称负相位可行。这个不对称也可能影响质量分数及方向稳定，需在任何新目标中被显式约束。

**最终可审查结论与所缺的最小状态。** 在底层预算、同向相邻 H/L、默认 thick 平坦起点及无外部 H/L 组成改变的特例，真实 split 恒为 0.5，§6.11 已严格否证 RMS/angle/预算同时通过。对有其他通道参与的实际产品，本轮仍缺：每个待执行边 split 前 donor 的 `bh/bl` 与 stock fallback 标记；本相位按实际顺序逐笔 `q,D,p`；其他通道在每子步的 `Δbh/Δbl/Δmh/Δml`；有效 physics `dt` 与实际 bottom/total grant。只有这些**状态上界或同等严格解析界**才能判断 δ 条件、完成动态 share 的全局可达性证明或反例，并将完整 `sandTop` angle 和 §6.11 的实际 `bh` residual 稳定窗口一起审查。此处没有声称绝对无解；保留“动态约束类未覆盖／无已证设计解”、`BLOCKED-V29-ANGLE` 与 `BLOCKED-V29-RELEASE`。半波的 release `0.10/0.05`、12 子步/0.20s 仍只是假设规格；崩塌可在原预算内 `REPAIR`，任何 angle 越界逐子步拒绝，预算耗尽 `BLOCKED`，不借修复绕门槛。2px 已定为局部 heavy 边界位移。

### 6.14 未批准路线的通用状态机契约、复杂度与未来断言

**前置条件**：此契约只有在另有一个同时满足实际动态 split、angle、预算与重沙形状的 `targetDelta_H/L` 且经设计审查后，才可作为实现输入。§6.3 四列与 §6.11 半波均是失败反例；下列伪代码**不**提供可行目标，不表示已解除两个 §16 阻断。`RMS_e<=0.10/max_e<=0.05`、12 子步/0.20s、崩塌连续 2 子步均为以失败半波说明释放逻辑的**候选**，不是产品/测试阈值。

```text
state = INACTIVE | TRACKING | STABLE | REPAIR | BLOCKED
fullSceneRunReset(): reset existing scene/run; clear route F,X,bh0,target,phaseSign,stableClock
inputModeReset(): clear route F,X,bh0,target,phaseSign,stableClock only
                 preserve bh/bl/mh/ml, runId, physicsTime, dual-input first-command rules
on valid nonzero sign change:
    close old phase; capture current bh as bh0; clear X and stableClock
    require a separately approved targetDelta and adjacent flux F
    wait for existing direction hold to permit that sign; then state=TRACKING
on repeated same-sign Want: keep bh0,target,F,X,stableClock
on short zero tilt: pause bottom grants; retain phase and actual-shape residual

for each physics substep:
    collect requests only; do not commit route state or debit budgets
    restore material and speculative route state before apply
    apply candidate bottom adjacent grants under donor stock, receiver capacity,
        actual split share, bottom<=10*dt, shared total<=300*dt;
        each successful transfer changes X and both budget ledgers exactly once
    apply other channels using remaining shared budget and current material limits
    before final commit, audit every actual sandTop neighbor against effectiveTilt,
        plus capacity, conservation, direction, maxSubstepBh and both budgets;
        reject/rollback any violating substep now, without a grace window
    e[c] = actual_bh[c] - bh0[c] - targetDelta[c]
    residualRms = sqrt(sum_ROI(e[c]^2)/104); residualMax=max_ROI(abs(e[c]))
    if blocked by stock, angle or insufficient remaining grant/time: state=BLOCKED
    if state in {TRACKING, REPAIR} and actual residual and all hard gates
       hold for candidate stable window: state=STABLE
    if state==STABLE and actual shape residual collapses for candidate 2 substeps:
       state=REPAIR; retain bh0,target; recompute needed adjacent grants from actual bh
    if REPAIR cannot restore shape within existing grants/capacity/time: state=BLOCKED
    emit distinct request, actual H/L grant, F-X, actual-shape residual and state
```

该事务伪代码不意味着当前 collect/restore/replay 顺序已能做到全通道 rollback，也不允许用多次不计成本的重试躲过 angle。若找不到既保持其他通道语义又满足逐子步完整 `sandTop` 的一次有界 apply，设计仍 `BLOCKED`。若 `STABLE` 后仅 `F-X=0` 而实际 `bh` 已塌回，必须进入 `REPAIR`；若硬 angle 不变量已破，立即拒绝该子步，绝不等连续 2 次。`BLOCKED` 是诊断失败态，不向测试或主观门槛报 PASS。

**复杂度目标**：设 `N=cols=128`、有向相邻目标边数 `m<=N-1`。计算 phase target/逐列 `e`/完整 `sandTop` angle 是每子步 `O(N)`，实际边 grant 为 `O(m)`，合计仍应为 `O(N+m)=O(N)`；route 私有 `bh0/target/F/X` 最多 `O(N+m)=O(N)` 空间。目标预处理、稳定计数与每笔 budget ledger 不需要 `O(N²)` 或跨列直接搬运。若引入不受限回滚重试、排序或全场多 pass，这个性能声明失效，须重新评估现行性能目标；目前没有实现、计时或内存证据。

**拟议未来验收断言，仅供路线设计复核，不修改/启动测试资产**：

1. 默认 thick `128/192/28`、`dual-input-v1`、完整原 0/+15/0/-15/0 与 531/sequence/100ms 规则不变；未来按**用户指定形式验收**，各有效子步对 127 条边用实际 `bh+bl+mh+ml` 和 `effectiveTilt` 核对 `|ΔS-tan(theta)|<=tan30°`，含 0°、反向、其他通道应用后与 repair 时刻。另记录实际 thick `angleDeg=4` 的 `relaxAngle` 请求/应用及崩塌对形状的影响；不能把 tan4 触发条件误当 tan30 验收，也不能把 tan30 静态目标证明当作 tan4 下稳定证据。
2. 每子步 `bottomActualGrant<=10dt`、`totalActualGrant<=300dt`；各已提交 H/L/其他 grant 与 ledger 对齐，只扣一次；材料守恒、库存、容量、TC03 `maxSubstepBh`、原质量分数/性能目标均按现行门槛。collect 请求和假设性 reserve 不算 grant。
3. 相位 release 同时满足目标 heavy RMS/max 原硬门槛、实际 `shapeResidualRms/Max` 候选容差及连续稳定窗；`F-X=0` 不单独放行。崩塌触发 `REPAIR` 后须在原预算内重新稳定；预算/容量/时间不足显式 `BLOCKED`，不能借重置 baseline 假达标。硬 angle/预算违反当子步拒绝，不等待稳定窗。
4. 冷 scene/run reset、同号连续 Want、短 0°、反号及方向 hold、TEST/EXPERIENCE 双向模式切换分别核对 phase/baseline/residual 生命周期；模式切换保留材料、runId、物理时间和 dual-input 首 Want/fresh sensor 语义，无旧路线残差重放。
5. 四列 `A=2.7` 必因 angle、平滑半波 `a=1.1` 必因 bottom 成本、固定 share 类必因 §6.11 下界判为设计失败反例；动态 share 的正/负 `D` 递推、每边 donor 基础比例、fallback 与其他通道贡献必须被约束或实测，未证明可行时不得标 `READY_FOR_ARCHITECT`、物理 PASS 或主观 PASS。
6. 2px 已由用户定为局部 heavy 边界位移，whole green mask 质心仅作诊断；不把它增作 TC-P011-V28-00 客观硬门槛，不改原主观 HG/≥6/10。

**本轮最终状态**：`DESIGN_PENDING_HIL / BLOCKED-V29-ANGLE / BLOCKED-V29-RELEASE`。以上是审查清单和状态机契约，尚无满足前置条件的 v2.9 目标，不交 architect，不启动 tester/effector。

### 6.15 全部现有通道与真实 thick 运行参数：全类别证明仍缺的约束

**先分清两个角度条件。** 用户本轮指定 `|S[c+1]-S[c]-tan(effectiveTilt)|<=tan30°`，§6.11–§6.14 的 tan30 计算均是对此**形式验收**的条件性推导。源码 `WaterSandField.ets` 的 `defaultWaterSandParams()` 虽给 `angleDeg=30`，但 `SandCanvas.ets` 在 `thick` preset 的 `configure()` 前覆盖为 `angleDeg=4,maxAvalancheFlux=80,bottomCreepRate=96`；`relaxAngle()` 使用实际 `params.angleDeg`。所以正式 thick 在有效 +15° 时，**不触发崩塌的坡度带**为 `[tan15°-tan4°,tan15°+tan4°]≈[0.19802,0.33788]`，-15° 为 `[-0.33788,-0.19802]`；用户的 tan30 合格带分别为 `[-0.30940,0.84530]` 与 `[-0.84530,0.30940]`。前者是运行时**请求崩塌的触发条件**，并非脚本验收 hard gate，更非运行时自动维持的严格不变量。§6.11 半波的理想 `|ΔS|<=0.13284` 通过 tan30 形式约束，但在 +15° plateau 若无其他通道改变 `S`，每条边均低于实际 tan4 带下缘，会请求崩塌；因此其“角度安全”不能推出运行稳定。架构 §16 的 `angleDeg=30` 运行参数归因需要更正；本轮不能改架构文件，亦不更改产品参数。

**通道守恒分解说明 bottom 下界的边界。** `SandCanvas.ets` 对 thick 每帧调用 `setShakeInput(min(1,abs(effectiveTilt)/90))`；±15° plateau 输入为 `1/6`，不是无摇动。初始 thick 确为每列 `bh=72,bl=72,mh=ml=suspH=suspL=0`，但后续 `liftByShake()` 可把顶层材料转成悬沙，`diffuseSuspension()` 可在相邻列移动悬沙，`settleSandV2()` 将悬沙加进 `mh/ml`，`percolateHeavyConservative()` 又把 `mh` 转成 `bh`。具体对一列的渗滤 grant `q,r`，源码更新为 `Δmh=-(q+r), Δbh=q+r, Δbl=r-q, Δml=q-r`，所以 `ΔS=0` 而 `Δbh=q+r>0`；它扣共享 total，不扣 bottom 独立 `10dt`。`relaxAngle()` 的相邻 top grant 也扣共享 total，`splitTopV2()` 先取 mixed、再取 `bl`、最后取 `bh`，可直接减少 donor 的 `bh` 并把重沙放进 receiver 的 `mh`。摇动升起也可能在表层库存变化后取到 heavy。`diffuseSuspension()` 在 collect/restore/apply 之前运行，源码未把它记入这五种 motion 的共享 grant；但要影响 `bh` 还须有后续沉降/渗滤。水面重排只搬水，不直接改变 `S`/`bh`。这些都是现有代码允许的机制，**不是**已观察到足量重沙形状的证据。

令 `div(F)[c]=F[c-1]-F[c]`，边通量以从左到右为正、从右到左为负；它与 §6.13 的 share 残差 `D` 不是同一量。按已提交量累计 bottom 总边通量 `G`、bottom heavy 边通量 `H`、top 崩塌总边通量 `A`。另以 `T[c]>=0` 表示沉降对本列 `S` 的累计增加、`L[c]>=0` 表示摇动升起对本列 `S` 的累计减少；以 `P[c]=Σ(q+r)>=0` 表示渗滤对本列 `bh` 的累计增加，以 `R_H[c],L_H[c]>=0` 表示崩塌/升起从本列 `bh` 实际取走的累计 heavy。忽略 run 外注入/倒置、但允许上述既有通道时，按物料账本必须写成

```text
ΔS[c]  = div(G+A)[c] + T[c] - L[c]
Δbh[c] = div(H)[c] + P[c] - R_H[c] - L_H[c]
```

`H` 仍由真实 donor 比例、逐边残差 `D` 递推、clamp、库存 fallback 与有序 grant 决定；不能独立指定。五个 apply motion 的已提交非负 grant 合计须 `<=300dt`，其中 bottom `Σ_j|G_j|<=10dt`（同一边若有往复，应计**绝对已提交量**而非净通量）且只扣账一次；崩塌、沉降、渗滤、升起会竞争共享额度，悬沙扩散还有自己的速率与悬沙库存约束。§6.11 的 `B_total>=72.8` 和 §6.13 的 donor δ 下界同时假设 `A=T=L=P=R_H=L_H=0`（或对形状/坡度贡献可严格忽略）；**不能**把它们用于否定上述全通道策略。§6.11 的 whole-mask 连续质量 `0.0445px` 乐观上界也只覆盖普通 bottom heavy 一阶矩，不是所有合法通道或 RGB mask 的上界。

**为什么本轮仍不能给“全允许策略绝对无解”或可行 target。** 在用户 tan30 形式约束下，动态组成项 `div((p-0.5)G)` 可改变 `bh` 而不同比例改变 `S`；渗滤项 `P` 更可在 `ΔS=0` 时直接改变 `bh`。它也不是免费控制量：源码先以 `through=min(mh,bl)`、`pair=min(mh,ml)` 及 `requested=min(0.5(through+pair),percolationRate*dt*(through+pair))` 限请求，再经共享额度裁剪，实际 `q<=min(through/2,allowed)`、`r<=min(pair/2,allowed-q,mh-q)`；初始 `mh=0` 时无此通量。悬沙扩散每边还受默认 `heavyDiffusion*dt=4dt`、`lightDiffusion*dt=10dt` 和浓度差/接收容量限制。因此仅从两个预算、`p∈[.35,.65]` 与全边坡度，无法推出已要求的 `||Δbh_ROI||₂<sqrt26`：还缺这些额外项由现有初始库存和速率在同次 0/+15/0/-15/0 中**可达到的严格上界**。相反，任意给定 `P` 或 `p_j` 也不是合法构造；必须先经 tilt 驱动的 shake/崩塌、悬沙扩散、沉降、donor 演化和每笔 grant 生成，再在正负相位每个子步满足容量、守恒、TC03、全部硬门槛、两个预算及实际 `bh` release。现有正式证据只提供粗粒度样本，host probe 只验证原 ETS 与假想 bottom-first 预算算术；两者没有逐列 `A,T,L,P,R_H,L_H`、split 前 donor/fallback、相位 `D` 递推或实际形状残差的可达上界。故本轮**既没有覆盖所有真实动态 share/现有通道的无解证明，也没有可信的新 `targetDelta_H/L[c]` 和有序 grant**。这是一条具体的数学障碍，不是把未运行实验当作否证。

**给 manager 的裁决点与最小后续证据。** 先确认 tan30 是用户要求的额外逐子步形式验收，而 thick 的 tan4 崩塌运行参数保持现状；不要把两者合并成一个阈值。再决定是否允许 v2.9 route 在现行物理语义中**依赖**摇动、悬沙扩散、沉降、渗滤与崩塌形成 heavy 目标；若仅允许 bottom-only，初始均匀场的 share=0.5 不变量与 §6.11 下界已否证这类路线。若允许全通道，最低限度需要同一正式几何/输入序列的逐子步材料列数组与上述通道 grant/库存/fallback/残差账本，或等价的解析上界；之后才能构造/否证具体 `targetDelta_H/L`，审查实际 tan30 逐边约束、tan4 崩塌竞争、`e[c]=bh_actual-bh_phase0-targetDelta_H` 的候选稳定窗与崩塌后修复。2px 已定局部 heavy 边界位移，whole green mask 质心仅作诊断。此阶段可交付的是**条件性不可行证明与全类未闭合的原因**，不是 v2.9 方案批准；`BLOCKED-V29-ANGLE`/`BLOCKED-V29-RELEASE` 均保留，不交 architect/tester。

### 6.16 逐函数可达增量审计与一次具体构造尝试

以下全部是对当前 `WaterSandField.ets` / `SandCanvas.ets` 的只读代数审计；符号 `h,l,m_h,m_l,u_h,u_l` 分别代表 `bh,bl,mh,ml,suspH,suspL`，`S=h+l+m_h+m_l`，`q` 表示**实际提交**的非负 grant。表中共享账本只计 apply 阶段，不能把 collect 的 request 再扣一次；正式 thick 初始化是每列 `(h,l,m_h,m_l,u_h,u_l)=(72,72,0,0,0,0)`，`waterTop=182`、`S=144`、`capacity=192`。

| 函数/方向 | 单次实际列增量与约束 | 速率、账本及对 v2.9 的含义 |
|---|---|---|
| `waterLevel` | 只改 `waterTop`，按水位误差在列间搬水；`Δh=Δl=Δm_h=Δm_l=ΔS=0`，但会改后续水深/接收容量。 | 不计五种材料 motion 的 `300dt`；不能直接造 heavy 形状。 |
| `diffuseSuspension` | 对相邻高悬沙浓度列→低列分别传 `u_h/u_l`；`ΔS=Δh=0`，但本列**总 heavy** `h+m_h+u_h` 增量是相邻 `u_h` 通量的 `div`。 | 每边 heavy `<=4dt`、light `<=10dt`，并受浓度差一半与接收悬沙容量限制；位于 collect/apply 账本前，不扣五种 material motion 的共享额度。初始 `u_h=u_l=0`，先要由 lift/其他材料转换产生悬沙；重沙扩散后要影响 `bh`，仍需沉降及渗滤。故它既不能视为可任意操控，也不能在全类下界里删掉。 |
| `settleSandV2` | 本列 `u_h→m_h` grant `q_h`、`u_l→m_l` grant `q_l`；`Δh=Δl=0`、`ΔS=q_h+q_l`。 | 请求 `q_h^*=u_h min(1,64dt/depth)`、`q_l^*=u_l min(1,38dt/depth)`；实际经本列 `28dt` 与共享 `300dt` 裁剪。仅垂向，无 tilt 方向性。 |
| `percolateHeavyConservative` | 本列 `Δ(h,l,m_h,m_l)=(q+r,r-q,-q-r,q-r)`，因此 `ΔS=0`、`Δh=q+r>=0`，本列总 heavy/light 各守恒。 | `through=min(m_h,l)`、`pair=min(m_h,m_l)`；请求 `min(0.5(through+pair),6dt(through+pair))`，实际 `q<=min(through/2,allowed)`、`r<=min(pair/2,allowed-q,m_h-q)`；只扣共享 `300dt`，未调用 `clipSubstepMotion` 的本列 `28dt`。初始 `m_h=0`，不能凭空给正 `Δh`；自身无方向性。 |
| `creepBottomRequests`/`splitBottomPhaseBalanced` | 相邻 donor→receiver，donor `Δ(h,l)=(-q_h,-q_l)`、receiver `(+q_h,+q_l)`，`q_h+q_l=q`，对应 `ΔS=(-q,+q)`；方向由目标倾角与既有 80ms hold 决定，+ 倾角左→右、- 倾角镜像。 | 原请求每边 `<=requestRate*dt` 且 `requestRate<=10`，独立预算对全部边按请求比例缩到 `10dt`；apply 逐边再经共享 `300dt` 与即时库存/容量裁剪。当前底流不调用本列 `28dt` 裁剪，须另守 `maxSubstepBh` 门槛。普通实际 share 由 donor 组成和残差递推/clamp 决定，库存 fallback 另计；不能指定反向 light 来抵消其坡度。 |
| `relaxAngle`/`splitTopV2` | 依当前 `S` 的 `deviation=S[c+1]-S[c]-tan(effectiveTilt)` 决定较高列 donor；实际 `q` 使 donor `S-=q`、相邻 receiver `S+=q`。donor 依次消耗 mixed、`l`、最后 `h`，receiver 分别增加 `m_h/m_l`；若取到 `h`，本列 `bh` 直接下降。 | thick 触发 `|deviation|>tan4°`，请求 `min((|deviation|-tan4°)/2,80dt,S_donor,receiverCapacity)`；实际还经 donor/receiver 各 `28dt` 及共享 `300dt` 裁剪。流向按**局部坡度误差**，不等同 bottom 的固定倾角方向。参数 `avalanchePasses=5` 在当前 transfer 函数中无额外循环，不能另算五次 top-creep。 |
| `liftByShake`/`splitTopV2` | 本列顶层材料→`u_h/u_l`，`ΔS=-q`；只有当 mixed 与 `l` 先被取尽，才直接取到 `h`。 | thick 的 `band=min(S,0.5)`；在正式无额外输入、从零 shakeEnergy 出发且 `|effectiveTilt|<=15°` 时，`shakeInput,shakeEnergy<=1/6`，`desired<= (1/6)*0.5*40dt=(10/3)dt` **每列**，再经本列 `28dt` 与共享 `300dt` 裁剪。±15° 的 5.122s 即便全算物理时间，单列 lift 总请求 `<=17.07`，不足以单独耗尽初始 `l=72`；可与崩塌、bottom 等串联，不能因此排除 heavy lift。 |

`diffuseSuspension` 的 `4dt` 是**每边相邻悬沙 heavy** 上界，不是 `bh` grant、不是总系统额外 4/s、也不是无限免费运力；任意由此形成的 `bh` 水平重分布还须经过可达悬沙库存及后续共享预算的沉降/渗滤。上述速率均按每个有效 physics 子步 `dt<=1/60s`，5.122s 壁钟仅是乐观积分上界；正式 v2.8 约 2.722s physics time 不能自动推广为任何新路线的精确可用时间。

**尝试最省局部形状的全通道构造，并指出断点。** 在 104 列 ROI 内，令某列 `c*` 的相位 heavy 目标 `targetDelta_H[c*]=6.2`，其余 ROI 为 0，则理想 `heavyShapeRms=6.2/sqrt(104)≈0.608`、max=6.2；即使按候选 `RMS_e<=0.10/max_e<=0.05`，三角不等式仍给实际 RMS≥0.508、max≥6.15。若该列**事先可达**足量 `m_h` 与相应 `l/m_l` 配对库存，渗滤的 `q+r` 可以逐次把 mixed heavy 加进 `bh`，且每次 `ΔS=0`；初次若 `m_l=0` 则 `r=0`、`Δl=-q,Δm_l=+q`，随后 `r` 可变非零，不能把这一初次比例固定套用。目标所需累计真实 percolation 转换至少 6.2，**仅此项**的理想共享成本与乐观上限 `300*5.122` 不矛盾。这是从一个**假设已有 mixed heavy** 状态出发的松弛问题反例，说明 bottom `10/s` 不是全类无解证明。它不是可执行 v2.9 target：正式首相位从 `m_h=u_h=0` 开始，现有 lift/top slip 先取 mixed/light；要在指定列提供至少 6.2 `m_h`，须证明从初始 heavy 库存经 light 暴露、崩塌/升起、悬沙扩散/沉降的**有序**流入，同时保证源列的 `bh` 减少、全部 127 坡度、局部 `28dt`、共享 `300dt`、底流 `10dt` 与方向门槛。单列垂向升高也不等于已证局部边界沿倾角**水平同向**位移。渗滤在有库存时自治继续，未证明能精确停在残差候选并维持 12 子步/0.20s；且其 `Δbh>=0` 无法单独给 -15° 相位要求的反向重沙响应。反之，要证明它绝不可能，也须对这些前置流入作严格上界。两个方向在现有材料、速率和预算下均未闭合。

**全类不能被当前标量证据否证的精确原因。** 对普通 bottom-only，§6.11 已给 `B_bottom>=72.8>51.22`，但在完整账本中一个 `6.2` 量级的 `P` 或由 top/悬沙引入的 heavy 形状，形式上足以使 RMS 留有候选 release 余量，而已知共享乐观上限 `1536.6`、单列 `28*5.122≈143.4` 和悬沙扩散每边 `4*5.122≈20.49` 都**未**小于该量级；这些上界又不足以证明它们从零库存可达。缺的最小证据不是另一个总 flux 峰值，而是每子步、每列的 `(bh,bl,mh,ml,suspH,suspL,S,waterTop)`，每条相邻 bottom/top/suspension 的实际有符号 H/L grant、每列 settle/percolate/lift 的实际分相 grant、split 前 donor 比例/stock fallback/相位 `D`、`effectiveTilt/shakeEnergy`、本列 `28dt` 与共享/独立账本、phase baseline/实际 `e`。可用等价的解析可达上界替代轨迹。manager 的**二选一**是限定 bottom-only 并据已证下界终止此路线，或允许既有全通道并授权先取得上述状态/解析界再继续设计；两者都不批准当前 target 实施。局部 heavy 边界 2px 已定，仍需真实 raster 标定。两个 §16 blocker 保留。

#### 6.16.1 本轮结论：没有可批准的 v2.9 target

§6.16 的全通道审计不改变本轮状态。它只证明两点：第一，bottom-only 下界不能外推为“全物理类无解”；第二，现有逐函数速率和标量预算也不足以构造一个合法的 `targetDelta_H/L`。因此本轮**没有可交 architect 的 v2.9 方案**，更没有可让 developer 实现的路线。`BLOCKED-V29-ANGLE` 与 `BLOCKED-V29-RELEASE` 继续保留，状态仍为 `DESIGN_PENDING_HIL`。

#### 6.16.2 manager 裁决口径

manager 必须先二选一，不能跳到实现：

1. **Bottom-only 终止路线**：接受 §6.11 对普通 bottom-only 的 `B_bottom>=72.8>51.22` 下界，宣告当前 bottom-creep v2.9 类无解，转入新问题或约束重议。
2. **授权全通道可达性审计**：允许摇动、悬沙扩散、沉降、渗滤、崩塌共同参与，但不改任何预算、几何、门槛或守恒/崩塌约束；先取得逐子步可达状态账本或等价解析上界，再回到 designer 构造/否证 target。

同时确认两条口径：`tan30°` 是用户要求的形式验收；thick 运行时 `tan4°` 只是崩塌触发参数，不合并、不替代前者。2px 已定局部 heavy 边界位移，whole green mask 质心只作诊断。

#### 6.16.3 全通道路线的后续证据与验收

若 manager 选择全通道路线，下一步不是调参，而是交付可审查证据：

1. 默认 `128/192/28`、`dual-input-v1`、完整 `0/+15/0/-15/0` 输入下，逐有效子步记录每列 `(bh,bl,mh,ml,suspH,suspL,S,waterTop)`、每条 bottom/top/suspension 的实际有符号 H/L grant、每列 settle/percolate/lift 的分相 grant、split 前 donor 组成与 fallback、相位 `D`、`effectiveTilt/shakeEnergy`、局部 `28dt` 与共享 `300dt` 账本。
2. 每子步核对材料守恒、库存、容量、`maxSubstepBh`、bottom `<=10dt`、total `<=300dt`、完整 `sandTop` 的 `|ΔS-tan(effectiveTilt)|<=tan30°`；`relaxAngle` 请求另记为 tan4° 触发证据。
3. 任何候选 release 必须用实际 `bh` 残差满足原硬门槛 `heavyShapeRms>=0.5`、`heavyShapeMaxDelta>=1.0`、非零样本 `>=30`；`F-X=0` 不单独放行。候选的局部 heavy 边界位移还须同分辨率 raster 标定达到 `>=2px`；whole green mask 质心只作诊断。
4. 反向相位不得复用正相位的稳定界；必须证明实际 `bh` 反向响应、相位 baseline/reset、同号 Want、0°、方向 hold 与 TEST/EXPERIENCE 切换均无残留。
5. 只有上述证据或等价解析上界能构造可行 target 后，designer 才能升级状态交 architect；不能因为“全类未被否证”就标 `READY_FOR_ARCHITECT`，也不能把未运行实验当作 PASS。

#### 6.16.4 当前交付边界

本轮交付到此为止：只完成阻断归因、全通道可达性审计和后续证据契约；不改业务代码，不启动 tester/effector，不新增客观硬门槛，不放宽任何阈值。后续若 manager 选定路线，designer 需先补齐证据，再进入 architect→developer→tester/effector 流程。

---

## 7. v2.10 方案：natural-observation-v2.10

**经验**：指标应从自然运动的观测里量出来，而不是让自然运动去凑指标。

### 7.1 路线状态与方向修正

- **routeId**：`natural-observation-v2.10`
- **状态**：`OBSERVATION_PENDING_HIL`
- **核心方向**：观测优先。先让现有自然规则完整运动，再从实际状态与真实 raster 中量化重沙界面的可见变化幅度与时间尺度；本方案不预设中央 4 列、±2.7 鼓包、目标曲线、`targetDelta_H/L` 或任何逐列搬运编排。
- **对 v2.9 的处置**：`bottom-creep-v2.9-central-dune-heavy-interface` 的目标曲线/逐列搬运设计标记为 `REJECTED`。被否原因是因果方向反了：它先画出目标形状，再把 `creepBottom` 改造成逐列搬沙工具，使崩塌规则与搬运机制互相竞争；§6.16 的阻断正是这场内部冲突的证据。
- **不继承的内容**：不继承 v2.9 的目标曲线、逐列 donor/receiver 编排、以构造合法 target 为目的的 release 设计。
- **保留的内容**：保留 §6.16 的逐子步状态账本、守恒核对与 hard gates；其用途从“构造合法 target”改为“支撑自然运动观测与物理上限测量”。

### 7.2 数学/逻辑原理

把现有自然规则表示为状态转移算子：

```text
S[k] = Φ(S[k-1], u[k])
```

其中：

- `S[k]` 为第 `k` 个有效子步的完整沙水状态，包含每列 `(bh, bl, mh, ml, suspH, suspL, S, waterTop)`。
- `u[k]` 为该子步的输入与相位，包括 `effectiveTilt`、`shakeEnergy` 与 `dual-input-v1` 命令。
- `Φ` 只由现有自然规则组成：`relaxAngle` 崩塌、`creepBottom` 蠕变、重/轻沙沉降差速、既有沉降/渗透/扬起/悬沙通道。

本方案不引入期望形状 `T`，而是观测可达状态在测量泛函 `M` 下的轨迹：

```text
y[k] = M(S[k])
```

`M` 输出重沙界面位置、每列重沙库存、可见 raster 变化、通量速率与相位响应，而不是与目标曲线的距离。完整输入序列下的观测结果构成当前规则与约束下的**经验物理上限**：

```text
R_obs = { y[k] : S[k] = Φ^k(S[0], u[1:k]) }
```

若 `R_obs` 的可见变化很小，则只能得到“现有规则在该场景下的上限很小”的结论；不能据此反推一个更强目标，也不能把搬运机制改造成目标执行器。

### 7.3 三步路线

#### 第一步：先跑，别画目标

在默认场景下完整执行倾斜过程：

- 几何：`128/192/28`
- 输入模式：`dual-input-v1`
- 相位序列：`0/+15/0/-15/0`
- 规则：仅使用现有自然规则，不新增目标、不调参、不修改通道优先级。

全程记录自然运动产生的实际变化，重点回答两个问题：

1. 重沙界面/重沙分布发生了多大的可见变化？
2. 这个变化需要多久才显现，正向与反向是否对称？

#### 第二步：测出的就是物理上限

若自然运动下重沙界面可见变化很小，必须区分两种可能，不得直接归因为“算法失败”：

1. **规则驱动力不足**：运输方向正确但速率或触发强度不足，例如 `creepBottom` 的 10 列/秒限制主导了时间尺度，或崩塌/蠕变没有足够的持续驱动。
2. **变化本来就慢**：状态逐渐接近自然稳定形态，驱动力随形态变化衰减，真实湿沙也可能表现出这种慢响应。

#### 第三步：再决定改哪边

根据观测证据二选一：

- **规则弱 → 改物理**：在既有预算、几何、门槛、守恒与崩塌约束内加强驱动。候选方向包括双角度滞回（触发角与延续角分离，使崩塌渐进）与重力直接驱动项（按 `effectiveTilt` 产生有界、守恒的输运）。这些是后续候选，不在 v2.10 中实现。
- **变化本来就小 → 改预期**：校准指标或接受现状，明确记录自然上限与产品预期之间的差距。不得为了达标而修改用例或阈值。

### 7.4 观测协议与数据契约

沿用 §6.16 的证据契约，但目的改为自然运动观测：

1. **逐子步状态账本**：记录每列 `(bh, bl, mh, ml, suspH, suspL, S, waterTop)`、每条 bottom/top/suspension 通道的实际有符号 H/L grant、每列 settle/percolate/lift 的分相 grant、split 前 donor 组成与 fallback、相位 `D`、`effectiveTilt/shakeEnergy`、局部 `28dt` 与共享 `300dt` 账本。
2. **守恒与约束核对**：每子步核对材料守恒、库存、容量、`maxSubstepBh`、bottom `<=10dt`、total `<=300dt`、完整 `sandTop` 的 `|ΔS-tan(effectiveTilt)|<=tan30°`；`relaxAngle` 请求另记为 tan4° 触发证据。
3. **真实 raster 标定**：重沙界面位移必须在同分辨率真实 raster 上标定，不用推测轮廓或 whole green mask 质心替代；后者只作诊断。
4. **相位完整性**：完整覆盖 `0/+15/0/-15/0`，记录 baseline、正相、回零、反相、再回零的残余响应。
5. **诊断不改变物理**：账本与 raster 采集为观测通道，不得反向影响状态转移、预算分配或通道优先级。

### 7.5 观测指标

以下指标均为**观测输出**，不是本轮的目标曲线或效果门槛：

1. **可见幅度**
   - `A_B+`：正相期间重沙界面边界相对 baseline 的最大绝对位移，单位 px。
   - `A_B-`：反相期间同一指标。
   - `A_H`：重沙可见面积或像素数的最大变化量。
   - `ΔHeavy(x)`：每列重沙库存的最终变化向量。
2. **时间尺度**
   - `t50+`、`t90+`：正相达到峰值变化的 50%、90% 所需时间。
   - `t50-`、`t90-`：反相对应时间。
   - `τ`：若轨迹近似单调收敛，用指数或分段线性拟合估计松弛时间。
3. **方向与对称性**
   - `sign(A_B+)` 与 `sign(A_B-)` 是否相反。
   - `symmetry = |A_B+| / max(|A_B-|, ε)`，记录不对称性，不设目标。
4. **残余与回零**
   - 每个回零相位结束时的边界残余位移。
   - TEST/EXPERIENCE 切换后的残余差值。
5. **驱动诊断**
   - 每子步 bottom/top/suspension 的实际通量、预算使用率、被拒绝或截断的 grant。
   - `relaxAngle` 请求、触发与未触发证据。
   - `creepBottom` 的速率限制命中情况。
   - 状态是否进入稳定形态：通量衰减率、连续无变化子步数、崩塌请求是否消失。

### 7.6 判定树

实验完成后按以下顺序判定：

1. **实验有效性**：若账本缺失、守恒失败、hard gates 失败或相位不完整，本轮观测无效，先修复观测/执行链路，不改物理规则。
2. **可见变化成立**：若同分辨率 raster 中 `A_B+` 或 `A_B-` 达到既有 `>=2px` 可见判据，则记录自然上限为该幅度与时间尺度；下一轮再决定是否校准产品预期或在该自然基础上加强物理。
3. **可见变化不足**：若两者均 `<2px`，结合驱动诊断分流：
   - **规则驱动力不足**：通量方向正确但持续受限，或速率/触发限制主导时间尺度，且状态尚未进入自然稳定形态。后续在既有约束内设计物理增强。
   - **变化本来就慢**：驱动力随状态接近稳定形态而衰减，松弛时间超过观测窗口，且无守恒/门槛违规。后续校准指标或接受现状。
4. **证据不足**：若无法区分上述两类，输出缺失数据清单，不得将推测写成结论，也不得升级为 `READY_FOR_ARCHITECT`。

### 7.7 伪代码

```pseudo
function observeNaturalLimit():
    resetSimulation(defaultScene)
    ledger = new Ledger()
    metrics = new Metrics()

    for phase in [baseline, plus15, reset, minus15, resetAgain]:
        command = makeDualInputCommand(phase)
        enqueue(command)

        while phase.isActive():
            stateBefore = readFullState()
            grantsBefore = readChannelRequests()

            executeOneNaturalSubstep()

            stateAfter = readFullState()
            grantsAfter = readChannelGrants()

            audit = auditConservationAndHardGates(
                stateBefore, stateAfter, grantsAfter
            )
            ledger.append(
                phase, stateAfter, grantsAfter,
                audit, effectiveTilt, shakeEnergy
            )
            metrics.update(
                stateAfter,
                renderRasterAtNativeResolution(),
                grantsAfter
            )

            if audit.failed:
                return ObservationResult.invalid(audit)

    summary = summarize(metrics, ledger)
    classification = classifyNaturalLimit(summary)
    return ObservationResult.valid(summary, classification)

function classifyNaturalLimit(summary):
    if summary.phaseIncomplete or summary.auditFailed:
        return invalid

    if summary.maxBoundaryShiftPx < 2:
        if summary.transportDirectionCorrect and
           summary.rateLimitDominant and
           not summary.nearNaturalEquilibrium:
            return ruleDriveTooWeak

        if summary.fluxDecaysToEquilibrium and
           summary.relaxationTimeExceedsWindow:
            return intrinsicallySlow

        return insufficientEvidence

    return visibleNaturalLimitObserved
```

### 7.8 输入、输出与复杂度

**输入**

- 默认几何 `128/192/28`
- `dual-input-v1`
- 完整相位序列 `0/+15/0/-15/0`
- 现有自然规则与既有预算、几何、门槛、守恒/崩塌约束
- 诊断账本与 raster 采集通道

**输出**

- 逐子步状态与通量账本
- 守恒与 hard gate 审计结果
- 重沙界面位移、重沙分布变化、时间尺度与方向性摘要
- “规则驱动力不足 / 变化本来就慢 / 可见自然上限 / 证据不足”的分类结论
- 后续物理增强或指标校准建议

**复杂度**

- 设有效子步数为 `K`，宽度为 `W=128`：
  - 状态与通量采集：`O(KW)`
  - 守恒与约束审计：`O(KW)`
  - raster 标定：`O(KR)`，`R` 为每帧渲染像素数
  - 账本存储：`O(KW)`；若保存完整 raster，则为 `O(KR)`
- 诊断通道不改变仿真转移复杂度。设备端可将完整账本用于实验/评审，量产路径继续使用现有自然规则，不默认携带逐子步全量日志。

### 7.9 验收与测试方法

本轮验收对象是**观测实验是否成立**，不是效果是否达标：

1. 完整执行默认场景与 `0/+15/0/-15/0`，不得跳过回零或反相。
2. §6.16 状态账本、守恒核对与 hard gates 全部通过；任何失败先判为实验/实现链路问题。
3. 输出 `A_B+`、`A_B-`、`A_H`、`t50`、`t90`、`τ`、方向性、残余与驱动诊断，缺失项必须列入 blocker。
4. 重沙界面位移必须来自同分辨率真实 raster；`2px` 只作为既有可见判据，不新增、不放宽、不替换原门槛。
5. 分类结论必须引用账本证据；无法区分时输出“证据不足”，不得选择性地把慢响应解释为物理弱或目标未达成。
6. 实验完成前，本方案状态保持 `OBSERVATION_PENDING_HIL`；不得标 `READY_FOR_ARCHITECT`，不得启动物理增强实现。

### 7.10 约束与 ASSUMPTION

**不变约束**

- 不修改任何预算。
- 不修改任何几何。
- 不修改任何门槛。
- 不修改守恒约束。
- 不修改崩塌约束。
- 后续若选择物理增强，也必须在上述约束内设计；不得通过放宽约束来制造可见变化。

**ASSUMPTION**

1. `natural-observation-v2.10` 仍是观测实验设计，尚未运行实验；在取得完整账本与 raster 证据前，不得升级为 `READY_FOR_ARCHITECT`。
2. 现有自然规则可以在默认场景完整执行倾斜过程；若执行被环境或实现问题阻断，先归因执行链路，不改物理规则。
3. 逐子步账本和 raster 采集只用于观测，不影响状态转移。
4. `>=2px` 是既有可见判据，仅用于判断自然变化是否可见，不是本轮新设效果目标。
5. “规则弱”与“变化慢”的区分依赖通量、预算使用、稳定形态与松弛时间证据；证据不足时必须显式保留不确定性。

---

## 8. v2.11 方案：gravity-advection-v2.11

### 8.1 路线状态

- **routeId**：`gravity-advection-v2.11`
- **状态**：`DESIGN_REVIEW_REQUIRED`（方案候选，外部评审问题解决前不交 architect/developer）
- **核心目标**：在不改变预算、几何、门槛、守恒与崩塌约束的前提下，让自然倾斜产生可见的重沙界面运动。
- **设计原则**：继续保持 v2.10 经验——指标从自然运动观测里量出来，不预设目标形状。本方案不引入中央 4 列、±2.7 鼓包、目标曲线、`targetDelta_H/L` 或逐列搬运编排。

### 8.2 观测证据与裁决

依据 `docs/observation-report-P011-natural-observation-v2.10.md` 与 `test/artifacts/p011-v210-natural-observation-20261019-run7/`：

- 判定为 `ruleDriveTooWeak`，不是 `intrinsicallySlow`。
- `+15` / `-15` 各 `5.1 s` 窗口内，重沙界面 raster 位移均为 `0 px`，模型单位最大位移仅 `4.2833e-4`。
- `+15` 相位 bottom 请求 `5522.6356`，实际 `10.0245`；`-15` 相位请求 `5540.3750`，实际 `9.7183`；两相位 `262/262` 子步全部受限。
- 重沙 `settle/lift/percolate/slip/suspensionDiffusion` 全为 `0`，仅 `bottomCreep` 约为 `5` model units；轻沙各通道有明显 grant。
- `transportDirectionCorrect=true`、`rateLimitDominant=true`、`nearNaturalEquilibrium=false`。

因此 v2.11 走 v2.10 既定的“改物理”分支，不校准指标、不接受现状。

### 8.3 候选评估

#### 8.3.1 双角度滞回

- **结论**：本轮不采用。
- **原因**：
  1. v2.10 证明 `relaxAngle` 已持续请求并应用，失败点不是触发过于突然，而是重沙无法进入有效搬运通道。
  2. 仅改变触发/延续角度仍主要作用于表层轻沙，无法直接改善 `bh` 的可达性。
  3. 引入第二个角度会带来新的内部阈值语义，容易与“不改门槛”的硬约束冲突。
- **保留条件**：若 v2.11 出现表面坡度振荡或 `relaxAngle` 与重力交换互相打架，再单独评估滞回，不与重力项混在同一次变更中。

#### 8.3.2 重力直接驱动项（采用）

- **结论**：采用。
- **理由**：
  1. 观测显示方向正确、非自然平衡，且瓶颈集中在重沙可达性与 bottom 限流。
  2. 重力直接驱动项可以在不改预算的情况下，把已有的 bottom 通道从“均匀总量搬运”改为“由局部坡度与重沙富余共同驱动的重/轻对向交换”。
  3. 该机制完全由当前状态决定，不依赖目标形状。

### 8.4 数学模型

#### 8.4.1 方向与局部势

设：

- `s = sign(effectiveTilt)`，`s>0` 表示 donor 为左列、receiver 为右列；`s<0` 表示 donor 为右列、receiver 为左列。
- `surfaceDrive = s * (sandTop[donor] - sandTop[receiver])`。
- `heavyExcess = bh[donor] - bh[receiver]`。
- `potential = surfaceDrive + heavyExcess`。

`potential > 0` 表示在重力方向上，donor 相对 receiver 具有更强的 downhill 或重沙富余驱动。`potential <= 0` 时不发生本接口的重力交换。

#### 8.4.2 交换量

使用既有 `bottomCreepVelocityGain` 作为物理增益，不新增参数：

```text
requestRate = bottomCreepVelocityGain * max(0, potential)
q = min(
  requestRate * dt / 2,
  vBottomCreepMax * dt / 2,
  bh[donor],
  bl[receiver]
)
```

`/2` 的含义是：一次交换同时移动 `q` 单位重沙与 `q` 单位轻沙，总运动量为 `2q`，必须计入既有 bottom 与 total motion 预算。

#### 8.4.3 状态更新

```text
bh[donor]   -= q
bh[receiver] += q
bl[donor]   += q
bl[receiver] -= q
```

性质：

1. 每列 `bh + bl` 不变，因此 `sandTop` 不变。
2. 重沙总量与轻沙总量分别守恒。
3. 不改变容量、几何或相邻坡度。
4. 重沙界面在 donor 侧下降、receiver 侧上升，产生自然分选，而非目标形状拟合。

#### 8.4.4 预算

- bottom 预算消耗：`2q`。
- total motion 预算消耗：`2q`。
- `vBottomCreepMax`、`totalMotionBudget`、`vSandTopMax`、`maxAvalancheFlux`、`angleDeg`、`bottomCreepDeadZone` 均不变。

### 8.5 伪代码

```pseudo
function gravityAdvectionExchange(dt):
    if abs(effectiveTilt) < bottomCreepDeadZone:
        return

    directionSign = sign(effectiveTilt)

    for each adjacent interface (left, right) in downhill order:
        donor = directionSign > 0 ? left : right
        receiver = directionSign > 0 ? right : left

        surfaceDrive = directionSign * (sandTop[donor] - sandTop[receiver])
        heavyExcess = bh[donor] - bh[receiver]
        potential = surfaceDrive + heavyExcess

        if potential <= 0:
            continue

        requestRate = bottomCreepVelocityGain * potential
        requestedHalf = requestRate * dt / 2
        budgetHalf = vBottomCreepMax * dt / 2

        q = min(
            requestedHalf,
            budgetHalf,
            bh[donor],
            bl[receiver]
        )

        if q <= EPS:
            continue

        if bottomBudgetRemaining < 2*q or totalMotionBudgetRemaining < 2*q:
            q = min(q, bottomBudgetRemaining/2, totalMotionBudgetRemaining/2)

        if q <= EPS:
            continue

        bh[donor] -= q
        bh[receiver] += q
        bl[donor] += q
        bl[receiver] -= q

        consumeBottomBudget(2*q)
        consumeTotalMotionBudget(2*q)
        recordGravityExchange(donor, receiver, q, potential)
```

### 8.6 输入、输出与复杂度

**输入**

- `effectiveTilt`
- `bottomCreepDeadZone`
- `bottomCreepVelocityGain`
- `vBottomCreepMax`
- `totalMotionBudget`
- `sandTop[]`
- `bh[]`
- `bl[]`

**输出**

- 更新后的 `bh[]`、`bl[]`
- bottom / total motion 预算消耗
- 每条接口的 `donor/receiver/q/potential`
- 重沙界面位移、方向、幅度与时间序列

**复杂度**

- 每子步遍历 `W-1` 条接口：`O(W)`。
- 额外状态：`O(1)`，不需要新数组。
- 端侧开销与现有 bottom creep 同阶，不引入新的嵌套循环或全局搜索。

### 8.7 验收与测试方法

1. 场景与输入保持 v2.10 口径：`128/192/28`、`dual-input-v1`、完整 `0/+15/0/-15/0`。
2. 硬门槛不变：材料守恒、库存、容量、`maxSubstepBh`、bottom `<=10dt`、total `<=300dt`、完整 `sandTop` 的 `|ΔS-tan(effectiveTilt)|<=tan30°`。
3. 重力交换后逐列 `bh+bl` 与交换前一致，`sandTop` 不因该算子改变。
4. 同分辨率 raster 上，`A_B+` 与 `A_B-` 至少一个方向达到既有 `>=2px` 可见判据；两个方向均应记录，不做全量 dominance。
5. `A_H`、`t50`、`t90`、方向性、回零残余与 TEST/EXPERIENCE 切换残余必须输出；`t90` 仍按既有 P95 `<=100ms` 口径评估。
6. 不得因预算耗尽而伪造达标；若 bottom 或 total 预算受限，必须输出受限子步数与截断量。
7. 报告独立成文：`docs/test-report-P011-gravity-advection-v2.11.md`、`docs/effect-report-P011-gravity-advection-v2.11.md`。

### 8.8 约束与 ASSUMPTION

**不变约束**

- 不修改任何预算。
- 不修改任何几何。
- 不修改任何门槛。
- 不修改守恒约束。
- 不修改崩塌约束。
- 不预设任何目标形状。

**ASSUMPTION**

1. `bottomCreepVelocityGain` 目前在实现中未被使用，v2.11 将其激活为重力交换增益；这不新增参数，也不改变预算。
2. 现有测试 `p011_velocityGainDoesNotOverrideShapeRateCap` 断言该参数无状态影响，属于旧契约；实现前须由 manager/tester 修订该测试，不得由 designer 私自改用例。
3. 重力交换保持 `bh+bl` 与 `sandTop` 不变，因此不会与 `relaxAngle` 的角度约束直接冲突。
4. 若 tester/effector 后续发现观测证据或主观基线与本设计冲突，先修订设计，不通过放宽门槛或改预算解决。

### 8.9 外部专家复核与设计修订要求（2026-10-19）

**判定**：重/轻对向交换的逐列守恒性质成立，但原 §8.4 的 `surfaceDrive` 在默认初态没有倾角驱动，且未证明能在中央 ROI 形成非零 heavy 通量散度。状态修订为 `DESIGN_REVIEW_REQUIRED`；不以 architecture §18 的 `READY_FOR_DEVELOPER` 替代算法可行性证明。

#### 默认场景冷启动

`SandCanvas.waterSandScene('thick')` 初始化 128 列 `bh=72, bl=72, mh=ml=suspH=suspL=0`，各列 `sandTop=144`。因此在第一个 gravity plan 读取状态时，对任意相邻接口：

```text
surfaceDrive = sign(effectiveTilt) * (144 - 144) = 0
heavyExcess = 72 - 72 = 0
potential = 0
q = 0
```

首个子步的 `diffuseSuspension` 虽在预算收集前执行，但初始悬沙为零，故该次扩散不改材料；其余 collect 分支只收集 settle/percolate/creep/relax/lift 请求，不提交这些通道的材料变化。因此 gravity plan 首个请求严格为零。后续 substep 的 `diffuseSuspension` 会在 collect 前实际改变悬沙，不能笼统视为只读。application 后现有通道也可改变边界状态，故“之后永远不触发”尚未由此首步证明；但 v2.11 必须给出第一个子步之后的有序状态递推，并报告首步、1s、5.1s 的 `potential` 与实际累计 `q`，不能把非零 tilt 本身当成非零 `potential`。

#### 驱动坐标与符号

既有 `relaxAngle()` 的目标坡度为 `S[c+1]-S[c]=tan(effectiveTilt)`；既有 downhill donor 顺序在正倾角为左→右。原公式 `sign(tilt)*(S_donor-S_receiver)` 对齐该坡度时为负值，而对平坦初态仍为零，故不能同时解释重力投影与局部表面坡度。供下一轮设计推导的候选表面项为：

```text
dx = 1 model column
slopeResidual = tan(effectiveTilt) * dx - (S[receiver] - S[donor])
surfaceHead = sign(effectiveTilt) * slopeResidual
```

在水平初态 `surfaceHead=abs(tan(effectiveTilt))*dx>0`；表面达到当前目标坡度时该项为 0；正负倾角镜像同号。此式只修正重力系坡度残差的符号和冷启动投影，**不是已批准的最终势能公式**。若要与 `heavyExcess` 相加，必须明确两项都以同一 model-height 单位表示并解释隐含权重；heavy mass difference 目前只是浓度偏差代理，没有密度差/侧压推导。不得将 `surfaceHead+heavyExcess` 直接称为已标定的物理势能。

#### 均匀 edge grant 的抵消与 2px 上界口径

在平坦均匀初态，即使给每条边同一个正 `potential`，collect plan 会对最多 127 条边生成相同请求；同一方向按边应用时，每个内部列一入一出，`Δbh[c]=q[c-1]-q[c]=0`，变化只落在端列或请求/库存变化的边界。因而“每相 heavy 总 grant”不能当作“局部 green 边界位移”。v2.10 实测相位 bottom 总 grant 约 `10.02`，在理想平均分到 127 边时，单边 heavy `q≈10.02/(2*127)=0.0395` model unit；按旧 ASSUMPTION-3 的 `2px/model-unit` 仅约 `0.079px`，且 ASSUMPTION-3 尚未 raster 标定。该估算只说明均匀请求不能证明 2px，不能当作所有自适应分配策略的绝对上界。

要保留当前路线，下一版必须给出一个来自现有物理状态、而非任意 ROI mask 或指定目标曲线的**空间变化 grant 规律**，并证明其离散散度在中央局部 heavy 边界上可见、满足每子步容量/守恒/`maxSubstepBh`、bottom `10*dt` 与 total `300*dt`，且不会依赖跨列非相邻搬运。若不能解释边通量为何随位置变化，当前 proposal 只把均匀双向交换换了名字，中央观测仍可能抵消。

#### 与既有 bottom 通道的预算竞争

v2.10 正相位 262/262 子步受限，`bottomUsed=10.0245`；负相位 262/262 子步受限，`bottomUsed=9.7183`。这接近该 run 实际可用的独立 bottom 总额度。architecture §18 将 gravity apply 放在 legacy `creepBottom` 之后，并规定 legacy 仍按自身请求使用 independent scale、gravity 只裁剪 `bottomRemaining`；在上述负载下，后置 gravity 可能几乎没有额度。不能一面声称复用原预算、一面把 legacy 已取得的 `10*dt` grant 当作 gravity 的可用额度。

若下一版保留该路线，须在 collect 阶段联合计入 legacy 与 gravity 请求，并在不超过同一 `10*dt` 的前提下明确定义有序分配/比例；随之说明 legacy grant 被挤占多少、共享 `300*dt` 与 settle/slip/lift/percolate 的竞争，并验证旧方向/速率语义仍满足原合同。此项是预算分配，不授权提高 cap 或改门槛。若不愿改变既有申请间的分配顺序，则需证明完整 v2.11 run 中 legacy 之后确有 bottom remaining；现有 v2.10 数据不支持此假设。

#### 对外部专家四点的处置

1. **冷启动**：接受“初始 potential 为零”的代数结论；“永久死锁”还需纳入现有通道后的递推才能成立。
2. **可见性**：不接受用总 `heavyFlux` 代替局部 raster 位移；上述均匀分流估算显示 2px 不能由总量本身推出。需按已定的局部 heavy 边界口径做真实同分辨率 raster 标定。
3. **速率瓶颈**：`vBottomCreepMax=10` 仍是每秒总 bottom motion 上限，且既有 legacy bottom 已近乎耗尽独立预算；新算法只有在联合分配内取得真实 grant、并证明非均匀通量减少内部抵消，才说明比旧路线更有效。预算不变，不把请求率提高当成解法。
4. **物理来源/量纲**：把 `surfaceDrive` 定义为相对重力坡面的 head residual；将 `heavyExcess` 明确为启发式组成项，提供可辩护的相对权重或独立归一化。不得把方向相反或尺度未说明的两个量直接相加。

#### 进入 architect 前的退出条件

方案修订至少需给出：默认初态第 1 子步/1s/5.1s 的逐边 `S,bh,bl,potential,q` 账本；正负相位符号一致的重力项定义；从 source state 推导出的非均匀 grant 及中央 ROI 离散 `bh` 散度；legacy/gravity 联合分配下按实际 physics `dt` 计算的 bottom/total 预算账本；`A_B` raster 位移预测与不确定度；保留原硬门槛、TEST/EXPERIENCE 生命周期与失败时 BLOCKED 的判定。现有资料不足以证明以上条件，也没有全策略无解证明。未满足前维持 `DESIGN_REVIEW_REQUIRED`，不请求 architect/developer；tester 停止，测试契约待 manager 按既有授权流程路由。

## 8.10 v2.11-R1 倾角残差候选的数值复核（未通过）

本节把“把倾角真正算进推力”落实为一个可复算的候选公式，并按默认 thick 初态逐子步演算。它用于判定 v2.11 是否闭合，不是实现批准。

### 势能项与交换量

令 `s=sign(θ)`，每列水平间距 `dx=1`，左/右列沉积总厚度为 `S_L/S_R`。按现有 `relaxAngle` 的目标坡度约定 `S_R-S_L=tan(θ)dx`，沿下坡方向的表面重力坡面余量定义为：

```text
D_s = s * (tan(θ)*dx - (S_R-S_L))
D_h = bh_donor - bh_receiver
P   = D_s + D_h
q_e = min(36 * max(0,P) * dt / 2,
          vBottomCreepMax * dt / 2,
          donor bh, receiver bl)
```

`D_s` 在平坦面上等于 `|tan θ|dx`，在目标坡面上为 0；正负倾角镜像。`D_h` 和 `D_s` 都用模型高度单位，系数 1 是未标定的候选闭合；它避免旧式 `surfaceDrive` 的符号错误，但没有额外密度数据来证明 heavy 项的物理权重。应用 `q_e` 时，沿下坡重沙 donor→receiver 搬 `q_e`，同时轻沙 receiver→donor 搬 `q_e`，故每列 `bh+bl` 与 `S` 不变。

公式修正了倾角推力，但不自动产生中央局部变形：均匀初态的每条边初始有同一个 `P` 和 `q`，边界效应只能从两端向内传播。

### 可复算的冷启动账本

初态为 128 列 `bh=bl=72`、`S=144`。对 `θ=+15°`：

```text
tan(15°) = 0.2679491924
P_e(0) = 0.2679491924 + (72-72) = 0.2679491924
q_raw/dt = 36*P/2 = 4.82308546
R_bottom/dt = 2*127*(q_raw/dt) = 1225.46373
```

如果为 legacy bottom 与 gravity bottom 各预留一半独立额度，gravity 的 joint grant 是 `G_g=5*dt`，比例缩放 `α=G_g/R_bottom=0.004080`，于是每边 `q/dt=0.01968`。使用正式日志首个有效子步 `dt=0.00835s`（仅作为逐步示例；实际步长逐帧变化），账本为：

| 时刻 | 状态/请求 | 重沙局部变化 | 2.0547px/列、1.6198px/模型单位 raster 位移 |
|---|---|---:|---:|
| 第 1 子步，0.00835s | `P=0.267949`；gravity bottom grant `0.04175`；每边 `q=0.00016437` | 两端 `|Δbh|=0.00016437`；内部列 `Δbh=0` | 端部 `0.000266px`；中央 `0px` |
| 累计 1s（120 个 0.00835s 子步，1.002s） | joint grant `5.01`；每步约 `q_edge=0.00015335`，随两端形成的 `D_h` 略降 | 中央 104 列 max/RMS `|Δbh|=0`；全场最大 `|Δbh|=0.01905` | 全场 max `0.0309px`；中央 `0px` |
| 累计 5.1s（611 个子步，5.10185s） | joint grant `25.51`；每步总请求约 `10.18`、scale 约 `0.00410` | 中央 104 列 max/RMS `|Δbh|=0`；全场最大 `|Δbh|=0.08561` | 全场 max `0.1387px`；中央 `0px` |

以上 1s/5.1s 是固定 `dt`、先收集全部 127 条边、按同一 scale 同步应用的可复现离散推演；由于角度残差在所有边相同，重沙变化只在端列，heavy 差产生的边界影响尚未到中央 ROI。真实帧步长与其他通道会改变数值，故这些不是产品设备的承诺值；它们足以否决“该公式本身保证中央可见”的主张。`-15°` 交换方向镜像，绝对值账本相同。

### 2px 判据与不可能项

默认 raster manifest 给出 `pixelsPerModelUnitY=311/192=1.6197917`。本算法的 green heavy 界面是 `bhY=boxY+(capacity-bh)*boxH/capacity`，所以指定列局部边界移动 2px 至少要求：

```text
|Δbh_local| >= 2 / 1.6197917 = 1.23472 model units
```

5.1s 中央 ROI 的推演值为 0，因此差 `1.23472` 个模型单位（2px）；即使把全部 `10*dt` bottom 额度给 gravity（不留给 legacy），有限传播仍未在中央 104 列出现，结论不变。uniform edge grant 的总通量不能换算成中央界面位移；本候选的模型/预算**不支持**≥2px 的声称。

### bottom 与 total 额度的联合分配

不得在 legacy 已用满 `10*dt` 后再给 gravity 叠加额度。下一轮若仍评估此路线，额度合同应写为同一子步、同一独立 bottom 上限内的联合申请：

```text
B = 10*dt
R_l = legacy bottom 已有逐边请求总和
R_g = gravity 双相请求总和（重、轻各 q，计为 2q）
A_l = min(R_l, B * R_l/(R_l+R_g))
A_g = min(R_g, B * R_g/(R_l+R_g))
```

按请求比例分配，没请求的一方份额自然归另一方；实际 apply 还须按 donor stock、单列 `28*dt` 和当步余量截断，截掉的额度不重发。所有通道再进入既有 `300*dt` total 额度统一裁剪；total 不足时使用同一比例 scale，不能超额补发。每笔 gravity `2q` 同时扣独立 bottom 和 total，legacy bottom `q` 扣其实际旧语义对应的账本。该规则不提高 cap，但会降低 legacy 获得量；在上表 equal-request 示例中两者各得 `5*dt`。即使授予 gravity 全部 `10*dt`，当前空间分布仍无法形成中央 2px。

### 判定与后续

v2.11-R1 已给出修正倾角项、首步/1s/5.1s 账本、2px 换算和预算分配规则，但**设计失败**：它不满足中央局部 heavy 边界 `>=2px`。不得把此节当作可交 architect 的方案。要解除阻断，必须再给出一个不指定目标曲线/ROI 人工偏置、却能从真实状态产生中央非均匀 grant 的机制，并在完整场景下满足 angle、两个预算、局部 raster 2px 和实际形状 release；否则应由 manager 裁决是否更换路线/目标。代码和测试用例仍未修改。

## 8.11 双角度滞回崩塌候选（natural-relax-hysteresis-v1）

**状态：DESIGN_PENDING_OBSERVATION**。本节是设计方案，不代表角度已标定，也不授权实现。目标是替换单一 `slopeLimit` 的反复开关行为；保留既有局部崩塌方向、预算、材料守恒与全局安息角验收，不引入目标形状、ROI 偏置或新几何。

### 角度定义与观测边界

对相邻列 `c,c+1`，保持现行重力参考坡度，定义有符号残差与无符号偏差：

```text
theta = effectiveTilt * pi / 180
targetSlope = tan(theta)
d[c] = sandTop[c+1] - sandTop[c] - targetSlope * dx
r[c] = abs(d[c]) / dx
```

阈值用斜率存储，而不是反复比较角度：

```text
slopeStart = tan(thetaStart)
slopeStop  = tan(thetaStop)
0 <= thetaStop < thetaStart <= thetaHardMax
```

设计初值只给**待校准搜索中心** `thetaStart≈30°`、`thetaStop≈20°`，不是最终参数。这里角度指相对于 `tan(effectiveTilt)` 的坡度残差角 `atan(r)`。`thetaHardMax` 是现行硬门槛（P-011 正式验收仍为 `30°`），与 `thetaStart/Stop` 分列；运行配置 `angleDeg`（thick 当前为 `4°`）也不被这两个候选值覆盖。若项目确认 30° 本身就是硬上限，起动门槛必须满足 `thetaStart<=30°`，观测到超过上限才启动的样本意味着硬门槛已先失败，不能靠滞回掩盖。

第 1 步 `natural-observation-v2.10` 的 ±15° 相位记录了 `relaxRequested=3125.60/3287.77`、`relaxApplied=718.10/731.64`，并判定重沙驱动过弱；报告没有逐边 `d[c]` 时间序列、首次触发/最终停止对应的残差样本或不同残差下的请求-应用曲线。因此这些 aggregate 不能反推 30°/20°。后续观测应额外输出每次状态转换前后的 `(runId, substepId, c, effectiveTilt, d, r, activeBefore, activeAfter, requested, granted, budgetScale)`；从自然坡面形成和回落样本估计起动/停止转折的置信区间，再将两阈值量化到测量分辨率。对样本不足或转折区间重叠的情况，保留 `DESIGN_PENDING_OBSERVATION`，不填猜测值。

**v2.10 逐边转换事件契约（供 v2.12 标定）**：ledger 升级至 schema v3；每个有效子步增加 `relaxStateTransitions[]`，只在边 `c` 的 active 状态发生 start/stop 翻转时追加事件；核心字段必须包含上列 tuple，并扩展存储 `transition`、显式 candidate `thresholdCalibrationId/thetaStartDeg/thetaStopDeg`、同一边前一有效子步的 `previousSubstepId/previousD/previousR/previousActive`、`previousRequested/previousGranted/previousBudgetScale`、本步 apply 后 `dAfter/rAfter` 与 `transitionPoint`。`d/r` 必须由该记录的实际 `effectiveTilt` 和同一材料快照计算，`granted` 是真实 apply grant，`budgetScale=granted/requested`（request=0 时为 null）。预算受限时 active 状态保持，限流样本仍留在常规逐边 slip trace；不可把无 grant 视作 stop。`previous*` 只可连接同 run、同几何且相邻有效 substep，间断时 transition 标为不可 bracket。

**零侵入与可标定性边界**：Probe 只记录状态机已有的真实 `sliding[c]`，不在 legacy v2.10 自行构造或推进 shadow state。旧 legacy run 没有 sliding 状态时写 `relaxStateTransitions=[]` 且 `transitionTelemetryAvailable=false`；不可将缺字段解释为无转换。标定采用显式 candidate grid，每个 run manifest 绑定 `thetaStart/thetaStop`、算法/source identity 和 `thresholdCalibrationId`；通过真实物理轨迹与 raster 结果筛选 candidate，独立 holdout run 不参与拟合。start/stop 事件用紧邻转换两侧的 r 夹出区间，按 run bootstrap 95% CI，并按残差符号、倾角方向、预算 scale 分层。无足够 start/stop 样本、发生 hard violation、连续性丢失或 holdout 区间失败时仍为 `DESIGN_PENDING_OBSERVATION`；不能发布默认30°/20°点值。

阈值附近转折需要超角激励，不能假设原 v2.10 的 ±15° 序列一定产生约30° residual。为此保留原 route 与历史 artifacts 不变，增加独立 calibration schedule `p011-v212-hysteresis-calibration-v1 = 0/+15/0/+45/0/-45/0`，±45 各5.1s；每个 candidate threshold pair 用新 run，至少5个拟合 run 与5个独立 holdout run。若 5.1s 内没有 stop transition，报 `stopTransitionUnobserved`，不得把 phase end 当作 stop 样本。该数据协议要求将真实 sliding 状态及候选阈值显式绑定；仅对既有 `relaxTriggered` 做事后重命名不能标定双阈值。

### 定标方法与接口

本方案输入为 `effectiveTilt`、逐列 `sandTop`、逐边 `sliding[]`、`dt`、既有局部/total 预算及 donor/receiver 库存和容量；输出为更新后的 `sliding[]`、逐边 request/grant、材料场和约束审计记录。它不接收目标形状或视觉 ROI。

观测定标在独立自然运行轨迹上进行，不注入指定坡形。对每次静止→滑动转换，记录转换前后 `r_before/r_after`；离散采样把真实 start threshold 限定在两者区间内。对每次滑动→静止转换，同样用相邻样本夹住 stop threshold。汇总多 run、正负残差与±倾角的区间，报告中位估计和 95% bootstrap 区间；校准数据只用于估参，另留至少一组完整 run 作 holdout。先验搜索窗口固定为 `thetaStart∈[25°,30°)`、`thetaStop∈[15°,25°]`，仅表示“约 30°/20°”的候选区间。最终取值须满足 `thetaStop < thetaStart <= thetaHardMax`，且两阈值间隔大于观测角噪声 95% 区间宽度；holdout 的状态转移误判率和斜率上限违规数须为 0。若窗口内没有可辨转折，或置信区间重叠，不拟合单点、不填默认值。
### 三态状态机

每条相邻边维护一个 `sliding[c]` 布尔状态。三态含义是“静止”“滑行保持”“重新静止”：

```text
if !sliding[c] and r[c] >= slopeStart:
    sliding[c] = true             # 起动
else if sliding[c] and r[c] <= slopeStop:
    sliding[c] = false            # 停止
else:
    sliding[c] stays unchanged    # 两阈值之间保持
```

只有 `sliding[c]` 为真时才产生崩塌请求。方向仍由现行残差符号决定：`d>0` 时右列高于倾角目标坡面，右→左搬运；`d<0` 时左→右。候选量沿用既有规则、参数和各层库存/容量限制：

```text
excess = max(0, r[c] - slopeStop)
requested[c] = min(excess*dx/2,
                   maxAvalancheFlux*dt,
                   donorSand,
                   receiverCapacity)
```

请求在 collect 阶段形成、apply 阶段按既有 share/预算缩放并应用；实际授予不足时 `sliding[c]` 保持为真，不能因为请求被限流或 `X=F` 就清除。完成本子步后以真实 `sandTop` 重算 `r`，下一 collect 才按 stop 条件退出。该状态机不指定列号、空间曲线或终态形状。

### 硬约束、预算与生命周期

- **总预算不变**：崩塌仍使用现有 `relaxAngle`/slip 的单列与 `totalMotionBudget=300*dt` 账本；每列 `vSandTopMax*dt`、`maxAvalancheFlux*dt` 和接收容量约束不变。它不额外获得 bottom `10*dt`，因为现有 relax 通道不属于 bottom creep。其他通道仍按既定共享 total cap 竞争，滞回不得增设额度、欠账或补发。
- **守恒不变**：每次申请只做相邻 donor→receiver 的等量转移，应用原 `splitTopV2` 材料拆分；每笔按现有事务更新并审计 water/heavy/light 总量、库存、容量，不得用 `sliding` 状态替代物料账本。
- **安息角上限不变**：每个 apply 后仍核对全场 `r[c] <= tan(thetaHardMax)`。任何改变 `sandTop` 的通道都不得提交会把原本合法状态推过该上限的增量；先按同一通道既有授予截断/拒绝，不能调用额外预算“修复”。若输入倾角改变使 `targetSlope` 本身令残差越限，记录硬门槛失败并由当前崩塌请求在原预算内修复；不得将暂时越限误报为通过。若在旧预算内不可恢复，则本方案不能声称同时保证硬上限，应保持 BLOCKED。
- **状态生命周期**：新 run/reset、列数/几何重建以及 TEST↔EXPERIENCE 切换时清空 `sliding[]`；同一连续运行中的 0°暂停、倾角方向翻转和输入短暂抖动保留状态，每子步按当前 `effectiveTilt` 与实际坡面重算方向/残差；材料状态保留规则不变。状态数组大小 `cols-1`，不保存目标或 phase baseline。

### 收敛与验收

在单一边、固定倾角、未限流且两列可用库存/容量充足的局部近似下，授予 `q=(r-slopeStop)*dx/2` 会把该边残差推向 `slopeStop`；预算缩放或相邻边竞争时只保证请求方向修正当前超差，不保证本子步落到 stop。跨边顺序必须用 collect/apply 既有快照语义，禁止用边遍历顺序制造隐式优先形状。

方案验收断言：

1. 参数不变量 `0<=thetaStop<thetaStart<=thetaHardMax`；严格用 `tan()` 比较，单位统一为 model-height/model-column。
2. 静止态在 `r<slopeStart` 不发请求；首次 `r>=slopeStart` 进入滑行；滑行中 `slopeStop<r<slopeStart` 状态保持并继续请求；只有 `r<=slopeStop` 才退出。恰好等于阈值时按伪代码的 `>=`/`<=` 规则处理。
3. 正负坡度、±倾角、阈值附近噪声、预算缩放和持续限流下，方向都按残差符号；限流期间不丢失 active 状态，不得每帧起停抖动。
4. 每子步守恒、库存、容量和全部既有 local/bottom/total 预算断言保持原值；对本通道的 bottom 用量应为 0，total 不超过 `300*dt`。全场硬坡度残差不超过 `tan(thetaHardMax)`，否则 FAIL/BLOCKED。
5. 不含任何目标 `targetDelta`、预设列号、ROI 权重或人工塑形；长时静置后无持续请求/零漂移，输入回零和反向时报告实际停滑时延与残差。
6. 观测确定角度后，分别报告起动/停止估计、置信区间、样本数和留出序列复核；若转折不可分辨，则不能将近似 30°/20°写成通过值。

每子步复杂度 `O(cols)`，额外状态 `O(cols)`，无全局搜索。测试断言只写入本方案文档，实际测试用例仍由 manager/tester 按流程授权维护。

### 设计结论

当前可落盘的方案是“按每边坡度残差触发的双阈值状态机”，两角保留待标定符号值；第 1 步观测数据不足以填入角度。实现前还需由观测补齐逐边转折分布，并由 architect 核对全体改变 `sandTop` 的通道确实服从硬上限 guard。满足这两项前状态为 `DESIGN_PENDING_OBSERVATION`，不进入 developer，也不启动 tester。

### 标定协议：双角度阈值（执行版）

**状态：协议已预注册，待观测数据；本节定义方法，不代表已经获得 `thetaStart/thetaStop`。** 数据到手后按以下顺序执行。分析前冻结 ledger schema/source identity、候选阈值网格、schedule、噪声定义、随机种子和排除规则；看过 holdout 结果后不得改阈值、分层边界、CI 或排除规则。任何偏离都须标为新 protocol/version，重新采集独立 holdout。

#### 1. 转折样本与单事件估计量

1. 从 `relaxStateTransitions` 只接受真实 `start`/`stop` 事件；必须有同 run、同 edge、相邻有效子步的前序样本，且 `effectiveTilt`、`d/r`、状态与实际 grant 对齐。缺失前序样本、substep gap、reset/几何变化、非有限量或已发生 hard-gate violation 的 run 不得用于阈值拟合；保留在失败审计中。
2. 对每个样本计算残差角 `alpha=atan(r)*180/pi`。`start` bracket 取翻转前后两侧 `alpha` 的闭区间；`stop` bracket 同理。单事件点估计固定为 bracket 两端中点 `m=(alphaBefore+alphaAfter)/2`，同时保留 bracket 宽度；不得把候选配置角直接当作观测值。
3. `thetaStart`、`thetaStop` 的拟合点估计分别取对应合格事件中点的**run 等权中位数**：先在每个 run 内求中位数，再对 run 中位数求中位数，防止高频 run 主导估计。逐 run、逐方向及全部 bracket 原始值必须留档。

#### 2. Bootstrap 与置信区间

使用 **10,000 次 run-cluster percentile bootstrap**，固定伪随机种子 `20261019`。每次在拟合集内按划分用的 `(thresholdCalibrationId, candidate pair, schedule, device/build, tilt coverage)` 分层，以 run 为单位有放回抽取该层原有数量的 run；一个 run 被抽中时保留其全部 edges、时间序列和 transition events，不独立重采子步或边。每次重采样按上节同一 run 等权中位数算法计算 `thetaStart` 与 `thetaStop`。95% CI 取 10,000 个 bootstrap 估计的第 2.5 与第 97.5 百分位（线性插值法）。

选 percentile 而非 BCa：本数据是 run 内强相关、run 间独立的 cluster 样本，run-cluster resampling 直接保留相关结构；percentile 区间定义简单、可复现，BCa 的 jackknife acceleration 在少量或不均衡 run clusters 下会不稳定。10,000 次重采样用于稳定尾部分位数；不得把边或子步当独立样本来人为缩窄区间。报告同时列出有效 run 数、事件数、CI 和 bootstrap seed。

#### 3. 拟合 / holdout 划分

按 **run 整体分层随机划分，目标比例 70% fit / 30% holdout**；绝不拆分同一 run 的子步、边或 phase。分层键为 `(thresholdCalibrationId, candidate threshold pair, schedule version, device/build identity, tilt-direction coverage)`，确保每个候选 pair、设备/build 和正负倾角都进入两组。先按 run manifest 的不可变 `runId` 排序，再以预注册 seed `20261019` 在每层内随机排列并分配；randomization manifest 在揭盲前保存。禁止按时间先后切割，避免设备/温度/负载漂移与组别完全重合；也禁止人工按结果挑 run。

每个候选阈值 pair 至少 **34 个独立完整 run**：24 个 fit、10 个 holdout（约 70.6/29.4，满足至少 70/30 的近似比例和 holdout 下限）。每组须覆盖 `+tilt/-tilt`，且每个方向至少 5 个独立 run；每个 run 均执行预注册的完整校准 schedule。每候选 pair 的 fit 与 holdout 均至少有 20 个有效 start 和 20 个有效 stop transition，此外还必须满足下节每层样本下限。若任一候选层或方向达不到最低量，只可继续按冻结 schedule 采集新 run 并按原随机规则分组；不得把 holdout 挪入 fit 后继续声称独立复核。

#### 4. 分层维度、噪声带与最少样本

先用独立 `no-transition` 稳定段估计角度观测噪声：对每个 run/edge 的稳定窗口求 `alpha` 中位数，汇总其绝对偏差，取 pooled MAD 的 `1.4826` 倍作为 `sigmaNoiseDeg`；另以 fit transition brackets 的中位宽度一半定义离散采样分辨率 `qResolutionDeg`；固定 `wNoise=max(1.96*sigmaNoiseDeg, qResolutionDeg)`。这同时覆盖稳定态噪声与子步离散夹值宽度，不另设可随结果调整的角度常数。`sigmaNoiseDeg/qResolutionDeg/wNoise` 只从 fit 集合计算并冻结，holdout 不参与重估；任一量非有限或 `wNoise=0` 时标记 `insufficientEvidence`。对每个逐子步状态样本，inactive/start 样本用 `thetaStartCandidate`，active/stop 样本用 `thetaStopCandidate`；以当前 `alpha` 到对应候选阈值的距离 `abs(alpha-thetaCandidate)` 分类。实际转换 bracket 仍只用于转折估计，不用 bracket midpoint 替代状态样本：

| 噪声层 | 分层条件 |
|---|---|
| `near` | `abs(alpha-thetaCandidate) <= wNoise` |
| `shoulder` | `wNoise < abs(alpha-thetaCandidate) <= 2*wNoise` |
| `far-control` | `abs(alpha-thetaCandidate) > 2*wNoise` |

交叉分层维度固定为：待判定状态 `inactive/start` 或 `active/stop` × 有符号残差方向 `d<0/d>0` × 有效倾角方向 `effectiveTilt>0/<0` × 噪声层 `near/shoulder/far-control`，共 24 个 cell。此处 cell 计数使用逐子步逐边的状态判定样本（包括 hold 与 flip），而不是只数实际转换事件；实际 start/stop bracket 另行计数。`d=0` 或 `effectiveTilt=0` 不混入正负层，单独报告为 neutral，不计入阈值识别样本。fit 与 holdout **每个 cell 各至少 30 个有效状态判定样本，且来自至少 10 个不同 run**；同一 run 对同一 cell 的贡献最多计 3 个样本，避免高频抖动灌满样本数。每组每个方向的实际 start 和 stop 转换各至少 20 个，且分别覆盖至少 5 个 run。holdout cell/转换不足时只能补采 holdout，不能降低样本门槛。所有用于阈值估计的 bracket 必须连续且无 hard-gate violation。

误判率按状态机真实转移语义计算：对 `inactive/start` 样本，`alpha>=thetaStartCandidate` 时下一状态应 active；对 `active/stop` 样本，`alpha<=thetaStopCandidate` 时下一状态应 inactive，否则应保持 active。其他状态区间按双阈值滞回保持规则判定。记录状态与该期望不一致即计一次误判；不把预算受限的 grant=0 当作状态误判或 stop。

#### 5. 不可分辨、通过与停止判据

对 `thetaStart` 和 `thetaStop` 分别应用以下预注册判据；任一项失败均将该阈值标为 `unresolved` 并保持 `DESIGN_PENDING_OBSERVATION`，不写入默认 30°/20°：

1. Fit 组 percentile 95% CI 总宽度必须 `<=2.0°`；超过即判“转折不可分辨”。2° 是既有角度观测/目标带 `30°±2°` 的最大可用量化粒度，不再另行按结果调宽。
2. fit 和 holdout 的点估计差 `<=1.0°`；holdout transition midpoint 落在 fit 95% CI 外的比例 `<=5%`，按 start/stop 与倾角方向分别报告；24 个状态判定 cell 的状态语义误判率各 `<=5%`。任一 cell 越界比例 `>5%` 判复现失败。
3. 两角的 fit CI 必须严格分离：`lower95(thetaStart) > upper95(thetaStop)`，且两点估计差至少 `2*wNoise`；否则滞回带不能与噪声区分。
4. `thetaStart` 的 95% CI 上界须 `<30°`；如 §8.14 的 30°±2°映射验收仍适用，`thetaStop` 的 95% CI 还须完全落入 `[20.563°,25.093°]`。不满足时结论为当前双阈值与硬边界/目标坡度约束不兼容，而不是改用几何中心填值。
5. holdout hard-gate violation、预算/守恒/库存/容量违规必须全为 0；状态语义误判不得超过 5%，任何越界都直接判候选 invalid，不可通过 bootstrap 平均抵消。

若有效 run、事件或任一 cell 不足，报告 `insufficientEvidence`；若样本量达到但任一 CI 宽度/间隔/holdout 判据失败，报告 `transitionUnresolved` 或 `candidateInvalid` 并保持 `DESIGN_PENDING_OBSERVATION`。只有所有判据均通过，才可报告两阈值点估计、run-cluster percentile 95% CI、每层样本数、fit/holdout 误判率和硬门槛结果；该结果仍须通过 architect 对逐 writer guard 与实际体验验收复核，才可改变设计状态。

### `thetaStart` 先验窗口决策树

本树适用于 `thetaStart` 的 run-cluster 点估计及其 percentile 95% CI；窗口为半开区间 `[25°,30°)`。先按上节计算当前可用的 fit 点估计/CI 并独立标记样本充分性；只要已有可计算的点估计或 CI，就先触发 A 节的近硬门槛预警，再按下列顺序决定 unresolved、invalid 或窗口去留。样本不足永远不能作为接受/拒绝窗口的证据。阈值角均指 `atan(r)` 的残差角。

```text
if 点估计 >= 30°:
    candidateInvalid（点估计违反半开窗口）；提交 manager，不得接受
else if 点估计 >= 29° 或 start 95% CI 上界 >= 29°:
    触发 manager 预警；标记 MANAGER_REVIEW_REQUIRED，暂停参数定稿
else if start 95% CI 宽度 > 2° 或最低样本/分层不足:
    transitionUnresolved；补采缺失 run/cell，不判窗口错误
else if start 95% CI 上界 >= 30°:
    candidateInvalid（证据充分但不能证明起动早于硬边界）；提交 manager，不得接受
else if start 95% CI 上界 < 25° 且满足“窗口错误”证据包:
    标记 PRIOR_WINDOW_REJECTED；交 manager 重议搜索窗，禁止自动外推/扩窗
else if 点估计 < 25° 或 CI 与 25° 相交:
    按下方边界证据规则区分加采与重议
else:
    留在 [25°,30°)；继续按既有双角验收判据
```

#### A. 顶近硬门槛：`>=29°` 预警

触发条件为拟合点估计 `thetaStartHat>=29°` **或** 95% CI 上界 `U95>=29°`；任一成立即预警，即使 CI 尚宽或 holdout 尚未完成也不得静默跳过。预警不等于自动否决，但 `MANAGER_REVIEW_REQUIRED` 解除前不得定稿该候选、进入 architect 批准或实现。点估计 `>=30°` 直接为 `candidateInvalid`；若 `U95>=30°` 且样本/CI 精度尚不足，状态保持 `transitionUnresolved` 并补采；只有样本充分且 CI 宽度 `<=2°` 后仍 `U95>=30°`，才判定候选未能证明早于硬边界并标 `candidateInvalid`。

提交给 manager 的警报必须包含：

1. `thetaStartHat`、`CI95=[L95,U95]`、`30°-thetaStartHat` 点裕度和 `30°-U95` 保守裕度；同时列 `thetaStopHat/CI95` 与两阈值置信间隔。保守裕度 `<=1°` 时明确标为 `CRITICAL_MARGIN`。
2. 同设备/build、schedule、目标输入、预算和 raster 采样下，与最近一个**有效且 `<29°`** 的候选配对，报告每 1,000 子步的 `materialGuardClipOrReject` 次数、被截断量/预 guard grant 比率、`tiltFeasibleClampOrHold` 子步数及最长连续 hold；给出绝对差和相对变化。缺少匹配对照时写 `comparisonUnavailable`，不得用非匹配 run 推断 guard 负担变化。
3. hard-gate violation、预算/守恒违规、holdout 状态误判率、各残差符号/倾角方向样本数，以及实际 guard 硬拒绝数。任何 hard violation 仍直接 invalid，不得以裕度估计抵消。

#### B. 落到窗口以下：加采还是重议

**判为“加采，不能判窗口错误”**，若符合任一项：

- start transition 不足、任一 24 个 cell 或方向 run 数不足、CI 宽度 `>2°`，或 fit/holdout 差异不合格；按 protocol 原分层随机规则只补缺失组，不重分既有 holdout。
- `thetaStartHat<25°` 但 `U95>=25°`，或转折 bracket 与 25° 相交/跨越 25°；这是边界处尚未分辨，不证明阈值真在窗外。优先补采更高时间分辨率的连续子步和对应方向/噪声层，保持参数、dt、预算与门槛不变。
- 触发证据仅来自单一残差符号、单一倾角方向、一个 run、预算限流造成的 active 保持，或 hard-gate 已失败的 run；这些不能证明真实 start 转折位置。

**判为“先验窗口本身不合适，交 manager 重议”**，必须同时满足全部条件：

1. `thetaStartHat<25°` 且 fit 95% CI 上界 `U95<25°`；CI 宽度 `<=2°`。
2. 至少 20 个有效 start brackets，覆盖正负残差、正负倾角方向；fit 与 holdout 各满足上节 24 个 cell 最低样本数、run 数和误判率门槛，holdout 点估计与 fit 相差 `<=1°`，并独立复现 `U95<25°`。
3. 至少 80% 的合格 start brackets **完整落在 25°以下**（两端均 `<25°`），而非仅 midpoint 落窗外；各方向均有完整窗外 brackets，排除跨界采样宽度造成的假性低估。
4. 这些转折来自真实 `activeBefore=false → activeAfter=true` 事件，`requested/granted/budgetScale` 有效；不是 grant=0、状态缺失、时间断档或 hard-gate violation 推断出的伪转折。

满足全部条件后，将现有 `[25°,30°)` 标记 `PRIOR_WINDOW_REJECTED`，提交 manager 的材料至少包括 fit/holdout bracket 原始表、CI/bootstrap manifest、分层样本量、窗外 brackets 比例、状态误判率与 hard-gate 审计。**designer 不自行把下界改低、不在同一 holdout 上重定搜索窗或重估最终值**；manager 若批准新窗口，必须增加 protocol version、冻结新候选 grid，并重新采集未见过的 holdout。落在 25°附近但不能满足以上全部条件时，一律按加采/`transitionUnresolved` 处理，不能主观宣布窗口错误。


## 8.12 BLOCKED：slip-overshoot-v1 越界归因（独立硬缺陷）

**优先级：BLOCKED / 最高。** 证据：`test/artifacts/p011-slip-overshoot-v1/summary.json`、`ledger.ndjson`、`surface-slope.png`。run=`p011-run-1792397031250-3`，共 2,512 子步；484 次 range failure：positiveOvershoot 268、positiveOvershootReturn 5、negativeOvershoot 206、negativeOvershootReturn 5。材料守恒、stock、capacity 与预算 excess 均通过；失败项集中在 `maxSandTopInvariantError > tan(30°)=0.577350`。

### 先区分指标

`0.729596` 是局部边 `abs(S[c+1]-S[c]-tan(effectiveTilt))` 的坡面残差，不是 `surface-slope.png` 上的整体可见线性坡角。曲线中的整体线性坡角峰值只有数度；局部 residual 可以越限而整体回归线仍接近平缓。不得把两者混为同一量。

### 首次越界：不是材料搬运通道把线推过

summary 所报首个失败为 recordIndex 1456 / sequence 1457。该记录 context 的 `effectiveTilt=24°`，但 `beginNaturalObservation()` 在 `substep()` 之前调用，而 `substep()` 首行会 `updateTilt(dt)`；故 ledger 的 context/stateBefore 记录的是倾角更新前值，通道 grants 与 audit 使用的是更新后的值。该步实际用于坡度残差的角为 36°。这是 ledger context 的一子步对齐缺陷，不能用其 24° 标签解读 audit。

使用更新后的 36° 重算该步材料快照：最大误差边 `c=57` 的 `S[c+1]-S[c]=-0.00311824`，因此在任何本步材料 grant 前，`d=-0.00311824-tan(36°)=-0.72966077`，已超过 `tan30°`。完成本步 channels 后为 `-0.72959622`。换言之，**首次 crossing 的直接原因是 effectiveTilt/目标坡度一步推进得比当前沙面能跟上的快；不是 settle、lift、bottom 或 slip 先把合法坡度推越线**。正相镜像的首次负向 crossing 同理：context `-24°`，实际校验 `-36°`，边 `c=117` 在材料变更前 residual `+0.79853325`。

在正向首次越界边，relax request 是 corrective：该边请求 `q=0.32986698`、实际 grant `0.03147921`，方向把 residual 往零推。bottom grant 全边合计仅 `0.015905`，低于独立上限 `0.166667`；settle 合计 `0.003090`、lift `1.085782`，都发生在已由倾角更新越线之后。因此“非 slip 通道绕过 guard 导致首次 crossing”不受本账本支持。当前 `range` 是子步后的审计判定，不是阻止提交的 admissibility guard；系统没有在 material/tilt 更新前拒绝非法状态的机制。

### relaxAngle 为什么追不上

这不是 `excess/2` 对单边的数学步长不足。首次 crossing 边上 `angleDeg=4°`，孤立修正所需量为：

```text
q_required = (0.72966077 - tan(4°)) / 2
           = 0.32987
```

与该边的原始 request `0.32986698` 一致；`maxAvalancheFlux*dt` 也不是这笔 request 的限制项。但它只获批 `0.03147921`，约为 request 的 `9.54%`。该子步全通道 request `52.39441`，total cap 只有 `5.0` 且用满；slip 总 request `40.81759`、实际 `3.89522`。因此第一层瓶颈是共享 `300*dt` 预算下的全局比例缩放，settle/lift 等请求与 slip 竞争；独立 bottom cap 没有耗尽。

第二层原因是空间通量抵消：127 条边同时收集/apply，局部边 grant 几乎相等。对同向相邻输运，单边坡度变化不是 `2q[c]`，而是相邻通量的离散差分（内部边可写为 `2q[c]-q[c-1]-q[c+1]`，具体符号随方向约定）。当 `q` 沿空间近似常数时，材料列的入/出抵消，局部坡度几乎不变。首次 crossing 处虽然每边都申请约 `0.33`，实际 grant 约 `0.0315`，相邻边同量级，所以沙面 residual 只从 `-0.72966077` 改到 `-0.72959622`。因此“孤立边一步能回到 angleDeg”不能推出完整 128 列同步 apply 能在一个子步内收敛。

正负 overshoot 全相位 relax grant/request 分别仅 `886.17/15363.97=5.77%` 与 `677.49/11912.33=5.69%`；所有 268 个 positiveOvershoot range-failure 子步 total 使用率均至少 90%，206 个 negativeOvershoot 失败中 140 个至少 95%。这说明这是持续预算竞争和局部通量散度不足，不是单一瞬时限幅偶发。

### 后续恶化通道与取舍

越界峰值在 positiveOvershoot sequence 1724：边 `c=3` 的 residual 在 settle 前已为 `-1.64701172`；settle 把相邻 `S` 分别增加 `0.02727760` 与 `0.01435503`，使 residual 恶化至 `-1.65993429`；随后 slip 把它拉回 `-1.64721811`，lift 对该边近似等量、不改变坡度，bottom grant也近乎相消。故 settle 是**已有越界的局部放大项**，不是首次越界的触发项；slip 是纠偏项但因共享预算和邻边通量相消而效果不足。

候选取舍：

1. **所选归因**：先推进的有效倾角参考越过可行坡度带，产生首个 residual violation；预算裁剪和空间通量相消阻止 relax 追平。证据直接来自首个失败快照和逐通道 grants。
2. **“bottom/lift/settle 绕过 guard”作为首因**：不采纳。首次 crossing 在材料变化前已存在；这些通道可改变后续曲线，且峰值样本的 settle 有小幅放大，但不能解释最初 crossing。
3. **“excess/2 单边公式太小”作为首因**：不采纳。孤立边请求恰好足以把误差降到旧 `angleDeg`；完整系统失败在共用预算与 grant 空间散度，不是该局部公式的数值下界。

设计后续必须独立处理：倾角参考更新前的可行性检查，以及所有改变 `sandTop` 操作的 pre-commit slope guard；任何 correction/priority 仍只能在既有 total/local/bottom 额度中分配，不能叠加预算。并先修正观测边界，使 `effectiveTilt` 与 `stateBefore/grants/audit` 使用同一子步快照。若大倾角目标在既有预算内不可行，应明确拒绝/延迟该物理状态或报告 BLOCKED；双角度滞回只改变何时滑，不会消除参考坡度跳变、全局预算缩放或均匀通量抵消，不能作为本缺陷的单独修复。

## 8.13 设计中：倾角参考速度与材料响应联合限幅（slip-overshoot-v2）

**状态：DESIGN_PENDING_CALIBRATION；目标是建立无越界可证明条件与 P95 跟手性 Pareto 选型，不预设 `maxTiltOmega` 单点。** 沿用现有 `totalMotionBudget=300*dt`、每列 `vSandTopMax=28*dt`、`maxAvalancheFlux`、几何、守恒和 `tan30°` 硬线。不得用新增预算或延后审计掩盖失败。

### 可行域约束先于固定 omega

令相邻表面坡度 `g[c]=S[c+1]-S[c]`，有效倾角参考 `t(θ)=tan(θ)`，带符号 residual `r[c]=g[c]-t(θ)`，硬界 `H=tan30°`。每子步倾角更新 `δθ` 与材料净坡度变化 `δg[c]` 必须联合满足：

```text
rNext[c] = g[c] + δg[c] - tan(θ + δθ)
abs(rNext[c]) <= H                         for every edge c
```

因此，不能单独把 `|δθ|` 约束为某个常数并宣称永不越界；安全步长依赖当前每条边的剩余 slack、grant 的空间散度以及符号。给定本步各边的已准入净坡度变化 `δg[c]`，可计算允许的倾角参考区间：

```text
lo[c] = g[c] + δg[c] - H
hi[c] = g[c] + δg[c] + H
lo = max_c(lo[c]); hi = min_c(hi[c])
thetaNext ∈ [atan(lo), atan(hi)]
```

目标步进再与 `targetTilt` 方向及角速度上限取交集。若 `lo>hi`，或当前状态已不可行且本步现有预算内不存在合法 `δg`，该子步没有可证明安全的 tilt update：拒绝推进参考并报告 BLOCKED/不可行，不能先越界再修复。`δg` 必须来自按现有 local、bottom、total cap 实际可授予的事务计划；仅用 request 或孤立边上界不能当成 grant 证明。

作为快速可计算的保守上界，若材料在本步对任一边最多能反向修正坡度 `C_slope[c]`，则倾角增加方向的可用余量为 `H-|r[c]|+C_slope[c]`，在 `|δθ|` 范围内用非线性的 `|tan(θ+δθ)-tanθ|` 检查，不以小角线性近似替代最终验算。孤立边相邻列各自最多移动 `vSandTopMax*dt` 时，粗上界 `C_slope<=2*vSandTopMax*dt/dx`；对同步全场该上界必须进一步乘以实际 grant 的离散散度，均匀相邻通量的坡度修正可接近零。共享 total budget 会让可用 grant 依并发请求变化，所以正式安全裁决应使用上述全边交集，而不是固定换算常数。

### 以本次越界轨迹量化角速度与延迟冲突

首次正向越界前，context 的 24° 是旧快照；材料坡度约 `g=-0.003118`。参考更新到 36° 后、材料 grant 前 residual 已为 `-0.729661`。若该步角度仍从 24° 起算且材料暂不动，最先触碰 `-H` 的参考角为：

```text
theta_safe = atan(g + H) = atan(-0.003118 + 0.577350) ≈ 29.87°
deltaTheta_safe ≈ 5.85°
```

实际一步推进 12°，约为该初始快照保守允许量的 2.05 倍。该例说明固定 `720°/s` 在当前约 1/60s 子步可产生 12° 跳步；但不能据单一场景直接宣布 `360°/s` 全局安全，因为在更小 slack、反向切换或材料 grant 空间抵消时同一固定速度仍可越界。动态可行域限幅负责硬安全，`maxTiltOmega` 只限制可行域内的最大追随速度。

在最坏“grant 对 local slope 无有效修正”的保守分支下，测试角度范围 `|θ|<=45°` 有 `d(tanθ)/dt=sec²(θ)*(π/180)*omegaDegPerSec`。因此一次 12°/1/60s 更新约相当于 `720°/s`；只把配置改成 `360°/s` 会把该例降为约 6°/子步，仍只在初始 slack 足够时安全。对 ±15° 测试输入，若希望 effective tilt 本身达到 90% 目标角度不超过 100ms，需要 `omega>=0.9*15/0.1=135°/s`；从 0 到 ±45° 的同指标需要 `405°/s`。这只是参考角到位的下界推算，**不等价**于既有 P95 T90 门槛（其口径是越过 dead zone 到首个有效 bottom 位移）；也不保证材料重沙边界 2px 或 `t90` 达标。由此可见，单一固定 omega 很难同时覆盖不同目标幅度和 slack：应使用“动态安全限幅 + 有界最大追随速率”，再用规定的输入序列测真实 P95。

### 候选控制律与不变量

```text
thetaRequested = moveToward(effectiveTilt, targetTilt,
                            maxTiltOmegaCandidate * dt)
collect existing motion requests; compute existing shared/local grants
derive each edge's actual planned deltaG from those grants
thetaFeasible = intersection over edges:
                atan(g[c] + deltaG[c] - H) .. atan(g[c] + deltaG[c] + H)
thetaNext = clamp thetaRequested into thetaFeasible
if thetaNext is infeasible or required grants violate existing caps:
    hold effectiveTilt and report infeasible for this substep
apply admitted transactions; recompute every residual from committed S and thetaNext
```

实现次序须避免预算与 tilt 互相循环依赖：先基于快照求候选请求，在原预算中按确定、可复现的既有优先序预演 grant；由预演得到 `δg` 后求 `thetaFeasible`；如角度被收窄，重算依赖 `effectiveTilt` 的请求并再做一次预算分配。第二次结果必须满足交集；若不满足则本步不应用 tilt-dependent material grants，保持 `theta` 并重新按旧参考角分配一次，所有被拒请求记为 clipped，不结转。若此回退也无可行状态则 BLOCKED。不得为了追角创建额外 correction grant。此流程至多两次计划 pass，`O(cols)` 时间与 `O(cols)` 临时存储；具体通道顺序由既有 apply 事务确定并逐项推导。

必要验收不变量：每个提交子步都以提交后的 `S` 和同一 `thetaNext` 检查 127 条边；`effectiveTiltRate=(thetaNext-thetaBefore)/dt` 必须输出实际受限速率；倾角被可行域夹住时记录 requested/accepted angle、最紧 slack edge、材料 grant 的离散散度、预算 scale 与 hold 原因。账本 context/stateBefore/grants/audit 必须属于同一子步快照。任何一条边越过 `H` 都是硬失败，即使下一个子步立即恢复也不能算通过。

### 联合标定：安全约束和跟手性 Pareto 前沿

不以公式假设取代设备测量。冻结同一输入轨迹、设备、渲染负载、物理 dt 与既有 capture 方案，固定测试 ±15°、反向和回零；对 `maxTiltOmegaCandidate` 做有限对数档扫描 `90, 135, 180, 270, 360, 540, 720°/s`。这些是用于暴露 Pareto 前沿的实验档位，不是批准参数；动态安全投影在每档均开启。另跑无越界对照和当前 720 基线。每档重复足够完整 run 以形成稳定 P95（建议至少 20 个有效方向转换/run，至少 5 runs），报告 run 内、跨 run 的 P95 和 bootstrap 95% CI。

对每个候选同时报告：

1. 硬安全：所有有效子步全边 residual violation 数必须为 0；预算超额、守恒/库存/容量失败均为 0；角度 hold/clamp 子步数和最长连续 hold 时间。
2. 既有跟手指标：越过死区到首次有效 bottom 位移的 P95 T90 `<=100ms`，严格沿用现行口径；不以 effectiveTilt 到位时间替代。
3. 新增诊断但不改旧门槛：输入命令至 effectiveTilt 达目标变化 90% 的 P50/P95；命令至首次可见重沙边界响应；实际 heavy interface `A_B`（局部位移像素）；回零/反向延迟。
4. Pareto 决策：先剔除任何硬安全失败档，再剔除 P95 T90 超 100ms 档。在剩余集合中选择满足体验门槛且 worst-run `P95` CI 上界不超过 100ms 的最低安全限幅代价方案；若没有候选同时通过，结论是约束冲突、交 manager 决定目标/路线，不降低 `tan30°`、不加预算，也不拍一个折中 omega。

当前证据未包含不同 omega 下的响应分布，v2.13 的失败 run 仅证明 `720°/s` 在已见 slack 下不安全；不能据此声称某候选可通过 100ms。上述扫描与动态可行域是方案设计/验收要求，角速度定标保持 `PENDING`，不进入实现或测试用例维护。


## 8.14 slip-overshoot-v2 修订：30°±2°可达性、额度与 5.1s 定量审计

**状态：BLOCKED / DESIGN_NOT_CALIBRATED。** 本节按当前 gap 的六项交付要求给出可复核的关系式、预算上界和标定契约。分析使用既有几何 `N=128`、`dx=1`、thick `maxAvalancheFlux=80/s`、`vBottomCreepMax=10/s`、`totalMotionBudget=300/s`、悬沙扩散 `heavy=4/s, light=10/s`；任何数值不构成改参数授权。测试账本没有双阈值 active 转换，故只能完成几何/预算的解析定标，不能声称已经从观测标定出 `thetaStart/Stop` 或 P95。

### 六项修订逐条闭合情况

| # | architect 要求 | 本轮量化结论 | 状态 |
|---|---|---|---|
| 1 | 30°±2°目标与双角判据之间的映射 | +45°稳态时，目标表面坡度30°对应残差角22.911°；允许28°~32°对应停止残差20.563°~25.093°。`thetaStop≈22.91°` 是几何中心，不是观测拟合值。 | 解析完成；观测待定 |
| 2 | 起动角不得与tan30硬线冲突 | `thetaStart<30°` 必须严格成立，且大于 thetaStop；在有效角噪声/滞后未观测前不给固定点值。候选只可在 `(thetaStop,30°)` 内按逐边状态转移数据估计，30°端点排除。 | 规则明确；观测待定 |
| 3 | 说明怎样在所有子步不越界 | 每步先按真实材料 grant 推出 `thetaFeasible` 全边交集，再推进参考角；若交集为空则 hold/FAIL，不能越界后用后续滑移追认。固定 omega 不是安全证明。 | 接续§8.13；实现前审查 |
| 4 | 额度够不够 | 对全宽线性30°坡面，从平坦面经相邻输运需累计 edge flux L1≈100,893 units；单中央边峰值约1,182 units。现有总额度在5.1s乐观上限只有1,530 units，中央 edge 80/s 也要约14.78s。对这种线性全宽目标，现额度/速率明确不够。 | 条件性解析否决 |
| 5 | 5.1s的t90能否达成 | t90达到30°目标的90%即27°时，线性全宽 profile 所需累计相邻 flux约89,075 units；总额度最多1,530，中央通道最多408 units（80/s×5.1），故按当前相邻滑移路线达不到。现有真机 raster t90约6.5s且最终仅0.23°/0.52°，并非接近目标的通过证据。 | 当前方案 FAIL/BLOCKED |
| 6 | 完成参数标定与独立复核 | 现有 overshoot ledger 无 hysteresis active 转换字段，无法估计转折置信区间；不得把22.91°几何反算冒充经验标定。需先获得同子步 aligned 的 start/stop 逐边轨迹，再校准/holdout分离验证。 | 未完成，禁止宣称送审通过 |

### 30°±2°与停止角的反算

停止角必须定义为残差坡度的角度 `alpha=atan(|g-tan(theta)|)`，不是容器倾角本身。+45°时 `tan(theta)=1`。若 raster 沙面坡度 `phi` 目标为30°，则：

```text
g_target = tan(phi) = tan(30°)
alpha_stop_center = atan(abs(tan(30°) - tan(45°)))
                   = atan(0.422650) = 22.911°
```

对应目标带边界：`phi=28° → alpha=25.093°`；`phi=32° → alpha=20.563°`。因为停止发生在 `alpha<=thetaStop`，仅从容差反推的候选范围为 `20.563°<=thetaStop<=25.093°`，中心估计22.911°。+45/-45按奇对称使用同一绝对阈值。这里的“±2°”是全局 raster surface linear-fit 的目标，不表示每条局部边都等于30°；逐边硬约束仍独立要求 `|g[c]-tan(effectiveTilt)|<=tan30°`。

`thetaStart` 不能取30°：那会让状态机到硬边界才起动，且噪声/离散步进可能先违反 hard gate。数据标定时需先剔除越界 run，仅对未越界的 start transition 用 `r_before<tan30°` 的样本估计，取起动转折区间的保守上置信界低于30°；stop 取上节几何候选范围内由真实停止转折估得的值。若 `thetaStop` 的95%区间不落在20.563°~25.093°，则现有逐边触发量不能同时把最终 raster slope 锁在30°±2°，双角方案判失败，而不是强行填22.91°。

### 额度和时间的空间输运下界

为把“30°全局线性坡度”转成材料账本，按当前 raster metric 解释为128列整个有效宽度拟合斜率；先审最有利、无其他通道损耗的线性目标。令列坐标 `x=0..127`，均值63.5，平坦初态 `S0`，终态 `S[x]=S0+tan(phi)*(x-63.5)`。由相邻守恒输运，边 `j` 的累计右向 flux 为 `F[j]=tan(phi)*(j+1)*(127-j)/2`，所以：

```text
sum_j F[j] = tan(phi) * N*(N*N-1)/12
max_j F[j] = tan(phi) * N*N/8       (central edge, N even)
```

| 全局拟合目标 | `sum edge flux` | 中央边累计 flux | 现有80/s中央边的纯速率下限 |
|---:|---:|---:|---:|
| 28°（容差内最易端） | 92,917 | 1,089 | 13.61s |
| 30°（中心） | 100,893 | 1,182 | 14.78s |
| 32°（容差内最难端） | 109,197 | 1,278 | 15.98s |
| 27°（30°响应的90%角度） | 89,041 | 1,044 | 13.04s |

5.1s壁钟下，假定全部时间都是有效 physics time、共享 total cap 每步用满，最多只有 `300*5.1=1,530` units；该总额比30°线性目标累计相邻 flux低约66倍。中央 edge 的 slip 速率 cap `80/s` 在5.1s只给408 units，低于t90所需中央边约1,044 units约2.56倍；已有total/local竞争只会更差。v1实测 overshoot阶段约270子步、`totalLimit=1350`，说明该run的 physics budget 比壁钟乐观上限还低。

这组下界精确适用于“平坦相位基线→全宽线性表面profile、变化由现有相邻材料通量形成”的路线，不能直接外推为任意全物理状态无解：悬沙的局部垂向升降会暂时改变 raster `sandTop`，而历史证据没有给出能否在空间上定向地形成该30°线性拟合的状态轨迹。要把垂向通道计入替代构造，必须同时逐列账清 lift/settle库存、悬沙跨边扩散（最多14/s/edge）、共享300/s额度与最终恢复/稳态；不能把它们当作免费的坡面位移。当前 ledger 中 lift/settle主要改变轻沙，slip重沙 grant为0，表面 raster最终坡度不足1°，因此没有现成证据推翻相邻输运下界或证明垂向通道可替代。

### 最终判定与可送审边界

结论是：**30°±2°的阈值映射可解析到停止残差中心22.911°，但真实转折角尚未标定；当前预算/速率不足以在5.1s内通过相邻输运形成全宽30°线性坡面。** 把 `maxTiltOmega` 降低或加入动态可行域只能避免参考线跨过hard band，不能增加最大edge flux，也不能令均匀边grant产生足够坡度散度。现有可见 raster `0.23°/0.52°`、t90 `6.5s` 与目标相差甚远。

因此本方案不应以“已完成经验标定/READY_FOR_ARCHITECT”送审。可交 architect 的仅是数学审计和明确的条件性BLOCKED结论，请其复核下界假设及 raster metric 是否确为全宽回归；如保持当前相邻输运、80/s edge cap、300/s total cap 与5.1s硬时限，则路线必须判不可达，交 manager 决定是否调整目标口径/响应时限/路线。未经 manager 授权，不调整预算、几何、硬阈值或体验目标。若要经验标定双角阈值，先需由独立 observation route 提供逐边状态转换 ledger 与独立holdout；当前tester已停止且本轮没有此类新观测，故不得虚构观测值。

## 11.1 预研：论文 252 `f_sl` 列标量律与论文 254 Janssen 压强修正

**类型：挂账行动项预研；状态 `DESIGN_ONLY / NOT_CALIBRATED`。不实现、不改现有默认 `percolationRate=6/s`，不改变 P-003/P-005 既有验收。** 目标是提供保持列标量、现有 `q/r` 守恒拆分和预算账本的候选 rate 闭合，并登记深沙压强边界。理论来源为 `D:\mdbooks\ReadPapers\src\252.md` 的 Trewhela `f_sl` 律及 `D:\mdbooks\ReadPapers\src\254.md` 的准二维 Janssen 结果；论文 254 本地笔记仍为初读，以下 Janssen 用法只属预研。

### 11.1.1 列标量简化公式

令边残差 `d_e=S[e+1]-S[e]-tan(effectiveTilt)*dx`，单位为长度。列 `c` 的局部粗糙剪切形状代理为：

```text
chi_c = abs(d[c+1] - d[c-1]) / (2*dx)       // interior，dimensionless
```

边界列改用相邻单边差分；列数不足 3 时 `chi=0` 并记录低阶边界。`chi` 是 residual 的空间梯度代理，不是物理剪切率：单独将它代入论文的 `gammaDot` 会量纲错误，并会使静态弯曲坡面永久分选。

为构成量纲一致且静止关断的候选闭合，定义活动频率与剪切率代理：

```text
activityRate_c = max(
  abs(effectiveTiltRate_radPerSec),
  maxNeighbor(abs(nonPercolationDeltaSLastStep) / (dtLast*dx))
)
gammaDot_c = chi_c * activityRate_c                 // 1/s
```

`nonPercolationDeltaSLastStep` 只取前一已提交子步 settle/creep/relax/lift 等非渗滤 writer 对局部 `S` 的实际改变量；不计 percolation 自身，避免循环地用待求 grant 驱动自身。无前一有效子步、`dtLast<=0` 或 tilt/material activity 为零时，本列 `gammaDot=0`。该式是列标量工程代理，不是论文 252 的原始速度梯度；需独立验证静置关断、dt 收敛和剪切响应，否则不得实现为物理定律。

粒径参数不得与 P-011 的 heavy/light 通道名混为一谈。定义论文小/大颗粒直径 `d_S,d_L`、`R=d_L/d_S`，限定 `1<R<=5`；`phi_L,c` 为大颗粒**体积分数**。如库存只有质量量，需用独立粒子密度换算体积；确认“heavy 是否等于 large”之前不得把 `bh` 直接当作 `phi_L`：

```text
phi_L,c = V_L,c / (V_S,c + V_L,c)
dBar_c  = (1 - phi_L,c)*d_S + phi_L,c*d_L
F_R,c   = (R - 1) + E*phi_L,c*(R - 1)^2
```

SI 单位下，候选偏析速度为：

```text
f_sl,c = B*rhoStar*g*gammaDot_c*dBar_c^2
         / (C*rhoStar*g*dBar_c + p_eff,c)
         * F_R,c                                      // m/s
if signHeavyDown * f_sl,c < 0: routeIncompatible
else k_seg,c = signHeavyDown * f_sl,c / dBar_c       // 1/s
```

文献候选常数 `B=0.3744, C=0.2712, E=2.0957`；`signHeavyDown` 由论文 s/l 偏析方向与 P-011 heavy/light—细/粗粒映射确定，不是拟合旋钮。若映射后方向与当前仅支持 heavy 向下的 `percolateHeavyConservative` 相反，不得取绝对值；需先另行设计有符号双向通量。完整论文的 `R/phi_L/f_sl` 记号须在实现前核对，不能仅凭摘要改变材料语义。

唯一拟替换的是逐列 rate：

```text
through_c = min(mh_c, bl_c)
pair_c    = min(mh_c, ml_c)
request_c = min(0.5*(through_c + pair_c),
                k_seg,c * dt * (through_c + pair_c))
```

随后沿用既有 `q/r` 等量拆分、库存/容量裁剪、`totalMotionBudget` 和 `0.5*(through+pair)` 防过冲上界；不增加预算、不重发 clipped grant。`k_seg,c` 的单位是 `1/s`，替换常数 `6/s`，不可将 `f_sl` 的 `m/s` 直接当 rate。

### 11.1.2 Janssen 深度压强修正与适用边界

线性 `p_eff∝sandTop` 会在深沙区无限增长。若列深处于侧壁承载/Janssen 区制，候选压力项写为：

```text
z_c = sandTop[c] * lengthPerModelUnit             // physical depth, m
ell_J = A_cross / (mu_wall * K_J * P_wall)         // Janssen length, m
rhoEffBulk_c = (1 - porosity) * (rhoSolid - Sw_c*rhoWater)
p_eff,J,c = rhoEffBulk_c * g * ell_J
            * (1 - exp(-z_c / ell_J))               // Pa
```

`A_cross/P_wall` 为每单位槽长的截面积/侧壁接触周长比；`mu_wall` 是颗粒—壁面摩擦系数，`K_J` 是侧向/竖向应力比，`Sw_c∈[0,1]` 是饱和度，`porosity` 是孔隙率，`lengthPerModelUnit` 把模型列高映射为米。有效 bulk density 以 `Sw` 近似浮力折减：干态回到固相 bulk weight，饱和态用有效重度。若未来有独立孔压场，应改用总 Janssen 应力 `sigma_zz` 减去 `u_w` 得有效压力，不可再重复做浮力扣减。

```text
z << ell_J  => p_eff,J ≈ rhoEffBulk*g*z       // 线性浅层极限
z >> ell_J  => p_eff,J → rhoEffBulk*g*ell_J   // 深层应力饱和
```

将 `p_eff,J,c` 代入 `f_sl` 分母即为深度饱和抑制候选。**已知边界**：论文 254 是单分散、干燥、准二维容器 DEM；本地笔记报告有效 `K_J` 随堆积历史变化。它支持“压力可能饱和且依赖壁面/历史”的方向，不给出本产品的 `mu_wall/K_J/ell_J` 数值，也未验证湿润双分散的有效应力闭合。P-011 列模型没有真实槽壁力链、壁摩擦或孔压字段，`sandTop` 只是列沉积深度代理；因此缺少独立几何/材料测量时必须标记 `UNIDENTIFIED_BOUNDARY`，不得拍定 `ell_J` 或声称该项已标定。

### 11.1.3 设计结论与后续门槛

可预研的方向是以 `k_seg,c=f_sl/dBar` 替代常数 rate，同时保留现有渗滤事务；当前还没有唯一可实现的参数集。前置缺口为：细/粗颗粒身份与 `d_S/d_L`、体积分数映射、heavy-down 符号、长度标定、剪切代理验证及 Janssen 几何/摩擦/历史压力测量。`gammaDot` 与 `p_eff,J` 是降阶闭合，并非论文原式自动给出的产品参数。

进入后续 architect/developer 阶段前，须冻结这些参数来源并完成：无渗滤对照；静置 `gammaDot=0` 关断；浅层 `p∝z` 极限；深层饱和极限；粒径比 `R<=5` 边界；H/L 方向映射；逐列守恒、现有预算和 dt 收敛检查。未完成前状态保持 `DESIGN_ONLY / NOT_CALIBRATED`，生产默认 `percolationRate=6/s` 不变，P-008 不解除挂账。

## 12. 阶段一·C：现有通道表层效果调对执行方案

**状态：预写执行方案 / 等观测数据定标；不授权实现、改测试用例或放宽任何硬约束。** 本节把 priority 当前生效的阶段一·C（滑移收敛行为、安息角响应、±15°静止回归）转成数据到手后可直接执行的判定表。沿用已有预算、材料守恒、127 边硬安息角、raster 与终点门槛；滑移阈值尚未观测定标时不得填入占位角作为通过依据。

### 12.1 参数等待清单

| 调对项 / 决策 | 现在可先做的工作（理论或既有合同） | 必须等待的观测证据及执行门槛 | 数据到手后的动作 / 禁止事项 |
|---|---|---|---|
| 起动角 `thetaStart`、停止角 `thetaStop` | 固化双阈值状态语义：超起动阈值开始滑；滑行中低于停止阈值停止；两者之间保持状态。角度与 slope/residual 的 `atan/tan` 换算可先解析核对。 | 使用 v2.12 逐边转换事件，按既定校准/holdout 协议估计转折角及区间；`thetaStart` 须按既有搜索窗 `25°≤thetaStart<30°` 审核，窗外值走既有决策树；不得从 ±15°静止段推定阈值，也不得把 ±45°相位末尾当作停止事件。 | 只有转折可分辨、校准与 holdout 一致且停止事件确实观测到，才可提交数值。区间过宽、重叠或缺少 stop transition 时维持 `DESIGN_PENDING_OBSERVATION`，不采用 30°/20°默认值。 |
| 滑移速率与超角的关系（`requested/granted` 对 residual angle） | 可先定义需分析的曲线：按 `effectiveTilt`、`d/r`、active 状态、预算比例分层，区分请求速率与实际获批速率；理论上速率不得脱离现有 `maxAvalancheFlux`、local 与 total 预算。 | 必须有逐边、逐子步的 `requested`、`granted`、`budgetScale`、前后 residual、当前倾角及其它通道同子步 grant；需要覆盖超角大小变化及预算未限流/限流样本。现有 aggregate 的 `relaxRequested/Applied` 不足以识别本关系。 | 先拟合/报告分段关系、置信范围和预算受限比例，再判断阈值逻辑是否能带来渐进收敛。禁止只凭单一 run 的总 grant 直接调大 slip rate 或改变预算。 |
| 倾角更新速度与材料响应的联合限幅 | 可以按 §8.13/§8.14 的公式计算安全可行域、角度变化造成的 residual 增量上界；硬约束 `tan(30°)`、预算和 `P95≤100ms` 口径保持不变。 | 具体 `maxTiltOmega` 或动态投影参数，必须结合同一输入轨迹下越界、滑移 grant、表层响应和跟手 P95 实测；须确认参考倾角的推进量在材料可提供修正范围内。 | 选择同时满足零越界与跟手门槛的 Pareto 可行值；若没有交集，报告冲突交 manager，不以牺牲硬坡度或 P95 隐性放宽。 |
| ±15°静止死区与误触发抑制 | 可按几何先验核对平面残差角：`atan(|tan(15°)|)=15°<30°`，因此在倾角已稳定、表面接近平面时不应启动安息角滑移。 | 需要稳定倾角后的逐边 active 转换与 slip grant；倾角过渡期和稳定保持期分开统计，以排除 tilt 追随造成的瞬态 residual。 | 观测确认稳定段无 start transition、无 relax grant 后，冻结该回归基线。不能把过渡段暂态误作静止误触发，也不能通过提高起动角掩盖 tilt overshoot。 |
| 可见崩塌量、渐进速度及 raster 终点 | 可先冻结 pixel/raster 采集口径、±45° phase 时间轴、OLS 与 `R²/RMSE` 计算公式；理论可由相邻输运守恒检查边界移动与通量一致性。 | 实际可见位移、各边滑移速度、共享预算竞争、目标坡度 `30°±2°` 和 target-based `t90≤5.1s` 必须由正式 raster + 同子步 ledger 证明。若“可见”需用像素位移，报告不得用模型单位替代像素证据。 | 对照 SLIP-02/03/04 一并判断。不得把“发生过 grant”当作可见崩塌，不得把相位末端瞬态拟合斜率当作稳定终点。 |
| 材料写入的角度安全性与守恒 | 可以先按既有事务/硬约束逐通道核对写入前后坡度与质量账本；各通道预算、`tan30°` 上限及守恒阈值不变。 | 若任何通道 grant 的实际作用方向、相邻边影响或 residual 修正效率不清楚，必须由逐子步 before/after 状态和 grant 明细验证，不能按 request 推断。 | 先定位造成非渐进、越界或预算挤占的实际 writer，再调其参数；禁止追加预算、事后修复越界或更改几何来使本轮通过。 |

### 12.2 预期行为与 SLIP 判定映射

| 行为场景 | 预期现象 | 判定方法（通过条件） | 对应 SLIP 用例 |
|---|---|---|---|
| `+15°/-15°` 稳定保持：静止回归 | 在稳定倾角与近水平表面下，`15°<`约 `30°` 的起动角；滑移状态不启动、不发生 relax 滑移。其它已存在通道的活动单独记账，不能据此误判为 slip。倾角过渡过程单独分析。 | 倾角稳定且上一阶段活动状态已按停止条件退出后，检查逐边转换：不得有新的 start，relax slip grant 应为 0，且不得出现由 relax 写入引起的 surface 漂移。把稳定保持段 raster 变化与静止噪声基线比较；整段仍须满足 127 边硬约束、预算与账本身份对齐。 | **SLIP-01**（逐步硬约束）+ **SLIP-04**（预算/账本对齐）；结合七相序列中两段 ±15° phase。SLIP-02/03 不替代静止判定。 |
| `+45°/-45°` 超起动角：表层渐进崩塌 | 超过起动阈值后，相关边进入 active 并逐步向降低 residual 的方向滑移；坡面变化可见且连续，不应一帧跳变。滑行持续到边 residual 角降至停止阈值以下；全宽 raster 坡度朝目标 `30°±2°` 收敛。正负方向应对称，允许报告真实测得的差异。 | 逐边核对 `active` 转换与 grant 方向；每子步比较坡度/residual 变化，不能只看累计 `F-X`。raster 用全宽 OLS，要求终点坡度 `30°±2°`、`R²≥0.90`，且 target-based `t90`（首次达到 `27°`）`≤5100ms`。越界数必须为 0；连续 raster/边界记录应能证明变化可见且渐进。 | **SLIP-01**（127 边 tan30 零违约）+ **SLIP-02**（±45°收敛、拟合质量、target-based t90）+ **SLIP-04**（预算与子步对齐）。 |
| 安息角启动/保持/停止响应（覆盖角度转折区） | residual 高于观测标定的起动角时开始滑；进入起动与停止角之间保持此前 active 状态；active 时 residual 低于停止角后停止。停止后不在阈值附近逐步反复启停。角度值来自逐边观测，不以“容器角度约 30°/20°”替代 residual 转折。 | 对逐边事件按同 run、相邻有效 substep bracket；检查 start、hold、stop 次序与 request/grant。active 不得仅因 grant=0 被重标为 stop；报告阈值置信区间、预算限流样本及转折可分辨性。停止后的最后 1s OLS 坡度变化 `≤1°`，终点悬沙 `≤初始沉积沙总量的5%`。 | **SLIP-01**（硬坡度）+ **SLIP-03**（末段稳定与悬沙）+ **SLIP-04**（预算与账本）；状态转折字段采用逐边 `relaxStateTransitions` 观测契约。 |

**执行顺序**：先确认 ledger 与 raster 同子步、同倾角且覆盖完整；再独立评估 ±15°静止和 ±45°崩塌；随后按逐边事件确定 start/stop 行为与速率-超角关系；最后才调速率/倾角限速并重跑全部 SLIP 门槛。每次只改变一个已有参数族并保留同输入 RED 基线。任一轮若硬坡度、守恒或预算失败，立即判该候选不合格；不能以视觉改善抵消硬约束失败。SLIP-01~04 是现行正式客观用例编号，本方案只引用，不改写测试定义。
