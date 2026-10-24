# P-011 endpoint protocol v1 — 协议—实现一致性核对报告

日期：2026-10-24。执行：tester（P-011）。状态：**CONFORMANCE_REVIEWED / 7 项差异待处理 / 协议未生效 / 未授权放行**。

## 0. 核对对象与复现方法

| 项 | 取值 |
|---|---|
| 协议 | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（`protocolId=p011-endpoint-terminal-nondeposited` / `protocolRevision=endpoint-terminal-nondeposited-v1` / `endpointMetricRevision=terminal-nondeposited-v1.1` / `endpointAuthority=APPROVED`） |
| 协议 commit | `69e27b9`（首发）+ `a3ca53c`（T1–T3 字段名对齐） |
| 实现 | commit `c73ed63`（manager 已验收）：`P011EndpointMetrics.ets`、`P011EndpointCaptureSession.ets`、`P011EndpointStartupClock.ets`、`P011NaturalObservation.ets`、`WaterSandField.ets`、`test/p011_endpoint_check.py` |
| 字段稿 | `docs/architecture-P011-endpoint-fields-revision-v1.md` rev2（字段名/语义/归属/可选性、§6 报告字段清单） |
| 对齐裁定 | `docs/decision-P011-endpoint-field-alignment-v1.md` §1 A1–A3（唯一落点）、§2（采纳 tester §5.3 结论） |
| 复现命令 | `python test/p011_endpoint_protocol_v1_execution.py`（tester 侧，只读实现与归档）<br>`python -m unittest test.test_p011_endpoint_check`（交付自测 17/17 通过）<br>`node test/run_p011_endpoint_units.cjs`（交付自测 8/8 通过） |
| 证据文件 | `test/reports/endpoint_protocol_v1_execution.json` |

核对方法：逐条比对协议条款文本与实现代码路径；凡涉及数值判定的条款以交付 checker 实跑（真实归档账本 + 合成夹具）取证，不以文档描述代替执行。

**本报告不改实现、不改判据、不宣布协议生效**；差异只列事实与证据（第 3 节），修复归 developer。

## 1. 逐条核对表

### 1.1 协议 §1 分子细则 M1–M8 与守恒先决

| 条款 | 协议要求 | 实现位置（commit c73ed63） | 结论 |
|---|---|---|---|
| M1 | `P_H/P_L` = `pState∈{1,2}` 的 `pMass` 按 `pPhase` 求和；`state=0` 不计 | `test/p011_endpoint_check.py:110-124`：`pMass/pPhase/pState` 在场时逐粒重算并与摘要逐项比对（`PARTICLE_SUMMARY_MISMATCH`→hard）；`state==0` 先跳过；`state∉{1,2}`→`INVALID_LIVE_PARTICLE` | 一致 |
| M2 | 水下、水面上方、接触待准入粒子全部计入 `P` | `p011_endpoint_check.py:70-81`：`P` 取摘要 `massH/massL` 整体，无位置/状态过滤 | 一致 |
| M3 | `DEGRADE`/`TO_SUSP` 累计量永不入分子 | `p011_endpoint_check.py:92-95`：`degraded` 仅作非负有限诊断，不参与 `P`；实测 R5 `beforeM=afterM=80` | 一致 |
| M4 | `MERGE` 后只计现存包一次 | `p011_endpoint_check.py:110-124`（按存量摘要计，事件不叠加）；实测 R6 `plainM=mergedM=80` | 一致（事件本体在粒子层事务，checker 不重复计） |
| M5 | `LAND` 只扣 `granted`，余量留 `state=2` 仍计 `P` | 口径由 post-commit 快照保证：`p011_endpoint_check.py:245-266` 只读同子步 `stateAfter`+`particles`；实测 R4 同提交 `M=90`（4.5% PASS）/ 混提交前 `P` 得 `M=110`（5.5% FAIL） | 一致 |
| M6 | `TO_SUSP` 若显式启用，提交后只计 `U` | 现版不启用 `TO_SUSP`（架构稿 rev1 T3 固定保留原粒）；checker 按摘要存量计 | 一致 |
| M7 | B tracer 无质量，一律不计 | checker 不读取 tracer；ArkTS 摘要来源为物理池 `P011ParticleField` | 一致 |
| M8 | 摘要与逐粒不得同时相加；不一致即 `INVALID_LEDGER` | `p011_endpoint_check.py:110-124` → `PARTICLE_SUMMARY_MISMATCH`（归入 hard 列表） | 一致 |
| §1 守恒先决 | `abs(D_H+U_H+P_H−E_H)≤1e-9`，未过不给正式终点 PASS | `p011_endpoint_check.py:259-263` → `PHASE_CONSERVATION`（hard）；R10 探针实测：同窗守恒失败 → `INVALID_LEDGER` | 一致 |

