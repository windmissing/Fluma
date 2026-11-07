# T146 FIELD_COLUMN 首错误归因报告（architect）

日期：2026-10-27。责任：architect。任务卡：`6adf6d30e4b06ee357480fa9`（滴答）。
状态：**CAUSE_ISOLATED / ENGINEERING_DEFECT_CONFIRMED / ROUTE_TO_DEVELOPER**。
输入唯一依据：T151 报告（`docs/effect-report-P051-T151-field-column-raw-v1.md`）+ 两臂载荷原文（`test/artifacts/t151-field-column-raw-20261027/`）。纯归因零代码：本卡未改 `entry/` 与 `test/` 任何生产文件（仅新增独立探针 `test/probe_t146_reassemble_payload.cjs`，只读载荷）。

## 0. 结论先行

**T151 报告的「八分量原值 bits 未进入 payload」披露与载荷原文不符——两份 8×128×8B 场快照原值完整到件，归因由此直接闭环，无需 V3 补采。**

- C1/C2 两臂**逐位一致**：`firstIllegalBytesHex` 中 `waterTop[48] = -2.7755575615628914e-17`（= −2⁻⁵⁵，IEEE754 bits `0xbc80000000000000` 的常规高字序；小端存储 `00000000000080bc`），`waterTop[49] = +0.44466802978515624`（C1）/ `0.4449276657104492`（C2）= 该列旧 `waterTop[48]` 精确值。两臂全 128 列×8 数组扫描：**唯一违例就是 col 48 waterTop 负值**（isFinite 且 <0 精确命中 beginStepHot :520-522 谓词），其余 1023 项全部合法。
- 非法化窗口：T151 定位的区间成立（BEGIN_COLLECT_AFTER 合法 → PREVIEW_FINAL_RESTORE_BEFORE 非法），且唯一被写坏的量是 `waterTop[48]`。`waterTop` 在预览 COLLECT 期间被写坏且恰与池列质量缓存的搬运一致（见 §2 因果链）。
- 根因类别：**工程缺陷（浮点累加顺序不对称），不是设计缺陷**。修复不动任何冻结阈值/预注册 estimand/谓词。修复路由 → developer（§5）。

## 1. 归因证据（载荷原文解码，探针可复跑）

`node test/probe_t146_reassemble_payload.cjs`（新增只读探针；参数=日志路径，默认 C1）重组 64 片×2048B 载荷并解码快照。关键观测（两臂一致）：

| 项 | C1 | C2 |
|---|---|---|
| lastLegal waterTop[48] | 0.44466802978515624 | 0.4449276657104492 |
| firstIllegal waterTop[48] | **−2.7755575615628914e-17 = −2⁻⁵⁵** | **−2⁻⁵⁵（逐位同值）** |
| firstIllegal waterTop[49] | 0.44466802978515624（=旧 col48 值） | 0.4449276657104492（=旧 col48 值） |
| lastLegal/firstIllegal 快照长度 | 各 16384 hex = 8192B = 8×128×8 | 同 |
| 全场唯一违例 | waterTop[48]<0 | waterTop[48]<0 |
| stage 序列（载荷尾部 4 条） | HOT_CHECKPOINT(合法) → BEGIN_COLLECT_AFTER(合法) → PREVIEW_FINAL_RESTORE_BEFORE(**col48 非法**) → PREVIEW_FINAL_RESTORE_AFTER(合法) | 同构 |
| firstFailure | FIELD_COLUMN / index 48 / attemptToken 1138 / substepId 380 | FIELD_COLUMN / index 48 / attemptToken 1147 / substepId 383 |

两臂独立 run 的 col 48 旧值不同（注入量差异），但**残差逐位同为 −2⁻⁵⁵**——这是确定性浮点效应的强指印（随机 NaN/越界写不会产生两臂同残差）。

T151 报告 §3 的「firstIllegalBytesHex 未进入 payload」为**误报**：`lastLegalBytesHex` 在重组 JSON 偏移 96628、`firstIllegalBytesHex` 在偏移 113035，各 16384 hex 字符完整闭合；131072B 上限真正截断的是 `failure` 详表（`"failure":` 键未出现在重组文本中）与 `stages` 数组尾部（第 5 条 PREVIEW_RESTORE_BEFORE 处截断）。**该误报不影响 T151 的失败链定位结论（区间仍成立），但 V3 建议须更正事实基础（§4）。**

