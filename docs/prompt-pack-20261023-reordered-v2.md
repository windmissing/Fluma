# 2026-10-23 流水线改排 Prompt v2：先验证已落地任务，大角度只做调研

日期：2026-10-23。原因：用户指出[旧 Prompt 包](prompt-pack-P011-large-angle-v1.md)把尚未成熟的大角度方案排进完整实现链。状态：**SUPERSEDED / NOT_FOR_DISPATCH / NO_AGENT_LAUNCHED**。本稿又误将用户已明确放弃的 correction 路线安排正式提数；该任务撤销，封存数据只作历史资产，不以“已有数据”构成继续投入理由。现行改排见 [v3](prompt-pack-20261023-reordered-v3.md)。保留本稿供追溯；各角色仍遵守 [.codex/PIPELINE.md](../.codex/PIPELINE.md) 与 `.codex/agents/*.toml` 的文件所有权。

## 已核对的现状与取舍

| 主题 | 当前证据 | 现在应走到哪里 |
|---|---|---|
| 大角度沙水 | [外部倾角 v2](design-P011-large-angle-external-tilt-v2.md)仅批准暂时超角语义；全宽线性坡在原邻边计费下约 60.7 倍条件性预算缺口，静动交换/新恢复律仍待精读与证明 | **RESEARCH_ONLY**：designer 消化论文、离线可达性账本；不派 architect/developer/reviewer，也不改 SLIP checker |
| P-012 E3 帧时 | [真机窗口回执 §2](window-receipt-5in1-20261023-v1.md)：s134 settle/tilt 约 9–10fps；`60d24bf` 已修 E3 q dirty 失效，但尚无同口径真机复测回执 | **优先验证修复**：tester 客观配对复测；性能过门后 effector 重录掉沙 V1–V4/E2 观感 |
| P-011 correction | [82/108 run 封存回执](corr-matrix-formal-108-closure-receipt-P011-v1.md)：21 对 D/A、20 完整 ABCD block；G 档不能当独立场景；数据移交 tester 未提取正式判定 | **立即离线提数**：tester 按封存统计单位产区间；designer 再判定归档/候选，不补采 26 run |
| P-011 S3 湿度首版 | [算法稿](algorithm-P011-S3-moisture-grading-v1.md)+[架构稿](architecture-P011-S3-moisture-v1.md)已具备，`01cc503` 已入库首版实现；工作区另有未提交修改 | **先做独立回归**：tester 验 OFF 位精确、ON 默认等价、非默认局部 pair；effector 后测可见差异。三工况摇晃与渗透仍为后续设计，不能连带宣称完成 |
| 方案 3 advection | [架构稿](architecture-P011-advection-v1.md)只对 a 段“搬已有悬沙” READY_FOR_DEVELOPER；b 段起沙链仍等 P-3 manager 批 | 可列下一波；不能把 a 段说成解决零悬沙大角度 |

## 现在可复制的 Prompt

### 1. manager：纠正排期与文档状态（先发）

> 你是本项目 manager。读取本稿、`docs/priority.md`、`docs/gap.md`、`docs/window-receipt-5in1-20261023-v1.md`、`docs/corr-matrix-formal-108-closure-receipt-P011-v1.md`、`docs/algorithm-P011-S3-moisture-grading-v1.md` 与 `.codex/agents/manager.toml`。用户已纠正排期：大角度方案尚需论文精读、目标形状定义和预算可达性证明，旧 prompt 包的 architect/developer/reviewer 接力撤回。请在你独占的 `priority.md/gap.md` 登记大角度 `RESEARCH_ONLY / NO_IMPLEMENTATION_ROUTE`，不把用户批准“短暂超角语义”写成完整算法批准。此轮最多安排三项已有证据基础的任务：① P-012 E3 `60d24bf` 帧时修复同口径真机复测；② P-011 correction 已封存 82 run 离线正式提数；③ P-011 S3 湿度首版独立回归。为每项写入负责角色、输入、产出、文件所有权与共享设备先后；测试先行并标注当前用例是否齐全。P-011 advection a 段、S4 主线集成、掉沙 V1–V4/E2 重录放在前置满足后的下一波。旧阶段一 CLOSED、冻结安全与阶段二 ±45° 效果目标不改。若目标测量对象仍有歧义，只登记待裁决问题，不自行调低指标。

完成条件：priority 能明确哪些是**已有实现待验证**、哪些仅**设计/研究**，不会把大角度送进开发。

### 2. tester：correction 82-run 封存提数（可立即离线）

> 你是 P-011 tester。只读取 `test/artifacts/p011-corr-matrix-formal-108-20261022/` 与 `docs/corr-matrix-formal-108-closure-receipt-P011-v1.md`、预注册矩阵口径及现有分析脚本；不采新 run，不改业务代码。先校验 `CLOSURE.json`、run 身份、完整 block 与哨兵。D/A 主比较限 21 个同 block 配对；涉及 B/C 的比较限 20 个 ABCD 完整 block；G1/G2/G0 不能当三种独立 working state，G2 稳健性与 G0 控制本批不可判。提取每臂 E/maxResidual/ΔE_relax/挤占代价/CORRECTION_STALLED、守恒/预算/guard、性能与对应区间，统计方法严格照已注册口径；不足样本或未定义指标写 `INCOMPLETE`。单列 61/82 个贴 tan30 界的安全余量分布，不把引擎容差内贴界当违规，也不隐瞒接近边界的风险。报告写到对应 P-011 correction test-report，附可追溯脚本/输入 hash；不得写 `VALIDATED`，归档或转正由 designer/manager 后续裁决。

