# P-011 终点指标双报告（旧口径 vs 新口径）

日期：2026-10-24。执行：tester（P-011）。状态：**DUAL_REPORT_ISSUED / 协议已批准但尚未生效 / 正式放行未授权 / 新口径在真实 run 上为 NOT_IMPLEMENTED**。

## 0. 身份与限制声明

| 项 | 取值 |
|---|---|
| 协议 | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（`protocolRevision=endpoint-terminal-nondeposited-v1` / `endpointMetricRevision=terminal-nondeposited-v1.1`） |
| 口径授权 | `endpointAuthority=APPROVED`（用户 2026-10-24 批准五项裁定） |
| 协议生效 | `endpointProtocolEffective=false` —— 协议**尚未正式生效**（实现虽已落地，仍缺一次符合采集口径的设备 run 与 manager 对适用范围的确认） |
| 正式放行 | `formalReleaseAuthorized=false`；报告 `applicabilityReasons=['FORMAL_RELEASE_NOT_AUTHORIZED']`；`status='CANDIDATE_ONLY'` |
| 字段对齐 | `endpointFieldAlignment=ALIGNED`（manager 裁定 §1 A1–A3） |
| 实现 | commit `c73ed63`（manager 已验收）；交付自测 Python 17/17、Node 8/8，tester 复跑通过 |
| 执行脚本 | `test/p011_endpoint_protocol_v1_execution.py`（tester 侧，只读实现与归档，不改实现） |
| 证据文件 | `test/reports/endpoint_protocol_v1_execution.json` |
| 一致性核对 | `docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md`（判定口径类 45 条：43 条一致，2 条指向同一枚举级缺陷 D-1；报告字段级另计 D-2～D-6） |

**三类身份标注规则（本报告逐处使用）**：

| 身份 | 含义 | 出现处 |
|---|---|---|
| `legacy-susp-only` | 旧口径结果，保留原 PASS/FAIL 与原分母，不追认 | §2.1 全部 legacy 列 |
| `RETROSPECTIVE_DIAGNOSTIC` | 离线回算的候选值，不构成协议生效后的验收证据 | §2.1 的新口径列（真实 run 属此类） |
| `endpoint-terminal-nondeposited-v1` + `APPROVED` | 新口径协议身份（口径已批准，**尚未生效**） | §2.2 全部新口径列 |

**本卡不使用真机**：真实 run 侧的数据来自既有封存归档账本（2026-10-20 采集批），新口径数值来自合成夹具；两者均已如实标注，未占用设备。

## 1. 双算数据来源

| 代号 | 来源 | 性质 | 用途 |
|---|---|---|---|
| Run A | `test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`（2512 记录，sha256 前缀 `f2815c98f9a886f5`，2026-10-20 采集） | **真实设备 run**，采集于端点字段 revision 之前 | 旧口径逐位交叉核对 + 新口径在真实 run 上的真实状态 |
| Run A 基准 | `test/artifacts/p011-slip-formal-v1/formal-checker-output.json`（`recordCount=2512`，与 Run A 同源） | 历史 SLIP checker 输出 | 旧口径逐位对照基准 |
| Run B | 合成夹具（`test/p011_endpoint_protocol_v1_execution.py` 内构造） | **合成数据，非设备 run** | 新口径候选数值与 §10 十条反例的可执行取证 |

## 2. 旧口径 vs 新口径双算

### 2.1 Run A（真实归档 run，SLIP 七相 overshoot 批）

**旧口径（`legacy-susp-only`）——与历史 SLIP checker 输出逐位一致**：

| 相位 | 相位终点 U（`endSuspHPlusSuspL`） | `R_SLIP` | 旧口径比例 | 旧判定（历史） | 新 checker 重算比例 | 逐位一致 |
|---|---|---|---|---|---|---|
| baseline | 0 | 18432 | 0.0000% | PASS | 0.0 | 是 |
| positive | 270.7249452019009 | 18432 | 1.4688% | PASS | 0.014687768294373963 | 是 |
| positiveReturn | 0.6820537072705127 | 18432 | 0.0037% | PASS | 3.700378186146445e-05 | 是 |
| positiveOvershoot | 282.22896091264204 | 18432 | 1.5312% | PASS | 0.015311901091180666 | 是 |
| positiveOvershootReturn | 6.370999547557592 | 18432 | 0.0346% | PASS | 0.00034564884698120614 | 是 |
| negativeOvershoot | 227.03531191598202 | 18432 | 1.2317% | PASS | 0.012317453988497288 | 是 |
| negativeOvershootReturn | 15.430403686890092 | 18432 | 0.0837% | PASS | 0.000837152977804367 | 是 |

