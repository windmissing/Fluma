# P-011 观测③ v2：九相平台双角标定预注册稿 v1

日期：2026-10-20。责任：designer。状态：`PREREGISTRATION_DRAFT / NOT_FROZEN`；观测③仍为 `BLOCKED / DESIGN_PENDING_OBSERVATION`。本稿交 effector 固化采集制品、tester 对齐 checker/cases，不执行采集，不改物理代码、测试用例或任何闸门。

依据：[priority.md](priority.md) 2026-10-20「观测③平台可达性矛盾裁决」方案 3、[algorithm-P011.md](algorithm-P011.md) §8.16（已由 manager 接受的识别边界）、§8.11「标定协议」、§8.15 阶段边界；衔接 [v2 补采执行单](effector-collection-plan-P011-observation3-v2.md) 与 [test-cases.md](test-cases.md) §11.7。本文是新协议，旧七相 run、随机化 manifest、平台资格不能迁入；历史记录不改写。

## 1. 研究问题与不变量

在实际接受的 ±15°/±30° 稳定平台内，是否能观测到同一物理边的真实 start/stop，并在原统计门槛下识别双阈值？平台可达不等于阈值可识别，单 run clean pilot 不输出标定结论。

- 物理路线仍为 `natural-relax-hysteresis-v1`，`runRole=calibration`；新增协议身份 `protocolId=p011-observation3-v2`、`protocolRevision=preregistration-v1`。`analysisRole=pilot/fit/holdout` 单独登记，不替代物理路线或 runRole。
- 搜索窗不变：`thetaStart∈[25°,30°)`、`thetaStop∈[15°,25°]`，并满足 `thetaStop<thetaStart`。候选输入与拟合输出分列；不把候选值、29.9° ceiling 或默认 30°/20°当作标定结果。
- 所有提交子步、所有边执行现行 tan30、预算、守恒、库存、容量、有限值、遥测对齐、物理边身份、stop 语义检查，零 capture 豁免。安全 gate 的阈值、数值契约及失败处理沿用已批准版本。
- ±45° 的 30°±2°、t90、终点等目标包仍 `DEFERRED / NOT VALIDATED`；停止角对 `[20.563°,25.093°]` 的目标映射不作为本轮阶段一标定门槛。阶段一 B 当前条件性通过不等于逐 writer guard 全部交付。
- manager 裁决只允许前置完成后的单 run clean pilot；本稿不新增执行授权。effector 须绑定 manager pilot 批准记录及适用构建，fit/holdout 扩批另由 manager 审核。

## 2. 九相 schedule 与规范摘要

平台命令序列固定为 `0/+15/0/+30/0/-15/0/-30/0`。每相沿用注册的 **5100ms**，总命令时长 **45900ms**；不含启动、导出和校验耗时。每相自命令接受时间起计时，爬升时间包含在 5100ms 内；不得等到平台到达后重新计时、延长相位或重复相位补 bracket。记录计划与实际命令时间、phaseIndex 和输入 sequence，按现行输入时序闸门核验。

以下为规范 schedule 内容：UTF-8、无 BOM、LF 换行、末行后一个 LF，无空行、空格或 Markdown 围栏。`startOffsetMs` 相对第一条命令，phaseIndex 为 0 起始；runner 输入 sequence 映射为 phaseIndex+1，并在采集制品中显式登记。

```csv
phaseIndex,targetTiltDeg,holdMs,startOffsetMs
0,0,5100,0
1,15,5100,5100
2,0,5100,10200
3,30,5100,15300
4,0,5100,20400
5,-15,5100,25500
6,0,5100,30600
7,-30,5100,35700
8,0,5100,40800
```

`scheduleSha256=685be2bcbfdc14e261d86e3a105fc9a383a386709c02b96613f48f46504fe5a4`。

该摘要绑定上述规范 CSV，不声称 runner 文件已生成。若 runner 使用另一格式，同时登记其原始字节 SHA-256、转换器版本/摘要及逐行等价映射；不能把规范摘要冒充执行文件摘要。任何角度、顺序或时长变化均为新 protocol revision，重新冻结，不现场修补旧 manifest。

## 3. 冻结清单与冻结完成条件

