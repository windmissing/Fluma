# T505 · T495首root、SOURCE、reset与呈现证据分流

2026-11-14，architect，P5，不使用真机。只读消费T495八份boot/17图、T493正式报告、T490与实际签包来源。[证据索引](analysis-T505-evidence-v1.json)保存原件hash、scope/算术及包内ELF核对。本稿纠正报告推论，不改旧报告或旧测试状态，不重跑设备。

## 1. 签包身份与T493边界

T495声明安装的M1为T492-m1-handoff，HAP `a87579df0500e12a3b902a54d6e8b30d3f47d2b432bc397109e52ebd337818ef`，包内ELF `993eb255958d1646470e8078be5878e2d6714f63115df07b8fcc9c513dc63a46`。M2为T492-m2-handoff签包，HAP `de742561b49ea83c85c6a4f19b1bb7ae12b3dee239da3e8965a9ee9d327490b8`，ELF `e626b8f2a5ae290e313791c9d6af4a66487bf8818197c101e47cd07d84cd732c`，实际unsigned输入目录是 **t492-m2-handoff-v2**。本轮重算与T257收据一致。

ELF内嵌source声明分别为`dc2edb28191ae10a4170ed9b593280c9fc35339d+T492-overlay-v1`、`e2dc1ec0576563c1213acad8dec9b602814c9ab7+T492-overlay-v1`，并非T493原产物所述`dcf105d…+T492-overlay-v1`。git交付commit、生成声明、签包SHA和安装事实是四个字段，不能互相代替。现有boot仅声明buildProfile，没有完整source/包ELF安装回执；本轮证明本地签包内容，与“现场实际安装字节”分项。

T493的8项原件独立检查保留为**所检查离线产物**的覆盖；真实GAP、首因、TTL、reader拒页、reset/actionSerial和sourceGL首SOURCE→Q fixture均不是设备复证。后续按上述实际handoff源/ELF核相关helper一致性后复用覆盖，不称所有新签包已按同源逐项实测，更不把页文件出现当设备已导出稳定snapshot。

本轮已实际比hash：immutable snapshot和graphics factory在delivery/handoff相同，但`execution_diagnostics.h`由delivery的`4493f69416e8…`变为两个handoff的`c328622a56b5…`，App bridge也不同。handoff Point无callStart/callEnd，Group无firstQuantum/faultRoot、只保存首root；其Fault/Add不能保随后root2156的同根尾。增强delivery已有首SOURCE后首QUANTUM、faultRoot及后续首错尾修订，**不重复实施**。T500所核delivery签包为M1 `801c2d63…`/M2 `c6ba4032…`，不是本次设备声明安装的handoff包。补证明确选择准确增强包或承接它的新签包，不把“同叫T492”当相同功能。

## 2. M1 250ms是检测界，不能当GPU服务耗时

PID17196，epoch3/run1/context3/session1，K192/Kw128，1280×1471。有效attempt3固定boot前缀256行，**dropped81、complete=false**；recovery另表当前dropped0。原始值：

| 点 | native ns |
|---|---:|
| permit started（firstFailure.a） | 21358571055156 |
| ROOT_INTENT记录 | 21358571071875 |
| deadline | 21358821055156 |
| native控制检查detected（首错记录TID17196） | 21358821445468 |
| firstFailure记录 | 21358821445937 |
| 最终Draw回执T398_PRESENT_REVOKED | 21358831812656 |

deadline−started=250,000,000ns；detected−deadline=390,312ns；firstFailure−started=250,390,781ns。用本文件实际ROOT_INTENT日志点相减为250,374,062ns，约250.374ms。报告另一处ROOT_INTENT数值偏782ns须按原件引用。**250.37ms说明控制检查检测到许可超过绝对期限，不等于GPU计算服务用了250.37ms**，也不能据它断言任何有效首QUANTUM都无法在250ms内ACCEPT。该记录TID与PID同为17196，不冒独立supervisor线程取证。