### 1.2 协议 §2 固定终点与末 1s 窗

| 条款 | 协议要求 | 实现位置 | 结论 |
|---|---|---|---|
| §2.1 SLIP 终点 | 每相位 occurrence 最后一个已提交子步 | `p011_endpoint_check.py:285-286`（`end == phaseEndElapsedMs` 恒等校验）、`:291-293`（按 `endpointPhaseOccurrenceId` 选样）、`:332`（`selected[-1]` 为终点样本） | 一致 |
| §2.1 WS-08 终点 | shake-off 生效后满 8.0s 的最后一个已提交子步 | `p011_endpoint_check.py:282-284`（`end == shakeOffEffectiveElapsedMs + 8000`）；`P011EndpointCaptureSession.shakeOffBoundary` 固定 `+8000/+7000` 边界 | 一致 |
| §2.1 WS-09 采样段 | 原 10s/末 5s 段不变，新末 1s 窗嵌在末段内 | `p011_endpoint_check.py:287-290`（`phaseEnd−phaseStart==10000` 且 `legacyWindowStart==end−5000`），不符 → `WS09_ORIGINAL_10S_5S_IDENTITY_MISSING` | 一致 |
| §2.1 禁止项 | 不挑首次低于阈值子步、不延长观察等绿、不用 250ms `WS_METRIC` 抽样替代 | `p011_endpoint_check.py:131`（边界由 manifest 拥有，不从 EOF 推断）、`:271-273`（缺 manifest → `FIXED_BOUNDARY_MANIFEST_MISSING`） | 一致 |
| §2.2 窗口范围 | 闭区间 `[t_e−1000ms, t_e]`，含边界样本 | `p011_endpoint_check.py:277-279`（`start != end−1000` → `INVALID_FIXED_WINDOW`）、`:294` | 一致 |
| §2.2 逐子步达标 | 窗内每子步 `M(k) ≤ 0.05R+1e-9` | `p011_endpoint_check.py:364-376`（`gaps` 逐样本，`massWindowPass = all(g[1]<=0)`）；实测 R3 窗峰 6% → FAIL | 一致 |
| §2.2 两值分列 | `endpointFraction` 与 `windowPeakFraction` 分别披露 | `p011_endpoint_check.py:358`（终点值）、`:373`（窗峰），互不替代 | 一致 |
| §2.2 覆盖不足 | 记 `INCOMPLETE`，不外推、不插值 | `p011_endpoint_check.py:300-304`（`WINDOW_LEFT_BOUNDARY_NOT_PROVEN` / `FIXED_END_NOT_PROVEN`）；休眠只读复核 `:43-55` `held_at()` + `DORMANT_STATE_RECHECK` 记录，冻结/失败不得冒充新提交 | 一致 |
| §2.2 窗口安静 | `sourceEpoch` 恒定、无摇晃、无 reset/invert/模式/route 切换 | `p011_endpoint_check.py:305-307`（`ENDPOINT_NOT_QUIET`）、`:310-311`（`shakeInput!=0`）、`:312-315`（四个计数恒定） | 一致 |
| §2.3 SLIP 不要求 `shake=0` | SLIP 保留既有派生摇晃耦合 | `p011_endpoint_check.py:310` 的 `case != 'TC-P011-SLIP-03'` 条件；实测 R2 注入 `shakeInput=0.6` 仍按 SLIP 口径判定 | 一致 |
| §2 场景身份 | SLIP 出现外源准入须重跑 | `p011_endpoint_check.py:316-318` → `SLIP_EXTERNAL_ADMISSION_REQUIRES_RERUN` | 一致 |

