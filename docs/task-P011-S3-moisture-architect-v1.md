# P-011 S3 湿度分级：architect 工程任务书 v1

日期：2026-10-23。发起：designer。状态：**READY_FOR_MANAGER_ROUTING / DESIGN_HANDOFF / NO_IMPLEMENTATION_AUTHORIZATION**。输入：[S3 设计稿](algorithm-P011-S3-moisture-grading-v1.md)、[双角用户配置设计](algorithm-P011-user-threshold-config-v1.md)、[现有 P-011 架构](architecture-P011.md)。本卡覆盖首版工程设计与高级版升级接口预留；产品终态须有局部渗透，但本卡**不要求首版实现渗透求解器**。

## 首版工程设计交付

| 模块 | architect 必答项 | 验收证据/阻断 |
|---|---|---|
| 逐列湿度场 | `N=128`、固定绝对高度格 K、占据掩码、`s[c,k]` 内存/快照/备份/重建归属；K=8/16 性能档如何选 | `moistureRoute=OFF` 旧路径逐步位精确；ON/default-reference 每边 26/20 同轨迹；无沙、满沙、沙顶跨格时无 NaN/旧格幽灵值 |
| 水量输入 | 现有 `inject(WATER)` 与拟新增产品加水/排水事务的边界；容量、余额、`initialWater`/守恒基准、UI“实际水量”口径 | 拒绝水操作不改变水、湿度或阈值；准入操作的水源/去向可追溯。不能将 `s` 作为第二份未扣 free-water 的水质量 |
| 代数刷新/搅拌 | `s_eq(z,q)`、shake 混合代理、`q_ref`、实际全水/全沙读取、子步开始冻结 `moistureRevision` 的两轮时序 | preview/apply 同一湿度版本；waterLevel 左右流动但总量不变时不假造全局湿度变化；正常 shake 不改真实水量 |
| 局部双角 | 有向边 donor 表层 `s_e`、`C/B` 映射、每边 start/stop 缓存及 active 生命周期；用户 pair 和 factory pair 的基准组合 | `0≤stop<start≤30` 与 `bottomCreepDeadZone<stop` 对所有有效 `s` 可证明；不能每子步 `configure()` 重置状态；湿度触发的 start/stop 与观测③原阈值标定事件隔离 |
| 五通道/安全 | 明确 relax 直接读取局部 pair，其余 settle/percolate/bottom/lift 的间接影响、共享预算与 §19.5 guard、§20 planner 接口 | tan30 常数、300/s 及其它既有额度、H/L/水量守恒零退化；阈值只改变请求，guard 对全部真实 writer 逐笔保持 |
| 验收/遥测 | `moistureModelVersion/q/q_ref/sRange/edgeThresholdRange/thresholdCause` 字段、OFF/默认/不同水量分组、帧时门槛沿现行测试流程 | 旧 26/20 冻结锚点只在 OFF 或 ON/default-reference 判；三水量档效果是候选待冻结，不以颜色差或无内力的“成团”主观印象直接判物理通过 |

## 高级版必须预留的接口

1. 同一 `(column,heightBin)` 寻址与 `saturationAt/edgeMobileSaturation` 只读调用约定；首版代数刷新算子将来可换成局部通量更新，不迁移消费者接口。
2. `P_pore/W_pore`、free↔pore 质量交换、重力排水/毛细/可选侧向通量的独立账本插槽；**仅预留类型和版本边界**，具体有限体积算子、参数与稳定性属于 `s3-local-seepage-v2` 后续设计。
3. 首版经验 `s` 转高级版真实孔隙水的版本迁移事务：必须从现有物理水量扣出 `W_pore`，不足时有拒绝或缩放回执；不得凭首版代理湿度创造水。
4. 材料 writer 改变占据格时的高级版孔隙水携带/挤出接口，及悬沙是否携水的独立裁决点；S4/方案 3 与 S3 同开时按真实物理库存和 routeId 隔离。
5. 高级版局部渗透使阈值在局部变化时，仍保留 `s→C/B→θstart/stop→planner/guard` 契约；原 26/20 与湿度参数的标定/观察协议必须新版本，不追认旧观测③ BLOCKED 数据。

工程文档建议追加 `docs/architecture-P011.md` 的 S3 专节：字段归属表、更新时序图、默认位精确通路、局部 pair 与 shadow 镜像、水量事务与高级版水账迁移边界、复杂度和真机帧时预算。交 manager 复审后再路由 tester/effector/developer；当前卡不授权实现或采集。
