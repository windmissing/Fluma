# T413 · 单沙截图 hold 生命周期与采集事务修订

2026-11-09，architect，P4。不使用真机，不实施业务代码、不重跑设备测试。依据 T408 原卡/正式报告、归档原件、T402/T406 合同及 T406 隔离生成源码作首次归因。本文修订 T402 §3.6 的执行合同与 T406 交接流程；空间映射、RGB零容差、ROI完整取数和旧结果码要求保持。

## 1. 首次裁定

**工程生命周期缺陷与采集操作/证据缺口并存，不能以“hold 太短”作唯一归因。** 已有页面在 hold 非空时停止 tick，hold 没有定时过期机制；“封存截图时段”仅写一个截至当时的时间证明，既不解除冻结，也不等于截图已完成。第二/第三 request 的封存区间分别长87.104650秒和228.954438秒，增加固定等待时间不能纠正先封存后截图、旧预约未清、图片未取回及软键盘遮挡。

唯一后续路由：**developer 按第4节修订隔离采集流程、状态事务和收据工具，完成非机检查及新签包后，tester 按第5节重新采集；结果回 architect 分流。** 不修改材质算法、粒子半径、shader颜色规则或时间匹配门，不直接将旧卡当可继续校准的完整原件。T408 未进入 calibrate 的拒判是正确边界；其“标定前置证据完整”“时钟映射可复算”需按下文收窄，产品系统显示尚未通过。

## 2. 原件核对与四轮重建

签包 `T406-complete-signed.hap` 实测SHA256为 `bb7b0558f4118c57896ca58024829034cf981b196e89dfeba47a0397d1f57cc9`，与签包记录/T408一致。`t408_verdicts.json::evidenceSha256` 列出的51件文件均存在且hash前缀一致；这不等于所有被引用的原件也齐全。

| 轮次/原档 | 可直接核验的身份与顺序 | 判定边界 |
|---|---|---|
| r1：`calib-manifest.json`、`calib.png` | request1 / sequence3 / frame30777，manifest 内已有非空 hold；图片目视为输入界面，键盘展开 | 没有独立 hold 封存和 capture.py 收据；不能将“未封存”误写为“从未建立hold” |
| r2：`t394-2-1-30778.json`、`t394-hold-2-30778.json`、`calib2-capture.json/png` | request2 / sequence4 / frame30778；hold副本、token、endFrame一致；startMonoMs=3579530190.841422，endMonoMs=3579617295.491891，跨度87.104650秒 | capture调用2110ms、exitCode0，但fileSha256为空；T408称映射晚于封存约206秒，缺原同步对/映射参数，不能独立复算该数 |
| r3：`l31/l32`、`calib3-capture.json` | 页面仍写“等待下一次有效绘制”；capture调用2171ms，fileSha256为空；没有归档calib3.png | 报告称旧evidence未ack导致probeBusy，源码支持该路径；没有完整调用日志，不能把人工轮号等同新request已经创建 |
| r4：request3 的 manifest/hold、`calib4-capture.json/png` | 最终导出request3 / sequence6 / frame44149，hold与sample/token一致；startMonoMs=3580059054.162047，endMonoMs=3580288008.600328，跨度228.954438秒；capture调用2140ms | 报告称沿用r3 probe且截图晚于封存；原件只确定request3身份，缺四轮动作日志/时钟映射，不能独立证明人工轮号或精确时差 |

另外三个独立拒判条件不能被延长hold修复：

- **截图文件没有绑定调用收据。** 三份capture收据均只执行远端 `hdc shell uitest screenCap -p /data/local/tmp/...png`，没有在同一包装调用中取回本地PNG，`fileSha256`全部为 `""`。现有 `screen.py::load` 会先报 `CAPTURE_TOOL_RECEIPT`，尚未走到 `CAPTURE_INTERVAL_OUTSIDE_HOLD`。不能事后补写SHA把旧收据伪装为当时完成取回。r3还缺PNG。
- **独立时间证据未归档。** 51件清单和目录中没有所称三组host↔uptime原始同步对、clockMap数值/误差界或capture spec。`rate≈0.9943`是报告摘要，不是可消费映射。页面 `p013NowMs()` 实际用 `systemDateTime.getUptime(TimeType.ACTIVE,true)/1e6`；仅称读取 `/proc/uptime` 不能自动证明已映到这个域。保持原报告的“未获覆盖证明”，不采其精确错窗秒数为已独立验证事实。
- **系统截图看不到标定组件。** `calib.png/calib2.png/calib4.png` 均目视核对：ROI输入框有焦点、软键盘展开，标定图未露出。`l0`的XComponent bounds为 `[51,2197][1229,2650]`，在原图下方键盘覆盖区域；有CURRENT/native读回也不能证明该内容在这些截图中可见。r4界面还显示“等待下一次有效绘制”，不是有效新预约的确认。

