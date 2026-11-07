# P-011 endpoint 诊断字段与 particleEvents 校验职责登记 v1

日期：2026-10-24  
任务：6adcc824e4b03273ad604303  
状态：**ARCHITECTURE_REGISTRATION / PENDING_MANAGER_ACCEPTANCE / DEVELOPER_ACTION_REQUIRED / NOT_IMPLEMENTED / NOT_VALIDATED**  
字段身份：沿用 `endpointFieldRevision=p011-endpoint-fields-v1`；本登记只裁定字段序列化名与职责，不改 estimand、阈值或已发布协议。

## 0. 输入和边界

本登记以以下文件/证据为唯一依据：

- [endpoint 协议 v1](endpoint-protocol-P011-terminal-nondeposited-v1.md) §6.1–§6.2，身份 `protocolId=p011-endpoint-terminal-nondeposited`、`protocolRevision=endpoint-terminal-nondeposited-v1`。
- [P-011 endpoint 字段稿 rev2](architecture-P011-endpoint-fields-revision-v1.md) §4、§6–§7。
- [manager 字段对齐裁定](decision-P011-endpoint-field-alignment-v1.md) §1 A1–A3。
- [协议—实现一致性报告](endpoint-conformance-report-P011-terminal-nondeposited-v1.md) §1.6、§3 D-2/D-6。
- 当前实现证据：`test/p011_endpoint_check.py` 中 `mass()` 对 `particles.byState` 的诊断分类，`check()` 对 `particleEvents` 的 list 存在性检查，以及行 329、407–412 的诊断字段生成路径。检查点引用以任务卡给出的代码版本/行号为定位，落地前 developer 应重新确认提交版本。
- 已交付双报告：[endpoint dual report](endpoint-dual-report-P011-terminal-nondeposited-v1.md)；manager 2026-10-24 22:50 已验收其作为本卡前置。

本文件是独立的字段/职责登记，不回写已发布协议 v1，不改 checker、测试或数据，不追认历史 run。任何 tester 协议字段表更新必须由 tester 发布新 protocol revision；历史 v1 仍保留原文和原身份。协议生效条件和双报告/真机队列不因本文件改变。

## 1. D-2 唯一字段落点裁定

### 1.1 裁定

**checker 报告唯一使用三个现存具名字段：**

- `particleResidueDiagnosticStatus`
- `particleResidueDiagnosticCodes`
- `particleResidueDiagnosticMeaning`

不输出、不读取、不保留并行别名 `endpointDiagnosis`。报告消费方只按上述三个具名字段取值。协议 v1 §6.1 的 `endpointDiagnosis` 是其当时的抽象对象描述；向前对齐时由 tester 在新协议 revision 的 §6.1 字段登记中拆成这三个字段，按下表映射，不改变协议诊断语义。不能只在文档中写“兼容两种名字”，不能在同一新报告中同时输出两套结构。

### 1.2 三字段契约

