# T353 · 停源冻结首次归因、倾角与入口分流 v1

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
