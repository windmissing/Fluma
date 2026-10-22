# P-011 correction A/B/C/D：预期产出与效果判据预注册稿 v1

日期：2026-10-22。责任：designer。状态：**DESIGN_ONLY / PREREGISTRATION_PENDING_MANAGER / NO_CAPTURE_AUTHORIZATION**。本稿定义采前方法；新增收益/非劣/止损数值须manager批准并绑定manifest后才冻结执行，不授权真机采集，不预设correction转正。

依据：[correction算法§3–10](algorithm-P011-relax-correction-v1.md)、[参数外置方案§6](algorithm-P011-user-threshold-config-v1.md)、[architecture§24.3/§27](architecture-P011.md)、[数据路径成本账本§7](design-P011-obs3-calibration-data-path-v1.md)。本实验是分配策略对照，不是观测③的阈值标定；其run数不代替或降低旧标定的34run/四cell/24层门槛。

## 1. 比较对象与采前冻结

| 组 | 固定算法因素 | 预期回答的问题（不是必达结论） |
|---|---|---|
| A | 双角26/20下原relax，无专用预留 | 当前分配基线；不是旧tan4单角 |
| B | 仅专用预留 | B−A是否减少relax受到的竞争，同时挤占哪些普通通道 |
| C | 预留+e² | C−B是否改善深超角边的资源分配 |
| D | 预留+e²×净贡献 | D−C是否减少邻边抵消，D−A是否产生可用净收益 |

四组同一build（或仅变体开关差异的可审计构建）、同128列/容量、材料总量、初态、输入schedule、渲染档、日志档和其他参数。固定 `relaxMode=dual-user-configured`、26/20、`thresholdSource=factory-recommended`、`thresholdCalibrationId=user-configured`，记录独立thresholdConfigId/configRevision；均为非标定推荐值。A使用原relax路径，B/C/D须显式绑定实验variant与correction路线，不能只改报告标签。

采前manifest必须列：route/build/source/schema/hash、四组开关与实际调用路径、候选资格/请求上界/guard/排序策略hash、唯一变化因素、场景/seed/初态hash、schedule CSV/hash、dt政策、设备/刷新率/温度起始条件、帧/账本时间对齐、分析脚本hash和本稿批准版本。B/C中未规定的公共细节由architect对照既有算法冻结；不得为使B更差而额外改变库存、活跃边资格、几何或预算。

**权重可辨识性检查**：若所有观察时段均 `R_limit=sum(u)`，分配全为 `p=u`，仅换权重可能无效果；若D>0仅一个候选，C/D也可能等价。影子预检须记录饱和比例/候选数，并确认用来比较权重的场景确实能区分分配；若不能，报告 `FACTOR_NOT_EXCITED`，不临场改算法或额度制造差异。

## 2. 现有字段与采前数据缺口

本次只读审阅 `P011NaturalObservation.ets`、`WaterSandField.ets`、`P011RelaxCorrection.ets`，没有运行矩阵。

1. D-1b已提供 `audit.correctionRequested/Granted/Rejected/StalledCount`，**单位是候选笔数**：requested=进入slot的候选数，granted=实际accepted>引擎EPS的笔数，rejected=其余笔数，requested=granted+rejected。不能拿它们当体积/通量，也不能按requested−granted解释guard拒绝量。
2. 现有实际搬运字段为 `slip[].allowed/heavy/light`、`audit.relaxAppliedTotal`，普通通道为 `settle/percolate/lift.allowed[]`、`bottomCreep[].allowed`；`budgets`含local/bottom/total账，尚无完整公开correction金额账。记录的 `slip[].dAfter/rAfter` 由整步stateAfter生成，**不是单笔relax完成时的三边快照**。
3. `stateBefore/stateAfter.S`或bh+bl+mh+ml可给整步形状，但缺少明确的relax槽位前后快照、候选零产出原因和完整预留瀑布。只有四个D-1b计数不足以证明ΔE_relax或结构性停滞。
4. 源码核对发现 `computeCorrectionReservation` 使用hysteresis.slopeStop，而 `applyCorrectionSlot` 创建的params仍写 `tan(params.angleDeg)`。**采前须由developer/architect确认并使实际reserve/apply/测量h均等于tan20，或报告执行身份不一致。** 本稿不修改该代码，不以四字段已落账推断执行链已满足此条件。