### 1.3 协议 §4 状态分类与判定顺序

| 条款 | 协议要求 | 实现位置 | 结论 |
|---|---|---|---|
| §4.1 判定顺序 | 路由/协议身份 → 硬闸门 → 资格 → 效果终点 | `p011_endpoint_check.py:397-402` 的状态覆盖次序（效果值先算，硬闸门最后覆盖），等价于「硬闸门优先」 | 一致 |
| §4.1 硬闸门先行 | 硬闸门失败不得用 5% PASS 掩盖 | `p011_endpoint_check.py:402`（`if hard: status='INVALID_LEDGER'`）、`:404`（`hardGateStatus` 独立字段）；R10 探针实测硬闸门优先于覆盖资格 | 一致 |
| §4.2 五码 | `NOT_IMPLEMENTED` / `INVALID_LEDGER` / `INVALID_BASELINE` / `INVALID_DENOMINATOR` / `INCOMPLETE`，均不得 PASS | `p011_endpoint_check.py:397-402` 五个状态全部实现；实测覆盖：缺字段→`NOT_IMPLEMENTED`、守恒失败→`INVALID_LEDGER`、基线无效→`INVALID_BASELINE`、`R≤0`→`INVALID_DENOMINATOR`、epoch 变化→`INCOMPLETE` | 一致 |
| §4.2 缺字段清单 | `NOT_IMPLEMENTED` 须列确切字段路径 | `p011_endpoint_check.py:135`/`:212-222`/`:267-268` 收集确切路径（如 `context.endpointSampleElapsedMs`、`particles`、`particleEvents`） | 一致 |
| §4.2 失败闸门/不可观测分列 | 分开记录，不得混写 | `p011_endpoint_check.py:405-406`（`endpointStatusDetail` 分 `missingFields`/`hardGateFailures`/`baselineErrors`/`incomplete`）、`:407-412`（滞留诊断独立字段） | 一致 |
| §4.3 滞留诊断 | 终点 `state=2` 报 `PERSISTENT_PARTICLE_RESIDUE`；无逐粒年龄证据记 `NOT_OBSERVABLE` | `p011_endpoint_check.py:96-109`（缺 `byState` → `NOT_OBSERVABLE`）、`:409`（`longTermParticleResidueStatus` 恒 `NOT_OBSERVABLE`）、`:410-412`（`CONTACT_WAIT>0` → 追加诊断码）；实测 R7 同时给出数值 PASS + 诊断码 + `NOT_OBSERVABLE` | 一致 |

### 1.4 协议 §5 双算规则

