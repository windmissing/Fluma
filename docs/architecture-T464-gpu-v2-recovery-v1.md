# T464 · V2 撤权后的显式恢复与代际隔离

2026-11-10，architect，P5，不使用真机。消费 T410 v3/T437 正式报告与冻结 T450 bridge，修订 V2 恢复工程接线。与 [T451首root归因](architecture-T451-gpu-experience-first-root-v2.md) 分开，不宣称同因。真实 GAP=33,333,333ns，GPU root绝对期限=250,000,000ns，quota仍为120Hz，显示30fps不改变这些数值。

## 1. 实际路径与报告纠偏

`m1/progress_clock_v2.h` 的 `Pulse` 在 token错或open=false时返回 STALE_PAGE_SESSION。`Fresh`/Pulse遇真实native或UI GAP会 Revoke，清pending/activeRemaining/carry/baseline/ready并关闭open。`app_bridge.cpp::ClockGapLocked` 提升permissionRevision、撤业务许可、发布 newRunRequired；后续同session pulse不再自动恢复，这是合理的fail-closed。

但 T410 报告“永久没有恢复路径”不完整：

| 调用 | 实际效果 |
|---|---|
| BeginPageSessionLocked | `BeginSession`使token递增、open=true；V2下撤业务许可，置WAIT_PAGE_PULSE |
| newRun → invalidate(nullptr,false) | **若旧run实际progressV2=true**，Revoke+ResetRun后显式open=true、hasPulse=false，仍复用旧session/sequence |
| owner检测epoch变更 | 再ResetRun，清v2ResourcesReady/permit/jobLive；ResetRun自身不改变open、hasPulse、receiveNs、sequence |
| TryReadyV2Locked | 只核progressV2/resourcesReady/active/window/pulse新鲜/gravity新鲜，未显式核旧root drain或恢复身份；不Allowed即CommitReady |
| 页面“新空场”、V1/V2冷选 | controller.reset只newRun，无新frame对象、无新page session；page hide/show才end/begin |

因此静置不恢复是可解释的关闭状态；点击新空场却再GAP，可以是**同token被重新打开后，创建/调度阶段再次失效**，不是已证“按钮没有打开open”。冷选V1→V2时 invalidate按旧V1状态执行，也不能保证新V2session开启。现有布局、截图未存每次open/sequence与阶段转换，无法倒推出每次设备是哪条路径。未证明 EGL wrapper、驱动编译和V2 GAP同根因。

工程缺口已确定：恢复复用旧session；初始化阶段pulse新鲜性与可工作READY混用；BeginSession后TryReady没有显式排空前置。不能用 `if(!open)open=true` 修复，这会复活旧回调和permit。

## 2. 本轮冻结恢复语义

采用**用户显式新空场、新run、新session**恢复，不在同run静默续算库存或续旧permit。现有产品已经把GAP标newRunRequired，本次是该语义的工程落地。若要保留沙水库存并恢复同run，另交designer设计快照/时间/来源语义，本卡不授权。

| 状态 | 允许动作与退出条件 |
|---|---|
| FAULTED_REVOKED | 原首因、epoch/run/session/lastCompleted锁存；旧session关闭，旧root不能接受/Draw/Swap。正常pulse不能自愈 |
| RESET_REQUESTED | 用户明确新空场；source OFF，拒新offer；同步撤permit并推进run epoch，取消旧RESERVED请求、冻结旧ARMED档原因RUN_RESET；停旧frame回调 |
| RETIRING_OLD | GL owner处理旧未封提交CancelSubmit、已封尾DiscardRevoked、publication release fence/lease；控制/UI线程只读进度，不等GL、不做GL |
| CREATING_NEW | 旧资源安全退休后才创建新空场，resourcesReady带不可变candidate epoch/run/context/window；长编译不能提前CommitReady或产quota |
| WAIT_NEW_SESSION | 当前资源准备完成且可交接，页面显式建立新session；新frame对象绑定pageGeneration+resetSerial+epoch+run，sequence从1开始，第一真实pulse建baseline |
| READY | 当前epoch/run/window/context/resourcesReady及session一致，旧代已退休，无在途旧root/lease，页面和gravity新鲜，明确提交readyEpoch；后续真实pulse才产quota |