| 冻结对象 | 本稿规定 | 采集前必须绑定的制品/状态 |
|---|---|---|
| schedule | §2 的九相、5100ms、规范摘要 | effector 导出的 schedule、runner 字节摘要及映射；当前待绑定 |
| 平台识别 | §4：角容差 0.20°，静止速率容差、200ms/至少 3 子步稳定窗口，planner 排除规则 | checker 版本及字段映射，不能只检查 targetTilt |
| 阈值候选 manifest | 搜索窗不变；列出精确 candidateId、thresholdCalibrationId、thetaStartDeg/thetaStopDeg、pairId、单位、路线、初态/几何/设备/build 身份、协议/schedule 摘要 | **精确候选列表及 manifest SHA-256 待绑定**；不从旧 run 的候选静默继承，不在看 pilot 后追认预注册 |
| 排除规则 | §4–6 的平台/连续性/首转折/安全/删失规则，以及原噪声分层 | 冻结规则版本与 checker 摘要；禁止残差大小驱动事后剔除 |
| 随机化 | §7 的 run 整体分层、seed=20261019、全新随机化制品 | 新 run 槽位、排列、分组文件及 SHA-256；禁止沿用旧 34-run manifest |
| fit/holdout | 每 pair 24/10；pilot 永久排除；先分组后读取结果 | 不可变槽位到实际 runId 的一一绑定、封存/揭盲审计 |
| 采集/检查身份 | source commit、HAP SHA-256、ledger schema、runner/checker 摘要、设备、模式、初态规则 | 精确身份与全部必要字段可用性证明；缺失不得填默认 false/0 |
| 授权 | manager 的单 run pilot 批准及适用 protocol/build | 引用裁决/批准记录；本稿不是启动命令，扩批权限不随冻结自动产生 |

冻结包应包含本文摘要和上述各制品摘要。候选 manifest 至少先冻结本次 pilot 的实际 pair；以后增加候选须在其采集之前登记新 revision，不能称为本次 pilot 的预注册候选。正式批次随机化制品须在扩批前冻结；pilot 阶段可登记 `NOT_STARTED`，不得伪造已分组的正式 run。

## 4. 实际接受平台识别与稳定平台过滤

### 4.1 数据与平台状态

每子步需要 runId、substepId、输入 sequence/phaseIndex、模拟时间/physicsDt、targetTilt、effectiveTilt、effectiveTiltRate、planner 的 thetaRequested/thetaAccepted、拒绝/限幅/hold 原因及提交状态。所有量须来自同一子步；effectiveTilt 与实际提交参考角 thetaAccepted 的映射由 tester 验证。`thetaRequested` 指本子步 planner 的真实 proposal，不能拿 UI target 代替。

对于命令角 `T∈{0,±15,±30}`，先定义**到达候选平台**：`abs(effectiveTilt−T)≤0.20°` 且 effectiveTilt 与 thetaAccepted 一致（角度数值核对容差 `1e-9°`）。0.20°用于区分平台身份，可容纳已观测约29.906°与名义30°的差；它不是 tan30 的额外安全余量，残差硬闸门照常逐边核验。不得把此容差用于阈值搜索端点、bracket 扩宽或安全判定。

**稳定接受平台**须在同 phase、同连续段内同时满足：

1. 每条记录均到达候选平台；`abs(effectiveTiltRate)≤1e-9°/s`，窗口内 effectiveTilt 极差 `≤1e-6°`（仅数值静止容差，不允许缓慢 ramp 冒充平台）。
2. 连续至少 3 个提交子步，首尾模拟时间跨度 `≥200ms`，无缺步、重排、重置、模式/候选/几何改变。
3. 完成上述前置窗口的当前记录起才获得平台资格；不得回填窗口前段为稳定样本。离开条件立即结束该段，重新进入须重新累计 200ms。

保留每相首次到达/稳定时刻、实际角 min/max/median、最大速率、稳定持续时间、planner 排除时间和剩余可采窗口。命令 hold=5100ms 不等于稳定样本时长5100ms。

### 4.2 planner 接受与 bracket 资格分离

稳定在约29.9°可以记为**到达的接受角平台诊断**，但若当前或前一 bracket 端点仍有 planner rejection/hold/clamp、未提交，或 `thetaRequested≠thetaAccepted`（按上述数值核对容差），该端点不具备标定资格。单有 `targetTilt≠effectiveTilt` 既不能认定平台已到，也不能替代 planner 决策字段来推断 hold。

只有稳定接受平台中无上述限制的连续记录可进入转折分析。持续被限幅在29.9°的 run 可给 ceiling 证据，不能把 hold 伪装成真实 stop 或可用 bracket；若因此全部 bracket 被排除，照实报 yield=0 / INCOMPLETE，交 manager 判断，不放宽过滤器。

正负 cell 由当前稳定平台 **effectiveTilt 的符号**决定，不能用 donor、残差符号或命令符号代替。0°相位单独诊断，零相位真实 stop 不移入前一正负 cell。±15°可提供静止/停止证据，但不能凭名义角证明启动；±30°也必须由实际局部残差证明越过候选阈值。

