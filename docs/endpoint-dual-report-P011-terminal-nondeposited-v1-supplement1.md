# P-011 终点指标双报告 · 补充证据版本 supplement1

## 0. 身份与边界

| 项 | 取值 |
|---|---|
| 文档身份 | `endpoint-dual-report-P011-terminal-nondeposited-v1-supplement1`（**补充版本，与已验收版本并存**） |
| `supplementId` | `dual-report-terminal-nondeposited-v1-supplement1` |
| 被补充版本 | `docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md`（已验收，commit `63fccf7`，22:50） |
| 同时适用 | `docs/endpoint-errata-P011-terminal-nondeposited-v1.md`（errata1，commit `f2d0aee`，已验收 23:29） |
| `protocolId` | `p011-endpoint-terminal-nondeposited`（不变） |
| `protocolRevision` | `endpoint-terminal-nondeposited-v1`（不变；已发布协议文件零改动） |
| `endpointMetricRevision` | `terminal-nondeposited-v1.1`（不变） |
| `endpointAuthority` | `APPROVED`（不变：口径已批准） |
| `endpointProtocolEffective` | **`false`**（未生效，仍由 manager 登记） |
| `formalReleaseAuthorized` | **`false`**（未授权放行） |
| 状态标记 | `SUPPLEMENT_ONLY` / `NO_JUDGEMENT_CHANGE` / `NOT_EFFECTIVE` / `NO_CAPTURE_AUTHORIZATION` |
| 触发原因 | 并行实现提交 `8e498c5`（endpoint 身份元数据补齐）落地，使已验收双报告 §2.1 的元数据计数由 22:45 时点值变为现值 |
| 数据源 | 与已验收版本**同一**归档 run（`test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`，2512 记录），**不采集新数据、不使用真机** |
| 本卡边界 | 不回改 `63fccf7` 双报告正文；不回改已发布协议；不宣布协议生效、不授权放行；不代 developer 修实现；不改测试判据 |

**本文件的性质**：已验收双报告 §2.1 记载的「missingFields 6 类 / reportMissingFields 37 项」是 **22:45 交付时点的真实值**，不是笔误。本文件以补充形式给出**现值**与变动原因，两份文档互相引用、并存有效。读者应**先读已验收版本，再叠加本文件**。

---

## 1. 元数据与身份字段：时点值 → 现值

### 1.1 逐项变动表