| 唯一字段名 | 类型/可选性 | 原有语义 | 采集/生成时机 | 归属 | 缺失或异常分类 |
|---|---|---|---|---|---|
| `particleResidueDiagnosticStatus` | 必填 enum `{OBSERVED, NOT_OBSERVABLE}` | `OBSERVED` 表示该提交端点的 `particles.byState.FLYING/CONTACT_WAIT` 可用、有限且与粒子摘要闭合；`NOT_OBSERVABLE` 表示没有可用 byState 诊断证据。它不改变 `particleMass` 或 `endpointFraction` 的估计对象 | checker 从终点同一 post-commit `particles` 快照读取/验证 byState 后，为该 occurrence 生成一次 | **checker 派生**；native 负责在账本提交 `particles.byState`，不自行声明最终报告诊断状态 | 输入缺 byState：合法诊断结果为 `NOT_OBSERVABLE`，质量窗仍可按已存在的 canonical 粒子摘要计算；输入 byState malformed/非有限/与摘要不闭合：既有 `INVALID_LEDGER` 硬失败。报告输出字段缺失/未知值：列入 `reportMissingFields`，`reportIdentityStatus=INCOMPLETE`，不得默认为 `OBSERVED` |
| `particleResidueDiagnosticCodes` | 必填 `string[]`；允许空数组 | 可机读的滞留诊断码。byState 不可观测时包含 `NOT_OBSERVABLE`；观测到末端 `CONTACT_WAIT.count>0` 时包含 `PERSISTENT_PARTICLE_RESIDUE`。两码可并列，后者不宣称永久滞留 | checker 完成终点状态与 byState 分类后产生；不从粒子事件的绘制/可见性推断 | **checker 派生**；native 只提供支持该判断的 byState/逐粒证据 | 输入缺 byState 时用 `NOT_OBSERVABLE` 码而非缺字段状态；报告缺 codes、非数组或含未知码时 `reportIdentityStatus=INCOMPLETE`/实现契约错误，保留原始问题，不伪造空数组 |
| `particleResidueDiagnosticMeaning` | 可选 `string`；没有解释性消息时可省略 | 对诊断码的可读说明。当前 `PERSISTENT_PARTICLE_RESIDUE` 的现有文案保持原义：`Endpoint CONTACT_WAIT present; no lifetime/permanent-suspension conclusion`。不增加寿命推断、不把观察窗外推 | checker 仅在产生需要说明的诊断码时填写；其余可缺省 | **checker 派生说明文本**，不是 native 原始量 | 省略是合法的；若存在则必须与诊断码相符。若 producer/报告声称实现此修订却给出不兼容文本，登记 schema/conformance error，不改变 endpoint 数值状态或阈值 |

**独立可选输入字段：** `particles.byState.FLYING/CONTACT_WAIT.{count,massH,massL}` 沿 endpoint 字段稿 rev2 仍为 optional。缺失时 checker 输出前两项诊断为 `NOT_OBSERVABLE`，不把其改成质量字段缺失的 `NOT_IMPLEMENTED`。若 `particles` canonical mass 摘要本身缺失，仍按既有协议输出 `NOT_IMPLEMENTED`，不能用 `NOT_OBSERVABLE` 掩盖。末窗 CONTACT_WAIT 的诊断码与数值 PASS/FAIL可并列；它不是新寿命阈值，也不改变 manager 的独立出口审查。

现有独立字段 `particleResidueEvidencePaths` 与 `residueObservedDurationMs` 继续按 endpoint 字段稿 §6 单独登记；它们不是上面三字段的别名或嵌套项，也不随 `endpointDiagnosis` 名称映射被删除。尤其观察时长缺证据仍按原不可观测语义，不可用诊断 meaning 补造。

若报告生成器未生成必填的 `particleResidueDiagnosticStatus` 或 `particleResidueDiagnosticCodes`，这是**报告契约缺项**：把确切字段路径放入 `reportMissingFields` 并置 `reportIdentityStatus=INCOMPLETE`。不覆盖 `endpointStatus` 的既有判定优先级；数值状态、硬闸门和报告身份分开披露。`particleResidueDiagnosticMeaning` 为可选解释文本，不因缺省单独报缺项。

### 1.3 与 tester protocol §6.1 的登记对照