| 条款 | 协议要求 | 实现位置 | 结论 |
|---|---|---|---|
| 正式判定口径 | 从原始 v7 账本重算 `M/R`，**不得**用显示分项之和 `M/E` | `p011_endpoint_check.py:335`（`reference = r_slip if SLIP else e`）、`:358`（`endpointFraction=(u+p)/reference`）；实测 R2：`endpointFraction=5.3571%`（M/R_SLIP）而显示和 `M/E=4.6875%` | 一致 |
| A2 legacy 量 | `legacyEndpointFraction = U/R`（R = 该 case 历史分母：SLIP `R_SLIP`；WS `A_hist=D+U`） | `p011_endpoint_check.py:336`（`legacy_reference`）、`:346` | 一致 |
| A3 legacy 量 | `legacyMetricSuspensionFraction = U/max(A,EPS)`，`A=D+U+P` | `p011_endpoint_check.py:334`（`a = d+u+p`）、`:347` | 一致 |
| 两组不得互为别名 | 独立字段、独立来源标注 | `:346-347`、`:378-380`（`legacyMetricSuspensionFractionSource='RECOMPUTED_FROM_SAME_POST_COMMIT_STATE'`）；实测 R8：`0.1` vs `0.095238`，`differ=true` | 一致 |
| 任一不单独放行 | 不得单独作为放行依据 | `:408`、`:429`（`formalReleaseAuthorized=False`、`endpointProtocolEffective=False`）、`:141`（`applicabilityReasons=['FORMAL_RELEASE_NOT_AUTHORIZED']`） | 一致 |
| 并列披露 | `R/U/P/M` 与各自比例 | `:337-353`（`depositedMass/suspendedMass/particleMass/nondepositedMass/expectedMass/actualMass` + Heavy/Light 分列 + `conservationError*`）、`:344-345`（三分项） | 一致 |
| legacy 身份 | 标 `legacy-susp-only`，保留原 PASS/FAIL | `:322`（`legacyReportIdentity='legacy-susp-only'`）、`:377`（`legacyEndpointResult`）、`:393-394`（缺端点字段时仍独立给出 legacy 值） | 一致 |
| 回算身份 | 回算值标 `RETROSPECTIVE_DIAGNOSTIC` | `:139` 仅从 manifest 读 `evidenceMode`（默认 `CANDIDATE`），**无 `RETROSPECTIVE_DIAGNOSTIC` 枚举约束与校验** | **不一致 → D-1** |

### 1.5 协议 §7 checker 适配 N1–N11

| 条款 | 要求 | 实现位置 | 结论 |
|---|---|---|---|
| N1 | 每子步 `M=U+P` | `p011_endpoint_check.py:245-266` | 一致 |
| N2 | 分子细则 M1–M8 校验 | `:58-127`（`mass()`） | 一致 |
| N3 | 固定终点按 occurrence 身份 | `:291-293`、`:231-232`（缺 `endpointPhaseOccurrenceId` → `MISSING_PHASE_OCCURRENCE`） | 一致 |
| N4 | 末 1s 窗按 `endpointSampleElapsedMs` 另取 | `:220-232`（非有限/回退 → `INVALID_ELAPSED`/`NON_MONOTONIC_ELAPSED`）、`:294`（窗按 elapsed 选取，与 legacy 的 `physicsTime` 窗分离） | 一致 |
| N5 | 窗内逐子步达标 | `:364-376` | 一致 |
| N6 | `endpointFraction`/`windowPeakFraction` | `:358`、`:373` | 一致 |
| N7 | 窗口资格（覆盖 + `sourceEpoch` 恒定） | `:300-315` | 一致 |
| N8 | 状态码分流 + 失败闸门与不可观测项分列 | `:397-412` | 一致 |
| N9 | `R≤0` → `INVALID_DENOMINATOR` | `:355-356`、`:368-369` | 一致 |
| N10 | `legacyEndpointFraction` + `legacyMetricSuspensionFraction` 两字段 | `:346-347`、`:377` | 一致 |
| N11 | 回算值标 `RETROSPECTIVE_DIAGNOSTIC` | 同 §5 末行 | **不一致 → D-1** |

### 1.6 协议 §6 报告字段与 architect §6 清单（字段级）

