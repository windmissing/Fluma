# P-011 路线 C：TC-WS-08 摇晃峰值量迁移方案 v1

日期：2026-10-24。责任：designer。技术路线：`dual-field-particle-v1`。
状态：**DESIGN_FINAL / READY_FOR_MANAGER_TO_ROUTE_TO_USER / PROPOSED / NOT_APPROVED / NOT_IMPLEMENTED / NOT_VALIDATED**。
任务：[peakSusp 另案 6adc64ff…ccb43](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adc64ffe4b017ab33dccb43)。本卡仅交付方案，不使用真机、不采集、不更改代码/测试或任何已批准 revision。

## 1. 唯一推荐与语义

**推荐将路线 C 的 WS-08 峰值子项迁移为：固定摇晃段内、每个成功提交子步的未沉积沙比例 `(U+P)/E` 的最大值，独立命名 `peakNonDepositedFraction`。** `P` 包括飞行和接触待准入的全部活粒子；不是仅空气中的粒子。建议保留 `0.5` 及格线及原分档锚点，旧 `peakSusp` 保留原身份、双报；新量获用户另案批准并完成独立协议/实现/验证后才替代 C 的峰值子项。**本稿没有替代现行冻结量。**

这个量回答“摇晃期间，最多有多少已准入沙尚未合法进入床层”，不回答“摇动输入最强的那个瞬间有多少沙”，也不证明全部非沉积量正在流动、进入水中或由本轮摇晃新近扬起。选择这一语义，是因为同一份沙在 susp/真实粒子之间改变表示或降级时，不应仅因所在桶不同而改变库存判据。

0.5 仍代表“一半总沙为非沉积库存”的数值及格锚点；没有观测依据支持调低该值，也没有证据证明 C 在原3s场景下可达。保留数值不等于新旧测量等价，不等于已标定效果。粒子生命周期、是否可见扬起/自然、无永久悬浮仍各按既有证据链判断，不能用这个峰值单独签发整个 WS-08 PASS。

## 2. 现行用法、历史证据和结构性失真

### 2.1 出处与当前实现边界

| 依据 | 原意/已知事实 | 本案处理 |
|---|---|---|
| [goals.md](goals.md) G2/G2.6 | 剧烈摇晃按比例扬起、可信悬浮；停止后≤8s再沉降与堆积稳定。没有把输入峰值与库存峰值定义为同一时刻 | 本案只澄清摇晃峰值，不动停止后终点 |
| [test-cases.md](test-cases.md) TC-WS-08 | default、正立tilt=0、已形成沙堆；静置3s→shake=1持续3s→停止静置8s；**摇晃段** `peakSusp≥0.5`，残余≤5% | 原场景时序/输入不变，峰值测量另案迁移 |
| 同文§5.2/§5.3 | peakSusp为TIERED，10/8/6/4/2/0分对应≥0.90/0.75/0.50/0.35/0.20/<0.20；并非全部峰值数值都是MUST-PASS | 新量建议沿用数值分档，及格仍≥6；安全不变量保持硬门槛 |
| `WaterSandField.getMetrics()` | 历史 `suspensionFraction=U/max(D+U,EPS)`；没有P桶。C totals含P后，原公式的live值成为U/max(D+U+P,EPS) | 必须区分历史分母语义复算与live原字段，不把二者冒同一历史值 |
| `test/water_sand_check.py::evaluate_shake()` | 只检查存在shake事件，随后 `max(values)` 取**全序列**、残余取最后一条；没有落实摇晃3s分段，也未完整实现TIERED评分 | 这是既有脚本与书面用例的适配缺口，不是已获批的另一估计对象。本卡不修脚本；迁移时tester单列分段/评分适配，不把全序列峰值当正式参照 |
| `test/record_tc_ws.py` WS-08计划 | rest3s/shake1.0×3s/rest8s；执行钩子仍有TODO | 计划不等于已完成正确时序采集 |

### 2.2 历史报告用于什么结论

