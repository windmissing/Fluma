# D1 待发任务评论：双端退款接口差异

2026-10-24，developer 反馈。目标任务：`6adc52e4e4b0e6ce82a12a85`。

发现 rev1 与当前真实退款接口的差异，不涉及修改冻结预算或守恒门槛：

- `clipSubstepMotion(c,c',m)` 实际对两端 local **各扣 m**，total 扣 m；D1 桩在 local/scale/total 参数网格中与它的 reserve 数值一致。
- `materialGuardRefund(m,[c,c'],false)` 调用 `P011GuardLedgerRefund.refundLocal`，后者先 `share=amount/columns.length`，实际只对两端 **各退 m/2**。若跨列 partial reserve 按 rev1 整笔 WAIT，需要两端各退实际 reserved，直接调用该旧函数会留下 local 净扣。
- D1 保持 rev1：票据按实际已扣额双端完整退款，total 只退一次。**建议 architect/manager 明确 D2 接法**：新粒子票据 adapter 按各端实际扣额退款、total 仅退一次，旧通道行为保持；不要简单用双端列表原样复用旧 materialGuardRefund，也不要对 total 双退。
- 不受影响的六事务、池原子提交、双跑和 Node 自验继续；真实退款直接对照的差异如实保留，不通过改变测试或判据掩盖。

发送状态：2026-10-24 评论**尚未发送**。本会话工具列表没有 dida365；发现本地已有 OAuth 配置，未携带 OAuth 的直接 HTTP initialize 返回401，该结果**不能证明缓存授权失效**。随后通过已安装 CLI 的正式 app-server RPC 获取到已认证的 dida365 工具目录（含 add_comment/get_comment），但 tool/call 要求已加载的 thread context；复用当前 thread 被服务器以“already has an active writer”拒绝，未绕过该检查，未启动新会话或改动授权。待当前会话具备工具调用能力后补发。不得将本地记录当成任务卡已留言。