## 5. 逐边统计单位、四 cell 与原门槛

### 5.1 状态样本与转换 bracket

物理边固定为 `c=interfaceIndex=min(donor,receiver)`，搬运记录还须 `abs(donor−receiver)=1`；不能把有方向的 donor index 当固定边身份。无搬运状态记录须有明确固定 edge index，不能靠最近一次 donor 猜测。

输入至少包含 `(runId, substepId, c, effectiveTilt, d, r, activeBefore, activeAfter, requested, granted, budgetScale)`、前一子步同边状态/残差及采样时点定义。`d` 为按同一物理快照和 accepted tilt 求得的有符号残差长度；`alpha=atan(abs(d)/dx)`，若 r 已是归一化残差，则 `alpha=atan(abs(r))`，输出单位为度。禁止把 apply 后残差与决策前状态混配。

状态样本按 `activeBefore=false → start 类`、`activeBefore=true → stop 类`分类，包括 hold 与 flip。真实 start 必须 false→true，真实 stop 必须 true→false；`grant=0`、phase/run 结束、reset、参考线回摆、planner hold 和限流均不能补成转换。

合格 bracket 须同 run/候选/几何/phase/稳定段/固定边，substepId 连续且无输入身份间断，前一 activeAfter 与本步 activeBefore 连续；两端均满足 §4，且不处于预算裁剪（含零 grant）造成的歧义状态。以状态判定时点的相邻 alpha 夹住候选：start 为 `alphaPrev<thetaStartCandidate≤alphaNow`，stop 为 `alphaPrev>thetaStopCandidate≥alphaNow`。等于门槛时的状态语义沿用原协议，不调整运行比较符。语义不一致列出 checker failure，不修改数据使其成 bracket。

沿用每 `(runId, phaseIndex, c)` **首个合格转折**规则（start 或 stop 中先出现的一个，不是各留一个）；后续往返翻转保留原始记录但不进入估计。转换残差分位数使用同一合格子集。不同 run/phase/稳定段之间绝不拼 bracket；起动发生在 ramp、进入平台时已 active，不能补一个平台 start。

### 5.2 四个主 cell

| cell | 状态样本归属 | 转换样本 | 必须独立报告 |
|---|---|---|---|
| start/+ | activeBefore=false 且 accepted tilt>0 | 合格 false→true | 状态数/run 数、start 数/run 数、alpha/bracket 覆盖、删失 |
| start/− | activeBefore=false 且 accepted tilt<0 | 合格 false→true | 同上，不能借 start/+ 补齐 |
| stop/+ | activeBefore=true 且 accepted tilt>0 | 合格 true→false | 状态数/run 数、stop 数/run 数、覆盖、未观测原因 |
| stop/− | activeBefore=true 且 accepted tilt<0 | 合格 true→false | 同上，不能借 stop/+ 补齐 |

每 candidate pair、每 fit/holdout 组分别输出此表。**每主 cell 至少30个有效状态样本、来自至少10个独立 run；每 run 每主 cell 最多计3个**，按时间、substepId、c 排序取前3个，不按残差选样。本稿对四主 cell 的数量要求不替代转换数要求：每组每方向的 start、stop 各至少20个合格转换，分别来自至少5个 run。

原协议的残差符号×near/shoulder/far 噪声审计仍保留：上述四 cell 各拆 `d<0/d>0 × 三噪声层`，合计24子 cell；fit/holdout 每子 cell 原有 ≥30状态样本/≥10run、每run最多3个及误判率闸门不删除。主 cell 与子 cell 各自独立按原始合格记录取样计数，不能把六个子 cell 的计数直接相加当作主 cell 独立样本。`d=0` 和0°平台仅作 neutral 诊断。四 cell 是 §8.16 的识别汇总维度，不是取消原噪声门槛的授权。

状态覆盖先执行平台、身份、连续性与安全资格，转换还执行 bracket/首转折规则；二者分母分列。原噪声定义保持 `sigmaNoise=1.4826×pooled MAD`、`qResolution=median(fit bracket宽度)/2`、`wNoise=max(1.96sigmaNoise,qResolution)`；只用 fit 数据冻结。距相应候选阈值 ≤wNoise 为 near，(wNoise,2wNoise] 为 shoulder，>2wNoise 为 far。缺量、非有限或 wNoise=0 时不得分配假噪声层，报告 insufficientEvidence。

### 5.3 数量与估计闸门