可核查的直接历史出处为 [test-report-P003-P005.md](test-report-P003-P005.md) 的2026-10-13 RED基线：TC-WS-08 因缺shake事件与suspensionFraction记 `BLOCKED / NOT_IMPLEMENTED`，产物 `test/artifacts/P003-P005-baseline-20261013/ws_check.json`。它用于判定“证据不可测”，**没有实测峰值通过结论**。该报告同时登记分段统计未闭环；不能事后把它当作已完成摇晃峰值采集。

[effect-report-P003-P005.md](effect-report-P003-P005.md) 的EC-WS-06同样为未实现基线，没有主观评分，更没有用0.5证明效果通过。当前docs测试/效果报告的`peakSusp`/`TC-WS-08`检索未找到另一份可核查的数值峰值正式结论；不推断其他报告曾使用本指标通过。P-011 SLIP报告中的“悬沙≤5%”属于终点，不能冒充本案摇晃峰值证据。未来若发现另批报告，按其原protocol/build/采样保留，不追认。

### 2.3 路线 C 的失真机制及量级

工程稿 [双场架构](architecture-P011-dual-field-particle-v1.md) §2.2 把合法LIFT的汇由susp改写为真实粒子，IH/IL不增；同量沙从床进入P时，旧分子U可不变甚至下降。池满时DEGRADE又把P转U，旧值可能上升，尽管未沉积总量没变。只看旧量会把表示方式、合并/降级策略当成扬起效果差异，**可能**结构性低估；并非每个C run都会失败。

ASSUMPTION，均非测量：§3.3的300 unit/s共享额度若全给LIFT、候选平均在途1.5s，则存量量级约450 units；CROSS/LAND还要竞争额度，450不是保证可达峰值。候选Nmax=512、未合并等包m=0.05/0.1/0.25，对应名义质量25.6/51.2/128 units；当前T6允许质量加权合并，**Nmax×nominal m不是合并后的质量硬上限**，这点工程稿当前§3.3也已澄清。

同总量E=8000的假设下，若450全在P，旧live漏掉5.625个百分点；若只有128/51.2/25.6在P，则漏掉1.6/0.64/0.32个百分点。余量在U时新量不变；没有实际账本，不把这些候选数当实际池上限或TC-WS-08正式初值。旧量相对**历史分母**的差值另见§4，不能一概写成P/E。

## 3. 候选定义、采样与取舍

统一记同提交床层D、列悬沙U、活粒子P、累计期望E，摇晃固定窗口W见§5。

| 候选 | 定义与采样 | 可比性、历史影响及误判风险 | 决定 |
|---|---|---|---|
| A 保留旧量，双报不替代 | 旧字段实际采样的摇晃段max；另列逐子步历史分母复算 `max_W U/(D+U)` 与C live `max_W U/(D+U+P)` | 原报告不变；真实P不进分子，可能“有大量未沉积真沙但旧量低”；池策略影响数值。历史采样250ms与逐子步复算不可称逐位等价 | **仅作迁移期策略/legacy披露**，不推荐为C长期主量 |
| B 未沉积库存峰值 | **`max_W [(U+P)/E]`**，对同提交先相加再取max；含state=2 | P=0且守恒、同样本时与旧量一致；C中语义变更须新revision。继承初始悬沙、接触待准入可贡献峰值，不能单凭它证明本轮新扬起或自然 | **唯一推荐主量**，必须同报基线/状态/响应诊断 |
| C 输入最大时刻的量 | 先找W内实际被物理消费的外部shake输入最大值，再固定取其中**最早成功提交**k*，报 `(U+P)(k*)/E`；完整披露tie规则 | 本用例shake=1是3s平台，输入最大并非唯一时刻；首个高输入提交时shakeEnergy/库存仍在响应。可在随后库存已达标时给低值；若在同输入最大ties中挑库存最高点，就悄悄退化为B | 不作为主量；如需“输入峰时刻”只作独立诊断，不设0.5放行 |
| D 仅移动库存峰值 | `max_W [(U+P_state1)/E]`，排除state2 | 可暴露接触阻塞，但LAND将state1变state2而未入床就会降低量；降级进U又改变分类。U自身也不保证每份在运动，非严格运动质量 | 状态分项诊断，不推荐替代未沉积库存主量 |
| E 净增量/累计LIFT | `max_W [(M-M0)/E]` 或 `sum_W LIFT.granted/E` | 净增量可反映初始残余混杂，累计量会重复计同份沙的再扬起，能>1；两者都不是旧“同时悬浮比例”，不能继承0.5/原分档的意义 | 只作因果/预算诊断，不替代主量 |

