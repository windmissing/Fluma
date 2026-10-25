# Gap 清单（目标 vs 现状）

## 2026-10-24 大角度新 route 登记（p011-external-tilt-transient-v1，当前生效）

> 依据：用户已批准「外部姿态可造成受控短暂超临界」为新 route 的安全语义候选。登记明细见 `docs/priority.md` 同日同名章节。本节只记目标与现状的差距，不启动实现。

| 项 | 内容 |
|---|---|
| 差距项 | 全宽 ±45° 沉积坡目标在 300/s 预算下不可达（全宽线性 28° 沉积坡一阶矩至少约 92,917，5.1 秒额度仅 1,530——条件性下界） |
| 现状 | 旧 route（`natural-relax-hysteresis-v1`）与阶段一 CLOSED 不覆盖该目标；新 route `p011-external-tilt-transient-v1` 状态 `NOT_VALIDATED / NOT_IMPLEMENTED`，安全语义仍为候选 |
| 目标 | 外部姿态输入下可控的短暂超临界，且不追认旧 route 结果、不放宽冻结阈值 |
| 量级 | 条件性下界，不是「所有局部观感不可达」的结论，也不得外推为替代通道无解 |
| 路由建议 | 按 A-G 门序推进：目标定义与证明 → 用例预注册（客观/主观）→ 算法复审 → 工程化复审 → 默认关闭实现 → 只读审核 → 阶段二路线判定。A-G 七卡当前整体 P3、B 卡暂停，三件事窗口后恢复 |
| 待用户裁决 | ①±45° 目标的测量对象（局部崩塌坡面 / 全宽沉积面 / raster OLS）②300/s 全通道可达性与条件性下界 ③超临界恢复与水沙同角输入验证；未裁决前不改 `docs/goals.md` 与冻结阈值 |

## 2026-10-23 P-003/P-005 范围分阶段裁决（当前生效）

> 用户裁决：场景二先做水 + 一种沙；暂不考虑双沙/三相。完整 G2 最终目标仍保留。此裁决覆盖本文件早期将 P-003/P-005 直接描述为三相实现的当前排期范围，不改变历史记录。

- **P-005（P0，测试前置）**：tester/effector 本轮只补单沙+水的客观/主观用例及 RED 基线；三相分选、重/轻沙比例、相间 percolation 指标留后续阶段。
- **P-003（P0，单沙+水第一阶段）**：实现水和一种沙的相内沉降、沉积/悬浮转换、堆积与动态响应；分别保证水总量、沙总量守恒。悬浮与沉积属于同一沙相的位置/状态，不视作第三相。不得实现第二种沙或重/轻分选。
- **后续阶段**：第一阶段客观/主观验收后，manager 再决定何时启用双沙/三相与 P-008；在此之前标记 `DEFERRED`，不宣称完整 G2 达成。
- **依赖**：P-005 用例/基线 → designer 更新单沙算法 → architect 更新工程方案 → developer 实现 → reviewer 自动审核 → tester/effector 复测 → designer 判定。P-009 可并行。

## 渲染现状 vs G2 目标差距（2026-10-21）

> 维护人：manager。依据 `docs/goals.md` G2/G2.7、P-012 第一梯队签收与 CLOSED 范围、`docs/effect-report-P012-v1.md`、`docs/test-report-P012-v1.md`、`docs/render-gap-analysis-P012-v1.md`。排序按用户要求：先效果、后效率。本节为差距盘点与排期建议，不启动实现。

| 优先级 | 差距项 | 当前现状 | 目标 | 差距量级 | 依赖 / 路由建议 |
|---|---|---|---|---|---|
| E1 | C 层理 / D 流痕 | 两项均 `UNLOCKED / NOT_VALIDATED`。已有主观差距材料指出沉积层理映射和流动方向纹理不足；本轮没有对应的 C/D 验收结论。 | 目标帧可读的沙纹层理与流动沙丘流痕。 | 质性差距，暂无对应量化验收基线；不能以 E/A/B CLOSED 推断达标。 | 按 **C→D** 顺序排期。先由 designer 定义可观测效果与验收锚点，再分别安排实现/效果验证；D 依赖动态流痕表达，排在 C 之后。 |
| E2 | 顶光立体感 | 当前目标帧评测指出沙丘受光面/背光面的整体明暗塑形尚未实现；光照项未单独开题。 | 沙丘受光面与背光面形成可读的体积感，符合目标帧柔和顶光观感。 | 新渲染维度，缺少独立指标与基线，暂不能量化倍数或像素差。 | 建议作为渲染新维度开题，由 designer 先界定表现与可测指标，再路由实现和主观复核；与 C/D 协调资源，维持效果优先。 |
| E3 | **网格感消除：雾带块状 → 连续悬浮感**（2026-10-21 用户反馈升级：网格感是观感差距主体之一，非锦上添花） | SV-06=3（可接受档）；用户体感确认「渲染的网格感很重」。**三来源拆解（代码核实）**：① 128 列逐列计算+按列绘制 → 相邻列阶梯差，斜视/流动时竖向条纹（雾带实测 800px 内 138 色段、直角转折）② 64px 噪声 tile（雾/亮粒）平铺 → 周期性重复纹样 ③ 256px 颗粒位图平铺 → 颗粒带重复感（较轻）。物理场 128 列分辨率本身被渲染层平滑曲线插值，「仿真网格感不重」与此一致——网格感出自呈现方式非物理分辨率。 | 雾带连续无块状转折、tile 纹样不可感知、整体接近目标帧「连续悬浮云雾」。 | 块尺度约 10–30px；定量提升门槛待 designer 定义（可与 Q1 新尺子协同：显示尺度统一后量化块状 vs 参考带的差距）。 | 修复方向：① 列间插值或提高渲染列密度（跨列平滑）② 噪声 tile 随机化/去周期化 ③ 颗粒 tile 多变体轮换。**须与帧时联合评估**（34~60fps 基础上加插值/加密会增成本，F1 100fps 归因时应合并考虑）。排 C/D 之后、100fps 之前。不得把 SV-06=3 改写成未通过。 |
| E4 | 沙丘形态 / 画面占比 | 当前 EAB 测试场景沙层较薄、雾带偏厚；目标帧沙丘约占画面 2/3。差异归因涉及场景初态，不应直接归因于渲染算法。 | 沙丘形态及画面占比接近目标关键帧（沙丘约 2/3 画面）。 | 当前比例与目标有明显视觉差距；缺少同构图量测，暂不写精确百分点。 | 依赖场景初态与物理分选。排期时与 P-011 观测③标定链协调，但不改变 P-011 当前裁决、状态或验收口径；由 designer/effector 明确场景和测量方案后再安排。 |
| F1 | 帧率 `fpsP95≥100` | tier3 当前报告 34–60fps；第一梯队按其既定窗口标准已通过并 CLOSED，但这不等于达到 G2.7 的 `fpsP95≥100`。 | G2.7 硬指标：`fpsP95≥100`，并满足动态连续、无瞬移等要求。 | 按当前 34–60fps 对比 100fps，约差 1.7–2.9 倍（约 2–3 倍）；窗口口径须由 developer 归因时与 G2.7 口径对齐。 | **效率项排在效果项之后。**建议先排 developer 做可达性/瓶颈归因，检查 Canvas2D 逐帧绘制及离屏预合成分层、部分硬件加速等架构方向是否必要，再据证据决定升级 P-013 级议题；此处不预判可达性，也不授权架构改造。 |