每候选 pair 至少 **34个独立完整合格 run：24 fit+10 holdout**，每组每方向至少5run；四 cell、转换及原子 cell 门槛分别检查。某方向有记录不等于该方向有有效转换。34run 是下限，不保证满足 cell/bracket 配额。

任一主 cell 不足，即 `overallVerdict=INCOMPLETE`、`reason=insufficientEvidence`；禁止跨 cell、跨候选、跨版本或 fit/holdout 合并补缺。任一原子 cell/转换/独立 run 配额不足同样不能标定通过。

估计仍为合格事件 bracket 中点→各 run 中位数→run 等权中位数，正负方向独立给出；禁止合并缺失方向来制造估计。10,000次分层 run-cluster percentile bootstrap、seed=20261019、95%分位线性插值不变。原门槛包括 CI宽≤2°、fit/holdout差≤1°、各方向/type 的 holdout中点落fit区间外≤5%、原状态子cell误判率≤5%、start CI下界>stop CI上界、点间隔≥2wNoise、start CI上界<30°；硬安全/身份/stop语义错误仍为零容忍，不以统计误判率豁免。任一不足/不可辨时 `thetaStartEstimate=null, thetaStopEstimate=null`；候选输入可照实展示，但不得发布默认拟合值。

## 6. 右删失与停止未观测的固定输出

每 cell 保存有效/被排除状态数、有效事件数、run 数、排除原因直方图、实际 `alphaMax`、bracket端点范围、acceptedTilt范围及 planner ceiling 证据。最大 alpha 仅从该 cell 合格的对应风险状态计算，不能拿另一个方向、active 状态或 ramp 的极值替代。

| 条件 | 输出与后续纪律 |
|---|---|
| start cell 的高端候选未被双侧 bracket 覆盖，安全/planner 边界阻止继续激励，实际 at-risk 残差覆盖止于候选以下 | `RIGHT_CENSORED_AT_TAN30`；报告实际可达残差端点、请求/接受角及约束原因。它表示当前安全实验暴露不足，不能证明材料真实阈值高于端点；不补端点事件、不拟合点值、不记 calibration PASS |
| 有稳定平台却无真实合格 stop | `STOP_TRANSITION_UNOBSERVED`；细分 neverActive、alreadyStoppedBeforePlateau、stillActiveAtEnd、onlyPlannerHold、onlyBudgetLimited、noEligibleBracket。报告 at-risk 时长和状态，不把结束、hold、grant=0或inactive到达当stop |
| start 只在 ramp 出现或进入平台已 active | `START_OUTSIDE_ELIGIBLE_WINDOW`；不能据此认定高端右删失，也不能纳入平台拟合 |
| 平台未到或无稳定窗口 | `PLATFORM_UNREACHED` / `PLATFORM_UNSTABLE`；不凭 target=30 推断 alpha=30；cell不足→INCOMPLETE |
| planner/状态字段缺失、连续性或边身份不可验证 | `TELEMETRY_INCOMPLETE`；不可把未知当无转换、无hold或已删失 |
| 任何现行硬闸门失败 | `RUN_INVALID_HARD_GATE`，隔离整个 run，不挑出其“好子步”进入fit/holdout；完整失败证据保留 |

局部 alpha 不等于 accepted tilt。仅看到 acceptedTilt≈29.9°不足以判定某候选删失；必须核对该 cell 的残差覆盖、风险状态和安全限幅证据。若有真实合格低段 start，也不能据此宣称 `[25°,30°)` 全窗可识别。右删失不是普通 FAIL，更不是 PASS；没有stop也不是thetaStop=0或20°。

输出有分层：硬失败的 run 先判 INVALID；统计配额不全的候选整体判 INCOMPLETE；cell 可同时携带上表删失/未观测原因码。pilot 的阈值判定始终 NOT_EVALUATED，即使碰巧产生多条转换也不拟合。

## 7. 新随机化与封存规则

沿用 run 整体分层随机、约70/30比例和 seed=20261019，重新生成 v2 manifest。分层键为 `(thresholdCalibrationId, candidate pair, schedule version, device/build identity, planned tilt-direction coverage)`；planned coverage 固定为完整九相的正负覆盖，不能依据观测成功与否改变随机层。不同设备/build 不拼成一个满足配额的层。

正式扩批批准后，每层先注册34个不可变 plannedRunId 槽位；按字典序排序，用固定 seed 的 Fisher–Yates 随机排列，前24 fit、后10 holdout。冻结生成器实现/版本、完整排列及输出摘要，以实际 manifest 为唯一分组依据；在读取任何该批结果前完成。每实际 runId 绑定一个槽位，采集后不改角色；不按时间切组、不交换好坏 run、不把 holdout 挪去fit。

