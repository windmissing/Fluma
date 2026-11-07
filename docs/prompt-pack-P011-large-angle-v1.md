# P-011 大角度沙水路线：分角色任务 Prompt v1

日期：2026-10-23。原因：用户已批准外部姿态造成受控短暂超临界；需把尚未解决的目标口径、可达性、恢复与验证转为流水线任务。状态：**SUPERSEDED / NOT_FOR_DISPATCH / NOT_EXECUTED**。用户随后纠正：大角度方案仍需精读和离线可达性研究，本稿把 architect→developer→reviewer 提前排入接力，时机错误；这些下游 prompt 全部撤回，不据此派发。现行排序见 [改排 v2](prompt-pack-20261023-reordered-v2.md)。保留本稿仅供追溯，不代表下游设计、实现、采集或验收已获通过。

## 共同输入与不可越界项

- 读 [用户批准后的设计 v2](design-P011-large-angle-external-tilt-v2.md)、[大角度路线 v1](design-P011-large-angle-sand-water-v1.md)、[priority](priority.md) 当前阶段边界及 [.codex/PIPELINE.md](../.codex/PIPELINE.md)。
- 用户批准的是**外部姿态引起的短暂超临界**，并未批准材料 writer 新增/恶化超临界边，也未批准提高 `300/s` 预算、改 128 列、放宽守恒/容量/库存/有限值、下调 `30°±2°`、`target-based t90≤5.1s`、`R²≥0.90` 等阶段二目标。
- 现有 `p011` 旧 route、阶段一 CLOSED 及历史报告保留原身份；新路线建议 `p011-external-tilt-transient-v1`，未实现/未验证。所有结论注明适用的几何、初态、材料身份、预算和指标定义。
- 一个算术基线：128 列平沙达到**全宽线性 28° 沉积坡**时，若每笔跨邻边沙量都计入 total，一阶矩至少约 `92,917`，5.1 秒额度仅 `1,530`。此为条件性下界，既不能被忽略，也不能外推成所有局部观感或替代通道无解。

## A. 立即派发：manager（目标口径与调度）

> 你是 P-011 的 manager。读取本稿共同输入、`docs/goals.md`、`docs/priority.md`、最新 P-011 客观/主观报告及 `.codex/agents/manager.toml`。任务：把用户批准的“外部姿态可造成受控短暂超临界”登记为**新 route 的安全语义候选**，与旧 route 和阶段一 CLOSED 隔离。按最多三个高优先级问题组织本轮：① ±45° 目标的测量对象（局部崩塌坡面、全宽沉积面、raster OLS 各测什么；不得自行把冻结目标改成更易过的局部指标）；② 300/s 下全通道可达性与条件性下界；③ 超临界恢复及水沙同角输入的验证。先核对客观/主观用例；缺项先路由 tester/effector 写可复现用例，再进入实现。授权 designer 对“目标几何/预算/时间”列出保留原目标与候选修订的证据包。若证据显示现有目标必须变更，明确列出待用户裁决项，未经裁决不得改 `goals.md` 或冻结阈值。维护且仅维护 `docs/gap.md`、`docs/priority.md`，记录日期、依赖、每角色交付门和当前 `NOT_VALIDATED`。输出本轮排期与可并行项；不要写算法、测试断言或工程代码。

完成条件：priority 中能看出“目标定义与证明 → 用例预注册 → 算法复审 → 架构 → 实现 → 双报告”的门序，旧 route 与新 route 状态分开。

## B. 立即派发：designer（目标形状、可达性与算法）