需要冻结的**分析输入契约**如下。可增加只读遥测，也可由经过位精确闭合验证的离线逐writer重放生成；字段名为拟议分析schema，不能冒称当前ledger已有：

| 拟议输入 | 必须包含/用途 |
|---|---|
| `relaxTrace` | run/attempt/substep、slot前S、slot后S、固定acceptedTilt、h、逐笔序号/edge/donor/receiver/q实际值、笔前D/K及三边残差、资格/取消原因 |
| `reservationTrace` | 原始需求、可分配u/p、R_limit、预留/used/remaining、库存/容量/贡献/预算/guard各层拒绝及unused reservation，金额与笔数分开 |
| `demandTrace` | **候选过滤前**的active与e>0风险边；D≤0/guard/库存等原因，包括无候选整步，避免空候选漏报停滞 |
| `ordinaryBudgetTrace` | 各普通writer原请求/实际金额/限制原因；将来若做同状态反事实，需pre-reservation材料、active、dt及相同accepted参考角 |
| `frameTrace` | 单调时间戳、frameId、呈现间隔/物理耗时、run与phase映射、录像/截图索引；不能用physicsDt当帧时 |

逐writer重放必须按实际操作顺序重现最终stateAfter和金额账，按现有数值契约闭合；顺序/退款/未提交attempt不全时为 `MEASUREMENT_BLOCKED`。不得用整步E变化、decision collect快照或slip最终dAfter替代slot增量。materialGuard当前channel标签不一定细分writer，需与逐笔序号关联，无法关联时不伪造通道归因。

新B/C/D正式run必须有四个D-1b字段（全零也要记）；旧run缺字段容忍仅为历史解析，不适用于本矩阵。A的correction计数标 `NOT_APPLICABLE`，其relax仍完整记录；不把A缺correction字段当作零停滞优势。

## 3. 各组统一测量口径与数据源

所有组都提取下表，时间窗口/空间域完全相同。N=128、127条边、dx=1，`S=bh+bl+mh+ml`，h=tan20，H=tan30。无ROI挑边；若需展示局部图，仍保留全边指标。单位：E为无量纲残差平方和，q为模型列高搬运量，时间秒，帧时毫秒。

