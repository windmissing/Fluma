# T353 · 停源冻结首次归因、倾角与入口分流 v1

**2026-11-06 续裁见 §6。** T358 已裁定旧窗口未保存。T360 原档确认新包在所覆盖窗口继续步进，并补足了采样边界。旧冻结的首处分歧仍待复现。

2026-11-05，architect，不使用真机。消费 T350 原卡评论、正式报告、`test/reports/t350/` 全部六份日志、T346/T347/T348/T349 报告和对应源码。T347 签包本地 SHA256 实核为 `0195c9359f6cf397cd5bf76afc17a7702e459da93a23bc47b862ff3252df445d`，与 T350 声明一致；源码基准为报告登记的 `41c5351`，使用 `git show` 读历史版本，不将 T351 在途修改冒充旧签包源码。未运行设备或重跑测试，未改产品代码。

## 1. 裁定和真实前置

| 问题 | 首次归因 | 接手和前置 |
|---|---|---|
| V2 UP 后悬空冻结 | **EVIDENCE_INCOMPLETE**。报告及原卡有描述，但所列原始资产不含 frame480/600/720，不能判物理未推进、发布旧态或采集错误的唯一原因 | tester 先补原 run 原始片段；拿不到则按 §4 最小有界补采，随后回 architect |
| “仅注入事务驱动 sink/LAND” | 当前源码及 T346 既有离线反例不支持这个通用解释；不按此假说派修复 | developer 不新增注入耦合，也不以无 request 跳过 motion |
| target89 / effective30 | 空场 tan30 可行域内的接受角收敛，现有证据不构成平滑/限幅缺陷；D 日志来自 V1 EmptySandDemo，非 V2 同 run | tester 修改预期，manager 登记；如要求空场可接受89°，需 designer 先修可行域合同 |
| 真摇晃体验 | 未执行，**NOT_RUN**；静置 shakeInput=0 不能证明摇晃功能失败或通过 | effector 最小真实手持需求见 §5，可消费 T355，不能软件伪手持 |
| V1 默认不见粒子 / 显式粒子 code100 | 路线/默认参数现状和既有 V1 数值域问题；触摸接线已存在 | manager 结合 T342/T350/T352 作路线决策，不重开 T348、不擅切 V2 |

本卡已完成可用证据内的源码追踪、倾角/入口裁定与补证合同；**尚未完成的是 V2 停源冻结的唯一根因定位及具体产品修复裁定**。因关键原始窗口缺失，本人行按规则登记“看评论”，不把缺证当产品 PASS，也不将整轮归因交 tester。

## 2. T350 原始证据核对

`hilog_C_v2.txt` 的 V2 run 为 `p011-run-1793879994174-3`、pid5352。`T347STEP` 仅三条：line3108 frame120/commit237/live0；line3758 frame240/commit477/live0；line6309 frame360/commit726/live126/pYmin114.5112901585653/pYmean128.0928537905885/motion126/mh0。末条已保存场景回执为 line7317，20:00:54.699、frame406/commit820、CURRENT、request==actual。检索 `test/reports/t350/` 全部 txt，未发现 frame480/600/720 对应 `T347STEP`，亦未发现报告所述 pYmin54.646947334308884/pYmean88.04859555536524 的原始行。`hilog_C_fgswitch.txt` 保存的结束时间为19:59:38，不能填补20:00:54之后的窗口。T350 原卡评论复述了摘要，未提供缺失片段。扩查 `.codex/local-test` 日志也未找回该窗口。

因此 frame480→720、commit966→1446、8模拟秒及逐位冻结目前是**待回补的报告声明**，不是本次核到的同 run 原始序列；不能凭本报告撤销观察者的发现，也不能当作已完成复现。R5 最后 dry.request 时间19:57:32.524来自 `hilog_B2.txt` 的 V1 run `p011-run-1793879756866-3`、pid2542，不能替代 C run 的 UP/停源时点。R8 target89/effective30来自 `hilog_D_shake.txt` 的 V1 `empty_sand_demo`、pid8130/run `p011-run-1793880401360-3`，不能与 C 的粒子位置拼成同一轨迹。

