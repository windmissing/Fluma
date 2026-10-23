# P-011 观测③·1B：双循环激励影子审计设计 v1

日期：2026-10-22。责任：designer。状态：**DESIGN_ONLY / SHADOW_NOT_RUN / NO_DEVICE_CAPTURE_AUTHORIZATION**。依据本次用户转达的manager“1B条件启动”授权，设计可立即交接；3-run真机pilot仍须manager另批。

本轮不改物理/测试代码，不执行候选搜索，不伪造各候选bracket数量。候选网格、判定顺序和输出schema由本稿给出，真正执行前须绑定初态、构建、时钟及重放器hash；尚未产生CSV胜出候选或SHADOW_COVERED结论。

## 1. 输入、授权范围与D1交接

依据：[数据路径§4.2/§10](design-P011-obs3-calibration-data-path-v1.md)、[D1报告§3.2/§3.5/§5](verification-report-P011-obs3-d1-replay-v1.md)、[v3冻结资格§5–7](observation-protocol-P011-v3-preregistration-v1.md)。

| 已交接事实 | 本设计使用方式 |
|---|---|
| stop/+：NONMONOTONE∩PREV_BUDGET 531笔；stop/−同交集697笔，合格均0 | 两类障碍必须同时消除，不以修复PREV标签代替激励改善；交集不可随意与其他原因相加 |
| start/−：fit 9事件/9run，holdout 2事件/2run | fit的≥5事件来源run达标，但距20事件仍缺11；holdout缺18事件且仅2run。D1正文“仍缺9”不作为本稿预算依据 |
| start/+与stop/±：两组均0合格 | 任何一cell为0都不能以start/−补足；旧14已由D1重核为11，不继续沿用14作产率 |
| 现有holdout残差未解封 | 只用上述覆盖QA；不打开holdout明细、用其拟合B或挑选初态 |

本次最新授权重新打开**1B设计任务**；此前“路径5：用户参数外置”仍是产品参数机制方向。影子研究不把26/20推荐值改称标定结果，也不擅自恢复“统计标定必须完成才能阶段一CLOSED”的旧标准。用户将本任务排为阶段一关键工作，其设计验收/排期由manager管理；若需进一步调整总阶段边界，由manager明文登记，本文不自行宣布阶段一关闭或回退参数外置。

研究路线固定 `natural-relax-hysteresis-v1`，**不启用correction**。26/20是本次唯一待研究的已知配置pair，configured/executed必须一致；产品推荐来源与研究manifest身份分列。不得以改变pair、改预算/材料参数或加入新修正机制来解决当前bracket不足。

## 2. 冻结候选域与搜索顺序

### 2.1 网格

用整数“十分之一度”生成，避免浮点累加偏差：

```text
a_s ∈ {152,...,299}; b_s ∈ {151,...,a_s−1}, s∈{+,−}
A_s=a_s/10; B_s=b_s/10
15 < B_s < A_s < 30
主档v=0.5°/s；命令间隔100ms；角步0.05°
所有平台HOLD=5100ms；所有峰值A_s不hold
```

A=15.1没有可容纳的0.1°网格B，故自动为空，不偷偷令B=15。单方向(A,B)共11026对，完整非对称四元组空间11026²=121,572,676。允许A+/A−及B+/B−分别登记，**不假设正负可达对称，不默认18.7可用**。0.3/0.7/1.0速率、延长hold及改command cadence不在本轮搜索域。

预先固定候选排序键：`(max(a+,a−), a++a−, a+, a−, −b+, −b−)` 升序。优先较小峰值；相同峰值从靠近A的B往15扫描，使回程后尽早进入停止平台的假设先被检验。这是确定性计算顺序，不是“B越高必然更好”的物理断言。最终选择该顺序下**第一个满足§7完整条件**的候选，不按看过的最高yield/最窄bracket反复调排序。

### 2.2 planner+guard包络不是单个固定上限

每条完整候选轨迹在冻结初态/时序下调用真实planner与全writer guard，不用去guard简化模型，不把旧正向29.9/负向26.88作为本轮永久包络。输出每次requested/accepted、约束边/原因、commitStatus和预算；包络与当时材料/active/历史共同绑定。