完成条件：所有比较的分母、配对关系、CI、失效预注册目标和原始记录链接可核对。

### 3. tester → effector：E3 修复配对复测与掉沙重录（共享设备串行）

> 你是 P-012 tester。输入 `docs/window-receipt-5in1-20261023-v1.md` §2、`docs/fps-seg3-P012-v0.md`、修复 commit `60d24bf` 和 `test/artifacts/p012-measurements/seg3-s134-*`。先确认当前构建源码身份及工作区脏改动是否进入包；用同设备、同热态分层、同 settle/tilt/heavy 窗口，配对复测修复前 s134 与修复后构建的 presented frame/FPS 和 phys/render/present P50/P95。判定 E3 q dirty 短路是否恢复、s133..HEAD 的 dyn 回归是否仍在；区分热降频与构建差异。冻结 E3 已通过的 J95/J90/std 锚点要做不退化核验，但不要把帧时修复自动写成 E3 全量 VALIDATED。原始 trace、包 hash、构建 commit、设备温度与报告落测试资产。若仍约 9–10fps，报 `PERF_BLOCKED` 并给可定位的最差窗口，停止让效能受污染的视觉证据用于裁决。

> 你是 P-011/P-012 effector。仅在 tester 确认掉沙效果录制所需帧时已恢复后，占用同一真机，按 `docs/window-receipt-5in1-20261023-v1.md` §3 的 V1–V4 固定场景重录；记录喷流可见性、方向、落点与真实沉降错时，NO_WATER/CAPACITY_BLOCKED 的视觉回执按已准入 receipt 核对。此前 9–10fps 批次保持“证据不足”，不得追认为通过；E2 原型真机对照可安排后续独立窗口。维护自己的 effect-cases/报告/素材，不改客观阈值。

完成条件：先有可靠帧时复测，再有可判读的主观帧序列；一台设备测试执行严格串行。

### 4. tester：S3 首版独立回归（可离线准备；设备排在 E3 后）

> 你是 P-011 tester。读取 `docs/algorithm-P011-S3-moisture-grading-v1.md` 首版范围、`docs/architecture-P011-S3-moisture-v1.md`、实现 commit `01cc503` 与现有 S3 测试资产；核对当前工作区脏改动并明确报告所测源码身份。先补/运行首版客观用例：`moistureRoute=OFF` 旧轨迹位精确；ON/default-reference 与 26/20 旧行为全轨迹等价；不同全局水量与高度格给出局部饱和度及 start/stop 输入变化；`0≤stop<start≤30`、tan30 guard、300/s、H/L/水账、preview/apply、reset/模式切换与有限值不退化。报告只判首版湿度→双角；局部渗透求解器、干/潮/水下摇晃三工况及粒子轨迹标未实现/待架构复审，不能因首版通过而追认它们。测试代码与报告归 tester，发现实现差异移交 developer，不改算法/目标。

完成条件：首版适用域与未实现高级版边界在报告里分明，所有数值绑定实际 commit/源码 hash。

## 大角度调研专用 Prompt（不进入本轮实现队列）

> 你是 P-011 designer。用户只批准外部姿态造成受控短暂超临界，**未批准完整大角度机制**。等外部智能体精读 Douady 等静止/流动层交换、Parez 等倾角阶跃非稳态及必要的滞回/非局部论文后，核对原文方程和适用条件；先在本地做条件性可达性账本：128 列、300/s、5.1 秒下分别计算局部坡、全宽沉积坡、raster OLS 与流动层/悬浮/粒子身份的跨界成本；给 0→±45° 固定姿态的前几个子步和 1s/5.1s 预测，分清姿态引起超角与材料 writer 改形。至少比较两种候选，若 5.1 秒目标仍无可提交构造，出 `UNRESOLVED_CONSTRAINT_CONFLICT` 报 manager。只追加 P-011 设计/复盘文档，不改代码、测试用例、priority，也不交 architect 实施。

完成条件：有原文映射、可复核通量/预算/库存证明与尚待裁决项；**没有**“论文说可行”式代替工程证书。

## 下一波而非本轮

- P-011 advection a 段：架构已 READY_FOR_DEVELOPER，但只搬已有悬沙；先确认 tester 用例、manager 排期与当前 S3 工作区隔离。b 段起沙需 P-3 独立批准。
- P-011 S4：离线原型与 sweep 有候选参数，主线合入和 S4×C 效果受 correction 终审/界面来源约束；不要把“平界面自发造波”当验收目标。
- P-012 参数面板与 P-011 手势注沙已有代码，不重复派开发；掉沙 V1–V4 效果受帧时回归阻断，待 E3 修复复测后再取证。
- P-013 换栈/E2 可保留立项与原型并行，但 s134 帧时回归未清前不拿其 FPS 推算换栈收益。
