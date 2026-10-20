# P-011 观测③ v3 识别策略重新论证 v1

日期：2026-10-20。designer交付。状态：`DESIGN_REVIEW / MANAGER_DECISION_REQUIRED`；观测③维持 `BLOCKED`。本文不改闸门、不移动搜索窗、不修改代码或checker、不授权采集，也不追认旧pilot的标定资格。

**后续状态与实测修订：** manager已批准方向2，方法已由 [v3预注册协议](observation-protocol-P011-v3-preregistration-v1.md) 冻结；下文§1–6保留原论证历史。2026-10-20新增§7，依据5e60f2d预验证修订正负可达包络与影子审计初态要求；不在本文件改写已冻结协议或授权新采集。

依据：[v2 pilot报告](effect-report-P011-obs3v2-pilot-run1.md)、[v2预注册稿](observation-protocol-P011-v2-preregistration-v1.md)、[algorithm-P011.md](algorithm-P011.md) §8.11/8.16、[architecture-P011.md](architecture-P011.md) §23.1，以及run-001原始 [ledger](../test/artifacts/p011-obs3v2-20261020/run-001/ledger.ndjson)、[source manifest](../test/artifacts/p011-obs3v2-20261020/run-001/source-manifest.json)、[calibration summary](../test/artifacts/p011-obs3v2-20261020/run-001/calibration-summary.json)。

**推荐方向2：将识别对象改为“安全提交的、可追溯的受控残差扫描”，允许符合新资格规则的ramp转折；稳定平台保留作状态保持/噪声/终点复核。** 搜索窗仍为start `[25°,30°)`、stop `[15°,25°]`。但第一道前置不是继续换平台角，而是确认真正执行的是双阈值状态机：本pilot的候选阈值只是元数据，原始转换尚不是这两个阈值的观测。方向1可作为回程stop激励的组成部分；方向4仅是以后可能的交付降级决策，当前连stop半份标定也没有证据。

## 1. 证据复核：失效不止是平台选择

### 1.1 可以成立的结论及其范围

run=`p011-run-1792472946390-3`，build=`891370d731eef32bf813f52910fa5a82439df2ed`。整run已有tan30硬失败，**INVALID且全部隔离**；即使单边超限仅约1.25e-7也不由designer豁免。修正旧phase名称不会让±30获得旧±45豁免，新协议明确没有该豁免。

±30命令没有合格稳定提交平台，长段帧为TILT_PLANNING_BLOCKED/substeps=0；不能把墙钟上“画面停在29.88°”当成持续提交的稳定物理观测。正30相位ledger仍有3条爬升记录、负30有4条，随后缺少稳定提交；因此精确表述是**本构建/初态/输入下的稳定平台不可达**，不是整个相位从第一帧起都没有子步，也不是证明所有安全轨迹都不可能到达某个较低accepted角。

平台策略在本pilot上失效，§8.16的“可能有约29.9°平台、高段右删失”不能用作当前事实。输出应为 `PLATFORM_UNREACHED / INCOMPLETE`，而非对未存在的合格暴露段赋 `RIGHT_CENSORED_AT_TAN30`。同样不能从一个已知实现存在R-3集成缺陷的失败run，推出tan30约束类在数学上全局无解。

### 1.2 新发现：候选元数据与实际驱动不一致

复核的是 `D:/hmprojects/hw-obs3v2-891370d/` 中与source-manifest **SHA-256逐字节匹配**的WaterSandField.ets和P011NaturalObservation.ets，不以正在演进的主工作区代码代替pilot构建：

- `WaterSandField::setNaturalObservationRunMetadata`仅保存thetaStartDeg/thetaStopDeg作为观测身份。
- 该构建`relaxAngle`实际使用 `limit=tan(params.angleDeg)`，activeAfter取 `requested>EPS`；thick场景angleDeg=4°。候选28°/22.911°没有进入这一双阈值驱动分支。
- Probe把上述请求状态与manifest里的28°/22.911°并列输出。`transitionTelemetryAvailable=true`及路线标签都不能证明实际双阈值动力学已运行。

因此，这批“start/stop”至多是**旧4°请求状态的转换诊断**，不是28°/22.911°的起停转折。即使解除平台过滤也不能拿它们拟合原候选；更不能由这批4°事件裁决start窗口应下移。

### 1.3 原始ledger的只读诊断计数

本节只做失效归因，不做拟合、bootstrap、wNoise估计或holdout读取。ledger共2020条、432024357 bytes，SHA-256=`3b47d2088539fea6fd4f8dd5677f0c17d646b93c8fd26fd66d3ec65a914ecdea`。