对历史结论，所有迁移候选都不重评分、不推翻原PASS/FAIL/BLOCKED；C的数值不是legacy route物理效果的反事实。A的旧值继续正式适用直至另案获批，B当前只允许 `CANDIDATE_DIAGNOSTIC`。C/D/E不另立冻结门槛。

## 4. 推荐口径的精确定义与差异证明

对成功提交子步k，全部为模型material-volume units，不按粒子数、像素或速度估量质量：

```text
D_H = sum(stateAfter.bh + stateAfter.mh)
D_L = sum(stateAfter.bl + stateAfter.ml)
U_H = sum(stateAfter.suspH); U_L = sum(stateAfter.suspL)
P_H = particles.massH;       P_L = particles.massL
D=D_H+D_L; U=U_H+U_L; P=P_H+P_L; M=U+P; A=D+U+P
E_H=VH0+context.sourceInjectedH; E_L=VL0+context.sourceInjectedL; E=E_H+E_L
f(k)=M(k)/E(k)
peakNonDepositedFraction=max_{k in W} f(k)
peakCommitKey=earliest_commit_in_W_attaining_this_max
peakStockMass=M(peakCommitKey); peakReferenceMass=E(peakCommitKey)
```

WS-08为原无外源场景，整run不得外源准入；故E=VH0+VL0恒定、sourceEpoch不变。公式保留累计期望定义以便账本对齐，**不因此授权新的有源WS-08变体**。VH0/VL0沿DUALFIELD初值身份；E≤0为 `INVALID_DENOMINATOR`，初值/注入前身份不可证为 `INVALID_BASELINE`。

P精确为`pState∈{1,2}`按相求和，水内/空气/接触待准入均计；state0、视觉tracer不计。LAND只把granted从P移D，余量继续计P；DEGRADE/显式TO_SUSP只在提交后计U，不再计P，不累加degraded事件库存。MERGE只计合并后的存量一次。摘要与逐粒/事件账本必须一致。三显示名复用已批准定义 `suspFraction=U/E`、`airborneFraction=P/E`、`depositedFraction=D/E`，不重命名已有字段，也不把endpoint字段换成peak。

**两个legacy值分列**，遵循[manager字段对齐裁定](decision-P011-endpoint-field-alignment-v1.md)/当前[字段稿§5](architecture-P011-endpoint-fields-revision-v1.md)：

- 历史分母语义复算 `h(k)=U/(D+U)`，用于说明旧列模型的质量口径；C全部入粒子时D+U=0，不存在合法历史分母，记 `legacyHistoricalReferenceStatus=INVALID_DENOMINATOR`，**不能阻断数据完整且E>0的新量计算，也不能冒称legacy有可比值**。
- C中原字段路径实测 `l(k)=U/max(A,EPS)`；缺真实WS_METRIC样本，只能叫 `LEDGER_RECOMPUTED_LIVE_FORMULA`，不能写“实测”。旧代码对极小分母的EPS钳位如实保留原样，报告clamp，不以其冒充h的有效分母。
- 仅在守恒A=E且D+U>0，同提交有 `f-l=P/E`、`f-h=P*D/[E*(D+U)]≥0`。所以与历史分母的差值**不是无条件P/E**；D=0或P=0时可能相等。容差内守恒时另报实际差值/守恒残差，不断言机器数严格相等。
- 同一W、同样本且有效分母时max f≥max h，但峰值可发生在不同提交。`max(U/E)+max(P/E)`通常**不等于**`max((U+P)/E)`；分项必须在总量峰值提交同时披露。两路线动力学、采样窗或密度不同不能套用该不等式宣称改善。