两份完整request2/3的三ROI native candidate/back各22528字节、token/hold配对确有窄证。`calib-manifest.json`仍引用 `t394-1-1-30777-{candidate,back}.rgba`，目录仅有重命名的 `calib-{candidate,back}.rgba`，原名字及对应manifest sidecar未成套保留；不得直接送现有loader。重命名别名内容可核hash，不等于manifest引用已经完整。原生数据齐备不推出截图时间、系统可见或校准齐备。

## 3. 可确定的源码问题

源码基线为 `test/t394/{controller.inc,page.inc,generate.cjs}`、`test/t406/{generate.cjs,capture.py,screen.py,README.md}`，并对照签包来源 `.codex/local-test/t406-release/entry/src/main/ets/`。不把共享树其它在途修改当本包实现。

1. **先破坏旧状态再发现预约失败。** T406给“预约一次”前置插入 `t394Phase(...)`，再解析ROI并调用 `armWindowProbe`。`beginT394`会增加sequence、清hold/lastEvidence等；但 `armWindowProbe` 因已有probeEvidence/pending而拒绝。因此“请先导出上一笔”也可能已经清掉旧hold，保留旧evidence而开始新phase，形成错代组合。无效ROI同样先清旧hold。`l26→l27`从等待到“请先导出上一笔”支持这个交互风险，不足以复原每轮所有动作。
2. **CAPTURED文字不等于开放hold确认。** 手动预约写“等待下一次有效绘制”，但tick只在probeStatus等于 `WAITING_LOCKED_FRAME` 时更新CAPTURED；`exportWindow`又会把状态覆盖为 `CAPTURED <manifest>`。该文字可以表示导出结果，无法唯一表示当前可截图的hold。`probeBusy`包含未ack证据，ack只在导出成功后清除；导出证据与冻结/截图状态目前没有独立显示。
3. **封存不是截图完成事务。** `t394SealHold`核当前hold后当场记endMonoMs并异步写文件，未绑定任何截图调用或PNG。多次封存还用相同文件名；有覆盖先前封存边界的风险。预约/换臂/光学操作没有统一防重入，旧导出完成后的无参 `ackWindowProbe()`可能不具备身份保护。冻结、导出ack、封存、释放必须分开。
4. **几何和可见性前置不足。** `onAreaChange`先写t406Geometry，再递增t394GeometryEpoch，归档浮点geometry字段为2而sample几何代为3；当前handler只按宽高改变撤销，位置变动未同等处理。键盘/焦点/控件高度改变可能挡住或移动组件。不能将旧浮点记录称作与当前token完全绑定的几何证据。
5. **包装调用未覆盖远端抓屏到本地文件的完整流程。** `capture.py`在subprocess结束后直接查本地路径，旧本地文件也可能被误用，且没有强制新文件/原子落盘。README虽然要求取回位于hold内，工具没有把错误用法挡在抓屏前。这里是工具/工程用法缺口；正确区间规则本身没有被证明不合理，无需交manager放宽。

## 4. 最小工程合同：显式冻结并由成功采集驱动封存

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部分支受阻先做其它部分。只实施隔离工程与诊断工具，不改常态材质/模拟规则。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，除本人行状态有限例外；不改滴答同名清单，不回退他人。前景背景只允许XComponent+shader，禁止Canvas上屏或回退；失效为FAULT或保留有完整旧token且标STALE的上一有效帧。

### 4.1 选择与状态机

选择**显式冻结、身份化封存、独立释放**。不采用“CAPTURED后固定N秒自动成功封存”：无法知道外部抓屏/取回何时结束。倒计时仅提示；增加诊断安全上限 `maxHoldMs`（预约前配置，正整数且≤300000ms），到期只能标 `EXPIRED/不可判` 并冻结失败终态，不生成可用成功封存。超时后也不得自动重新发布/重放一帧伪装成功，需显式释放或重采。该上限只约束隔离截图会话，不改GPU期限、仿真GAP或显示帧率。