一致性判据：新 checker 的 `legacyEndpointFraction` 与「历史 `endSuspHPlusSuspL` ÷ 历史 `initialDepositTotal`」逐位相等（容差 1e-15）→ **7/7 相位一致**。旧口径原 PASS/FAIL **原样保留**，不改写、不追认。

**新口径（`endpoint-terminal-nondeposited-v1` / `APPROVED`）——真实 run 上的真实状态**：

| 项 | 取值 |
|---|---|
| 逐相位状态（不接安全报告） | `NOT_IMPLEMENTED` |
| 缺失字段（确切路径） | `context.endpointFieldRevision`、`context.endpointSampleElapsedMs`、`context.endpointSampleStage`、`context.particleRouteActive/particleRouteId`、`particles`、`particleEvents` |
| 逐相位状态（接入既有 safety checker） | `INVALID_LEDGER`（硬闸门失败优先） |
| 硬闸门失败明细 | `EXISTING_HARD_GATE_FAILED:tan30 (violatingEdges=59413, violatingSubsteps=484)`；历史 `hardConstraint`：`checkedEdges=319024, violatingEdges=59413, violatingSubsteps=484, passed=false`；历史 `overall=FAIL` |
| 覆盖/资格原因 | `CLOCK_DOMAIN_MISMATCH`、`ENDPOINT_NOT_QUIET`、`FIXED_END_NOT_PROVEN`、`INVALID_COMMITTED_ORDINAL`、`INVALID_SOURCE_EPOCH`、`MISSING_PHASE_OCCURRENCE`、`WINDOW_LEFT_BOUNDARY_NOT_PROVEN` |
| 新口径数值 | **未产出**（缺端点字段与粒子摘要，不做任何补造；`endpointFraction`/`windowPeakFraction`/`massWindowPass` 均为 null） |
| 身份 | `RETROSPECTIVE_DIAGNOSTIC`（回算候选）+ 协议身份 `terminal-nondeposited-v1`/`APPROVED`（口径已批准但**未生效**） |
| 报告必填元数据 | `reportMissingFields` 共 **37** 项（`reportIdentityStatus=INCOMPLETE`）——按协议不得以空串或 0 掩盖 |

**这一格的关键事实**：该 run 的旧口径 7 个相位全部 PASS，但同一 run 的既有硬闸门（tan30）失败（59413 条越界边），故按协议 §4.1「硬闸门失败不得用 5% PASS 掩盖」，该 run **整体不合格**；新口径状态取 `INVALID_LEDGER`（硬闸门优先于 `NOT_IMPLEMENTED`）。这正是协议要求的「先过硬闸门再判效果终点」在真实数据上的体现。

### 2.2 Run B（合成夹具候选值，**非设备 run**）

| 场景 | 旧口径 `legacyEndpointFraction` | 旧口径 `legacyMetricSuspensionFraction` | 新口径 `endpointFraction` | 窗峰 `windowPeakFraction` | 状态 | 身份 |
|---|---|---|---|---|---|---|
| R1（E=8000, D=7550, U=322, P=128） | 4.0904% PASS | 4.0250% | **5.6250%** | 5.6250% | FAIL | `terminal-nondeposited-v1`/`APPROVED`（未生效） |
| R2（SLIP，R_SLIP=7000, E=8000, M=375） | — | — | **5.3571%**（M/R_SLIP） | 5.3571% | FAIL | 同上；显示和 `M/E=4.6875%` 并列披露 |
| R3（窗早期 6%、末步 4%） | 4.0000% PASS | — | 4.0000% | **6.0000%** | FAIL | 同上 |
| R4（同提交 M=90 / 混提交前 P 得 110） | — | — | 4.5000% / 5.5000% | — | PASS / FAIL | 同上 |
| R5（DEGRADE：P 减 q、U 加 q） | — | — | 1.0000% → 1.0000% | — | PASS | 同上（`M` 不变） |
| R6（MERGE） | — | — | 1.0000% → 1.0000% | — | PASS | 同上（`M` 不变） |
| R7（D=E−0.04E, U=0, P=0.04E, 同粒永久待准入） | 0.0000% PASS | — | 4.0000% | 4.0000% | PASS + `PERSISTENT_PARTICLE_RESIDUE` | 同上；`longTermParticleResidueStatus=NOT_OBSERVABLE` |
| R8（C run：U=100, D=900, P=50） | 10.0000% | 9.5238% | — | — | — | 同上；两 legacy 量 `differ=true` |
| R9（76.8 单位从粒子转入列） | — | — | 5.6250%（`M`=450 不变） | 5.6250% | FAIL | 同上（不奖励降级） |
| R10（窗内 `sourceEpoch` 变化） | — | — | — | — | INCOMPLETE（`ENDPOINT_NOT_QUIET`） | 同上 |

Run B 全部为合成夹具，**不得作为任何设备级效果结论**；其作用是证明判定式、窗口语义与状态分流按协议执行。

