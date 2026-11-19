# T678 · 正式M1忙态与a2创建原件首次归因

2026-11-19；architect；不使用真机，不运行构建/产品/测试。已读T590-v2/t590c完整boot、T593-v3/t593c六份布局及hilog、T664、两真实签包/对应源码、T663和T676在途交接范围。原字节证据 `analysis-T678-evidence-v1.json`，正式分流 `architecture-T678-contract-v1.json`。

结论：**必须拆成a2 M2开放创建观测和正式M1创建后的运行/呈现忙态**；没有证据把它们与旧T591或host T496汇成共同创建根因。当前无需新增产品恢复/线程/观察实现，先用现有出口补准确原件；T676-A/B分别接续，不重复T663或另建同类夹具。

## 1. 实际产物域

| 产物/观察 | 签包及包内libgrsm1 | 真实域与限制 |
|---|---|---|
| T590-a2 / pid14871 | HAP5dcaeda5…；lib f5d23d510c8a47a0bad76ab054ea7e25f25513d250a4411ebc1f21123d8f51ad；BuildId c789cf34c0b0f16212341c39010b7780e15654e1 | t541-t624-a2的GRS_APP_M1实际使用m2route::FullM1Transport、Kw128/feature7，含M2 create Store；同名M1 UI/库不把它变成T597的M1物理线 |
| T593-m1 / 六UI | HAP798747a0…；lib527c863f36bf543f1d4c45d212ef60ec62c2401f5fe693e5a453589c7f4280bf；BuildId dc54802f9e1a0f0f8fae93a95aa7b343bce3a59c | 正式T597-fixed，确已纠正旧T593-v2误包；本次重新签名但native/unsigned身份沿既有正式M1 |

实签HAP均核收据SHA，包内ABC/ELF逐项登记。包内BuildId不是本次设备maps/所有映射字节或当前GL program证明；T593 UI没有完整pid-start/context与native时间，不能与a2 pid14871拼一个attempt。a2 boot的`windowId`是bootSurface代理计数1，**不是**旧T591 raw window指针；不能以数值相似合scope。

## 2. T590 boot terminal 的真实语义

`boot_probe.h`的terminal由显式失败/终态Mark、生命周期撤权transition、首次Draw失败，或**首次quantum接受且首次Draw已完成**等路径设置；不是glCreate或LINK完成flag。`complete=terminal && dropped==0`只说明该封口观测未报告丢行，既非创建成功也非完整产品PASS。

attempt1 epoch1在SURFACE_CREATED把1→2时被撤权封口，firstFailure为该生命周期事件、result/rawGL/EGL均0；不是SURFACE创建API失败。attempt2 epoch2遇RESET_REQUIRED，随后UI_RESET到epoch3，已terminal/complete，同样不是驱动报错。允许firstFailure保“撤权首事件”，不能改写历史或给它补非零GL错。

attempt3 epoch3/window代理1，ownerTid15134，candidateRun1、publishedRun0/readyEpoch0/session0；1256×1399，terminal=false/complete=false。27点包含EGL init/create/makecurrent/query返回及TRANSPORT_CREATE_ENTER，最后SUBMIT_OBSERVER的a依次26/27/28，分别ShaderEnter/ShaderExit/ProgramEnter。最后ProgramEnter ns283477263148127/root0/code0，未见ProgramExit29或TRANSPORT_CREATE_EXIT。故**在此快照尚无Create返回/发布证据，最后可见复合program阶段开放**；code0是ENTER默认，不是LINK成功。

该JSON没有snapshotNowNs/capture monotonic时间，不能由文件名墙钟或“terminalfalse”计算开放持续时长/永久卡死。现有ProgramEnter边界仍包围link/status/error等复合调用，不定位某单API；没有object/source/layout/实际program handle可直接指认。按Create顺序可推候选SOURCE对象，但这只是源码推断，须同attempt m2CreateDiagnostics原件确认。recovery.inCall只覆盖其已接线的flush观察，不是全部GL API的统一inCall。

250ms是有实际root claim/permit起点后的期限，不是整个冷启动shader编译/创建的统一期限；本快照root0、无root-start/deadline字段，不能把ProgramEnter至文件导出墙钟套成某次root250ms超时。冷创建与后继root/Draw成本分别记，不放宽任何原有门。

recovery retired=true/resources=false/CREATING_NEW/serial1/epoch3意味着该快照已过旧资源退休、未登记新资源；firstReason WINDOW_OR_PAGE_CHANGED是锁存历史原因，不自动是当前创建故障。该证据不能跨到另一个正式M1进程。

## 3. T593 正式M1原件反而支持“已进入运行期”

六UI：l1 run1/epoch3/session1物理375；reset后l2 run2/epoch4/session2物理88；l3–l6同run2/epoch4/session2依次 **318→536→1002→1513**，显示GPU_WORK_PENDING、DRAW_PENDING再GPU_WORK_PENDING。相应账本scope与主状态同行，不能跨reset把375→88算回退。

源码中FreezePermitLocked在已有lastGoodControl时为**新运行root**把旧帧标STALE/GPU_WORK_PENDING；AcceptLocked在受控接受回调中更新accepted/sourceCommit/physicsCommit后写DRAW_PENDING，等待呈现。非零run、同scope提交计数推进及上述状态生产点与“始终停在未返回Create、READY从未发布”的推断矛盾。它们支持应用owner已经走过创建/资源绑定并收到接受回调；**不是**独立证明物理正确、GPU持续活跃时间/性能或完整typed proof都通过的报告。