## 5. 采样、分档、输出及判定伪代码

### 5.1 摇晃窗口与采样身份

原场景不变：default、TEST、tilt=0、已形成沙堆，静置3s，shake=1持续3s，shake-off后静置8s。令t_on为物理输入链实际接受shake-on的单调elapsed，**新峰值固定测量窗W为 `[t_on,t_on+3000ms)` 中实际消费该shake-on输入的全部成功提交子步**；窗口右边界的shake-off后提交不纳入峰值，不把停止后shakeEnergy衰减产生的更晚峰值移进来。

以输入事件/提交全序判定同一毫秒边界归属，必须记录on/off实际生效边界及本子步消费的输入身份；只看context内预更新shakeEnergy不能替代输入接受证据。若原调度off稍迟，超过固定3000ms的样本仍不计入本峰值；若提前off或窗口内输入不是原1.0，记场景资格不符。输入时序漂移按既有schedule/后续tester预注册的适用门禁单列审查，**本稿不新增允许延长摇晃的容差或更改endpoint的实际shake-off+8s边界**。场景合规性未经确认时不得据候选峰值签整体PASS。

连接键用 `(runId,sequence,frameId,substepId)`，同时绑定case occurrence及 `context.endpointPhaseOccurrenceId`（引用现成身份，不更名）；采样复用字段稿 `context.endpointSampleElapsedMs` 的成功提交后单调elapsed。记录 `integratedPhysicsSeconds`，不得用它代替3s实际时间；context/metric `physicsTime`可处于不同取时阶段，不要求时间戳逐值相等。

必须有整个摇晃窗每成功提交的原始v7状态/粒子摘要、on/off及采集边界证据，断档/提前退出记INCOMPLETE。峰值所需字段缺失记NOT_IMPLEMENTED，不能用250ms抽样估出“精确子步峰值”。合法休眠段可带身份只读复核保持值，不能把冻结/回滚/漏记当休眠，不能伪造成功子步。单独250ms旧数据可以保留 `OBSERVED_SAMPLED_MAX`（对同轨迹真max的下界），即便已采样值≥0.5，也不据此宣称新协议采集完整。

**不新增峰值末1s持续窗、不取均值/平滑值、不等待达标延长输入。** 峰值本就是最大值；单个真实且守恒有效的提交可为峰值。异常尖峰先查守恒/有限值/身份/事务，不能仅因短暂而删除它。采集停止后8s的终点判定完全委托已批准endpoint协议。

### 5.2 推荐的评分与结果分工

| peakNonDepositedFraction | 建议分数 |
|---|---|
| ≥0.90 | 10 |
| ≥0.75且<0.90 | 8 |
| ≥0.50且<0.75 | 6（及格） |
| ≥0.35且<0.50 | 4 |
| ≥0.20且<0.35 | 2 |
| <0.20 | 0 |

原数值锚点迁移也是本次待用户批准的内容；不写成终点revision已冻结了新峰值评分。用未四舍五入的M/E评分，0.5及格比较为 `M≥0.5E`，**不新增向下放宽的判定容差**；库存/分相守恒仍执行各自原有容差。字段/资格/覆盖无效不打0分凑平均，输出对应无效状态。安全硬闸门失败时只可留原始候选峰值诊断，不签正式峰值PASS。

新分相诊断 `peakNonDepositedHeavyFraction=max_W[(U_H+P_H)/E_H]`、L同理；某相E_phase=0记N/A，不能EPS造比例。保留旧 `peakSuspHeavy/Light` 原初值分母/旧列悬沙语义，不互为别名。总量峰值提交同时记录H/L分项（独立分相峰值不能相加）。不新增H/L通过阈值；总量达标而heavy几乎未扬起时，仍按现用例要求登记“摇晃交融构成未达成”。