RESET_REQUESTED 可立即给出独立resetSerial和WAIT_RETIRED回执，不能伪称READY。run/session使用检查递增，溢出FAULT。页面以resetSerial绑定一次重建请求，Create完成通知不能用轮询看到“任一ready”代替；旧通知/旧onFrame/旧endPageSession不能改变新代。逻辑新session须在资源完成后开启，旧session不因run reset重开。页面hide、窗口换代、再次reset均取消这轮恢复；用户再发起新空场时以新的resetSerial继续。

同context退休路径遵守现有RootIsolation和lease fence，未封候选不接受，晚到完成只退休，禁止轮询结果直接 commit。若原context失效或无法继续安全drain，走**整个旧context销毁/状态丢失**路径，记录原因及所有未完成root/lease tombstone，不把旧SSBO/FBO/窗口引用交给新代。GL owner退出旧context、资源生命周期完成后才能创建新context；销毁尚未返回则仍RETIRING_OLD，不能为了恢复成功假装退休。无需在UI锁内glFinish；超时是可导出的状态，不是允许复用的证明。

BeginPageSession不能绕过状态机恢复同run；活动run上的session更换先撤权并排空，READY只由上述恢复事务交接。不准在poll、callback或每次TryReady里延长旧permit。新root在实际INTENT只设一次250ms期限，所有continuation、accept、scene/Swap都查同epoch/session/plan/revision/deadline。旧run生成/接受/拒量和清carry账冻结保存，新run零账；空闲初始化不追补时间或补发quota。30fps显示抽样只影响呈现节奏，pulse必须按实际更高频控制回调交付，不能用33.333ms定时器制造边界心跳。

## 3. 实施与检查任务书

developer 不使用真机。优先独立实验生成器和shared helper的实际接线，覆盖诊断页与体验页的newRun/冷选/hide/show；不修改wire、物理数学、正式默认clock或业务deadline。把resourcesReady从bool提升为带identity的交接，TryReady必须验证当前恢复事务和旧代退休结果。提供reset状态、原因、session handoff、oldRoot/lease、capture终态的有界冷诊断；原end/begin/pagePulse接口保持兼容，可增加独立恢复回执适配器，不另造在线业务endpoint。

必要非机检查使用真实生成bridge与页面frame类：GAP后同token多次pulse仍拒；newRun分V1→V2/V2→V2，旧token/旧seq/旧callback不能准入；Create>GAP且期间真实pulse缺失不能误READY，Create结束后新session可由真实pulse建立READY；两个快速reset、后台/窗口换代、旧通知迟到、session溢出；在dispatch/Flush/fence/poll/map/accept/Swap各切点撤权，旧尾不能提交；lease BUSY保持退休状态；capture旧请求取消/旧ARMED冻结；新run quota平衡且无追时。250ms和GAP边界仍按原比较符检查，禁止改阈值。完成编译、非机检查、签包交付，不把真机卡加作关闭门。

tester 后续带真机标签串行：同包分别触发一次真实GAP、显式新空场、一次页面hide/show，核完整reset状态和新session实际READY，再续T410同窗双源/负控与T437完整/前缀收益。原卡取证已交不重开；新证据先回architect。实际GAP是否仍来自UI调度，需原native/UI域时间和首次撤权切点，不能用显示帧率或静置时间代替。

执行prompt必须带：要么完成本卡范围及必要检查，要么遇到具体不可继续问题才停止，局部分支受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态例外，不改其它列/行/滴答同名清单。CPU停投。全部背景前景只XComponent+shader，禁止Canvas/软件GL回退，失败FAULT或标STALE完整旧有效帧。成功原卡回执并本人行结束待验收；失败/缺证原卡留未完项、复现原错、证据、尝试、不能继续原因、前置和接手角色，本人行看评论。

本轮完成工程路径核验、报告纠偏和恢复合同，未运行真机，不声明设备已恢复或30fps达标。
