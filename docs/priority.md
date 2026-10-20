# 优先级队列（priority）

> 维护人：manager。一轮聚焦最多 1~3 个高优先级问题。

## 2026-10-20 P-011 观测③平台可达性矛盾裁决（当前生效）

> 日期：2026-10-20。依据 `docs/effect-report-P011-recalib3-gate-run1.md`。835e663 基线的阶段一 MUST-PASS 全部通过：164,084 条边 0 tan30 违约、A-1 对账 0 不匹配、账本对齐 0、边身份 0 错位；本次失败仅由 deferred SLIP-02 收敛包驱动。与此同时，4b1f635 planning pass 将运行时 `thetaAccepted` 限制在约 29.9°，冻结 ±45° schedule 因而没有 ±45° 稳定平台或有效 bracket。

1. **三选一裁决：选择方案 3——接受可达平台边界，重定义观测③为 ±15°/±30° 平台的双角阈值标定；±45° 行为和全局收敛目标继续留在阶段二。** 不采用标定 run 绕过 planning guard；阶段一安全 MUST-PASS、tan30 约束和阶段一 B 的实现边界不变。
2. **协议改版授权与前置**：现有 ±45° schedule 及其随机化层不再适用于新校准协议；effector 不得在冻结 schedule 上现场替换角度。designer §8.16 已完成边界论证：±15°不能单独激励 `thetaStart∈[25°,30°)`；±30°可能触及低段，而有效角约 29.9° 会使未触发的高端候选右删失。manager 复核接受这一**显式识别边界**，不要求把右删失伪装成完整可辨识；允许进入新 protocol 的单 run clean pilot，以测实际平台/bracket yield。凡被 29.9° ceiling 右删失的候选必须标记 `RIGHT_CENSORED_AT_TAN30`，不产出点估计、不算校准通过。effector 建立全新 protocol/schedule 版本并重新预注册；不得沿用旧版 34-run 分层/随机化 manifest。
3. **tester 闸门**：新 schedule 预注册后，tester 同步 checker、test-cases 和阶段标签。校准 run 的资格检查仍执行 tan30 hard gate、budget、conservation、stock、capacity、finite、telemetry alignment、edge identity、stop semantics 等全部 MUST-PASS；不设 capture-mode 豁免。checker 必须明确识别可达平台上限、右删失、有效 bracket 与“真实 stop vs planner hold/限流”。平台不稳定或无可用 bracket 的 run 不能进入 fit/holdout，保留为 pilot/invalid evidence 并说明原因。
4. **执行顺序**：designer §8.16 论证已由 manager 复核，允许继续协议化；effector 发布新 protocol/schedule 并重新预注册 → tester checker/cases 就位 → 仅执行单 run clean pilot。clean pilot 只评估可达平台和 bracket yield；manager 审核 pilot 结果后再决定是否扩充 fit/holdout，绝不自动开跑 34-run 批次。旧 `0/+15/0/+45/0/-45/0` protocol 的 run-001 证据保留、不计入新 protocol 的 fit/holdout；不得把历史无平台 run 追认为有效。
5. **Blocked 状态与阶段边界**：观测③整体仍 `BLOCKED（尚无合格 bracket/阈值估计）`；旧协议停止扩批，新协议仅获 clean pilot 授权，fit/holdout 扩批仍冻结。835e663 阶段一安全验收记录为 PASS，不能据此声称阈值标定完成。SLIP-02 ±45° 收敛目标仍为 `DEFERRED / NOT VALIDATED`，阶段二再做全通道可达性审计；45° 目标不进入本次改版标定 schedule。
6. **不变量**：禁止放宽 tan30、取消 planning 闸门、修改 `thetaAccepted` 安全语义、在 calibration capture mode 下绕过产品保护，或将硬闸门豁免外推到运行时材料提交。任何后续安全判据修改须另行 manager 裁决。

## 2026-10-20 P-011 阶段一·B architect CONDITIONAL_PASS 裁决（当前生效）

> 日期：2026-10-20。依据 `docs/architecture-P011.md` §22 对 commit `4b1f635` 的独立评审。七项 checklist 全 PASS；R-1 为逐 writer pre-commit guard 尚未交付，R-2 为两个 legacy 30° 用例 FAIL。

1. **B 实现状态**：接受【阶段一·B】实现完成（`CONDITIONAL_PASS`）。当前倾角可行域闸门、纯预演/apply 镜像一致性与每子步核验共同构成已交付保护；不得把 CONDITIONAL_PASS 扩写成全通道逐 writer guard 已完成或所有旧用例已通过。
2. **C-1｜立【阶段一·B·续】**：实现 §19.5 全通道 pre-commit 材料提交 guard，包括逐 writer 闭式拦截、违规态 non-worsening 处理和四层预算回写；同步完成 `hardGuardAvailable` telemetry 与 checker 逻辑。该任务与阶段一 A/C 验证并行排期；但必须在阶段一整体验收签字前交付、评审并纳入集成安全验证。若尚未完成，只可报告 B 当前 commit 的条件性实现状态，不得签阶段一整体完成。
3. **C-2｜选择维持文档化 FAIL 至阶段二**：`bottomCreep_tiltMovesLowerLayers` 与 `p011_commandSignFlip...` 作为旧引擎真实 tan30 违约被新闸门暴露的具名 legacy FAIL 保留并隔离记录，不归因于 4b1f635 新回归；当前不授权改变可行域/安全判据，也不授权重调测试参数或测试期望。它们不阻断本次 B 实现完成，但在阶段二由 tester/designer 结合全通道方案与新 guard 对这两个场景重新分类和验证前，不得删除白名单标记或声称解决。
4. **安全不变量**：维持 tan30、预算、几何及实际接受倾角语义；不得用 epsilon/margin 放宽边界。只有用于有限精度数值稳定且不改变判定阈值的实现容差，才可按现有设计契约另行审查。
5. **状态/路由**：B implementation `CONDITIONAL_PASS`；B·续 `OPEN / IN_PROGRESS`；两个 legacy FAIL `KNOWN_FAIL / DEFERRED_TO_PHASE_2`。developer 实现 B·续，architect 复审，tester 补齐直接 guard 与回写覆盖并重跑；与 A/C 验证并行但纳入阶段一总验收 gate。阶段二再复核两项 legacy 用例，未裁定前原始测试和 FAIL 证据完整保留。

## 2026-10-20 P-011 阶段边界复议裁决（当前生效，覆盖下文旧阶段一标准）

> 日期：2026-10-20。依据 designer `docs/review-basis-P011-stage-boundary-v1.md` 与 `docs/algorithm-P011.md` §8.15。裁决：接受设计复议建议。相邻输运必要条件只证明该路线在 5.1s、总预算 1530 下从平坦场最多形成约 0.501627° 全宽 OLS 斜率；不外推为所有含升降/悬沙方案无解。45° 参考下同向 30° 线性沙面本身满足 tan30 局部硬约束，停机报告把该几何关系解释为“目标本身不相容”有误；真正未证成的是相邻机制在时限/预算内达到全局目标的能力。

1. **A｜阶段一·A 双角滞回/状态机完成标准**：逐边 start/hold/stop 语义、真实 active 转换、限流不误报 stop、物理边身份和遥测对齐全部通过；对原始观测按既定 fit/holdout、CI、样本数门槛完成 `thetaStart/thetaStop` 估计，不填默认阈值。阶段一可报告实测阈值及其 CI；`thetaStop` 映射到 ±45° 全局目标窗口 `[20.563°,25.093°]` 的适配检查延期到阶段二目标包验收。标定③在样本与合格 run 不足期间继续 `BLOCKED`，不因阶段边界调整而伪称完成。
2. **B｜阶段一·B 安全限幅与提交 guard 完成标准**：每个提交子步按实际接受参考角检查全边 `|ΔS−tan(effectiveTiltAccepted)|≤tan30°`，零违约；只允许在完整事务可行时提交。若可行域为空或回退计划仍不合法，必须不提交并记录 `hold/BLOCKED` 原因、未接受角度与最紧约束边。预算、守恒、库存、容量和账本对齐保持现行 MUST-PASS。不得要求系统为证明安全而成功追随到 ±45°；无限 hold 也不算收敛或体验通过。
3. **C｜阶段一·C 现有机制效果完成标准**：分别验证（a）`±15°` 静止回归无新增运动/越界，（b）滑移方向、active/stop 及预算裁剪语义正确，（c）有可复核、用户可见的局部渐进斜坡响应。局部效果必须由 raster/录像可见证据支持，非零 grant 不算通过；报告全宽 OLS 坡度、R²、通量与 run 时间作诊断/能力记录。守恒、库存、容量、预算、±15°安全检查均不得退化。阶段一·C 不要求 ±45° 全宽坡面收敛到 30°±2°。
4. **阶段一·验证与总完成标准**：tester 对 A/B/C 各自出具独立结果；阶段一可完成的条件为 A、B、C 按上述边界通过，且逐边安全、有限值、守恒、库存、容量、总/local/bottom 预算、遥测/账本一致性全部 MUST-PASS。不得把阶段一标记为 P-011 CLOSED，也不得写成 SLIP 全 PASS。±45° 目标包仍保留为项目目标：`30°±2°`、target-based `t90≤5100ms`、`R²≥0.90` 联合验收；整个 SLIP-02 包（含适用的 ±45° 终点稳定/悬沙约束）移至阶段二可达性审计之后，不以 FAIL 阻断阶段一完成，但须持续列为 `DEFERRED / NOT VALIDATED`，原始测量继续记录。SLIP-03/04 中守恒/库存/容量/预算/有限值等安全不变量不延期。
5. **方案 3 与阶段二入口**：阶段二开端由 designer 先对全部相关通道做离线/账本驱动可达性审计，覆盖相邻输运、lift/settle、悬浮输运与库存变化，逐项结算表面一阶矩、共享预算、稳定终点和悬沙≤5%；先证明可达构造或必要条件，再决定是否进入方案 3 设计。方案 3 保持阶段二候选，不提前实现；本次也不预断它必然达标。阶段二审计不通过则保留目标未达成并报告冲突，不靠调参、放宽预算或改阈值放行。
6. **Blocked 状态**：解除“±45° 目标包未达成因此阻断阶段一”的阻断关系；SLIP-02 标记 `DEFERRED / NOT VALIDATED`，仍不是 PASS。`slip-overshoot-v2` 在相邻表面输运路线下仍为 `BLOCKED（条件性不可达）`；v2.14 限幅方案继续 `BLOCKED / DESIGN_NOT_CALIBRATED`，直到 omega/真机数据达到其自身解除条件。观测③保持 `BLOCKED（合格标定 run / 样本不足）`，旧 run-001/002 不追认。停机报告的 ±45° gate 豁免仍仅用于标定 run 资格；它不豁免阶段一实际提交的安全 guard。
7. **路由**：tester 更新 `test-cases.md` 与 checker，将阶段标签和 SLIP-02 目标包的适用阶段分开；保留全量测量，将 ±45° 目标包标为延期而非删除，并重测/更新报告。designer 维护算法文档中的复议状态，并在阶段二开端负责全通道可达性审计；审计通过后再交 architect 评审。developer 只按阶段一批准边界实现，不能将目标未达成伪装成通过。

