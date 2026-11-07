# T69：T63 触摸零源归因与 T71 最小修复方案 v1

日期：2026-10-26。角色：designer。原卡 `6adeac6ee4b03273ad764d35`。技术路线 `P051_TOUCH_TO_SOURCE_V1`。状态：**离线归因／条件设计交付，现场唯一根因未闭合；不是修复完成或产品验证通过**。不使用真机，不改业务代码、物理、测试判线或既有证据。本轮后段已消费T70交付及新建T72/T74任务边界，更新见§7.1。

## 1. 裁决

优先修复**输入事件接线**。T63 页面将 `.onTouch` 挂在带 `libraryname:'p013renderer'` 的 TEXTURE XComponent 本体上；对应 native callback 的 DispatchTouchEvent 为 nullptr，且没有其它源输入桥。这是与平台事件契约不符的确定接线缺口，足以作为首个修复对象。现场没有逐事件记录，故不能把“确定接线缺口”夸大为已经观测到每次手指事件在哪层消失。

唯一推荐：在同一渲染容器内增加**无绘制的独立输入节点**，用 ArkUI 原始 Down/Move/Up/Cancel 驱动现有真实模型；保留 XComponent+shader 作为唯一画面输出。补同链路诊断与严格坐标／生命周期边界，不先调注沙率、shake 阈值、LAND 或动力学。

| 问题层 | 结论 | 分流 |
|---|---|---|
| 事件接收 | 平台契约和源码存在明确缺口；当前 native 没有触摸转发 | T71 接独立输入节点；T70 固定版反例和新适配器证据 |
| 坐标／命中 | 原始 UI 树没有应用／XComponent 节点，缺实际触摸接收和 local 坐标 | T71 同内容区逆映射；新设备卡补精确命中日志，不从全屏像素倒推 vp |
| gesture slop | 该页面用 raw onTouch，没有 PanGesture／LongPress，不能归因于 Pan 未过阈值 | 不加 Pan 门槛，不改手势距离求通过 |
| 模式／sensor | 页面固定 EXPERIENCE；model.start 受 active/failed 门控；stale 输入会 stop | 保留门控与 stale 规则，记录明确原因；不因零源修改 T50 参数 |
| 时钟 | 源率用 FrameCallback 的 ns，首个样本=0，后续正间隔才请求 | 未见必须改单位的证据；T70 补静止按住的多帧正例与异常时间负例 |
| 请求／准入 | 隔离版直接 injectGestureSand，却忽略返回值且未接 receipt sink | T71 显式报告 null／拒因／成功；不能把零源直接解释成 NO_WATER |
| 渲染／LAND | 有零粒 draw/swap 成功，首粒未形成 | 保留 T62 尺寸门设备 PASS；投影／LAND／非空 reset 仍未验证 |

## 2. 强制渲染边界与版本

执行本任务及 T71 时：**所有画面渲染仅走新 XComponent+shader 管线，禁止 Canvas 上屏或失败回退，包括背景／前景。** 失败显式报告，或保留标明 stale 的上一有效新管线帧，不能冒充当前成功帧。

本方案的输入节点没有 Canvas、背景色、纹理、文本、边框、粒子、触点标记或其它绘制；只承担命中与事件状态。它不是替代 renderer，也不遮住 shader 画面。若需要可见触点／事件诊断标记，也须由新 shader 输出并携带 token，不能在输入节点画点。

T63 原页面有 ArkUI 背景／状态文字；本任务不将历史页面宣布为满足新强制合同。T68 负责历史渲染合同与用例的显式版本修订；T71 与它按页面 hunk 协调，不能恢复旧 Canvas。T64 的历史 Canvas 回退修复建议已被用户最新约束取代，不是 T71 的实施依据。旧报告、旧测试保留历史身份，不能静默删除以制造通过。

## 3. 固定基线与原始旁证