| # | 项 | `63fccf7` 时点值 | 现值 | 变动原因（对应缺陷编号） | 影响面 |
|---|---|---|---|---|---|
| 1 | `occurrence.missingFields`（逐 occurrence 缺字段类数） | **6 类** | **7 类** | `8e498c5` 在 checker 新增 `context.externalAdmissionCount` 的存在性/类型/与 `sourceEpoch` 一致性校验（`test/p011_endpoint_check.py:263-270`）。该归档 run 早于该字段，故新增 1 类缺项 → 对应 **D-5（产出点已建，缺项从「无字段」变为「该 run 无此字段」）** | 无数值影响。必填元数据清单扩大，缺项披露更完整；终点状态仍为 `NOT_IMPLEMENTED` / `INVALID_LEDGER` |
| 2 | 新增的那一类 | — | `context.externalAdmissionCount` | 见上 | 见上 |
| 3 | `reportMissingFields`（报告必填元数据缺失项数） | **37 项** | **38 项** | `8e498c5` 在报告元数据必填元组中新增 `endpointSampleStage` 与 `protocolSourceHash` 两个名字（`test/p011_endpoint_check.py:451`）。其中 `protocolSourceHash` **已产出故不缺**，仅 `endpointSampleStage` 实际缺 → 净 +1 项 → 对应 **D-4（部分推进：缺项已显式暴露，回填仍未做）** | 无数值影响 |
| 4 | `protocolSourceHash` | **无**（全仓 0 命中） | **有值**：`ffe337d73a4ee78b21a48543025468238cb3607d68b386de7b2097fba5148837`（`protocolSourceHashAlgorithm=SHA-256`，`protocolSourcePath=docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`） | `8e498c5` 新增 `protocol_source_hash()`：对**入库协议文件字节**（含换行）取 SHA-256 → **D-3 已修复** | 报告可自证所用协议文本版本；不改变任何判定 |
| 5 | `evidenceMode` / `evidenceClassification` | occurrence 行**无此键**；checker 仅透传 manifest 默认值 | `evidenceMode=CANDIDATE`、`evidenceClassification=CANDIDATE`（实测两分支一致） | `8e498c5` 新增 `EVIDENCE_MODES=('CANDIDATE','RETROSPECTIVE_DIAGNOSTIC')` 枚举约束与 `evidenceModeError=INVALID_EVIDENCE_MODE`（`test/p011_endpoint_check.py:18-19`、`:147-150`、`:349-350`） → **D-1 已修复** | 回算值不再可能被静默误标为候选证据；取值非法时进入 `incomplete` |
| 6 | `externalAdmissionCount`（实现侧产出点） | **无产出点**（ArkTS 无写入） | **已产出**：`P011NaturalObservation.ets:702/:714` 新增形参与 `context.externalAdmissionCount` 落账；`WaterSandField.ets:3432` 调用处传入实参 | `8e498c5` → **D-5 已修复（产出点已建）** | 正式 run 上该字段不再恒缺；**但见观察 O-2** |
| 7 | `endpointSampleStage`（报告行回填） | 账本有值、checker 校验、**报告行无该键** | **仍未回填**：`row = dict(occurrence, ...)`（`test/p011_endpoint_check.py:348-352`）不含该键；仅在 `reportMissingFields` 中作为缺项出现 | `8e498c5` **未**做报告行回填，仅补入必填清单 → **D-4 未修复（部分推进）** | 报告读者仍无法从报告本身判断末样本是否 post-commit（原始 D-4 诉求未解决） |

### 1.2 变动的共同性质

第 1、3 项计数的增加，**全部来自「必填元数据清单扩大」**，不是该归档 run 的质量或判定发生了变化：

- 清单扩大前，某些字段根本不在检查范围内，故不报缺；
- 清单扩大后，同一 run 上这些字段仍缺，于是缺项数**如实增加**。

因此「6→7 / 37→38」应读作**披露更完整**，而非「run 变差」。已验收版本 §2.1 的 6/37 与本文的 7/38 都各自在其时点为真；**已验收版本正文不回改**（红线：历史报告结论不改写）。

### 1.3 一处层次区分（`evidenceMode` 与 §2.1 的回算标注）

已验收版本 §2.1 的「身份」行写的是 `RETROSPECTIVE_DIAGNOSTIC（回算候选）`，而本次重跑产物的 `evidenceMode` 实测为 `CANDIDATE`。两者**不在同一层**，且不矛盾：

- `evidenceMode` 是 **manifest 输入**；执行脚本 `test/p011_endpoint_protocol_v1_execution.py` 的 manifest 未声明该键，checker 取默认值 `CANDIDATE`（`p011_endpoint_check.py:147`）。
- §2.1 的 `RETROSPECTIVE_DIAGNOSTIC` 是 **tester 对该归档 run 的判定性标注**（该 run 是回算，不是协议生效后的验收证据）。

D-1 修复的实质正是**补上这条显式通道**：修复前 checker 只能透传默认 `CANDIDATE`，报告里的回算标注无机制表达、也无法被机器校验；修复后若要声明回算身份，必须在 manifest 显式设 `evidenceMode=RETROSPECTIVE_DIAGNOSTIC`，非法值会落入 `INVALID_EVIDENCE_MODE`。

> **登记为观察 O-3（建议，不在本卡改动）**：本卡不改证据脚本（改脚本会改变证据产物并越界到判据边界）。建议下一证据版本在 manifest 中显式声明 `evidenceMode=RETROSPECTIVE_DIAGNOSTIC`，使产物字段与 §2.1 的判定层标注一致。交 manager 决定是否单开。

---

## 2. 缺陷状态表（D-1～D-7）