## 2026-10-20 P-011 ±45° 补采 gate 几何冲突裁决（补采资格规则仍生效；阶段边界已由上条更新）

> 日期：2026-10-20。依据 `docs/effect-report-P011-recalib-gate-run1.md`（run-001/002 复测停机报告）、`docs/architecture-P011.md` §21、`docs/algorithm-P011.md` §8.13/§8.14。run-002 复现 run-001：修复构建 e2f02a1 的边身份错位为 0、限流误标 stop 为 0，但 45° 平台 hard-gate 系统性失败，且 0/454 子步合规；相邻滑移 5.1s 实测只实现 0.17°~0.45°，远低于 45° 平台 tan30 判据要求。此为验收与当前机制能力几何不相容，不归因于构建污染或已修复的 A/B 问题。

> 适用性更新（2026-10-20）：上方最新阶段边界裁决纠正了“30°线性坡面与45°参考几何不相容”的说法，并将全宽±45°目标包延期至阶段二。以下 tan30 形式 gate 豁免只作为补采 run 资格口径继续有效；它不定义阶段一运行时的提交安全规则。

1. **组合裁决：短期采用方案 1，中期按方案 2 重议目标，方案 3 仍留在阶段二排期。** 不提前启动悬浮平流实现；先让不受该冲突影响的标定转折数据继续采集，同时要求 designer 对机制能力上限和目标/机制路线提出量化复议依据。
2. **判据修订授权 tester 执行**：仅对 `+45°/-45°` 稳定平台及其 overshoot/return 子步，豁免 `|ΔS−tan(±45°)|≤tan30°` 这一参考线形式判定；保留 `0°/±15°` 以及非 45° 子步现有 tan30 判据。45° 子步继续执行并报告有限值、守恒、库存、容量、预算、字段/账本对齐、边身份和 stop 转换语义等其余闸门；不得将豁免描述为坡度达标、物理无越界或整体 run 自动合格。对补采资格而言，run 只在修订后的适用 hard-gate 全部通过后才可进入候选池；转折事件和 cell/bracket 仍按原 §8.11/补采执行单过滤、分组与样本门槛，原 run-001/002 不追认、不纳入 fit/holdout。
3. **明确非目标**：本裁决不改变 `30°±2°` 收敛目标、预算、几何、相位时长、转折阈值或样本配额；不声称现有相邻滑移机制能在 45° 达成 tan30 形式判据。若后续阶段一验收仍要求该机制实现不可能的 45° 参考线约束，designer 应按 §8.13/§8.14 提交机制能力上限与目标适配的正式重议，而不能靠再次补采解决。
4. **路由与前置**：
   - **tester（立即）**：同步修订 checker 与 `docs/test-cases.md`，以本裁决口径重测并更新报告；保留旧 run 与旧判定审计，不改物理、不删除其他 gate。新口径就位后再由 effector 按冻结 schedule 继续采集，仍先做单 run gate。
   - **designer（立即，机制/目标复议）**：依据 §8.13/§8.14 与停机报告量化说明 0.17°~0.45° 实测能力对 ±45° 目标、限幅+guard 与验收判据的影响，提出是否推迟目标及阶段一/二边界建议；方案 3 保持阶段二排期，除非后续正式复议另有裁决。
   - **developer WIP（下一次补采前置条件②）**：认领并说明主工作区两份 WIP 源文件 `P011TiltPlanningPass.ets`、`WaterSandField.ets` 的归属，整理为可审阅提交并修复当前 10 个 ArkTS 编译错误；提供成功构建证据。修复提交不得混入或覆盖 e2f02a1 补采基线；未满足前不得启动下一轮 gate run。
   - **effector**：本次停止扩批继续有效；待 tester 新 checker/cases 就位且 developer 前置条件②满足后，按同一冻结 schedule 与单 run gate 恢复采集。不得自行修改判据或 schedule。
5. **状态**：P-011 保持 `P0 / IN_PROGRESS`；观测③继续 `BLOCKED`，目前有效合格 run 为 `0/34`，本裁决不输出 `thetaStart/thetaStop`，不解除统计门槛。

## 2026-10-19 P-011 两阶段定案（历史；阶段一验收范围由 2026-10-20 阶段边界裁决更新）

> 日期：2026-10-19。原因：用户在 `slip-overshoot-v2` 条件性不可达结论（相邻输运 66 倍缺口）、方案 3（悬浮平流定向搬运，纯增加、理论出处 246/251/252 平流项）与「现有机制潜力未兑现」三者之间做出裁决：**先把当前方案（梯度驱动机制）能达到的效果做对，然后考虑引入方案 3**。本节确立两阶段结构，取代同日可达性复核裁决中的路线选择部分；其验收口径（硬约束/收敛/预算/账本对齐/OLS R²）全部保留并适用于阶段一。

1. **阶段一（当前执行）：现有梯度驱动机制的潜力全部兑现**。三项任务：
   - **A. 双角度滞回落地**（v2.12 `natural-relax-hysteresis-v1`）：逐边三态状态机 + 角度标定（依赖逐边转折观测，`thetaStart ∈ [25°,30°)` 严格排除上端点、`thetaStop` 由观测定）；
   - **B. 484 次越界修复**：tilt 参考更新动态可行域限幅（v2.14 设计）+ pre-commit admission guard（全通道 tan30° 提交前拦截）+ ledger 对齐修复（context 采样移至 updateTilt 之后）；
   - **C. 现有通道表层效果调对**：滑移收敛行为、安息角响应、±15° 静止回归。
2. **阶段一完成标准**：现有机制能产生的所有行为均正确——±15° 静止（已验证）、±45° 表层崩塌可见且渐进、守恒零漂移、127 边零越界、OLS R² ≥ 0.90、稳定终点悬沙 ≤ 5%。
3. **阶段二（评估后启动）：引入方案 3 悬浮平流定向搬运**。
   - **前提**：阶段一完成，且 `advectionGain=0` 的状态自然构成 A/B 对照基线；
   - **触发条件**：表层效果做对后，若「倾斜时沙向低处聚」的观感仍不足（缺失定向输运维度被实际确认）；
   - **实现形态**：纯新增平流项（漂移速度从 leveling 迁移场推导，量级由拖曳定标），与现有扩散项叠加；约 30 行新代码 + 1 个新参数，无 schema 变更、无现有代码改动、无用例修改；
   - **理论依据**：246 三相平流-扩散方程的平流项（深度平均降阶），非新物理。
4. **两阶段的衔接**：阶段一的观测数据（大角度崩塌行为、滑移速率-角度关系）恰好是阶段二平流项参数定标的输入；每阶段独立可验证，避免两改动混叠。
5. **流水线路由（阶段一）**：
   - **designer**：双角度标定所需的逐边转折观测协议补充 + tilt 限速方案的 architect 对接材料；
   - **tester**：P011-SLIP 用例固化 + 账本对齐核验（承接可达性复核裁决的任务）；
   - **architect**：待 designer（A+B 设计定稿）+ tester（用例就位）后进；
   - **developer**：等 architect + tester 用例，先 RED 后实现；
   - **体力驱动蠕动项**：保留为将来候选，登记不删，阶段二评估时一并考虑。
6. **不变量**：全部验收阈值不放宽、预算不提高、守恒审计照常、routeId 隔离、双报告独立成文；方案 3 引入时同样受「新增项自身守恒 + 可开关退化 + 不改现有通道」三重约束。