root1 INTENT.kind=2，即QUANTUM，不是SOURCE。13对Flush日志合172,756,562ns；FenceEnter→Exit为25,766,562ns。这是native调用墙钟，含驱动/线程调度，不是GPU timestamp。最后保留的poll BUSY点位于期限前约25ms，之后ROOT_ACCEPT/Draw/Swap普通点可能因boot前缀满或terminal过滤缺失；不能用无行证明没有执行。

截图物理0:1、最近源收据#0/0.0000；boot.latest为root1的T398_PRESENT_REVOKED，frameTargetBytes15,063,040（恰8×1280×1471）。实际handoff App只在接受量增长后从Tick.latest调用Scene::Draw并记此回执；结合root1.kind2，**支持首QUANTUM已走到接受后的呈现路径这一源码推断**，不是“SOURCE接受1”的证据。精确ACCEPT时刻、Draw进入时刻和Swap前/后许可仍缺executionDiagnostics原件，不能给出剩余呈现预算数。rawEgl12288为EGL_SUCCESS，不是eglSwapBuffers成功调用证明；late回执距started260.7575ms只界定该native回执时间。

按T490合同仍将250ms从INTENT固定到最终呈现，不移至ACCEPT、不延长期限，不把失败保留前缀回滚。T495缺少新增executionDiagnostics及packet/action/reset档，不能唯一分摊poll、FBO/Scene、Swap或撤权后的清理成本。当前先精确补证，**不因黑屏直接派Swap缺陷、不把“GPU队列结构性必超”作为既定根因**。

## 3. SOURCE与reset：文件数量不等于操作数量

T495原件没有实际32U32 command/hash、Offer返回、SOURCE_ACCEPT、sourceCommit及对应requestSeq的同scope闭环。图片“当前：沙”是选择态，最近源收据#0/0.0000仍是零值；物理commit可由空场QUANTUM增长。点击坐标转述不能证明pointerDown命中XComponent、坐标转换有效或成功送源。因此“显式源→Offer→accepted SOURCE→Q走通”保持 **NOT_MEASURED**；不把它转成已证产品拒源。

三个完全重复组：

- M1 boot-reset1/2：53,876B、同SHA `0e4f7552c29b9147…`，同PID17196/epoch3/session1，transition共6、只一次UI_RESET epoch2→3。
- M2 boot-m2-reset1/2：13,118B、同SHA `0dbd45610dd92bdf…`，同PID22680/epoch3，publishedRun0、session0，尚在创建。
- M2 boot-m2-gap/rebind：139,951B、同SHA `4e5f031dc6b47cd8…`，同PID22680/epoch3/run1/session1、root2156、相同firstFailure与尾部。

它们可证明重复保存了同scope诊断，**不能证明两次显式新空场成功或rebind已发生**。无actionSerial、native reset receipt、新epoch/session增量和按键前后时序，不能断言按钮坏、旧frame继续发，或旧context退休失败。T492离线case18仍保其范围，不能填补设备缺口。设备补证必须在按键前后主动调用现有executionDiagnostics/Read回执，不再仅导出启动boot。

原件`t495_layout.json`另给出可执行的测试工程线索，屏幕坐标域1280×2832：

| 节点 | bounds |
|---|---|
| 导出启动诊断 | [416,164][864,304] |
| 沙 | [178,651][346,791] |
| 新空场 | [822,651][1102,791] |
| XComponent | [0,1081][1280,2552] |

报告的“新空场(731,263)”若在该像素域注入，实际命中**导出启动诊断**；“投沙(147,263)”既不在沙按钮也不在XComponent。这与同hash重复boot相容，优先交tester核输入命令/屏幕缩放与命中。缺完整原始注入命令和操作当时UI树，不能假设坐标一定未经换算而宣已复现误点；但也不能把这组坐标当有效按键/Down证明。下一轮按实际控件/实时bounds定位，保存坐标域变换，禁止沿用猜测的DP/截图缩放坐标。

## 4. M2长期推进与黑区分项

