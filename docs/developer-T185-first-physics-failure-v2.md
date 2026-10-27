# T185 开发回执

2026-10-27；按 T181 §4.2；不使用真机。

`P051EmptySandDemo` 在原 field.step 返回 false 后、failed/stop 清理前锁存 P051_FIRST_PHYSICS_FAILURE_V2。固定首错及最近 8 条生命周期独立于原 256 行可丢弃事件流；所有原码含 null 原样记录，未知 stage/Hot/M2/候选/实际 θ 为 null + NOT_CAPTURED。仅本次新生成且匹配 run/commit 的 trace 可补字段，拒绝复用上次 trace。旧 beforePool=LAST_PRESENTED 仍保留原语义。

记录原 step 前后 commit、实际秒 dt 与纳秒 frameTime、source 成功序号/epoch/真实准入计数、传感器/输入/stale、last-good publication 身份。source before 在本 tick 准入前，commit before 在准入后、step 前；准入成功后 step 失败不回滚 source。没有增加 step/snapshot/copyStateTo/updateSandTop/audit/rollback。

重复 page stop 仅计 repeatCount、生命周期关联同一 firstFailureId；sticky 不重跑 step。reset 前页面先导出，模型保留一份冻结旧 run 导出，当前槽清空并增加 generation；导出返回独立副本。页面每首错仅输出一次 V2；保留原 emitFirstCode5，未改失败/reset/自动恢复策略。诊断异常不覆盖原物理结果。新增捕获会读取已有 getDryDiagnostics/getLastError/trace 标量及计数，非 Hot 无分配认证；序列化只在冷导出，未测设备开销。

自验：新增 11/11（原码1–6/null、sticky三次、禁止额外物理取证调用、reset/两模型身份、8槽截断计数、成功source后step失败、旧trace拒用）；原 T72 3/3、DEMO 5/5 通过。结果在 `test/reports/t185/`。

Native、CompileArkTS、PackageHap 均通过。assembleHap 末端仍因既有 SignHap 00303116 密码长度配置失败；未改签名凭据。没有设备/E2E/历史首错复原或数值修复结论。页面实际日志长度/系统截断须由独立设备取证确认，完整模型冷导出可直接读取；原code5 trace截断逻辑不变。