pilot 使用独立槽位 `analysisRole=pilot`，永久不计34run、不参与拟合/噪声估计、不消耗或解封holdout。新批次中旧协议run计数为0。无平台或无任何合格 bracket 的run保留为pilot/invalid evidence，不能进入统计池；原分组记录仍保留，不偷偷补位或重随机化。

缺额按原组/原层登记追加槽位，由 manager 批准补采后预先分配给缺失组；不得重分既有 run。修改候选/平台规则/schedule/排除规则须新 revision，旧 holdout 不得成为新 revision 的独立确认集。

holdout 原始数据封存。tester 可独立执行预注册安全与资格检查，输出是否违规及缺额，不向拟合人员暴露阈值/残差分布；fit 候选选择、噪声量和分析制品冻结后才按既定流程揭盲一次。揭盲后不改判据或以holdout继续拟合。

## 8. 单 run clean pilot 判定与交接

启动前置：冻结 pilot 所需制品/候选、确认 checker/cases 已覆盖本协议（不能仅凭旧 checker 有15/30判断就视为就位）、必要 planner/状态字段齐备、绑定 manager pilot 批准。本文未填写的摘要/身份均是明确待办，不允许运行时自动补默认值。

| 维度 | 判定与报告 |
|---|---|
| clean资格 | 单一完整九相run、固定初态/身份、输入序列正确、无遥测/边身份/stop语义错误、所有现行硬gate通过；失败立即记INVALID，停止扩批 |
| 平台可达 | 每个非零相分别报告 §4 的到达/稳定时刻、稳定时长、接受角范围；四个非零相均有≥200ms稳定窗口方可报告 `platformReachability=PASS`。planner限制导致的不可用窗口另列，绝不折算成bracket |
| bracket yield | 四cell逐个报告有效状态/真实转换/首转折bracket数及排除原因，报告本run对各cell的有效贡献（最多3状态）。四cell均≥1合格bracket时仅标 `pilotCoverage=FOUR_CELL_OBSERVED`；任一为0标 `pilotCoverage=INCOMPLETE` 并附 §6 原因，不能据一run宣称可达正式配额 |
| 阈值输出 | `thresholdVerdict=NOT_EVALUATED`，thetaStart/Stop估计与CI均null；不生成拟合图、不估wNoise、不读取任何holdout |
| 后续 | 交manager平台与yield表、删失证据、预计缺额；是否扩批/加采/重议识别边界由manager决定，任何pilot结果均不自动触发34run采集 |

平台可达与 bracket yield 是两个独立结论：允许 `clean=PASS, platformReachability=PASS, pilotCoverage=INCOMPLETE`，不把它改写成标定PASS。尤其九相中15°在同向30°之前，停止可能发生在回零/ramp；若到15°已inactive，该设计可能没有stop/+或stop/−。此为要测量的识别边界，不通过调相序、延时或移用零平台事件掩盖。

## 9. 执行伪代码与资源估算

```text
verify frozen identities / schedule / candidate / analysisRole
audit every submitted substep with unchanged hard gates
if any hard gate fails: quarantine whole run; retain diagnostic output
for each phase and continuous substep:
    update accepted-platform window from actual tilt and planner telemetry
    record reachability and all exclusion causes
    for each fixed edge c:
        validate state identity, timing and same-edge continuity
        assign eligible state to one of four cells (neutral separate)
        if both endpoints eligible and true threshold-crossing transition:
            retain first eligible event per (run, phase, c)
aggregate four-cell yield / residual coverage / censor reasons
if analysisRole == pilot:
    output NOT_EVALUATED, null estimates; stop before any fit/holdout access
else:
    verify preregistered role, run quotas, four-cell and noise-subcell quotas
    if any quota missing: output INCOMPLETE; publish no threshold estimates
    else: apply original fit/bootstrap/frozen-holdout confirmation gates
```

流式提取为 O(N×127)，N为子步数；连续性缓存 O(127)，平台窗口 O(W×127)（W为200ms内记录数，可用滚动统计压缩）；输出状态/转换审计表随有效记录增长。bootstrap仅在正式fit阶段执行，10,000次按run重采样；pilot不运行。协议只分析已有物理字段，不改帧率、128列几何、预算或通道次序。

交付回执：designer 预注册规则稿已落盘；effector 负责采集制品与身份摘要冻结，tester 负责实现规则一致性及用例同步，manager 负责 pilot/后续扩批裁决。当前没有新增采集，没有阈值估计，观测③ BLOCKED 状态不变。