## 2026-10-19 P-011 slip-overshoot-v2 可达性复核裁决（历史；两阶段定案承接其验收口径）

> 日期：2026-10-19。原因：architect 复核确认 designer 的相邻输运下界与 raster 口径，并发现 tester 现行 `slope90TimeMs` 是相对本相位实测终值而非 30° 目标。manager 依此裁决路线、终点状态与正式验收口径。本节取代同日早前的上层沙调度口径，历史结论保留。

1. **可达性结论**：接受 `slip-overshoot-v2` 在当前 `maxAvalancheFlux=80/s`、`totalMotionBudget=300/s`、`5.1s` 时限与相邻输运路线下**条件性不可达**。全宽 OLS 斜率达 `30°` 需相邻输运总量至少 `100,893` units，`28°` 也需 `92,917`；对照 5.1s 总额度上限 `1,530`，缺口约 `66` 倍。中央单边需求 `>=794` units，也高于 5.1s 单边 cap `408`。该结论只否证相邻输运路线，不等于所有物理通道绝对无解。
2. **路线裁决**：维持 `30°±2°` 与 `5.1s`，不放宽目标、不增加预算、不改几何。`slip-overshoot-v2` 标记为 `BLOCKED（条件性不可达）`，本轮不交 architect/developer。授权 designer 对 `lift/settle/suspension-diffusion` 替代路线做**离线可达性审计**，只使用现有 `ledger.ndjson`，不写新物理、不改参数、不授权新观测。
3. **终点状态**：相位结束必须是**稳定/沉积完成**状态，不允许大量悬沙留在终点来制造瞬态 OLS 斜率。正式门槛：每个 `±45°` 相位最后 `1s` 的 OLS 斜率变化不超过 `1°`，且 `suspH+suspL <= 0.05 * 初始沉积沙总量`；若离线审计无法证明该稳定终点，不能放行。
4. **t90 口径**：正式 `t90` 必须是 **target-based**，即首次达到 30° 目标的 90%——约 `27°`，不是相对本相位实测终值的 90%。现有 `6.5s` 只能作为诊断，不得作为通过或失败结论。tester 须修正 checker 并同时保留 relative-final 值作诊断。
5. **raster 口径**：OLS 斜率仍是主判据，但不能只看斜率。正式验收须同时 gate 拟合优度 `R² >= 0.90`（128 列有效表面），并报告 RMSE；逐边 `tan30°` 硬约束仍独立生效。不得用局部噪声或瞬态悬沙拟合出斜率后放行。
6. **新观测授权**：本轮不授权新观测。仅当离线审计无法闭合时，manager 再决定是否授权一次**零物理改动**的 `lift/settle/suspension` 观测；该观测不得改参数、不得改物理、不得用于直接放行，只用于补足可达性证据。
7. **下一步流水线**：
   - **designer（立即）**：基于现有 `ledger.ndjson` 对 `lift/settle/suspension-diffusion` 做离线可达性审计，输出每通道的空间定向能力、库存/容量、总预算、稳定终点与 5.1s 内能否达到 `30°±2°` 的必要/充分界；不写新物理。
   - **tester（立即，与 designer 并行）**：修正 `slope90TimeMs` 为 target-based 口径，新增 OLS `R²/RMSE` 与终点稳定/悬沙阈值；用现有证据输出 RED 基线，不改物理。
   - **architect（等待 designer 审计）**：复核替代路线可达性与工程边界，确认是否值得进入新方案设计。
   - **developer（等待 architect + tester）**：在替代路线被接受且用例就位前不得实现；不得修参数或加预算。
   - 后续按 reviewer → tester → effector → designer 独立复盘闭环。

## 2026-10-19 P-011 上层沙优先与 slip-overshoot 违约裁决（历史；被同日可达性复核裁决取代）

> 日期：2026-10-19。原因：`gravity-advection-v2.11` 经数值复核被否决；新七相真机观测暴露 `484` 次 `tan30°` residual 越界、表层滑移未收敛到安息角，以及账本 `context/audit` 子步错位。用户已裁决先修上层沙，重沙机制押后。本节取代 2026-10-18 v2.9 的当前调度口径，历史结论保留。

1. **违约定性**：`484` 次 `audit.range=false` 是真实物理违约，不是单纯测量口径问题。独立复盘确认 `updateTilt()` 在 `substep()` 内先执行；首个 crossing 在任何 material grant 前，`effectiveTilt` 已推进到 `36°`，边 `c=57` 的 residual 已为 `-0.729661`，超过 `tan30°=0.577350`。后续 484 次失败集中在正向/负向 overshoot 及其 return 段，说明是持续的角度推进与材料响应失配，不是一次性数值抖动。
2. **口径缺陷**：首个 failure 的 `context effectiveTilt=24°` 与实际校验角度 `36°` 不一致，来源是 `context/stateBefore` 在 `substep()` 前采样，而 audit/grants 对应子步内更新后的状态。该缺陷影响事件标签与归因展示，不改变“真实违约”的结论；须由 tester 独立核验并修复观测对齐，不得用 `context=24°` 重新解释为通过。
3. **路由决定**：`P-011-A`（硬坡度违约 + 表层滑移收敛）直接路由 **designer** 修物理，不等 tester 先完成口径核查；**tester** 并行固化客观用例并修复/核验账本对齐。两条线可并行，但 developer 必须等 tester 用例就位且 architect 复审通过后才可实现。不得把本问题退回为“仅测量问题”。
4. **上层沙验收口径**：
   - 硬约束：同一七相序列 `0/+15/0/+45/0/-45/0` 下，每个提交子步的全部 `127` 条边均满足 `|S[c+1]-S[c]-tan(effectiveTilt_committed)| <= tan30°`，violation 数必须为 `0`；不得用事后修复或下一子步恢复放行。
   - 收敛：`+45` 与 `-45` 相位结束时，raster 可见线性坡度均须落在 `30° ± 2°`；从相位开始到 90% 收敛的 `t90 <= 5.1s`，`t50` 只作诊断记录。`±15` 无超角段保持回归，不得引入新违约。
   - 不变量：材料守恒、库存、容量、local/bottom/total 预算全部保持原阈值；不得增加预算、不得放宽 `tan30°`、不得预设目标形状或 ROI 偏置。
   - 观测：同一 ledger 记录必须同时给出 pre/post `effectiveTilt`、committed `sandTop` 与实际 audit residual，确保 `context` 与 audit/grants 属同一子步快照；不得覆盖旧证据。
   - 重沙：本轮不以重沙界面 `>=2px` 作为上层沙子目标的放行条件；该要求押后，只要求不引入守恒或预算回归。
5. **下一步流水线**：
   - **tester（立即，与 designer 并行）**：输入 `docs/observation-report-P011-slip-overshoot-v1.md`、`docs/retro-P011-v2.13.md`、`docs/algorithm-P011.md` §8.12~8.13。任务：独立核验 484 次违约的计算口径与 `updateTilt` 时序；把七相观测转正为 `P011-SLIP` 客观用例（硬约束、收敛、预算、守恒、账本对齐）；输出 RED 基线与正式报告，不改物理。
   - **designer（立即，与 tester 并行）**：完成 `slip-overshoot-v2` 的联合限幅设计，覆盖倾角参考速度、材料响应、pre-commit hard guard、共享预算竞争与观测对齐；不得重开 v2.11 重力交换，不得为追角增加预算。产出更新 `docs/algorithm-P011.md`，状态须等 tester 用例就位后再转 `READY_FOR_ARCHITECT_REVIEW`。
   - **architect（等待 designer + tester）**：把通过口径复核的设计翻译为工程方案，明确 pre-commit guard 的落点、账本字段、回归矩阵与实现边界；确认后才可 `READY_FOR_DEVELOPER`。
   - **developer（等待 architect + tester 用例）**：只实现上层沙修复与观测对齐，不改重沙机制、不改阈值与预算；先跑 tester 提供的 RED 用例，再交 reviewer。
   - **后续闭环**：developer→reviewer→tester 正式客观复测→effector 上层沙主观评估→designer 双报告独立复盘→manager。重沙机制另行排期。

## 2026-10-18 P-011 默认重层 FAIL 与 v2.9 设计调度（历史；被 2026-10-19 上层沙裁决取代）

> 日期：2026-10-18。原因：dual-input-v1 完成审核、安装后，tester 新包正式 green 预检确认默认重层响应未达硬门槛，effector 已追加前置 BLOCKED。P-011 仍 P0、`IN_PROGRESS（默认重层响应失败，转 designer v2.9）`；不是用例争议，不等待用户操作，不改门槛。