- T63 独立快照 `86366dd416c7d4b9dd66d4d9bcdbf5a6c4fa23ff`，读取 `D:/hmprojects/t63-build`；归档 `test/artifacts/t63-demo-retest-20261026/source.zip` SHA256 `782FDF322D04DA329DC8CFCB0EFF799579B7B6EC4735044C59863057608141B0`。
- 测试 HAP SHA256（T63报告）：`6AAC39F187C5A74E992CD1EBA45258B56728F44E227512936F233D9EAE7D227E`。页面／model／clock／gravity／WaterSandField／native bridge 六文件均逐字节匹配归档与隔离构建目录；逐文件 SHA256、日志样本复算保存在 `docs/analysis-P051-T69-raw-evidence-v1.json`。
- 主工作区页面、model、WaterSandField、native bridge 已不同于该归档，不能用后来 T59 干场改动解释 T63。clock 和 gravity 读取时仍匹配；这个“读取时相同”不承诺后续共享工作区不变。
- 输入报告 `docs/effect-report-P051-T63-render-repair-v1.md`；原 actions 显示中心滑动起于09:08:20.479、右壁09:08:28.967、09:09:31用户确认手动触摸／倾斜摇晃完成，09:09:58 reset，之后 Home/恢复。600px/s 属 T44，**T63 两次自动滑动是200px/s**，不混用。
- PID31761：27/27 采样帧 code0/swap=true，8个model抽样；PID32518：111/111成功，31个model抽样。两组粒子计数、sourceEpoch、源H/L、expectedMass、particleMass、depositedMass的采样最大值均0；真实surface为1178×2026。
- 39个model抽样 input.source 都为 sensor，shakeInput最大0；主run有效角范围约[-5.661434,30]°。不能从稀疏抽样推断全程无stale或全程shake恒零，更不能签完整手持轴／强度标定通过。
- 原 controls.json 没有应用／XComponent节点，不可充当容器权威命中边界；截图与视频亦没有逐事件或逐commit触点水印。

本轮只对归档源码和已有日志做归因复算，没有重跑产品测试、设备或独立客观测试。日志中完全相同的重叠行只为样本计数合并，raw原件不动，不涉及删物理账本记录。

## 4. 已证实的输入断点

### 4.1 平台契约与源码对应