| 诊断 | 直接复核结果 | 对识别的含义 |
|---|---|---|
| 原始全部start | 500个；报告所列236+223=459是非零target方向子集，另41个在回零命令段 | 分母不同，不能混称全部转换；500个观测alpha均低于25° |
| 原始start时alpha范围 | 4.0002834°～14.3469999°（用atan(abs(edge.r))，r已归一化） | 不形成 `[25°,30°)` 的start bracket；这里只是原始事件覆盖，不是阈值估计 |
| 原始全部stop | 373个，均在commandSequence=3/5/7的回零命令段；按实际effectiveTilt符号为正246、负115、零12 | summary的stop/+、stop/−=0不能解释成整个轨迹没有stop；命令目标0不等于当前接受角0 |
| 原始stop时alpha范围 | 0.0842654°～3.9990762° | 属旧请求状态语义，不能识别stop `[15°,25°]` |
| +15精确静止记录 | target=effective=15且rate=0：206条；127边的activeAfter全true（26162个），alpha范围5.1336330°～15.3513866° | 与报告“到平台时边已inactive”不一致；仅证实旧请求态active，不证实双角滑行态 |
| −15精确静止记录 | 同条件204条；activeAfter全true（25908个），alpha范围5.5061580°～18.6714640° | 实际残差不等于名义倾角；仍没有原start窗口覆盖 |
| 相邻记录状态连续性 | 同物理边activeAfter→下一activeBefore不一致数0 | 原始旧状态链连续，主要问题不是把这些stop全丢成了不连续事件 |
| 协议/完整性 | summary报告calibrationProtocol等元数据缺失；committed ledger仅commandSequence 1..8，无第9回零的提交记录 | 不可把九相schedule文件存在等同于九相完整有效run |

以上“206/204静止记录”采用精确角/rate筛选，**不是重新认证200ms稳定窗口或合格bracket**。报告中的6.9s跨度还需effector按实际命令接受时间复核与5100ms注册hold的执行对应，不据此追加平台样本。缺少thetaAccepted/requestRejected/plannerHold的事实继续成立。

复核方法：顺序流式读取2020行，按context.commandSequence/targetTilt定位命令段，遍历relaxHysteresis.edges；统计transitionType与activeAfter，残差角用atan(abs(r))，方向统计另外使用effectiveTilt符号。不按旧phaseD推断新九相，不剔除越界行制造合格池。此处纠正诊断表述，原pilot INVALID处置不变。

## 2. 可辨识性首先取决于局部残差和真实状态

```text
g[e,t] = (S[e+1,t]-S[e,t])/dx
r[e,t] = g[e,t] - tan(thetaAccepted[t])
alpha[e,t] = atan(abs(r[e,t])) * 180/pi
```

阈值约束的是alpha，不是手机targetTilt、也不是accepted倾角本身。近乎平沙g≈0时才有alpha≈abs(thetaAccepted)。在+15°，若要alpha≥25°，必须 `g≤−0.1983585` 或 `g≥0.7342569`；所以15°平台**不是对所有可能坡形都不能start**，而是平沙附近不提供该激励，本pilot实际残差也没有覆盖25°。不能用更换名义角代替逐边覆盖证明。

双阈值识别需满足：同一物理边真实inactive→active的alpha跨start、真实active→inactive的alpha跨stop，判定时点/实际接受角正确，候选参数实际进入状态机，库存/容量/guard/预算不能伪装成状态转换。无量可辨时输出null，不填默认值。

另须区分两种研究量：给软件预设候选阈值后重现该阈值的转换，只证明**执行语义及该候选行为可复核**，不能凭这一自洽性估计材料“天然阈值”或证明候选最优。候选优选仍按原协议独立行为/效果判据与holdout验证；若要标定真实材料常数，还缺独立外部观测依据。v3 manifest必须写清此估计对象，避免用输入值重现输出值自证标定成功。

## 3. 四个方向的可辨识性与兼容性

### 方向1：±15°→0°→±15°滞后窗口

**可辨识性：有条件识别stop，不能单独保证start。** 若真正双阈值系统从inactive、近乎平沙开始，alpha最多约15°，达不到start≥25°；循环本身不能合法制造active。必须先由同一连续run内已记录的真实起动把边带入active，不能手工设active、从不明前史恢复，或把旧4°请求态当作预激励。

即使曾经active，若g≈0、thetaStop=22.911°（仅用pilot候选示例，非拟合值），从高角回落时应在accepted角约22.911°附近stop，**在到达15°之前**；15°→0°段不会再发生这个stop。若停止发生在回零途中，必须计算实际g与alpha，而非声称“从15°降到某个手机角就是停止角”。一般解是 `abs(g−tan(theta))=tan(thetaStop)`，g也随搬运变化。