### 5.3 最小报告契约（拟议字段，另案未实施）

| 组 | 字段/内容 |
|---|---|
| 独立方法身份 | 建议`peakProtocolId=p011-ws08-peak-nondeposited`、`peakProtocolRevision=ws08-peak-nondeposited-v1`、`peakMetricRevision=shake-peak-nondeposited-v1`、`peakAuthority=PROPOSED`、`peakProtocolEffective=false`；route/build/params/input/hash/clock身份 |
| 窗口/证据 | `peakWindowStartElapsedMs,peakWindowEndElapsedMs,shakeOnEffectiveElapsedMs,shakeOffEffectiveElapsedMs,peakWindowSampleCount,peakWindowCoverageStatus,missingFields`；输入消费/阶段occurrence、提交序列及积分时长 |
| 主结果 | `peakNonDepositedFraction,peakStockMass,peakReferenceMass,peakCommitKey,peakSampleElapsedMs,peakScore,peakStatus`；峰值提交的D/U/P/E、suspFraction/airborneFraction/depositedFraction |
| 双算 | `legacyPeakSuspReported`（旧原字段、原样本/原窗口）、`legacyPeakHistoricalReference`（同W逐子步h）、`legacyPeakLiveFormula`（同W逐子步l或原字段实测），各自分母/密度/证据类型；`sameReferenceSuspOnlyPeak=max_W(U/E)`独立命名 |
| 初态/响应/滞留 | shake-on前最近可证post-commit的`baselineNonDepositedFraction=M0/E`与提交键、`peakNetIncreaseFraction=max_W((M-M0)/E)`、本轮`cumulativeLiftGrantedMass`及guard/budget拒绝、峰值FLYING/CONTACT_WAIT分项；是否具备逐粒滞留时长证据 |
| 结果分离 | 旧峰值原结果、新候选峰值项、`hardGateStatus`、已批准endpoint协议的独立结果引用、自然度/无永久悬浮证据状态；不得把三者合成“峰值已证明全链” |

byState不存在时，只要canonical P及原安全账本完整，库存主量可算，但状态归因记NOT_OBSERVABLE；不虚构“长期不落已排除”。基线/响应证据缺失时不能宣称本轮新扬起效果，按既有测试/效果链保留未验证项。本案**不新增基线≤5%、最小LIFT量或粒子最大寿命**等通过线；若产品需要这些因果/动力学门槛，另行明确测试目标，不把终点5%自动套到shake-on前。

```text
read independent peak protocol identity; current authority = PROPOSED
validate run/build/route, no external admission, baseline E>0
bind fixed on+3000ms window and actual consumed input identity
validate v7 commit sequence, same-commit masses, coverage and existing hard gates
if missing required fields: NOT_IMPLEMENTED(missing_fields)
if bad mass/commit/input ledger: INVALID_LEDGER
if bad initial expected identity: INVALID_BASELINE
if E<=0: INVALID_DENOMINATOR
if incomplete window/input timing qualification: INCOMPLETE
for each eligible committed k in W:
    f=(U[k]+P[k])/E[k]  # sum on this commit, then max
    if first or f>peak: save f and commit key  # equal ties retain earliest
    accumulate independent legacy maxima and phase/response diagnostics
score = 0; then select highest applicable anchor from {0.90:10,0.75:8,0.50:6,0.35:4,0.20:2}
emit new peak item (score>=6), legacy items, hard gates and diagnostics separately
# before independent approval/effectiveness: only CANDIDATE_DIAGNOSTIC
# residual/window result is consumed by reference; never recalculated by this peak protocol
```

成本：若读取既有v7数组/摘要，O(128+Nmax)的质量核对可复用原守恒扫描；在线统计max及其key为O(1)/提交、O(1)附加累计内存。独立checker流式读取；若需逐粒滞留轨迹则另需O(Nmax)，本案不强加该新增数据。无需改物理事务或预算，无真机成本结论。