1. **已完成输入工程**：dual-input-v1 的39项实际产品测试独立重跑 PASS；模式冷热启动、首 Want、体验模式拒绝脚本、真实静态 sensor callback 和面板双向切换 smoke PASS。reviewer 业务与 host 审核 PASS，本地提交 `130a52c74e8964fb024677b672cad1965f5cf1db`。上述通过保留，不等于默认物理效果或正式双测通过。
2. **包与源码身份**：已安装 `test/artifacts/p011-v28-dual-input-20261018/entry-default-dual-input-v1-signed.hap`，SHA256 `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`。实际构建基线 `fdc8190`，sourceCommitType=`reviewed P011 baseline + recorded dirty source snapshot`；48源哈希 manifest 绑定包，不能宣称该 reviewer commit 单独 clean 重现。身份明细见最新客观报告与 `test/artifacts/p011-v28-dual-input-20261018/source-manifest.json`。
3. **最新硬前置 FAIL**：`docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 最新正式轮及 `test/artifacts/p011-v28-canonical-20261018/formal-gate-result.json` 确认默认128列/capacity192/max28，单 run script 0/+15/0/-15/0、seq1..5准确，±15各约5.1秒。heavy非零样本124/154达标，但RMS `1.0555e-7/5.3672e-7`（需≥0.5）、max `1.7731e-7/1.6499e-6`（需≥1.0）失败，green质心仅0.006/0.001px；light RMS可达1.719不能代替heavy。旧预检10列/capacity1000/max30与默认几何不同，旧PASS保留但不外推。
4. **立即路由 designer**：输入 `docs/goals.md`、最新P-011客观FAIL/主观 `BLOCKED(green_default_scene_gate_failed)` 双报告、上述gate JSON及算法/架构最新版本。期望在 `docs/algorithm-P011.md` 追加默认几何heavy静止的证据归因、候选方案/取舍与v2.9设计，保留双模式、全部阈值与10列压力回归；不得把非零flux计数当可见形变。随后 architect→developer→reviewer→tester默认green硬预检/模式回归/canonical531冻结审计/完整压力suite→effector自动连续录像视觉评估→designer双报告验证/独立复盘→manager。
5. **测试资产整改直路由 tester**：`p011_v28_preflight_check.py` 仅ROI存在即OBSERVATION_READY/exit0，漏实现已有heavy样本/RMS/max硬门槛。tester 按已有用例先补失败测试，再最小修工具并用当前失败证据复跑RED；不经过设计/业务链、不降低阈值、不覆写旧JSON，与designer文档设计无写冲突可并行。该工具标签不具有放行效力。
6. **执行闸门**：当前canonical531/冻结审计/压力suite与正式subjective均因默认green FAIL标BLOCKED，未执行项不伪记自身FAIL。原生录像能力已具备，effector仅host准备，正式采集需修复后新包硬前置与完整客观PASS、身份固定及设备串行交接。双模式通过、旧物理PASS、录像能力均不能据此CLOSED；最终仍需最新双报告及designer验证/复盘。

## 2026-10-18 P-011 业务开发状态同步（历史）

> 日期：2026-10-18。原因：双模式测试先行与设计前置已交付，流水线进入业务实现阶段。P-011 保持 P0、`IN_PROGRESS（双模式业务开发）`，本次不改变目标或任何门槛。

- **前置完成**：tester `docs/test-cases.md` §10 MODE01~07 及 RED 基线、effector 自动姿态双模式协议及 RED 基线已交付；designer `docs/algorithm-P011.md` §5.8 与独立复盘、architect `docs/architecture-P011.md` §15 `READY_FOR_DEVELOPER` 已交付。
- **当前执行**：developer `p011_input_impl` 正在实现两模式输入隔离及真实 helper unit；host capture/compact 整改已有 19 项回归、PowerShell 与 build 检查，reviewer 已 PASS，尚未 commit。业务实现尚未审核，也未完成新 HAP 正式双测。
- **录制前置解除**：原生录像自动启动、停止、导出能力已实测成功，设备已释放给业务 developer。历史 `BLOCKED(no_screen_recording)` 保留为历史结论，当前能力可进入后续正式采集准备；能力验证不等于主观效果 PASS。
- **下一接力**：业务 developer 交付→reviewer 统一复审并定向提交→记录匹配新 HAP/源码/日志身份→tester canonical precheck + objective→effector 自动连续录像与视觉评估→designer 双报告验证/独立复盘→manager。设备实测串行；原阈值、审计及独立双报告条件保持。

## 2026-10-18 P-011 双模式输入决策（历史前置；输入策略继续有效）

> 日期：2026-10-18。原因：用户正式要求测试模式自行构造手机倾斜状态、体验模式读取真实传感器。输入与验收方式变更已获用户授权，不再要求用户手持操作或代录；物理机制与全部验收阈值不变。

- **归属/状态**：保留 P-011、P0，当前 `IN_PROGRESS（双模式设计前置 + 验证/录制整改）`，不新增独立问题 ID。测试模式生成可复现姿态，体验模式读真实传感器，输入互斥并共用后续处理链路；目标见 `docs/goals.md` 当日双模式说明。
- **测试先行**：tester/effector 输入当日目标与各自 P-011 用例，先补模式隔离、序列复现、进入/退出/重启、传感器接线与自动视觉证据用例及可行 RED 基线；按用户授权追加模拟姿态替代人工输入，不放宽 HG-TT-04、≥6/10 或其他门槛。设计可先准备，输入模式实现等待用例就位。
- **设计交接**：designer 输入上述目标/用例，追加 `docs/algorithm-P011.md` 输入契约、方向/时间语义、共用链路与能力范围；architect 据此追加 `docs/architecture-P011.md` 模式入口、生成器、传感器订阅/滤波隔离、pending/script/forceInvert 状态重置及观测/runner 接入方案。仅输入工程，不扩大物理机制。
- **汇合/路由**：当前 developer 的 host capture/compact 整改继续；双模式用例→designer→architect→developer 与其汇合→reviewer 审核/提交及匹配 HAP→tester canonical precheck+objective→effector 自动视觉评估→designer 双报告验证/独立复盘→manager。共享设备执行串行。
- **录制与边界**：保留历史 `BLOCKED(no_screen_recording)` 至取得连续视频能力；effector 自行探测系统录制 UI 等能力，探测不算正式评分。合成姿态可完成自动验证，无需用户手持/代录；连续视频、hilog 时间对齐与源码/HAP/序列/日志身份要求不变。报告明确自动输入范围，未经实测不声称真实传感器噪声、标定或人体体验已验证。

## 当前优先级

| 优先级 | 问题 ID | 状态 | 路由 | 依赖 / 前置 | 备注 |
|---|---|---|---|---|---|
| P0 | P-003 | IN_PROGRESS | designer → architect → developer | **P-005 用例就位**；v2.2 修订与 developer 修复闭环 | B3，场景二三相模型；一级体验优先：翻转重力响应 + 倒扣轻沙上浮分选；二级：摇晃混乱 + 再沉淀 |
| P0 | P-005 | IN_PROGRESS | tester + effector 并行 | 一级体验动态指标补齐；与 developer 修复并行 | 场景二测试用例与判定脚本缺口；客观交 tester，主观交 effector；是 P-003 的前置 |
| P1 | P-001 | OPEN | tester + effector（实测）→ 视失败归因回流 designer/architect/developer | **P-006 已闭环**；场景一客观/主观测试资产可执行，可进入实测排期 | B1/M1’，场景一核心能力已实现但未完成客观/主观实测闭环 |
| P1 | P-006 | CLOSED | tester | 2026-10-15 manager 复验通过 | 场景一测试资产缺口已清零：客观脚本、正式 JSON/Markdown 报告、主观证据模板与三件套输入校验均可执行 |
| P2 | P-002 | OPEN | designer → architect → developer | 相关客观/主观用例就位 | B2/M3，倒立渲染 want 路径已 PASS，面板路径与完整回归未闭环 |
| P2 | P-004 | OPEN | tester + effector 并行 | B3 与 B1/B2 完成后推进 | B4，全量回归与判定体系；依赖前置实现与用例齐备 |
| P1 | P-007 | IN_PROGRESS | designer → developer → tester | v2.4 设计已就位，待 developer 实现并进入验证 | slopeLimit 姿态修正：`|d - tan(effectiveTiltDeg)| > tan(α)`；`α` 语义为“崩塌角” |
| P2 | P-008 | OPEN | designer → architect → developer | G2+G3 主线闭环后推进 | percolationRate 参数化与干湿门控 |
| P0 | P-009 | IN_PROGRESS | architect → tester | 立即并行，不阻塞 v2 基线提交 | WS_SNAPSHOT 4KB hilog 截断，阻塞 E2E 客观验收 |
| P2 | P-010 | CLOSED | architect | 2026-10-15 manager 复核通过 | architecture §10 已包含“本节预算接口已被 §11 v2.2 取代”导航提示 |
| P0 | P-011 | IN_PROGRESS（slip-overshoot-v2 条件性不可达，转 lift/settle/suspension 离线审计） | **designer（离线可达性审计）∥ tester（target-based t90 + 拟合优度/稳定终点口径）→ architect → developer → reviewer → tester → effector → designer独立复盘 → manager** | 维持 `30°±2°`、`5.1s`、预算与几何不变；重沙另行排期 | 相邻输运缺口约 66 倍；不得用瞬态悬沙或仅 OLS 斜率放行 |

状态说明：OPEN → IN_PROGRESS（designer 已开工）→ VALIDATING（测试中）→ CLOSED（双测试达标，designer 验证通过）

## 2026-10-15 P-006 验收结论（当前生效）

1. **复验通过**：`test/liquid_mode_check.py` docstring 与结果 `problemId`、`test/reports/p006_liquid_mode_check.json`、`docs/test-report-P006.md` 均已统一标识 `P-006`；未再检出 `P-001` 遗留。manager 临时输出复跑自检结果为 `PASS`。
2. **TC-12 通过**：硬门槛 `HG-RECOVERY-TIME` 断言 `recoveryTime <= 4s` 且稳定 1 秒；峰值混色、恢复时间、终态混色三项 TIERED 映射齐全，最低分为及格线 6。最新自检实测恢复时间 `3.308s`，TC-12 最低分 6。
3. **TC-13 与帧输入通过**：脚本校验事件边界外的质心位移与像素分类变化连续性，且 TC-13 TIERED 最低分达标；帧目录、时间戳 manifest、颜色标定 JSON 必须三件套成组输入，并执行存在性、严格递增、目录一致性、颜色空间/颜色值与阈值校验。
4. **闭环结论**：P-006 是测试资产缺口，客观脚本、正式报告与主观证据模板均已可执行，标记 `CLOSED` 并移出 `docs/gap.md`。P-001 的测试前置解除，可进入场景一实测排期。

## 2026-10-18 P-011 验证流程整改与接力（历史；被 2026-10-19 上层沙裁决取代）

> 日期：2026-10-18。原因：继续流水线时复核到最新 architecture §7.5.6/§7.5.7 的采集生命周期要求尚未实现；原 v2.8 客观报告未携带完整冻结/身份审计。此节取代下方历史路由，目标、算法、验收阈值不变；本轮仅聚焦 P-011。

1. **现状与优先级**：P-011 保持 P0。底层倾斜响应直接影响 G2+G3 一级体验，当前阻断可限定为测试采集链路的最小整改，成本低于重设物理机制。历史 `docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md` 的 PASS、531/531 与行为指标作为已有证据保留，但其 JSON 缺少 `capturePath`、`captureFrozen`、`captureAudit`，不能充当新增闸门的 PASS。reviewer 首轮已明确该阻断项，随后因 workspace requirements 环境异常终止，尚未完成正式复审/提交；这不是物理机制失败结论。
2. **测试先行及责任边界**：已有 `docs/test-cases.md` P-011 §9、`docs/effect-cases.md` P-011 及 architecture §7.5.6/§7.5.7 的明确判定要求。一般测试用例/断言缺口仍直接路由 tester/effector；本次是已进入 developer 的 reviewer 阻断项整改，只允许按“先补阻断项测试，再最小实现修改”处理，禁止扩大到业务/算法或调整阈值。tester 在正式复测前核对这些验证项与用例描述一致，不足时先补测试资产及 RED 基线。
3. **developer → reviewer**：输入为 `docs/algorithm-P011.md` §5.7、`docs/architecture-P011.md` §7.5.6/§7.5.7 与 reviewer 阻断清单。仅整改 `test/p011_tc02_formal_suite.ps1`、`test/p011_tc02_schedule.py`、`test/p011_check.py` 及对应测试：formal suite 和 `schedule-precheck-only` 共用唯一 lifecycle；同步注入结束后停止采集并冻结；输出 canonical 身份与完整 pre/post size/mtime/SHA256 审计；拒绝活动日志、残留句柄、`capturePathMismatch`、`captureNotFrozen`。期望产出“阻断项 → 最小修改 → 测试证据”映射供 reviewer 复审。
4. **reviewer → tester**：复审通过并定向 git 提交后，交付实际新 commit、相关源文件身份、HAP SHA256、检查结果。现有 HAP 哈希一致不等于源码已被旧 commit 完整收录；`fdc8190` 只是原构建/历史代码基线，当前 v2.8 业务改动仍有 dirty 内容，不得标作完整 v2.8 源码身份。若沿用现有 HAP，正式报告必须同时注明原 build 基线、reviewer 新 commit、与包内容对应的源文件身份及 HAP 哈希，明确测试脚本整改与包构建的关系。
5. **tester → effector**：tester 先通过同一 canonical runner 执行独立 schedule precheck，再执行全量 objective。必须携带相同 schedule CSV/注入脚本 SHA256、命令模板、shell 间隔与 hilog 参数，验证 capture audit，随后验证单 run、531/531、连续 sequence、accepted gap ≥100ms、target 一致及全部既有观测/预算/守恒/稳定性门槛。期望在独立 v2.8 客观报告中追加新一轮结果与全部身份、日志哈希；保留原 PASS 和历史失败，不追认旧证据满足新闸门。未通过前不得进入主观正式采集。
6. **effector → designer**：effector 当前仅静态准备采集工具；旧后台主观批次已定向停止，其证据保留为探索材料。tester 交付当前闸门 PASS 后，effector 接管同一设备，按 `docs/effect-cases.md` 独立产出 `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md`、录像/逐帧证据和身份/哈希；不得复用 v2.7.x 或探索批次替代正式验收。`HG-TT-04` 必须通过，TIERED 仍为 0~10 分且 ≥6；共享设备全部安装、操作和 hilog 采集串行。
7. **designer → manager**：输入为同 route、可追溯 HAP 身份的最新双报告与原始证据。designer 必须验证已解决硬门槛保持、软门槛及格、当前问题改善，给出 `VALIDATED` 或未达成归因，并沉淀独立 `docs/retro-P011-v*.md`（可扩展已有相应轮次复盘，须注明日期原因）。manager 仅在双测达标且 designer 验证/独立复盘完成后 CLOSED 并移出 gap；未达成则按算法/工程实现/测试资产归因回流，不自动修改用例。

## 2026-10-18 P-011 v2.7.3 双测复核（历史；路由由上节取代）

1. **客观结果**：`docs/test-report-P011-bottom-creep-v2.7.3-20261018.md` 记录 routeId=`bottom-creep-v2.7.3`，总体 **PASS**；TC02 schedule 校验 531/531，硬门槛与 TC02 行为分档通过。reviewer 提交 `fdc8190` 可作为当前代码基线。
2. **主观结果**：`docs/effect-report-P011-bottom-creep-v2.7.3.md` 总体 **FAIL**。水与轻沙可见响应，但绿色底层在所有有效脚本倾斜场景中保持视觉静止，触发 `HG-TT-04`；所有底层相关 TIERED 维度低于及格线 6。证据根目录为 `demo/effect/P011-v273/`。
3. **闭环判定**：不 CLOSED。客观 PASS 与主观 FAIL 构成观测分歧，必须先归因：客观 telemetry 是否命中了用户不可见的状态、场景/颜色/证据采样是否遮蔽底层，或算法/实现是否实际未驱动底层。
4. **designer 复核**：`docs/algorithm-P011.md` §5 定义 `bottom-creep-v2.8-heavy-interface`，状态 `DESIGN_PENDING_APPROVAL`。归因：绿色层渲染绑定 `bh`，v2.7.3 优先消耗 `bl`，正式数据 `maxSubstepBh=0`、`maxSubstepBl>0`，客观 checker 把聚合位移复制为 H/L，因此 PASS 不代表绿色 heavy 层移动。
5. **architect 复核**：`docs/architecture-P011.md` 已整体切到 `bottom-creep-v2.8-heavy-interface`，状态 `READY_FOR_DEVELOPER`；批准 phase-balanced split、35% heavy floor、compact telemetry、schema v2 checker isolation 与 green-ROI preflight。阈值与硬门槛不变。
6. **developer 复核**：v2.8 实现自检 `p011-selfcheck.json` 总体 PASS，schema v2；Hvigor 编译/打包通过并产出已签名 HAP。green-ROI preflight 为 `OBSERVATION_READY`。已知 TC02 schedule precheck FAIL 属于已单独归因的输入观测状态机问题，不作为 v2.8 行为验收结论。
7. **tester 复核**：v2.8 客观报告总体 PASS，green-ROI preflight `OBSERVATION_READY`；TC02 schedule precheck 531/531、0 gap violation；formal suite 所有 MUST-PASS 通过，TC01/02/03 为 10/8/10。证据与哈希独立成文于 `docs/test-report-P011-bottom-creep-v2.8-heavy-interface.md`。
8. **路由**：P-011 → **effector**。输入：v2.8 签名 HAP、主观用例、客观报告与 green-ROI 证据。期望产出：独立 `docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md`；主观 PASS 后交 manager 最终复核。不得合并 v2.7.x 或放宽 HG-TT-04。
9. **effector 阻塞**：`docs/effect-report-P011-bottom-creep-v2.8-heavy-interface.md` 正式判定为 `BLOCKED(no_screen_recording)`。设备原生 `uitest` 无屏幕录像能力，连续性、瞬移、撕裂、冻结与抖动门槛无法正式判定；9 轮探索证据仅存于 `demo/effect/P011-v28-heavy-interface/subjective/`，不评分、不判 HG-TT-04 PASS。解除条件：恢复设备原生/外部连续屏幕录制或用户手动采集同 run 连续视频，并提供录制与 hilog 时间对齐；解锁后继续 effector 复测。

## 2026-10-16 P-011 调度（历史）

1. **设计状态**：`docs/algorithm-P011.md` §3 已将 `bottom-creep-v2.7` 标记为 `READY_FOR_DEVELOPER`；`docs/architecture-P011.md` 同步状态，并给出实现步骤、测试钩子与验收口径。
2. **路由**：P-011 → **developer + tester + effector 并行**。developer 按算法与工程方案实现业务代码；tester 完善 `test/p011_check.py` 与客观报告资产；effector 准备 `bottom-creep-v2.7` 独立主观场景、录像与逐帧证据模板。三角色不得修改 manager 独占文档。
3. **串行约束**：tester 与 effector 本轮仅准备测试资产，不启动真机/模拟器执行；后续实测须串行或分设备，避免同机资源竞争。
4. **验收前置**：开发实现必须输出 `P011_EVENT input`、`P011_METRIC`、`P011_BOTTOM` 钩子，且报告固定 `routeId=bottom-creep-v2.7`；闭环仍以客观与主观双测达标为准。
5. **主观阈值裁定（2026-10-16）**：`docs/effect-cases.md` P-011 已定义 0~10 分、及格线 6；`docs/algorithm-P011.md` §3.7 与 `docs/architecture-P011.md` §9.2 的 `3.5/5` 属口径冲突。manager 裁定以 **6/10** 为准，designer/architect 需在闭环前同步修正各自文档；本轮不阻塞 developer、tester、effector 并行。

## 2026-10-15 v2.5 冻结条件复核

1. **复核通过**：`docs/algorithm-P003-P005.md` §20.1 已明确 `effectiveTilt > 0` 时左列为 donor、右列为 receiver；已采用独立 `vBottomCreepMax=10 列/秒` 子通道预算；§18.1 已明确 `slopeLimit` 中 `α` 的语义为“崩塌角”，不是最终收敛角。
2. **调度结论**：三项冻结条件在设计层已清零。因该文档 §20 同时将 v2.5 标记为 `REJECTED`，不恢复 v2.5 实施；P-011 后续指向 `docs/algorithm-P011.md` §3 的 `bottom-creep-v2.7`。
3. **路由**：P-011 → **designer**。输入：`docs/algorithm-P011.md`、`docs/algorithm-P003-P005.md` §18/§20、P-011 客观与主观报告。期望产出：将 `bottom-creep-v2.7` 从 `PROPOSED` 推进到 `READY_FOR_DEVELOPER`，保留上述三项语义、补齐总预算与观测钩子口径，并给出 tester/effector 验收断言；不得修改业务代码。
4. **测试现状**：P-011 客观报告仍因输入时间戳、逐子步 `effectiveTilt` 与 `bh/bl` 钩子缺失而 `BLOCKED`；主观报告因缺少录像与逐帧证据无法评分。闭环仍以双测达标为准。

## 当前阻塞

> 2026-10-14 更新：本节为 2026-10-13 历史阻塞记录；当前由下方“2026-10-14 G2 体验优先级与推进口径（当前生效）”取代，P-003 已解除 `BLOCKED`，保持 `IN_PROGRESS`。

**阻塞项**：`docs/algorithm-P003-P005.md` §3.3 的渗透公式 `hl[c] -= q; hs[c] += q` 会把轻沙转换为重沙，导致 `VH` 增加、`VL` 减少，只保持 `VH + VL` 不变，无法满足三相各自守恒。

**影响与路由**：

1. P-003 不能进入开发实现；developer 不得实现该公式。
2. P-005 不被该阻塞项卡住：tester/effector 可先补客观/主观用例，并把三相守恒写成硬断言，用于暴露此类公式错误。
3. P-003 保持 `IN_PROGRESS（BLOCKED）`；P-005 保持 `IN_PROGRESS`，按新路由分别交 tester 与 effector。
4. **测试先行**：P-003 在 `docs/test-cases.md` / `docs/effect-cases.md` 仍为占位的情况下，不得先交 designer 修订；必须等 P-005 的客观/主观用例就位后再进入实现闭环。

**下一步**：

1. P-005 → tester + effector：立即并行补客观/主观用例，不经过 designer/architect/developer；客观用例必须包含三相守恒断言。
2. P-003 → designer：待 P-005 用例就位后，修订 `docs/algorithm-P003-P005.md` §3.3，给出保相守恒的渗透/分选表达，或明确 v1 禁用该子机制并改由沉降顺序与表层滑移实现分选。
3. P-003 后续仍需 architect 更新工程方案；developer 待 designer 修订且 architect 确认后才可开工。

## 本轮选定

**问题 ID：P-003 + P-005（分路推进）**

### 选择理由

- 用户最新指令明确“场景二（水中两种重量的沙，堆积成形）优先，其次是场景一（两种不互溶液体油水分层）”，因此 B3/P-003 提升为当前最高优先级。
- P-003 是 G2 的最小可验证核心，直接覆盖三相守恒、重沙/轻沙分选、安息角、摇晃悬浮与再堆积等场景二关键目标。
- P-005 是 P-003 的验证前置：当前 `docs/test-cases.md`、`docs/effect-cases.md` 均为占位，仓库中也没有 WaterSandField/三相判定脚本；不补齐测试用例，P-003 无法客观/主观闭环。
- 当前 `test-report.md` 与 `effect-report.md` 均无有效数据，优先完成 P-003 + P-005 可先建立场景二的客观守恒、堆积/分选与主观观感基线，再反哺场景一的倒立与渲染验收。
- 修复成本相对可控：P-003 主要新增 WaterSandField 列标量三相模型、第四渲染模式与 Node 单测；P-005 可复用现有两相脚本框架扩展为三相与主观用例。

### 验收要求

1. **P-003 客观验收**：多轮旋转、倒立与摇晃角序列下，water、Σhs、Σhl 各自总量守恒，漂移 < 0.1%；susp ↔ 沙堆只做相内转移，不改变各相总量。
2. **P-003 分选与堆积**：静置后重沙在沙堆上 1/3 的占比 / 下 1/3 的占比 ≤ 0.5（初值）；沙堆轮廓平滑、坡度不超过安息角上限；长时静置零漂移。
3. **P-003 动态表现**：重沙沉降快于轻沙（vH > vL）；摇晃时沙堆表层按比例扬起为悬浮态，E 衰减后重新沉降并堆积；全过程连续平滑，无粒子瞬移，实测帧率 ≥ 100fps。
4. **P-005 客观测试用例**：产出可执行用例，覆盖三相守恒、分选比、沉降时序、安息角、水面水平/倾斜、倒立相序、摇晃→悬浮→再堆积、长时静置零漂移与 FPS；实现或扩展判定脚本以支持 VV-07/08/09 与三相像素/粒子分类。
5. **P-005 主观测试用例**：产出可复现的主观用例，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷；评分标准需可量化。
6. **闭环条件**：P-003 与 P-005 均需 tester 的客观测试与 effector 的主观效果测试达标；任一不达标则保持 OPEN，不得仅凭实现完成或用例编写完成而关闭。

### 路由与交接说明

#### P-003 → designer

- **前置条件**：P-005 的客观/主观用例已就位，能验证三相守恒、分选、堆积、沉降、水面、倒立与主观观感。
- **输入文档**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四（B3）、`docs/algorithm-P003-P005.md` v1、`docs/architecture-P003-P005.md` §0.1、补齐后的 `docs/test-cases.md`、`docs/effect-cases.md`。
- **期望产出**：修订 `docs/algorithm-P003-P005.md` §3.3，保持 `VH = Σ(hs + suspH)` 与 `VL = Σ(hl + suspL)` 各自守恒；如 v1 无法给出保相渗透公式，应明确禁用该子机制并说明替代分选机制，同时在变更记录中写明原因。
- **必须包含**：不改变其他章节的修订说明；替换公式或明确的禁用策略；对测试钩子的影响；以及 developer 可据此实现的伪代码。
- **后续依赖**：designer 修订完成并经 architect 确认后，developer 才可按更新后的 `docs/architecture-P003-P005.md` 开工。

#### P-005 → tester + effector

- **tester 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/test-cases.md`、`test/TC_TEST_CASES.md`、`test/vv_s75_analysis.py`。
- **tester 期望产出**：补齐 `docs/test-cases.md` 中 P-005 的客观用例与可执行判定脚本，覆盖三相守恒、分选比、沉降时序、安息角、水面、倒立、摇晃→悬浮→再堆积、长时静置与 FPS。
- **effector 输入**：`docs/PROJECT_GOALS.md`（场景二）、`docs/DESIGN_GAP_CLOSE.md` §四/§五、`docs/priority.md`、`docs/effect-cases.md`。
- **effector 期望产出**：补齐 `docs/effect-cases.md` 中 P-005 的主观场景与评分标准，覆盖水面观感、沙丘轮廓、重沙/轻沙分区、混合带、摇晃悬浮与再堆积自然度、卡顿/闪烁/瞬移等缺陷。
- **并行规则**：tester 与 effector 可并行；二者不修改 algorithm/architecture/developer 代码，不等待 P-003 修复。
- **测试先行**：P-005 是 P-003 的前置；用例未就位前，P-003 不进入 designer 修订，developer 更不得开工。