| tester protocol v1 §6.1 | 新报告唯一字段 | 对齐结论与理由 |
|---|---|---|
| `endpointDiagnosis: object`（对象内含诊断码与依据） | 不再作为新报告字段；拆分为 `particleResidueDiagnosticStatus`、`particleResidueDiagnosticCodes`、可选 `particleResidueDiagnosticMeaning` | **一对三拆分登记**。保留“可观测状态、机器码、解释”三个原义，不新增证据/推断。消费方不再读取泛型对象，避免实现已产字段无人消费 |
| 状态/可观测性 | `particleResidueDiagnosticStatus` | 采用已有实现字段和字段稿名；byState 缺失映射 `NOT_OBSERVABLE`，质量分子不变 |
| 诊断码 | `particleResidueDiagnosticCodes[]` | 采用已有实现字段；保留 `NOT_OBSERVABLE` / `PERSISTENT_PARTICLE_RESIDUE` 的既有含义 |
| 依据/说明 | `particleResidueDiagnosticMeaning` | 采用已有实现字段；仅解释已输出 code，不替代账本证据 |
| 协议 §6.2 `particleEvents[]` | ledger record顶层 `particleEvents[]` | 七类事务名/语义不变；checker实现与测试fixture当前读取record顶层，本登记将该处作为运行时唯一路径并登记tester新revision同步修正文档路径 |

### 1.4 已发布协议 v1 的处理方式

- `endpoint-protocol-P011-terminal-nondeposited-v1.md` §6.1 **保持原样、原 hash、原 `endpointProtocolRevision`**；不得为了清 D-2 原地改正文、改 commit 记录或修改历史报告中的协议身份。
- tester 在新 protocol revision 的 §6.1 登记三个具名字段和上述 mapping；新版本中不再保留 `endpointDiagnosis` 作为第二个消费名。新 protocol revision ID/生效身份由 tester 按协议版本流程登记，本文件不猜造版本号。
- 新实现/新报告的 `endpointProtocolRevision` 按运行时实际使用的 tester protocol revision 填写。旧报告仍标 v1 及其旧字段规范；重跑才使用新 protocol revision，不追改旧报告 JSON/Markdown。
- 对照表与修改证据链接回本文件和 tester 新 revision；字段名统一不构成 endpointFieldRevision 或 estimand 自动升级。如需改变任何诊断语义/判定条件，另立 revision 并重新走批准。

## 2. D-6 `particleEvents` 事件级校验职责

### 2.1 唯一归属

**七类 `particleEvents` 的逐笔内容、提交前后状态、质量守恒和原子关联的验收校验归独立 endpoint checker。** checker 是从序列化 ledger 复算的独立验证者，不依赖 producer 自报的 `eventValid=true` 或事务层自己的断言作为 PASS 证据。

`P011ParticleField`/`WaterSandField` 事务层的唯一职责是：按已冻结事务契约生成完整事件，和粒子池、字段数组、预算票据、列缓存、journal 在同一物理提交中原子 publish/rollback；提交前局部预检可保留作防御性断言，但不能将这些断言当作跨层验收 checker，字段 revision 明确写 **`eventLevelValidationOwnership=ENDPOINT_CHECKER` / transaction layer: emit-and-atomic-commit only; event-content conformance check is not owned here**。

checker 的职责边界也要明确：它读取已发布 ledger、计算事件与快照关系；它不替事务层创建/修补事件，不回写物理状态，不决定物理事务原子化，也不按事件累计值代替 endpoint 的 post-commit `U+P` 原始质量重算。事件只是守恒/原子性关联证据，不加入 endpoint 分子第二次。

### 2.2 输入与逐笔通用判据

每个 endpoint eligible 成功提交记录至少关联同一 `commitKey=(runId,sequence,frameId,substepId)` 的：`stateBefore`、`stateAfter`、`particles` canonical摘要/逐粒快照（协议要求的证据可用时）、预算/guard/audit、**record顶层** `particleEvents[]`。当前 checker 与测试fixture均以 `record.get('particleEvents')`读取顶层字段；协议 v1 §6.2登记为`context.particleEvents`，本登记确认新写入和新checker契约唯一落点为record顶层并要求tester仅在新协议revision更新该项登记，v1不改。checker 对数组逐项遍历；`[]` 是合法空事件序列，但仅当该提交的池/字段/来源状态没有未解释的事务变化。数组缺失/类型错误或事件需要的字段缺失 → `NOT_IMPLEMENTED` 并报告确切 JSON path；已存在事件与提交状态矛盾、重复/未知 kind、非有限或负的事务量、同一质量双记、预算/guard/列缓存/总量闭合失败 → `INVALID_LEDGER`，列出 commit key、数组项序号、kind、粒子ID和具体规则。

