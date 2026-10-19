# Gap 清单（目标 vs 现状）

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
| P-003 | B3：WaterSandField 三相模型、第四渲染模式、面板与 Node 单测缺失；当前算法 v1 §3.3 存在分相守恒缺陷，developer 暂缓实现 | `docs/DESIGN_GAP_CLOSE.md` §四；`docs/architecture-P003-P005.md` §0.1；两份最新报告均无有效验收数据 | G2、G3 | IN_PROGRESS（BLOCKED） |
| P-005 | 场景二测试用例与判定脚本缺口：`docs/test-cases.md`、`docs/effect-cases.md` 为占位；仓库无 WaterSandField/三相用例；`test/vv_s75_analysis.py` 等仅支持两相；无 VV-07/08/09、三相守恒、分选比、沉降时序、安息角、水面与主观观感用例；客观/主观用例可直接由 tester/effector 并行补齐 | `docs/test-cases.md`、`docs/effect-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`、`entry/src/ohosTest/ets/test/SandGridTest.ets` | G2、G3 | IN_PROGRESS |
| P-001 | B1/M1‘：场景一 SandGrid 液体规则已实现（s79），但 `docs/test-report.md` 与 `docs/effect-report.md` 仍无有效数据，未完成客观/主观闭环 | `docs/DESIGN_GAP_CLOSE.md` §二 v1.1；`docs/test-report.md` 报告 #1 无数据；`docs/effect-report.md` 报告 #1 无数据 | G1、G3 | OPEN |
| P-002 | B2/M3：倒立渲染 `want` 直启路径已 PASS，面板路径与完整回归仍未闭环 | `test/reports/TC-09..10_s77r_independent_verify_report.md`；`docs/test-report.md`、`docs/effect-report.md` 均无有效数据 | G1、G3 | OPEN |
| P-004 | B4：三相判定脚本扩展、全场景动态回归矩阵、TC/SV/VV 报告缺失 | `docs/DESIGN_GAP_CLOSE.md` §五；两份最新报告均无有效验收数据 | G1、G2、G3 | OPEN |
| P-007 | slopeLimit 姿态修正设计条件已清零：v2.4 §18 明确 `|d - tan(effectiveTiltDeg)| > tan(α)`，且 `α` 为“崩塌角”；仍缺 developer 实现、倾斜边界/守恒/直立回归的正式验证报告 | `docs/algorithm-P003-P005.md` §18；`docs/test-report-P003-P005.md` | G2+G3 主线 | IN_PROGRESS |
| P-008 | percolationRate 固定为 `6/s`，未按密度差/黏度参数化，且缺少干湿门控 | `docs/algorithm-P003-P005.md` §16.2；外部复核 2026-10-14 | G2+G3 主线 | OPEN |
| P-009 | WS_SNAPSHOT 超 4KB 被真机 hilog 截断，阻塞 E2E 客观验收；需确认快照分片输出或文件落盘方案 | 真机复核 2026-10-14 | G2+G3 主线 | IN_PROGRESS |
| P-011 | `slip-overshoot-v2` 在相邻输运、现有 80/s edge cap、300/s total cap 与 5.1s 时限下条件性不可达；需改做 lift/settle/suspension 离线可达性审计，并修正 target-based t90、OLS 拟合优度与稳定终点口径 | architect 对 `docs/algorithm-P011.md` §8.14 与 `test/p011_slip_overshoot_check.py` 的复核；`docs/retro-P011-v2.14.md`；现有 `ledger.ndjson` | G2.1 表层滑移与安息角；G2.2 堆积轮廓；G3 动态连续响应 | IN_PROGRESS（替代路线离线审计；重沙押后） |

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
