# P-011 终点未沉积沙比例 endpoint protocol 勘误 revision（v1-errata1）

日期：2026-10-24。发布：tester（P-011）。状态：**ERRATA_ONLY / NO_JUDGEMENT_CHANGE / NOT_IMPLEMENTED / NOT_VALIDATED / DUAL_KALIBER_MANDATORY / NO_CAPTURE_AUTHORIZATION**。

本文是对已发布协议 `docs/endpoint-protocol-P011-terminal-nondeposited-v1.md`（commit `69e27b9` 首发 + `a3ca53c` 字段改名）的**叠加勘误**：只登记勘误内容与修订理由，**不回改已发布协议文件**（该文件保持原样，读者须与本文件合并阅读）。

## 0. 身份与勘误范围

| 项 | 取值 |
|---|---|
| `protocolId` | `p011-endpoint-terminal-nondeposited`（不变） |
| 被勘误版本 | `protocolRevision = endpoint-terminal-nondeposited-v1`（commit `69e27b9` + `a3ca53c`） |
| 本勘误 revision | `endpoint-terminal-nondeposited-v1-errata1` |
| `endpointMetricRevision` | `terminal-nondeposited-v1.1`（**不变**——勘误不改指标版本、不改估计对象） |
| `endpointAuthority` | `APPROVED`（不变） |
| `endpointFieldAlignment` | `ALIGNED`（不变） |
| 勘误范围 | 仅协议 **§10 示例表第 1 行**的数值与口径标注；外加两份已交付报告内的两处内部不一致（笔误值与计数口径） |
| 明确不改 | 协议 §1–§9 全部判定条款：分子细则 M1–M8、分母、阈值 5%、`epsMass=1e-9`、末 1s 窗、固定终点定义、采样与身份契约、时钟契约、六码状态分类与判定顺序、双算规则、九项不涉及项；§7 checker 适配 N1–N11；DUALFIELD-* 预注册 estimand；冻结指标（30°±2° / t90 / R² / 5%）；历史报告结论；实现代码与测试判据 |
| 依据 | 一致性报告 `docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md` §3 D-7 与 §4；双报告 `docs/endpoint-dual-report-P011-terminal-nondeposited-v1.md` §4.2 P4；协议 §5.3（A2/A3 定义）；`docs/decision-P011-endpoint-field-alignment-v1.md` §1 A2/A3、§2 |

**性质声明**：本勘误为**文本与标注勘误**，不改变任何判定结论、不改任何判定式、不新增或删除断言。协议在此勘误前后的判定行为逐位相同。

---

## 1. E-1：协议 §10 示例表第 1 行的 legacy 分母口径混用

### 1.1 勘误前（v1 原文，逐字引述）

> | 1 | `E=8000, D=7550, U=322, P=128`，末窗保持 | `U/E=4.025%` PASS | `M/E=5.625%` FAIL | 旧口径假 PASS，新口径正确 FAIL；只加粒子不会把旧 FAIL 变 PASS（`F_new−F_old=P/R`） |

### 1.2 勘误后（本 revision 生效的读法）

| # | 输入 | 旧口径（两个 legacy 量分列） | 新口径 | 结论 |
|---|---|---|---|---|
| 1 | `E=8000, D=7550, U=322, P=128`，末窗保持 | **A2** `legacyEndpointFraction = U/(D+U) = 322/7872 = 4.0904%` **PASS**；**A3** `legacyMetricSuspensionFraction = U/(D+U+P) = U/E = 322/8000 = 4.025%` **PASS** | `endpointFraction = M/R_WS = M/E = 450/8000 = 5.625%` **FAIL** | 两个 legacy 量**均** PASS、新口径 FAIL；只加粒子不会把旧 FAIL 变 PASS。差额恒等式须**按分母配对**陈述：对 A3（`A = R_WS = E`）有 `F_new − F_A3 = P/E = 1.6000%`；对 A2（`R = D+U = 7872`）有 `F_new − F_A2 = 1.5346%`，**不等于** `P/R = 1.6260%` |

### 1.3 修订理由