本盘点不改变任何已冻结口径、已 CLOSED 项或其历史结论；第一梯队 E/A/B 继续保持原状态，C/D 继续 `UNLOCKED / NOT_VALIDATED`。100fps 项目前仅是路由建议，能否达到目标由 developer 完成归因后报告，不作预先结论。

## 2026-10-21 P-011 D-1 correction 越权激活（manager 裁决）

> 日期：2026-10-21。依据 `docs/architecture-P011.md` §26.2。裁决：Option A 加显式路线隔离；维护人：manager。

- **D-1 `OPEN / BLOCKING_FOR_V3_TESTER_SIGNOFF`**：LEGACY 和普通 `natural-relax-hysteresis-v1` 必须走旧 relax apply；仅 thresholds 非空不够启用 correction，另须显式 `relax-correction-weighted-v1` route 身份。不得把 correction 追认为默认行为或混进普通 v3/SLIP/观测①b run。
- **developer → architect → tester**：developer 加资格门、恢复旧路径并补跨版本 240-step 轨迹测试（覆盖 step25、step214）；architect 审查 route/实现边界；tester 在修复构建上重跑 legacy 等价和 v3 身份核验。6ad709d6 仅代表此前被测构建的结果，修复后必须重新验证。
- **D-1b `OPEN / BLOCKING_FOR_CORRECTION_VALIDATION`**：correction audit 字段尚缺。补齐 `correctionRequested/Granted/Rejected/StalledCount` 并由 tester 核验账本前，不得执行或报告 correction 路线正式效果验证。
- **correction 路线状态**：`CANDIDATE / NOT_PROMOTED`。后续按 architecture §24.3 完成隔离的 A/B/C/D 对照矩阵与效果报告，再由 manager 裁决是否转正；不沿用/拼接 LEGACY 或 `natural-relax-hysteresis-v1` 历史 run。
- **不变量**：本次裁决只恢复行为隔离与授权顺序，不修改 tan30、预算、阈值、状态机安全门或历史证据。

## 2026-10-20 P-011 观测③平台可达性矛盾裁决（当前生效）

> 日期：2026-10-20。依据 `docs/effect-report-P011-recalib3-gate-run1.md` 与 designer §8.16。维护人：manager。裁决：采用方案 3，重定义观测③为 ±15°/±30° 可达平台标定，±45° 留待阶段二；批准单 run clean pilot，不批准后续扩批。

- **安全基线已验证**：835e663 gate run 阶段一 MUST-PASS 全部 PASS（164,084 边 tan30 违约 0、A-1 对账不匹配 0、账本错位 0、边身份错位 0）；run 因 deferred SLIP-02 被 checker 汇总为 FAIL 不改变这些分项 PASS。阈值标定仍未通过/未完成。
- **结构性缺口**：planning pass 将运行时 accepted tilt 限制在约 ±29.9°，冻结 ±45° 协议不能形成要求的平台/bracket。保留 planning guard 与 tan30 安全 MUST-PASS；明确拒绝在标定 run 添加 capture-mode bypass。
- **designer 结论已复核**：±15°不能独自激励 `thetaStart∈[25°,30°)`；±30°可能触及搜索窗低段，约29.9° accepted ceiling 会使未触发的高端候选右删失。右删失样本必须标 `RIGHT_CENSORED_AT_TAN30`，不产生点估计/校准 PASS；真实 stop 必须是同边连续 bracket 上的 active true→false，hold、拒绝请求、限流、run-end 均不算 stop。
- **后续路由**：effector 发布全新 protocol/schedule 并重新预注册（不得复用旧 34-run 分层/随机化）→ tester 同步 checker/cases、验证平台稳定与 bracket/删失识别 → 仅运行单 run clean pilot。manager 审查 pilot 的平台/bracket yield 后另行决定是否扩批。
- **样本与状态**：旧 ±45° protocol 的 run-001 证据保留但不计入新 protocol；观测③整体保持 `BLOCKED（尚无合格 bracket/阈值估计）`，仅 clean pilot 获授权，fit/holdout 扩批仍冻结。阶段一安全结果独立记为 PASS；SLIP-02 ±45° 目标仍 `DEFERRED / NOT VALIDATED`，归阶段二。

## 2026-10-20 P-011 阶段一·B commit 4b1f635 评审裁决（当前生效）

> 日期：2026-10-20。依据 `docs/architecture-P011.md` §22。维护人：manager。