旧页面观测在每120个完成复制/绘制调用的页面帧，调用 `DerivedPublishedSlotV2.exportPayloadCold()` 复制**已发布 slot**，统计所有 live 的最小/平均 pY 和 `abs(pVy)>0` 的数量。它不是 canonical 独立读数；`motion` 不是 `plan.motionCount`、不是 writer 次数、更不是本帧位移证明。只知道 min/mean 相同不能证明全部粒子 F64 位型相同；不同粒子的位移抵消、live 集变化也可能保留汇总。CURRENT 只证明当前请求 token 被新管线消费成功，不能独立证明该 token 的物理内容有变化。页面 serial 和 controller frameSerial 也不可凭整数相同强行对齐，须以 run/epoch/commit/frameToken 绑定。

## 3. 41c5351 实际步进链

`P051DerivedDemoV2.stop()` 仅 `held=false` 和停止源 clock；`tick()` 的 `owner.step(dt)` 在 held 分支之外。stale sensor 输入停止源也不取消 step。页面 active=false 时不进入 tick；恢复会重新设置时间基准，首帧 dt=0；正常活跃帧为 `min(1/30,墙钟间隔)`。`WaterSandField.stepWithTiltTrace` 的 dormant 快路明确排除 liveCount>0 的活动粒子路线。Q=1或2子步的每次成功提交才走 STEP publication；但有 commit 增长仍不能仅靠 commit 差认定实际累计dt，外源也增加 commit。

每子步 `planAndCommitTilt` 之后进行权威 collect，`saveMotionSnapshot` 调 `particleBinding.beginCollect`，后者 `beginStepHot→collect planA/planB→比较`；`restoreMotionSnapshot` 恢复 field，不以 held 为门。随后 `applyMotionChannels→particleBinding.apply→applyParticlePlanIntoBody`。该函数先循环 requestCount 调 sink，再**独立循环 motionCount**，将 `plan.motionX/Y/Vx/Vy` 写入 patchScratch，经 `copyPoolDiagnosticHot(...,true)` 写回池，再 APPLY_TAIL audit。requestCount=0不跳过 motion 写回；LAND 是 contact 请求经 M2Sink 转床，与无接触时的纯运动写回不同。

已有 T346 harness 在三次初始 inject 后连续600次 step(1/60)，中途不注入；`test/reports/t346/harness-run.json` 前三窗 requestCount均0、motionCount30，pYmin215.01833333333335→200.23666666666668→175.65499999999997。它足以反驳“通用实现只在注入时推进”的解释，**不能排除 T350 特定输入/设备状态下的新缺陷**。T345 旧稿把离线不复现直接等价为设备层唯一原因的二分过强；不同输入、角度、生命周期、采样来源未等价时不能作此推断。本次不沿用该二分。

失败路径会 rollbackStep/restoreBackup；非可继续错误冻结 field，facade 有 derived fault 时 stopped，model step返回false时也不走正常发布/绘制。正常 publication 的 `fillDerivedPublicationV2→copyStateTo` 复制当前池，经 exact70边界校验和 `matchesAuthority` 逐字节比较，`commitPrepared` 切 active bank；页面再复制 active bank。`exportPayloadCold()`每次新分配并复制当前 slot，源码没有“首次导出永久缓存”的分支。以上排除了直接可见的 held 门和固定导出缓存，但旧日志没有各切点状态，不能证明特定窗未发生空计划、预测未变、motion写回又被恢复或采样错位。

T347 注入期间 pY下降、LAND和床量增长保留。其后床量不变只说明已沉积量未变；live仍非零且有非零pVy，不能由此推导全部粒子已着陆静置。T350 的冻结声明足以使该静置解释需要重审；缺失窗口补齐前不宣唯一新机理，也不泛用 T346 的“模拟时间不足”解释全部冻结。

## 4. 最小补证合同及后续工程接口

先由 tester 找回 C run 从最后一次真实 UP/CANCEL 前一采样点至报告frame720的原始日志、文件路径/hash、pid/run/epoch、Want、包SHA；给出 DOWN/MOVE/UP 时点与采样行号。不得把 B run 的停源日志填到 C。若找不到，明确 `RAW_WINDOW_NOT_SAVED`，新补采独立命名 run，不冒旧证据重建。原卡报告 min/mean 的“逐位”措辞改为“所报汇总字符串相同”，除非有逐 ID raw bits。

