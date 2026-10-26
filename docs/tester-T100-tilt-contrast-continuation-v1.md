# T100：规划对照证据纠正 v1（真实 θ5 轨迹与浮点辅助标签）

日期：2026-10-26。角色：tester。任务卡：T100。依据：`docs/architecture-T90-tilt-interval-routing-v1.md` §2/§4（版本化续证，不重跑整套；T86 bfe45b4 旧 raw/6 例保留不覆盖）。执行器：`test/p100_tilt_contrast_continuation.cjs`。raw：`test/reports/t100/p100_results.json` + `p100_theta5_trace.json`（**真实 θ5 trace**）+ `p100_theta0_edges.json` / `p100_theta5_edges.json`（全边复算）+ `p100_settilt_snapshots.json`。5/5 PASS。

## 1. T90 §2 指出的三个采集缺口全部关闭

1. **θ5 trace 误归档**：T86 的 `p086_theta5_trace.json` 写的是主臂 `traces[traces.length-1]`（θ0）。本版归档**局部 t5 数组的最后一条**：`target=5`、REQUESTED 候选 `θ=5`、`roles=[1:REQUESTED:PLANS_READY]`、resultStatus 非 blocked，与 θ0 trace 逐字节不同（T100-01）。
2. **E01 非严格断言**（`accepted!==null && traceAfter!==undefined`，且 setTilt 返回 void）：本版改为**严格断言链**——`stepAfter.ok===true`、`trace.target===5`、REQUESTED `θ=5`、`decision.feasible===true && acceptedComparison===true`（T100-03）。setTilt 的 void 返回值不参与判定。
3. **tan30Hex 全零 / exceedsHardMax=false**：旧代码 `Buffer.from(DataView-set 的 buffer, Uint8Array)` 取错字节源。本版用正确的 DataView→Uint8Array 路径：`tan30Hex=3fe279a74590331c`；col63 `s0Difference=0.5773502691896262`（hex `3fe279a745903320`）**超 hardMax 精确成立**（差 = +4.440892098500626e-16），且在 1e-12 preexisting 容差内（T100-05）。finite/Inf/NaN 用显式 `{tag, value, hex}` 编码（enc()）。

## 2. setTilt 前后身份摘要（T90 §4 材料起点关系）

`p100_settilt_snapshots.json`：setTilt 前后 **S0 逐值相同、池 liveCount 相同**，变化仅 target（0→5）、effectiveTilt、p011SampleSequence/命令阶段——**材料起点一致，但 θ5 会重跑粒子/collect 生成不同的候选计划**（T90 §1 提醒成立，见 §3）。physicsCommitId 在 setTilt 前后不变，step 后推进。

## 3. θ0 失败 vs θ5 成功：真实计划逐边复算（生产顺序，IEEE hex 归档）

| | θ0（失败 attempt） | θ5（成功 attempt，真实 trace） |
|---|---|---|
| deltaS[63..65] | [0, 0, 0] | 非零（复算见 p100_theta5_edges.json） |
| 交集 slopeLo | +4.440892098500626e-16 | −0.04636900367897223（hex bfa7bdad94b2d610） |
| 交集 slopeHi | −4.440892098500626e-16 | +0.0901133354419345（hex 3fb711aae4a8160c） |
| 空交集 / 含 0 | **empty=true** / 否 | **empty=false** / containsZero=true |
| 取界边 | loEdge=63 / hiEdge=64 | loEdge=63 / hiEdge=64 |

**T90 §2 的判断被数据证实**：θ0 计划的空区间**不可**外推到 θ5 计划——θ5 的真实计划区间非空且含 0，feasible/accepted 严格成立。两个 preview 是不同的计划（不同 collect/粒子 delta），材料起点相同不蕴含计划相同。

## 4. 与 T98 设计的接口

本续证不改变 T90 §3 分流（局部数值/合法态设计问题成立，交 designer）。θ5 真实计划的 delta/全边区间可直接供 T98 做"不同候选 θ 的计划差异"与"定向区间/角速度交集"的设计输入。tester 不整体归因。

## 5. 边界

未改生产/判据；T86 旧 raw/6 例保留；未重跑整套（只补缺）；未用真机。执行器无 NOT_IMPLEMENTED 项；0 真实失败。