`status.ready`由AllowedLocked给出，与status.state/current/STALE显示分离。GPU_WORK_PENDING/DRAW_PENDING本身不撤销READY；controller.allowed也不要求`state==CURRENT`。因此报告“STALE/busy所以Down必未过READY门”不成立。必须取实际ready/readyEpoch/permission/Fresh和down原返回。上屏#0只说明该UI源收据读数，不是零offer/claim调用总账，150–250ms触摸也须实际handler/changedTouches/pointer/time及同seq证据。

这些忙态也可能是正常root/呈现阶段被稀疏UI采样命中，尚未证明整个观察窗持续无新CURRENT帧。需实际accepted-root与新present/swap计数及同token时序区分“合法瞬时忙”“采样未命中CURRENT”“真正呈现不前进”，不能先把正常阶段标签当产品FAIL再修线程。

报告称recoveryState无端点也不成立：实际正式M1注册`recoveryState`、`beginRecoverySession`、`bootDiagnostics`，ELF字符串和注册源码相符。未在UI展示或未调用不是NOT_AVAILABLE_IN_THIS_BUILD；必须有准确接口调用/原错误，才可登记具体不可用。

## 4. 最小精确补证：沿已有交接，两个域不互等

**A：T590/a2与T591的M2域。** tester在T676-A已负责T663真实128B API cursor新包及collector；不再造新API/seam。旧a2若同进程仍在，先读现有m2CreateDiagnostics/M2 log与boot完整原串，锁同bootSerial/epoch/window/context/tid/source/role；旧a2无新cursor就明确字段不可得，不能倒填新API结果。后续新包同attempt的cursor才可区分LINK_PROGRAM、LINK_STATUS或原GET_ERROR哪次未返。T590旧boot是当前构建/创建窄证，不给新包或另域背书。

**B：T593正式M1域。** tester在原T676-B维护同页采集/已有正式包交接，effector原T593串行取证。固定同一run2类实际scope，不反复reset/切页制造新identity：

1. 原返回完整保存：status.ready/readyEpoch/permissionState/clockPhase/firstFailureStage、recoveryState完整字段、run/context/surface/geometry/config/input/permissionRevision/session、pageSequence/callback/receive/Fresh/重力新鲜度；sampleSerial仍只作status读取定位。
2. bootDiagnostics记录创建EXIT、资源/SESSION/READY历史及firstRoot/首次Draw，确认是否已封口/丢点。注意它先Copy boot Store再锁App追加recovery，二者**不是原子同刻**，也非整API完全不依赖App锁；不要把它误称无限安全独立线程通道。
3. 通过既有`executionDiagnosticsV1`/`timelineSnapshotBegin/Page/End`或`coldTraceBeginV1`家族，取原inputStages、sourceClaimWitness、ROOT_INTENT/ROOT_ACCEPT、permit/plan/nonce/deadline、presenting token/lease、DRAW/SWAP原返回/GL/EGL与clock producer。先用已有相同scope snapshot；源与图形的正负结果分别记，不把全局acceptedQ代SOURCE。
4. 同页一次真实Down/hold/Up，完整surface bounds/坐标/pointer/changedTouches、allowed十字段与实际reason、UI/native offer返回、app/run queue、同intent/seq eligibility/claim/reject及原SOURCE终态。没有Down就回handler/操作，不以busy直接改恢复。
5. 运行显示需分别统计ROOT接受时刻、DrawEnter/SwapReturn/receipt.current、FrameCallback pulse接收；同root标识从生产点关联，不能把UI读取时间当GPU执行或接受时间。

这是T676现有范围的字段纠偏/补齐，不新增集中准备卡或待manager形式批准门。预算沿已有接口上限并额外限定本轮collector：≤60s、≤64次诊断native查询（所有page/log计入）、每持久记录UTF8≤16KiB、目录≤1MiB含32KiB索引预留；一次大JSON只调用一次API，以≤8192B raw片段分存并记录offset/整串SHA，不能slice丢字段或按片多次重取。timeline/cold已有credit/TTL/页上限保持，容量到即明确NOT_CAPTURED_CAPACITY；不为了“全字段”无限加读。普通实际控制/输入事件另列，不把正常pagePulse隐藏成诊断调用预算。

读取status会执行既有ObserveControlLocked，必须记录其真实时间/可能的期限撤权，不宣纯读毫无副作用。采集不增加GL query/sync、热IO、假pulse/SOURCE、时间刷新或GAP/deadline放宽。若确有同scope调用不返回，保start/end/异常、准确接口参数、加载模块、App锁相关边界与已有可用出口，再由architect**只针对实缺字段**定小Store接缝；现在没有证明正式M1缺这个实现，不默认新线程或大三槽。

## 5. 分流与边界

本轮为测试结论/证据分档修订：a2开放创建精确到复合program边界；M1运行接受与呈现/输入缺证精确补取；产品根因未证，不直接developer/designer。不登记NOT_IMPLEMENTED正式恢复，不将source/物理/呈现产品FAIL或PASS混为一项。

旧T591 M2 LINK40.14s、a2另一boot ProgramEnter、正式M1运行期忙态、T496 Windows/host ANGLE退出是四个独立域。前两个即使实际shader同源，也仍需对象/ELF/上下文/attempt/原GL时间才能讨论关联；后两个更不能靠“缺terminal”合根因。T519/T410/T437真实非空SOURCE前置不假放行，T590/T593剩余原范围不缩减。

执行prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。architect不使用真机、不运行构建/产品/测试；tester离线脚本/host/构建签名，effector全部真机部署/探针/客观/效果采证且串行，developer仅开发编译。十九目标13→14→12→15→16→17→18→19，10暂停、CPU停投、旧湿卡暂缓。全部背景前景XComponent+shader，FAULT或明确STALE，无Canvas/软件GL产品回退；无新增manager/设备关闭门。