## 2. 因果链（数值重构逐位命中）

### 2.1 谁写了 waterTop[48]

预览 COLLECT 子步（`previewChannelPlansAt` :5188-5273）内的执行序（`substep(dt)` :4589-4618 → saveMotionSnapshot→beginCollect :4492-4496 → 各通道 → 粒子段）。在 DRY 模式下，`waterTop` 的写点收敛为两类（全代码 grep 核对）：

| 写点 | 位置 | 写法 |
|---|---|---|
| W1 updateSandTop | WaterSandField.ets :7311-7316 | `waterTop[c]=sandTop[c]+suspH[c]+suspL[c]+pool.particleMassAt(c)`（DRY 时） |
| W2 M2Sink.applyInto 尾部 | P011ParticleM2Sink.ets :249-252 | `f.waterTop[c]=sand(work,c)+f.suspH[c]+f.suspL[c]+pool.particleMassAt(c)`（DRY 时，c 与 target 双写） |

两者都读 **池列质量缓存** `pool.particleMassAt(c)=colParticleMassH[c]+colParticleMassL[c]`（P011ParticleField.ets :369-372）。sand 数组（bh/bl/mh/ml/suspH/suspL）在 col 48 全 0（快照解码证实），因此 **waterTop[48] 的值就是 colParticleMass 缓存本身**。negative 值只能产生于缓存扣减。

### 2.2 缓存如何变成负

池列质量缓存的全部写点（P011ParticleField.ets :333/:349/:422/:616/:708；P011ParticleHot.ets :995/:1010/:1021/:1022）：

- credit（spawn 入池）：`colParticleMassH[c]+=pMass[slot]`，累加序 = 出生序（0.1 包 ×k + tail 尾包）
- debit（deposit/land 扣减）：`cache[c]-=amount`（P011ParticleHot.ets :1010）
- debit/credit（crossing 搬运）：`cache[c]-=amount; cache[target]+=amount`（P011ParticleHot.ets :1021，T123 索引辅助）

**数值重构（Node 逐位复现，两臂同残差的机制）**：

```
credit 序列（spawn 按出生序左结合）：fl(fl(fl(fl(0+0.1)+0.1)+0.1)+0.1)+tail = 0.44466802978515624  (C1 X 值)
debit 序列（apply 按同一批粒子序扣减）  ：X−0.1−0.1−0.1−0.1−tail = −2.7755575615628914e-17 = −2⁻⁵⁵  ← 观测非法值
```

credit 与 debit 用**相同加数集合、相同顺序**，但 `fl(a+b)` 与 `fl(a−b)` 的舍入方向不对称（加法向偶舍入可下抛进位、减法可下抛借位），半 ulp 残差落号为负。C2 的 X=0.4449276657104492 同样复现 −2⁻⁵⁵（`fl` 演算见探针输出）。**这不是 NaN/越界写、不是未初始化读、不是索引越界——是纯浮点顺序性残差，且量级 |−2⁻⁵⁵|≈2.8e-17 远小于一切物理阈值**（EPS=1e-9，fieldEps=1e-9，1e-7 容量容差）。

### 2.3 为什么出现在预览窗口内、且 commit 路径从未报错

- 预览 COLLECT（θ=42 的 `substep(dt)`）会真实运行粒子 apply（`particleBinding.apply` :5231）并真实写场——这是「预览=真实执行+回滚」架构的既有语义（R-3 影子管线先例）。
- 同一子步稍后第二次 beginStepHot（authoritative COLLECT 的 beginCollect）以 `waterTop[col]<0` **精确谓词**（:520-522，无容差）扫描到 −2⁻⁵⁵ → FIELD_COLUMN → blocked → code5。
- commit 路径 audit（:7215-7221）用的是 **`waterTop[c] < -EPS`**（EPS=1e-9）**容差谓词**——|−2⁻⁵⁵|≈2.8e-17 ≪ 1e-9，静默通过。**这就是 T119「s0-vs-现场矛盾」的完整解释：谓词不对称，不是污染不存在。**
- PREVIEW_FINAL_RESTORE_BEFORE 观测到的非法值只存在于预览执行窗口内；`restoreBackup` → `restoreMotionSnapshot`/`copyParticleStateDiagnostic(particleBackup,true)` → `restoreSnapshot` → `copySnapshot`（:533-540）**完整恢复 colParticleMassH/L**（:538），场与池同时回到子步初值 → 无持久污染成立（T151 结论 §2 保留，T119 修订结论保留）。