> 你是 P-011 的 designer。读取共同输入、`docs/phase2-reachability-audit-P011-v1.md`、`docs/algorithm-P011-phase2-suspension-advection-v1.md`、`docs/algorithm-P011-S3-moisture-grading-v1.md`、`docs/design-P011-S4-deep-creep-bh-v1.md` 及最新阶段二目标。只做设计与离线证明，不改代码/测试。第一，分别形式化“局部崩塌坡”“全宽沉积线性坡”“raster OLS”测量对象，对每一个给出 ±45° 终点、像素可见性、一阶矩与跨边成本；保留现行冻结指标并清楚标出哪种解读是现行口径、哪种仅供 manager/用户裁决。第二，在 128 列、5.1 秒、total 300/s 下，对沉积、流动表层、悬浮 H/L、粒子以及水拖曳的**真实质量跨界**逐通道建账；给必要界、可构造见证或明确未证条件，不把垂直身份交换/渲染运动当成免费水平搬运。第三，把 v2 的外部姿态 `r0→rE`、writer `rE→rW`、逐边 non-worsening 与**复合相邻通量原子准入**写成伪代码和闭式准入式，覆盖正负角、邻边、墙边、分相库存、四层预算、preview/apply、TEST/EXPERIENCE 与反转生命周期。第四，给固定姿态超临界恢复曲线 `Emax/EΣ`、有效运动和停滞诊断的可测定义；恢复期限/最小进度如果缺证据则标 `PENDING_CALIBRATION`，不得拍数。第五，结合新增精读论文提出至少两条可比较的移动表层/静动交换候选，列相同预算下的行为差异与适用条件；若所有候选均无法覆盖原目标，出量化冲突单交 manager。把修订追加到 `docs/algorithm-P011.md` 或独立 P-011 算法修订稿，附复杂度、输入输出、伪代码、未解前置与设计状态；不宣称 VALIDATED。

完成条件：每一条效果主张均能指向物理库存、通量、预算与对应验收量；全宽下界没有被“粒子飞行”“水带走”“视觉层”绕账。

## C. 立即派发：tester（客观用例与 RED 基线）

> 你是 P-011 的 tester。读取共同输入、`docs/test-cases.md` 的 SLIP 用例和 `.codex/agents/tester.toml`。只维护自己的用例、测试代码与报告，不裁决目标变更。先把新 route 的客观测试预注册为版本化独立用例：平沙/有水 `0→+45°→保持 5.1s→0` 与负向镜像、方向反转、±15° 回归、无沙、零预算、库存边界、复合组使单边受阻的反例；TEST 构造倾角与 EXPERIENCE 传感器须通过同一物理角路径。冻结逐子步口径：`r0/rE/rW`、`poseAdded`、每笔 writer 前后受影响边、`Emax/EΣ`、有效通量、H/L/水守恒、local/bottom/channel/total 预算、preview/apply 身份、恢复窗口与失败原因。新 route 的 SLIP-01 必须区分**姿态引入**与**writer 引入/恶化**；旧 route 用例与历史结果不重解释；不得用 `SKIP_TAN30_PHASES` 代替来源分类。原 SLIP-02 的 `30°±2°`、target-based `t90≤5.1s`、`R²≥0.90`、末段稳定和悬沙终点仍单列测量；若目标形状口径有歧义，列出两套**诊断值**并提交 manager/用户裁决，不擅自选择更易通过者。新 route 未实现时执行可行的静态/现行构建 RED 基线，如实记 `NOT_IMPLEMENTED/BLOCKED`，不要伪造运行结果。写入 `docs/test-cases.md` 和按新 route 独立的 `docs/test-report-P011-*.md`，原始数据可追溯。

完成条件：任何新 route 安全 PASS 都能逐 writer 溯源；效果 FAIL 不被安全语义改动掩盖；用例在实现前已可执行或明确标明缺哪个遥测字段。

## D. 立即派发：effector（真实观感与目标参照）

> 你是 P-011 的 effector。读取共同输入、`docs/effect-cases.md`、现有目标视频/基线关键帧与 `.codex/agents/effector.toml`。设计并预注册真实使用场景：手机 `0→±45°→保持→回零/反转`，TEST 模式等价输入，干沙/湿沙/水下沙分组。分别描述并固定记录：水开始晃动的延迟与惯性、沙表层何时开始流、流动区域的宽度/前缘、局部崩塌是否看得见、全宽坡面是否形成、恢复后是否持续卡在超临界、颗粒是否只在画面动而物理床面不动。请把“局部可见崩塌”和“全宽 30° 收敛”作为**两个观察对象**，提供同设备、同窗口、同尺度的录像/关键帧与主观量表，不把前者替代后者。与 tester 共同提出可测的恢复窗口候选，但不要单方面冻结时限或修改 SLIP 硬阈值。新 route 未实现时用现有构建做基线，无法观察的项目记 `BLOCKED/NOT_IMPLEMENTED`。只维护 `docs/effect-cases.md`、效果 demo/素材和独立 route 报告；共享设备测试与 tester 错开。