| 指标 | 提取口径 | 原始字段/分析输入 |
|---|---|---|
| E轨迹 | `r_e=(S[e+1]−S[e])/dx−tan(thetaAccepted)`；`e_e=max(0,abs(r_e)−h)`；`E=0.5Σe_e²`，**不按active遮罩**，防止清active假降E | stateAfter.S、context.thetaAccepted、executedThresholds.slopeStop；slot测量用relaxTrace |
| maxResidual | 每快照全127边 `max abs(r)`，同时报告 `atan(maxResidual)`度数；各phase峰值/末值/时间P95 | 同上；硬检查另取所有writer提交快照，不仅整步末尾 |
| ΔE_relax | 每slot在同一accepted角/h下 `E_preSlot−E_postSlot`；允许A/B/C报告负值。逐笔D组同时验证算法§3下降界，不把参考角变化当贡献 | relaxTrace；完整重放仅在§2闭合通过后可替代 |
| 收敛效率/速率 | `eta=ΣΔE_relax/Σq_relax`；`R_relax=ΣΔE_relax/T_window`，另报负贡献笔数/金额；分母0写N/A，不能补epsilon算百分比 | relaxTrace、Σslip.allowed、audit.relaxAppliedTotal、clock |
| 累积残差负担 | `J_E=∫E(t)dt/T_window`，按实际单调时间分段常值积分；每phase及整段均报告。整段J_E是主效果量 | 已提交状态+完整attempt/wall-clock轨迹；hold期间保持真实当前场，不删除时间 |
| ordinary通道代价 | 每通道Q_req/Q_used/Q_clip和 `loss=1−Q_used/Q_req`；相对配对A报loss增量（百分点）及used减少量。总loss以同单位金额汇总，保留逐通道 | settle/percolate/lift、bottomCreep、普通预算trace；按实际扣total金额规范，不把percolation双向交换重复算两次 |
| 预留浪费/guard截断 | `U=ΣunusedReservation/ΣR_limit`，guard拒绝金额/送guard金额另列；瀑布层互斥，不把局部与total扣账相加成新容量 | reservationTrace、materialGuard[].rejected/clipped、budgets |
| D-1b候选停滞率 | `S_candidate=ΣcorrectionStalledCount/ΣcorrectionRequested`；分母0=N/A。NO_POSITIVE_LOCAL_CONTRIBUTION/DIRECTION_REVERSED/GUARD_REJECTED是当前计数语义 | audit四字段；逐原因由trace补齐 |
| 需求时间停滞率 | `S_time=风险存在且q_relax=0的持续时间 / active且e>0风险存在时间`；另记最长连续时长，区分D≤0/预算/guard/库存/方向/无候选。A同口径称relax停滞，B/C/D称correction需求停滞 | demandTrace、slot金额、clock；不因reserve过滤掉风险边而缩小分母 |
| 输入可达代价 | BLOCKED/hold/reject的时间率、`J_tilt=mean(abs(targetTilt−acceptedTilt))`；同命令轨迹比较 | context target/requested/accepted、commitStatus、attempt时间；不能因更少接受倾角而冒称E改善 |
| raster/局部可见性 | 固定相机/viewport/渲染/颜色，取全宽整体沙面profile，报告首末profile RMS/max差、OLS/R²、帧间跳变；完整视频供盲看连续性/局部响应 | raster固定提取脚本+frameTrace；不是绿色heavy质心，非零grant不等于可见 |
| FPS/帧时 | 呈现间隔P50/P95、`FPS_window=帧数/实际秒数`、物理耗时P50/P95，含输入/hold期间；每run先统计后配对 | frameTrace及原始WS_FRAME/系统呈现trace；禁止把P95帧时倒数命名FPS P95 |
| 安全/记账 | 全writer guard、最终全边tan30、preview/apply一致、有限/库存/容量、H/L各自守恒、local双端/total/bottom/correction金额退款闭合、笔数闭合 | materialGuard、stateBefore/After、audit、budgets、reservationTrace及既有正式checker |

D-1b全零但demandTrace显示active且e>0，必须进入需求停滞分析，不能判“没有需求”；库存不足与不存在有效风险状态也应分开。`CORRECTION_STALLED`是机制限制诊断，不天然等于硬安全失败。

系统整步能量归因必须分项：`ΔE_total=ΔE_reference+ΣΔE_writer`，固定材料先换参考角得到reference项，随后各writer按实际顺序计算，relax只取其slot。每项可正可负，整步E不承诺单调。该闭合不能由四个候选计数推出。

**比较层次**：系统层比较同命令输入下每组真实轨迹（accepted可不同），必须同时受输入可达非劣门约束；机制层可在同一冻结pre-reservation状态/active/dt/accepted角做离线四策略反事实，独立标记 `source=shadow`，只用于归因，不能代替真机整体效果。系统层Q_used减少是“实际通道代价”，并非同状态纯预算因果；不得混称。

## 4. 场景、窗口与配对设计

保持§9已有均匀/热点/相邻冲突类型，拟定三个采前登记strata：

- **G1竞争场（主效果）**：合法初态、普通通道与relax同时有实际需求、预留可能起作用。需影子证明有足够active风险时间；不能通过减少其他通道需求优化D。
- **G2非均匀热点/邻边约束场（稳健性）**：合法全边初态、能区分权重/邻边贡献，报告限制边和局部可见响应。不以任意已违规初态作为用户正常收益证明。
- **G0均匀/相邻无正贡献控制**：检查无贡献/受guard限制时的诚实停滞；预期可能没有提升。归为机制边界控制，不能混入主组稀释停滞率，也不能因其不提速判算法必败。