推荐的失败证据定位形式（诊断路径，不新增 estimator 输入）：`ledgerPath + ledgerHash + JSON Pointer /records/<recordIndex>/particleEvents/<eventIndex> + commitKey + particleId(s) + before/after snapshot key + observed/expected deltas`。checker 的 `hardGateFailures` 挂稳定 rule code，`missingFields`/`reportMissingFields` 保存缺项路径；原始 ledger 与摘要/快照保留为主证据，汇总 Markdown 只作展示。数组位置作为同一 commit 内事件定位符，不改变既有事件的 sourceId/particleId语义。

通用审计必须满足：

1. event 必须只归属一个成功 post-commit `commitKey`；不能把 failed attempt 的事件纳入成功窗。若失败 attempt 在归档中保留，另作失败审计证据，不能与成功事件混算。
2. event `kind`、phase/source/particle ID、requested/granted/mass、from/to列等必填字段符合架构 §5.6 的该类 schema；值有限、质量非负、枚举合法，关联ID在对应提交前后快照/lineage 中可追溯。
3. 每个粒子池或物理字段的前后差分，都要被本提交的一笔或一组明确定义事件解释；事件不能重复解释相同 delta，也不能遗漏 delta。将事件按事务类型映射到来源/床/悬沙/粒子桶后，分相质量 `D+U+P` 与 `E` 的 post-commit 恒等式仍按既有守恒判据复核；累计 journal 量不叠加进存量分子。
4. 对应预算/guard账本的 request、grant、退款/余额、逐列 delta 与事件相互一致；粒子事务和 field sink 的成功/失败状态一致。只对事件结构检查通过但状态差分未闭合，仍为 `INVALID_LEDGER`。
5. 同一事务所有事件关联与状态快照必须在原子提交边界闭合；publish/rollback 后的孤立事件、同一事件重复落账、成功事件却未改变对应源/目的桶均失败。重放或 duplicate commit 由既有 commit-key 检查先行拦截，不能因 eventId 不同重复计量。

### 2.3 七类事件的事件—状态核查表

具体字段名以 dual-field 架构 §5.6、事务契约 §13 和 tester protocol §6.2 为准；下表是其 checker 对账职责，不增加事件估计量/门槛。

| kind | 至少关联的事件内容（以v7现有表为准） | checker 对提交前后状态/账本逐笔核查 |
|---|---|---|
| `SPAWN` | `particleId,phase,mass,sourceKind,x,y,sourceId,requested,admitted` | 新增粒子ID/质量/相别/source 与池差分一致；`GESTURE_EXTERNAL` 的 admitted 与 sourceInjected增量/receipt闭合且只计一次；`SHAKE_LIFT` 不能增加 expected/sourceInjected |
| `LIFT` | `particleId,phase,mass,fromLayer,column,requested,granted` | 床层对应分相净减少与真实 spawn 粒子增加一致；通过的guard和预算量与grant一致；相内转移不增加IH/IL。拒绝/零grant不能凭空生粒或扣床 |
| `LAND` | `particleId,phase,mass,column,requested,granted,deltaS,guardPreview,remainder,remainderDisposition` | requested=granted+remainder；床层/phase实际增加等于grant；粒子质量只减grant；remainder保留原ID/sourceKind并为CONTACT_WAIT；只有质量归零才释放；不能同一grant还记入susp |
| `CROSS_COLUMN` | `particleId,phase,mass,fromColumn,toColumn,budget{local,bottom,total},distance` | 原列粒子扣与目标列粒子加相等、粒子总质量与床/悬沙不变；from/to邻列和CFL合法；local双端/total单次记账符合已冻结预算票据 |
| `DEGRADE` | `particleIds[],phase,mass,toColumn,toSuspH/toSuspL,flag=PARTICLE_DEGRADED_TO_COLUMN` | 在途粒子扣除量等于悬沙增加量，分相/目标列和flag一致；expected不变，不把累计degraded量再加入U/P或分子 |
| `MERGE` | `particleIds[],phase,massBefore,massAfter,positionMomentPreserved` | 入包身份/phase/source/状态符合免费合并域；池质量前后等量、幸存ID/lineage可追；positionMomentPreserved与实际加权位置/事件字段一致；不得靠跨列/跨源免费合并消账 |
| `TO_SUSP` | `particleId,phase,mass,toColumn,suspDelta,reason` | 若现行协议/route revision启用：粒子质量减少必须等于显式guard授权并记账的susp增加，事件说明唯一原因，不重复计质量。当前 dual-field rev2 明确此路径 schema保留、v1不启用且LAND余量必须留原粒；当前revision run若出现TO_SUSP，按与冻结事务路径冲突记 `INVALID_LEDGER`，未来启用须由新字段/协议revision先登记 |

