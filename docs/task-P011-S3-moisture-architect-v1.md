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

## 2026-10-23 产品三维候选的评估交接（用户建议，未裁决）

按 [S3 设计 §7–8](algorithm-P011-S3-moisture-grading-v1.md) **比较**分层高度场与“高度场主体+局部 3D 体素”；用户仅提出后者作为建议，designer 当前推荐前者优先，**未授权将体素定为高级版基底**。首版 1D `s[c,k]` 风险对冲排期不因此宣称具备三维渗透。若经理选择局部体素候选，architect 另需给出：

1. 当前 128 列一维场如何过渡到 `S[x,y]` 二维水平高度场；`Ny=1` 的守恒/行为兼容边界与前后 `y` 方向真实库存何时建立。若未建立 `y` 库存，不得将局部体素称为物理 3D。
2. 活动孔隙水砖的 `W_fine/P_fine` 与非活动高度场区的 `W_coarse` 唯一所有权、砖块激活/回收时守恒细分与聚合、上下/侧向/粗细交界的同笔通量；free↔pore 交换和 snapshot/checker 字段。
3. 固体沙仍由高度场拥有的首批适用域；只有高度场不能表示的多值几何另设计固体体素所有权转移，禁止 `heightfield+voxel` 双算一份重/轻沙。
4. 误差驱动的局部体素触发/关闭与不足分辨率诊断；活动块数、内存（含快照/临时缓冲）、步时和帧时上限由真机数据定档。给 1D、2D 高度场、单块、多块四级验证门与 routeId 隔离。

优先提交的高级版工程比较应先给 `S[x]+W[x,k]`、必要时 `S[x,y]+W[x,y,k]` 的分层高度场方案：水量守恒、竖向/横向通量、每列层数、误差与设备帧时；再明确哪些冻结场景是它无法表示且局部 3D 体素能够解决的。上述体素条目是**条件审核清单**，并非首版或高级版的实施授权。manager 可据比较分别排 2D 底座与局部 3D 水账的设计/实现评审。

## 2026-10-23 摇晃三工况效果增补：请 architect 复审

用户明确要求干沙、潮湿沙、被水淹没的沙在摇晃时效果不同；[S3 设计 §9](algorithm-P011-S3-moisture-grading-v1.md)已登记三工况的介质身份、请求律、在途库存和守恒语义。现有 S3 架构只覆盖湿度→双角，`liftByShake` 仍把三工况写进同一 `suspH/suspL`，故其 `READY_FOR_DEVELOPER` 状态**仅适用于原双角范围**，不覆盖摇晃三工况。请在实施三工况前补充工程复审：①自由水与局部饱和度双输入及子步冻结；②有限空气在途批次、潮湿团簇、水中悬沙的库存/snapshot/checker 所有权；③`lift/落回/跨水面` 的预算与逐笔 guard 顺序；④与掉沙路线 A、方案 3 水下 advection、P-012 渲染和 TEST/EXPERIENCE 输入的接口；⑤真机粒子/批次上限及退化回执。未复审前不得把干沙写入水中悬沙的现状认作已满足新需求；本增补不授权代码修改或新采集。

**颗粒轨迹追问补充**：按 S3 §9.4，将待复审的②扩为“空气粒子/潮湿团簇/水中可见粒子/水中未解析细沙”四类唯一库存；将④改按掉沙路线 C 的稀疏真实颗粒层与高度场耦合评估，路线 A 仅为批次物理原型。须证明离床、跨水面、撞床及粒子池满额时每一相质量只有一个所有者，并给出有界粒子数的真机帧时证据；此补充不构成经理选路或实施授权。