## 3. 协议 §10 十条自检反例的执行记录

执行入口：`python test/p011_endpoint_protocol_v1_execution.py` → `test/reports/endpoint_protocol_v1_execution.json`。**10/10 条实际输出与协议预期一致**，无一条需改判据或放宽断言。

| # | 协议 §10 场景 | 协议预期 | 实际输出 | 结论 |
|---|---|---|---|---|
| R1 | E=8000, D=7550, U=322, P=128，末窗保持 | 旧 PASS / 新 FAIL | 旧 `legacyEndpointFraction=4.0904%`→PASS；新 `endpointFraction=5.6250%`、`massWindowPass=false`→FAIL | 符合 |
| R2 | R_SLIP=7000, E=8000, M=375 | 正式判定 M/R_SLIP=5.3571% FAIL；显示 M/E=4.6875% | `endpointFraction=5.3571%`（`endpointReferenceMass=7000`）、`endpointReferenceKind=SLIP_INITIAL_BH_BL`、显示和 4.6875%、状态 FAIL | 符合 |
| R3 | 末窗早期 M/R=6%，末步 4% | 旧单点 PASS / 新质量窗 FAIL | 旧 `4.0000%` PASS；新 `endpointFraction=4.0000%`、`windowPeakFraction=6.0000%`、`massWindowPass=false`→FAIL | 符合 |
| R4 | LAND 提交 20；提交前 P=80 vs 提交后 U=30,P=60 | 同提交 M=90（4.5% PASS）；混提交前 P 得 110（5.5% FAIL） | `correctM=90`/`correctFraction=0.045` PASS；`mixedM=110`/`mixedFraction=0.055` FAIL | 符合 |
| R5 | DEGRADE：P 减 q、U 加 q | `M` 不变 | `beforeM=afterM=80`，两口径比例均 1.0000% | 符合 |
| R6 | MERGE：合并包只计一次 | `M` 不变 | `plainM=mergedM=80`，比例均 1.0000% | 符合 |
| R7 | D=E−0.04E、U=0、P=0.04E 且同粒永久待准入 | 数值 PASS 但不等于完全落床 | 状态 PASS、`endpointFraction=4.0000%`、诊断码含 `PERSISTENT_PARTICLE_RESIDUE`、`longTermParticleResidueStatus=NOT_OBSERVABLE` | 符合 |
| R8 | C run：U=100, D=900, P=50 | 历史复算 10%、现行路径 9.5238%，两者不同 | `legacyEndpointFraction=0.1`、`legacyMetricSuspensionFraction=0.095238…`、`differ=true` | 符合 |
| R9 | 同总量、76.8 单位从粒子转入列 | 新口径 `M/E` 不变 | `endpointFraction=5.6250%`（`M`=450、`U`=398.8、`P`=51.2） | 符合 |
| R10 | 窗内 `sourceEpoch` 变化 | `INCOMPLETE`（`ENDPOINT_NOT_QUIET`），不外推不插值 | 状态 INCOMPLETE、原因含 `ENDPOINT_NOT_QUIET`；附探针：同窗守恒失败 → `INVALID_LEDGER`（硬闸门优先） | 符合 |

**两处需要登记的差异（不影响结论）**：

- R1 的旧口径列：协议 §10 表所记「旧 4.025%」对应的是 `legacyMetricSuspensionFraction`（`U/max(A,EPS)`，`A=D+U+P`），而裁定 A2 的 `legacyEndpointFraction`（`U/(D+U)`）为 4.0904%。两者判定同为 PASS，结论不变；示例表把两个 legacy 量混在一列，属协议文本勘误项（一致性报告 D-7），**本卡不改冻结内容**，须另起 revision。
- R10 的探针显示：同一窗口内若同时存在守恒失败，状态升级为 `INVALID_LEDGER`（硬闸门优先于覆盖资格），与协议 §4.1 判定顺序一致。

## 4. 协议生效前置核对

### 4.1 取值链路核对（是否始终保持「未生效 / 未授权放行」）

| 环节 | 实现位置 | 取值 | 结论 |
|---|---|---|---|
| 口径授权 | `p011_endpoint_check.py:138` | `endpointAuthority='APPROVED'` | 一致（协议 §0：口径已批准） |
| 协议生效标志 | `:139`、`:429` | `endpointProtocolEffective=False`（写死，非由 schema/代码推导） | 一致（architect §1：只能由 manager 确认适用范围后登记 true） |
| 正式放行标志 | `:140`、`:408` | `formalReleaseAuthorized=False`（写死） | 一致 |
| 放行原因披露 | `:141` | `applicabilityReasons=['FORMAL_RELEASE_NOT_AUTHORIZED']` | 一致（architect §6：生效为 false 时须标该原因） |
| 报告总状态 | `:443` | `status='CANDIDATE_ONLY'`（有 occurrence 时） | 一致 |
| CLI 退出码 | `:472` | 仅 `--candidate-only` 且全部 occurrence PASS 时返回 0，否则 1；正式放行不由退出码表达 | 一致（无任何路径可表达放行） |
| 数值 PASS 的标注 | `:403` | `revisedEndpointResult` 与 `endpointStatus` 并列，`endpointStatus` 不会因内部 PASS 变成放行 | 一致（architect §6：内部 PASS 仍须标未授权放行） |

