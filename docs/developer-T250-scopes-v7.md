# T250 原函数父子作用域接线

2026-10-29，不使用真机。隔离 T250-v14 在原 WaterSandField 206 与 ParticleHot 33 个函数作用域采用已有 TxWorkTrace 记录 enter/leave/fail、root/parent、inclusive/exclusive 与原 code；clock 来自同包 caller monotonic ms。4096行固定存储，满置 INCOMPLETE，未捕获 read-set context 明标 NOT_CAPTURED；不以零填充当有效身份。生产默认未接此 trace。

开关等价12/0：authority pool/budget/commit 原字节一致、父子栈/时长身份成立，2036个子scope、最大深度12；4096行满且incomplete=true，不能说全run完整trace。既有聚焦59/0、cut6两臂24/0。native/ArkTS/PackageHap BUILD SUCCESSFUL，新包签名verify-app/profile PASS。证据 `test/reports/t250/v14-{scopes,focused,manifest}.json`、`v14-cut6.json.gz`、`v14-package.log.gz`，签包见 `test/reports/t257/T250-v14.json`。

**仍部分交付，看评论。**除cut6 BEFORE外39边缘原注入器尚缺、read-set上下文和全阶段/外部资源峰值不完整；设备未跑。定容trace满的事实不能记T205全合同PASS。
