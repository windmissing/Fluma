# D1 R3：缓冲复用证据与待裁定的后续收口方案

日期：2026-10-24。developer。依据：manager 双端退款裁定 §3-R3（保持严格零分配，或提出降级方案交 manager 裁定）。**PROPOSED / NOT_APPROVED；不替代原零分配门槛，不关闭 D1。**

## 已完成的收口与证据

1. R9 adapter 的 sidecar 是构造时定容 TypedArray/字符串槽；refund/close 只填调用方定长2收据。10,000 次跨子步注册/退款/close 的构造器 trap + 正常路径 AST 审计通过。仅证明 adapter 局部，不外推整个 M1。
2. M1 增加 `warmUpTransactions()` 与 `copyStateTo(out)`，prepare 复用一个 scratch 池及 before/after 完整状态缓冲，不再逐笔创建完整粒子池和 TypedArray 快照；旧 `snapshot()` 保持诊断端独立副本语义。
3. `test/probe_p011_particle_allocations.cjs`：预热后100次零授予 deposit prepare/publish/finish，scratch 与两份状态缓冲引用恒定、TypedArray 构造次数0。但全 M1 正常路径保守静态审计仍登记60处潜在分配站点。探针明确返回 `NOT_ZERO_ALLOCATION`、exit=2；不是60次/步的运行时堆分配计量。

## 尚未完成的路径

包括 `beginStep` 的参数/冻结视图对象和身份序列化、collect 的请求/运动数组与排序闭包、journal finish 的复制/序列化、apply 的结果数组、prepare 的独立 patch 句柄及出生/合并事件对象、prototype 字段/预算 checkpoint、动态 key Set 等。删除 TypedArray 分配不能令这些路径自动变成零分配。

## 建议 A：保持严格门槛（需 architect 确认补充热接口）

- 保留原公开 API 作为冷诊断/兼容入口；新增调用方拥有的定容 plan/request/motion/result/field/budget/guard/event 缓冲，显式 count/index，不用 push/slice/临时 Map/Set。
- 给热路径使用数值 request 身份与代际 patch 句柄；序列化字符串 requestKey 及 JSON 只在证据导出边界生成，保持输出字段及事务身份含义。
- patch 句柄不能简单换成被循环覆写的同一个返回对象：调用方保留旧引用时，它会别名到后续新句柄，破坏现有“旧句柄/跨子步拒绝”要求。应由 architect 明确数值代际令牌或 caller-owned out 的生命周期与 publish/restore 校验契约。
- 为此需要收口 `prepare/collect/apply` 的热侧签名及返回值所有权。按 developer 纪律，不自行把 §13 精确接口改成另一套接口，先交 architect/manager 确认。
- 确认后以完整 M1 可达调用链的显式分配审计、长时固定缓冲身份、错误回滚及既有全部守恒/预算用例验证，不能只测 refund 或只禁 TypedArray。

## 备选 B：有界分配降级（仅提案，未获批准）

由 manager 显式裁定先接受“定容 canonical/scratch/票据/日志存储，热交易无 TypedArray/池重建；允许有界计划/句柄/序列化轻量分配”的隔离原型门槛，剩余严格零分配后续独立卡。此方案会改变当前完成标准，**developer 不自行采用**；本轮不会据此宣布 D1 全完成或性能已验证。物理阈值、冻结 estimand 与 R9退款含义均无变化。

当前等待的是 R3 后续接口/验收决策，不再是退款语义争议；R1 adapter 桩、R4旧行为守卫及授权的旧退款负对照已可独立审查。
