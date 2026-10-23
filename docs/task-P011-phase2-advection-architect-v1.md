# P-011 阶段二方案 3：architect 工程任务书 v1

日期：2026-10-23。发起：designer。状态：**READY_FOR_MANAGER_ROUTING / DESIGN_HANDOFF / NO_IMPLEMENTATION_AUTHORIZATION**。输入：[悬浮平流算法设计](algorithm-P011-phase2-suspension-advection-v1.md)、[可达性审计](phase2-reachability-audit-P011-v1.md)、[架构 §28 前置清单](architecture-P011.md)、[S4 深层设计](design-P011-S4-deep-creep-bh-v1.md)。由 manager 调度 architect；本任务书本身不替代 `architecture-P011.md` 的工程复审，不授权 developer 或真机。

## 任务边界与待决定基底

工程设计对象是 `suspension-advection-v1` 的列标量深度平均相内输运，保留 N=128、现有扩散、守恒、tan30 guard、planner 与全部预算上限。主分支按自然 relax，无 correction 快车道；若今晚裁决 correction 转正，architect 必须给出共用 total/预留账重算附录，不得悄悄把旧矩阵结果外推。P-3 起沙来源需 manager 独立批准；architect 可先完成平流 a 的接口设计，但不得把旧 shake 在两个零悬沙 legacy 用例中写成已可用来源。

| 编号 | 工程设计交付 | 必须回答的接口与验收证据 |
|---|---|---|
| **P-2 通道账本** | 设计独立 `advection` channelId/routeId、逐相逐边 requested/granted、通道速率 cap、双端 local、shared total 的扣费和 clip 原因；bottom 不扣。把 `advectionGain=0` 定义为位精确 OFF 路径。 | 列出 preview/request 收集、snapshot、apply 的同序伪代码；每笔 `q_H+q_L` 如何进 300·dt 分母和四层 `used/remaining`；多出边同供体如何防超卖；通道/total/local clip 后逐层回写；无隐式第二额度。与旧 `diffuseSuspension` 的零新增预算语义分隔。 |
| **P-4 全链 shadow 证书** | 在批准的 P-3 来源及 P-1 退款修复后，定义从 accepted θ 到 source→advection→settle 的可复现逐笔轨迹导出。 | 证书含 run/seed/初态、planner 接受角、每相库存和 `Mdep/Msusp`、最紧边、guard/预算逐层扣退、通道竞争、tail 悬沙率、稳定性和最终形状；preview/apply 逐 grant 一致。先复核 16 列 `q=.01` 条件构造，再审 128 列与 5.1 秒；报告必要上界与真实可提交链之间差距，不将条件包写成产品可达。 |
| **P-5 分域遥测** | 定义向后兼容 ledger/snapshot/checker 字段；`Mdep,H/L` 和 `Msusp,H/L` 独立，逐相守恒与矩闭合。 | 表明旧 diffusion、既有 bottom/relax/percolate、S4（若启用）各由谁记 signed edge flux；起/落同列对 `Mtotal` 净零；给 `R_M,H/L≈0` 的计算式、浮点容差来源与未升级旧 run 的缺字段语义。性能估计 O(N) 与日志开关/批量写入策略。 |
| **P-6 guard 语义** | 将 advection 注册为 `ΔS=0 / S-invariant` writer；source、settle 保持逐笔 §19.5 数值 guard。 | 说明预演受影响边集如何登记为空、为何 `d_e` 数值不变；仍如何核对正库存、`S+U≤waterTop≤capacity`、预算及 BLOCKED；`updateTilt` 继续只由 §20 可行域负责。禁止“平流被 guard 豁免所以整步安全”的措辞。 |

## 依赖顺序与停止条件

1. **P-1 前置核验**：developer 按 §28 修复 bottom guard 退未扣 local 的潜伏路径，并给构造单测；否则 P-4 正式可达证书挂起。工程方案可以先出，但不能以虚增额度作证。
2. **P-3 独立来源审批**：designer/manager 决定表层活动起沙的触发、库存来源和分相；若只批 a，P-4 效果证书限“已有悬沙”域，KF-1/KF-2 仍无新增通量。厚层 heavy 的深层来源不包含在本任务内。
3. **correction 基底裁决**：记录 `ARCHIVED` 或 `PROMOTED` 的明确 routeId；任何未裁决状态不得把 advection 插进 correction 批的统计结论。S4 如同批规划，按实际两相 `2q` 扣费，与 advection 不共享免费额度。
4. **止损**：preview/apply 不一致、`used+remaining` 任一层不闭合、source 无真实 grant、`M` 账不闭合、guard/planner 不安全，或在冻结约束下 shadow 全链不可提交，标对应 `BLOCKED` 并退 designer/manager；不调整 300/s、tan30、阈值或原 legacy 用例求绿。

architect 交付建议：在 `docs/architecture-P011.md` 追加方案 3 独立节，包含字段表、两轮时序图、接口签名、预算回写表、guard 分类、生命周期、shadow 证书格式、复杂度与帧时预算、P-1/P-3/correction/S4 分支条件，并提交 manager 复审。完成工程设计后再由 manager 排实现与独立测试/效果回执。