完成条件：观感判定能区分“水动沙不动”“沙局部塌”“全宽终点达成”，每项都可由固定录像位置和时间轴复现。

## E. 上游齐备后派发：architect（工程化复审，不写代码）

> 你是 P-011 的 architect。前置：manager 已登记新 route，tester/effector 用例与 designer 算法稿已就绪。读取这些文档和现有 `WaterSandField.ets`、`P011MaterialGuard.ets`、`P011TiltPlanningPass.ets`、输入模式/水层代码。产出 `docs/architecture-P011-large-angle-external-tilt-v1.md`：routeId 隔离、默认关闭、TEST/EXPERIENCE 同源 `thetaGravity`、旧 planner 与新姿态提交的分支、原子快照/回滚、成组相邻通量的全边预演与一次准入、逐边真实计费、逐 writer guard、四层预算回写、H/L/水/移动层库存、telemetry 与 checker 数据契约；细化模块/接口/时序/复杂度/帧时风险/异常降级。说明当前 `waterLevel` 只能验证几何，何时需要 `h,hu` 惯性水模型；物理零 grant 时姿态与保守水运动如何提交且 run 效果不误判 PASS。逐项复核 300/s 的可达性账本，若算法与现有接口矛盾，退回 designer，不自改算法或阈值。无需真机，不授权 developer 开工。

完成条件：接口和事务边界精确到开发可实施，所有冻结不变量有对应检查点，且没有靠关闭 guard 来获得 45° 输入。

## F. 架构与测试门通过后派发：developer（实现）

> 你是 P-011 的 developer。前置：manager 接受算法/架构修订，tester/effector 已预注册用例，所有目标冲突已按权限裁决。仅按新 route 的算法/架构文档实现默认关闭的 `p011-external-tilt-transient-v1`；保留旧 route 行为和阶段一 CLOSED 回归。先实现可审计的输入来源/姿态与 writer 分账、原子成组通量和完整 ledger，后按架构门序实现水动量/可动沙层；每笔真实搬运仍过 guard 并按四层预算计费，不能为求图像效果跨过质量/预算。TEST 自构造倾角与 EXPERIENCE 传感器共用物理路径。完成后运行 hvigorw 构建、可用环境启动/模式验证及适用客观测试，交修改文件、构建记录、运行记录与失败项；发现算法/架构互相矛盾就停下反馈，不自改冻结阈值。

完成条件：构建、启动、旧 route 回归和新 route 安全用例可审计；效果目标未达则如实上报，不以代码实现等同通过。

## G. developer 交付后派发：reviewer（只读审核与提交）

> 你是 P-011 的 reviewer。前置：developer 给出构建、运行与测试证据。逐项比对新 route 代码与已批准算法/架构、版本化 test-cases：姿态与 writer 源分类、复合组原子性、受影响邻边 guard、分相库存、逐边/四层预算、preview/apply、TEST/EXPERIENCE、旧 route 隔离、回滚与日志。特别查“部分组已提交后失败”“粒子/水路免费搬运”“旧 SLIP 结果被新 route 追认”的风险。按阻断项给文件行号；通过后仅用 `.codex/PIPELINE.md` 指定的完整 Git 路径提交本轮代码与测试代码，不更改算法/架构/验收文档，不 push。

完成条件：通过则可追溯 commit；不通过则按严重度列最小整改清单，退 developer。

## 接力与状态

本文件**不启动任何 agent**。A–D 的文档分析/用例设计可按文件所有权协调；共享设备执行必须串行。E 等 A–D 的口径与算法交付；F 等 E 和测试先行门；G 等 F。之后 tester/effector 正式执行，designer 对双报告做阶段二路线判定，manager 决定是否 CLOSED 或重议目标。
