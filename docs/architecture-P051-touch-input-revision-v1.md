# P051 触摸输入工程修订 v1（T83→T71）

日期：2026-10-26。architect。原卡 `6adef4bee4b03273ad7e4c91`。工程合同 `P051_TOUCH_INPUT_ENGINEERING_V1`；消费T69既有 `P051_TOUCH_TO_SOURCE_V1`，不改变干场物理或源率。状态：**确定工程接线缺口，文档交付供T71实施；产品仍待复测**。不使用真机。

## 1. 证据与归因

已读 `docs/effect-report-P051-T74-device-touch-capture-v1.md`、`docs/analysis-P051-T69-touch-source-routing-v1.md`、T72/T66开发报告及当前页面/model/native桥。T74固定commit545f45e，entry/src指纹5b34bc45ce9cc5b7，HAP SHA256 babc44c65f8fae41daecfd8f911b0bb53d01261a12d6e9c11bb1395061c9d440。五臂touch=0，内容区bounds=[51,624][1229,2650]内注入，tick与swap活跃，日志无丢失/断档；native `OH_NativeXComponent_Callback` 第四项为nullptr。T69已提出平台接线缺口，T74使§7.2第一分支成立。

裁定：**工程问题→T71接独立无绘制输入节点**，不再等重复设备补采或designer重新选路线。不修改T74原报告；其中R4按钮点击落在按钮外不能作reset失败证据，但不撤销其独立swipe和全臂L1失败。

当前共享页已经在T56工作中变化：renderer为P013SceneController，XComponent设置None、onTouch在外Stack；不能把它当545f45e现场源码，也不能认定这些未交付变化已经修好平台接收。T71须以本合同明确独立输入节点，避免仅移动parent handler后宣称完成。native桥仍空触摸callback这一事实不等于要求另写native事件桥。

## 2. 页面与文件所有权

| 文件/hunk | T71责任 | 与T56/T72交接 |
|---|---|---|
| `pages/EmptySandDemo.ets`：build内场景Stack | XComponent后放一个同框无绘制普通Stack，100%内容区，Block命中，raw onTouch唯一入口；移除源的parent/XComponent重复入口 | T56保留XComponent、onLoad/scene渲染和背景前景shader；共同页面hunk串行交接，以小patch合入，不覆盖整页。 |
| 新 `physics/P051TouchSourceAdapter.ets` | 纯逻辑单主指、逆映射、状态世代、停止原因 | 不绘制、不访问native/GPU、不建立第二注入队列。 |
| `physics/P051EmptySandDemo.ets`：start/move/stop只及只读输入状态 | start先有限值/域/active/failed校验，显式接受回执；暴露只读sourceHeld/stopGeneration，使页面能同步内部stop | 保留T59 dry接口/真实receipt/物理及T72事件hook；不得重写tick、gravity或WaterSandField。 |
| `physics/P051DiagnosticBuffer.ets`：typed event扩展 | 在既有event类型增可选输入身份/几何/原因字段；复用256 ring、sequence和dropped | T72被动诊断身份保留，声明inputRevision；不新建平行日志或无界数组。 |
| 页面syncActive/reset/area/frame尾 | 统一stopCapture(reason)，同步model内部stop | T56持有实际surface px、scene receipt、FAULT/STALE；几何变化先停输入再更新共用epoch，不分别递增两个epoch。 |

若T56仍在改同页，双方先传递具体patch/当前文件身份，再串行修改共享hunk；纯adapter可先独立实现。不用整页checkout回旧commit。T71开发范围不是完整scene实现。

## 3. 唯一几何与输入接口

页面保存 `InputGeometry{revision,geometryEpoch,widthVp,heightVp,ready}`。初始ready=false，不能因历史默认320×480而开门。T56渲染内容矩形与input同父、同原点、无独立padding/offset/border；实际onAreaChange确认有限正尺寸后发布**同一**几何版本给输入/渲染。若measure不一致，停源GEOMETRY_MISMATCH，不猜DPI。native surface px由T62/T56 vp2px路径负责，不拿px除local vp。

无letterbox时 `x=128*u/W`、`y=256*(1-v/H)`。域 `0≤u<W,0≤v≤H`，中心映射(64,128)，右界u=W拒绝，内部列归floor(x)。无效/越界不能夹取旧点继续。将来viewport非满框必须显式几何revision并使用其逆变换；当前不创造第二投影算法。底部y=0的准入仍由真实dry接口判断遮挡，不改为“床上方”。

Adapter建议边界：`handle(eventSnapshot, geometry, gate)`、`invalidate(reason)`、只读`held/pointerId/inputGeneration/gestureId`；eventSnapshot明确touch type、changedTouches/touches及local vp，不依赖ArkUI以便离线测试。model `start(x,y)`可升级返回`{accepted,reason,stopGeneration}`，原调用方需显式消费；`stop`幂等，内部stale/准入拒绝/failed同步推进stopGeneration。调用方不得把未接受start记成HELD。