## 6. 反例与可达性约束

以下数值全为假设合成反例，不是正式测试或真机数据。

| 场景 | 数值与结论 | 防止的误读 |
|---|---|---|
| 表示漏计 | E=1000、D=480、U=320、P=200；历史h=40%，live=32%，新f=52% | 按“总未沉积”目标旧量会假FAIL；按旧“列悬沙”语义它仍是正确的旧结果，不能倒改报告 |
| 降级奖励旧量 | 上例把200全DEGRADE：D=480、U=520、P=0；旧变52%，新仍52% | 旧结论改变不等于扬起改善；新量对转列/合并不敏感 |
| 跨时刻加峰值 | E=1000，k1:U=400/P=0/D=600；k2:U=0/P=400/D=600 | 分别取峰再相加得80%假PASS，正确总量峰40%不及格 |
| 输入平台响应滞后 | 首个shake=1提交f=10%，平台2.4s时f=55% | 最早输入峰时刻法为10%，B为55%；二者不同估计对象，不能互相追认 |
| 全序列污染 | 初始静置段f=80%，摇晃段max=40% | 全序列max可假PASS；正式峰只取固定W。若M0本身仍≥50%，B可数值达标，但并未证明新增扬起，必须报告M0/净增量/LIFT证据 |
| 接触库存伪装运动 | E=1000、D=480、U=20、P_state2=500 | B库存52%正确，但若解释成“52%真实飞行/自然悬浮”是假PASS；分报state2、生命周期、主观效果，不能只凭峰值放行 |
| 250ms漏峰 | 真子步峰52%，所有250ms样本最多48% | 稀疏旧采样可在新目标下假FAIL；新量必须保留全部成功提交，不插值补峰 |
| 停止后才高 | shake段max48%，off后因残余能量到55% | 摇晃段峰仍不及格，不能扩大窗口到rest求PASS；endpoint照已批准协议独立算 |

**迁移不保证0.5可达。** 对无外源、无invert/reset的本场景，若全部床↔非沉积变化只有已记账LIFT与SETTLE/LAND，则精确库存结算：

```text
M(k) = M0 + cumulativeAcceptedLift(k) - cumulativeSettledOrLanded(k)
peak M <= M0 + cumulativeAcceptedLift(W)
cumulativeAcceptedLift(W) <= 300 * tau_W
必要条件：M0 + 300*tau_W >= 0.5*E
tau_W = W内成功子步的sum(physicsDt)，单位秒，不是wall elapsed
```

存在其他合法床↔非沉积通道时，必须在恒等式中逐笔补入，不能漏记后套这个界；MERGE/DEGRADE/CROSS只改表示或位置，不直接改变M。假设M0=0、tau_W=3s，最乐观峰≤900 units，要到50%必须E≤1800；若假设E=8000，则仅此条件就要求lift积分时间≥13.33s，原3s假设下不能达标。真实settle/LAND及其他通道分走预算只会收紧这个条件。上述不是对实际default build的无解证明（本卡没有实际E、M0、tau轨迹），但足以否定“改成U+P就自然能过0.5”的说法。

如后续实际账本证明目标不可达，应按原流程向manager报告目标/场景/模型冲突，由用户另裁；本稿不暗增300/s、不延长摇晃、不调低0.5、不减总沙量求绿。量级450/E也不能直接当3s摇晃正式峰值，更不能拿~450作为降阈值依据。

## 7. 已批准终点 revision 的严格边界

以下全部引用既有批准记录/协议，不由本稿重新裁定：