状态口径：**已修复** / **部分推进** / **未变** / **已裁定待实施** / **已勘误闭环**。状态一律以现值证据为依据。

| # | 缺陷（原文摘要） | 当前状态 | 现值依据 | 归属 / 后续 |
|---|---|---|---|---|
| D-1 | 回算身份 `RETROSPECTIVE_DIAGNOSTIC` 无枚举约束，checker 只透传 manifest `evidenceMode` | **已修复** | `p011_endpoint_check.py:18-19` 定义 `EVIDENCE_MODES`；`:147-150` 校验并产出 `evidenceClassification` / `evidenceModeError`；实测 `evidenceMode=CANDIDATE`、`classification=CANDIDATE` | developer（已闭环） |
| D-2 | 诊断字段名不一致：协议 §6.1 写 `endpointDiagnosis`（对象），实现为三个具名字段 | **已裁定待实施** | architect 裁定稿 `docs/architecture-P011-endpoint-diagnosis-particle-events-v1.md` §1.1：报告唯一使用 `particleResidueDiagnosticStatus` / `particleResidueDiagnosticCodes` / `particleResidueDiagnosticMeaning` 三个现存具名字段，**不再保留 `endpointDiagnosis` 作为第二消费名**；§1.4 明确协议 v1 §6.1 保持原样、原 hash、原 revision，不得为清 D-2 原地改正文；由 tester 在**新 protocol revision** 的 §6.1 登记该一对三拆分 | tester（新 protocol revision 登记，本卡不改冻结内容）+ developer（实现按三字段，已符合） |
| D-3 | `protocolSourceHash` 全仓未产出（architect §6 必填） | **已修复** | 见 §1.1 第 4 项：实测有值，`SHA-256`，`protocolSourcePath` 指向协议文件 | developer（已闭环） |
| D-4 | `endpointSampleStage` 账本有值、checker 校验，但**报告行未回填** | **部分推进（未修复）** | 账本侧有值（`P011NaturalObservation.ets:710`）、checker 校验（`:214`、`:229-230`）、已列入报告必填清单（`:451`）；但报告行 `row = dict(occurrence, ...)`（`:348-352`）**仍无该键** → 原始诉求「读者可从报告本身判断样本是否 post-commit」未解决 | developer（仍需报告行回填一行） |
| D-5 | `externalAdmissionCount` 无产出点，恒进 `reportMissingFields` | **已修复（产出点已建）** | `P011NaturalObservation.ets:702/:714` 写入；`WaterSandField.ets:3432` 传入实参；checker `:263-270` 三级校验（缺失 → 类型/负值 → 与 `sourceEpoch` 不一致） | developer（产出点已闭环）；**观察 O-2 待裁定** |
| D-6 | `particleEvents` 仅校验存在性，不校验七类事件内容与守恒/原子性关联 | **已裁定待实施** | architect 裁定稿 §2.1：唯一归属明确为 `eventLevelValidationOwnership=ENDPOINT_CHECKER`（独立 endpoint checker 从序列化 ledger 复算）；事务层只 `emit-and-atomic-commit only`，其自报 `eventValid` 不得作为 PASS 证据。§2.2 给出五条通用判据与失败证据定位形式。**裁定已下，checker 侧逐笔断言仍待实施**（现状 `:214-215` 仍为存在性校验） | developer（按裁定实施 checker 事件级断言）+ tester（新 protocol revision 同步 §6.2 落点：record 顶层 `particleEvents[]`，v1 不改） |
| D-7 | 协议 §10 第 1 行示例表两个 legacy 量混在一列 | **已勘误闭环** | `docs/endpoint-errata-P011-terminal-nondeposited-v1.md`（v1-errata1，commit `f2d0aee`，已验收 23:29）：按裁定 A2/A3 分列（A2 `U/(D+U)=4.0904%`；A3 `U/(D+U+P)=U/E=4.025%`），结论不变（两 legacy 均 PASS、新口径 `M/E=5.625%` 仍 FAIL） | tester（已闭环，不回改协议正文） |

### 2.1 观察 O-2（新登记，事实与证据）