## 4. 单主指与时序

状态仅IDLE/HELD；只有合法新Down进入HELD，锁主指id和run/geometry/inputGeneration。Down依据changedTouches选新增指，touches兼容分支需可诊断；不能每次Down都begin。第二指忽略，不抢主指、不双发。

Move查主指id，不用touches[0]；无主指Move不start。主指缺失/坐标非法或越界即stop。Up按released主指判断，非主指Up不结束；主指Up后不自动转移其它指。Cancel即使数组空也stop。Up若无法确定释放身份，fail-closed并报AMBIGUOUS_RELEASE，不用旧坐标猜主指。

每有效frame至多沿现有clock产生一次请求；静止hold无需Move，首样本0，后续8×sampleSeconds，qL=0，保留原clock上限与ns单位。原事件timestamp只作带单位raw诊断，不与frameNs或Date.now混减，不据诊断时间重算质量。Move不begin，释放后不追赶。真实receipt拒因原样保留，非NO_OP拒绝按当前干场策略stop；不绕过容量/遮挡，不补固定质量。

隐藏、后台、销毁、reset、resize/geometry改变、sensor stale、物理失败、model内部拒绝必须统一清主指/HELD/clock及未提交输入。恢复均须新Down。frame尾或模型stop通知必须同步外层HELD，不能内层touching=false外层仍HELD。普通stop不清成功源、池或床；reset先invalidate再沿既有新run reset。若新renderer为STALE/FAULT，不把旧帧当当前坐标证明：输入几何/前台/物理门仍独立判定；viewport不可信或epoch失配则停源。渲染错误不能自动回滚真实已成功源，也不在本卡私增“所有render fault必须重置物理”的策略。

## 5. 诊断复用与失败处理

RECEIVE放在独立输入入口校验之前，连空Cancel也记；记录事件type、pointer/changed数量、raw local、实际W/H、geometryEpoch、run/inputGeneration/gestureId/eventOrdinal、前台/ready门与sourceApiRevision。MAP/START/STOP明确接受/拒绝原因、映射坐标与主指；既有CLOCK/REQUEST/RECEIPT/PUBLICATION/RENDER继续相联。

不调用改变attempt/commit身份的getter。已有诊断字段不足则增typed可选字段，不把身份挤入无法解析的自由文本。保留环形容量、每次最多八行、两record/chunk与dropped；关键转换必记，Move限频但累计计数，若日志溢出明确证据不全，不伪造L1零事件。frame编号需注明page frame与model tick不同，不仅凭相等数字关联；真实receipt sequence/run/sourceEpoch为源证据。

渲染仅新XComponent+shader，含背景/前景，禁止Canvas上屏或失败回退。输入节点无背景/文本/边框/纹理/触点标记，不用opacity=0或visibility=None伪装无绘制；可见诊断若需要交shader。失败明确FAULT，或标明STALE的上一有效新管线帧，不冒充当前成功。

## 6. T74 swap与T66 FAULT为何不矛盾

T74测试入口是独立`EmptySandDemo`、TEXTURE id=p013_lmat_surface，在545f45e隔离HAP内有零粒swap回执；T66描述产品采集/`SandCanvas`场景缺完整T56 ScenePacket/FBO时显式NEWPIPE_FAULT。入口和能力范围不同，swap证明该次draw接口活跃，**不能证明完整背景/前景、非零源粒子、LastGood或屏幕可见沙成功**。

当前两页已出现未提交T56 scene调用，只能描述为工作区候选，不能倒填进T74设备证据。T82/T78后续记录页面route、build commit+dirty patch/source hash、HAP hash、pipeline/renderContract/sceneTransaction标志、frameToken/epochs/geometry及requested/rendered source，才可逐层对照。此身份说明不撤销T74已成立的L1首失败。

## 7. 回归与交付

T71开发/编译/离线自测：真实adapter＋model联测，Down一次begin、静止正源、Move-only零源、多指/非主指Up/空Cancel、无效坐标/未测几何/右界/列界、hide/reset/resize/stale停止且新Down恢复、不追赶；源成功后step失败保留已成功源。诊断开关同输入的物理结果一致；保留T59/T62/T72现有测试，不改冻结阈值或把stub命中当设备证据。

设备交独立T82（真机串行）：重跑T74同款五臂，先实测bounds，修正其按钮误点坐标；RECEIVE→MAP/START→正CLOCK→REQUEST→真实receipt→非零publication→新管线pack/swap→可见沙逐级报告。正源之后才测LAND/床累积、非空reset、松手/后台/Cancel、真实resize、重力动作；完整画面依赖T56，不把这个设备前置加到T71开发卡关闭条件。

T83工程文档交付足以供T71实施，不需另一个manager批准门。仅自己的本地行状态改结束待验收并原卡报告；不动T71/T56队列、滴答同名清单或历史报告。