1. **原表把两个不同的 legacy 量写成了一列**。按裁定 §1 的 A2/A3 与协议 §5.3：
   - `legacyEndpointFraction`（A2）= `U/R`，`R` 为该 case 的历史分母；**WS 侧 `R = A_hist = D+U`**；
   - `legacyMetricSuspensionFraction`（A3）= `U/max(A,EPS)`，`A = D+U+P`（C run 的现行代码路径实测值）。
2. **原表的 4.025% 是 A3 值，不是 A2 值**。本例守恒闭合使 `A = D+U+P = 8000 = E`，故 A3 恰好等于 `U/E = 4.025%`；而 A2 按 `D+U = 7872` 应为 `322/7872 = 4.0904%`。原表把 A3 的数值冠以「旧口径 `U/E`」的标签，与 §5.3 的 A2 定义（`D+U`）同列并列，读者会误以为「4.025% 就是 A2 值」。
3. **两个量的分母不同是设计意图**（引入第三桶的必然结果），§5.3 已冻结「禁止互为别名、禁止任一单独放行、报告须分别标注」。示例表作为自检工具，若自身混用两列口径，就失去「核对实现是否照协议执行」的作用——这正是它本应拦截的错误类型。
4. **差额恒等式的标注同样受分母配对影响**：原写 `F_new − F_old = P/R` 只有在「旧口径分母与新口径分母相同」时成立。本例 A3 的分母 `A` 恰与 `R_WS = E` 相同，故对 A3 成立；对 A2（分母 `D+U`）不成立。原表未区分，属同一处口径混用的连带标注问题。

### 1.4 结论不变

- 旧口径（无论 A2 的 4.0904% 还是 A3 的 4.025%）均 **PASS**（< 5%）；
- 新口径 `M/E = 5.625%` 仍 **FAIL**（> 5%）；
- 该行的教学结论「旧口径假 PASS、新口径正确 FAIL」**完全不变**；
- 实测佐证：`test/reports/endpoint_protocol_v1_execution.json` 的 R1 实际输出 `legacyEndpointFraction=4.090447154471544`（=4.0904%）、`legacyMetricSuspensionFraction=4.025`、`endpointFraction=0.05625`，与本勘误后的三值逐位一致。

---

## 2. E-2：`4.0894%` → `4.0904%` 数值笔误（三处）

`322/7872 = 0.040904471544715444 ≈ 4.0904%`。一致性报告 §3 D-7 与执行脚本/JSON 的 note 曾把该值写成 `4.0894%`，属数字笔误（同报告 §1.6/§3 另处已写 4.0904%）。三处已更正，痕迹保留在对照表与 commit 历史中。

| # | 位置 | 勘误前值 | 勘误后值 | 依据 | 影响面 |
|---|---|---|---|---|---|
| E-2.1 | `docs/endpoint-conformance-report-P011-terminal-nondeposited-v1.md` §3 缺陷表 D-7 行 | `U/(D+U)=4.0894%` | `U/(D+U)=4.0904%（322/7872）` | `322/7872` 复算；实测 R1 `legacyEndpointFraction=4.090447154471544` | 无——D-7 的判定结论（结论不变、须另起 revision）不受影响 |
| E-2.2 | `test/p011_endpoint_protocol_v1_execution.py` 中 R1 的 `note` 文案 | `U/(D+U) -> 4.0894%` | 拆为 A2/A3 两量的完整表述，`U/(D+U) -> 4.0904%` | 同上 | 无——`note` 为说明性文案，不参与 `verdictMatches` 判定 |
| E-2.3 | `test/reports/endpoint_protocol_v1_execution.json` 中 R1 的 `note`（由脚本重跑再生成） | 同 E-2.2 旧文案 | 同 E-2.2 新文案 | 同上 | 无——JSON 的 `verdictMatches` 仍 10/10 |
| E-2.4（附带） | 同 note 内的缺陷编号指针 | `See defect list D-1.` | `See defect list D-7 and docs/endpoint-errata-…` | D-1 为回算身份枚举缺口，与本处无关的编号误指 | 无——仅为指针更正 |