相容性：回程由planner正常限幅、材料逐笔guard照常；不保证回程每步可提交，BLOCKED时间不能补事件。统计上回零的真实转折在v2被ramp资格排除，因此仅换循环仍不解决问题；需要方向2的新ramp资格。accepted角恰0的事件保持neutral，不挪入上一个符号cell。stop可能成为有效激励，但四cell、原24子cell与34run门槛依旧，不能据本pilot原始373个stop交付半份标定。

### 方向2：稳定平台 + 安全提交ramp联合激励（推荐）

**可辨识性：ramp并非数学上不能识别阈值；v2禁用它是为避免参考角错位、planner hold/回摆、采样宽bracket和请求态假转换。** 若这些混杂可独立观测并排除，同一边的决策前alpha在连续已提交子步间跨阈值，仍是有效区间观测。安全提交的角度改变是真实外力输入，不应仅因rate≠0就丢掉所有起动。

例：g≈0且真实双角状态机执行候选28°/22.911°时，安全accepted残差扫描27.8°→28.2°可夹start，回程23.1°→22.7°可夹stop，全在30°以内；这是解析可行的局部例子，不是本pilot已经实现的轨迹，也不证明前后全场guard必然允许。v3以实际alpha覆盖为准，不要求最终驻留在±30°。

相容性：只接纳planner已提交、未回滚、无hold/拒绝/裁剪歧义的样本，逐笔三边guard和最终全边tan30均必须通过；失败run整体隔离。新协议可以使用成功提交的普通rate-limited ramp，不把“角度不同于远端target”误判成planner拒绝；thetaRequested必须是本子步真实proposal。为最小化变更，**planner改变proposal的clamped子步仍不进入主标定池**，可作可达边界诊断。

统计兼容：样本数量、四cell/原噪声子cell、独立run、fit/holdout、CI与误判门槛不降；只是经manager批准更改转换资格及冻结ramp episode定义，必须新protocol、新候选执行身份、新随机化及独立holdout，不能给旧pilot补发合格证。若所有原24噪声子cell在可达场内不能覆盖，仍INCOMPLETE，不能因采用ramp就删除难采的层。

### 方向3：把start搜索窗移到15°～20°附近

**数学上较低start不与H=tan30直接冲突，但不是当前证据支持的窗口重估。** 若 `thetaStop<thetaStart<30°`，低start有更大静态余量；仍不能保证动态参考角跳变、库存和邻边guard都可行。原25°下界是先验搜索限制，并非由tan30定理唯一推出；是否重议是manager事项。

辨识收益可能是可达残差更容易穿过start，但有三项代价：现stop窗 `[15°,25°]` 与新start范围重叠，必须重新论证有序候选pair及噪声可分离；±15°静止回归可能受影响（真实alpha可超过15°，本pilot最高约18.67°但属于旧动力学）；较低启动会改变材料轨迹、视觉效果与后续收敛，不能只改标签。

统计上可以继承方法及数值门槛，但必须新候选manifest/独立数据/holdout，旧4°请求转换不能作为“真实start<25°”证据。按原先验决策树，只有真实双阈值/独立观测证据支持窗外转折，或合法可达域审计证明原窗无法检验，才能提交重议；不能为消除INCOMPLETE主动挪窗。本稿不推荐立即走此分支，也不授权修改。

### 方向4：接受部分标定，仅交thetaStop

**条件性后备方案，当前不能交付。** 必须先取得真实active前史与stop双侧bracket，stop/+、stop/−及对应噪声分层、独立run/holdout均达原门槛；当前pilot的stop都在旧4°语义下，不能据其证明stop窗可辨。单有±15稳定平台不是stop识别的充分条件。

安全门槛不变；缺start时不能确认两阈值CI分离/完整滞回带，不能把partial签为阶段一A完成。按现行协议任一cell不足整体INCOMPLETE且不发布阈值点值；若manager批准分量交付，才能另记 `STOP_COMPONENT_VALIDATED / OVERALL_INCOMPLETE`，thetaStart仍null，部署双角参数不随之获批。

对start的状态也须精确：无合格风险暴露/状态机身份不实为 `UNOBSERVABLE`，有完整合法暴露但未越start、且安全边界截断才可报告对应右删失；未到平台不能机械改名RIGHT_CENSORED。部分标定既不把stop样本挪给start，也不降低34run配额或跨符号合并。

## 4. 推荐方向2的v3工程/观测契约草案