[OpenHarmony XComponent 事件文档（6.0分支）](https://github.com/openharmony/docs/blob/OpenHarmony-6.0-Release/zh-cn/application-dev/reference/apis-arkui/arkui-ts/ts-basic-components-xcomponent.md#事件)说明：配置 libraryname 时，触摸等事件由 C API 侧响应；这与不带 libraryname 的 XComponentOptions 方式需要区分。

隔离版 `pages/EmptySandDemo.ets:98–101` 使用带 libraryname 的构造，将 onTouch 直接挂到该节点。`cpp/p013renderer/napi_bridge.cpp:159` 注册 `callbacks{Created, Changed, Destroyed, nullptr}`，最后一项正是 DispatchTouchEvent；该文件没有其它 Touch 分发路径。本机 SDK26 的 native_interface_xcomponent.h 也定义了此 callback 与 GetTouchEvent，但产品未接。

所以“native 绘图能跑”并不意味着 ArkTS touch(event) 能收到事件。onLoad 和 draw/swap 成功只验证了加载／渲染路径。平台文档加源码足以确认契约不完整；实际设备命中、事件数量及首个丢失点仍需新日志核实。T70 的 host 夹具不能模拟一个声称真实的 ArkUI 分发器来替代设备证据。

### 4.2 下游并非必然零源，但诊断缺失

隔离版 model.start 先检查 active/failed，再 move、touching=true、clock.begin。model.tick 若 touching，用 clock.sample(frameTimeNs,'ns')；正秒数产生一列H=8×seconds、L=0、sourceId=51请求，立即调用 injectGestureSand，再 step。

`P011GestureRateClock` 的首样本返回0是反突发设计；只要持续按住且跨过两个有效递增帧，应有正请求，不能要求 Move/Pan 才继续供沙。Move 不应重置 begin，否则每帧首样本都为0。本机 SDK26 FrameCallback.onFrame 的 time 参数明确是 ns；现有 ns→ms→seconds换算无直接错误证据。

旧 model.reset 确实预置128列×160隐藏水；injectGestureSand 的 NO_WATER 检查是“所请求列是否有自由水”，不是“触点y是否高于水面”。后续水位/容量/池slot/审计也可能拒绝。原准入返回值被忽略，receipt sink未接，null预检也没有可辨日志。因此既不能把隐藏水解释成“所有时刻所有列必准入”，也不能把 NO_WATER 指定为本次唯一根因。T59 干场解决介质／出生语义，不能替代事件接线修复。

此外 start(NaN,y) 调 move 时可保留旧坐标，随后仍将 touching=true；原 Up 不分pointer即停，touches[0]也未锁主指。这些是输入边界漏洞，可在同一适配器内最小修复，不是 T63 零源的已观测原因。

## 5. 唯一推荐：无绘制输入层＋单指状态机

### 5.1 层级与所有权

保留 XComponent type/libraryname/native渲染绑定。与其同父容器增加后放置、覆盖相同内容区的无绘制 `Stack`（或等价普通节点）；显式 width/height=内容区100%，hitTestBehavior=Block，raw onTouch 仅挂在此节点。无绘制节点不设置 opacity=0 或 visibility=None 来“隐藏”，否则事件参与可能改变；它从一开始就没有可画内容。

其作用是让普通输入节点成为命中目标，阻断较低层兄弟的重复处理，**不依赖 native XComponent 的事件冒泡**。避免在 parent、overlay和XComponent三处同时调用start。[官方触摸测试说明](https://github.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-arkui/arkui-ts/ts-universal-attributes-hit-test-behavior.md)给出了 Stack 上层节点和 Block/Transparent 的分发规则；本方案选择不向底下透传，不以“透明外观”推断Transparent模式。

T71 默认不增加 native事件callback/N-API消息队列，不改T62 surface创建、swap、Float32投影策略。若后续架构必须直接使用native输入，应另版定义线程、队列、pointer/cancel、时钟及generation再迁移，不能同时启用两套源。

### 5.2 同一几何与逆映射

输入层与XComponent同原点、同内容尺寸，无border/padding/独立offset；记录同一geometryEpoch。尺寸未实际测量、非有限或≤0时输入门关闭；不能用初始化默认320×480接受触点。若二者测量不一致，拒输入并报 GEOMETRY_MISMATCH，而不是猜DPI或用全屏1280×2832。

设输入local坐标为(u,v)，内容vp尺寸(W,H)，模型cols=N=128、capacity=C=256：

`x = N*u/W; y = C*(1-v/H)`，输入域 `0≤u<W, 0≤v≤H`，物理域 `0≤x<N, 0≤y≤C`。

转换仅用同单位local vp与vp尺寸；不能拿local vp除以native px。surface真实px和DPI由T62控制器准备，内容完整铺满的归一化映射与上述坐标一致。若T68将来引入有效viewport/letterbox，应复用其显式viewport逆变换并升geometry revision，不能静默仍按整框映射。

有限值、域和pointer校验在start之前完成；右边界u=W拒绝，不能夹为127.999来掩盖错误。越界／无坐标／epoch改变即stop并清速率时钟，返回明确原因。v=H对应y=0虽是几何合法点，物理是否BIRTH_OCCLUDED由真实准入判，不篡改为水面／列中心／床上方点。

### 5.3 状态机

`IDLE → HELD → IDLE`。HELD只由新合法Down进入；单主指，不将Move当Down补发。输入状态随run、geometryEpoch和page生命周期绑定。

- Down：选择changedTouches中真实新增指；兼容只有touches可用的事件时须有显式fallback标记。当前无主指、running、geometryReady且model可接收才start；一次gesture仅begin一次，重复Down不重开时钟。
- Move：按primaryPointerId寻找对应点，不盲用touches[0]。合法Move只更新坐标；没有HELD／主指缺失不凭旧坐标继续注入。静止不依赖Move，HELD由帧tick持续采率。
- Up：主指释放才stop；非主指Up不结束主指。Cancel不要求触点数组非空，直接stop。多指不做多源，把忽略／拒绝原因记清；主指Up后其它指仍在不能自动接棒。
- 隐藏／后台／销毁／reset／geometry改变／物理失败／sensor stale：stop并清主指、clock和未提交输入；回来或sensor恢复均要求新Down，不重放旧手势／积时。
- 任何stop幂等，不改变已成功源或已落床库存。reset仍按显式新run语义清物理；普通Up/Cancel不能清床／清池。
- 模型拒绝start必须返回可辨状态，不能适配器记成HELD。模型内部因stale或失败停止时，其诊断需让页面同步清主指状态；不能保留一个外层HELD而内层touching=false的失配状态。

示意伪代码（非业务实现）：

```text
onRawTouch(e):
  emitReceive(e, currentRun, geometryEpoch)
  if e is Cancel: stopCapture(CANCEL); return
  if e is Up:
    if releasedPointer is primary: stopCapture(UP)
    else: recordIgnoredNonPrimaryUp()
    return
  if not running or not geometryReady: stopCapture(GATE); return
  p = resolvePrimaryOrNewDown(e)
  if p missing/invalid/outside: stopCapture(INPUT_INVALID); return
  q = inverseMap(p.localVp, contentVp)
  if e is Down and IDLE:
    if model.start(q) accepted: state=HELD, primary=p.id
  else if e is Move and HELD: model.move(q)

onFrame(frameNs):
  applyExistingSensorInputWithoutChangingT50()
  model.tick(frameNs, existingDt)    // existing clock, once per frame
  correlateActualSourceResultAndPublication()
  if modelStoppedInternally: clearPrimaryAndHeld()
```

### 5.4 请求与准入策略

继续每有效tick至多一笔同步请求；保留T36 clock与8单位/秒速率，不按事件数量计质量。源计时只用递增frameNs，不把TouchEvent.timestamp、Date.now、sensor时间与它相减；touch时间只作原始事件诊断。首个有效帧0，后续 `qH=8×min(1,ΔframeMs/1000)`，`qL=0`，与原 clock 上限一致；不在T71调整长帧上限或物理dt求效果。短按若在第二个有效帧前释放，可以没有源；用明确时序解释，不为证明首粒而补一笔固定质量。

T63旧湿场夹具继续测原injectGestureSand；实际修复集成若已包含T59，则沿其显式dry接口和mediumMode调用，不恢复隐藏水，也不改T59物理hunk。给请求标明接口revision。两者可作为独立身份对照，不能把旧湿场正例包装成新干场通过。

必须读取实际返回值：null标明NO_RECEIPT/PRECHECK_UNKNOWN，不伪造成NO_WATER；rejectedReason原样记录，已成功源只按admitted量计。遵循当前干场“非NO_OP拒绝即stop”的明确策略，要求新Down再试；不在同一帧反复重试或绕开容量／guard。旧湿场夹具的对照仍如实保留其旧策略。

## 6. 最小观测合同

新增 `P051_TOUCH_SOURCE_TRACE_V1`，与模型物理route和渲染pipeline版本分开。每条记录有runId、inputGeneration、gestureId、eventOrdinal、primaryPointerId、geometryEpoch、sourceApiRevision；禁止只有“touch ok”无身份。**本轮新建T72已独占被动日志／回执hunk**：本节作为其观测需求，T71复用其只读接口和事件hook，不重复实现第二套日志，不由T72顺手改变触摸或stale语义。逐tick source/stale和sensor投递计数应在固定缓冲内记录；热路径不逐tick拼无界字符串，溢出有计数并标明覆盖缺失。

| 阶段 | 必须证据 |
|---|---|
| RECEIVE | Down/Move/Up/Cancel、原时间和单位、changed/touches计数、选中pointer、local vp与实际内容vp、running/foreground/geometry门 |
| MAP/START/STOP | model坐标、接受/拒绝原因、主指、输入世代、stop来源（包含stale/生命周期）、一次gesture begin次数 |
| CLOCK | frameNs、sampleSeconds、first/nonmonotonic/invalid/stopped原因；start后是否进入持续帧 |
| REQUEST | sourceId、sequence、inputTimeMs、x/y、requestedH/L、实际接口revision；无请求时明确门控而非伪造拒因 |
| RECEIPT | 请求关联键、null或真实rejectedReason、admittedH/L、before/after sourceEpoch/IH/IL、成功准入计数；数值不截零／不改容差 |
| PUBLICATION/RENDER | 请求成功后的run/stateEpoch/physicsCommitId/mediumMode、实际sourceEpoch和源累计、liveCount；后续packet/frame/swap关联到该token |

用紧凑平坦日志，Down/Up/Cancel/首次拒绝/状态转换必记，Move和稳定tick限频但保留累计接收/请求/成功/拒绝计数和最后键。短复测的源事务证据不能只剩每60帧model抽样；不打印数组。不为诊断额外调用会分配提交序号的旧getP011Metric，不用fake ledger key填metricCommitKey=null。

发生源成功但后续step失败时，应先记录真实外源发布和准入，不把已成功源回滚或抹为0；输出具体step错误。渲染成功、源成功和触摸成功是独立状态：零粒swap只标rendererReady；不能用“按住发沙”提示语当作事件／源已经成功的证据。

状态机／逆映射/计数 O(1) 时间与空间；每frame仍一次原事务，模型事务成本沿T59／旧物理实现，不复制扫描池来做触摸判断。固定大小诊断状态加限频输出，不保存无界触摸历史，不额外建注沙队列。

## 7. T70 补证需求与实施前反例

最初分析时尚无T70报告；本轮后段已收到并消费，结果与限制见§7.1。下列是所需证据合同，未交部分作为具体补充需求；不把已交T70任务重开或重新验收。T70不使用真机，设备缺字段如实MISSING，不合成实际输入事件。

1. 固定86366dd快照：页面构造/onTouch目标、native callback第四项、未有其它转发的源证据；host能够确认绑定缺口，但不能声称已经模拟ArkUI系统分发。
2. 真实旧model正例：合法湿列点start→两个递增ns帧，应出现正request／真实receipt／sourceEpoch；记录x/y、质量、回执和首个成功publication。另做stationary hold证明不依赖Move。
3. 真实旧model反例：无start只Move、每帧错误begin、首帧0、同／倒退／非法frameNs、stale、inactive/failed、start(NaN,y)、Up/Cancel空数组。哪些原实现失败就保留RED，不静默修模型。
4. 实际injectGestureSand独立正例和NO_WATER/CAPACITY等可达拒例，报告null路径；未到注入与注入拒绝分开。当前干场若已到则单独revision测，不拿介质切换掩盖触摸问题。
5. 对本方案提出修复后适配器用例：same local geometry、中心/右界/内部列界、多指、Up/Cancel、reset/Home/resize、计时单位与stop不追赶；所有平台／sensor/fs替身边界显式说明。
6. 现有raw设备日志缺RECEIVE/START/CLOCK/REQUEST/RECEIPT，controls无应用节点；列入后续设备补采，不倒推“Down=0”或“准入拒绝次数=0”。

T70负责客观反例与记录，不接整体归因。缺旧现场事件日志不能要求tester重新发明历史现场。T69不自行更改T70/T71状态或解锁依赖，manager据交付调度；按本轮新增卡的明确边界，先由T72被动诊断→T74现场补采，再按§7.2决定T71业务修复，不将本方案交付写成已经定位唯一现场根因。

### 7.1 T70到件后的证据裁定

已读 `docs/tester-T70-touch-zero-source-evidence-v1.md`、两个执行器及三份JSON结果；未重跑或修改tester资产。

- 工作区模型夹具报告7/7 PASS；快照夹具A01/B03/D01报告3/3 PASS。A01源H=15.866666666666678、P=15.866666666666578、firstNonzeroRow=1，说明被测模型路径可形成正源；B03/D01证明stale停止机制可产生零源签名。它们不证明设备现场一定发生stale，也不授权取消该门。
- `p070_t63_snapshot_model_check.cjs`仅对P051EmptySandDemo和P051GravityInputAdapter执行git show替换；其它.ets依赖从共享工作区读取。WaterSandField等已不同于T63归档。因此“两个入口文件为精确快照”成立，**全物理依赖均为86366dd的闭合快照不成立**。若要把正例用作完整T63物理反证，应在不可变完整快照中补跑，并记录全部依赖hash；不要求designer替tester重跑。
- 7例JSON中A01的depositedMass=0，判定只检查sourceH>0且P>0；3例JSON未记录床量。报告中“deposited>0／准入物理阻断排除”不能扩展为LAND通过或排除设备特定坐标／库存／容量拒绝。模型正例只能说明该条件可行，不能从“没有安装receipt日志”推导“没有请求到达”。
- T70逐阶段合计106个抽样包含重叠日志；最终全窗口与本轮整行合并均为39个独立抽样（8+31）。106是阶段行数总计，不能当106个独立时刻。两种计数结论并不冲突：所有已见抽样source=sensor且源量0，间隙仍未知。
- T70报告已明确事件接收／坐标／短时stale现场不可判；本轮加入平台文档与native空callback，补出确定接线缺口。它提高输入边界优先级，仍不能替代真实命中与callback日志。T72需逐tick stale命中计数、sensor投递计数，以及原事件→start→请求→回执链。

上述限制属于本次designer证据解释，保留T70的执行完成登记，不改其报告／结果，不另设manager验收门。

### 7.2 T72→T74后的T71实施分支

T72 `6adeb1e9e4b01278329aa00f`只补被动观测，不接本方案的输入节点／业务状态修复。T74 `6adeb1e9e4b017ab33f703e0`按现卡先等T72与T73相关硬前置，固定T59干场候选取证，**不等未知T71修复**。T74测的是新的固定候选，不能与T63混成同一run或宣称精确重现旧湿场；共同接线hunk须列hash比较。

| T74首个可证失败点 | T71按本任务书执行的范围 |
|---|---|
| 活跃页面、内容区命中证据充分，却无RECEIVE，且仍是libraryname＋空native触摸桥 | 实施§5独立无绘制输入节点；后续完整链复测，保留设备前后差异 |
| RECEIVE已有，MAP或start被几何／指针／active门拒绝 | 按明确首拒修§5状态／坐标hunk；不能仍把所有零源都归给XComponent吞事件 |
| start接受，随后stale→stop | 保留既有安全停止；查实际sensor投递间隔／生命周期／时钟证据，若需更改T50策略另回designer裁决，不取消250ms门 |
| 正CLOCK无REQUEST | 按实际touching/failed/时钟状态定位具体漏接，禁止凭空补固定质量 |
| REQUEST已有，真实receipt拒绝 | 路由具体dry/湿接口、坐标遮挡／容量／审计错误；不以输入节点替代物理问题修复，不泛改NO_WATER |
| 正准入与源发布已有，画面仍空 | 转新管线packet/token/合成路径，不再声称触摸零源；沿T68新管线合同排查 |

T71唯一条件方案已可审阅，但现场实施分支待T74；本轮报告为**离线部分完成／现场待证**，不是用设计文件解除现有实质前置。

## 8. T71 精确文件／hunk任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容；仅最新有限例外允许改自己行状态。不改滴答同名清单。共享工作区中不得撤销他人修改，按当前T59/T62/T68版本定位hunk。所有渲染仅新XComponent+shader，禁止Canvas上屏／回退，包括背景前景；失败显式故障或标明stale的新管线旧帧。

| 文件／hunk | 本卡所有权 | 不改的责任区 |
|---|---|---|
| pages/EmptySandDemo.ets：build输入节点、touch入口、geometry输入就绪门 | 按§7.2分支移除XComponent上的源onTouch；增加同框无绘制节点、单入口分发；接T72既有原始接收／几何诊断hook，坐标与内容区一致 | T72日志hunk、T62显式surface准备／T68 shader背景前景及stale合同；禁止覆盖整页 |
| 新 physics/P051TouchSourceAdapter.ets 或同职责纯逻辑小模块 | 单指状态机／逆映射／epoch/生命周期失效、显式start接受状态；不依赖ArkUI runtime以便真实算法测试 | 不含绘制、不生成假粒子／ledger、不重写gravity |
| physics/P051EmptySandDemo.ets：start/move/stop/tick输入状态hunk | 合法坐标在start之前校验、内部stop与页面同步；复用T72实际请求／receipt／stop只读接口，保留frame clock节奏 | T72独占日志／回执hunk；T59干场initialize/injectDry事务、动力学、容量／预算、真实LAND不动 |
| pages/EmptySandDemo.ets：syncActive、reset必要hunk | 隐藏/reset/geometry变化清主指，不重复start；复用T72同run/generation与publication关联 | T72 tick/sensor计数日志hunk；不改T50传感器门槛／滤波／订阅间隔，不把抽样shake=0“修成”非零 |
| 开发离线自测与交付文档 | 接T70反例，补适配器和真实model联测；记录当前构建commit/差异/结果 | T70原证据／脚本、T63报告、raw、冻结判线不改；旧版本用例变更须显式标版本 |

默认**不改**P013RenderController、native bridge、WaterSandField或HOT粒子实现。若T70实际反例暴露具体源事务失败，应按该失败另列hunk，不借本卡顺手修物理。仅移除既有页面背景的工作仍由T68明确装配边界；T71不能给输入层新增任何可见背景。

开发完成可量化要求：

- 一次合法Down只begin一次；只Move不begin；静止hold跨两个有效帧后正请求，低于容量时有真实成功准入与publication。保持8单位/秒速率及原时间上限，按同序计算，不新增误差容限。
- (u=W/2,v=H/2)→(64,128)，右外界u=W拒绝，内部列界按floor(x)单归属；0/NaN/未就绪尺寸及无效start不发旧坐标源。
- 单主指接收一次，第二指不双发，非主指Up不错误终止；主指Up/Cancel/失效后成功源累计不再增加，已有粒子可继续运动／落床。
- 隐藏/reset/resize/stale清源且恢复不追赶，必须新Down；phase/active/failed门真实报告。源成功后step失败不抹除源事务。
- 新source trace与真实receipt/sourceEpoch/源累计相符；正源的包／GPU结果仍使用同发布token。尺寸准备、非法源拒绝、Float32表示策略不回退。
- 编译通过、必要离线自测通过并报告commit。平台命中／合成／实际落沙仍交独立真机卡，不增开发卡设备通过前置。

## 9. 修复后同用例设备条件

由manager安排带“真机”标签的串行tester/effector卡。沿EC-T52-01~05及T63动作：两次冷启、中心(400,1200)→(850,1200) 200px/s、右壁(1160,1200)→(1220,1700) 200px/s、用户按住约5秒并移动。须先记录本次内容区实际屏幕bounds，若布局改变不盲点旧坐标，另给同local目标与实测屏幕坐标及变更原因。

按同run链核对：RECEIVE→MAP/START→正CLOCK→REQUEST→真实RECEIPT→sourceEpoch/源量→非零池publication→新管线pack/swap→屏幕可见。只有前面已有证据仍零源时，才按具体首拒原因继续分流，不能再只收每60帧摘要。

正源形成后才覆盖：右壁／内部列界量化投影、state2/LAND同事务、松手后不增源、床累积、非空reset、后台不续发、Cancel、真实resize。手持倾斜保留原未标定身份；需要高频shake证据另补，不拿有效角变化或抽样最大0签完整摇晃成功。最终画面必须由新shader，包括背景／前景；失败/stale与当前帧身份分别展示。

T63保持部分完成／FAIL_TOUCH_NO_SOURCE，F2、LAND、非空reset等未覆盖项不改判。本轮完成离线归因、T70证据解释、T71条件方案和复盘；现场唯一根因部分待T74，到件后按§7.2追加裁定。仅改T69交付状态并回原卡，manager据部分完成报告登记与推进依赖；不由本会话修改T71/T74状态或增加manager验收门。
