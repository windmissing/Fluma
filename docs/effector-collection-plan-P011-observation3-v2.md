# P-011 观测③可达平台标定 protocol v2（effector 执行稿）

**状态：DRAFT / NOT PREREGISTERED / NO RUN AUTHORIZED BY THIS DOCUMENT。** 本文落实 manager 对观测③的裁决及 `algorithm-P011.md` §8.16：阶段一只在现有 planning/tan30 safety 闸门内观察可达的 ±15°/±30°平台；±45°行为留待阶段二。本文不授权绕过 planning、tan30 或其他 MUST-PASS guard，也不表示已执行任何采集。

## 目的与边界

以新版本对称 schedule 观察 accepted effectiveTilt 平台上的真实 start/stop 转换，评估四个方向 cell 的 bracket yield 和右删失情况。现有 34-run manifest、原 v1 schedule hash、分组结果及其 run/artifact 均不得沿用于本 protocol；旧资料只作为历史证据保留，不纳入 v2 的 run 数、fit、holdout 或 bracket 配额。

保持 `thetaStart∈[25°,30°)`、`thetaStop∈[15°,25°]` 候选窗及既有统计门槛。±15°是静止/停止观察平台，本身不能激励完整 start 窗；±30°提供当前 safety 域内最大规划激励。实际 accepted 平台约 29.9°时，未覆盖到的近 30°区间按右删失处理。不得通过 capture-mode、测试模式分支或其他方式放宽 planning/tan30 闸门。

## 新 schedule 草案

每个 run 按以下完整顺序执行，不删相、不调序、不改目标角。每个 phase 的 hold 暂沿用 v1 已注册值 `holdMs=5100`；本 v2 schedule 的逐行文件、字段、runner 映射和 hash 仍须在预注册前生成并冻结。零度相位是显式命令，不得由输入衰减或状态机隐式复位代替。

```text
0 / +15 / 0 / +30 / 0 / -15 / 0 / -30 / 0
每个 phase holdMs=5100
```

平台角按 telemetry 中实际被接受的 `effectiveTilt` 判定，不能按命令值 `targetTilt` 判定。必须同时记录 `targetTilt`、`thetaAccepted`、`effectiveTilt`、`effectiveTiltRate`、拒绝/钳位状态、phase、连续 `substepId`、时间戳及完整逐边事件。稳定平台须满足预注册的角度容差与 rate 判据；容差数值、rate 判据、最少连续子步数和平台取样窗口是**预注册阻断项**，必须由 designer 给出并由 tester/checker 审核后冻结，本文不自行猜定数值。若请求角与 accepted angle 不符，该期间是 planner hold/拒绝区间，不得计入目标平台或 bracket。

每 run 仍须保持设备/build、几何、candidate pair、raster/ledger schema、source identity 与运行路径一致。所有计划输入必须与 schedule 1:1 对应；任何实际提交子步的 hard-gate violation 必须为 0，并通过预算、守恒、库存、容量、账本/telemetry 对账和边身份审计。观测身份不构成安全豁免。

## 转换与删失判读

每个 fit/holdout 集分别报告 `start/+`、`start/-`、`stop/+`、`stop/-` 四个 cell。一个有效 bracket 必须来自同一 run、同一候选、同一物理 edge、稳定 accepted 平台内连续有效的相邻 `substepId`；保留实际 `alpha=atan(abs(d)/dx)`、前后 active、request/grant/budgetScale、accepted angle 与 planner 状态。按 run 聚类统计，不能把边或子步当作独立 run，也不能跨 cell、跨 run 或跨符号补齐。

真实 stop 的必要条件是实际状态转换 `activeBefore=true → activeAfter=false`。阶段结束、run reset、平台切换、planner hold（包括 `requestedAngle!=thetaAccepted`）、请求被拒绝、预算限流（含 `granted=0`）及数据中断都不是 stop；限流期间必须保持 active。抵达 ±15° 时若 edge 已 inactive，该平台静止不能算 stop。真实 start/stop 还须由同一 edge 的相邻有效残差样本形成 bracket；ramp、拒绝/钳位期间事件不得冒充平台 bracket。

对每个 cell 报告有效平台样本数、独立 run 数、真实转换数、bracket 残差覆盖范围及最大实际可达 `alpha`。若 start 未发生且安全样本最大值受约 29.9° 上限截断，标记 `RIGHT_CENSORED_AT_TAN30`；不报告点估计、不把它解释为阈值失败或通过，也不把未覆盖区间当作 bracket。若没有真实 stop，标记 `STOP_TRANSITION_UNOBSERVED`。任一适用 cell 缺少转换或不满足既有置信区间/样本门槛，保持 `INCOMPLETE / BLOCKED / NOT_CALIBRATED`，不得镜像推断另一符号结果。

## 预注册、pilot 与扩批 gate

1. 本 protocol 进入设备采集前，必须形成全新的版本化 schedule 文件和 hash，并冻结平台容差/rate/连续子步规则、阈值候选 manifest、排除规则、随机化方案、数据 schema、runner/build/source identity 与 checker 版本。由 designer 提供可辨识性/平台判定参数，tester 同步 checker 与 test-cases 并确认 hard gate、planner hold、删失和真实 stop 口径；effector 记录预注册材料及审批身份。上述事项未全部冻结和审核，不得启动 run。
2. 原 34-run manifest 不可沿用。新 protocol 必须重新预注册并为 v2 建立独立 run manifest；原 `24 fit / 10 holdout` 与每 cell/run 配额只在新注册材料明确确认适用后方可执行，不因旧 manifest 自动继承。pilot 的身份及其是否计入后续 fit 配额须在预注册材料中预先写明，不能看结果后再决定。
3. 预注册完成后仅允许一个 clean pilot。Pilot 必须使用新 schedule、完整九相及生产等价安全闸门；本执行稿本身不启动设备。Pilot 只检查 schedule/命令身份、±15°与±30°正负 accepted 平台是否可达、稳定窗口是否满足冻结判据、MUST-PASS 安全审计和四 cell 的真实 bracket yield/删失状态；不得解封 holdout、拟合并宣称标定完成、临场改 schedule 或放宽门槛。
4. Effector 提交 pilot 的原始 artifact、manifest/hash、逐边 telemetry、平台可达性与四 cell 结果、hard-gate/预算/守恒/库存/容量/账本/边身份审计及删失记录。**manager 审阅 pilot 证据后才决定是否扩批**；在 manager 明确裁定前不得启动第二个 run。若安全 MUST-PASS 任一失败，立即停止并报告；若平台/某 cell 无可用激励或呈右删失/无真实转换，照实报告，不得自行扩批或改方案。
5. Manager 批准扩批后，按新预注册 manifest 执行；run 整体分层随机，fit/holdout 隔离。原 v1 34-run 分组和任何 v1 artifact 均不移组、不拼接、不补入 v2。fit-only yield 审核通过前不得解封 holdout。样本不足时按预注册规则标为 incomplete 并提交 manager 再裁决，不得自行增加样本量或改变阈值/闸门。

## 当前执行状态

`NOT PREREGISTERED`；pilot 尚未执行；没有新增 run 或有效标定数据。观测③仍不得报告 `thetaStart/thetaStop` 已标定。±45°行为及无法在现有安全可行域内识别的部分保持阶段二议题。