**结论**：实现侧的生效/放行取值链路正确保持为「未生效 / 未授权放行」，不存在由 schema 版本、代码上线或数值 PASS 反推生效的路径。

### 4.2 协议可宣布正式生效的前置清单

按协议 §9.1 与批准记录 §3 的门序，**以下全部满足**前，协议不得宣布生效、任何 C run 不得单独放行：

| # | 前置 | 当前状态 | 证据/责任 |
|---|---|---|---|
| P1 | 协议 revision 发布且字段名与裁定唯一落点一致 | **已满足** | `69e27b9` + `a3ca53c`；`endpointFieldAlignment=ALIGNED`（tester 侧） |
| P2 | architect 字段 revision 完成 R1–R3 并与 tester 稿字段名一致 | **未满足**（architect 卡 `6adc64ff…ccb44` 仍开放；rev2 已写 ALIGNED 待其正式提交） | architect |
| P3 | 实现落地并通过交付自测 | **已满足**（Python 17/17、Node 8/8） | commit `c73ed63` |
| P4 | 一致性核对无未决的判定口径差异 | **已满足**（28/30 实质条款一致；D-1/D-2 为字段级/枚举级） | 本报告 §1、§3 |
| P5 | D-1～D-6 缺陷修复或经 manager 裁定豁免 | **未满足** | developer；manager 裁定 |
| P6 | 一次符合协议采集口径的**设备 run**：同一时钟域、同 run 起点单调实际 elapsed、post-commit 同子步 `stateAfter`+`particles`、occurrence 身份、manifest 预声明固定边界、休眠只读复核证据 | **未满足**（尚无端点字段的设备 run；真机队列由 effector 独占，需 manager 排队） | manager 排队列 → effector 采集 → tester 出双报告 |
| P7 | 该设备 run 的路线专属双报告（旧/新口径并报、硬闸门与不可观测项分列、`reportMissingFields` 齐备） | **未满足**（本报告 §2.1 的新口径为 `NOT_IMPLEMENTED`/`INVALID_LEDGER`，非该 run 的合格双报告） | tester |
| P8 | designer 验证 | **未满足** | designer |
| P9 | manager 确认适用范围并登记 `endpointProtocolEffective=true` | **未满足** | manager（唯一有权宣布生效者） |

**在 P9 之前**：C run 的终点验收继续双口径并报，任一单口径都不得单独放行；旧报告保留原 PASS/FAIL 与 `legacy-susp-only` 身份，不追认。

## 5. 未解项与后续

| 项 | 状态 | 归属 |
|---|---|---|
| 符合协议采集口径的设备 run（P6） | 未开始；真机为共享串行资源，需 manager 排队 | manager → effector → tester |
| D-1 回算枚举、D-3 `protocolSourceHash`、D-4 `endpointSampleStage` 回填、D-5 `externalAdmissionCount`、D-6 事件级校验 | 缺陷清单已登记（一致性报告 §3） | developer（D-2 需 architect 先出对齐结论） |
| D-7 协议 §10 示例表两列 legacy 量混用 | 勘误须另起 revision（本卡不动冻结内容） | tester |
| 本报告 Run B 的候选数值 | 仅证明判定式与状态分流，不构成设备级效果结论 | — |
| 协议生效 | 由 manager 在 P1–P9 全部满足后宣布 | manager |

## 6. 证据路径

| 证据 | 路径 |
|---|---|
| 真实 run 账本 | `test/artifacts/p011-slip-overshoot-v1/ledger.ndjson`（2512 记录） |
| 历史 SLIP checker 输出（旧口径基准） | `test/artifacts/p011-slip-formal-v1/formal-checker-output.json` |
| 本卡执行结果（十条反例 + 真实 run 双算） | `test/reports/endpoint_protocol_v1_execution.json` |
| tester 侧执行脚本 | `test/p011_endpoint_protocol_v1_execution.py` |
| 协议—实现一致性核对表 | `docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md` |
| 协议正文 | `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` |
| 字段 revision | `docs/architecture-P011-endpoint-fields-revision-v1.md`（rev2） |
| 对齐裁定 | `docs/decision-P011-endpoint-field-alignment-v1.md` |
| 批准记录 | `docs/decision-P011-terminal-nondeposited-fraction-v1.md` |
