# T86：step73 规划预览机制精确补证 v1

日期：2026-10-26。角色：tester。任务卡：T86。依据：`docs/architecture-T79-tilt-blocked-routing-v1.md` §3/§4 与 `docs/developer-T85-tilt-preview-trace.md`（`P011_TILT_PREVIEW_TRACE_V1`，50234fb）。执行器：`test/p086_tilt_preview_evidence.cjs`（独立设计，不镜像 T85 开发自测作证据）。raw：`test/reports/t86/p086_traces_step70_75.json`、`p086_results.json`。6/6 PASS。

## 1. 身份

依赖基线 50234fb（T85 trace 接口）；当前树唯一差异为 T56（114a7ee）对 `WaterSandField.ets` 的只增方法 `copyPublishedSceneStateInto`，不触碰 planner/trace 路径。trace 修订 `P011_TILT_PREVIEW_TRACE_V1`，全部 trace `status=COMPLETE`。开/关两臂 76 步结果与全状态逐字节等价（T86-F01 独立复算）。

## 2. 首个失败阶段与实际候选序列（回答 T79 §2 的问题）

step73 失败 attempt 的实际 preview 调用序列（非 passCount 猜测）：

1. `1:REQUESTED:PLANS_READY`（θ=0）
2. `2:OLD:PLANS_READY`（θ=0 旧角回退）
3. `CLAMPED` **未执行**，原因 `REQUESTED_INTERVAL_INFEASIBLE`（REQUESTED 区间已不可行，无需钳角重算）

**两个实际执行的候选都到达 PLANS_READY 且都在决策层被判 infeasible**；PASS1 确非 null（T79 §2.2 推断成立），collect/particle 阶段未失败。T47 遗留问题"preview-null vs 决策区间不可行"就此关闭：**是决策区间不可行**。

## 3. 决策层逐值证据（IEEE754 可往返，hex 已归档）

REQUESTED 候选（θ=0）：

- `slopeLo = +4.440892098500626e-16`，`slopeHi = −4.440892098500626e-16` → **lo > hi，空交集**；
- 取界边：cumulativeLoEdge=63、cumulativeHiEdge=64；
- 逐边：col63 `gNext=0.5773502691896262`（hex `3fe279a745903320`），`lo=gNext−tan30=+4.44e-16`；col64 `gNext=−0.5773502691896262`，`hi=−4.44e-16`；
- **关键**：`deltaS[63..65] = [0,0,0]`——preview 计划的通道 delta 为零（planner 不把雪崩松弛计入计划）；限制完全来自 **S0 本身**：col63 的 `s0Difference = 0.5773502691896262` = tan30（0.5773502691896257）+ 5e-16，恰好是 step72 雪崩松弛的安息角平台；
- preexisting 检查：violated=false（5e-16 在 1e-12 容差内），残差已归档——**两个判据（preexisting 容差 vs candidate 交集）确认分离**，非混称。

OLD 候选同θ=0，同样空交集。θ=5 对照臂（同状态 setTilt(5)）：REQUESTED feasible=true、thetaClamped=5、acceptedComparison=true——5° 成功的机制是**同状态**下其区间非空，不构成 0° 应成功的证明（与 T79 §1 一致）。

## 4. 机制定性（tester 只陈述机制，不裁设计）

当前每边区间 `[gNext−H, gNext+H]`（H=tan30）对**已松弛到安息角的床沿**结构性产生空交集：col63 要求候选坡度 ≥ +4.44e-16，col64 要求 ≤ −4.44e-16，任何 θ 都无解；θ=0 的"不变"也被要求坡度严格递增/递减。即：**雪崩的正常稳定产物（坡度恰在安息角）在现行区间合同下对全部候选角不可行**，与"安息角浮点 5e-16"无关——即使 S0 精确等于 tan30（lo=0=hi 的闭区间），θ=0 在 col63(col64) 仍要求 g'≥0 与 g'≤0 同时成立且后续边再加 tan(θ) 偏移，θ=0 恰好可行当且仅当交集中含 0；本例 5e-16 使 0 落在交集外。

## 5. 按 T79 §5 分流表

落入**第 3 行**：S0 与计划正确、现行边界/角交集仍排除正常松弛产物 → **设计候选成立，交 designer** 定义安息角合法态、容差一致性与 0 目标定向区间。不是工程 preview/缓存错误（第 1 行排除），不是粒子/collect 拒绝（第 2 行排除），不是预期 fail-closed 正确（第 4 行需 designer 先定义合法态）。

## 6. 边界

不判业务已修；无新阈值/epsilon；未改产品/planner/trace；未用真机；缺值无（s0/edges/decision 全齐）。原 FAIL 保留，本报告只供 architect 分流。
