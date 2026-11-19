# T664 · 真实 SOURCE 的 STATE_LOST 与恢复链归因

2026-11-19；architect；不使用真机，不运行构建/测试/产品。原T593 v2、九份t593_l1–l9原件、T623、实际T591与T597签包及两份native/page/controller源码均已读取，原字节证据见 `analysis-T664-evidence-v1.json`，完整分流表见 `architecture-T664-contract-v1.json`。

结论：**WRONG_ARTIFACT_AND_CAPTURE_ROUTE_WITH_UNPUBLISHED_RUN**。不能由本批推出正式M1缺恢复路径，也不能把UI sample递增当owner活性。先由原T593 tester使用既有正确M1产物按下述同代路径补证，不派猜测性的时钟/恢复开发，不转designer。

## 1. 九份原件真正包含什么

l1没有该状态链字段；l2/l3为run0:0、epoch2、session1、sample3678/4205；l4至l9为run0:0、epoch3、session1、sample5224/6129/6702/8213/10538/12441。账本标签的epoch/session仍0，源收据#0/物理0:0。证明UI显示的状态仍不可消费，**没有已发布非零run/READY/current counters证据**；零收据不是完整offer/claim日志，不能据此证明一次typed调用都没发生。

`status()`原实现每次读取都执行`++a.sampleSerial`。因此上述增量首先是**状态读取计数**，不是accepted page pulse计数、GL线程进度或物理提交。实际页frame先pagePulse再status，但V1路径还把内部Pulse结果统一回`PAGE_PULSE_ACCEPTED`；仅看frame循环/显示不能反推内部时钟准入。需要真实pageSequence/pageReceivedNs/原始返回及同scope权限原件。

l7/l8/l9明确显示 **`REJECTED V3_USE_M2_TRACE`**。此包`beginEvidenceCapture`源码无条件返回这个拒绝。没有captureId，自然不会进入exportCapture的文件写出；并非已创建同窗档后“导出吞失/写到别域”的证据。此时补目录扫描或重复按导出也不能制造所需M1同窗原件。

九份UI没有onTouch/changedTouches/down结果原件、boot/pid-start/context/Create ticket/事件单调时间。源码model.down在!allowed时具名理由是 **NOT_READY**，不是STATE_LOST；UI的STATE_LOST标签不能冒已捕获的down返回。所报220ms触摸、Home、重置也没有同intent原始命令与Down/Up/pulse收据可核，保为操作报告而非完成全部输入链的证明。

## 2. 同名页、同名库其实是两个产物域

| 产物 | 实签HAP SHA | libgrsm1.so SHA / BuildId | 实际工程域 |
|---|---|---|---|
| 本次T593所称T591-collector | 9550454c… | 04b98996346e2706fd54e6d3da5e91fefd028d5180c408fb231473554f3c559f / 0cf0ffde1232b06e0ee044ef519cf01c11120dd9 | t562-m2-fix，GRS_APP_M1使用m2route，T468_M2_V3_DIAGNOSTIC；没有recoveryState/beginRecoverySession注册字符串，采集明确V3_USE_M2_TRACE |
| T623已引用的既有T597-fixed | 20bad499… | 527c863f36bf543f1d4c45d212ef60ec62c2401f5fe693e5a453589c7f4280bf / dc54802f9e1a0f0f8fae93a95aa7b343bce3a59c | 正式M1体验恢复/输入链，含recoveryState/beginRecoverySession、WAIT_NEW_SESSION、T575输入证据 |

两实签包均核对签名收据SHA，包内库原字节已静态核对；设备实际加载资格仍应由本次同进程收据确认，不能拿包内字节代替设备加载。原件与代码同时支持**测试对象/接口不符**，不是只靠缺某个字符串猜实现不存在。T591的UI也叫GpuResidentM1、库也叫libgrsm1.so，但这两个名字不足以证明它拥有T623正式恢复/输入接口。现有T597-fixed是可直接选择的既有产物，不要求developer再次构建/签名才让tester开始准备。

## 3. 创建先于资源发布，不能跨attempt认因

被测旧t562 App先生成候选id/transportIdentity，再调用GPU Create；仅当Create返回OK、同epoch/window仍current，才CommitReadyLocked发布identity.run/readyEpoch。newRun调用invalidate，先epoch+1、撤权、置STATE_LOST；它不是强行打断/替换正在执行的GL调用。GL owner尚未从Create返回时，UI线程可继续status/epoch变化，资源仍未发布。run0与此机制相容，并不说明READY后时钟无法恢复。

