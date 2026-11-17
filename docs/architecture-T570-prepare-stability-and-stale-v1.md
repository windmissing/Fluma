# T570：单沙准备收敛、真实换代与STALE首次归因

2026-11-17，architect，不使用真机，不执行产品测试。消费T408 attempt10、T553、`test/reports/t491/t408vA/`原件及T540→T516→T469→T455→T426生成链。当前卡完成首次归因和工程接缝；READY/HOLD/严格PNG/标定仍NOT_REACHED，不修改旧失败或宣称产品通过。

## 1. 证据裁定

真实IME可见和keyboardAreaSeen=true已有正式报告及原件，撤销“uitest设施根本不能弹IME”的全称推论。inputText单独不弹与真实点击聚焦后可弹是不同操作序列。prep-74：keyboardConfiguration=UNKNOWN、keyboardHeight=0、avoid四项零、sessionEverEdited/keyboardAreaSeen均true，三个stopEditing与clearFocus回执code0；其181个frame观察期间serial只从72到74，不是每个frame都被blur推高。该原件的UNKNOWN由未确认固定键盘条件直接解释，不再派IME设施缺陷或假置HIDDEN。

**prep-40/41不是“只有serial变而permit过期”的实证。** BEGIN的prepareId1/page1/serial38后，第一个snapshot run=`p011-run-1794916671870-3`、context1、geometry11，下一snapshot变为run=`p011-run-1794916708845-9`、context1、geometry12。`PrepareReadiness.observe`明确在prior.nativeReady且run/controller/session/surface/context改变时invalidate(PREPARE_STALE)；这是有原件支持的真实owner换代，不能关掉该门。prep-41还追加HIDE、page变2，而observation仍page1；所有这些冷原件permit均null、没有READY。完整事件里没有同一prepare的INPUT/KEYBOARD_AVOID_CHANGED作为这次失效唯一前因。

报告叙述“确认后仍STALE”、lI/lJ/lK/lM的INVALIDATED文本保留为REPORT_OBSERVATION；本目录没有与“已确认固定键盘”的后继失败一一配对、含first-invalidating callback与before/after owner的完整冷原件。因此尚不能判定该后继是同一run换代、frame owner/branch失配、真实输入变化，还是permit签发后晚回调。lP/lQ仍出现“确认全屏手机固定键盘”，不能拿它们证明另一时刻已确认。需要一次窄补证，不能用prep-41替后继scope补链。

## 2. 实际源码因果与需要修订的工程边界

`changed`在非READY态仅递增observationSerial、清稳定候选并调用capture.invalidate；T413Capture.invalidate在IDLE直接返回，不会反调prepare失效。INPUT事件总会推进inputSerial；这使PREPARING重新收集两帧，**本身不会将PREPARING置INVALIDATED**。当前硬失效入口还有observe中的native/window换代、draw回调window不匹配、prepareFrameDeadline的owner失配，以及READY布局不匹配。报告中“serial变所以verify失效”不能跳过这些入口直接成为根因。

工程采用 `T570_PREPARE_TRANSACTION_V4`：外部PREPARING不变，内部subphase=DISMISSING/SETTLING/VERIFY_PAIR，仍从用户本次begin起严格3000ms，不续期。先begin并清旧候选/permit→记录三个stopEditing与clearFocus真实返回→配置分支切换且注册本attempt的didLayout/willDraw/frame callback→在**同page/window/native owner**下等实际input均不editing/focused、真实avoid归零/height0/animation结束、原visibility/raster/native谓词全部满足→两个递增frame ticket及各自独立willDraw采样相同语义快照→READY一次签发permit。不能用API返回成功替blur/avoid已完成，不禁用observer，不隐藏sessionEverEdited，不用延时sleep代替真实帧。

同代PREPARING中soft变化（真实focus/edit/avoid/height/layout/配置内容变化）全部记录并清firstFrame/候选，等新didLayout与两个新draw pair，state保持PREPARING。必须重新通过原谓词，不能跳过变化。window/page/context/surface/controller/session/run硬换代、窗口失活/隐藏、用户显式configure、API失败/超时仍立即终止本attempt，候选/permit清空；绝不把新run偷接入原permit。若关闭键盘导致renderer真实重建run，应完整报告该换代，等待该新owner就绪后由下一次用户begin进入新attempt，不自动循环续3000ms。

begin时尚无有效nativeReady的窗口，native owner可暂UNBOUND；同page/window内第一次有效完整owner只绑定一次并重新开始两帧候选，仍使用原deadline。绑定后任何hard owner变化必终止；不能把已ready的run换代伪装首次绑定。源码已正确的soft重采样/硬换代门予以保留，本稿不要求为未证的“blur必失效”重写renderer或放宽比较。

