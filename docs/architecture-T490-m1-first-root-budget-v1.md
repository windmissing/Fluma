# T490 · M1首root预算、撤权与恢复再入归因

2026-11-13，architect，P5，不使用真机。消费T448 v2原始boot/full日志、T483所列实际T467生成源和最终签包，完成首次归因与工程/设计/补证分流。原件hash、签包/ELF重算和整数纳秒算术见 [证据索引](analysis-T489-T490-evidence-v1.json)。本稿修订T451/T464对应历史解释，不重跑产品或设备测试；M2另见 [T489](architecture-T489-m2-presentation-timeline-triage-v1.md)。

## 1. 来源、attempt与有效ACCEPT

T448 v2实际包是T467-experience-final，HAP SHA256 `1a3d8d5313480cc5bfbcac54d12ff461d2db2ef8cb79da70572ad9687b62f6f2`，包内libgrsm1 SHA256 `b2dd4ba23abc90fb0f1ce9b31f2de0f31878e82f59eccf09f54f5e77ec561ea7`，重算与T257收据一致。冻结目录 `.codex/local-test/t467-unit`。该包不含后来的compute持久化及T485场景缓存/冷准备，不能把当前工作区成果视为旧包已安装。

boot1/boot2同PID16802，三份attempt完全相同，boot2 recovery.events是boot1的原样前缀扩展，不是两次独立启动。attempt1/epoch1没有窗口，attempt2/epoch2终态RESET_REQUIRED，attempt3/epoch3才为有效运行：candidate/publishedRun1、session1、实际V2、K192/Kw128、1280×1609。attempt3均237行、dropped=0、complete=true；这里complete是该有界boot记录终态，不等于后续所有回调均收录。

本次首root为kind2 **QUANTUM、127 compute passes**，不能称为场景shader链。冻结app_bridge的isolation.accept在同一短锁内先PermitLocked及activeRemaining检查，再`commit(data)`、AcceptLocked、记ROOT_ACCEPT。因此本次ROOT_ACCEPT是实际提交接受，既不是普通tail marker，也不是期限后误记接受。

| 事件 | native ns | 解释 |
|---|---:|---|
| TRANSPORT_CREATE enter → exit | 70292288071717 → 70343571098749 | 51.283027032s，发生于READY和本root INTENT之前，不能塞进250ms账 |
| READY_COMMIT（boot） | 70343576510311 | recovery同名点70343576506561，记录点略有差异 |
| ROOT_INTENT携带started | 70343609929217 | INTENT写日志为70343609933592，预算起点必须用携带值 |
| deadline | 70343859929217 | started+250,000,000ns |
| ROOT_ACCEPT | 70343855447967 | intent起245,518,750ns，即**245.51875ms** |
| DRAW_ENTER | 70343855472342 | accept后24,375ns，即**0.024375ms** |
| supervisor timeout观察值 | 70343860670155 | 超deadline740,938ns；note记录70343860670624 |
| 晚期RootCut::Swap拒绝 | 70343998527030 | note70343998529530、result9，Draw后约143.055ms |
| 最后呈现回执 | 70343998668592 | T398_PRESENT_REVOKED，Draw后143.19625ms |

原报告“2455ms提交链超期”“accept到draw约2.4ms”均为单位错误。实际ACCEPT在期限前，剩余 **4,481,250ns**；Draw进入剩 **4,456,875ns**。不能据此宣称本GPU无法在250ms内完成QUANTUM，也不能宣称呈现可通过：许可同一绝对deadline继续约束Draw/Swap，supervisor在Draw进行约5.198ms后已撤权，尾部Swap切点明确晚拒，当前帧未获有效呈现。

冻结swapped回调先记SWAP_ENTER，再Check(Swap)，拒绝后不调用eglSwapBuffers。本次boot无SWAP_ENTER/RETURN，但**不能由缺行证明回调未到达**：boot_probe::Root在attempt.terminal后直接return，timeout的bootNote已将terminal置true，而最终Draw回执仍可记录。result9晚拒支持Swap门被行使；原件rawEgl=12288是EGL_SUCCESS、rawGl=0，既非驱动错误证据，也不证明eglSwapBuffers执行成功。补证必须保留撤权后有界尾部，不能用complete=true或零raw错误覆盖这个记录边界。