`IDLE → RESERVED → HOLD_OPEN → SEALED → RELEASED`；期间可转 `INVALID/EXPIRED`。归档ack作为独立标志 `NATIVE_ARCHIVE_PENDING/SAVED`，**归档成功不结束HOLD_OPEN**。每次预约分配不可复用的attemptId，成功draw后分配holdId，绑定phaseSequence/requestId/run/token/frame、surface/context/geometry、浮点全局box/vp2px和截图配置。不得只用人工“第几轮”作键。每个动作按身份校验、非法动作零副作用。

- 预约先完整检查ROI、budget、旧pending/evidence、导出状态、旧hold、phase与几何，全部通过才一次提交新phase与probe request。失败保留旧hold、sequence、evidence及可重试导出；不得先beginT394再检查busy。旧hold未封存/释放时，预约、切臂/光学/计划等返回 `HOLD_ACTIVE`；显式取消写失效终态。
- `HOLD_OPEN`只在同次native CAPTURED/CURRENT、token/frame/ROI与sample全匹配后建立。冻结publication与呈现帧，不继续tick/替换；保留当前组件，禁止将诊断状态更新计成新物理帧。后续owner/surface/context/geometry、位置/缩放/可见区域变化、后台、重置或取消立即使hold无效并保存首因。
- 封存调用核hold身份/有效性/真实elapsed后，固定第一次endMonoMs/endFrame/endToken；重复调用同id返回同一份封存，不重新取end或覆盖。异步写盘之前先固定内存终态；写盘失败可重试同一快照。所有异步完成按attemptId/requestId确认，不能无参ack清掉后续证据。
- 释放不代表封存；SEALED需归档完成后显式释放才允许下一臂。INVALID/EXPIRED允许在保留失败记录后显式释放；从未OPEN的失败预约也输出原始拒因。hide/disappear不能隐式转换成成功seal。

### 4.2 接口、文件归属及固定布局

2026-11-09，T426 修订。页面准备动作以 [T426 页面准备与键盘就绪合同](architecture-T426-single-sand-prepare-readiness-v1.md) 为准。收键盘命令回执与页面就绪分别记录，使用标准 TextInputController 退出编辑及同代窗口/焦点/遮挡/稳定帧观察生成本地许可，在预约提交前复核。`configuring=false`、API 成功或内部错误码55均不能独立放行。该修订同时补充 §4.4 的生成页异步/键盘非机检查与 §5 的 T408 新 attempt 接续，不改变以下 hold、时间、颜色与映射合同。

在后续隔离生成器（基于T406，建议 `test/t413/generate.cjs`）与独立inc中实现，避免破坏T394/T406历史包重放。主要落点为生成页 `P051DerivedV2Demo.ets`、生成controller `P013SingleSandController.ets`、`T394Phase/SingleSandWindowCold` 的版本化扩展；native只补必要的当前owner/geometry验证，不改shader/读回布局。

| 新隔离控制接口 | 返回与约束 |
|---|---|
| `reserveCapture(roi:Int32Array,fixtureKey:string,maxHoldMs:number):CaptureReply` | 冷调用；全校验后原子建attempt/request；`RESERVED`或具体 `REJECTED` |
| `holdStatus():HoldStatus` | 只读当前身份、阶段、archive标志、剩余时间/失效首因；先按真实时钟验证是否到期，不续hold |
| `sampleHoldClock(expectedHoldId:string):ClockSample` | 在同一app `p013NowMs` 域采原时刻，返回clockId、boot/session、holdId、严格递增sampleSerial；不从PNG/hold端点倒算 |
| `sealCapture(expectedHoldId:string):CaptureReply` | 只封同一仍有效OPEN；成功后同id幂等，异步保存固定快照 |
| `ackArchive(attemptId:string,requestId:number,manifestSha256:string):CaptureReply` | 只确认确已写完并校验的同一manifest；不释放hold，拒旧完成回调 |
| `cancelCapture(expectedAttemptId:string,reason:string):CaptureReply` | 将同一RESERVED/OPEN置INVALID，冻结失败记录；不造成功seal，重复取消保留首因 |
| `releaseCapture(expectedAttemptId:string,expectedHoldId:string):CaptureReply` | 核SEALED+归档成功或已归档失败；未曾OPEN的失败仅允许空holdId且attempt精确匹配；释放后下一预约才可开始 |

