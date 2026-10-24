# P-011 D1 B 段开发检查点（rev1）

日期：2026-10-24。原因：用户明确 A 段已验收、B 段按 rev1 立即开工。**状态：IMPLEMENTATION_CHECKPOINT / NOT_ALL_GREEN / NOT_D1_ACCEPTED**。本记录不替代 manager 验收，不解锁 D2。

## 修改

- `P011ParticleField.ets`：补 Budget/Commit/AtomicPool 全接口；prepare 在独立池 scratch 准备整笔多相 patch，publish/restore 仅复制完整 SoA、空槽栈、nextId 与列缓存，无外部回调；stale/重复 patch 操作拒绝。新增事务成功后的 owner 收尾 `finishPatch`，外层子步 checkpoint/rollback 同时恢复已消费 key，确保失败后可重试。
- 池侧六事务：spawn 按质量包+精确尾数、确定性种子抖动与沙面/水面出生夹持；lift 双相池汇或同事务 SUSP 降级；deposit 仅减 granted、原 ID 留余量，state=2 重试；crossing 整粒 MOVE/WAIT、双端缓存；degrade 整粒扣池；merge 限同列同相同来源（并保接触状态），确定性最旧 ID 幸存、质量加权 x/y/v，跨列/跨来源不免费合并。
- 双跑：冻结视图与完整池快照，预报运动包含无事务的自由飞行；requests、motions 分别核验序列身份；两次 collect 完整计划相等后才 apply。deposit→crossing 全序不变；接触候选本子步不再 crossing，target 较高床面只限位、下子步接触。
- `P011ParticlePrototype.ets`：独立 Node 可替换的字段/预算存储，不导入 WaterSandField/renderer。复用真实 `p011GuardAdmit/ConstraintsForDelta/AffectedEdgesForDelta`、tan30 与现行 mixed→light→heavy split 公式；reserve 为 `min(l,l*scale,totalRemaining)`，单端/双端退款按实际票据扣额，bottom 恒0。成功后发布逐粒出生/合并与 LAND/LIFT/CROSS/DEGRADE 事件；失败恢复字段、池、预算、日志，失败 attempt 独立保留。完整计划外层故障同样回滚，允许原计划重试。
- `run_p011_particle_field_units.cjs`：A 12 例保留，新增 B 21 例。真实 ETS 单元包含 600s/36,000 子步物理沙盒、跨界容量与整粒预算拒绝、tan30 部分授权/退款、满池合并和降级、来源隔离、分母与300/s、shadow/live 同源对照、故障/日志溢出与整计划回滚。
- `P011ParticleM1Test.ets` 与 ohosTest `List.test.ets`：新增纯 M1 构造/余量 smoke 注册，强制新模块进入 ArkTS 测试目标的依赖图；未操作设备。

## 检查点证据

粒子 Node：**32 PASS / 1 FAIL**，失败唯一来自“直接复用旧双端 materialGuardRefund 的余额应与 rev1 桩相同”对照，详见下节；未删除或放宽该断言。600s/36,000 子步、全部池六事务与整计划回滚用例均通过。该长时测试是隔离字段/预算存储，不冒充 WaterSandField 全链或真实注入 λ/η。

既有通道：gesture 13/0、planning 106/0、S3 14/0、S4 10/0、input 124/0、advection 8/0，合计 **275/0**。与粒子合计 **307 PASS / 1 FAIL**，不得表述为全绿。

默认 HAP：hvigorw `--mode module -p module=entry@default -p properties.enableSignTask=false assembleHap --no-daemon`，**BUILD SUCCESSFUL / 18.607s / 零错误**。TypeScript strict/noEmit 检查通过。

额外 ohosTest 编译：第一次出现本卡3处 ArkTS 类型/对象视图问题，均已修复；复跑仍有 **175 个测试目标既有错误**，包括 HeightFieldTest 错误相对路径/嵌套函数、WaterSandFieldTest 未定义符号、P012FrameContext 缺字段；最新日志不再出现本卡 P011Particle* 文件错误。本轮不越权修旧测试，**不宣称 ohosTest HAP 构建成功**。日志 `.codex/local-test/d1-b-{units.json,build.log,ohos-build.log}`。

## 阻断与交接

1. **退款接法需上游明确**：rev1 要求 pair 两端各退 reserved，现有 `P011GuardLedgerRefund.refundLocal` 却把 amount 平分给传入 columns。reserve 与真实 clip 网格逐值一致；真实 pair 退款直接复用对照仍 FAIL。建议 D2 新票据 adapter 每端退实际扣额、total 只退一次，不改变旧通道。用户要求留言 D1 任务；正文已落本地 `feedback-P011-D1-refund-adapter-v1.md`，发送状态以该文件记录为准。
2. 本实现是隔离原型执行路径，prepare/collect/snapshot 仍使用 scratch 对象与数组分配，未达到原稿 §4.4 的生产热路径零分配目标；合并候选查找为有界池内搜索。**不宣称性能达标或生产热路径就绪**，需要继续收口复用缓冲/性能验证后才能完成 B 段验收。
3. 真实 λ/η/sourceEpoch、WaterSandField 的 guard/床层/susp/route/preview 适配与 checker v7 仍属 D2；本轮未改物理场接线、渲染、页面、冻结阈值或旧退款函数。用户明确真机端到端由其它智能体负责。