硬安全失败立即淘汰该轨迹。planner clamp/hold/BLOCKED是合法拒绝，但本轮要求输出的pilot候选在注册轨迹集内全部命令可完整接受，无这些限制；受限轨迹只留诊断，不能被过滤后仍称可提交包络。不能从某A受限推断所有更大A必受限——历史改变后可能非单调，只有相同前缀状态下的严格共享失败才可剪枝。

### 2.3 可复用前缀与计算预算

完整正程（至中间回零）只依赖(A+,B+)与初态/时序；其start/+或stop/+覆盖失败，任何负程后缀都无法补回正cell，可安全淘汰该前缀。通过的正程终态必须携带全部材料、active、预算临时/输入/配置状态继续负程，**不能把独立负程的合格计数拼成完整run**。

可缓存逐命令共享前缀，恢复完整快照并核验hash；不能只缓存S或把active清零。对称组合也在同一排序中，不单独优先到破坏最小峰值声明。搜索运行采用固定分批、可恢复枚举账本；每候选记TESTED/REJECTED/PRUNED_BY_PREFIX/NOT_RUN及可核验理由。

执行前用固定网格起始的10个完整候选量测host秒/轨迹、峰值内存和输出大小（只估算吞吐，不据收益修改选择规则）。复杂度上界为 `O(候选数×注册轨迹数×子步数×128)`，比“三个A试一下”大得多。每轨迹只流式保留前驱窗口、计数及事件邻域，选中/异常轨迹再留完整账本；不得为省IO丢资格端点。

若算力/磁盘/执行时限耗尽而未覆盖全部剩余候选，输出 `SHADOW_SEARCH_INCOMPLETE`，不是SHADOW_UNCOVERED。只有遍历/严格剪枝足以覆盖全部注册域才可报告域内未找到。执行预算由manager排定；本设计没有无限搜索授权或真机成本。

## 3. 十三平台、十六leg及边界规则

| 顺序 | 目标/动作 | 标识 |
|---|---|---|
| 0 | 0 hold | H0 |
| 1 | 0→+15；+15 hold | L01 / H1 |
| 2 | +15→+A+，到峰立即反转 | L02 / T+1（turn） |
| 3 | +A+→+B+；+B+ hold | L03 / H2 |
| 4 | +B+→+15；+15 hold | L04 / H3 |
| 5–7 | +15→+A+→+B+ hold→+15 hold | L05/T+2/L06/H4/L07/H5 |
| 8 | +15→0；0 hold | L08 / H6 |
| 9 | 0→−15；−15 hold | L09 / H7 |
| 10 | −15→−A−，到峰立即反转 | L10 / T−1 |
| 11 | −A−→−B−；−B− hold | L11 / H8 |
| 12 | −B−→−15；−15 hold | L12 / H9 |
| 13–15 | −15→−A−→−B− hold→−15 hold | L13/T−2/L14/H10/L15/H11 |
| 16 | −15→0；0 hold | L16 / H12 |

L01–L08预注册正向cell归属，L09–L16负向；两端actual accepted仍必须非零且同符号。每个循环legId独立，不能把两轮L02/L05等拼接。所有ramp endpoint命令唯一归属：到平台的终点属于新H；到峰的终点命令在同一时间戳归属新下程leg起点，同时记录turn标记，不额外发送一条重复峰值命令、不多等100ms。下程必须重新积累相邻三点，不能借上程端点。若现有播放器不能表达此规则，先由architect完善身份绑定，不能通过省略turn标记获得bracket。

平台hold仍为5100ms，从终点命令时刻计，不因accepted迟到或暂未见stop重启/延长。稳定资格依据该H注册目标±B/±15/0、actual accepted容差0.20°、rate≤1e−9°/s、连续≥3提交子步、跨度≥200ms、角极差≤1e−6°；建立窗口后才入池。不同H即使角度相同，也不能跨边界接窗口。

```text
T_main = 13*5.1 + [4*(A+ + A−)−60]/0.5 秒
```

此式仅是交叉校验；实际 `scheduleEndOffsetMs` 由CSV终点及末hold精确求和，不沿用旧九相公式。A+=A−=18.7时主段245.5s仅为成本情景，不是可用候选声明。

## 4. 针对stop锯齿与限流的设计对策