`WaterSandField.ets:3432` 传给 `stampEndpointObservation` 的第 8 个实参（即 `externalAdmissionCount`）是 **`this.sourceEpoch`**，与第 7 个实参 `sourceEpoch` **同值**。ArkTS 注释亦说明「This is the existing sourceEpoch counter」。

由此产生两点事实：

1. `context.externalAdmissionCount` 与 `context.sourceEpoch` 在真实 run 上**恒等**，前者当前不携带独立于后者的信息。
2. checker 的 `EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH` 判据（`p011_endpoint_check.py:270`，条件为 `admission_count != sourceEpoch`）在真实 run 上**结构性不可触发**——它是一个对未来解耦（两者语义分离后）的防御性判据，当前不构成有效校验。

**处理**：仅登记事实，不代改实现，不改判据。是否需要在两者语义真正分离前调整该判据或字段定义，交 manager 转 developer / architect 裁定。

### 2.2 观察 O-4（新登记）

两份 architect 稿（`docs/architecture-P011-endpoint-fields-revision-v1.md`、`docs/architecture-P011-endpoint-diagnosis-particle-events-v1.md`）**内容已就位但文件尚未入库**（`git status` 为未跟踪）。tester 只核对文件内容，入库由 architect 完成；本文对上述裁定的引用以文件内容为准。

---

## 3. 前置清单 P1–P9 刷新

对照已验收双报告 §4.2 的门序（协议 §9.1 + 批准记录 §3）。**P9 仍只能由 manager 宣布，本卡不代为宣布协议生效或放行。**

| # | 前置 | 已验收版本状态 | **刷新后状态** | 依据 |
|---|---|---|---|---|
| P1 | 协议 revision 发布且字段名与裁定唯一落点一致 | 已满足 | **已满足**（不变） | `69e27b9` + `a3ca53c`；`endpointFieldAlignment=ALIGNED`（tester 侧） |
| P2 | architect 字段 revision 完成 R1–R3 并与 tester 稿字段名一致 | 未满足 | **已满足**（内容已就位） | architect rev2 稿「当前对齐（R1–R3）」已写明 `endpointProtocolId=p011-endpoint-terminal-nondeposited`、`endpointProtocolRevision=endpoint-terminal-nondeposited-v1`、`endpointFieldAlignment=ALIGNED`，三处唯一落点与 tester 稿逐字一致；§6 报告字段清单已补两个 legacy 字段名（R3）。**附注 O-4：该稿文件尚未入库，入库由 architect 完成** |
| P3 | 实现落地并通过交付自测 | 已满足（17/17、8/8） | **已满足**（现值 20/20、9/9） | commit `c73ed63` + `8e498c5`；用例数增加为并行会话新增，仍零失败（见 §5） |
| P4 | 一致性核对无未决的判定口径差异 | 已满足（45 条 / 43 一致 / 2 条指向 D-1） | **已满足**（D-1 已修复后该 2 条随之闭环） | 一致性报告 §1.1–§1.5、§4；errata1 §3（计数口径）；**D-1 已由 `8e498c5` 修复（§2 本表）** |
| P5 | D-1～D-6 缺陷修复或经 manager 裁定豁免 | 未满足 | **部分满足**（tester 实测口径，见下） | 已修复：D-1、D-3、D-5；未修复：D-4（部分推进）；已裁定待实施：D-2、D-6；已勘误闭环：D-7。详见 §2 |
| P6 | 一次符合协议采集口径的**设备 run** | 未满足 | **未满足**（不变） | 尚无端点字段的设备 run；真机为共享串行资源，由 effector 独占，需 manager 排队列 |
| P7 | 该设备 run 的路线专属双报告 | 未满足 | **未满足**（不变） | 本报告与该归档 run 的新口径仍为 `NOT_IMPLEMENTED` / `INVALID_LEDGER`，非该 run 的合格双报告 |
| P8 | designer 验证 | 未满足 | **未满足**（不变） | designer |
| P9 | manager 确认适用范围并登记 `endpointProtocolEffective=true` | 未满足 | **未满足**（不变，**本卡不得代宣布**） | manager（唯一有权宣布生效者） |