本节是待manager批准的资格修订建议，不替换已冻结v2。选择理由：保持阈值先验和物理保护，同时把真实起停最可能发生的升/降残差区间纳入观察；比下移窗口少改变一个物理假设，比只交stop更有机会覆盖四cell。

### 4.1 必须先证实的输入与状态机身份

1. manifest分别登记 `configuredThresholds`、`executedThresholds`、状态机实现版本/代码hash。实际执行应具备独立start/stop及active保持逻辑，不能由 `requested>0` 代理；仅增加元数据不满足。
2. 每边在**状态机读取残差的时点**输出 `decisionG/decisionR/decisionAlpha`、effectiveTiltAccepted、activeBefore/activeAfter、transitionReason、thresholdUsed。当前ledger用整步stateBefore推算残差，而relax前已有settle/percolate/bottom；二者可能不同，必须显式消除这个时点混杂。
3. 对齐planner proposal/accepted、accepted/blocked/clamped/rejected、commit/rollback、attemptId、已提交substepId及受限边。BLOCKED无子步也须有独立attempt结果可审计；不能靠稀疏ledger推断拒绝次数。状态机和遥测不得在被回滚的预演中留下active变化。
4. phase从冻结schedule/command sequence映射，rampEpisode/leg及符号预先登记；不依赖旧七相phaseD。完整九相/新v3序列必须有实际执行证据，不能只看schedule文件。
5. 修复/证明architecture§23.1 R-3预演与guard的集成保真及本run超限原因；不在本设计里选择放宽epsilon。新clean资格要求完整安全闸门通过，不能因ramp首次可用就忽略5条违规。

### 4.2 ramp bracket资格（建议替代v2“只能rate=0”）

```text
same run / executed candidate / geometry / edge / preregistered ramp leg
adjacent committed substeps + adjacent attempts (no intervening blocked/reject)
previous activeAfter == current activeBefore
both records fully committed, actual decision residual available
no reset / input-source switch / inversion / rollback / hold / planner clamp
no budget/stock/capacity/guard-induced pseudo stop or ambiguous transition
start: false->true and alphaPrev < thetaStartCandidate <= alphaNow
stop:  true->false and alphaPrev > thetaStopCandidate >= alphaNow
```

两端按原硬闸门全部合法，run任何位置失败仍整体隔离。主分析沿用原转换对预算裁剪的排除纪律；因排除导致样本不足时如实INCOMPLETE，不在本轮额外开放“限流也算转换”的另一变更。

每run/ramp leg/固定边只取首个合格转折，升程和降程是分别预注册的leg，禁止从回摆中选最窄事件。慢ramp由多条命令构成时可跨相邻commandSequence但必须同一预注册leg；不能跨反转点/零点/平台边界拼bracket。正负cell由两端实际accepted角同号判定；跨零或零端点归neutral，不借其补正负cell。

主识别段要求实测alpha沿所需方向单调跨门槛，输入角单调不等于alpha单调；非单调段单独诊断，不按残差事后挑好片段。选段和首转折规则须采前冻结。建议新增采样可辨门槛：单事件bracket宽≤0.5°；它是预注册测量分辨率要求，不改物理阈值，不能靠许多宽区间的中点bootstrap得窄CI来掩盖采样不足。

稳定平台仍用于no-transition噪声、持态与终点复核，ramp不能直接用未经去趋势的alpha波动估计sigmaNoise。噪声估计沿用原独立稳定窗口/MAD规则；缺乏对应状态/噪声层便保持不足，不用扫描信号本身当噪声。

### 4.3 激励组织与解析条件

建议的新序列结构为 `0 → +15 → +A → +15 → 0 → −15 → −A → −15 → 0`，上/下行均用受控输入ramp连接；注册平台hold仍沿用5100ms。它明确把同向15°放在高残差激励之后以检查stop是否已在回程发生。**该序列及A尚未授权执行，不沿用v2 hash**。

A不是thetaStart，也不直接固定为30°；须由修复后planner+guard逐笔影子审计给出可提交输入包络，再选能使实际alpha覆盖候选的最小峰值。若所需峰值只得到BLOCKED，则该候选未暴露，不继续用更大target逼迫闸门。对给定g的start越阈条件为 `|g−tan(thetaAccepted)|≥tan(thetaStart)`，同时全边提交需 `|r|≤tan30°`；二者之间的可用余量是残差余量，不是target角差。