| 字段 | 协议/architect 要求 | 实现现状 | 结论 |
|---|---|---|---|
| `endpointMetricRevision`/`endpointAuthority`/`endpointProtocolId`/`endpointProtocolRevision`/`endpointFieldAlignment`/`endpointFieldRevision` | 必填且取值固定 | `p011_endpoint_check.py:136-140`、`:427-429` 全部输出，取值与协议 §0 一致 | 一致 |
| `endpointProtocolEffective` | 恒 false（manager 确认适用范围后才登记 true） | `:139`、`:429` 恒 `False` | 一致 |
| `formalReleaseAuthorized` | 恒 false | `:140`、`:408` 恒 `False` | 一致 |
| `endpointReferenceKind`/`endpointReferenceMass`/`endpointFraction`/`windowPeakFraction`/`massWindowPass` | 必填 | `:320`、`:338`、`:358`、`:373`、`:376` | 一致 |
| `endpointStatus`/`endpointWindowStatus`/`endpointStatusDetail` | 必填 | `:403-406` | 一致 |
| `legacyEndpointFraction`/`legacyMetricSuspensionFraction`/`legacyEndpointReferenceKind`/`legacyEndpointReferenceMass`/`legacyEndpointResult`/`legacyReportIdentity` | 必填 | `:321-322`、`:339`、`:346-347`、`:377` | 一致 |
| `particles.byState.*`、`particleResidueDiagnosticStatus`/`particleResidueDiagnosticCodes` | 必填/诊断 | `:96-109`、`:329`、`:348`、`:407-412` | 一致 |
| `endpointDiagnosis` | 协议 §6.1 列为报告字段（诊断对象） | 实现拆为 `particleResidueDiagnosticStatus`+`particleResidueDiagnosticCodes`+`particleResidueDiagnosticMeaning`（architect §6「byState/滞留」组） | **字段名不一致 → D-2** |
| `protocolSourceHash` | architect §6「文档/启用身份」组必填 | 全仓未产出（ArkTS 与 checker 均无） | **未实现 → D-3** |
| `endpointSampleStage` | architect §6「实际末样本」组必填 | 账本侧有值（`P011NaturalObservation.ets:706`），checker 校验但**报告行未回填** | **未回填 → D-4** |
| `externalAdmissionCount` | architect §6「阶段/源身份」组必填 | 仅出现在 checker 的 `reportMissingFields` 元组（`:419`），全仓无产出点 | **未实现 → D-5** |
| `residueObservedDurationMs` | architect §6 列出 | 无产出点，恒进 `reportMissingFields`；与协议 §4.3「架构不存 age → 长期 `NOT_OBSERVABLE`」一致 | 一致（按协议不可补造） |
| `evidenceMode` | 枚举应区分候选/回算 | `:139` 透传 manifest，未约束取值 | 见 D-1 |
| `particleEvents` 七类事件内容 | 协议 §6.2 要求逐笔可溯 | checker 仅校验存在性（`:214-215`），不校验事件类型与守恒/原子性的关联 | **仅存在性校验 → D-6（说明性）** |

## 2. 执行侧验证（客观结果）

| 项 | 结果 |
|---|---|
| 交付自测（Python checker） | `python -m unittest test.test_p011_endpoint_check` → **17 tests OK**（tester 复跑） |
| 交付自测（Node 原生） | `node test/run_p011_endpoint_units.cjs` → **8/8 PASS**（tester 复跑） |
| 协议 §10 十条自检反例 | `test/p011_endpoint_protocol_v1_execution.py` → **10/10 结论符合协议预期**（明细见双报告 §3） |
| 真实归档 run 的 legacy 逐位交叉核对 | 7 个相位全部 **bitwise 一致**（新 checker 重算值与历史 SLIP checker 输出完全相同，容差 1e-15） |
| 真实归档 run 的新口径 | `NOT_IMPLEMENTED`（6 类端点字段缺失）／接入既有安全 checker 后 `INVALID_LEDGER`（硬闸门 tan30 失败） |

## 3. 缺陷清单（只列事实与证据，不代改）