令 `z=G−tan(thetaAccepted)`、`alpha=atan(abs(z))`。同输入期间Δtheta=0但writer改变G；离散步有 `Δz=ΔG−[tan(theta_next)−tan(theta_now)]`。因此输入“下降程”不能代替三点alpha非升，进入B平台也不能保证停止或单调。

| 对策 | 被检验的假设 | 每候选验证与失败解释 |
|---|---|---|
| A峰不hold | 避免在峰上等待5.1s把active全部耗尽，尽早进入下程 | 记录每个turn真实active集合及距停止线alpha−20；若反转前全inactive，记NO_ACTIVE_AT_TURN，不能人工激活 |
| A从低往高的有限网格 | 足以产生合格start且不过度积累竞争/超角请求 | 同时检查start/+，不是只优化stop；有start但当前端点被裁剪仍不合格 |
| B从靠近A往15扫描 | 较高B较早去除命令阶梯；较低B可能给较强下降驱动但让stop发生在平台建立前 | 输出下程/平台各自stop量、STOP_BEFORE_STABLE、到B后active存活时间、三点窗口与前端点grant。不能凭经验固定高B或低B |
| B固定hold5100ms | 给竞争writer衰减和平台窗口建立机会，但不增加已注册时间额度 | 全hold记录，不能只取后半段“干净片段”。稳定后仍active且无裁剪的真实stop才能贡献；平台等待不自动等于激励 |
| 双循环 | 自然历史可能让第二圈更易形成完整窗口 | 每圈分别输出；第一圈与第二圈不算两个独立run，不拼端点，不删除较差一圈 |

孤立边如果只按 `q=λ*(r−tan20)/2` 修正且0≤λ<1，正超角E下一步为 `(1−λ)E`，可能只渐近停止线而不发生有限时间跨越；因此“hold更久”不是必然解。B的位置必须使真实下降与预算未受限窗口相交，且两端宽≤0.5°。不暂停settle/percolate/creep/lift，不提高额度，不改变自然q来强迫停止。

逐stop窗口输出三个相邻decision的 `alpha,G,accepted,active,requested,granted,budgetScale,guard/stock/capacity限制`，并按实际writer顺序重放ΔG：区分参考角阶梯、材料反馈和哪个通道引起反向增量。slot前后增量不足以重建时标ATTRIBUTION_UNAVAILABLE，不拿整步前后差冒充逐writer因果；资格判定仍用真实decision时点。

拟报告两个描述性产率：`P_mono=三点非升stop窗口数/可检验的真实stop窗口数`；`P_joint=全资格首事件stop数/原始真实stop数`，并报告分母、边/phase及原因交集。分母0=N/A。它们是有限影子轨迹集频率，**不是独立样本概率或全局最优概率证明**。优化方向是减少“单调但前端点被限流”和“无限流但锯齿”同时出现的机会，最终仍用完整资格的实际bracket数选候选，不用P_mono最大替代覆盖。

平台继续按原平台资格取两端bracket；ramp严格检查两端及紧前点的三点链。为防止仅移到平台标签就掩盖锯齿，平台另外报告三点形状作诊断，但不擅自把原平台资格改成新的物理/统计规则。所有阶段均不平滑alpha、不跨反转/零点/平台边界、不向前搜索有利端点。

## 5. G1/G2/G0初态与时序分层

首轮仅覆盖D1的已登记物理变体：128列、capacity192、thick材料及其完整物理参数。以D1采集manifest/归档源码参数为基准，不用旧shadow脚本的临时diffusion=0等覆盖充当默认；缺完整参数绑定则 `INPUT_BINDING_PENDING`，不能直接开跑。

G1/G2/G0沿用“竞争/非均匀/均匀控制”的分析目的，但本轮是**自然观测路线**，不导入correction矩阵的人造热点坡形或预置active。用同一canonical初态和合法输入前缀形成历史：

| 层 | 采前固定的形成方式 | 验证目的 |
|---|---|---|
| G0均匀基准 | canonical thick直接初始化，然后主段H0；active按正常run初始化 | 最接近原数据的基准；不因无覆盖将其悄悄剔除 |
| G1竞争历史 | canonical后固定前缀0 hold→+15 hold→0 hold；ramp同0.5°/s，hold同5100ms | 检查正向沉积历史及普通writer/滑移竞争下的资格；是否实际竞争由trace确认，不宣称前缀必然制造竞争 |
| G2非均匀/反向历史 | canonical后固定前缀0 hold→+15 hold→0 hold→−15 hold→0 hold，ramp/hold同上 | 检查自然非均匀与反向历史；输出实际G分布/限制边，不指定目标坡形 |