输入ramp速率建议从0.5°/s主档、1.0°/s敏感性档做**待批准的可行性审计**，不修改运行maxTiltOmega。若命令间隔100ms，每次输入分别0.05°/0.1°；仅在accepted逐命令跟随、无积压追赶且|theta|≤30°时，纯参考项每次变化上界约0.0667°/0.1333°的alpha量纲（由sec²theta≤4/3及atan导数≤1）。BLOCKED后的追赶不满足该上界，必须按实际accepted变化重算。实际材料变化另加：

```text
abs(deltaAlpha) <= (180/pi) * abs(deltaG - deltaTanAccepted)
```

故慢输入不能保证bracket必窄；必须由真实decision残差核验≤0.5°，没有实测界就不承诺可采。两速度档的转折若有系统差异，不能合并成一个“静态角”；报告速率依赖或另行设计。主档需要独立满足原34run及全部cell门槛；若要求两档都形成独立确认结果，则各自足额，最低68run/pair，不能把34拆两半声称各自达标。这里只估算方法成本，不新增采集配额或授权。

不规定“每run必产多少bracket”：需先在身份正确且完整安全的新clean pilot中观察上程start/下程stop、四cell和24子cell覆盖，再由manager决定是否值得扩批。

## 5. 统计门槛与结构性不足的处置

v3推荐分支保持每候选pair≥34独立完整run（24fit+10holdout），每组每方向≥5run，每四主cell≥30状态样本/≥10run、每run每cell最多3；每组每方向start/stop各≥20合格事件且≥5run。原24噪声子cell、误判率、10000次run-cluster percentile bootstrap（seed20261019）、CI宽≤2°、fit/holdout差≤1°、holdout区间复现、start/stop CI分离与start上界<30°均不降低。

这些门槛**不是简单多跑34遍必然能满足**。近乎平沙在正倾角时r通常为负，负倾角时r通常为正；某些残差符号×方向×噪声子cell可能结构性缺失。v3预演必须输出可达覆盖矩阵：风险状态是否出现、alpha范围、真事件上下端、噪声层能否定义。若固定输入和现有机制根本不能生成某一要求层，则继续堆run不增加识别性，交manager裁决实验激励/验收适用范围，designer不默默合并cell。

不能用非法run中的“好段”、不同实现身份、不同协议或fit/holdout互挪来补缺。ramp新资格从新预注册起生效；本pilot永久保持诊断/INVALID身份。候选输入不是估计输出，当前thetaStartEstimate与thetaStopEstimate均null。

## 6. 输出规则与manager决策材料

| 情况 | 固定输出 |
|---|---|
| 当前pilot | `RUN_INVALID_HARD_GATE` + `EXECUTION_THRESHOLD_IDENTITY_MISMATCH`（诊断发现）+ `PLATFORM_UNREACHED`；标定整体INCOMPLETE，无阈值点值 |
| 新v3真实状态机尚未证实/关键decision字段缺失 | `UNOBSERVABLE / TELEMETRY_OR_EXECUTION_INCOMPLETE`；不得自动当无start/stop |
| 新v3有完整合法风险暴露，上界受安全约束截断且未穿过候选start | 如manager批准新ramp删失定义，可记 `RIGHT_CENSORED_AT_TAN30`，附实际alpha上界、风险状态及planner原因；不产生点估计，不将缺样本本身当右删失 |
| 已有active风险暴露却无真实stop | `STOP_TRANSITION_UNOBSERVED`，附是否始终active、到平台前已stop、零点事件、限流/guard排除等原因 |
| 任一主cell/原子cell或run门槛不足 | `INCOMPLETE / insufficientEvidence`；不能跨cell合并 |
| 仅stop足额且manager尚未批准分量交付 | 可报告覆盖/可辨识性诊断，整体不通过，不自行发布半套运行参数 |

请求manager裁决的是：**批准方向2进入新协议设计/执行身份整改评审**，保持原搜索窗及所有安全/统计门槛；允许把受控、完整提交的ramp纳入新资格，并明确ramp leg/残差采样时点/分辨率要求。不是请求绕过planner，也不是立即批准新pilot或68run。

路由建议：designer据裁决冻结v3协议及候选估计对象；architect核对真正双角状态机/decision时点/planner事务语义；developer按独立工程任务补齐执行与遥测；tester核验执行身份、相名/完整性、硬安全和新资格过滤；effector在新的manager pilot批准后采集。未达到这些前置，继续换±15/±30平台或扩大旧run数量没有标定意义。

本稿修订的是§8.16的适用假设与识别方向，不覆盖历史结论、不改v2预注册、不更改其隔离纪律。当前推荐有明确可检验条件，但不声称thetaStart或thetaStop已可标定。

## 7. DEVIATION-2/3实测修订：按历史状态区分的正负可达包络