| # | 缺陷 | 证据 | 影响 | 建议归属 |
|---|---|---|---|---|
| D-1 | 回算身份 `RETROSPECTIVE_DIAGNOSTIC` 无枚举约束：checker 只透传 manifest 的 `evidenceMode`（默认 `CANDIDATE`），不校验取值、不区分候选与回算 | `test/p011_endpoint_check.py:139`；协议 §5.4、N11 | 回算值可能被误标为候选证据，违反「回算不构成生效后验收证据」 | developer（checker 枚举校验）+ tester（协议侧措辞已明确，无需改） |
| D-2 | 诊断报告字段名不一致：协议 §6.1 写 `endpointDiagnosis`（对象），实现为 `particleResidueDiagnosticStatus`/`particleResidueDiagnosticCodes`/`particleResidueDiagnosticMeaning` 三个字段（architect §6「byState/滞留」组） | 协议 §6.1 vs `p011_endpoint_check.py:329`、`:407-412` | 报告消费方按协议字段名取不到诊断对象 | 由 architect 在字段 revision 登记唯一落点（建议采用三个具名字段），tester 侧按对齐结论回填协议 §6.1 登记 |
| D-3 | `protocolSourceHash` 全仓未产出，但 architect §6 列为「文档/启用身份」组必填 | `grep` 全仓 0 命中；architect §6 清单 | 报告无法自证所用协议文本版本 | developer（产出）+ architect（清单确认） |
| D-4 | `endpointSampleStage` 账本侧有值、checker 也校验，但报告行未回填该字段（architect §6「实际末样本」组要求） | `P011NaturalObservation.ets:706`、checker `:193/:208` 校验 vs 报告行无该键 | 报告读者无法从报告本身判断样本是否 post-commit | developer（报告行回填一行） |
| D-5 | `externalAdmissionCount` 无产出点，恒进 `reportMissingFields`（architect §6「阶段/源身份」组要求） | checker `:419` 仅有元组项；ArkTS 无写入 | 正式 run 的报告必填元数据恒缺 | developer（产出计数） |
| D-6 | `particleEvents` 仅校验存在性，不校验七类事件内容与守恒/原子性的关联 | checker `:214-215` | 事件级溯源只到「有事件数组」，未到「逐笔自洽」；原子性目前靠粒子层事务保证 | developer（补事件级断言，或在字段 revision 中明确该职责在粒子层） |
| D-7 | 协议 §10 第 1 行「旧口径 `U/E=4.025%`」与裁定 A2（WS 历史分母 = `D+U`）不一致：按 A2 应为 `U/(D+U)=4.0894%` | 协议 §10 第 1 行 vs §5.3 A2 定义；实测 R1 `legacyEndpointFraction=4.0904%`、`legacyMetricSuspensionFraction=4.025%` | **结论不变**（legacy PASS / 新 FAIL），但示例表两列数值口径混用（`U/E` 与 `U/(D+U)`），易被误读为两个不同 legacy 量 | tester（协议侧文本勘误，须另起 revision；本卡不改冻结内容） |

## 4. 结论

- 判定口径类条款共 **45 条**（§1 分子细则与守恒 9 条、§2 固定终点与末 1s 窗 11 条、§4 状态分类与判定顺序 6 条、§5 双算规则 8 条、§7 N1–N11 11 条）：其中 **43 条完全一致**，另 **2 条指向同一枚举级缺陷 D-1**（§5「回算身份」与 N11 为同一条款在两表中的出现）。无一条差异改变判定式、阈值、窗口、终点或状态码语义。
- 报告字段级核对（§1.6）另计：D-2（诊断字段名）为命名差异，D-3/D-4/D-5/D-6 为报告元数据与事件级校验的缺口——均不改变终点数值判定，但**正式 run 的报告必填元数据会因此不齐**（`reportMissingFields` 会如实列出，不得以空串或 0 掩盖）。
- 协议**尚未生效**：实现已落地且自测通过，但缺一次符合协议采集口径的设备 run 及其双报告，缺 manager 对适用范围的确认。
- 缺陷清单已按事实与证据登记，转 manager 转 developer；tester 不代改实现、不改判据。

**交办建议**：D-1～D-6 转 developer（其中 D-2 需 architect 先出字段对齐结论）；D-7 由 tester 另起 protocol revision 勘误。协议生效前置清单见 `docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md` §4。
