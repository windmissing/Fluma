# P-011 观测③ v3 预注册协议 v1

日期：2026-10-20。责任：designer。`protocolId=p011-observation3-v3`，`protocolRevision=preregistration-v1`。**状态：METHOD_FROZEN / EXECUTION_BINDING_PENDING / NO_CAPTURE_AUTHORIZATION。** 方法、估计对象和资格规则按本文冻结；A、修复构建及执行制品按下述确定性程序绑定，不以占位值冒充已可执行。观测③仍为 `BLOCKED / DESIGN_PENDING_OBSERVATION`。

授权依据：[priority.md](priority.md) 2026-10-20「观测③ v2 pilot 停机与 v3 识别方向批准」第3–7条。方法依据：[v3识别策略重论证](observation3-v3-identification-redesign-v1.md) §4–5、[algorithm-P011.md](algorithm-P011.md) §8.11原标定协议。本文取代v2对**新v3数据**的稳定平台限定，不回写 [v2预注册稿](observation-protocol-P011-v2-preregistration-v1.md)，不追认v2 pilot。

## 1. 冻结范围与候选估计对象清单

物理观测路线保留 `natural-relax-hysteresis-v1`，其实现版本/代码hash必须另列。`runRole`按既有calibration/holdout契约；`analysisRole=pilot/fit/holdout`与 `rateRole=primary/sensitivity`独立登记。pilot不得计入fit/holdout，1.0°/s数据不得填0.5°/s缺额。

| 对象 | 字段/定义 | 允许结论 |
|---|---|---|
| 配置候选 | `configuredThresholds={thetaStartDeg,thetaStopDeg}`，来自冻结candidate manifest | 待检验输入，不是拟合值；start `[25°,30°)`、stop `[15°,25°]`，stop<start |
| 实际执行参数 | `executedThresholds={thetaStartDeg,thetaStopDeg,slopeStart,slopeStop}`，由状态机实际读取值产生；附 `stateMachineVersion/sourceCommit/sourceFileSha256/buildSha256` | 配置与执行身份一致、真实双阈值分支生效；仅复制manifest到日志不满足 |
| 起动转折 | 同边inactive→active、决策残差跨start的区间，按方向/速率独立汇总 | 该候选在该输入/速率/实现下的可复核转折区间及复现性 |
| 停止转折 | 同边active→inactive、决策残差跨stop的区间，按方向/速率独立汇总 | 同上；零grant、hold、reset不产生stop样本 |
| 候选优选 | 独立行为/效果判据的外部结果与holdout确认，判据版本/摘要另绑manifest | 只有独立判据足额通过才能称候选被支持；本协议不另造优选目标 |
| 非估计对象 | 材料“天然起停角”、候选最优性、±45°全局收敛、真实全通道速度 | 不能由预设软件阈值的自洽转换推出，不以本协议通过替代这些结论 |

事件中点/CI输出字段命名为 `observedStartTransition/observedStopTransition`，与configured/executed明确分栏。无有效识别证据时输出null；不能用输入候选回填。独立效果判据未绑定时，只能检验执行语义，不发布“候选最优”或部署参数结论。

## 2. 数据身份与decision时点契约

### 2.1 每run、每attempt与每边必须具备的字段

| 层级 | 必填字段与语义 |
|---|---|
| run manifest | protocol/revision、candidateId/pairId、configuredThresholds、执行实现/hash、geometry/初态/seed/device/build、scheduleSha256、rateRole/rateDegPerSec、A、analysisRole、随机化槽位、独立效果判据摘要 |
| attempt | runId、attemptId、frameId、commandSequence、segmentId/legId、phaseKind、targetTilt、thetaRequested（真实本步proposal）、thetaAccepted、plannerStatus、requestRejected、plannerHold、limitingEdge/reason、commitStatus、rollbackReason |
| committed子步 | substepId、physicsTime/physicsDt、对应attemptId、effectiveTiltAccepted、完整安全审计及材料/预算账本；未提交attempt不得伪造substep |
| 每边decision | c=固定物理interfaceIndex、decisionOrdinal、decisionG、**有符号**decisionR、decisionAlpha、activeBefore/activeAfter、transitionReason、thresholdUsed、executedThresholds身份、requested/granted/budgetScale、guard/stock/capacity限制状态 |