- **B 实现**：`CONDITIONAL_PASS`，七项限幅 checklist 全 PASS；倾角可行域闸门 + preview/apply 镜像一致 + 子步核验获接受为当前已交付保护。其范围不包含 §19.5 全通道逐 writer 拦截。
- **B·续（OPEN / IN_PROGRESS）**：补齐全通道 pre-commit 材料 guard、违规态 non-worsening、四层预算回写，以及 `hardGuardAvailable` telemetry/checker。与 A/C 验证并行执行，但必须在阶段一总验收前完成、评审并进入集成安全验证。
- **legacy FAIL（KNOWN_FAIL / DEFERRED_TO_PHASE_2）**：`bottomCreep_tiltMovesLowerLayers` 与 `p011_commandSignFlip...` 是新闸门暴露的既有 tan30 违约，不认定为 commit 4b1f635 回归。阶段一维持其文档化 FAIL，作为已知 legacy 例外单独列示；不放宽 tan30 判据、不改场景参数/测试期望、不删除失败证据。阶段二由 tester/designer 结合 B·续及候选机制复核并重测，处理完成前不能声称这两项已解决。
- **总体验收语义**：本裁决只允许将【阶段一·B】实现标记为条件性完成，不自动放行阶段一整体完成；B·续仍是阶段一总验收 gate。已知 legacy FAIL 不阻断 B 实现完成，但须在总验收报告中明示并保留原始失败。
- **路由**：developer 实现 B·续；architect 复审；tester 同步用例/checker 并验证逐 writer 拦截、各 writer 影响边、四层预算回写及 telemetry。三线与 A/C 验证并行，B·续完成后再做阶段一集成验收。

## 2026-10-20 P-011 阶段边界复议裁决（当前生效）

> 日期：2026-10-20。依据 designer `docs/review-basis-P011-stage-boundary-v1.md`、`docs/algorithm-P011.md` §8.15。维护人：manager。状态：`IN_PROGRESS`。

- **阶段一·A（状态机/标定）**：完成逐边 start/hold/stop、限流语义、边身份、遥测对齐及原始 fit/holdout/CI/样本门槛。实测阈值和置信区间照常报告；`thetaStop` 对 ±45° 全局目标窗的映射检查推迟阶段二。标定③因合格 run 与样本不足仍 `BLOCKED`。
- **阶段一·B（安全）**：所有实际提交角度的逐边 tan30 约束、可行事务提交/不可行时 hold 或 BLOCKED、守恒/库存/容量/预算和账本对齐仍为 MUST-PASS。不得以未追随到 ±45° 为由判阶段一安全失败，也不得将 hold 当作收敛通过。
- **阶段一·C（现有机制行为）**：完成 ±15° 静止回归、滑移方向/active/stop/限流语义，以及有 raster/录像证据的局部渐进可见响应；记录全宽坡度、R²、通量与时长。非零 grant 不算可见效果。C 不含 ±45° 全宽坡面收敛目标。
- **阶段一·验证/总完成**：A/B/C 分项通过并满足所有剩余 MUST-PASS 方可报告“阶段一完成”；不能报告 P-011 CLOSED 或 SLIP 全 PASS。±45° 的 30°±2°、target-based t90≤5100ms、R²≥0.90 联合 SLIP-02 目标包连同阶段性收敛验收延期至阶段二可达性审计后，状态 `DEFERRED / NOT VALIDATED`；测量继续保留。守恒/库存/容量/预算/有限值等安全约束不延期。
- **阶段二**：开端由 designer 先审计全部相关通道的可达性，涵盖相邻表面输运、lift/settle、悬浮输运/库存、一阶矩、共享预算、稳定终点与悬沙≤5%；先证明构造或必要条件，再决定方案 3。方案 3 保持候选，不提前实现，也不预判必达。
- **状态映射**：阶段一目标阻断关系解除；SLIP-02 仍延期且未验证；相邻输运路线的 `slip-overshoot-v2` 继续 `BLOCKED（条件性不可达）`；v2.14 继续 `BLOCKED / DESIGN_NOT_CALIBRATED`；观测③继续 `BLOCKED（合格 run/样本不足）`，run-001/002 仍排除。补采资格 gate 豁免不影响运行时安全 guard。
- **路由**：tester 更新 checker、`test-cases.md` 与报告的适用阶段，保留测量并把 SLIP-02 标记延期；designer 更新算法复议状态，并在阶段二开端承担全通道可达性审计，之后交 architect 评审。

## 2026-10-20 P-011 ±45° 补采 gate 停机记录（资格口径仍生效；原因解释已纠正）

> 日期：2026-10-20。来源：`docs/effect-report-P011-recalib-gate-run1.md` 的 run-001/002 复测；依据 `docs/algorithm-P011.md` §8.13/§8.14 与 `docs/architecture-P011.md` §21。manager 裁决：短期修订 ±45° 平台形式 gate、保留转折数据补采；中期由 designer 量化复议目标/机制能力；悬浮平流方案 3 保持阶段二排期。

> 适用性更新：上方 2026-10-20 阶段边界裁决已接受设计复议并纠正停机报告的几何解释。下列形式 gate 豁免仍是补采资格规则；阶段一实际提交的安全检查仍按已接受参考角执行。阶段一延期 SLIP-02，不代表验收数值被取消。

> 维护人：manager。状态：`IN_PROGRESS`；标定仍 `BLOCKED`，直至修订后的 gate、样本及有效 run 条件达成。

- **停机证据**：e2f02a1 复测证明边身份与 stop 状态修复有效，但原补采 checker 在 ±45° 平台未通过参考线形式 gate；该结果解释为当前相邻表面输运能力/窗口不足，不能据此声称 30° 同向线性沙面与 45° 参考几何本身不相容。相邻输运的平坦场一阶矩上界为 0.501627°；此上界不覆盖含升降/悬沙的全部机制。
- **判据边界**：tester 将稳定 ±45° 平台及其 overshoot/return 子步从 `|ΔS−tan(±45°)|≤tan30°` 形式判定中豁免；0°/±15° 与其他适用子步继续原 tan30 判定。其余守恒、库存、容量、预算、账本/字段、边身份、stop 语义闸门仍全部生效。豁免仅表示不以 45° 参考线形式指标否决标定 run，不表示坡度达标、无物理越界或 run 自动合格。转折事件筛选、fit/holdout 规则及样本门槛不变，旧 run-001/002 仍 invalid/excluded。
- **目标复议**：designer 根据 §8.13/§8.14 量化限幅+guard 与实测机制能力的关系，给出 ±45° 收敛目标是否推迟及阶段路线建议；不把机制能力缺口交由重复采集解决。方案 3 不提前实现，仍按阶段二评估。
- **构建前置**：developer 需认领主工作区两份 WIP 源文件、修复当前 10 个 ArkTS 编译错误并形成可审阅提交，作为下一次补采前置条件②；不得与干净 e2f02a1 基线混同。
- **路由**：tester 先同步 checker 与 `docs/test-cases.md` 并输出修订后 RED/验证报告；developer 处理 WIP 构建；两项就绪后 effector 方可按冻结 schedule 恢复单 run gate 采集。designer 的机制/目标复议并行推进。