事件字段或v1事务路径并不支持某规则时，checker 标记缺证据/不一致并按上述状态分流，不能由checker临场“合理化”新路径。特别地，`PERSISTENT_PARTICLE_RESIDUE`诊断只依终点byState/其可选逐粒证据；不从LAND/TO_SUSP历史事件推断粒子永久年龄。

### 2.4 两层明确的“不在本层”声明

- **M1/M2 粒子事务层：**“本层负责事件构造、事务字段校验、与池/场/预算/journal的原子提交或回滚；跨记录端点报告的`particleEvents`事件级守恒/证据一致性验收由 endpoint checker 负责，本层不宣称该checker职责已由事务成功替代。”
- **Endpoint checker：**“本层负责对序列化v7事件和提交前后快照做独立事件级核查；本层不执行物理事务、不生成/修补事件、不将事件累计量加入`endpointFraction`，也不代替粒子事务层的原子publish/rollback。”

以上声明随 developer 的 D-6实现报告/测试报告登记；不能因为两层都做局部断言就把D-6标成双重归属。

## 3. tester 协议字段对照与版本交接

| 资产 | 当前状态 | 本裁定效力 |
|---|---|---|
| tester protocol v1 §6.1 `endpointDiagnosis` object | 已发布，原文/哈希/身份冻结 | 保持历史；只作为三字段之前的抽象命名，不再被新报告消费 |
| tester protocol v1 §6.2 `context.particleEvents` location | 已发布，原文/哈希/身份冻结；与当前 checker/test fixture 的record顶层读取路径不一致 | 保持历史；tester 新revision将字段位置登记为record顶层 `particleEvents[]`，不得在新实现中同时接受两处路径以“兼容” |
| tester 下一 protocol revision §6.1–§6.2 | 尚待tester登记 | §6.1将三个具名诊断字段登记为准确类型并删除并行`endpointDiagnosis`选项；§6.2登记record顶层 `particleEvents[]`。两项只更新登记，不改端点公式和诊断码语义 |
| architect endpoint fields revision v1 rev2 §6 byState/滞留 | 已有三字段名及optional byState语义 | 与本裁定对齐；该字段身份不改变，独立文档作为本任务的实现交接 |
| manager decision A1–A3 | 已完成且只约束occurrence/legacy双算命名 | 本任务在它之后新增 D-2/D-6 登记；不重写或扩大A1–A3原裁定 |
| developer endpoint checker | 三字段已部分生成；事件数组只验list | D-2补齐字段schema/对外消费；D-6新增上述事件语义校验；未验收前不表示缺陷关闭 |