日期：2026-10-20。状态：`IDENTIFIABILITY_REVISED / SHADOW_AUDIT_REQUIRED`。依据 [preverify-P011-c-plus-slip-v1.md](preverify-P011-c-plus-slip-v1.md) §3–5和其两个原始ledger；本节完成包络/归因/负向cell/初态集合四项修订。全部数值为预验证诊断，不转为正式标定样本，不对DEVIATION-1静止验收口径作附带裁决。

### 7.1 负向包络修订：−26.88°必须纳入，但不是所有历史状态的统一上限

| 证据与前史 | 正向实际accepted峰值 | 负向实际accepted峰值 | 可成立的比较 |
|---|---:|---:|---|
| [runA-seven45 ledger](../test/artifacts/p011-preverify-20261020/runA-seven45/ledger.ndjson)：0/+15/0/+45/0/−45/0 | +29.882778° | **−26.884164°** | 同run正负幅值差2.998615°，约3°；负向−45命令没有到达目标 |
| [runB-nine30 ledger](../test/artifacts/p011-preverify-20261020/runB-nine30/ledger.ndjson)：0/+15/0/+30/0/−15/0/−30/0 | +29.885198° | **−29.035283°** | 幅值差0.849915°；负向距−30目标0.964717°，超0.20°平台容差 |

因此，旧“正负共同约29.9°可达”的工作假设撤回；负向审计必须包含**约−26.88°即受限的历史状态**，不能用正向实测上限镜像代替负向。0.965°差值属于runB的−29.035°，不能与runA的−26.884°拼成同一个平台结论。runB本身也说明−26.88°不是所有状态下的负向不可突破常数。

采用记号 `envelope(sign, entryState, inputHistory, dtTrace, build)`，不再写无条件的 `A−=A+`。−26.884°是已观测的较紧压力锚点，不是v3保证可达值或新A默认值；两个run仍属旧输入/旧动力学预验证，不能推定修复后真实双角0.5°/s扫描的包络完全相同。

runB +30的101条rate=0记录只说明按角容差筛选的平台改善；抽到的首次峰值记录仍 `requestRejected=true`。v3主标定池排除planner裁剪/拒绝，故“角度接近目标且静止”不等于该段已经具备v3事件资格。runA安全报告包含588子步豁免，本节不把该预验证安全PASS扩写为v3全程零豁免合格。

### 7.2 归因：起始对称不等于进入正/负高角相位时仍对称

直接逐数组复核，两run第一条stateBefore的bh/bl/mh/ml/suspH/suspL/S/waterTop均与自身左右镜像完全一致，S列间坡度为0。**现有证据不支持“本次reset的初始沉积场原本就偏一侧”。** 但负向在正向搬运/回零之后执行；回零没有reset材料，入段条件并非正向条件的镜像。

| 状态对比（列高单位，L∞差） | runA | runB |
|---|---:|---:|
| 正向高角入段S与自身镜像 | 1.212772 | 1.136555 |
| 负向高角入段S与自身镜像 | 1.231587 | 0.341135 |
| 负向高角入段S与“正向高角入段S的镜像” | **1.427999** | **0.564757** |

“初始沉积形状”应区分**run初始化形状**与**某leg的入段形状**。后者已被前序输入/通道改变，差异有直接账本证据，足以使正负可行角区间不同；两份不同schedule产生不同负向上限也符合路径依赖。不能由此排除材料遍历顺序或guard集成对形状演化的影响，但无需假定guard数学公式有符号错误即可解释当前角度差。

更强的逐步量化解释：令当前固定材料坡度 `g[e]=(S[e+1]−S[e])/dx`、`H=tan30°`。在**暂不再移动材料的同一S**上，全部边合法要求

```text
max_e(g[e]−H) <= tan(theta) <= min_e(g[e]+H)
thetaLo(S) = atan(max_e g[e]−H)
thetaHi(S) = atan(min_e g[e]+H)
```

下面按各高角相位首次达到最大accepted幅值的ledger记录复算，g取该记录stateBefore原始S，dx=1；角度计算转换为度：

| run/sequence | 约束边与关键坡度 | 固定S解析界 | 实测accepted |
|---|---|---:|---:|
| A/1063，正向 | e=57，g_min=−0.0027246620 | thetaHi=29.8827781366° | +29.8827781366° |
| A/1361，负向 | e=122，g_max=+0.0703688026 | thetaLo=−26.8841635743° | −26.8841635743° |
| B/1066，正向 | e=57，g_min=−0.0026684883 | thetaHi=29.8851976609° | +29.8851976609° |
| B/1724，负向 | e=117，g_max=+0.0222359384 | thetaLo=−29.0352825018° | −29.0352825018° |