### 3.1 关于 P5 的两点说明（交 manager 裁定）

1. **任务卡预期与实测的偏差**：任务卡描述为「D-1/D-3/D-4/D-5 已修复」。tester 实测 **D-4 的报告行回填仍未做**（§1.1 第 7 项、§2 表），故按「逐条以现值证据为依据」记为**部分满足**。
2. **若 manager 按「判定层缺陷全部闭环」口径裁定**，可将 P5 记为**已满足**——理由是 D-4 属报告展示层（读者可判断性），不改终点数值判定、阈值、窗口、终点或状态码，与已验收版本 P4「D-3～D-6 为元数据/事件级缺口」的定性一致。**tester 不自行做此裁定，仅提供事实与两种口径的后果说明。**

**在 P9 之前**：C run 的终点验收继续双口径并报，任一单口径都不得单独放行；旧报告保留原 PASS/FAIL 与 `legacy-susp-only` 身份，不追认。

---

## 4. 三项主结论一致性声明

本次更新**不改变**已验收双报告的任何一项主结论。逐条以现值证据复述：

| # | 主结论 | 现值证据（重跑产物） | 是否改变 |
|---|---|---|---|
| 1 | 旧口径与历史 SLIP checker 输出**逐位一致 7/7** | `legacyCrossCheck` 7/7 `bitwiseAgree=true`（容差 1e-15；`R_SLIP=18432`，七相位 baseline/positive/positiveReturn/positiveOvershoot/positiveOvershootReturn/negativeOvershoot/negativeOvershootReturn 全数一致） | **未改变** |
| 2 | 无 safety report 时 `NOT_IMPLEMENTED`；接入既有 safety report 后 `INVALID_LEDGER`（硬闸门优先） | `withoutSafetyReport.occurrences[0].endpointStatus=NOT_IMPLEMENTED`；`withSafetyReport.occurrences[0].endpointStatus=INVALID_LEDGER`，`hardGateFailures=['EXISTING_HARD_GATE_FAILED:tan30 (violatingEdges=59413,violatingSubsteps=484)']`（历史 `checkedEdges=319024`，`overall=FAIL`） | **未改变** |
| 3 | 报告总状态 `CANDIDATE_ONLY`；`endpointProtocolEffective=false`；`formalReleaseAuthorized=false` | 两分支均为 `status=CANDIDATE_ONLY`、`endpointProtocolEffective=false`、`formalReleaseAuthorized=false`、`applicabilityReasons=['FORMAL_RELEASE_NOT_AUTHORIZED']` | **未改变** |

**该 run 仍整体不合格**：旧口径 7 相位全 PASS，但同一 run 的既有硬闸门（tan30）失败 59413 条越界边。按协议 §4.1「硬闸门失败不得用 5% PASS 掩盖」，旧口径全 PASS **不构成**放行依据——此结论不因本次元数据更新而改变。

**新增但未改变结论的项**：`protocolSourceHash` 已有值、evidenceMode 已受枚举约束、必填清单扩大（6→7 / 37→38）。这些均属**身份与元数据层**，不触及分子 `M=U+P`、分母、阈值 5%、末 1s 窗、固定终点或状态码判定。

---

## 5. 验证与复现

| 验证项 | 命令 | 结果 |
|---|---|---|
| §10 十条自检反例 | `python test/p011_endpoint_protocol_v1_execution.py` | **10/10 MATCH**（R1–R10 全部与协议预期一致） |
| checker 单元自测（Python） | `python -m unittest test.test_p011_endpoint_check` | **20/20 通过** |
| checker 单元自测（Node） | `node test/run_p011_endpoint_units.cjs` | **9/9 通过** |
| 现值证据 | `test/reports/endpoint_protocol_v1_execution.json`（重生成） | 本文 §1、§2、§4 每个数字均由该产物核对得出 |
| 已发布协议未被回改 | `git status --short docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` | **空**（零改动） |
| 已验收双报告未被回改 | `git status --short docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md` | **空**（本卡以补充文件并存） |

