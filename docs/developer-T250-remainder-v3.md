# T250 续工：隔离计量与故障计划接口

2026-10-29，developer；不使用真机。接续 T256，保留先前同包三 backend/两 freeze 臂和 12 组终态一致证据；本轮没有重跑旧 64-root 矩阵。隔离工程 `.codex/local-test/t250-v5`，生产默认路由与业务判据均未修改。

本轮在实际 `P011TiltPlanningPassService.previewAt` 与终结句柄记录 step 预览角色尝试、返回有效/空、最终接受/拒绝；与 `WaterSandField` 原 source λ 循环计数分开。当前 step planner 使用角度候选/回退，没有名为 λ 的独立重试标量，因此不把预览次数伪称 step λ。选对路径补 `M_FIND/FOUND/NO_PAIR/ERROR`，`M_CANCEL` 随 owner 取消；原有 birth 完成、merge 尝试/成功、A 角色、source λ 尝试保留。尚无完整 B_enter/commit、审计 logical/execute/certHit、全部父子作用域计时，计量仍非完整 T205 合同。

可见资源证据逐实际 `byteLength` 统计 selector 行、CPU 计数、metadata、冻结副本及索引数组；root/workload/native/error/state 行以实际 UTF-8 长度求和。该值限于 fixture owner 在结束后保留的存储，不冒充 JS heap 峰值或 GPU driver/RSS 总量；外部副本和 wrapper 实测仍 NOT_MEASURED。归档原 source/step root 的 before/after、回执与 firstCode 继续保留，不能代替 cut 内权威字节。

新增 `TxFaultPlan` 独立隔离接口，强制核 `fixtureId/buildSHA/backend/freezeArm/PID/rootId/cutId/edge/ordinal`。20 cut × BEFORE/AFTER 的合法计划均明确返回 `NOT_IMPLEMENTED_ORIGINAL_CUT_INJECTOR`，身份或范围错误另报专码，Worker 在启动物理 root 前拒绝，绝不捏造原错误码/序号、回滚通过或 fault PASS。精确缺口：生产 20 cut 虽有 TxWorkTrace 入口，但没有可控、能保留原 firstCode 的逐 cut 故障注入器及对应内部 authority/receipt/events/lineage 冷归档接口。撤销后独立冷跑的检查尚无法执行。候选异步 FIFO 未实现，不作为旧同步实现缺陷。

`test/t250/focused.cjs` 单一 WET_192_64/LEGACY_SCAN/1 step 的针对性自测 **50/0**，核原工作量身份、可见字节和 20 cut 双边计划显式不可注入；GPU 为 CPU 以外路径未在本离线用例运行。hvigor 隔离包 `CompileArkTS`、native、`PackageHap` **BUILD SUCCESSFUL**，零错误；见 `test/reports/t250/v5-focused.json`、`v5-package.log.gz`、`v5-manifest.json`。unsigned SHA `8565f14c3dafd2a3df3e8f69e916e759625b3f1f5e3d1a86a0c30b6f741e731d`；T257 用原授权材料签名且 verify-app/profile 通过，signed SHA `c4a569dc17045ec7d0519d896d5f469c1486c60abcb701f5587d1e7bb9fbab76`，未装机。

结论：**部分交付，T250 看评论**。继续缺原 cut 可控故障接口、逐点原字节/首错与回滚验证、完整事务工作量及外部资源可采面；T251 只能消费已实装的局部证据，不能把 20 cut 或整体收益当 PASS。