### 2026-10-14 G2 体验优先级与推进口径（当前生效）

**用户指令**：G2 实现优先级为：

1. **一级**：手机翻转时沙子随重力移动，覆盖重力方向连续变化时的响应。
2. **一级**：手机倒扣时，原位于下方的轻沙慢慢上浮、重沙下沉，完成分选反转的动态过程。
3. **二级**：剧烈摇晃时的混乱效果，包含悬浮/扬起。
4. **二级**：混乱后的沉淀与再堆积效果。

**放行规则**：

- 一级体验不达标时，二级项不作为放行理由。
- 二级项的验收要求不降低，仅调整推进顺序；摇晃混乱与再沉淀仍须按原阈值闭环。
- P-003 的 v2.2 修订与 developer 修复，应优先保证一级体验的物理正确性与观感自然度，再优化二级混乱与沉淀链路。
- P-005 的 tester/effector 应优先补齐并执行一级体验的动态验收，再执行二级体验回归。

**测试覆盖评估（2026-10-14）**：

- `TC-WS-07` 目前覆盖 180° 倒立后的最终相序与守恒，但对“重力方向连续变化时沙的响应”以及“倒扣后轻沙上浮、重沙下沉的分选反转动态”覆盖不足；需补连续重力扫描、倒置过程时序和相序交换时间指标。
- `EC-WS-07` 覆盖倒立/翻回的最终形态与过渡自然度，但对倒扣后轻沙慢慢上浮、重沙下沉的过程缺少明确时序与评分锚点；需补充过程动态与节奏评分。
- `TC-WS-08`、`TC-WS-09` 与 `EC-WS-06` 已分别覆盖摇晃扬起、再沉降、再堆积与观感节奏，与二级体验优先级基本对齐；后续只需按原阈值执行，不降低要求。