## 2. 全首root成本与可执行分流

13个Flush enter/exit区间合计 **186,712,341ns**，其中四段为39,427,187 / 53,469,375 / 51,743,281 / 41,828,594ns。SUBMIT_OBSERVER FenceEnter36→FenceExit37占 **45,501,406ns**，其余INTENT→ACCEPT墙钟约 **13,305,003ns**。这些是native调用区间，可能含驱动阻塞或线程被调度出去；不能直接称GPU执行时间，也不能把未测的批间CPU空档解释为2.27秒。sum不重叠的这两类区间只用于本次账，不代替其它root的长期性能。

Draw首次会Scene::Ensure编译三个graphics program、分配FBO并走全背景/床/粒子/水体/前景/呈现。旧包没有子阶段区间，**143ms不能唯一归因shader编译**；驱动lazy工作、FBO和draw均待分段。现有 [T480场景缓存及冷准备合同](architecture-T480-scene-program-cache-v1.md) 与在途T485正覆盖graphics创建移出首物理root，不重复建开发范围、不将新取证塞入T485关闭门。T485只完成它约定实现/非机检查即可交付，T491独立真机验证。

新增工程诊断/修复接续（developer，不使用真机，manager另卡接力）：

1. 保持现有物理pass、批边界、Flush/fence语义与shader字节；首QUANTUM用固定槽记录permit的完整identity、nonce、kind、plan、revision、started/deadline，以及每批from/to、Flush enter/exit、fence创建/等待/返回、poll/audit/commit/accept时间和结果。只首root/首错与有限尾部，无热文件、全粒子读回或无限日志。
2. 在Scene分别包住program Build/load/compile/link、VAO/FBO准备、各层draw及Swap前检查/实际调用/返回/事后许可。每点注明线程、时间域和root；原始GL/EGL错误在清理前保存。记录call未返回及因撤权被跳过，不能补造exit。保证故障后同root最后少量检查仍可记，独立于boot terminal过滤；dropped/截断显式保留。
3. package/source provenance沿T483签包收据，M1/M2分别标路由、HAP/ELF、生成器/source commit和实际clock。暖load成功不等于graphics暖或driver lazy执行已热；比较首次/后续、场景冷/暖分别列账。

若缓存/冷准备后仍不足，designer以精确区间与真实物理合同裁定compute批策略或算法成本。改变deadline起点、把物理passes预跑到READY之前、用预热状态当初始场、拆root改变事务消费身份、调小实际工作量或延长250ms，均不能由architect/developer自行采用。可独立资源准备已由T480冻结为不消费物理状态；其它全pass预热必须先给可证明的scratch隔离/守恒/初态/退出合同，交designer与manager裁决。不得把deadline移到ACCEPT、fake pulse或豁免首root。

## 3. 首因被旧pulse再标记：确定的工程问题

本次最先故障为supervisor线程25768的 **GPU_PROGRESS_TIMEOUT**，owner为25769。随后页面旧session还在发pulse：首拒receive距上个receive约16,601,875ns，小于33,333,333ns；accepted历史native max17,778,125ns、UI max16,687,867ns。它们不支持本次首错是pulse GAP；反过来，较小已接受max也不能排除其它时刻真实GAP。

冻结PagePulseLocked在`progressV2 && !progress.open`时，只排除phase=CLOCK_GAP/NEW_RUN_REQUIRED，便调用ClockGapLocked。GPU_PROGRESS_TIMEOUT已先关闭open且phase不在排除表，于是正常间隔的晚pulse也能触发V2_PULSE_CLOCK_GAP/CONTROL_REVOKED，改写recovery展示原因。boot.firstFailure仍锁存GPU_PROGRESS_TIMEOUT；recovery.firstReason=WINDOW_OR_PAGE_CHANGED是全局初始事件，也不能当该root首错。boot2含512 recovery行、dropped10290；boot1为360行/dropped0。两份accepted PAGE_PULSE均18，boot2 rejected应为450，不是449。