`decisionG=(S[c+1]−S[c])/dx`，`decisionR=decisionG−tan(effectiveTiltAccepted)`，`decisionAlpha=atan(abs(decisionR))*180/pi`。取值点是**状态机实际读残差、选择分支之前**；若前面已有settle/percolate/bottom或先前边写入，必须反映当时真实材料状态。整步stateBefore或apply后残差不能替代decision值。decisionOrdinal标明同一步内顺序；同一边若实现存在多次判定须完整登记，不能只挑一次符合候选的值。

`thresholdUsed`为对象 `{branch:start|stop, angleDeg, slopeLimit, comparator}`：inactive时读取start分支，active时读取stop分支；需记录实际比较符和slope值。`transitionReason`至少区分 `START_THRESHOLD_CROSSED / STOP_THRESHOLD_CROSSED / HOLD_INACTIVE / HOLD_ACTIVE`，以及限制/生命周期导致的非标定事件。有限值、单位、公式和实际分支由tester核验；状态翻转必须来自运行状态机，不由probe按候选阈值重构。

configured/executed角度和代码身份不一致，或仍以 `requested>0` 代替双角状态，整run标 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`。不能因为两份日志都写了28°/22.911°就通过：architect须追踪从manifest配置到状态机读取及实际分支，tester验证真实active保持与start/stop不同门槛。角度到tan的核验沿用既有数值契约，不新增放宽物理阈值的epsilon。

### 2.2 事务完整性

preview状态/active不得泄漏到真实轨迹；只有已提交attempt才能成为状态样本。被拒绝的attempt仍需可审计记录，保持拒绝次数/原因可见，但不计算材料样本或bracket端点。跨BLOCKED/回滚的两条稀疏ledger不能当连续子步。

采集侧修复必须覆盖九相及v3多命令leg映射、planner字段、decision时点、真正双角执行身份，核验R-3预演/guard一致性和v2硬失败根因处置。仅修相名/schema字段不等于执行身份整改完成。本文冻结不等待这些代码交付；执行采集必须等待其完成并经architect/tester核验。

## 3. 激励序列、平台与ramp leg预注册

### 3.1 九个平台与八个独立leg

平台序列严格为 `0 → +15 → +A → +15 → 0 → −15 → −A → −15 → 0`。平台编号H0…H8，每个平台命令hold **5100ms**；不因accepted角迟到重启计时、不延长hold等事件。A严格小于30°，由§4确定，不拿30°或29.9°作自动默认。

| legId | 前后平台 | 输入角方向 | 振幅方向/预期主要激励 |
|---|---|---|---|
| L1 | H0(0)→H1(+15) | 增 | 远离零，上程；持态/可能start诊断 |
| L2 | H1(+15)→H2(+A) | 增 | 正向上程，start/+主要激励 |
| L3 | H2(+A)→H3(+15) | 减 | 正向下程，stop/+主要激励 |
| L4 | H3(+15)→H4(0) | 减 | 正向回零，stop/+补充；零点归neutral |
| L5 | H4(0)→H5(−15) | 减 | 远离零，上程；持态/可能start诊断 |
| L6 | H5(−15)→H6(−A) | 减 | 负向上程，start/−主要激励 |
| L7 | H6(−A)→H7(−15) | 增 | 负向下程，stop/−主要激励 |
| L8 | H7(−15)→H8(0) | 增 | 负向回零，stop/−补充；零点归neutral |

leg身份由采前schedule赋值，不根据是否看见转折重分段。输入角上/下行不保证decisionAlpha上/下行，事件资格另按§5核验。表中“预期主要激励”不是强行给事件贴start/stop标签；任一leg内真实事件均按同一规则分类。

### 3.2 确定性命令生成与边界归属

主档v=0.5°/s、敏感性档v=1.0°/s；命令步距时间固定100ms，角步距分别0.05°/0.10°。不改maxTiltOmega，v只是实验输入扫描速度。每leg从前一平台终点T0线性扫到T1，`n=abs(T1−T0)/(0.1*v)`，必须为整数；峰值网格在§4保证两速率均可精确到达。

H0在t=0发送0°命令；各hold结束时发leg开始标记命令（仍为T0），随后每100ms发送一个增量命令。第n步终点命令T1归入下一平台H，开始5100ms hold；leg采用半开时间区间 `[legStart, endpointCommandTime)`，平台区间 `[endpointCommandTime, nextLegStart)`。因此终点命令不同时属于leg和hold。平台/leg标记自身不能触发物理active重置。

commandSequence对全部命令严格递增。一个leg中多个commandSequence可共同构成观察段：bracket两端可为同一命令，或相邻命令，须同leg、无丢命令/重排/外部输入；相邻命令间的身份映射必须在manifest列出。不跨反转点、零点、平台边界，亦不把边界两侧端点拼接以补窄bracket。首个leg记录没有同leg前驱时不构成bracket。

平台驻留不要求采到start；平台资格保留角容差0.20°、abs(effectiveTiltRate)≤1e-9°/s、窗口角极差≤1e-6°、连续≥3个提交子步且跨度≥200ms，完成窗口后才开始计数。全部依据actual accepted角，非target；无planner hold/rejection/clamp。该容差仅为平台标签，不给tan30增添余量。hold内未满足稳定资格的过渡记录不借名归ramp。

理想命令总时长 `T=9*5.1 + 4*A/v` 秒，平台hold与ramp时长分列。实际接受时间/间隔照实记录并执行既有输入时序闸门，设备执行不能用多等数秒冒充5100ms hold。改变角步距、时序或边界规则需新revision。

## 4. 影子审计、最小覆盖峰值与BLOCKED降级

### 4.1 采前审计输入冻结

审计使用**修复后的实际执行代码**及同一planner/逐writer guard/预算逻辑，不使用去guard的简化模型。冻结candidate列表、初态/种子集合、几何、所有物理参数、device/build目标、子步dt轨迹集合、采样/输入时序和审计器hash。dt/初态集合由architect/tester绑定到正式采集配置，必须非空、在查看审计结果前登记；影子模拟不计真实run配额，不作为真机安全/效果通过证据。

峰值搜索网格冻结为 `A=15.1°,15.2°,…,29.9°`（共149点），正负使用同一A；这只是输入峰值候选，**不改变thetaStart/Stop搜索窗**。输出“最小”只指该离散网格内、所登记初态/dt集合上的最小覆盖峰值，不宣称连续域或所有真机状态的全局最小。

每candidate pair、主档速率独立审计，不能用一个pair的A代替另一个。对每个A从注册初态重新模拟完整九平台/八leg，逐笔记录proposal、guard前后grant、四层预算、active及decision值，核验preview/真实执行路径的影子重放一致性；不能将前一个A的材料场继续当下一个A的初态。

### 4.2 单A判定与选择

`SHADOW_SAFE`要求所有登记初态/dt情形：完整计划可执行，所有提交状态的现行安全项通过，grant及预算/库存/相守恒正确；所有平台实际满足§3平台窗口；无身份/遥测不全。planner BLOCKED、回滚或失败终点均不称完整安全可执行包络；出现clamped须单列，不能把clamped事件用于候选覆盖。

`SHADOW_COVERED`进一步要求每个登记情形在四主cell均存在至少一个§5合格的真实bracket，start/stop各自双侧跨实际候选且宽≤0.5°，风险状态/端点有证据。影子“1事件/cell”只是激励覆盖条件，绝不替代正式的30状态/10run/20事件等门槛。

对所有A输出安全性、覆盖矩阵、限制边及原因；选满足 `SHADOW_SAFE && SHADOW_COVERED` 的**最小A**作为主档 `A_selected`。四cell覆盖合格但原24子cell存在缺层风险，仍须报告§7矩阵，不凭A选出就批准扩批。无任何A合格时 `A_selected=null`、`EXECUTION_ENVELOPE_UNRESOLVED`，不能填29.9°默认数。

敏感性档首先在主档同A上独立审计，保持物理/几何/初态相同以隔离速率因素。失败不改主档A、不合并结果。若以后要求敏感性档独立确认且需要不同峰值，必须先另绑schedule/A审计，结果不能只归因为速率差；由manager另行决定，不自动扩展本次执行计划。

### 4.3 BLOCKED及部分包络

某A的轨迹首次BLOCKED时停止该条影子轨迹，保留此前合法暴露与失败attempt；不靠反复发更大target、延长hold、提高预算或放宽guard让它通过。其他预注册A可从原初态独立审计，但不能假设安全性对A单调或用二分跳过未审计点。

若较小A可安全完成但不能覆盖start，记录 `SAFE_BUT_NOT_IDENTIFYING`；若仅单方向/stop可覆盖，记录 `PARTIAL_COVERAGE`。可报告最大已证安全输入包络供manager选择后续诊断，不将其自动变成新采集峰值，更不将缺start降为半份标定通过。无法满足本协议时manager可调整激励、上报用户决定验收适用范围、或维持INCOMPLETE；本协议不自动降门槛。

影子边界原因如果不是tan30（如库存、预算、数据缺失、实现错误），不得赋 `RIGHT_CENSORED_AT_TAN30`。影子可见删失只标 `SHADOW_PREDICTED_CENSOR`，不得代替真实run的风险暴露证据。

## 5. 样本资格、分辨率与首转折算法

### 5.1 共用资格与状态样本

run整体须通过所有现行硬闸门：tan30/planning、逐writer中央及邻边guard、local/bottom/total及适用channel预算、H/L守恒、库存、容量、有限值、账本/decision对齐、固定物理边、真实stop语义。**无capture豁免，微小硬越界也不由本协议豁免。** 任何硬失败隔离整run，不能提取“好子步”补配额。

样本须来自实际提交子步，执行阈值身份正确，attempt与子步连续、字段完整。同一边用interfaceIndex，不用方向性donor当身份。主标定池排除planner clamped、blocked、hold、rejected、rollback；`targetTilt!=thetaAccepted`本身不等于planner clamp，须比较本步thetaRequested与实际接受决策。合法、完整提交的正常受控ramp允许rate非零。

四主cell按activeBefore与**actual accepted角符号**分类：inactive为start类，active为stop类；符号为+或−，零为neutral单列。状态hold与flip都可按原规则计状态样本；bracket另行计数，不能共用分母。预算受限状态的状态语义仍核验“不能误报stop”，但不能借这些记录补受限转折的合格事件。

### 5.2 bracket资格

```text
same run / executed candidate / geometry / fixed edge / rate tier
same preregistered ramp leg OR same eligible stable-platform segment
adjacent committed substeps AND no intervening noncommitted attempt
both endpoint decisions committed; previous activeAfter == current activeBefore
same nonzero accepted-tilt sign; no reset/source switch/inversion/phase boundary
no planner clamp/hold/reject/rollback at either endpoint
no budget/guard/stock/capacity clipping ambiguity at either endpoint
start: false->true; alphaPrev < thetaStartExecuted <= alphaNow
stop:  true->false; alphaPrev > thetaStopExecuted >= alphaNow
bracketWidth = abs(alphaNow-alphaPrev) <= 0.5 degrees
```

thresholdUsed比较符需与原批准状态机语义一致；并非按此分析式更改运行比较符。阈值相等端点按现行start/stop契约核验。requested=0且granted=0的真实stop可有budgetScale=null，不把它默认成“限流”；requested>0但因额度/guard grant=0绝不能据此认定stop。

ramp事件还要求**包含两端及紧前一个同leg decision的3点窗口**按事件方向单调（start非降，stop非升，允许相等），全部符合身份/提交/未受限条件；窗口仅由相邻记录决定，不向前搜索有利端点。该窗口不要求整条leg的alpha单调，也不把输入单调代替残差单调；不满足记 `NONMONOTONE_LOCAL_WINDOW`，不人为平滑alpha或删除中间记录造bracket。

采前选段固定为§3所有leg及满足平台稳定规则的窗口；不按看到的峰值/宽度再裁剪段。对每 `(runId, segmentId, fixedEdge)` 按时间顺序评估，保留**首个满足全部规则的真实转折**（start或stop中先出现者，不是两种各留一个）；后续转折留原始审计但不进入估计。不能改成选“最窄/最接近候选”的事件。跨命令仅允许§3的同leg相邻命令规则。

0.5°是单事件测量分辨率门槛，不是新增物理阈值。宽事件记 `BRACKET_TOO_WIDE`，不得把其切成多个窄区间；再多宽事件的中点bootstrap也不能补救此资格缺失。所有排除原因按先身份/事务、后segment/方向、再转折/宽度的固定顺序登记，另保留全部命中原因，避免统计分母随分析人改变。

## 6. 原统计门槛、速率隔离与随机化

| 项目 | 冻结门槛 |
|---|---|
| 主档0.5°/s | 每candidate pair至少34独立完整合格run：24fit+10holdout；每组每方向≥5run |
| 四主cell | start/+、start/−、stop/+、stop/−；fit和holdout各cell≥30状态样本/≥10run，每run每cell最多3 |
| 真实事件 | 每组每方向start和stop各≥20合格转换，分别来自≥5run |
| 24子cell | 四主cell×有符号decisionR正/负×near/shoulder/far；每组每子cell≥30状态样本/≥10run，每run最多3，不合并缺层 |
| bootstrap | 10000次分层run-cluster percentile，seed=20261019；95%分位2.5/97.5，线性插值；不重采单边/子步当独立run |
| CI/复现 | start/stop分别CI宽≤2°；fit/holdout估计差≤1°；各方向/type的holdout中点落fit95%CI外≤5%；各噪声子cell状态语义误判率≤5% |
| 两阈值关系 | lower95(start)>upper95(stop)，点间隔≥2*wNoise，start上95%界<30°；硬安全/身份/限流误报stop为零容忍 |
| 敏感性1.0°/s | 独立表/独立分析，不补主档缺额；若要求独立确认，同样34及全部门槛，两档最低68run/pair，不自动授权扩批 |

原±45°停止角映射窗口、全宽收敛目标继续阶段二延期；不作为本次隐含筛选，也不声称已通过。每个方向独立给出事件区间、run数和CI；缺方向不能用另一方向估计替代。

状态计数采用时间/substepId/固定边排序，各主/子cell分别取每run前3个合格状态；二者各自从原始合格状态计数，不能把子cell计数直接加作主cell独立样本。decisionR=0或accepted角0归neutral，不塞正负层。

噪声定义保持：独立no-transition稳定窗口的pooled MAD×1.4826得sigmaNoiseDeg，fit合格bracket宽中位数的一半得qResolutionDeg，`wNoise=max(1.96*sigmaNoiseDeg,qResolutionDeg)`。只用fit计算并冻结；near距离≤wNoise，shoulder在(wNoise,2wNoise]，far>2wNoise。距离针对状态对应的configured且已核对一致的候选。缺量、非有限、wNoise=0时insufficientEvidence；不得用ramp变化幅度作噪声、借holdout估噪声或填默认值。

估计沿用事件bracket中点→run内中位数→run等权中位数，按方向/type/速率分别分析。噪声/统计不足时发布覆盖诊断，候选估计字段null。两速率有差异时报告速率依赖，不能平均成“天然角”。敏感性档只有可行性审计数据时输出NOT_EVALUATED；不得把不足当作速率无影响。

随机化重新生成v3 manifest，分层键 `(candidateId/pair, schedule version/A, rate tier, device/build, planned ± coverage)`；seed=20261019，按不可变plannedRunId排序后固定实现的Fisher–Yates排列，前24fit、后10holdout，绑定生成器版本/hash及完整排列。每个正式层独立足额，按run整体分组，不拆leg或边，不按观测结果换组。pilot独立槽位永久排除；需补采时在manager批准后按原层/角色追加槽位，不挪holdout。

holdout在fit候选/噪声/分析制品冻结前封存；独立资格核验不得向拟合人员泄露残差分布。揭盲后不能改窗口、分层、排除规则或A，再声称同一holdout独立。v2 pilot以及任何旧run均不迁入v3。

## 7. 可达覆盖矩阵与结构性缺失

影子预演和经批准试采都必须输出同构矩阵，前者 `source=shadow`、后者 `source=pilot`，严格分表。每candidate/rate/A/初态或run分别列四主cell及24子cell，零记录也保留行，不只输出有事件的格子。

| 列 | 定义 |
|---|---|
| cellId / parentCell / residualSign / noiseBand | 固定状态×实际角符号×残差符号×噪声层；主cell子维度为空 |
| atRiskPresent / atRiskRecords / atRiskCommittedDuration / distinctRuns | start风险态为inactive，stop为active；只算真实合法提交时间，不算BLOCKED墙钟时间 |
| alphaMin/Max / acceptedTiltMin/Max | 对该cell合格风险状态计算；无值为null，不借另一状态/方向极值 |
| trueEventCount / eligibleBracketCount / endpoints | 原始真实状态转换与最终合格转换分列；逐事件保存alphaPrev/Now、区间端点、宽度、边/leg/attempt/substep身份 |
| stateSampleCount / cappedCount / runCount / required / deficit | 原始、每run封顶后计数与对应门槛分列，影子run不折算真实独立run |
| noiseDefinable / wNoiseSource / reason | 正式fit未形成前不可假装已知near/shoulder/far；pilot不估wNoise，子cell保留 `UNDEFINED_NO_FIT_NOISE` |
| exclusionCounts / plannerBound / limitingEdge | hold/clamp/不连续/宽事件/限流等原因及实际安全限制证据 |
| coverageStatus / structuralAssessment | OBSERVED、NOT_OBSERVED、UNDEFINED、PROVEN_UNREACHABLE_IN_REGISTERED_CLASS或UNKNOWN，附解析/穷举审计依据 |

影子/试采阶段仍可输出decisionAlpha到候选的原始距离和正负覆盖，从而说明未来噪声层的可定义性；不能为给24格染色而给pilot拟合wNoise。有限样本计数为0不自动证明结构性缺失。只有给出登记初态/输入/dt类的不可达证明或完整审计覆盖，才可写该类结构性缺失；不得外推为所有物理路线无解。

任一子cell结构性缺失或反复无法定义，交manager附矩阵与证据裁决：调整激励、上报用户决定验收适用范围、或维持INCOMPLETE。不会自动合并残差符号、正负方向、start/stop、两速率、fit/holdout，更不删除难采层。

## 8. 诚实输出与删失纪律

判定分层：run安全/身份→观测暴露→cell资格/数量→统计可辨识→独立候选行为。安全/身份失败先隔离；其他层可以附多个原因码，但不能用局部PASS覆盖整体失败。

| 情况 | 输出 |
|---|---|
| 硬闸门失败 | RUN_INVALID_HARD_GATE，整run隔离，不使用好段 |
| 配置/执行/状态机身份不一致 | EXECUTION_THRESHOLD_IDENTITY_MISMATCH + UNOBSERVABLE，估计null |
| decision/planner/连续性关键证据缺失或无真实风险状态 | UNOBSERVABLE，细分TELEMETRY_INCOMPLETE / NO_AT_RISK_EXPOSURE / EXECUTION_INCOMPLETE |
| 平台未达 | PLATFORM_UNREACHED作为诊断；v3并非必须靠平台产生start，但完整run/平台要求仍核验，缺失不能改名已稳定 |
| 无足够事件/cell/run或无法定义噪声 | INCOMPLETE / insufficientEvidence，估计null |
| 已有stop风险暴露但无合格stop | STOP_TRANSITION_UNOBSERVED，附下述细分 |
| 合法风险暴露被tan30安全边界截断且未穿过候选start | 仅满足下述全部条件时RIGHT_CENSORED_AT_TAN30；估计null，不算标定PASS |

v3右删失**不再以稳定平台到达为必要条件**，可以来自合格ramp风险暴露，但须同时满足：

1. 执行身份真实、风险态inactive、该段从开始到截止有完整合法已提交记录，实际alpha上界可审计且低于候选start，没有合格越阈start。
2. 紧接着的planner/guard约束证据明确是tan30边界阻止继续激励，附attempt、constraint/limitingEdge、requested/accepted、风险状态、实际alphaMax；若是遥测缺失、库存、时间耗尽、实现错误或任意实验峰值不够，不能用此码。
3. 被拒绝attempt本身不作观测端点，不能把其未提交alpha当达到值；仅用此前合法暴露的上界。该输出说明实验暴露受限，不证明材料真实阈值一定更高。
4. 此前leg身份/符号完整，不能从另一个cell、active态、非法子步或影子预测借最大alpha。run另有安全/身份失败时只保留删失诊断，不赋统计资格。

安全停止后的不完整run可保留上述“合法前缀受限”诊断，但仍不进入完整34run池；不是把部分轨迹追认为合格run。缺样本不等于右删失，单看accepted≈29.9°也不等于tan30导致该候选未暴露。

stop细分：`NEVER_ACTIVE`（同时UNOBSERVABLE）、`STOP_BEFORE_ELIGIBLE_SEGMENT`、`STILL_ACTIVE_AT_END`、`ONLY_NEUTRAL_ZERO_ENDPOINT`、`ONLY_PLANNER_HOLD_OR_CLAMP`、`ONLY_BUDGET_GUARD_OR_STOCK_LIMITED`、`BRACKET_TOO_WIDE`、`NONMONOTONE_LOCAL_WINDOW`、`CONTINUITY_MISSING`。真实stop存在但被排除时列原始事件与原因，不能报告“完全没有stop”。grant=0、回零命令、phase结束、reset、planner冻结不替代stop。

任一四主cell或24子cell未足额整体INCOMPLETE；本次未批准stop半份参数交付。pilot无论yield多少都 `thresholdVerdict=NOT_EVALUATED`，thetaStart/Stop估计与CI均null，不拟合wNoise，不解封holdout。

## 9. 执行绑定清单与版本纪律

| 冻结项 | 本文冻结内容 | 执行前绑定 |
|---|---|---|
| 方法/估计对象 | §1–8全部规则；搜索窗/速率/宽度/leg/输出/统计 | 本文UTF-8制品SHA-256及版本归档 |
| 候选 | configured/executed/decision分列、范围、有序性、候选优选边界 | 精确candidate列表manifest/hash；不得暗用旧28°/22.911°作为默认 |
| 实现 | 真双角状态机、逐decision读取、planner+guard事务一致 | source commit、相关源码hash、实现版本、HAP hash、architect/tester核验回执 |
| 峰值 | §4网格、审计和最小覆盖规则 | A_selected、初态/dt集合、影子审计器hash、全部A结果及覆盖矩阵摘要 |
| 执行schedule | §3确定性生成、5100ms hold、八leg身份 | 每pair/rate的完整CSV及SHA-256、生成器hash、runner字节hash与命令映射 |
| 随机化/封存 | §6的分层、seed、24/10、pilot排除 | 新v3计划槽位/实际runId绑定/排列hash；正式扩批前冻结 |
| 视觉/独立行为 | 不以阈值自洽自证最优，不混物理路线证据 | 已批准独立判据版本、评分/测量工具hash；未绑不出优选结论 |
| 授权 | developer整改、architect/tester核验后manager单独批pilot | 适用protocol/build/pair/rate的批准记录；无批准不采集 |

执行CSV规范固定UTF-8无BOM、LF、末行一个LF，列为 `commandSequence,offsetMs,segmentId,phaseKind,legId,targetTiltDeg,rateDegPerSec`；整数/有限十进制、禁止科学计数法/多余空格，hold行legId为空、rate=0，ramp行填对应leg和正速率幅值。全部命令行由§3生成，最后另记H8结束时刻以证明最终5100ms完成。A和代码未绑定前**不编造schedule hash**。

上述待绑定字段是按已冻结算法生成的执行实例，不是方法自由裁量；若影子审计后想换网格、速率、hold、资格、首转折或统计规则，须新protocol revision并重新登记，不能在本v1中偷偷修改。A变更、新候选/构建也须形成新执行实例/hash，不能与旧实例混池或继续复用其holdout。

## 10. 交接与执行停止线

designer已冻结方法与候选估计对象；architect据此核对双角状态机、decision时点、planner提交/回滚语义；developer采集侧修复可并行，但全部执行身份和字段必须在新pilot前就位；tester核验新资格、完整性、安全与覆盖矩阵输出。effector仅在manager另批pilot后执行，不因本文出现METHOD_FROZEN启动设备。

影子审计无可覆盖A、存在结构性缺cell、planner持续BLOCKED、执行身份不实或关键字段不全时，提交具名缺口给manager，不扩批、不逼迫闸门、不降低门槛。v2 pilot `p011-run-1792472946390-3`保持INVALID诊断证据，不参与v3任何fit/holdout或噪声估计。

本次完成标准为本文落盘；未执行影子审计或采集，未产生A数值、候选参数估计或标定PASS。协议冻结与采集就绪是不同状态，当前仅前者的方法层完成。