T591先前collector有serial1/epoch2、候选run1、SOURCE LINK开放，T662已判仅OPEN_CALL_AT_OBSERVATION_END。**本批九UI没有足够boot/ticket/context/native时间连接这段旧collector**；同包、同目录或相同小整数epoch都不等于同进程/attempt。不能断言“这九次STATE_LOST必由那次LINK永久卡死”，也不能把候选run1与UI已发布run0当字段矛盾。精确首阻断是“当前没有已发布可消费资源证据”；具体Create API/是否终态仍需当前attempt原件。

旧M2单API游标沿已经在途的T663/T662；不重复另建相同开发卡，也不把那条M2创建证据自动作为正式M1真实SOURCE前置。若选定正式M1包也遇Create未返回，补它自己的原始创建/恢复窗口，再按实际缺口交architect。

## 4. 正式M1合法恢复和同代证据顺序

既有T597工程已具备以下链，故本轮不宣NOT_IMPLEMENTED：

1. `newRun`显式请求，先撤旧permit/source intent/配额/capture，冻结原firstFailure与旧账；生成resetSerial+epoch+window，转RESET_REQUESTED/RETIRING_OLD。不得用自动重盖时间把旧flight当新成功。
2. 真实旧root/fence/scene lease退休，GL owner创建新资源；只有Create成功且epoch/window current、非零candidate run，`recovery.Resources`才登记resources/retired并转WAIT_NEW_SESSION。
3. 新page frame持有自己不可复用的generation，按同resetSerial/epoch/run/window调用`beginRecoverySession`；旧frame停止且旧session拒绝。`GpuRecoveryPulse`不是定时造heartbeat，绑定真实FrameCallback。
4. `TryReadyV2Locked`须同时满足v2ResourcesReady、recovery.CanReady、active/window、真实progress.Fresh、ui.Fresh；完成同锁CommitReady。V1/V2必须记录实际选择，不能拿V1 UI标签冒V2 Fresh已通过。
5. 新的真实Down才可开启source；先首pulse建锚，再合法间隔offer；GAP33333333ns、root250ms、原始SOURCE时间/seq/witness规则不放宽。reset/Home/选源只是操作，不是完成了资源/session/Fresh链的替代证据。

原T593 tester先固定既有正确签包/实际加载ELF/clock revision/接口清单、页/Ability/XComponent bounds，保存完整`status/recoveryState`和现有冷diagnostic/inputStages/同seq witness接口原返回。配置/显式newRun一次后，仅按真实恢复phase推进；若在Create或WAIT_NEW_SESSION停住，**就记录该最早边界**，不要继续用触摸空收据猜后段故障。

同一文件manifest至少含boot、pid-start、实际模块SHA/BuildId、resetSerial/epoch/window/context/run、pageGeneration/session/permissionRevision、ns的producer与时钟域；status.sampleSerial只定位状态读。资源未发布时published与candidate身份分列。收完整Create ENTER/EXIT/code/sealed/native时间、恢复phase/resources/retired、旧flight退出、page session请求/返回和pulse原返回/Fresh/firstFailure；之后再收onTouch changedTouches/down reason、pointer/局部vp与screenpx、intent/typed入队/同seq Claim/SOURCE终态。原T623的150–250ms且跨真实pulse操作与scope门保持。

只用现有只读Store/diagnostic/cold分页导出。若某字段该包不支持，写NOT_AVAILABLE_IN_THIS_BUILD并附实际接口/返回，不把缺功能当未知0；如果正确正式包出现调用不返回或没有所需边界，再提交准确endpoint参数、同scope原返回/超时与源产物身份给architect冻结最小有界观察接缝。本轮不添加产品端点、GL查询、热IO、恢复线程或CPU求解。

## 5. 接续与检查

原T593修对象/采集设施后续完整未完；T519/T410/T437仍需其各自真实非空SOURCE前置，不由本卡归因或旧零收据放行。T591/T663保持M2独立域，T597已有开发资产不回退，不因tester签名/部署/测试增加开发关闭门。此为完整architect分流交付，具体产品指标仍未证；静态检查与机器表不冒设备/恢复/SOURCE PASS。

执行prompt：architect/developer不使用真机；tester需要真机标签并按设备队列串行，维护脚本/签名部署/原件。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人行状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。GPU驻留优先、CPU停投、目标10暂停、水13→14→12、旧湿M1暂停。全部背景前景XComponent+shader，失败FAULT/明确STALE，无Canvas/软件GL产品回退；不设manager验收门。