确需新签包诊断时，developer 在独立功能诊断卡按以下接口增加固定容量标量/少量粒子位型收据，tester 执行设备窗口；不要求 T351 自带功能修复。每帧绑定 run/epoch/page frame/commit-before-after、requested/actual token、sourceEpoch/admission count、held/active/previousNs/currentNs/rawDelta/clippedDt、step(ok/stepped/dormant/Q/substepDt)、frozen/lastError/derived status。每成功子步附 attempt/substep/acceptedTilt、plan motionCount/requestCount、apply次数、成功写回数、rollback次数/原因，以及同一批稳定ID的 pY/pVy F64 bits 在 canonical-before→predicted→canonical-after-apply→canonical-prepublication→published-bank→page-slot 的切点读数。优先3个悬空ID，live集合变化记录ID失效并选新锚点，不把旧槽当旧粒子；每秒至多一条/固定环冷取，禁止全池逐帧JSON。counter/切点记录有上限和丢样标志，诊断不修改状态。

最小序列为同入口、同包、固定直立可接受角：真实DOWN/MOVE产生悬空粒子→UP停止新准入→持续前台累计至少8个**实际成功物理秒**或先全落床，记录elapsed与各次Q/dt求和。若旧包在有限墙钟上限内不能达到该窗，报观察不足并保留进展，不改dt强行加速。补充一次源停后继续setTilt与一次前后台恢复即可定位是否输入/生命周期相关；摇晃另交effector。不需要重复所有入口或巨型冷档。

返回 architect 的判别表：

| 首个分歧 | 路由与精确修订范围 |
|---|---|
| active/dt/Q与成功elapsed不成立 | 工程：页面时钟/生命周期或step调度；先修对应接口，再developer，不扩大重力参数 |
| dt>0、live悬空，但collect motion计划为空或预测未变 | 工程：beginCollect输入、medium、accepted角与plan生命周期，核identity/consumed，不能以注入强迫唤醒掩盖 |
| predicted变化，after-apply未写或prepublication被恢复 | 工程：Hot motion transfer/事务rollback/preview恢复边界；修提交与错误传播，不把空成功记正常 |
| canonical变化、bank/slot不变 | 工程：DerivedCommit/fill/copy/封签和buffer绑定；保exact70、逐边界检测与事务，禁止旧内容换新token |
| slot变化、图像不变或actual旧token | 渲染/采集：核pack投影/实际token/截图时间，仍仅XComponent+shader；失败FAULT或完整旧token显式STALE |
| 仅汇总相同，逐ID变化 | 用例/报告口径问题交manager登记；不派物理缺陷修复 |

没有以上首次分歧证据前，不指定产品删除/增加哪段writer。若定位暴露“无源时冻结”确为设计行为，则与持续重力的既有合同冲突，architect列不变量交designer；不能将其标预期NOT_IMPLEMENTED。新能力（如突破现有倾角可行域）才按新设计/NOT_IMPLEMENTED登记。

## 5. 倾角、手持与入口

重力适配器对sensor方向atan2映射限制在±89°，有alpha=0.6低通；`setTilt` 接受目标亦限制±89°。`P011TiltPlanningPass` 对每边使用 `g0+Δg ± tan30` 的允许斜率区间，再与角速度区间、朝target的有向区间求交，取atan得到accepted角。空场g0=Δg=0时自然得到±30°（浮点回转为29.999999999999996）。这不是全场固定±30°输入上限；有沙面梯度时可行区间由真实边决定。故D run target89、effective30符合空场合同，并非两者应恒等或平滑不收敛。绘制重力方向消费publication.effectiveTiltDeg；后续核同commit accepted角、publication角和渲染方向，不用target替代。扩大支持域应designer先处理残差硬界与空容器姿态语义，不由developer松tan30。

effector 使用现有真实手持基线 T355：同run/入口/包，直立→小幅左右倾→越过摇晃阈值→停止→有悬空沙时UP，记录真实物理动作、sensor时间/target/accepted/shakeMagnitude/shakeInput、画面方向与跟手；static静置、按钮或合成传感器不签“真实摇晃”。无法移动设备/无人手时如实交具体前置，不把静置0值判坏，也不以软件假动作补齐。

T348 已证触摸→干源注入存在，不能移植湿链 `stageGestureSandAndFeed` 到干场。T350 A 默认是FLUID/rendermode1、粒子请求默认关闭，属于STRUCTURAL_ZERO_OFF；B显式粒子路线有出生但复现code100，T342 exact70 V2只修V2链，不给V1补了隐含修复。T342 S1-S4和T347窄功能证据可供切换裁决，但T350停源争议、T352性能/真实触摸续证仍未闭合；由manager作默认入口/生产切换决策，不自行修旧V1或切V2。

