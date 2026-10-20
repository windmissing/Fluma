# P-011 SLIP §21 修订裁决（designer）

**日期：2026-10-20**  
**依据：** [architecture-P011.md §21.1 表 #8/#9、§21.2 结论 1/2](architecture-P011.md)  
**范围：** 裁定 tester 是否按超出原授权的审查建议修订 MUST-PASS checker；不改物理算法、预算或验收上限。

## 裁决

1. **A-2：删除 SLIP-04 的 `relaxAppliedTotal==0` MUST-PASS 判定。** 理由：该断言与 ±45° 崩塌收敛要求直接冲突；relax 是否越界由逐步预算上限和 SLIP-01 的坡度硬约束判定，零工作断言没有额外合法性信息。
2. **A-1：删除 `clipped==0` 判定，改为裁剪账目对账。** 对每个子步检查 `clipped ≈ requested - used`（分别按适用的 bottom/total 账本字段和既定浮点容差核对）；保留 `localUsed<=localLimit`、`bottomUsed<=min(bottomRequested,bottomLimit)`、`totalUsed<=min(totalRequested,totalLimit)` 上限判定。共享预算下发生裁剪是设计内行为，账本不一致或实际超上限才是失败。
3. **B-1：批准 tester 同批修正 runRole 判别。** `runRole=null` 的 SLIP 观测 run 跳过校准专属的 `runRole/thresholdCalibrationId/thetaStartDeg/thetaStopDeg` 校验，但仍检查 `transitionTelemetryAvailable`；`runRole∈{calibration,holdout}` 时执行完整校验；其它身份值拒绝。此修订使 SLIP schema-v3 run 遵守 architecture §19.13.3 的裁定。

## RED 基线与交付路由

上述修改只修正判定口径，不预判现有 v1 run 通过。tester 同步更新正式 checker、`test-cases.md` §11.4/§11.5 与 RED 基线报告；报告保留历史 RED 数字作为旧口径证据，并用修订后的 checker 重新计算修订口径结果，清楚区分两者。不得改物理参数、预算上限、tan30 硬门槛或 SLIP-02/03 标准。

**一句话理由：** 去掉与崩塌目标矛盾或否定共享预算机制的零值断言，改为验证坡度、预算上限及裁剪账本的真实不变量。

**执行状态：** 已完成。checker 删除两项零值失败条件，加入 bottom/total clipped 对账；schema-v3 runRole 分支已修复；`test-cases.md` §11.4/§11.5 和 RED 报告已同步。修订口径下历史 run 仍为 `FAIL`：756 个预算账目不匹配子步（bottom 742、total 486，存在重叠），tan30 仍有 484 个违约子步/59,413 条边。历史旧口径的 1,738 仅保留为旧判定产物。py_compile 通过、逐边遥测单测 14 项通过；正式 checker 对该 run 返回预期 FAIL。该裁决不改变 SLIP 总体状态。