**当前推进状态**：

- P-003 保持 `IN_PROGRESS`：designer 正在产出 v2.2 修订，developer 正在修复实现问题；二者不得因二级效果达标而提前放行一级缺口。
- P-005 保持 `IN_PROGRESS`：tester 与 effector 需并行补齐一级体验的客观/主观动态指标，二级项排在一级之后执行。

### 2026-10-14 跨角色同步决策与路由

#### G3 事件式 90° 语义

- **决策**：G3 当前目标采用“±89° 连续重力变化 + 90° 由经审计的 `invert180()` 事件跨越”的事件式语义；严格连续过 90° 不纳入本期验收，如有需要另立 v3。
- **已更新**：`docs/goals.md` 的 G3 指标与变更记录。

#### `docs/effect-cases.md` H06 硬门槛措辞

- **评估结论**：外部评审指出 H06 现有“倒立/翻回状态破坏，含数据模型被改写→FAIL”与经审计的 `invert180()` 物理事件设计冲突，结论成立。
- **路由**：P-005 → **effector**。
- **输入文档**：`docs/effect-cases.md` 的 H06 硬门槛条目。
- **期望产出**：将 H06 修改为“绕过 API 私改内部数组/状态 = FAIL；经 `invert180()` 且通过守恒审计，不因事件式跨越 90° 本身判 FAIL”。
- **原因**：区分违规改写与合法事件式翻转，避免误伤经审计的实现。