T351继续T349的性能工程，T354/T357继续独立计数/位型差分；本卡只写本工程稿，与其WaterSandField/Hot/facade/page/renderer共享hunk不冲突。新诊断优先复用其固定收据，但要保原签包复现身份与功能对照，不能把速度变好当冻结自动修复。后续开发需本角色编译/非机回归，设备执行另卡；持续推进至当前范围完成或具体阻断，清单仅改本人行状态，渲染前背景均XComponent+shader。

## 6. 2026-11-06 续裁：T360 新包步进有证，旧冻结仍待定位

消费 T358 `RAW_WINDOW_NOT_SAVED` 裁定、T359 诊断源码与交付、T360 正式报告和原卡回执。以 `test/reports/t360/v2-budget-t360-arm{1,2,2b}.json` 自带 fields/stages 解码，同 `hilog_arm1/2.txt` 对齐 run/epoch/commit/token。签包本地 SHA256 实核 `cd9fbef5143100bc28f4c2d49d36a8af9bb876e18e67208e8acd8da614358316`，与 T350 `0195c935…` 是不同包。三份冷档 SHA256 分别为 `a8a81138c882de739ed9a9e7d78446a0edf54235cd389ceb1e0edba08903950a`、`2b69a6ac2cdefe8a5453fd6ed31d7255bee8fad7301920becadbc832f3d9ee31`、`f9d2d09a5e2657e7cd7561b0b868a3a825d34e129340bcee6aa1898de2ccd27f`。本次只离线核原档与实现。

### 6.1 原档裁定和对报告措辞的修正

三份 cuts 都是同一 run `p011-run-1793926161426-3`、epoch 2 的 256 行最近窗口。arm1/arm2/arm2b 各只含 callback 437–441、1177–1180、1459–1463，且首帧可能只剩部分切点。跟踪集合三档全为 ID 1/2/3，另有 ID 0 命令回执，覆盖不会随“多看90秒”而扩大。T359 `StopCuts.predicted()` 只搜 `plan.motionParticleId[]`，未采 DEPOSIT/CROSSING 请求的预测和 grant。所有三档 stage1 对 ID 1–3 都是 `present=0`。stage2 是应用后池读数，且收到了 planner preview 的写回，需用 attempt/substep 及最终 bank 判断权威提交。`physicalSlot` 在预测行实为 motion 行位置，跨切点统一按 ID，而非槽号对比。

| 资产 | 原始结果 | 裁定 |
|---|---|---|
| arm1 | 末段 callback 437–441 均 held=0、STEP OK、Q=2、dt=1/60、fault=0，成功 elapsed 14.500→14.633 s。子步收据显示较早 live 94→83，最终被追踪 ID 1–3 在 bank/slot 仍 live，y≈1.1547015、vy=0。 | 停源窗口内有成功步进和活池减少。live 减少单独无法区分全额 LAND、合并等退役原因。仅凭 y/vy 无法判定三 ID 已计入床库存。报告“441→1050 的20.3物理秒”不是 arm1 cuts 的 UP 后实测区间，不能用帧差替代 UP 锚点/累计成功 elapsed。 |
| arm2 | callback1179 stage8 held=1，commit2367，sourceEpoch/admissionCount=11，elapsed39.233333 s。下一帧 canonicalBefore held=0，仍为2367，随后 stage8 commit2369、Q=2、elapsed39.266667 s，A=0。128 帧预算仅一次 A=1（1179），live 83→124 对应净增41。 | 捕获了 held 从 1 到 0 的页内边界及紧随其后的正常 STEP。`held` 无 stopReason，不能独立确认为系统 UP 事件，手势动作由执行记录佐证。报告称“新粒子 stage2 预测 y=1.218/1.282/1.346”实为旧 ID 1/2/3 的应用后预览读数，vy=0，同窗 bank/slot 仍约1.1547；不能据此证明新粒子下降。 |
| arm2b | 同 run held=0、sourceEpoch/admissionCount仍11，128 帧全部 Q=2/A=0、live=124；末 stage8 commit2935、elapsed48.700000 s。以 arm2 末次 held=1 回执作边界，成功累计 elapsed 增约9.466667 s。旧 ID 1–3 仍 present、同 F64 y/vy。 | 已覆盖≥8 实际成功物理秒与停源后无新增准入。live 恒定不能证明124粒全部沉积，且124包含原有83粒的基数。原档无新粒逐ID、state/床量或同 commit LAND grant，冻结或完成态都不能在该批粒子上定案。 |