每层另登记镜像输入前缀（G1先−15，G2先−15后+15），G0镜像同一状态只记一份。原始形状与镜像不得合并计数。前缀属于同一安全run且全量观测，但明确处于conditioning段，不进入主段bracket统计；主段H0开始不清材料/active或重置预算时钟，只断开测量段前驱，不能用conditioning中的事件填四cell。

冻结时序集合：帧驱动dt为1/30、1/60、1/120s三档，命令相对帧起始偏移为0、100/3、200/3ms三档，共9轨迹/历史；由引擎真实子步拆分，不能直接把frame dt写成physicsDt。G0共9、G1共18、G2共18，合计45条注册轨迹/四元组。采用偏移测试命令阶梯与材料子步相位耦合，不能选唯一最有利相位。

使用虚拟单调时钟驱动真实输入处理/播放器与物理引擎，所有attempt均记录；timestamp单位与shadow来源显式绑定。禁止以快速host执行时的Date.now间隔冒充100ms接受间隔，禁止为了凑连续性覆盖真实decision/commit字段。不假装这45条确定性轨迹等于45个独立真机run。

每个层输出实际风险状态、普通通道请求和非均匀程度；若预期层特征没有形成，保留结果并标 `STRATUM_FEATURE_NOT_EXCITED`，不在看结果后换前缀。要换前缀需新搜索manifest。首轮先按上述有限集合检验，之后真机时序/更多自然初态适配另议，不把它称为全产品泛化。

## 6. D1重放器复用契约与必须先解决的接口问题

复用 [p011_obs3_d1_eligibility_replay.py](../test/p011_obs3_d1_eligibility_replay.py) 的逐边资格路线，不另建一套“shadow简易判定”。本次所读SHA256为 `6cb6882b19fc338c90483b44947379dee074205c8e87cdc2ef8fba34dae468e1`，与D1报告登记一致。**复用意味着tester版本化扩展schedule绑定及补全契约检查，不意味着现文件可直接接受十三平台。**

| 代码核对点 | 执行前要求；不改变冻结资格 |
|---|---|
| RAMP_LEG_SIGN写死L1…L8；PlatformWindow只认识旧±A等名单 | 从冻结CSV读L01…L16、turn和H0…H12的真实平台/符号；新峰值A不是平台，B才是新平台。稳定窗口在每个segment边界重新建立 |
| scheduleEnd表达式将 `9*PLATFORM_HOLD_MS` 与秒量相加后再乘1000 | 单位修复：旧A=18.7正确结束偏移195500ms，当前表达式得到46049600ms；新版直接使用CSV endOffsetMs。保留D1历史报告，不在本设计里重算或覆写其34run结论 |
| endpoint_clear读取edge requested/granted，没有用context独立核验planner | 材料grant不等于thetaRequested/thetaAccepted。每端检查context的clamp/hold/reject/rollback/commitStatus及完整attempt连续性；材料限制独立检查 |
| 前驱存了p_leg，但同段分支只检查p_seg_kind非空；三点历史仅保存alpha | 显式比较run/segment/leg/配置身份；紧前第三点也要连续、同leg、提交且未受限。无decision的拒绝attempt仍要打断窗口，不能靠稀疏substep编号推断完整 |
| timing目前只在当前seq检查；状态计数仍直接累加 | 两端与第三点timing按原链独立核验；状态每run每cell最多3，与事件首个去重分离。缺字段为UNVERIFIABLE，不能靠候选26/20 fallback通过身份 |

这些检查针对新shadow输入的复用充分性；D1交接的“旧结构stop仍无合格”作为已报告事实引用。若tester发现D1已有计数受残余问题影响，应独立追加更正报告，不让designer用未经重放的新数字替换它。

旧 `run_p011_obs3_v3_shadow_audit.cjs`/detail脚本可复用ETS加载/真实引擎调用思路，但不能复用其简化bracket判断、固定60Hz、旧九相时序或只取每帧最后record的覆盖逻辑。必须捕获引擎每个子步及未提交attempt；当前配置主链已迁移，26/20须经configure显式绑定，metadata只作身份记录，不依赖其旧副作用。