版本迁移：tester 新协议生效前，报告继续附现有 `endpointProtocolId/endpointProtocolRevision`，字段登记状态不宣称完全一致。新协议 revision 发布后，新的 checker output 只输出三个具名字段，读取record顶层 `particleEvents[]`；通过其 protocol revision 标识消费契约。历史报告保持原 protocol v1身份与历史字段快照，绝不反向改写或重新解释。

## 4. 验收判据、责任门序与证据交接

### 4.1 D-2验收

- 新报告序列化严格具有 `particleResidueDiagnosticStatus` 和 `particleResidueDiagnosticCodes`；`particleResidueDiagnosticMeaning` 仅在有解释时可选。
- 输出/消费路径无 `endpointDiagnosis` 别名。Tester 新协议revision登记的 schema 与本文1.2/1.3逐字段相同。
- 用既有协议冻结样例核对：byState缺失→`NOT_OBSERVABLE`，contact wait>0→已有诊断码与原meaning，byState正常无残留→OBSERVED+空codes；byState不闭合→现有 `INVALID_LEDGER`，canonical particles缺失→现有 `NOT_IMPLEMENTED`。不得改判定公式或任何estimand。
- 缺报告必填字段可定位至确切 JSON path 并反映在报告身份完整性；不挤入数值状态码或填空串。

### 4.2 D-6验收

- 完整复核上面七类事件，覆盖空数组、各类合法/非法事件、事件字段缺失、重复/未知事件、无池差分/无字段差分、质量/guard/预算不闭合、失败回滚、source/particle lineage 和跨commit错配。
- 每个反例均以原始ledger hash、commit key和event index为证；期望 `NOT_IMPLEMENTED`（必要证据字段缺失）或 `INVALID_LEDGER`（语义/原子性/守恒矛盾）按2.2分流，保留既有overall状态优先级。
- 合法事件只验证，不重算/累加 endpoint M；同一输入原有 DUALFIELD estimand、endpoint U+P/M/R、5%/末窗/阈值完全不变。
- 事务层记录原子提交证据及其本层责任声明；checker单独给出逐笔核对结果及其不负责物理提交的声明。两者有明确接口，验收职责唯一在checker。

### 4.3 门序

1. manager 验收本登记，关闭 architect D-2/D-6 设计前置。
2. tester 在新 endpoint protocol revision 更新 §6.1 字段登记；历史协议 v1 不变。
3. developer 按唯一字段名与 checker 事件职责实施，补充符合现行测试职责的用例/报告；本卡不修改判据或测试。
4. reviewer/manager按新字段revision复核后再推进已登记协议生效前置；真机采集仍由 effector 按队列独占，本卡不申请、不占用设备。

## 5. 红线自检

- [x] 诊断对象只有唯一新落点：三个具名 `particleResidueDiagnostic*` 字段；无可供新报告选择的 `endpointDiagnosis` 别名。
- [x] 明确 tester protocol v1不回改；更新只发生在 tester 新 protocol revision 的字段登记。
- [x] 七类事件级验证唯一归 checker；事务层只负责原子生成/发布及局部内在约束，并明确本职责不在本层。
- [x] 所有原字段含义只作显式化，不把 `NOT_OBSERVABLE` 改成 `NOT_IMPLEMENTED`，不推断粒子寿命，不将事件量加进 endpoint mass。
- [x] 不改 `DUALFIELD-*` estimand、冻结指标、任何数值阈值、endpoint公式/窗口、协议生效状态或历史route结果。
- [x] 本文件是纯登记设计；未改代码/测试，未用真机，未宣称D-2/D-6实现通过。

## 6. 修订记录

- 2026-10-24，v1，按任务6adcc824e4b03273ad604303登记D-2诊断字段唯一落点与协议新revision更新方式、checker/事务层D-6职责边界、七类事件逐笔判据、状态分流和证据路径。依据协议v1、architect字段稿rev2、manager字段对齐裁定及tester一致性报告；纯设计，待manager验收，未触碰实现、用例、阈值或真机。