每个场景的数组/seed、active初始化（正常run reset，不人工置active）、预算/参数必须在pilot前冻结。保持真实writer启停规则；用测试fixture定义初态属于该矩阵的机制实验，不迁入自然观测③标定池。

建议复用九相输入模板 `0→+15→+A→+15→0→−15→−A→−15→0`，hold各5100ms，ramp 0.5°/s、100ms命令步距。**A不是本稿直接批准的18.7或30**：须影子核验四组共同安全输入包络、G1/G2有效风险与因素可辨识性，再冻结最小覆盖A和CSV；选择依资格/覆盖而非D收益高低。无法找到共同可用输入时停止本矩阵排期，不能跨组使用不同schedule。

一run执行完整模板；主窗口从首命令到末H8结束（含所有ramp/hold），禁止删掉BLOCKED/无活动期。每个非零平台另报告完整5.1s及最后1s；截图按平台开始0/1.0/2.55/5.1s就近首个呈现帧并记录时间差，视频全程保留。不得按运行后出现的最好相位重分主终点。

同一block含A/B/C/D各一run，同初态/seed、配置和命令。四组按平衡Williams顺序 `ABDC / BCAD / CDBA / DACB` 轮换，正式每stratum12block正好每序列3次；按seed=20261022打乱序列块的执行先后并冻结manifest。每run正常冷重置；热状态进入条件各组相同，额外冷却时间入成本，不删掉慢组。硬件实际dt差异如实保留；同dt的因果机制验证单独离线做，不能声称真机逐步dt天然相同。

统计单位是block/run，非127条边或子步。G1/G2分开判定，不因某层好看合并；G0只作控制。pilot永久不进正式统计，34run阈值标定分组不适用于本实验。

## 5. “有收益”的数值判据（提案，待manager批准）

### 5.1 主结论与固定门槛

本轮唯一主比较为 **D vs A，G1整段J_E下降**。其余B−A/C−B/D−C解释机制，不做多项择优后改称主成功。预注册建议：

| 条件 | 提议冻结线 | 论证/性质 |
|---|---|---|
| 主效果G1 | 每block `gain=1−J_E,D/J_E,A`；12block中位gain≥10%，配对bootstrap95%CI下界>0 | 10%为最低有用改进，20%列目标档、≥30%列较强档；不是事后在10–30间选线。§8历史竞争上界1.2836只说明不宜预期数量级收益，不证明本次能达10% |
| G2稳健性 | 同一gain中位≥0，95%CI下界≥−5% | 防止只在竞争场有效、在邻边受限场明显倒退；5%是拟议效果非劣幅度，不是安全epsilon |
| 归因支持 | 报告R_relax与eta的配对变化；若声称“修正速率提高”，其G1中位相对提升须≥10%、95%CI下界>0 | 总J_E改善不能自动归因于relax速率；分母A≤测量分辨率时不报百分比，结论限系统效果 |
| 普通通道代价 | 相对A，总ordinary loss增量≤5个百分点；任一有需求通道≤10个百分点；G1/G2分别满足 | 5pp总量、10pp局部是拟议资源非劣门，限制用沉降/底层停工换relax改善；必须同时报告绝对金额及used差 |
| 输入可达代价 | hold/BLOCKED/reject时间率相对A增加≤1个百分点；J_tilt增加≤0.1° | 拟议非劣门，防止D用更少接受倾角“省能量”；不放宽任何提交安全规则 |
| 帧时 | 配对run P95呈现帧时比D/A的中位≤1.05，另逐项满足当前适用性能硬门槛 | 5%为本实验新增开销预算提案，不替代原FPS/耗时门槛，也不移植P-012的30fps标准 |
| raster/主观 | 局部响应有完整视频证据；对应P-011既有适用软项≥6/10、不退化到不及格 | 不把P-012的3/5量表套用到P-011；不新增“全宽30°达标”或heavy≥2px要求来混淆矩阵目的 |
| 硬安全与D局部证明 | 既有硬闸门全通过；D的实际正q逐笔满足固定h下的下降界 | 零容忍逻辑不变，数值处理只用现行批准契约；任何软收益不能抵消硬失败 |