tester在实施时应以相同资格函数核验小型契约样例：跨turn/零点/H边界必须排除；非零同leg合法三点可入；前端点及第三点分别受限必须排除；同角不同H不能接；postSchedule尾部不计；requested=granted不能掩盖planner clamp。此处只定义验证需求，不改或执行测试代码。

设计运行流程：

```text
bind source/config/initial-history/timing/csv-schema/eligibility hashes
for candidate in frozen ordering, with proven prefix pruning:
    generate its 13-hold/16-leg CSV and exact end offset
    for each registered G/history/timing trajectory:
        initialize true engine; replay full safe conditioning prefix
        execute candidate through real planner/writers/guard under virtual clock
        preserve all attempts and all per-edge decisions, no hand-set active
        run shared D1 eligibility analyzer with manifest-driven segments
        output four cells, 24 subcells, reason intersections and witnesses
    reject unsafe/limited/incomplete identity candidates without hiding failures
    if all coverage/noise conditions hold: emit first passing candidate; stop search
if no candidate passed and domain completely tested/proven pruned: SHADOW_UNCOVERED
if search/input/tool binding incomplete: separate INCOMPLETE/BLOCKED, never fake zero
```

## 7. 四cell覆盖矩阵与选择判据

每个 `(candidateId, variantId, G, historySign, dtProfile, commandPhase)` 必须有四行，另带24子cell；总表不能只输出合格事件。建议输出 `candidate-summary.json`、`coverage.csv`、`reasons.csv`、`bracket-witness.ndjson` 及搜索账本。

| 字段 | 含义 |
|---|---|
| A+/B+/A−/B−、input/source/config/tool hash | 唯一候选及可复现身份 |
| cell=start/+、start/−、stop/+、stop/− | 固定四格，零记录也输出 |
| rawTransitions / eligibleTransitions / retainedBrackets | 原转换、全资格、每(run,segment,edge)首事件三个分母，不能混用 |
| perCycle / rampVsPlatform / distinctEdges | 区分两循环及发生位置，避免全部机会来自一次边界现象 |
| alphaRiskMin/Max、activeAtTurn、activeAfterBStable | 判断未激励、过早stop与平台存活 |
| P_mono/P_joint及分母、reasonCounts/intersections | 非单调、前限流、过宽、边界、零点、timing及其交集 |
| plannerLimited/blocked/hardFailures / stableWindow | 可提交包络、安全与平台资格 |
| residualSign×noiseBand及noiseDefinable | 24子cell的存在性、计数及噪声来源，缺层不能合并 |

本设计尚未执行，当前每候选/层表应为：

| 候选/G层 | start/+ | start/− | stop/+ | stop/− | 状态 |
|---|---|---|---|---|---|
| 每个注册四元组/G0 | null | null | null | null | NOT_RUN |
| 每个注册四元组/G1（两历史分别） | null | null | null | null | NOT_RUN |
| 每个注册四元组/G2（两历史分别） | null | null | null | null | NOT_RUN |

执行后将null替换为该轨迹实际 retainedBrackets；0仅表示已执行且无合格事件，未跑、预算截止或提前中止用null与原因。跨dt/层汇总同时保留min/median/max，选择时看最差轨迹，不能把45条中各贡献一格拼成“每个run四格齐”。

判据：

1. **SHADOW_COVERED**：完整45条轨迹均安全、输入无限制、身份/资格可核验，**每条轨迹四cell各≥1 retained bracket**，有注册稳定窗口提供噪声审计输入。这沿用“每cell有真实完整bracket”的shadow门，不冒充正式统计充分。
2. **产率目标**另列：每条轨迹各cell≥2，且报告两循环的贡献，标 `YIELD_TARGET_2_MET/NOT_MET`；若只有1，不降正式20事件/组门槛，成本从最低34run改为按实际保守yield估算。该目标不是新资格过滤条件。
3. **噪声/24层**：只用shadow设计集按原定义计算暂定sigmaNoise/qResolution/wNoise；输出来源 `SHADOW_ONLY_NOT_FIT`。稳定无转换窗口不足、量非有限或wNoise=0→SHADOW_NOISE_UNDEFINED，不补默认值。每层保留实际覆盖；某子cell缺失为ABSENT_IN_REGISTERED_SET，不直接当全域结构性无解。四cell覆盖但噪声/层缺口未解释时为 `SHADOW_PARTIAL`，不能自动提交“完整可行”候选。
4. **正式门槛独立**：shadow不填真实run配额。未来真机仍按每pair/层34run、24fit+10holdout、每组每方向/type≥20事件/≥5run、四cell/24层各≥30状态/≥10run、原bootstrap/CI执行。改schedule需新统计层，不重分旧34runs；不得借shadow的wNoise解封holdout或替代真机噪声。