所采 final bank/slot 中三 ID 的位型一致，stage8 请求/实际 token 匹配。前缀中的 commit 差由 Q 和外源 A 共同解释。T360 的“未复现”限于**新包已覆盖的场景和可观测粒子**，能排除这一窗口内整条 step/发布链停止。新生群仍缺观察，故不能升级为“相同负载必然不冻结”，也不能推出复位残留或注入节奏是病因。旧日志 runId 的 `-3` 不能代替用户实际重置次数，新包三个档也都是 `-3` 后缀。T350 可核原件只有九笔实际准入和末段至406，报告所称长负载/重启史须作为待控制变量，不能当已证触发条件。

acceptedTiltRad 的30°/42°同时出现在同一档中的不同切点。stage1 的角来自传入计划，stage2 等可继承最近计划角，preview 候选不等于最终 publication。现有证据无法从该分布证明“按钮+15使提交角30→42”。保留空场 tan30 可行域裁定。要验证按键与传感器覆盖，须同 commit 记录输入源、target、最终 accepted 和 publication 角，排除 preview 值。真实手持仍按 §5 独立处理。

### 6.2 条件分流和最小下一步

当前定案为 `STEP_CONTINUES_IN_OBSERVED_WINDOW / NEW_COHORT_NOT_OBSERVED / ORIGINAL_FIRST_DIVERGENCE_UNRESOLVED`。旧 T350 原始窗口已确认不可追回，新包首次分歧仍无足够逐 ID 证据。只保留一次有明确判别条件的补证，停止原样重跑 T360。T347 注入期 LAND/床增窄证、V1 默认/code100 和 T361 性能分流各自保留。源码中 `pState==2` 会走接触沉积请求，未全额沉积可以保持 live、贴面、vy=0。该合法候选需要 state/余量/guard 收据区分，不能据贴面读数认定是旧悬空冻结。

developer 先修 T359 **观测**接口，复用 `test/t359/StopCuts.ets`、生成器与既有有界冷出口。按 `(run,epoch,admissionTransactionId)` 在最后一笔实际 accepted admission 选择最多3个**新生且离床的 ID**，为其单独固定 UP/停源锚点，旧 ID 只作可选对照。记录 stopReason=`UP|CANCEL|STALE_SENSOR|ADMISSION_REJECT|LIFECYCLE` 与事件时间，帧回执关联 sourceEpoch、实际 accepted 增量、step Q/elapsed。每 ID 记录 state、床高度/净高、质量、motion/DEPOSIT/CROSSING 行类别，requested/granted/remainder、零 grant 首因及本 commit 的 LAND/退役原因，before→predicted→afterApply→prepublication→bank→slot。显式区分 PREVIEW/AUTHORITATIVE 和 `NO_MOTION_ROW`/`ABSENT_ID`。同 commit 保存床量差和最终 accepted 角，热路固定容量，保留独立锚点和最近短环，覆盖不得吞掉前后锚点。编译和非机证明观测开关位型等价，产品物理保持原合约。

tester 使用同一新签包/入口和确定的 accepted 角。先用已观测的9笔节奏做单 run 对照，再用相同脚本在明确 reset/重启历史的 run 复做，仅改变这一因素。若两者都没有冻结且新生群观察完整，最多加一组明确上限的较长注入或高 live 负载，登记真实 accepted 数/列/高度/时间。每臂满足有效悬空 ID、真实停源及 ≥8 成功物理秒或同 token 全额 LAND，10分钟墙钟上限，固定采停源前、首个成功子步及末端。无悬空样本或超墙钟为 `INSUFFICIENT_SUPPORT`，不得填 PASS。若已捕获原形态，按 §4 首分歧表归 architect。若三组皆不复现且覆盖闭合，回 architect 按已覆盖条件结案，不扩大矩阵。

本卡续裁交付完成到上述证据边界。旧悬空冻结根因和生产修复仍缺新生群首次分歧，原卡“看评论”，请 manager 按新观测前置派工。此为 tester 的定点证据和 developer 的诊断工作，architect 继续负责首次归因。前景与背景使用 XComponent+shader，故障沿 FAULT 或完整旧 token STALE。