PID22680，root1 INTENT started21817909901718，首错root2156的GAP observed22045063948750；两者跨度227.154047032s，约3分47.15秒。截图物理747→1173→1892→2155和root2156确实支持同run长期有物理进展，不代表有效SOURCE或全程健康/完整呈现。

首错记录origin22045029381718、observed22045063948750，差34,567,032ns，超过33,333,333ns，支持该次真实GAP。recovery仅512前缀、dropped71342；boot256前缀、dropped165，前15个ROOT_INTENT窗口不能外推4分钟所有root耗时。现有档没有本scope的newPhysicalFrames、逐root实际Draw/Swap和native presented token账，也没有分层像素。不能把T489另scope的2441/2446 native呈现数移植来证明本run。

截图仅见沙水场景区域黑色，无系统像素分层/已接受源可视库存见证；空场本来可能没有可见沙水，场景光学/几何、源输入未入、FrameToken、GL/Swap、系统显示都有待区分。**不能从物理计数长链＋黑区直接锁定present独因，也不能据计数称compute所有数值门都健康。** 当前不是已证产品PASS，也不是未实现预测试RED。

## 5. 最小接续，不重复实现现成探针

tester/effector新补证卡，带“真机”标签，同设备串行；先核实际安装HAP/ELF与生成目录及上述helper版本。T492已有稳定snapshot、executionDiagnostics、recordDiagnosticAction/newRunAction及source packet回执，**先使用现有能力，不先增日志或扩大ring**；首QUANTUM/后续故障root尾使用已交增强delivery或消费其修订的新签包，旧handoff能取的packet/reset原件先保，不追认缺失增强字段。两个页面稳定导出入口在光学/诊断区域；启动boot与稳定导出不是同按钮。保存UI布局/控件文本、所点按钮、文件manifest及导出结果；若按钮不可达/文件写失败，给原错误和实际命中节点，再回architect，不用自动注入成功源绕过真实入口。

同一run最小四组：

1. 首root前后executionDiagnostics：完整scope/nonce/kind/started/deadline、Flush/fence/poll/ACCEPT、Scene/FBO各层、Swap前/actual/return/事后许可、firstCause及有界尾。保原ns/线程/时间域；正常预算与监督迟检分列。首错后立即导出，不等四scope耗尽。若driver调用未返只留ENTER，不猜内部耗时。
2. 先导出实际选择/Down/READY/坐标/32U32 hash及Offer，后核相同requestSeq、SOURCE receipt和sourceCommit，再独立核后续QUANTUM。源拒绝与未发源分开。不能用物理count或显示“沙”作SOURCE正控。
3. 两次**独立显式按键**：before/handler actionSerial→restart/cancel→newRunAction→native RESET_ENTER/RECEIPT→新epoch/run/session/READY，第二次序号必须增加。可短录屏或系统动作trace绑定scope，不能以两份同hash文件追认。无callback交互路径；有callback无native桥路径；native收到未推进才查具体retire blocker，失败先回architect。
4. 有明确已接受可视源后，同窗记录native current token/Draw/Swap/newPhysicalFrames与系统截图/短录屏；必要用现有层冷取证分辨几何/光学/显示。黑区、系统上屏、30fps、物理速度、GAP各自报告，不把120Hz quota要求120fps。

现有probe确认缺失/冷热接口确有工程断点时，developer另卡按首个丢失切点修；不改shader/数学/控制阈值、不把新真机卡加作已交开发关闭门。T493原产物覆盖及T491缓存结果按真实来源复用，精确补证先回architect。manager只登记/派工，不重跑、不新增验收门。

各执行prompt共用：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投。背景前景仅XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。成功原卡回执并本人行结束待验收；失败/关键缺证保未完项、复现原错、证据、尝试、不能继续原因、前置与接手角色，本人行看评论。

本卡完成来源、计时、接受/呈现边界、重复scope与源链缺证首次分流；不以未导出的现场数据冒通过。本轮不使用真机、不修改产品/测试或旧报告。