## 2026-10-19 场景二沙种类扩展需求（待 manager 裁决）

> 日期：2026-10-19。来源：用户在对话中直接提出。需求原义：场景二（水中沙）需要细化；沙子的种类不一定是 2 种，可能是 1 种或更多种；优先处理 1 种和 2 种的情况。沙子的重力属性也可以由用户指定；优先级为：（1）1 种轻沙；（2）2 种沙，一轻一重。

> 登记说明：本条仅转交 manager，designer 暂不出算法方案、不改目标、不改现有实现。需要 manager 先裁决：是否并入 P-003/P-005，还是新开问题 ID；与 P-011 v2.9 的调度优先级；“沙子重力属性”对应的用户配置语义（例如沉降速度、比重或等效重力参数）；版本兼容与默认行为；以及新增 1 种轻沙预设的验收口径。裁决后再路由 designer→architect→developer→tester/effector。

> 维护人：manager。状态：`PENDING_MANAGER`。

## 2026-10-19 P-011 slip-overshoot-v2 可达性复核缺口（当前生效）

> 日期：2026-10-19。来源：architect 对 `docs/algorithm-P011.md` §8.14 与 `test/p011_slip_overshoot_check.py` 的复核。manager 依此裁决路线、终点状态与验收口径。本节取代同日早前的上层沙调度口径，历史结论保留。

> 维护人：manager。状态：`IN_PROGRESS`。

### P-011-A：slip-overshoot-v2 条件性不可达（P0，已裁决）

- **现状**：全宽 OLS 斜率达 `30°` 需相邻输运总量至少 `100,893` units，`28°` 也需 `92,917`；对照 5.1s 总额度上限 `1,530`，缺口约 `66` 倍。中央单边需求 `>=794` units，高于 5.1s 单边 cap `408`。该结论在 `maxAvalancheFlux=80/s`、`totalMotionBudget=300/s`、`5.1s` 与相邻输运路线下成立。
- **裁决**：接受条件性不可达；`slip-overshoot-v2` 标记 `BLOCKED（条件性不可达）`，不交 architect/developer。该结论不外推为所有物理通道绝对无解。

### P-011-D：lift/settle/suspension 替代路线离线可达性审计（P0，当前主线）

- **目标**：使用现有 `ledger.ndjson`，不写新物理、不改参数、不授权新观测，判断 `lift/settle/suspension-diffusion` 能否在 5.1s 内、现有预算与稳定终点约束下形成 `30°±2°` 的全宽 OLS 表面。
- **终点要求**：每个 `±45°` 相位最后 `1s` OLS 斜率变化 `<=1°`，且 `suspH+suspL <= 0.05 * 初始沉积沙总量`；不允许大量悬沙留在终点制造瞬态斜率。
- **路由**：designer 立即执行离线审计；tester 并行修正 target-based t90、OLS 拟合优度与稳定终点口径。architect 等 designer 审计后复核；developer 在新路线被接受且用例就位前不得实现。

### P-011-B：观测/账本口径错位（P0 支撑，继续有效）

- **现状**：`context/stateBefore` 与 `audit/grants` 子步错位仍需 tester 修复；同时现有 `slope90TimeMs` 是相对本相位实测终值的 90%，不是 30° 目标的 90%。
- **裁决**：正式 `t90` 必须改为 target-based，即首次达到约 `27°`；旧值只作诊断。tester 还须新增 OLS `R² >= 0.90`、RMSE 与终点稳定/悬沙阈值。

### P-011-C：重沙机制（P1，押后）

- **现状**：v2.11 已否决；新观测确认滑移是轻沙专用通道。重沙机制继续押后，不占用本轮 designer/architect/developer 资源。

### 验收口径

1. 维持 `30°±2°`、`5.1s`、现有预算与几何不变。
2. 正式 `t90` 为 target-based：首次达到 30° 目标的 90%，约 `27°`。
3. raster 验收不只看 OLS 斜率，还须 `R² >= 0.90` 并报告 RMSE；逐边 `tan30°` 硬约束仍独立生效。
4. 相位结束必须稳定/沉积完成：最后 `1s` OLS 斜率变化 `<=1°`，`suspH+suspL <= 0.05 * 初始沉积沙总量`。
5. 本轮不授权新观测；仅当离线审计无法闭合时，再由 manager 决定是否授权零物理改动观测。

## 2026-10-19 P-011 上层沙 slip-overshoot 缺口（历史；被同日可达性复核缺口取代）

> 日期：2026-10-19。来源：`docs/observation-report-P011-slip-overshoot-v1.md`、`docs/retro-P011-v2.13.md`、`docs/algorithm-P011.md` §8.10~8.13。用户已裁决：先修上层沙（表层滑移收敛 + 坡度约束），重沙机制押后；本节取代 2026-10-18 v2.9 的当前 P-011 调度口径，历史结论保留。

> 维护人：manager。状态：`IN_PROGRESS`。

### P-011-A：真实坡度硬违约与表层滑移不收敛（P0）

- **现状**：七相 `0/+15/0/+45/0/-45/0` 真机运行中，`484` 个子步 `audit.range=false`，首个违约在 ledger sequence `1457`，`maxSandTopInvariantError=0.729596 > tan30°=0.577350`。`+45/-45` 相位可见线性坡度分别为 `0.23°/0.52°`，未收敛到 `30° ± 2°`；`t50/t90` 约 `4.5s/6.5s`。独立复盘确认 `updateTilt()` 先于 material grant 执行，首个 crossing 在材料变更前已越限，属于真实物理违约。
- **目标**：在不增加预算、不放宽 `tan30°`、不预设形状的前提下，让上层沙在超角段收敛到安息角，并保证每个提交子步全部 `127` 条边的 residual 合法。
- **路由**：designer 立即完成 `slip-overshoot-v2` 联合限幅设计；tester 并行固化客观用例。developer 须等 tester 用例就位且 architect 复审通过。