### 2026-10-14 v2.2/v2.3 批量路由决策

| 事项 | 决策 | 路由 | 优先级 | 执行时机 |
|---|---|---|---|---|
| H06 硬门槛措辞 | 同步为“绕过 API 私改物理数组=FAIL；经 `invert180()`、带事件日志且过三相守恒审计的坐标切换不判 FAIL” | **effector** | P0 | 立即执行 |
| HG-04 单帧位移口径 | 明确为“物理子步”检查；30Hz 两子步分别校验，不得相加后冒充单子步 | **tester** | P0 | 立即执行 |
| TC-WS-08 分相悬浮记录 | 增加 `suspensionFractionHeavy/Light` 分相指标与记录 | **tester** | P0 | 立即执行 |
| slopeLimit 姿态修正 | 认定为真实算法缺陷，批准为独立小修，不并入 v3；正确判据为 `|d - tan(tiltDeg)| > tan(α)` | **designer → developer → tester** | P1 | P-003 当前修复完成后立即执行 |
| percolationRate 常数 | 当前固定场景可作工程近似；后续需参数化并增加干湿门控 | **designer → architect → developer** | P2 | G2+G3 主线闭环后推进 |
| WS_SNAPSHOT 4KB 截断 | 登记 E2E 客观验收阻塞项；需 architect 确认快照分片或文件落盘方案，tester 同步解析与校验 | **architect → tester** | P0 | 立即并行，不阻塞 v2 基线提交 |
| architecture §10 取代提示 | 增加“本节已被 §11 v2.2 取代”导航行 | **architect** | P2 | 下一轮文档维护 |

### 2026-10-14 等待期可并行启动项

> 2026-10-15 备注：本节为历史调度记录。P-006 已复验通过并闭环；P-007 的算法设计已由 v2.4 §18 完成；P-011 的后续调度以 2026-10-19 上层沙裁决为准。

> 目标：在 P-003/P-005 主线等待 developer/reviewer/tester 期间，优先补齐测试用例与支撑能力，不抢占场景二主线资源。

| 问题 ID | 类型 | 现有用例情况 | 可启动内容 | 路由 | 启动时机 | 与主线关系 |
|---|---|---|---|---|---|---|
| P-006 | 测试用例缺口 | 客观：`test/TC_TEST_CASES.md` 已有 TC-11~13 草案；主观：`docs/effect-cases.md` 仍为占位 | 将 TC-11~13 转正并补齐场景一客观/主观用例与判定脚本 | **tester + effector 并行** | 立即启动 | 不阻塞 G2+G3 主线；为 G1+G3 支线提前铺路 |
| P-009 | 测试支撑缺口 | 无可执行的快照分片/落盘方案 | 确认 `WS_SNAPSHOT` 分片输出或文件落盘方案，并同步 tester 解析与校验 | **architect → tester** | 立即启动 | 支撑 G2+G3 主线 E2E 客观验收 |
| P-010 | 文档支撑缺口 | 无 | 在 architecture §10 增加“本节已被 §11 v2.2 取代”导航行 | **architect** | 立即启动 | 低成本文档修正，不阻塞主线 |
| P-004 | 回归测试缺口 | 客观/主观用例均未成体系 | 先行设计两场景组合线的全量回归矩阵与判定脚本框架 | **tester + effector** | 可预研，执行待 P-003/P-005 就位；P-006 前置已解除 | 为 G2+G3 与 G1+G3 最终回归做准备 |
| P-007 | 功能缺口 | 客观用例尚未覆盖倾斜姿态修正 | designer 可先行准备 `slopeLimit` 修正方案与测试预期 | **designer** | 可预研，实施待 P-003 当前修复闭环 | 不抢占主线；避免后续返工 |

### 2026-10-14 子目标排序与路由

> 全局排序遵循组合目标线 **G2+G3 → G1+G3**；G3 不是独立交付项，而是分别并入两条组合线的共享容器动态能力。组内按 **重要 > 视觉效果好 > 容易实现** 排列。`需补用例` 表示现有 TC/EC 覆盖不足，需由 tester/effector 先补齐，再进入实现闭环。

#### G2+G3 主线（当前优先）