工程修订：把首次撤权scope/原因/时间/原始切点锁存为不可被尾部覆盖的独立记录。终态关闭后的旧pulse仅返回原失效原因并计有界late/stale统计，不再进入ClockGapLocked、不重复撤权或改变permissionRevision；真实GAP仅在**当前有效且open的session**满足既有比较条件时触发。保旧接口返回兼容与原firstFailure，增加原因字段不能重命名历史GAP充成功。native需修正该入口，即便9565704页面已停止同代FAULTED_REVOKED pulse，状态读取与native撤权之间仍可能竞态；UI修复不能代替native终态幂等。真实GAP、deadline、hide/window更代首因分别保留。

## 4. 第二次重置未进入native：精确补证

boot1/boot2均transitions=6、droppedTransitions=0，只含首次UI_RESET epoch2→3，之后recoverySerial1/epoch3/run1/session1未变。现有UI截图/滑动记录不能绑定“新空场”按钮回调与native调用；recovery大量丢弃也不能抹掉独立transition记录没有第二次RESET这一事实。不能据此判GPU旧任务退休死锁，不能据此判页面按钮失效。

冻结GpuSandWaterExperience.restart先停旧frame，model.reset无条件port.newRun，再建frame/bindRequested/postcallback；“新空场”按钮接restart，native newRun应写UI_RESET。**缺的是本轮实际入口是否执行及回执**。developer补最小有界actionSerial：按钮handler进入→restart→newRun发起/同步返回或异常→native request receipt→epoch/reset状态→旧frame取消/新frame bind。记录各端时间域、旧新identity；不能把自动重试当用户第二次操作。

tester/effector后续带“真机”标签，同设备串行，安装核身份后显式两次新空场，保存按下视频/动作回执与before/after恢复快照。若无handler则交互路径；有handler无native请求则ArkTS桥；native收到却无RESET/退休推进则带具体oldRoot/lease/capture状态回architect。RESET写出而BUSY则按T464退休合同记录drain enter/exit与失败，不猜死锁。设备等待不是阻塞；本轮历史缺证不能补造，也不要求重开已交付旧卡。

## 5. 接力检查与交付边界

developer必要非机检查用真实生成bridge/页面frame：在截止前ACCEPT但Swap晚到必须拒且不回滚已接受物理提交；deadline等号边界沿既有比较符；GPU超期后连续/竞态旧pulse不覆盖首因、不重复撤权；当前open的真实GAP仍故障；错session/旧callback不消费quota；两次显式reset与hide/show新session，旧root/lease/capture有界退休和迟到尾部拒；首错后的Swap探针可保留但不放行。compile/签包及本角色必要检查完成即交付，不加独立真机关闭门。

tester沿T491及另卡补本稿的首root成本/第二次reset证据；现场保首次冷启动和暖启动分项、原始ns、完整scope、max GAP与触发stamp。不能用平均pulse或30fps显示替代33,333,333ns控制GAP，不以120Hz quota要求120fps；显示目标30fps。250,000,000ns root期限、permit贯穿呈现、守恒/确定选对/事务和设备证据均不豁免。补证先回architect分流；未有场景细分前只记成本未知，不派无依据算法缺陷。

所有执行prompt：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用既有oracle。背景/前景全部XComponent+shader，禁止Canvas/软件GL产品回退；故障FAULT或标STALE上一完整有效帧。成功原卡回执并本人行结束待验收；失败/关键缺证写未完项、复现原错、证据、已尝试、不能自行继续原因、所需前置与接手角色，本人行看评论。

本卡已完成旧包身份、真实ACCEPT与晚期呈现拒绝、单位纠偏、终态pulse工程缺陷、第二次reset证据边界及角色分流。细分驱动成本与真实第二次操作仍待上述独立证据，不记产品PASS；本轮未实现业务代码、未使用真机、未改期限或测试报告原文。