### 2.4 与 T103「粒子全 airborne」的一致性

col 48 的 sand 数组全 0 → 该列无床面 → 悬空粒子在该列只产生 spawn credit（手势注入/摇晃扬沙干批次入池）与 crossing/motion 搬运。T103 首次发现时粒子全 airborne 与本归因一致：**airborne 粒子的缓存记账（spawn credit + crossing debit/credit）就是负残差的来源；LAND/deposit 不是必要条件**（C1/C2 的 col 48 全程无 deposit 成功——如有 deposit 成功扣至 0，残差会被 surface 合法性掩盖成 0 附近值）。T151 报告将嫌疑收敛到「airborne 粒子在 theta=42 重力下的落点/写入计算」——**修正：不是落点计算写坏场，是池列质量缓存的浮点记账残差经 dry waterTop 重算被放大暴露**。

## 3. 分量与代码路径表（嫌疑 writer 判定终表）

| # | 嫌疑 writer（T151 报告） | 判定 | 证据 |
|---|---|---code-|---|
| 1 | 预览内 airborne 粒子 apply/LAND（deposit） | **排除为主因** | col48 sand 全 0（两臂快照），该列无 deposit 目标床面；残差量级 2.8e-17 排除 guard/budget 级写入 |
| 2 | dry 通道对 field 列的写 | **确认（暴露器）** | M2Sink :249-252 与 updateSandTop :7314 把缓存值赋给 waterTop；非负谓词命中 |
| 3 | 池列质量缓存记账（spawn credit / crossing debit-credit） | **确认（根因 writer）** | :995/:1021 credit-debit 舍入不对称；数值重构逐位命中 −2⁻⁵⁵；col49 恰得旧 col48 值 |
| 4 | 索引/排序/规划器写场 | 排除 | 无场写点；写点收敛于 W1/W2 |
| 5 | 观测器本身写场 | 排除 | P011FieldColumnEvidenceV2 只读（observe/scan 只扫描不写） |

**唯一 writer 结论的强度**：本归因达到「数值重构逐位命中 + 两臂同残差 + 全场唯一违例」证据强度，仍按红线要求标注： depos(crossing debit 池缓存) 与 W2（waterTop 重算暴露）**共同构成失败链**，deposit 路径 :1010 的 `cache[c]-=amount` 在其他列/其他 run 也产生同构残差（审计容差掩盖）——修复必须覆盖整类，不是只补 col 48。

## 4. V2 导出上限评估与 V3 建议（观测工具问题）

事实更正（§0/§1）：**原值 bits 已到件**，T151 报告的截断影响面需修正为：failure 详表（含 badMask/poolCanonicalBytes 等）与 stages 尾部未完整导出。原因：`exportEvidence`（P011FieldColumnEvidenceV2.ets :176-192）构造对象的字段序是 revision→status→…→`lastLegalBytesHex`→`firstIllegalBytesHex`→`stages`→`failure`，两块快照 hex（各 16384 字符，合计 32KB）排在 stages/failure 之前，131072B 上限先被 hex 吃满。为此：

- **V3 建议 A（主建议，导出字段重排序）**：`exportEvidence` 返回对象把 `failure` 详表与 `stages` 放到最前、两块快照 hex 放到最后。截断发生时损失的只是 hex 尾部（锁存字节仍在设备内存中可复采），而首错详表/窗口环必达。改动仅 `exportEvidence` 一个方法的字段排列，零物理影响、零锁存改动、零谓词改动、分片协议不变。
- **建议 B（备选，锁存选择性）**：把快照锁存范围从全 128 列收缩为「非法列 ±1」（3 列×8 数组×8B=192B/快照）。**不建议**：本归因正是靠全 128 列扫描确认「全场唯一违例」（§1），收缩锁存会失去该对照能力。
- **调大上限**（64→128 片）作为兜底：131072→262144B 仍可能溢出（本载荷 dropped=11692 条窗口条目，环 32 容量 + 首查 32 上限不变），治标不治本。