**保留痕迹的方式**：勘误前值记录在本对照表与 commit 历史中；正文一律写勘误后值，不保留旧数值（避免二次误读）。

---

## 3. E-3：条款计数口径统一（双报告 §4.2 的 P4 行）

### 3.1 来源查明

双报告 §4.2 的 P4 行原写「**28/30 实质条款一致**」。逐项查明：

| 候选来源 | 是否对应 | 说明 |
|---|---|---|
| 一致性报告 §1.1–§1.5 节点表 | **否** | 实际为 9+11+6+8+11 = **45 条**判定口径类条款，其中 43 条一致、2 条指向同一枚举级缺陷 D-1（§5「回算身份」与 §7 N11 为同一条款在两表中的出现） |
| 一致性报告 §1.6 字段级核对表 | **否** | 该表 **28 行**，但性质是「报告字段与 architect §6 清单」的**字段级**核对（承载 D-2～D-6），不是「实质条款」，不能作为 28 的来源；其判定分布为 8 条一致 + 若干命名/未实现/未回填项 |
| 一致性报告 §4 结论 | **是（唯一现行口径）** | 「判定口径类条款共 45 条……43 条完全一致，另 2 条指向同一枚举级缺陷 D-1」；manager 已在 `docs/priority.md` 逐节点复核 9/11/6/8/11=45 |

**结论**：`28/30` 是双报告撰写时沿用的**初稿粗略计数**，与一致性报告 §1.1–§1.5 的节点表**无逐节点对应关系**，属被节点级复算取代的超期数字。**特别提示**：`28` 易与 §1.6 字段级表的 28 行混淆，但两者性质不同，不得互相引用。

### 3.2 更正

双报告 §4.2 的 P4 行已改为按一致性报告 §4 的口径陈述（45 条 / 43 条一致 / 2 条指向 D-1），并注明计数依据（§1.1–§1.5 节点表逐节点复算）与本次勘误出处。

**关于旧值是否保留**：P4 行内仍以「原写的『28/30 实质条款』为初稿计数」的形式出现旧值——这是**来源说明的有意保留**（对应任务要求「查明 28/30 的实际所指并写明来源」），不是未清理的笔误；其余正文一律使用 45/43 口径。若后续经理方要求彻底移除旧数字，可另起勘误登记。

### 3.3 P4 结论不变

**P4 仍为「已满足」**：未决项在更正前后一致——D-1 为枚举级、D-2 为字段级命名、D-3～D-6 为报告元数据/事件级缺口，均不改变终点数值判定。更正只替换了计数表述，未改变满足性判断。

---

## 4. 勘误对照汇总表

| 编号 | 对象 | 勘误前 | 勘误后 | 依据 | 影响面 |
|---|---|---|---|---|---|
| E-1 | 协议 §10 示例表第 1 行「旧口径」列 | `U/E=4.025%` PASS（单列、口径混用） | A2 `U/(D+U)=4.0904%` PASS 与 A3 `U/(D+U+P)=U/E=4.025%` PASS **两量分列**；差额恒等式按分母配对陈述 | 裁定 §1 A2/A3、协议 §5.3、§10 自检表用途 | 无判定变化：两个 legacy 量均 PASS、新口径 5.625% 仍 FAIL |
| E-1（连带） | 同行结论列的 `F_new−F_old=P/R` | 未区分分母 | 对 A3（`A=R_WS=E`）成立 `P/E=1.6000%`；对 A2（`R=D+U`）为 `1.5346%`，不等于 `P/R` | 同上 | 无判定变化 |
| E-2.1 | 一致性报告 §3 D-7 行 | `4.0894%` | `4.0904%（322/7872）` | `322/7872=0.04090447…`；实测 R1 | 无 |
| E-2.2 | 执行脚本 R1 `note` | `4.0894%` | `4.0904%` + A2/A3 完整表述 | 同上 | 无（文案不参与判定） |
| E-2.3 | 执行 JSON R1 `note` | 同 E-2.2 旧文案 | 同 E-2.2 新文案（重跑生成） | 同上 | 无（`verdictMatches` 仍 10/10） |
| E-2.4 | 同 note 缺陷编号指针 | `D-1` | `D-7` + 勘误文件指针 | 编号归属 | 无 |
| E-3 | 双报告 §4.2 P4 行 | `28/30 实质条款一致` | `判定口径类 45 条中 43 条一致、2 条指向 D-1`（注明依据=§1.1–§1.5 节点复算） | 一致性报告 §4；`docs/priority.md` manager 复核 | 无——P4 仍「已满足」 |

