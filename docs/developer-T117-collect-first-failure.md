# T117 developer 回执：COLLECT 首错误原码诊断

日期：2026-10-26。不使用真机。

M2 `beginCollect` 记录 ENTRY/TOKEN/BEGIN_HOT/COLLECT_A/COLLECT_B/COMPARE/READY 阶段、原 Hot 与 M2 返回码、原 rollback 返回码及 stepOpen/work.active/epoch/budget/pageBacklog 前后状态。Hot begin 的主要失败谓词与首列索引可读；compare 失败仅诊断第一个身份/计数/请求列字段。所有标量槽冷创建，诊断开关关闭时不生成 trace。

T85 trace 的 `firstFailure` 在 cleanup 前首次写定：PARTICLE_BEGIN、CHANNEL 或 AUDIT。audit 漂移沿原路径只调用一次 `getDrift()`，记录实际三分项、原 DRIFT_LIMIT 与 initial/source/waterTransactionNet；没有更改 audit 判定或公开 errorCode。每条子步 trace 另记录提交前后 ID、状态相关性 digest、cleanup 后粒子与预算身份；T102 首次 code5 导出自然携带这些字段。digest 仅供关联，不替代权威逐值快照/回放证明。

离线 `run_t117_collect_failure_units.cjs` 4/4 PASS：真实 M2 BUSY、Hot PAGE_BACKLOG 原码、实际 audit 漂移失败和诊断开关场/池等价。T85 7/7、T102 首 code5 与 M2 事务 14/14 回归通过。hvigor ArkTS/native/HAP BUILD SUCCESSFUL，零编译错误。真实设备 COLLECT99 首因尚待 T119/T118 取证及 architect 分流；本卡不宣称物理故障已修。