**推荐 A**。此项是观测工具改进，与物理失败链无关，且本卡归因未依赖它（快照已完整）。

## 5. 修复路由与验收判据（供 T95 后复测消费）

**路由：工程问题 → developer**（设计侧无缺陷：λ/η 准入数学、预算、guard 均正常工作，问题在浮点记账与谓词不对称）。

修复方案（最小工程范围，三选一由 developer 按代价评估，均不违反冻结约束）：

1. **方案 F1（推荐）：谓词对齐**。beginStepHotBody FIELD_COLUMN 谓词（:520-522）的 `f.waterTop[col] < 0` 改为与 audit 一致的 `< -EPS`（EPS=1e-9）。与 :7215 audit 的容差完全对齐，消除「预览窗口内比 commit 更严」的不对称。物理含义：|x|<1e-9 的 waterTop 残差非物理（比单个 massPackage 0.1 小 10⁸ 倍）。**风险**：干模式 waterTop 每子步重算 `sandTop+susp+poolMass`，若残差持续累积不归零，容差可能掩盖缓慢性漂移——但 deposit 完成/缓存回零（massAfter==0 路径）会使残差消失，且 audit 的守恒分母公式（D-C1 修复后）独立把关总量。验收：Node 单测构造 X=0.44466803 序列（credit 同序 debit）断言 beginStepHot 不再 FIELD_COLUMN。
2. **方案 F2：缓存舍入吸收**。P011ParticleField cache 写点在 debit 后加「≤EPS 视为 0」清理（`if(Math.abs(cache[c])<EPS)cache[c]=0;`）。治根但写点分散（:995/:1010/:1021/:1022/:708 等 5 处），每处都是热路径（T162 门禁关注区），且违反「粒子池零分配/最小改动」精神的风险低但需逐点验证。
3. **方案 F3：dyadic 记账**。用 P011ExactDyadic（:1-40，272×16bit 符号数值，无堆增长）做缓存精确记账。代价最高，收益是根除整类残差；本缺陷量级 2.8e-17 已被 audit 容差覆盖，**不建议为此启用**（防御过度）。

**验收判据（T95 复测口径，均不改冻结阈值）**：
- J1：Node 单测：credit(0.1×4+tail)+debit 同序 → beginStepHot 不返回 FIELD_COLUMN（C1/C2 数值用例直接固化）。
- J2：两臂复采（同 T151 流程）：stage 序列变为 4 条全 legal 或 code5 消失；`fieldColumnEvidenceV2.status=NO_FAILURE`。
- J3：回归全绿（Hot 11/M2 13/patch 12/sink 17/T149 13/T85 7/T123 155，与 T154 回归集一致）。
- J4：audit(true) 与 beginStepHot 谓词口径一致性检查（构造 −2⁻⁵⁵ 值同时过两个谓词）。
- J5（判据边界）：不要求 waterTop 严格非负——容差内残差合法存在（F1 语义）；F2 方案下要求 cache 残差 |x|<1e-9 者归零。

**缺陷登记**：D-T146-1（beginStepHot :520 谓词无容差，与 :7215 audit 不对称）；D-T146-2（池缓存 credit/debit 舍入不对称，量级 ≤2⁻⁵⁵ 级，audit 容差已覆盖）。修复优先级 P5（与 T95 解锁链同批）。T95 解锁条件：J1-J4 全绿（manager 门）。

## 6. 变更记录

- 2026-10-27 v1：初版。归因闭环（原值 bits 已到件的更正 + 数值重构逐位命中 −2⁻⁵⁵ + 因果链 W1/W2/缓存记账 + 修复路由 developer + V3 建议字段重排序）。新增只读探针 `test/probe_t146_reassemble_payload.cjs`。