**数据源**：`test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`，2512 记录，与已验收版本同源；**未采集新数据、未使用真机**。

**差异核对方法**：用 `git show 63fccf7:test/reports/endpoint_protocol_v1_execution.json` 取时点值，与现值做集合差——新增 `context.externalAdmissionCount`（missingFields）、`endpointSampleStage`（reportMissingFields），移除均为空集。

---

## 6. 红线自检

| 红线 | 本卡执行情况 |
|---|---|
| 不改协议正文与已发布版本 | **遵守**：协议文件 git status 干净；身份字段沿用 v1 |
| 不回改 `63fccf7` 双报告正文 | **遵守**：新建补充文件并存、互相引用；已验收版本 git status 干净 |
| 不改判定式 / 阈值 / 窗口 / 终点 / 状态码 / 双算规则 | **遵守**：本文只登记计数与字段现状，未触及 `M/R`、5%、末 1s 窗、固定终点、六码分流、双算规则 |
| 不改 DUALFIELD-* 预注册 estimand | **遵守**：未触碰 |
| 不改冻结指标（30°±2° / t90 / R² / 5%） | **遵守**：未触碰 |
| 不宣布协议生效、不授权放行 | **遵守**：P9 维持未满足，全文标注 `NOT_EFFECTIVE`；两项 boolean 实测仍为 false |
| 不追认旧 route 结果 | **遵守**：旧口径保留 `legacy-susp-only` 与原 PASS/FAIL，硬闸门失败仍压过 5% PASS |
| 不代 developer 修实现 | **遵守**：D-4 / O-2 只列事实与建议归属 |
| 不改测试判据 | **遵守**：未改动 checker 与执行脚本的判定逻辑（仅重跑） |

---

## 7. 追加记录与资产索引

### 7.1 追加记录

- 2026-10-24 23:3x（本文件）：已验收双报告（commit `63fccf7`）的元数据计数因并行提交 `8e498c5` 由 6 类 / 37 项变为 **7 类 / 38 项**。以**补充版本**形式登记，不回改历史报告正文。三项主结论经重跑复核**全部未变**。缺陷状态：D-1 / D-3 / D-5 已修复，D-4 部分推进（未修复），D-2 / D-6 依 architect 裁定稿记为「已裁定待实施」，D-7 已由 errata1 闭环。新增观察 O-2（`externalAdmissionCount` 实参为 `sourceEpoch` 同值）、O-3（证据脚本未声明 `evidenceMode`，建议下版显式声明）、O-4（两份 architect 稿未入库）。

### 7.2 资产索引

| 资产 | 路径 |
|---|---|
| 已验收双报告（被补充版本） | `docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md`（commit `63fccf7`） |
| 本补充版本 | `docs/endpoint-dual-report-P011-terminal-nondeposited-v1-supplement1.md` |
| 一致性核对表 | `docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md`（§3 缺陷清单 D-1～D-7） |
| 协议勘误 errata1 | `docs/endpoint-errata-P011-terminal-nondeposited-v1.md`（commit `f2d0aee`） |
| 已发布协议 v1（不得回改） | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（`69e27b9` + `a3ca53c`） |
| architect 字段 revision rev2 | `docs/architecture-P011-endpoint-fields-revision-v1.md`（R1–R3 已就位，未入库） |
| architect 诊断字段与事件裁定 | `docs/architecture-P011-endpoint-diagnosis-particle-events-v1.md`（D-2 §1、D-6 §2，未入库） |
| 字段对齐裁定 | `docs/decision-P011-endpoint-field-alignment-v1.md` |
| 批准记录 | `docs/decision-P011-terminal-nondeposited-fraction-v1.md` |
| 证据脚本 | `test/p011_endpoint_protocol_v1_execution.py` |
| 证据产物 | `test/reports/endpoint_protocol_v1_execution.json` |
| 归档 run | `test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`（2512 记录） |
| 实现提交 | `c73ed63`（实现）、`8e498c5`（身份元数据补齐） |