`CaptureReply`固定含revision/state/reason/attemptId/holdId/requestId/phaseSequence；`HoldStatus`额外含token/frame、完整几何/owner身份、startMonoMs、maxHoldMs、archiveState和单调actionSerial。`ClockSample`增加sampleSerial/appMonoMs及单位。新hold封存用 `T413_DISPLAY_HOLD_V2`，显式携带start/end与有效性/首因/全部身份，文件名含attemptId/holdId，禁止覆盖。旧V1 loader保持历史语义；新工具必须按新revision分支完整校验，不能放松成“任意hold JSON都可读”。

页面改为固定尺寸诊断控制区与始终可见的XComponent；phase/状态长文本截断或另导出，不让文本长度撑动组件。ROI/plan配置、焦点与键盘收起、旋转/窗口配置全部在预约前完成，布局稳定后采浮点box及整数bounds；先递增几何代再记快照。位置、尺寸、vp2px、配置变化都纳入同一几何身份。进入RESERVED后禁用文本编辑和会改变布局的控制；状态/倒计时更新不得遮挡标定ROI或改变组件位置。不能在old hold内关闭键盘后仍使用old几何，须按失效重采。

冷状态文件/日志分别显示 `HOLD_OPEN id`、`NATIVE_ARCHIVE_SAVED id`、`SEALED id`，不能复用“CAPTURED”一条文本表达三件事。沿现有按钮/冷文件通路提供稳定控件id与对应JSON状态，供tester既有hdc自动化读取；不要求新增在线业务endpoint、后台模拟ticker或截图写入shader。UI当前holdId只供操作，host与离线工具仍须核读到的完整身份。

### 4.3 host采集工具与独立时钟区间

交付 `test/t413/capture_hold.py` 与使用说明，实际命令用argv数组、逐步骤记录原始返回，不以shell拼接字符串传ROI。工具输入固定device/tool配置、预登记ROI/spec、cold状态/时钟/导出/封存命令定义与全新输出目录。preflight先验证完整命令路径和本地目录可写；已有同名PNG/receipt拒绝，不能借旧图成功。host不修改app时刻或伪造JSON。

严格执行：**预约→核同id的HOLD_OPEN及无遮挡几何→导出native并身份化ack（继续冻结）→独立clock采样→抓屏+取回本地PNG→后clock采样→核PNG/hash及仍为同一OPEN→请求封存→取回封存和完整原档→区间/身份预检→释放。** 任一步失败停止本次成功路径，保留错误/原件，显式取消/归档失败；不能点击新预约试图解忙。app封存只证明截至该点未换帧，最终能否配对仍由离线工具验证。

新版收据 `T413_CAPTURE_CALL_V2`含attemptId/holdId、各命令argv/stdout/stderr/退出码、host monotonic起止、设备远端唯一文件名、本地PNG路径/字节/SHA、预后cold状态actionSerial。整个保守区间从截图命令启动前到文件取回及SHA核验结束后，完整包含各子调用；取回失败/空SHA/旧文件/PNG损坏一律拒绝。封存必须在该区间结束之后发起，所有子收据和原PNG写入同一不可覆盖目录；与hold的关联进入bundle/spec，不能手改旧T406收据补字段。

时钟不继续依赖未证实的 `/proc/uptime→ACTIVE` 等价：使用 `sampleHoldClock` 的原始app域采样。每组保存host请求前 `h−`、取得对应新sample后的 `h+`、app返回 `d`、sampleSerial/clockId/holdId和原命令输出，确认 `d`确在该调用内采得。至少三组、跨越截图前后，拒重复sample、换boot/session、回拨和跨挂起等失去稳定映射证据的窗口。不得从hold端点/粒子图推拟合参数。