### P-011-B：观测/账本口径错位（P0 支撑）

- **现状**：首个 failure 的 context `effectiveTilt=24°`，但 audit/grants 对应子步内更新后的 `36°`；`context/stateBefore` 在 `substep()` 前采样导致事件标签落后一子步。该缺陷不改变真实违约结论，但会污染归因与验收证据。
- **目标**：同一 ledger 记录同时输出 pre/post `effectiveTilt`、committed `sandTop` 与实际 audit residual；`context`、grants、audit 必须属于同一子步快照。
- **路由**：tester 立即独立核验并修复/固化观测用例，不改物理。

### P-011-C：重沙机制（P1，押后）

- **现状**：`gravity-advection-v2.11` 经数值复核被否决；均匀推力只动两端，中央 5.1s 后 `0px`，远低于 `>=2px` 判据。新观测中 `+45` 相位 slip 通道 `H=0、L=886.17`，重沙界面 `0px`，确认滑移是轻沙专用通道。
- **目标**：待 P-011-A/B 闭环后，再重新评估重沙路线；本轮不占用 designer/architect/developer 资源，不以重沙位移作为上层沙子目标放行条件。

### 验收口径

1. 七相序列下每个提交子步、全部 `127` 条边满足 `|S[c+1]-S[c]-tan(effectiveTilt_committed)| <= tan30°`，violation 数 `0`。
2. `+45/-45` 相位结束时 raster 可见线性坡度均在 `30° ± 2°`；`t90 <= 5.1s`，`t50` 仅作诊断。
3. 材料守恒、库存、容量、local/bottom/total 预算保持原阈值；不得增加预算、不得放宽阈值、不得预设目标形状。
4. 账本同一记录包含 pre/post `effectiveTilt`、committed `sandTop` 与实际 audit residual；不得覆盖旧证据。
5. 重沙界面 `>=2px` 不作为本子目标放行条件，只要求不引入守恒或预算回归。

## 2026-10-18 P-011 默认重层响应 FAIL（历史；被 2026-10-19 上层沙缺口取代）

> 日期：2026-10-18。原因：dual-input-v1 已实现、真机安装、39项实际产品测试独立重跑PASS，模式冷热启动/首Want/体验拒绝脚本/静态真实sensor callback/面板切换通过；但新包默认green正式硬前置FAIL，完整客观与主观尚未放行。P-011 保持 P0、`IN_PROGRESS（默认重层响应失败，转 designer v2.9）`，不CLOSED。

> 最新来源：`docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 正式新包FAIL、`docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md` 的 `BLOCKED(green_default_scene_gate_failed)`、`test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json`。默认128列/capacity192/max28，单run五条script准确，±15各约5.1秒；heavy样本124/154达标，但RMS约1e-7、max约1e-6远低于既有0.5/1.0，green质心仅0.006/0.001px。不是输入未送达或待裁定用例争议。旧10列/capacity1000/max30 PASS不外推默认场景；现工具OBSERVATION_READY仅表示ROI存在，不能覆盖硬闸门FAIL。

> 身份：reviewer业务/host PASS并提交 `130a52c74e8964fb024677b672cad1965f5cf1db`；新签名包 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`。实际build基线fdc8190，sourceCommitType=`reviewed P011 baseline + recorded dirty source snapshot`，48源hash manifest绑定；不称clean重现。

> 路由：designer立即归因并设计v2.9→architect→developer→reviewer→tester默认green/模式回归/canonical531冻结审计/完整压力suite→effector自动录像视觉评估→designer验证/独立复盘→manager。保留双模式实现、全部阈值和10列压力回归。预检工具漏硬闸门作为测试资产子项直接交tester先补测试并最小修复；当前canonical/压力/full subjective均BLOCKED。原生录制能力具备，effector仅host准备；不等待用户手持、不改阈值、不覆盖历史。

> 2026-10-19 manager 状态同步（历史；被同日上层沙裁决取代）：architecture §16 对 `bottom-creep-v2.9-central-dune-heavy-interface` 保持 `BLOCKED`。tester 已交付仅限阻断证明的 `docs/test-report-P011-bottom-creep-v2.9-blocking-proof.md` 与 reviewer 复核 `docs/review-report-P011-v2.9-blocking-proof-20261019.md`；两项只证明 `BLOCKED-V29-ANGLE` 与 `BLOCKED-V29-RELEASE`，不解除阻断、不授权 developer。designer 仍需先解决 angle-feasible target、actual-shape release、预算口径与相位生命周期。用户已裁定 green 2px 为局部 heavy 边界位移，whole green mask 质心仅作诊断；该决定不解除任一架构阻断。完成后必须交 architect 复审，而不是直接实现。

## 2026-10-18 P-011 业务开发状态同步（历史）

> 日期：2026-10-18。原因：tester §10 MODE01~07/RED 基线、effector 自动姿态双模式协议/RED 基线，以及 designer algorithm §5.8/独立复盘、architect §15 READY_FOR_DEVELOPER 均已交付。P-011 当前为 P0、`IN_PROGRESS（双模式业务开发）`；剩余缺口是输入隔离/helper unit 业务实现交付、业务审核和新 HAP 自动双测，而非等待用例或设计。

> host capture/compact 的 19 项回归、PowerShell 与 build 检查已获 reviewer PASS，尚未 commit；业务 developer `p011_input_impl` 正在实现两模式输入隔离与真实 helper unit。原生自动录像启动/停止/导出已实测成功，设备已释放给业务 developer，历史 no_screen_recording 阻塞不再代表当前能力；该能力验证不等于主观效果通过。下一步：业务交付→reviewer 统一复审/定向提交→tester canonical precheck/objective→effector 自动录像/视觉评估→designer 双报告验证/独立复盘→manager。原目标、阈值、采集审计及同机串行要求不变；保留全部历史结果。

## 2026-10-18 P-011 双模式输入决策（历史前置；输入策略继续有效）