| 排序 | 子目标 | 重要度 | 视觉效果 | 实现难度 | 当前路由 | 对应用例 / 需补用例 |
|---|---|---|---|---|---|---|
| 1 | G2.1 三相守恒与静置分选基线 | 高 | 中 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-01~04`、`TC-WS-10`；`EC-WS-01`、`EC-WS-03`、`EC-WS-04` |
| 2 | G2.2 静态倾角响应 | 高 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-05~06`；`EC-WS-02`、`EC-WS-05` |
| 3 | G2.3 连续翻转时序 | 高 | 高 | 高 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补连续重力扫描与时序用例 |
| 4 | G2.4 倒置分选动态 | 高 | 高 | 高 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补倒置过程时序与交换完成度用例 |
| 5 | G2.5 相序交换完成度 | 高 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-07`、`EC-WS-07`；需补交换完成度评分用例 |
| 6 | G2.6 摇晃混乱与再沉淀 | 中 | 高 | 中 | P-003/P-005：designer + developer + tester + effector | `TC-WS-08~09`；`EC-WS-06` |
| 7 | G2.7 渲染一致性与流畅度 | 中 | 高 | 低 | P-003/P-005：designer + developer + tester + effector | `TC-WS-11~12`；`EC-WS-08` |
| 8 | G3.1 事件式 90° 跨越语义 | 高 | 中 | 中 | P-003/P-005/P-004：developer + tester + effector | `TC-WS-07`、`EC-WS-07` 部分覆盖；需补专项事件语义用例 |
| 9 | G3.2 动态守恒与状态稳定 | 高 | 中 | 中 | P-003/P-005/P-004：developer + tester + effector | `TC-WS-01`、`TC-WS-10`；`EC-WS-08` |
| 10 | G3.3 长时回归矩阵 | 高 | 中 | 高 | P-003/P-005/P-004：developer + tester + effector | 需补全量回归用例 |

#### G1+G3 支线（另行开题）

| 排序 | 子目标 | 重要度 | 视觉效果 | 实现难度 | 当前路由 | 对应用例 / 需补用例 |
|---|---|---|---|---|---|---|
| 1 | G1.1 双相守恒与基础分层 | 高 | 中 | 中 | P-001：tester + effector | `TC-11`、`EC-OL-01`；P-006 用例已就位，待实测 |
| 2 | G1.2 翻转交换时序 | 高 | 高 | 中 | P-001：tester + effector | `TC-13`、`EC-OL-03`；P-006 用例已就位，待实测 |
| 3 | G1.3 界面恢复与视觉稳定 | 高 | 高 | 中 | P-001：tester + effector | `TC-12`、`EC-OL-02`；P-006 用例已就位，待实测 |
| 4 | G3.1 事件式 90° 跨越语义 | 高 | 中 | 中 | P-001/P-004：developer + tester + effector | `TC-13` 部分覆盖；需补专项事件语义用例 |
| 5 | G3.2 动态守恒与状态稳定 | 高 | 中 | 中 | P-001/P-004：tester + effector | `TC-11~13`、`EC-OL-01~03`；P-006 用例已就位，待实测 |
| 6 | G3.3 长时回归矩阵 | 高 | 中 | 高 | P-001/P-004：tester + effector | 需补全量回归用例 |

## 已闭环

| 问题 ID | 闭环日期 | 结论 |
|---|---|---|
| P-006 | 2026-10-15 | 场景一测试资产补齐并复验通过；客观脚本与正式报告统一为 P-006，TC-12/TC-13 判定、TIERED 映射、帧输入三件套与主观证据模板均满足执行要求 |
| P-010 | 2026-10-15 | `docs/architecture-P003-P005.md` §10 已加入“本节预算接口已被 §11 v2.2 取代；本节仅保留为历史设计记录”的导航提示 |

## 变更记录

- 2026-10-19：原因：architect 复核确认 slip-overshoot-v2 相邻输运下界与 raster 口径。manager 接受条件性不可达，维持 30°±2°/5.1s/预算不变，改路由 designer 做 lift/settle/suspension 离线可达性审计、tester 修正 target-based t90 与拟合优度/稳定终点口径；本轮不授权新观测或实现。

- 2026-10-19：原因：用户裁决 P-011 先修上层沙。manager 复核 v2.11 否决结论与 slip-overshoot-v1 证据，判定 484 次坡度越界为真实物理违约、context/audit 错位为观测缺陷；新增 P-011-A/B/C 分线，tester 与 designer 并行，重沙机制押后，并固化硬约束、收敛、预算与账本对齐验收口径。

- 2026-10-18：原因：新双模式包默认128列green正式硬前置FAIL，主观相应BLOCKED。记录双模式工程/commit/HAP真实交付身份，P-011从业务开发推进到designer v2.9归因设计；既有预检硬门槛工具遗漏另直路由tester，不改阈值或历史结果。

- 2026-10-18：原因：双模式用例/RED 基线与设计前置完成，原生自动录制能力验证成功。P-011 推进到双模式业务开发，等待交付后 reviewer 统一定向提交及新 HAP 自动双测；保留历史结论，不改阈值。

- 2026-10-18：原因：用户正式要求测试/体验双模式。将输入工程纳入 P-011，采用先补用例→designer→architect→developer 与采集整改汇合路径，取消用户手持/代录前置，保留全部阈值和连续录像证据要求。

- 2026-10-18：原因：发现最新 architecture §7.5.6/§7.5.7 的 canonical runner 与冻结审计证据缺失。保留历史 v2.8 objective PASS，将 P-011 从 VALIDATING 调整为 IN_PROGRESS（验证流程整改），接力 developer→reviewer→tester→effector→designer 双报告验证/独立复盘→manager；补充源码/HAP 身份说明与共享设备串行要求。

- 2026-10-18：原因：manager 复核 P-011 v2.7.3。客观 suite 与 reviewer 提交 `fdc8190` 均确认 PASS，但同路线主观报告缺失，因此状态改为 `VALIDATING` 并路由 effector 产出独立主观报告；未确认 CLOSED。
- 2026-10-18：原因：effector 产出同路线主观报告。EC-TT-01~05 真机证据显示底层完全不动并触发 `HG-TT-04`，总体主观 FAIL；P-011 保持不闭环，改为路由 designer 做客观遥测与主观静止的分歧归因和下一版方案。
- 2026-10-18：原因：designer 复核 v2.7.3。证据链确认 heavy/light 分配失衡与 checker 聚合口径造成客观 PASS 但主观底层静止；新增 `bottom-creep-v2.8-heavy-interface` 设计，状态 `DESIGN_PENDING_APPROVAL`，下一站 architect。
- 2026-10-18：原因：architect 审批 v2.8。确认现有架构可实现 phase-balanced split 与观测链路改造，状态升级为 `READY_FOR_DEVELOPER`；下一站 developer 实现，随后 tester/effector 独立复测。
- 2026-10-18：原因：developer 完成 v2.8。route/相位拆分/compact telemetry/checker schema v2/green-ROI preflight 已实现，checker 自检与打包自检通过；P-011 改为 `VALIDATING` 并路由 tester 先行。
- 2026-10-18：原因：tester 完成 v2.8 客观复测。green-ROI、TC02 schedule 与 formal suite 全部 PASS，客观与主观观测链路分离；路由 effector 执行最终主观验收。
- 2026-10-18：原因：effector 复核 v2.8 主观链路。设备可用但无连续屏幕录制，正式主观判定登记 `BLOCKED(no_screen_recording)`；未评分，HG-TT-04 保持未判定，P-011 不 CLOSED。

- 2026-10-15：原因：manager 复核 P-010。确认 architecture §10 导航提示已就位，满足文档支撑验收条件；P-010 标记 CLOSED 并从 gap 清单移除。

- 2026-10-15：原因：manager 复验 P-006 客观脚本返工。确认脚本与正式 JSON/Markdown 报告均标识 P-006，TC-12 恢复时间、TIERED 映射、TC-13 连续性与帧输入三件套校验全部通过；P-006 标记 CLOSED 并移出 gap，P-001 测试前置解除。

- 2026-10-15：原因：manager 复核外部专家提出的 v2.5 三项冻结条件。确认方向语义、独立 `vBottomCreepMax=10` 与 `α=崩塌角` 均已在算法文档清零；因 v2.5 为 `REJECTED`，将 P-011 指向 `bottom-creep-v2.7` 并路由 designer，同时将 P-007 更新为设计就位、待实现验证。
- 2026-10-15：原因：manager 复验 P-006 返工。`liquid_mode_check.py` 仍输出 P-001，复验不通过并路由 tester 修复；其余客观判定项与 `environment.json` 模板校验通过。
- 2026-10-14：原因：用户真机反馈底层沙能动但不跟手，且临时调参未确认。新增 P-011 为 P0，先补客观/主观用例；实测不达标后再进入 designer。

- 2026-10-13：初始创建。
- 2026-10-13：原因：启动第一轮调度。按 `docs/DESIGN_GAP_CLOSE.md` 的 B1→B4 顺序建立优先级；因两份最新报告均无有效数据且 P-001 是 G1 的前置核心，本轮仅选定 P-001 交由 designer 输出算法方案。
- 2026-10-13：原因：用户最新指令调整场景推进顺序。将 B3/P-003 提升为当前最高优先级并选定为本轮问题，B1/P-001 与 B2/P-002 降为次优先级，B4/P-004 保持最后；同步更新选择理由、验收要求与 Designer 交接说明。
- 2026-10-13：原因：用户补充测试用例也属于 gap 管理范围。新增 P-005/P-006 独立测试缺口，本轮选定 P-003 + P-005，并要求算法方案提供可测试钩子以支撑 tester/effector 后续落地用例。
- 2026-10-13：原因：architect 发现 algorithm v1 §3.3 渗透公式破坏分相守恒。登记当前阻塞项，将 P-003/P-005 标记为 `IN_PROGRESS（BLOCKED）`，并明确 developer 需等待 designer 修订与 architect 确认后开工。
- 2026-10-13：原因：用户更新 `.codex/PIPELINE.md` 问题路由规则。为每个问题标注路由：P-003 走 designer→architect→developer 完整闭环；P-005 由 tester+effector 并行直补，客观/主观用例不被算法修订阻塞。
- 2026-10-13：原因：用户新增“测试先行”原则。将 P-005 明确为 P-003 的前置依赖；在客观/主观用例就位前，P-003 不进入 designer 修订或开发实现。
- 2026-10-14：原因：用户明确 G2 实现优先级。将 P-003 状态更新为 `IN_PROGRESS`，补充一级体验（翻转重力响应 + 倒扣轻沙上浮分选）与二级体验（摇晃混乱 + 再沉淀）的验收排序；一级不达标时二级不作为放行理由，二级阈值不降低；同时评估 `TC-WS-07` / `EC-WS-07` 的一级动态覆盖不足，`TC-WS-08` / `TC-WS-09` / `EC-WS-06` 与二级链路基本对齐。
- 2026-10-14：原因：designer v2.2 完成后的跨角色同步。固化 G3 为“±89° 连续 + 90° 事件式”语义，并将 `effect-cases.md` H06 硬门槛措辞修正路由给 effector，要求区分违规私改与经审计的 `invert180()` 事件。
- 2026-10-14：原因：用户要求拆小目标。新增 13 个可独立验收子目标并按“重要 > 视觉效果好 > 容易实现”排序，标注当前路由与现有用例/需补用例，支撑后续逐项闭环。
- 2026-10-14：原因：用户纠正目标结构。将排序从三条独立目标线改为 **G2+G3 主线** 与 **G1+G3 支线** 两条组合目标线，G3 作为共享动态能力并入两线，不再独立排队。
- 2026-10-14：原因：v2.2 复核与 designer v2.3 提出批量路由请求。新增 P-007~P-010，分别登记 slopeLimit 姿态修正、percolationRate 参数化、WS_SNAPSHOT 4KB 截断与 architecture §10 导航；同时将 H06 与 HG-04/TC-WS-08 口径修正纳入 P-005 的 effector/tester 立即执行事项。
- 2026-10-14：原因：用户要求识别主线等待期可并行启动项。将 P-006、P-009、P-010 标记为立即启动，P-004、P-007 标记为可预研，明确测试用例缺口优先于功能实现，且不抢占 G2+G3 主线。