---

## 5. 红线核验

| 红线 | 状态 |
|---|---|
| 不改协议 §1–§9 的判定条款、阈值（5%）、末 1s 窗、固定终点定义、六码状态分类与判定顺序、双算规则 | ✅ 已发布协议文件**零改动**（`git status` 干净） |
| 不回改已发布的历史协议版本 | ✅ 本文件为叠加勘误，`69e27b9`/`a3ca53c` 的协议文件保持原样 |
| 不改 DUALFIELD-* 预注册 estimand | ✅ 未触碰 |
| 不改冻结指标（30°±2° / t90 / R² / 5%） | ✅ 未触碰 |
| 不改历史报告结论 | ✅ 两份报告只改笔误值与计数表述，结论与身份标注原样 |
| 不改实现代码与测试判据 | ✅ 仅 `note` 文案（说明性，不参与判定）；`verdictMatches` 逻辑未改 |
| 不追认旧 route 结果 | ✅ 无追认 |
| ESTIMAND 变更须另起 revision（本卡只做勘误） | ✅ 估计对象零变更，`endpointMetricRevision` 仍 `terminal-nondeposited-v1.1` |

---

## 6. 复现与验证

| 验证项 | 命令 | 结果 |
|---|---|---|
| §10 十条反例 | `python test/p011_endpoint_protocol_v1_execution.py` | **10/10 MATCH**（改后重跑确认） |
| 交付自测（Python） | `python -m unittest test.test_p011_endpoint_check` | **20/20 通过**（本卡改后重跑；较交接时的 17/17 增加了 3 例，为其他会话并行新增，非本卡改动） |
| 交付自测（Node） | `node test/run_p011_endpoint_units.cjs` | **9/9 通过**（较交接时的 8/8 增加 1 例，同为并行新增） |
| 三处 `4.0894%` 残留 | `grep -rn "4\.0894" docs/ test/ --include="*.md" --include="*.py" --include="*.json"` | **判定文本与正文 0 处残留**。仅剩两类有意保留：①本文件与两份报告「勘误记录」对照表的**勘误前值列**（痕迹，按「保留原文本痕迹」要求）；②`docs/priority.md` 的 manager 发现记录（他人文件，不在本卡改动范围）。三处原笔误所在正文（一致性报告 §3 D-7 行、执行脚本 R1 `note`、JSON R1 `note`）均已为 `4.0904%` |
| 协议文件未回改 | `git status --short docs/endpoint-protocol-P011-terminal-nondeposited-v1.md` | 无改动 |
| 计数口径一致 | 双报告 §4.2 P4 行 vs 一致性报告 §4 | 均为 45 / 43 / 2→D-1，且注明来源 |

算术复核（本次勘误全部数值）：

```
322/(7550+322)      = 4.0904471544715444 %   (A2 legacyEndpointFraction)
322/(7550+322+128)  = 4.025 %                (A3 legacyMetricSuspensionFraction, = U/E)
(322+128)/8000      = 5.625 %                (新口径 M/E = M/R_WS)
5.625 - 4.025       = 1.6000 %  = P/E        (A3 配对，A = R_WS = E)
5.625 - 4.0904...   = 1.5346 %  ≠ P/R        (A2 配对，R = D+U = 7872;  P/R = 1.6260 %)
```

---

## 6.1 附带观察 O-1：重跑时 checker 已被并行会话更新（本卡范围外，仅登记）

本卡重跑 `test/p011_endpoint_protocol_v1_execution.py` 时，`test/p011_endpoint_check.py` 已被并行会话的 commit **`8e498c5`** 更新（`fix(metrics): complete endpoint identity metadata`）。因此本次重生成的 `test/reports/endpoint_protocol_v1_execution.json` 与双报告（commit `63fccf7` 时点）的数字出现以下**实现侧漂移**：