> 日期：2026-10-18。原因：用户正式要求测试模式程序生成手机倾斜状态、体验模式读取真实传感器。该输入工程缺口并入 P-011，保持 P0、`IN_PROGRESS（双模式设计前置 + 验证/录制整改）`，不新增独立问题 ID；输入源隔离、模式切换/重启与姿态复现待补用例、设计和验证。目标见 `docs/goals.md` 当日双模式目标。

> 当前路径：tester/effector 补模式用例及可行基线→designer 输入契约→architect 工程方案→developer 与既有 host capture/compact 整改汇合→reviewer→tester canonical precheck/objective→effector 自动视觉评估→designer 双报告验证/独立复盘→manager。用户手持/代录不再是验收前置，程序姿态替代人工输入已获用户明确授权；物理机制、全部阈值、连续视频及 hilog 对齐要求不变。保留历史 objective PASS 与 `BLOCKED(no_screen_recording)`，继续自动探测录制能力；报告注明模拟输入能力范围，不据此声称真实硬件噪声/标定/人体体验已验证。此说明取代下文旧路由和人工采集前置，审计要求保留。

> 维护人：manager。数据来源：docs/test-report.md（客观）、docs/effect-report.md（主观）。
> 已闭环的问题移出本清单，历史记录见 git。

| 问题 ID | 描述 | 来源（哪份报告的哪项发现） | 影响的目标 | 状态 |
|---|---|---|---|---|
| P-003 | B3 第一阶段：水 + 一种沙；单沙沉降/堆积/悬浮状态及动态响应缺少独立算法闭环；双沙/三相分选延期 | `docs/DESIGN_GAP_CLOSE.md` §四；`docs/algorithm-P003-P005.md`；2026-10-23 用户范围裁决 | G2、G3 | IN_PROGRESS（单沙+水；三相 DEFERRED） |
| P-005 | 场景二单沙+水客观/主观用例与判定脚本需补齐；测试覆盖水/沙各自守恒、沉降/堆积、安息角、水面、动态倾斜/摇晃恢复、视觉质量。双沙分选和三相专用判据暂不属于当前阶段 | `docs/test-cases.md`、`docs/effect-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`、2026-10-23 用户范围裁决 | G2、G3 | IN_PROGRESS（单沙+水 RED 前置） |
| P-001 | B1/M1‘：场景一 SandGrid 液体规则已实现（s79），但 `docs/test-report.md` 与 `docs/effect-report.md` 仍无有效数据，未完成客观/主观闭环 | `docs/DESIGN_GAP_CLOSE.md` §二 v1.1；`docs/test-report.md` 报告 #1 无数据；`docs/effect-report.md` 报告 #1 无数据 | G1、G3 | OPEN |
| P-002 | B2/M3：倒立渲染 `want` 直启路径已 PASS，面板路径与完整回归仍未闭环 | `test/reports/TC-09..10_s77r_independent_verify_report.md`；`docs/test-report.md`、`docs/effect-report.md` 均无有效数据 | G1、G3 | OPEN |
| P-004 | B4：三相判定脚本扩展、全场景动态回归矩阵、TC/SV/VV 报告缺失 | `docs/DESIGN_GAP_CLOSE.md` §五；两份最新报告均无有效验收数据 | G1、G2、G3 | OPEN |
| P-007 | slopeLimit 姿态修正设计条件已清零：v2.4 §18 明确 `|d - tan(effectiveTiltDeg)| > tan(α)`，且 `α` 为“崩塌角”；仍缺 developer 实现、倾斜边界/守恒/直立回归的正式验证报告 | `docs/algorithm-P003-P005.md` §18；`docs/test-report-P003-P005.md` | G2+G3 主线 | IN_PROGRESS |
| P-008 | 双沙阶段的 percolationRate 参数化与干湿门控；因当前先做单沙+水，暂不适用 | `docs/algorithm-P003-P005.md` §16.2；2026-10-23 用户范围裁决 | G2+G3 后续阶段 | DEFERRED |
| P-009 | WS_SNAPSHOT 超 4KB 被真机 hilog 截断，阻塞 E2E 客观验收；需确认快照分片输出或文件落盘方案 | 真机复核 2026-10-14 | G2+G3 主线 | IN_PROGRESS |
| P-011 | 观测③ v2 pilot 标定失效（三层根因）：clean=INVALID（5 边 tan30 违约）、±30° 平台 TILT_PLANNING_BLOCKED 不可达、EXECUTION_THRESHOLD_IDENTITY_MISMATCH——构建 relaxAngle 实际由 tan(4°) 驱动，候选阈值未进状态机，459 个 start 均为旧请求态诊断。v3 方向 2（平台+安全 ramp 联合激励）已批，执行身份整改链进行中 | `docs/effect-report-P011-obs3v2-pilot-run1.md`；`docs/observation3-v3-identification-redesign-v1.md`（SHA-256 复核 + ledger 只读诊断） | G2.1 表层滑移与安息角；G2.2 堆积轮廓；G3 动态连续响应 | IN_PROGRESS（v3 身份整改： designer 冻结协议 ∥ developer 采集侧修复 → architect → tester；effector 待新 pilot 批准；观测③标定本身维持 BLOCKED / INCOMPLETE） |

> 2026-10-16 P-011 阈值裁定：主观验收以 `docs/effect-cases.md` P-011 的 **0~10 分、及格线 6** 为准；`docs/algorithm-P011.md` §3.7 与 `docs/architecture-P011.md` §9.2 中的 `3.5/5` 表述需由 designer/architect 在闭环前同步修正，避免双测判定口径漂移。

> 2026-10-18 P-011 复核备注（历史）：v2.8 objective PASS；主观 HG-TT-04 复测仍必须独立通过。不得沿用 v2.7.x 证据合并判定，也不得放宽任何阈值。

> 2026-10-18 P-011 历史补充；原因：最新架构要求与旧客观证据不一致。旧 PASS 不覆盖采集冻结/canonical 等价性缺口，也不能据此进入最终主观验收。历史路由为 **developer 最小整改 → reviewer 复审与定向提交 → tester canonical precheck + objective → effector（需连续录制）→ designer 双报告验证/独立复盘 → manager**。整改仅修复验证流程，不能推出物理机制失败。reviewer 因环境异常尚未完成正式复审/提交；主观首次正式判定为 `BLOCKED(no_screen_recording)`，需恢复连续录制/人工录制与 hilog 时间对齐后方可评分。测试前需记录新 commit、源文件身份与 HAP SHA256，明确 `fdc8190` 只代表历史/原 build 基线，不能代表仍 dirty 的完整 v2.8 源码。同机采集严格串行，主观 HG-TT-04 与 ≥6/10 及格线不变；双测和 designer 验证/独立复盘齐备前不 CLOSED。详见 `docs/priority.md` 的 2026-10-19 当前调度。