| 已批准内容 | 本案关系 |
|---|---|
| [终点批准记录](decision-P011-terminal-nondeposited-fraction-v1.md)与[endpoint协议](endpoint-protocol-P011-terminal-nondeposited-v1.md)：残余M=U+P含state2、WS用E/SLIP用R_SLIP、5%与epsMass=1e-9、固定末1s每提交达标 | **不改一个字**；峰值没有5%要求，也不借用其末1s最大值作为摇晃峰值 |
| WS-08实际shake-off后8s终点，SLIP原耦合与阶段适用性 | 不改；本峰值窗是shake-on后3s，两个时段及阈值分别判断 |
| endpointMetricRevision/Protocol/Authority及字段对齐裁定 | 引用现成post-commit/clock/key定义；新峰值使用独立peak*方法身份，不升级或覆盖endpoint身份 |
| endpoint state=2滞留诊断、无永久悬浮、各硬闸门 | 继续执行；本案只在峰值另报state分项，不创设寿命标准 |
| DUALFIELD CONS/BUD/LIFE/SRC/GES、30°±2°/target-based t90/R²、E2/E4 | estimand/目标不变；本峰值不代替手势/落床/坡度/渲染验收，不解除真机队列前置 |

用户已批准**终点**不等于批准本案峰值。终点METHOD_FROZEN也不意味着peak*生效；C run的旧/新终点双报和单口径不放行纪律仍按原协议，不能用本案覆盖其权限。

## 8. 独立 revision、迁移与不追溯步骤

1. manager验收本设计完整性，转用户**另案裁决**候选B、全部活粒子分子、E分母、固定3s/每提交采样、原0.5及分档锚点的迁移。明确这是“库存峰值”语义而非“输入峰时刻”或纯动态飞沙量。
2. 获批后tester发布**独立peak protocol**，登记§5拟议身份及必要字段/时序资格/分档规则、旧checker全序列问题的适配清单，另行追加测试资产。不修改已批准endpoint协议，不把本设计写成tester已经冻结。
3. architect登记独立peak字段revision，复用既有commit/elapsed/particles契约，补齐shake-on/off有效边界与子步实际消费输入身份；developer按获批契约实现采样/流式峰值/checker。现行WS_METRIC原名原式继续输出。
4. tester先用最新协议输出诚实RED基线（字段未实现即NOT_IMPLEMENTED），之后同C路线、同build/输入/参数跑同一原始账本的legacy与新量。另报**共同250ms样本对照**与**全部子步新峰值**，将分子/分母差异与采样密度差异分开；不能用旧250ms峰与新全子步峰之差全归因于物理改善。
5. 真机需求由manager交effector串行排队，原3s静置/3s摇晃/8s静置，配对录像只验证扬起/自然与永久残余，不以画面代替守恒账本。本卡不安装HAP、不占设备。
6. 路线C专属客观/主观报告齐备后designer验证（同路线前版为基线），manager确认适用范围/独立peakProtocolEffective。批准前不替代旧量；获批但未实现/未验证也不能直接放行。旧结果带`legacy-susp-only`及原build/protocol/采样身份，原分数与结论保留。
7. 旧C账本若有真实P/同提交/完整W及必要时钟字段，可离线补算为`RETROSPECTIVE_DIAGNOSTIC`，不追认旧验收。旧OFF/B明确无P时数学对照P=0，但不向旧文件补造v7字段；不同路线只作选型参考，不要求全量dominance。

本设计交付条件已满足；方案效果尚未验证。设计阶段复盘见 [retro-P011-ws08-peak-nondeposited-v1.md](retro-P011-ws08-peak-nondeposited-v1.md)。

## 9. 变更记录与交接摘要

- 2026-10-24：首次定稿。依据用户另案任务，核对当前终点批准、tester协议及manager字段裁定、WS-08分档/书面时序、历史RED报告和checker源码；明确推荐库存峰值、原0.5/分档迁移、独立方法身份、不追溯及预算必要界。新增仅本方案与designer复盘，不改goals/test-cases/既有revision/代码/测试。

**交manager建议：采纳B，转用户另案裁决。** 这修复第三桶漏计与表示策略偏差；它不会自动解决预算下的扬起量不足、初始残余混杂或接触滞留，自然度/生命周期/已批准终点继续分别验收。
