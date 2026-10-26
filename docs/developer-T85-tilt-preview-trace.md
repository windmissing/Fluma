# T85 规划预览只读诊断交付

日期：2026-10-26。角色：developer。合同：`docs/architecture-T79-tilt-blocked-routing-v1.md` §3，身份 `P011_TILT_PREVIEW_TRACE_V1`。不使用真机；未修改 planner 判线、epsilon、阈值、源或物理事务。

可选 `WaterSandField.setTiltPreviewTraceSink(null|sink)` 默认 null。只有开启时才复制冻结 S0、实际 preexisting 检查残差、每次实际 REQUESTED/CLAMPED/OLD 的通道 delta、生产顺序 deltaS、粒子增量、逐边区间与累计交集（含独立上下界来源边）、角/角速度/定向交集与比较差，以及关键边 Float64 IEEE754 hex。实际 collect/particle 阶段及状态随调用序号封存，未执行候选附控制流原因。单 attempt 最多 3 候选。外部 sink 在 step 返回/rollback 和 preview finally 之后调用；抛异常只标 `TRACE_INCOMPLETE`，物理异常仍抛原 Error。无 sink 时不分配 trace 快照、不触碰原有结果。

`node test/run_t85_tilt_trace_units.cjs`：7/7。原 step73 code5 仍发生；trace ON/OFF 的真实字段、预算、粒子/空槽、sliding、提交身份逐值一致；抛错 sink、真实 collect/particle 拒绝阶段、异常、默认 null 均覆盖。保留独立 `test/reports/t85/selftest-traces.json` 中 step71–74 原始 trace。planner/gesture/input/particle/advection/S3/S4 等现行套件见 T71 regression.json，均 exit0。hvigor `CompileArkTS + BuildNativeWithNinja + PackageHap` 零错误。T86 tester 用此诊断采真实机制补证，再交 architect 分流；本卡不宣称 step73 业务已修复。