完整pilot候选要求1通过且3无未解决的噪声/缺层问题；2不满足须显式提交额外成本，manager决定是否仍批探索pilot。选中的首候选输出资格端点和完整轨迹供复核，不仅给一个“最优A/B”。

## 8. CSV交付、失败分支与成本

找到满足条件的首候选时，输出而非部署：

- `candidate-<id>.csv`：commandSequence、offsetMs、segmentId、phaseKind、legId、targetTiltDeg、rateDegPerSec、turn marker与terminal endOffsetMs；真实hash只能在生成后计算，本文不虚填。
- freeze manifest：四元组、13hold/16leg表、边界归属、初态与conditioning hash、45时序配置、完整参数/路线/配置来源、source/tool版本、eligibility映射、未决项为空、每cell/min-yield与噪声覆盖。
- `shadow-report`：已测试/可证明剪枝/未测试数量、完整矩阵、失败原因交集、选中候选原始证据和适用域。标source=shadow，不写HIL PASS、CALIBRATED或用户可见效果已达标。

没找到时分三类：

| 结果 | 含义与路由 |
|---|---|
| SHADOW_UNCOVERED | 注册有限域全部测试/严格剪枝后，没有完整可行组合；报告最小缺失cell及主要原因，转路径4，由manager明文延期；不是证明全通道/所有未来激励无解 |
| SHADOW_PARTIAL | 四cell或部分历史可覆盖，但其他层/噪声/产率证据尚缺 | 不合并、不自动选最好一层；交manager决定新revision或延期 |
| SHADOW_SEARCH_INCOMPLETE / INPUT_BINDING_PENDING / TOOL_CONTRACT_BLOCKED | 算力、输入或复用器契约未完成 | 不用“0 bracket”冒充执行结果，不据此证明无解；先补对应工程/执行条件 |

真机本任务成本为0。若未来manager独立批准3-run pilot，主段时长按§3计算；成本例A±=18.7为245.5s，沿用旧均摊额外开销55.09s/run，G0三run约15.03min（未含缓冲）。G1前缀为 `3*5.1+30/0.5=75.3s`，G2为 `5*5.1+60/0.5=145.5s`；若pilot三run各取G0/G1/G2一条历史，总约18.71min，加20%约22.45min，仅作情景预算。镜像/45时序全真机复核不是这3run已覆盖的内容；不得声称一次pilot验证所有层。

pilot角色永久与fit/holdout隔离。即使shadow成功，也不能越过manager采集批准或以设计稿替代tester对新schedule/资格绑定的复核。

## 9. 变体泛化与实施交接

[已完成的变体分析§10](design-P011-obs3-calibration-data-path-v1.md)已覆盖数量、插值、ID语义和路径矩阵；本稿将其作为输入，不等待外部任务状态。首轮冻结**单一已知物理变体**，G0/G1/G2是初始历史层而不是三种新材料。SKU仅改外观且证明物理等价时可申请复用，材料/容器真的变化则candidate包络、风险状态和bracket产率都需分别审计。

20物理变体逐个34run的高成本结论仍成立，故本轮不扩成变体×pair×速率的全面搜索。不使用未经验证的2–3锚点插值A/B或阈值；未来如果需要插值或跨变体免采，必须协议revision＋manager裁决。新变体/几何/构建不自动继承本轮CSV，产品参数外置也不授予其标定身份。

交接顺序：manager验收本设计的有限搜索域/历史与时序→architect绑定真实输入、完整状态快照与CSV/turn接口→tester复用并版本化D1资格工具、完成契约核验→离线执行者运行有限候选审计并输出矩阵→designer依§7/§8给出结论→manager决定3-run pilot或路径4延期。任何“条件启动”均不绕过安全、身份、资格和采集权限。
