# T250 续工：事务工作量与同步服务入口

2026-10-29，developer；不使用真机。接续隔离同包 v3，本轮只修改 `test/t250/generate.cjs` 和聚焦自测，生成独立 `.codex/local-test/t250-v7`，不碰生产默认或 gpu_resident 主核。

实际出生循环补 `B_BIRTH_ENTER`，槽位/派生写入完成后 `B_BIRTH_COMPLETED`，action journal 成功后 `B_BIRTH_COMMIT`；三者分别计入失败前工作，不用 receipt 倒推。选对 `M_FIND` 在调用入口计一次，`M_FOUND/NO_PAIR` 按原结果，抛错单记 `M_ERROR`；原先退休时误称 `M_CANCEL` 的行改 `OWNER_CANCEL`，避免把正常销毁当 merge 取消。全量 `auditParticleHot` 入口计 `A_AUDIT_LOGICAL/A_AUDIT_EXECUTE`，该隔离 fixture 无 cert-hit 分支，不能声称通用 A 全覆盖。保留 source λ 和 step preview 分账。

Worker 从 Launcher 同一 uptime 时钟取得 `enqueuedAtMs`，记录 `queueMs`（postMessage 前→Worker 接收处理）与原 `serviceMs`（Worker 开始→归档写毕）分列。现有同步 Worker 一次性 owner 不提供多请求异步 FIFO；不能以此声称 FIFO p50/p95，设备分布仍待 T251。

单 fixture 聚焦离线 **55/0**，核 B 三计数身份、M 结果拆分、audit logical=execute、可见资源字节与 20 cut 双边 faultPlan 显式 NOT_IMPLEMENTED。hvigor native/ArkTS/PackageHap **BUILD SUCCESSFUL**，零错误。unsigned SHA `0d05c2130c275b29e2ac219db6512dc5513078a5ff17a2ad9298a98cca89c115`；按 T257 原授权流程另签新 v7 包，signed SHA `f78aef78bf082d20f0a0881de369dde4b9d99973be12dfb55c7e861e64a8e692`，verify-app/profile 均通过，未装机。`test/reports/t250/v7-focused.json`、`v7-package.log.gz`、`v7-manifest.json`、`v7-dictionary.json` 与 `test/reports/t257/T250-v7.json` 为本轮证据；没有重跑既过 64root/六路矩阵。

**仍为部分交付，看评论。**20 个原 cut 没有可控原码故障入口；原 `firstCode/receipt/events/lineage` 的逐点冷归档与回滚身份仍缺。完整父子作用域、M active/cancel 原语义、独立 step λ 标量和资源外部峰值仍未实现/不可测，不能宣 T205 全合同、设备 GPU 或整体收益 PASS。T251 的相应项目保留真实工程前置。