READY后任何参与许可的input/avoid/layout/config/owner变化仍立即撤权，含值相同但来源为真实输入的晚事件；reserve必须在同UI事件循环重读完整快照+owner+permit serial，check与consume间不await、不触发stopEditing/clearFocus/改布局。不在reserve再次收键盘。consume后仍保原RESERVED/HOLD token检查和native许可；不能因prepare已消费就跳过late/context撤销。纯报告导出/计数不生成新的物理generation或输入变更；T540已将firstDraw/raw serial按代冻结，不凭空宣称存在逐帧必变JSON缺陷。

## 3. API、线程和文件落点

真源为 `test/t426/PrepareReadiness.ets`、`test/t426/generate.cjs`、`test/t455/{generate.cjs,pump.inc,PrepareFrameObservation.ets}`、`test/t469/{generate.cjs,page_raster.inc}`、`test/t540/{overlay.py,page_raster.inc,RasterGeometry.ets}`。新v4 overlay生成同源page/readiness/frame observation；不得只改 `.codex/local-test/t540-final`。旧T540包/目录/原件留存，新包、生成清单和SHA独立。

新增 `PrepareAttemptOwner={prepareId,pageGeneration,windowId,windowGeneration,branch,controller,session,surface,context,run}`。`begin(now,allowed,owner):boolean`冷建本次owner和固定deadline；`noteChange(reason,before,after,kind):void`先记录再分SOFT/HARD；`sampleFrame(owner,ticketNs,drawObserved,now,snapshot):void`核owner及原predicate；`checkForReserve(snapshot,now):PrepareCheckTicket`同步核准但不提前消费，现有capture.reserve在同一调用栈核相同serial/owner，**仅返回RESERVED后** `consume(ticket)` 单次消费。中间不await/改布局，reserve失败保原失败结果/permit规则，不能新增接口绕过capture.reserve或失败也消费。UI只在UI线程读布局/observer；native current身份走既有冷readiness接口，无UI线程等GL/future/IO。

用first-invalidating witness补观测，**不是改许可算法**：每attempt最多64条×256B紧凑转换记录、before/after两个原snapshot各≤16384B、8条API记录×256B、含首因的2048B header，总payload≤53248B、整JSON≤65536B，计入原2MiB writer/4MiB累计信用。超限完整INCOMPLETE，firstCause不得被后续frame/timeout/HIDE覆盖。每条含原app单调时间、prepare/page/branch、callback来源、state/subphase、input/observation serial、before/after完整owner及reason；snapshot保存真实kbCfg/avoid/height/keyboardAreaSeen/history与三框，不能只留reason。firstCause是实际失效分支生成，不从最终diff回填。

新窄接口 `prepareAttemptWitnessV4(prepareId):string`只冷导出，读取失败保原API错误，不能唤醒渲染owner或重启attempt。frameNs只用于递增/对应票据，elapsed只用app monotonic，不能混减裸时间。准备UI状态文案保持定长区域，导出不改实物raster布局；独立诊断文件IO沿原异步冷writer。

## 4. 接力与精确补证

tester在新v4包串行一次保PID/HAP/ELF/生成SHA、真实点击输入框→系统IME与非零avoid→固定键盘确认原值→本次prepareId全链→首失效witness或READY→reserve/HOLD。优先同原签包已有raw接口保后继失败；缺窄见证才开发后补。分别比较两条路径：原已稳定无焦点的prepare；真实IME收起中的prepare。必须覆盖同代soft重采样、hard run/context换代、READY晚blur撤权、超时不续、新旧callback owner、API失败/诊断超限，不用重复12s等待代替因果证据。取得后回architect按firstCause分流，设备证据不是developer关闭门。

交developer：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按上述真源完整交窄见证、v4状态/转换显式化和生成身份，保留已符合合同的原谓词/soft重采样/hard失效/3000ms；同步check-consume不得绕开原capture.reserve结果。先补首因原件，不把本卡当成已证所有prepare失败的产品修复；具体native run重建触发器须等同attempt原件后再修，不先泛化重启/IME物理。仅开发/编译/提交、不执行测试。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其余项。阻断原卡写未完、原错/复现、证据、尝试、不能自行继续原因、输入前置与接手角色；失败看评论、成功结束待验收。

CPU停投，全前景背景仅XComponent+shader，禁Canvas/软件GL产品回退；故障FAULT或标STALE的上一完整有效新管线帧。250ms/GAP/120Hz配额/显示30fps、准备门/原严格三框和PNG零容差不改，不新增manager验收门。