| 项 | 双报告 63fccf7 时点 | 本次重跑（含 8e498c5） | 说明 |
|---|---|---|---|
| `protocolSourceHash` | 未产出（缺陷 **D-3**） | **已产出**（SHA-256，指向协议文件路径） | D-3 现状已变化，需 manager/tester 复核后更新缺陷状态 |
| `endpointSampleStage`（报告行） | 未回填（缺陷 **D-4**） | 键已存在但值为 `null`（真实 run 无该字段） | D-4 部分推进，仍待真实 run 验证 |
| `context.externalAdmissionCount` | 恒缺（缺陷 **D-5**） | 仍缺（新进 `missingFields`） | D-5 现状未变 |
| 真实 run `missingFields` | 6 类 | **7 类** | 新增 `context.externalAdmissionCount` |
| 真实 run `reportMissingFields` | 37 项 | **38 项** | 同上 |

**未变的三项主结论**（本卡已复验）：

- legacy 口径与历史 SLIP checker 输出**逐位一致 7/7**；
- 真实 run 新口径：无 safety 报告时 `NOT_IMPLEMENTED`、接入 safety 后 `INVALID_LEDGER`（硬闸门 tan30，`violatingEdges=59413`/`violatingSubsteps=484`）；
- 报告总状态 `CANDIDATE_ONLY`、`endpointProtocolEffective=false`、`formalReleaseAuthorized=false`、`applicabilityReasons=[FORMAL_RELEASE_NOT_AUTHORIZED]`。

**处理方式**：本卡**不改写**双报告 §2.1 的「6 类 / 37 项」等数字（红线：不改历史报告结论；且这些数字在 `63fccf7` 时点为真实状态）。上述漂移登记为 O-1 反馈 manager，由后续卡决定是否对双报告出补充更新或另起证据版本；本卡的三项勘误（E-1/E-2/E-3）结论不受 O-1 影响。

---

## 7. 追加记录

- 2026-10-24（首发）：`protocolRevision=endpoint-terminal-nondeposited-v1` 发布（commit `69e27b9`），冻结分子/分母/阈值/末 1s 窗/固定终点/采样身份/时钟/状态分类/双算规则/不涉及项。
- 2026-10-24（字段改名）：manager 字段对齐裁定 §4 的 T1–T3 落地（commit `a3ca53c`），字段名与裁定 §1 A1–A3 逐字一致；冻结口径逐字未动已脚本验证。
- 2026-10-24（本勘误）：`endpoint-terminal-nondeposited-v1-errata1`。E-1 修正 §10 示例表第 1 行的两个 legacy 量分母口径标注（A2 `U/(D+U)=4.0904%` / A3 `U/(D+U+P)=U/E=4.025%`）与差额恒等式的分母配对陈述；E-2 更正三处 `4.0894%` → `4.0904%` 笔误（含一处缺陷编号指针 D-1→D-7）；E-3 统一条款计数口径（双报告 §4.2 P4 行 `28/30` → 一致性报告 §4 的 `45/43`，注明来源；P4「已满足」不变）。三项均为文本与标注勘误，**判定结论、判定式、阈值、窗口、终点定义、状态码、双算规则、估计对象全部不变**；已发布协议文件不回改。

## 8. 交接

- 本协议勘误由 tester 发布，交 **manager 验收**；生效与否仍受双报告 §4.2 的 P1–P9 前置清单约束，本勘误**不改变任何一项前置的状态**（P4 仍「已满足」，P2/P5/P6/P7/P8/P9 仍「未满足」）。
- 协议生效（`endpointProtocolEffective=true`）仍由 **manager 唯一有权宣布**；本勘误不构成生效、不构成放行。
- E-1 属协议侧文本，若 architect 在字段 revision 中对 A2/A3 的定义另出结论，须走对齐结论并回告 tester，由 tester 另起 revision 登记（不得回改本勘误文件与本卡的已冻结内容）。