> 现状说明：根据用户最新指令，目标按两条组合线组织：**G2+G3 主线**（场景二在容器动态下的表现）优先，**G1+G3 支线**（场景一在容器动态下的表现）另行开题。G3 不是独立交付项，而是分别并入两条组合线的共享容器动态能力。当前客观与主观报告均为空，因此所有问题仍无法判定闭环。

> 2026-10-14 体验覆盖备注：G2 一级体验为“翻转时沙随重力连续响应”与“倒扣后轻沙上浮、重沙下沉的分选反转动态”；二级体验为“剧烈摇晃混乱”与“混乱后再沉淀”。一级不达标时，二级项不作为放行理由；二级验收阈值不降低，仅调整顺序。当前 `TC-WS-07` / `EC-WS-07` 对一级动态覆盖不足，需补连续重力与倒置分选时序；`TC-WS-08` / `TC-WS-09` / `EC-WS-06` 已基本覆盖二级链路。

> 2026-10-14 跨角色同步备注：G3 明确为“±89° 连续重力变化 + 90° 经审计 `invert180()` 事件跨越”，严格连续过 90° 不在本期目标。`docs/effect-cases.md` 的 H06 需由 effector 修正为“绕过 API 私改内部数组/状态 = FAIL；经 `invert180()` 且通过守恒审计不判 FAIL”，避免与事件式设计冲突。

> 2026-10-14 P-005 追加子任务：`docs/effect-cases.md` H06 措辞修正与 `docs/test-cases.md` HG-04/TC-WS-08 口径修正。H06 需改为“绕过 API 私改物理数组=FAIL；经 `invert180()`、带事件日志且过三相守恒审计的坐标切换不判 FAIL”；HG-04 需明确为“物理子步”检查，30Hz 两子步分别校验，TC-WS-08 需增加 `suspensionFractionHeavy/Light` 分相悬浮记录。
>
> 2026-10-14 P-011 测试先行结论：现有 `TC-WS-06` 只验证稳态倾斜，`TC-WS-07` 只验证事件式倒立；`EC-WS-05` 固定 180°/s 且无跟手性阈值。连续快速倾角、输入-有效倾角时延、跟踪率/相位滞后、底层沙响应时间等客观/主观用例均缺失，须先由 tester/effector 并行补齐；用例就位前不得进入 designer 或继续调参。

## 2026-10-14 等待期可启动项与类型划分

> 2026-10-15 备注：本节与其上方 2026-10-14 P-011 测试先行结论均为历史记录；P-006 已复验通过并闭环。当前以 gap 表中 P-007 的实现/验证缺口与 P-011 的 `bottom-creep-v2.7` 路线为准。

| 问题 ID | 类型 | 现有用例情况 | 可启动内容 | 路由 | 启动时机 |
|---|---|---|---|---|---|
| P-006 | 测试用例缺口 | 客观已有 TC-11~13 草案，主观仍为占位 | 转正 TC-11~13，补齐场景一客观/主观用例与判定脚本 | **tester + effector 并行** | 立即启动 |
| P-009 | 测试支撑缺口 | 无快照分片/落盘方案 | 确认 `WS_SNAPSHOT` 输出方案，并同步解析与校验 | **architect → tester** | 立即启动 |
| P-004 | 回归测试缺口 | 客观/主观用例均未成体系 | 先行设计全量回归矩阵与判定脚本框架 | **tester + effector** | 可预研 |
| P-007 | 功能缺口 | 客观用例尚未覆盖倾斜姿态修正 | designer 先行准备修正方案与测试预期 | **designer** | 可预研 |

## 2026-10-14 组合目标线与现有 gap 映射

| 组合线 | 包含子目标 | 对应 gap | 说明 |
|---|---|---|---|
| **G2+G3 主线** | G2.1~G2.7、G3.1~G3.3 | P-003、P-005、P-004 | 当前优先；G2.1~G2.7 对应场景二实现与测试，G3.1~G3.3 为共享容器动态能力 |
| **G1+G3 支线** | G1.1~G1.3、G3.1~G3.3 | P-001、P-004 | 另行开题；G1.1~G1.3 对应场景一实现与测试，G3.1~G3.3 为共享容器动态能力；P-006 测试资产已闭环，P-001 可进入实测 |

> 2026-10-13 阻塞项：`docs/algorithm-P003-P005.md` §3.3 的 `hl[c] -= q; hs[c] += q` 会破坏重沙/轻沙分相守恒，违反 P-003 的最高优先级不变量。architect 已在 `docs/architecture-P003-P005.md` §0.1 标注，developer 不得实现该公式；需 designer 先修订算法，再由 architect 同步工程方案。P-005 不因此阻塞：tester/effector 可先编写用例，并把三相守恒作为硬断言，用于暴露该类公式错误。

## 变更记录

- 2026-10-20：原因：【观测③·v3】manager 裁决。批准方向 2（稳定平台+安全提交 ramp 联合激励）进入新协议设计，搜索窗与全部安全/统计门槛不变；确认 EXECUTION_THRESHOLD_IDENTITY_MISMATCH 根因并启动执行身份整改链（designer 协议冻结 ∥ developer 采集侧修复 6ad6fb88 → architect → tester → effector 待批）；结构性缺失预案生效（子 cell 缺失交 manager 三选一裁决，禁默默合并）。P-011 gap 描述从 slip-overshoot 审计更新为 v3 身份整改主线；观测③标定状态维持 BLOCKED / INCOMPLETE。

- 2026-10-19：原因：architect 复核确认 slip-overshoot-v2 条件性不可达与 checker 口径缺陷。manager 接受下界结论，维持 30°±2°/5.1s/预算不变，新增 P-011-D 替代路线离线审计，明确稳定终点、target-based t90、OLS 拟合优度与零物理改动观测边界。