以独立样本约束正斜率仿射映射 `d=a*h+b`（统一ms）；每组满足 `a*h−+b ≤ d ≤ a*h++b`，量化误差独立列出并扩入约束。计算所有可行映射对完整capture区间的最小下界L和最大上界U，需可行集合有界且一致；不取一个最有利offset/rate。只有 `holdStart ≤ L ≤ U ≤ sealedEnd` 且有效身份全同才通过时间门。clock源/稳定性/界限无法证明则 `CLOCK_MAPPING_UNKNOWN`，区间越界则 `CAPTURE_INTERVAL_OUTSIDE_HOLD`，不增容差消除失败。显示倒计时或发生先后顺序只辅助操作，不能替代此数值证据。实现也可用被独立证明更精确的同域时钟接口，但必须保留原始同步与误差来源，不把“同是uptime”当证明。

### 4.4 开发必要检查

非机验证实际controller与文件流程：busy/无效ROI/重复点击零副作用；导出失败保留证据及重试、旧异步ack拒绝；导出ack不结束OPEN；seal早于capture则工具拒绝、重复seal边界不变；实际到期但timer延迟时仍拒绝；hide/owner/geometry/位置变化均失效；PNG缺失/远端取回失败/预存旧图/空SHA拒绝；时钟样本跨域/失配/无界映射拒绝。正控与反控各自使用全新会话/文件，不能靠覆盖封存获得通过。继续跑必要旧T406映射/颜色/预算与正常零readback回归、ArkTS/native编译并签包，交新SHA、原始结果与完整可执行README。开发关闭不以另卡真机为前置。

## 5. tester 后续任务书与独立负控

真机标签，设备串行。真实前置为第4节开发交付及新签包；先离线读取README/命令定义并核SHA、路径、完整bundle检查器。执行要求、清单修改边界和渲染边界同第4节。采用全新attempt，旧T408样本只保历史拒判，不能补写旧端点或收据挽救。

1. 首次独立标定：先收键盘/固定布局，再预约三处ROI；确认同id的OPEN、native归档、可见标定图和独立时钟样本，runner完成PNG取回后才seal。归档manifest实际引用的原文件名/sidecar和全部原RGBA、PNG/capture/clock/bundle/spec/action日志。先报告时间/身份/可见性门各自结果，再执行原精确calibrate；颜色/模型不匹配仍拒绝，不把hold修好当校准通过。
2. 独立时间门正控：一个全新的合法OPEN窗口，完整调用区间连映射不确定性被封存包含。**独立负控**另开全新会话且保持同样的PNG完整性、几何和其它身份：先seal后运行抓屏/取回，再消费相同hold封存，应精准拒绝 `CAPTURE_INTERVAL_OUTSIDE_HOLD`，不能被空SHA/错身份等更早错误遮住。另测调用开始在hold前/结束在封存后、中心时刻在内但误差包络越界；后两种可用独立合成工具夹具验证，不改真实设备端点。报告哪些为真机、哪些为合成，负控拒绝不记产品正常通过。
3. 独立身份/操作负控：未ack或OPEN状态点击预约、无效ROI、重复seal、导出失败重试、旧ack迟到；核旧hold/phase不被失败动作清空。同内容不同holdId、换context/geometry、键盘/窗口变化即失效；不把一个负控的无效样本复用于另一个精准错误码检查。
4. 时间链和标定都通过后才冻结calibrationId与planId，用原材质shader做静态中性/非零gains及CONTROL/LOCK A/B，同一映射/完整native反查列，沿T402/T406正反控和RGB零容差。任何改几何重标，禁止逐ROI/逐臂搜索。所有正反控结束回architect；执行失败在本人行“看评论”并报告原始错误、未完成项/已尝试/具体前置，不能把缺证标产品PASS。

## 6. 本卡交付与边界

完成原卡/报告/51件hash、签包hash、四轮request/phase/hold身份与区间、三张原图目视、生成器和签包来源源码核对；未使用真机、未改业务/测试实现。T408“未运行calibrate/未获hold覆盖”的结论保留；其“时钟可复算、标定前置整体完整”按本稿收窄，关于延长/自动封存/确认操作节拍的候选建议以本稿唯一工程路线替代。87秒与229秒均不能替代正确操作次序、PNG绑定、时钟原件及可见性证据。

2026-11-09：新增T413工程修订，选定显式冻结+采集完成后身份化seal，不选固定延迟自动成功封存；倒计时仅提示、到期只失效。architect本卡交付不等于单沙系统截图/标定或材质通过，由manager登记交付并据此派developer，不增加manager验收门。