四个角界均与实测钳位吻合；相应stateAfter的限制坡度也保留同一界（并列限制边身份可能变化）。这证明**当步已有形状的正负坡度极值不平衡足以解释钳位数值**。它不是完整动态可达性证明：后续材料计划的Δg、预算、guard和遍历次序仍会改变下一步包络，不能将固定S公式当作替代planner。

guard闭式式本身有镜像等变性：令 `S'[c]=S[N−1−c]`、theta'=−theta，则 `r'[e]=−r[N−2−e]`；镜像同一搬运增量时k也变号，`|r+kq|≤L`与 `|−r−kq|≤L`等价，合法/违规态的L=H或|r|不变。所以闭式区间求交**没有必然的符号偏好**；实现调用、邻边索引、遍历次序、预算分配或预演/apply不一致仍需单独排查。

交developer/architect的定向排查要求（本任务不改代码、不执行新真机采集）：

1. 固定一次原始guard输入及其严格镜像 `(r,k)→(−r,−k)`，保持q/库存/容量/local/bottom/total一致，比较准入q与限制边映射 `e→N−2−e`。不一致才优先查符号端点交换、k构造和数组位置当物理边号的问题。
2. 对完整planner使用完全镜像的S、各相库存、水位、accepted/target角、历史方向/滤波/active及预算状态，逐笔镜像同一request顺序，定位首次预演/实际grant分歧。只反转tilt而不镜像材料/历史不构成对照。
3. 分离“固定升序遍历的数值顺序偏差”与“guard公式错误”：若单笔镜像相等而全步不同，检查排序、相等时优先边、通道收集/apply次序与边界分支。工程语义可能有顺序偏置，但不能通过放宽邻边guard来恢复对称。
4. 在原初态分别重放正先/负先的镜像输入，仅用于离线归因；对照与原串行历史后再判是否还存在不能由entryState解释的符号偏差。输出首个分歧子步/边的S、r/k/q、预算、thetaRequested/Accepted及原因，不只给最终两个角。

### 7.3 start/−与stop/−：较低accepted角不等于较低alphaMax

负向accepted角为−a时，`r=g+tan(a)`、`alpha=atan(abs(g+tan(a)))`。近乎平沙g=0时alpha=a；**26.884°只覆盖start窗低段约[25°,26.884°]，绝不覆盖整个[25°,30°)**。若g正，可在较小负倾角下达到更大的残差；若g负则可能抵消激励。必须看真实inactive风险态的decision残差，而非只看accepted角或全场最大值。

用runA实测a=26.8841635743°、tan(a)=0.5069814666，对r≥0这一分支，达到候选角β所需局部坡度为 `g≥tan(β)−tan(a)`：

| start候选β（仅覆盖算例） | 所需g下界 | 结论 |
|---|---:|---|
| 25° | −0.0406738084 | 平沙在此负倾角可激励；真实inactive、双侧bracket仍须验证 |
| 28° | +0.0247279651 | 需保留一定正坡度，不能只靠26.88°平沙扫描 |
| 29° | +0.0473275849 | 更依赖历史形状与该边仍处于风险态 |
| 29.9° | +0.0680440372 | 接近当步g_max=0.0703688026，距H安全边界很窄；不保证有可采bracket |
| 30°硬线（非候选） | +0.0703688026 | 正好是限制边；不得将硬线端点拟合成候选或允许越界采样 |

若r<0则另有 `g≤−tan(β)−tan(a)` 分支，须从真实局部坡度核验，不能以绝对值丢掉残差符号层。上述单边存在条件不等于127边全场、全程安全可达条件。

**原始账本已展示“accepted低但局部alpha不低”**：A负高角段的原始edge.r最大alpha约30°，B负高角段也约30°；在上述首次钳位记录上g_max+tan(a)=H。因此不能推断负向accepted少3°必然令全场alphaMax也少3°，也不能以26.88°直接否决28°候选。

但这些最大值不是合格start/−证据：预验证仍含tan4旧请求状态，缺v3所需executedThresholds/decision时点认证；其负高角段旧inactive记录的最大原始alpha，A仅约14.014706°、B约13.778540°。将旧active边上约30°的峰值移给新start风险态，就是跨状态补证。原始edge.r还不一定等于relax实际decisionR，本节只用于几何诊断，不做阈值估计或删失认定。

对stop/−的影响分两种：

- 若修复后真实候选start在低段并已合法起动，平沙参考下从26.884°回零会依次穿过stop窗 `[15°,25°]`，有约1.884°超过最高stop候选的激励余量；应主要在负向回程ramp观察，不等到−15°平台再假定会stop。
- 若28°等start候选在该负向entryState根本没被激励，则没有合法active前史，stop/−随之 `UNOBSERVABLE / NEVER_ACTIVE`。不能手工设active，也不能用正向曾active、旧4°请求态或grant=0代替负向合法历史。若真实active跨相位保留，必须完整追溯其真实起动并按既有v3规则计stop，不据假设补样本。

一般stop轨迹须满足同边连续 `alphaPrev>thetaStop>=alphaNow`，g随材料运动变化，不能用手机角直接当停止角。若正坡度残留使回零后alpha仍高于stop，可能完全没有stop；若在平台边界/零点才发生则按v3排除或neutral纪律处理。

**覆盖裁决：当前影响不能定性为“有限/无影响”。** 低段start有条件可覆盖，高段可能依赖形状也可能不可激励；stop能否识别依赖真实active前史和回程。维持四cell、24子cell及样本/CI门槛；A、B的全场原始alpha峰值均不能解除start/−或stop/−缺证。只有完整合法风险暴露被tan30截断才用v3右删失，不能从−26.88°钳位数单独赋码。

### 7.4 影子审计初态必须按符号和历史分别登记

**需要。** 对称输入幅值不是对称状态证据；v3九相为正先负后，同一个run中的负向leg天然继承正向历史。按以下三个层次登记，不能把负向leg悄悄reset来凑对称：

| 审计层 | 登记内容 | 用途与约束 |
|---|---|---|
| 实际协议完整重放 | 每run初始化X0、全部九相/八leg；记录各leg入段state hash、符号、前史、S/g极值、active、theta状态、dt轨迹、库存/预算、实现身份 | 正式A选择必须基于完整重放；正负覆盖各算，不从正向结果镜像填负向 |
| 符号镜像单步/全步对照 | `(X,+输入)`与严格镜像 `(M(X),−输入)`，包括相数组/历史变量/边身份和request顺序映射 | 离线判定guard/规划等变性；不算真实run，不替换正式协议输入 |
| 历史压力场 | runA式较紧负向entryState（−26.88锚点）、runB式entryState（−29.035锚点）及修复后完整协议实际生成的负向入段状态，全部注明originBuild/完整性 | 旧状态是回归/归因fixture，不能直接作为新双角标定初态或冒充其active；只有完整重建隐状态并登记时才用于动态审计 |

字段建议在既有影子审计实例manifest中增加 `entrySign/historyId/entryStateSha256/mirrorPairId/originBuild/activeProvenance`，覆盖矩阵按 `candidate×rate×sign×entryState/history×dt`分别输出。当前审计输入集合尚待绑定，这些是其必须显式区分的维度；在实际运行影子审计前冻结集合，不能看完结果再只留下负向容易通过的形状。

冻结v3协议仍使用同一个A：可接受峰值集合应为**正负各自完整安全且四cell可覆盖的交集**，而不是由正向A推定负向可用。形式上先保留分侧诊断集合F+、F−，最后还须对同一A完整连续run重放通过，不能把两个不同初态的局部通过拼起来。−26.88锚点应在负向压力审计中体现，但不把A直接固定为26.8°；accepted峰值和输入A仍是两个量。

若交集为空，依v3协议输出 `A_selected=null / EXECUTION_ENVELOPE_UNRESOLVED`，连同缺失start/−、stop/−及噪声层证据交manager。若要使用A+≠A−、改变正负先后或中途reset，应另提激励修订并更新protocol/schedule hash，经manager裁决；本节不擅改已冻结九相、不降低门槛、不授权设备采集。

### 7.5 本轮结论与交接

1. 负向可达性论证已纳入−26.884°实测较紧包络，撤回不加历史条件的正负对称外推；−29.035°/0.965°作为另一独立schedule结果保留。
2. 已有材料形状极值给出的固定S角界精确复现四个实测钳位，**历史入段形状是有定量证据的解释**；run初始形状本身对称。guard符号实现是否还存在附加偏差，按镜像等变对照定位，不先行定罪或宣称无缺陷。
3. 26.88°不能在平沙上覆盖完整start窗，但局部正坡度可把alpha推至近30°；“角低3°→alpha低3°”不成立。真实风险状态与decision bracket仍未证实，不能宣布两负向cell已覆盖或直接输出阈值。
4. designer按本节将符号/历史维度交给影子审计执行方；developer/architect排查镜像调用与遍历集成，tester核验真实decision覆盖。正式协议、闸门、搜索窗和样本门槛不变，观测③继续BLOCKED。