普通通道、输入可达和帧时的非劣线需对配对量输出95%CI：用于总收益放行时，one-sided对应端（取双侧95%CI的上界）也须不超过上表上限；否则 `INCONCLUSIVE_COST`，不能只拿中位数盖住不确定性。既有主观维度与新G1/G2的适用映射由effector在采前冻结，若无可用映射则效果证据尚未就绪，不能随意新增评分量表或跑后挑项。

**阈值理由的局限**：10%收益/5pp挤占等是工程取舍提案，不是已有数据推得的功效保证。系统需要用户可见收益且不靠大幅牺牲其他通道，本稿选择中等幅度作最小有用改进。manager可在采前批准另一组数并升revision；采后不能为结果调整。G0不受10%收益线约束，仍受安全、记账、停滞真实性要求约束。

### 5.2 统计、零分母与输出状态

每个stratum独立，采用10000次**paired-block percentile bootstrap**，seed=20261022，按block整体有放回抽取，A/B/C/D配对保留；每次计算12个配对指标的中位数，95%CI取2.5/97.5百分位（线性插值）。G0仅3block，描述性报告，不借bootstrap声称泛化。

E、q、时间的数值分辨率由采前同状态重放误差/时钟分辨率登记，不能临场加一个有利epsilon。若任一主比较block的A整段J_E为0或低于冻结分辨率，主百分比结论为 `INCOMPLETE_EXCITATION`，报告绝对差，不能丢该block或把分母改成小常数。ordinary某通道Q_req=0时loss=N/A，报告金额；D新增需求但A无需求时不构造差百分比，需单独说明代价，不能自动算非劣通过。

B/C辅助比较的CI为描述性，不能以多个比较中任意一个显著作为D主成功；若将来需要给B/C正式转正，各自另行预注册主假设。12block是固定首轮估算，并无先验方差支持“已达到80%功效”；CI不足时输出INCONCLUSIVE，不连续加run直到过线。任何补样本需manager采前追加计划或新revision，新确认样本与已见数据区分。

输出状态分开：`SAFETY_PASS/FAIL`、`MEASUREMENT_READY/BLOCKED`、`BENEFIT_SUPPORTED/NOT_SUPPORTED/INCONCLUSIVE`、`COST_ACCEPTABLE/INCONCLUSIVE/EXCESSIVE`。只有主收益+G2稳健性+代价/帧时/软项+全部硬门槛满足，才能提交“26/20、注册场景内有可用净收益”建议；转正仍由manager裁决。

## 6. run数与设备成本预估

| 层 | 每组run数 | 四组合计 | 用途/排期条件 |
|---|---:|---:|---|
| pilot：G1/G2/G0各1block | 3 | 12 | 仅核验数据闭合、因素身份、安全与激励；不以收益择样 |
| 正式G1 | 12 | 48 | 主比较，4平衡顺序各3次 |
| 正式G2 | 12 | 48 | 稳健性，单独统计 |
| G0控制 | 3 | 12 | 停滞/无贡献边界，描述性 |
| 合计 | **30** | **120** | 其中正式/控制108，pilot12；没有34run标定含义 |

沿用成本账本形式：`T_run=9×5.1+4A/0.5`秒，原扩批平均附加开销约55.09s/run。仅以A=18.7°作时长情景（不是批准峰值），T_run=195.5s，设备占用约250.59s/run=4.18min。

