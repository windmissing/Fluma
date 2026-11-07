# P-011 endpoint 字段与协议对齐结论（manager 裁定）

日期：2026-10-24。裁定人：manager（`6ac8a7e3e9adb60000000419` 项目）。
依据：tester 交付 `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（commit 69e27b9，368 行）与 architect 交付 `docs/architecture-P011-endpoint-fields-revision-v1.md`（未提交，226 行），两稿经 manager 逐段核对源码与 checker 后比对。

状态：**ALIGNED / 附两项字段名统一要求 / 双方各改一处后即对齐 / 未实现 / 未验证**。

---

## 1. 裁定结论（字段名统一口径）

两稿的**语义**一致，冲突只在**字段命名**。以下为唯一落点，tester 与 architect 均按此执行；改完即 `endpointFieldAlignment=ALIGNED`。

| # | 语义 | 采用字段名（唯一落点） | 处理 |
|---|---|---|---|
| A1 | 相位 occurrence 身份 | `context.endpointPhaseOccurrenceId` | **采用 architect 名**。tester 稿 §3.2/§6.2/§11 的 `context.phaseOccurrenceId` 改为 `context.endpointPhaseOccurrenceId`（同义，语义与 tester 描述完全一致：区分同名重复相位、缺失记 INCOMPLETE） |
| A2 | legacy 双算·历史分母语义复算 | `legacyEndpointFraction`（= `U/R`，R 为该 case 历史分母） | **采用 architect 名**。tester 稿 §5.3/§6.1 的 `legacySuspOnly.recomputed` 改为 `legacyEndpointFraction` |
| A3 | legacy 双算·现行代码路径实测 | `legacyMetricSuspensionFraction`（= `U/max(A,EPS)`，A = D+U+P） | **采用 architect 名**。tester 稿的 `wsMetricSuspensionFractionLive` 改为 `legacyMetricSuspensionFraction` |

**A2/A3 的语义分工（两稿原本都写对了，只是名字不同，合并后保留两组而非两组之一）**：

- `legacyEndpointFraction = U/R`——历史分母语义，`R` = 该 case 的历史分母（SLIP 用 `R_SLIP`、WS 用 `A_hist`）。它与「与历史 run 逐位对照」这一用途对应。
- `legacyMetricSuspensionFraction = U/max(A,EPS)`——现行 `WS_METRIC.suspensionFraction` 代码路径的实测值（`A` = `D+U+P`，C run 下与历史值必然不同）。
- tester 稿 §5.3 的**核心结论成立且必须保留**：C run 中两者必然不同（引入第三桶的必然结果，不是双算失败）；禁止互为别名；禁止任一单独放行；报告须分别标注。**该结论不因字段改名而变**。

## 2. tester §5.3 请求的结论（源码核对后给出）

tester 请 manager 就「历史分母语义」出结论。manager 已逐行核对源码，结论如下：

**tester 的源码主张全部属实**：

1. `getMetrics().suspensionFraction = suspended / max(totals.heavy + totals.light, EPS)`（`WaterSandField.ets:3400`）——属实。
2. `getTotals()` = `bh+mh+suspH`（重）+ `bl+ml+suspL`（轻）（`:1936-1947`）——**不含粒子桶**，故历史分母 `A_hist = D+U`——属实。
3. `updateMetrics()` 内另有一个局部量 `suspended / Math.max(this.initialHeavy + this.initialLight, EPS)`（`:6006`）——属实，且该量**只用于 dormant 判定、未发射**（不进入 `getMetrics()` 返回）——属实。**注意**：该局部量与 t90 的分母口径（`:6000/:6003` 除 initial）属同一族问题，已由 t90 epoch 修复卡（`6adc6412…beac`）按 CONS-04 处置，不在本 revision 范围。
4. `nextP011TimeMs()` 为 `Date.now()` 递增——属实（见 `WaterSandField.ets:1919` `this.lastP011PhysicsTime = nextP011TimeMs();`），故不能单独证明 1s 窗口完整覆盖。

**结论（manager 裁定，采纳 tester 方案）**：tester §5.3 的双算分列方案正确，采纳，并按 §1 的 A2/A3 统一字段名。理由：引入第三桶后，「按历史分母语义复算」与「现行路径实测」是两个不同的量，混用即失去与历史 run 的逐位对照能力；分列是唯一能同时满足「对照历史」与「披露现况」的做法。

## 3. architect 稿的两处修正要求

architect 稿的 `endpointFieldAlignment` 记为 `PENDING_TESTER_PROTOCOL`，并声明「本次核查时工作区尚未发现 tester endpoint protocol revision 落盘，故先以批准记录中的原名定契约，不虚构其 protocolId/revision 或文件名；字段名对齐状态不得写 ALIGNED」。

**该诚实声明成立且必须保留**（稿件在 tester 稿落盘前写成，16:24 vs 16:26）。现 tester 稿已落盘，故：

- **R1（必改）**：architect 稿 §7 字段对齐表末行与 §1 身份表须更新为：`endpointProtocolId = p011-endpoint-terminal-nondeposited`、`endpointProtocolRevision = endpoint-terminal-nondeposited-v1`（取自 tester 稿 §0，非自造）；`endpointFieldAlignment` 由 `PENDING_TESTER_PROTOCOL` 改为 `ALIGNED`，并记录对齐证据=本文件 + tester 稿 commit `69e27b9`。
- **R2（必改）**：architect 稿 §1 的「本次核查时 tester 稿尚未落盘」段落须改为历史陈述（标注 16:24 核查时点与 16:26 落盘事实），不得删除该诚实声明的痕迹。
- **R3（必改）**：architect 稿 §6 报告字段清单的「展示与双算」组须按 §1 补入 `legacyEndpointFraction` 与 `legacyMetricSuspensionFraction` 两个名字（其 §5 表已有定义，仅报告清单命名未统一）；`particles.byState` 的 `bySourceKind` 不可与 `byState` 再相加这条已在 §4 写明，保留。

## 4. tester 稿的三处修正要求

- **T1（必改）**：§3.2/§6.2/§11 的 `context.phaseOccurrenceId` → `context.endpointPhaseOccurrenceId`；§6.2 表格行同步改名。
- **T2（必改）**：§5.3/§6.1 的 `legacySuspOnly.recomputed` → `legacyEndpointFraction`、`wsMetricSuspensionFractionLive` → `legacyMetricSuspensionFraction`；§6.1 的 `legacySuspOnly` 整块按 §1 拆为两个具名字段（其块内已含「原分母/原 PASS-FAIL/身份」等信息，随字段更名保留内容）。
- **T3（必改）**：§13 追加记录须补一条「字段名统一（依 manager 对齐结论 §1），语义未变；本 revision 的冻结口径与反例表逐条未改」。

**不要求改动**：tester 稿 §1（分子细则 M1–M8）、§2（固定终点与末 1s 窗）、§4（状态分类与判定顺序）、§7（checker 适配说明 N1–N11）、§8（九项不涉及项）、§10（十条自检反例）经 manager 核对全部正确，**逐字不动**。

## 5. manager 已核实为属实的源码事实（两稿引用均对）

| 事实 | 出处 | 状态 |
|---|---|---|
| `getTotals()` 不含粒子桶（`bh+mh+suspH`/`bl+ml+suspL`） | `:1936-1947` | 核实属实 |
| `getMetrics().suspensionFraction = suspended/max(totals.heavy+totals.light,EPS)` | `:3388/:3400` | 核实属实 |
| `updateMetrics()` 内 `suspended/max(initialHeavy+initialLight,EPS)`，仅用于 dormant、未发射 | `:6006` | 核实属实 |
| t90 分母除 initial（与 depositedFraction 除 initial+sourceInjected 分叉） | `:6000/:6003` vs `:3397` | 核实属实（已另卡处置） |
| `finishNaturalObservation(null)` 位于 step 成功分支（`:1902`），**早于** `updateMetrics`（`:1918`）与 `p011SubstepId += 1`（`:1921`） | — | 核实属实——architect 稿「采样点须绑在成功封装点、只增新 ordinal 一次、不把旧 substepId 与下一帧 particles 串接」的风险描述成立 |
| 失败分支 `finishNaturalObservation(error)`（`:1886`）先于 restoreBackup | — | 核实属实——失败 attempt 不得进 endpoint 窗，但须留账做硬闸门审计 |
| checker 现按 `context.phaseD` 分组（同名重复相位会被合并） | `test/p011_slip_formal_check.py:213` | 核实属实 |
| checker `initial_deposit_total` = 首条 `before.bh + before.bl` | `:228-229` | 核实属实，即 `R_SLIP`；§7.1「不改」的要求正确 |
| checker 末 1s 窗按 `physicsTime`（墙钟）选取 | `:345-346` | 核实属实，§7.4「双窗并存」要求正确 |
| checker `EPS = 1e-9` | `:58` | 核实属实，与 revision 的 `epsMass=1e-9` 一致 |
| SDK `systemDateTime.getUptime(timeType: TimeType, isNanoseconds?: boolean): number` 存在，`TimeType.STARTUP = 0` | `~/AppData/Local/OpenHarmony/Sdk/26.0.0/ets/api/@ohos.systemDateTime.d.ts:256` / `:231` | 核实属实——architect 稿 §3.1 的时钟依据成立 |

## 6. 对 developer 实现卡的效力

`6adc6513…bdb0`（终点指标实现）的前置为「两版 revision 均已交付且字段名一致」。现状：两稿均已交付，**字段名经本裁定统一后即一致**。

→ 双方按 §3/§4 完成 R1–R3 / T1–T3 的改名与登记后，本文件 §1 的字段表即为 developer 的唯一实现契约。**developer 不得自行选择任一稿的字段名。**

## 7. 未变更项（引用两稿时必读）

- 用户 2026-10-24 批准的五项裁定（分子/分母/5%/末 1s 窗/peakSusp 另案）**逐字未改**，两稿与本裁定均未触碰。
- `DUALFIELD-*` 16 例的 CONS/BUD/LIFE/SRC/GES estimand **原样保留**。
- 冻结指标（30°±2°/t90/R²/5% 阈值/300/s/128 列/守恒库存容量有限值）**零改动**。
- 协议生效条件不变：实现落地 + designer 验证 + manager 确认适用范围之前，协议**不产生放行效力**；C run 继续双口径并报，任一单口径不单独放行；旧报告保留 `legacy-susp-only` 身份不追认。
