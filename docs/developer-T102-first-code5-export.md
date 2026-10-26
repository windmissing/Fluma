# T102 developer 回执：设备首次 code5 有界导出

日期：2026-10-26。未使用真机。

空容器 DEMO 在页面可见期间启用既有 T85 只读 preview trace。每 tick 仅保留本次至多两条子步 trace，首次 code5 后脱钩 sink 并分片导出一次；失败门、已成功外源与物理回滚/停止逻辑均保持原样。用户主动重置新 run 时重新布防，不自动重试。

导出包括 run、attempt/state/config、介质、dt、target/effective、派生输入与最近真实三轴 sensor，完整 T85 S0/候选 stage/delta/edge/blockedReason，以及提交前后 commit、池 airborne/contact、源 epoch、池进度与最近至多八条粒子事件。单次最多 64 个 2048 字符片；trace 不全、溢出或截断明确 `TRACE_INCOMPLETE`/dropped，片序号可用于重组。不会每帧打印全数组或调用递增身份 getter。

离线 `run_t102_first_code5_units.cjs` 用真实 step73 code5 验证一次失败轨迹、S0 128 列、候选上界、run/state 身份、保留外源与脱钩后无再次送达；T85 7/7 等价回归通过。hvigor ArkTS/native/HAP BUILD SUCCESSFUL，零编译错误。设备日志分片完整性与真实 R2 同机制尚待 T103 真机采证，不推定 T95 长窗 LAND 已放行。