| 批次 | 纯schedule时长 | 加既有平均开销 | 加20%排期缓冲 |
|---|---:|---:|---:|
| 12run pilot | 39.10min | 50.12min | 60.14min |
| G1或G2各48run | 156.40min | 200.47min | 240.56min |
| 12run G0控制 | 39.10min | 50.12min | 60.14min |
| 108run正式+控制 | 351.90min | 451.06min | 541.27min |
| 总120run | 391.00min | **501.18min≈8.35h** | **601.41min≈10.02h** |

每组30run约125.29min（含平均开销），同场景/同组渲染与记录负担一致。真实A、日志IO、冷却、额外frameTrace开销和场景重置确定后，effector按冻结CSV复算，不把8.35h作为保证上限；大账本离线分析另计。不沿用用户已暂停的1B双循环245.5s模板来混算。本次只估预算，manager可以仅批前12run pilot，不能据本表自动扩批。

## 7. 失败、中止与止损

1. **任一组硬失败立即停该实验批**：已提交子步tan30、守恒/预算/库存/容量/finite失败，或guard旁路、preview/apply不一致。隔离该run并保留同block其余数据用于诊断，停止后续矩阵采集等待修复/manager重排；不能删失败run后重试到全绿。正常未提交BLOCKED本身不是硬越界，但进入可达/停滞代价。
2. **身份/测量失败立即停**：pair/source/route变动、reserve与apply的h不同、D-1b计数不闭合、金额退款不闭合、slot重放不匹配、缺关键attempt或帧时间。输出MEASUREMENT_BLOCKED，不填0、不发效果结论。必要只读采集补齐后更换build/hash，旧新批次不混。
3. **结构性停滞信号（拟议止损线）**：在G1或G2，active且e>0的需求持续≥一个完整5.1s保持段，relax实际q全为0；完整trace证明原因持续为D≤0、guard无准入或库存/容量不可用，且相同初态/命令离线重放复现。此时标 `STRUCTURAL_STALL_SIGNAL`，暂停该stratum扩批并交architect/designer归因；一个轨迹不能证明全约束类无解。G0同现象是预期控制结果，只验证诚实上报。
4. **高停滞止损（拟议）**：首个完整pilot block内B/C/D任一组在G1/G2的S_time≥80%，且相对A增加≥20个百分点，停止自动扩批申请，先检查风险集合与预留/guard原因。80%意味着大部分有需求时间无兑现，20pp要求确有相对恶化；是试验投入止损线，不是物理硬阈值。实际风险时间为0则INCOMPLETE_EXCITATION，不判S_time=0通过。
5. **性能或普通通道明显受损**：任何既有性能/效果硬门槛失败立即停；新增§5非劣门不满足时记代价过高/不确定，不据大grant强行升级。正式固定批未满足收益只报告NOT_SUPPORTED/INCONCLUSIVE，不改指标、不择优相位、不无限追加run。

预演候选角失败但未提交的E/ΔE单列诊断，不能算进实际收益；计时仍覆盖其消耗。未使用的预留不能作为已减少的能量，零候选也不能代替停滞原因。

## 8. 与用户可调pair、交付及授权的关系

矩阵结论严格限定 **26/20 factory-recommended、注册材料/几何/初态/输入速率/构建及渲染档**。用户改pair或换材料/容器会改变active集合、h、E含义和预算竞争；不得外推效果、停滞率或帧时代价，更不能称26/20已完成材料标定。安全守恒要求仍跨配置维持；新pair需要另开配置层/对照计划，不能混入本矩阵。

最终交付至少包括：冻结manifest及各组真实执行身份、逐run/phase指标表、A/B/C/D配对差与CI、全安全结果、风险/停滞原因交集、普通通道代价、E归因闭合、原始frameTrace/录像与固定关键帧、失败/缺失run清单、结论适用域。tester负责客观提取/闭合与统计，effector负责相同录像的盲看和适用软项，architect核对因素与数据契约，manager批准新阈值/样本计划及采集排期。

本稿完成的是采前定义。§2字段和h一致性、四variant绑定、场景/schedule/hash及阈值批准均是**执行前置**，不是本次设计任务的阻断；本次不改代码、测试用例或采集脚本，不宣称实验已可启动或correction已转正。