- 2026-10-19：原因：用户裁决 P-011 先修上层沙。登记 `slip-overshoot-v1` 的 484 次真实坡度硬违约、表层滑移未收敛与 context/audit 口径错位；拆分 P-011-A/B/C，重沙机制押后，并固化七相验收口径与 tester∥designer 路由。

- 2026-10-18：原因：双模式功能交付后，新HAP默认128列green正式硬前置FAIL。记录真实源码/包身份与默认几何差异，P-011路由designer v2.9，工具闸门遗漏直交tester；完整双测BLOCKED，保留历史、不改目标或阈值。

- 2026-10-18：原因：模式用例/RED 基线、算法/架构前置交付，原生自动录像能力验证成功。P-011 当前缺口更新为业务开发→统一审核提交→新 HAP 自动双测，保留历史 PASS/BLOCKED，不改目标与门槛。

- 2026-10-18：原因：用户正式变更输入/验收方式。双模式工程纳入 P-011，采用模式用例先行→designer→architect 设计路径并与既有整改汇合；取消用户手持/代录依赖，保留原阈值、连续录像及最终双报告验证条件。

- 2026-10-18：原因：恢复流水线时复核最新 architecture §7.5.6/§7.5.7，旧 v2.8 objective PASS 的 JSON 缺少 canonical/frozen audit。P-011 保持 P0 范围的验证流程整改，不改目标/算法/阈值；同步 developer→reviewer→tester→effector→designer→manager 依赖、正式证据身份与闭环条件，保留全部历史结论。
- 2026-10-18：原因：effector 首次正式主观判定受阻于设备无连续屏幕录制，登记 `BLOCKED(no_screen_recording)` 并保留探索证据；未评分，HG-TT-04 未判定。

- 2026-10-16：原因：manager 复核 P-011 设计与工程文档。确认 `bottom-creep-v2.7` 已进入 `READY_FOR_DEVELOPER`，将路由推进为 developer、tester、effector 并行；真机/模拟器实测保持串行或分设备约束。

- 2026-10-15：原因：manager 复核 P-010。确认 `docs/architecture-P003-P005.md` §10 第 1121 行已包含“本节预算接口已被 §11 v2.2 取代”的导航提示；P-010 闭环并移出本清单。

- 2026-10-15：原因：manager 复验 P-006 客观脚本返工。确认脚本与正式 JSON/Markdown 报告均标识 P-006，TC-12 恢复时间、TIERED 映射、TC-13 连续性与帧输入三件套校验全部通过；P-006 闭环并移出本清单，P-001 测试前置解除。

- 2026-10-15：原因：manager 复核外部专家提出的 v2.5 三项冻结条件。确认三项条款在 `docs/algorithm-P003-P005.md` 已清零；因 v2.5 状态为 `REJECTED`，将 P-011 后续路线指向 `bottom-creep-v2.7`，并同步 P-007 的剩余实现与验证缺口。
- 2026-10-15：原因：manager 复验 P-006 返工。发现客观脚本与正式报告仍标识 P-001，复验不通过并路由 tester 修复；其余客观判定项与 `environment.json` 模板校验通过。
- 2026-10-14：原因：用户真机反馈底层沙“不跟手”且临时调参尚未确认。新增 P-011，登记客观/主观跟手性用例缺失，并路由 tester + effector 先行补齐。

- 2026-10-13：初始创建。
- 2026-10-13：原因：启动 gap 管理。将 `docs/DESIGN_GAP_CLOSE.md` 的 B1~B4 纳入 P-001~P-004；结合 `test-report.md` 与 `effect-report.md` 当前无有效数据的事实，全部保持 OPEN。
- 2026-10-13：原因：用户最新指令调整场景推进顺序。将 gap 表与现状说明同步为“场景二（B3）优先，场景一（B1/B2）次之，B4 最后”。
- 2026-10-13：原因：用户补充测试用例也属于 gap。新增 P-005（场景二测试缺口）与 P-006（场景一测试缺口），并同步修正 P-001/P-002 描述以对齐 `DESIGN_GAP_CLOSE.md` v1.1 与 s77r 复核结果。
- 2026-10-13：原因：architect 发现 algorithm v1 §3.3 渗透公式破坏分相守恒。将 P-003/P-005 标记为 `IN_PROGRESS（BLOCKED）`，并记录 developer 暂缓实现，等待 designer 修订算法。
- 2026-10-13：原因：用户更新 `.codex/PIPELINE.md` 问题路由规则。P-003 继续走 designer→architect→developer 完整闭环；P-005 改为 tester（客观）+ effector（主观）并行直补，不再经过 designer/architect/developer，也不被 P-003 算法修订阻塞。
- 2026-10-14：原因：用户明确 G2 体验优先级。追加一级/二级体验覆盖备注，要求优先保障翻转重力响应与倒扣轻沙上浮分选，再推进摇晃混乱与再沉淀；一级不达标时二级不作为放行理由，二级阈值不降低。
- 2026-10-14：原因：designer v2.2 完成后的跨角色同步。补充 G3 事件式 90° 语义，并将 H06 硬门槛措辞修正列为 P-005 的 effector 待办。
- 2026-10-14：原因：用户要求拆小目标。建立 13 个子目标与现有 P-001/P-003/P-004/P-005/P-006 的映射，明确哪些子项已有用例、哪些仍需补用例。
- 2026-10-14：原因：用户纠正目标结构。将 gap 映射从三条独立目标改为 **G2+G3 主线** 与 **G1+G3 支线** 两条组合线，G3 作为共享动态能力分别并入两线。
- 2026-10-14：原因：v2.2 复核与 designer v2.3 提出批量路由请求。新增 P-007~P-010，分别登记 slopeLimit 姿态修正、percolationRate 参数化、WS_SNAPSHOT 4KB 截断与 architecture §10 导航；同时将 H06 与 HG-04/TC-WS-08 口径修正列为 P-005 的立即执行子任务。
- 2026-10-14：原因：用户要求识别主线等待期可并行启动项。将 P-006、P-009、P-010 标记为 IN_PROGRESS，P-004、P-007 标记为可预研，并区分测试用例缺口、测试支撑缺口、文档支撑缺口与功能缺口。
