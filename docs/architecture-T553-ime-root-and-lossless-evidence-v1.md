# T553：IME准备域、后继root和严格系统PNG的分流

> 2026-11-17 T570修订：单沙IME/STALE因果以 `architecture-T570-prepare-stability-and-stale-v1.md` 为准。attempt10已证真实IME与keyboardAreaSeen；prep-74为固定键盘声明UNKNOWN，prep-40/41有真实native run换代且没有READY permit，不能直接归为blur仅推serial。确认后的后继STALE仍需同attempt首失效见证；本稿其它root/PNG已证边界不改。

2026-11-17，architect，不使用真机，不重跑产品测试。消费T408 attempt9/T510 v2正式报告与原件、T540/T537/T536生成链及签包。三个域独立处理，不以诊断三色图成功替真实物理或无损像素PASS。机器证据 `analysis-T553-evidence-v1.json`，可执行取证表 `architecture-T553-evidence-manifest-v1.json`。

## 1. 单沙：位置域已证，KEYBOARD_UNKNOWN仍需拆项

attempt9的T532_WINDOW_HOST_RASTER_V3实际整数框(51,990,1154,1644)与desired相符，保留已证位置域；旧attempt8不改判、不重拟合ROI/0.2px。READY/预约/HOLD/独立新标定未到达。

实际T540生成页是 `pages/P051DerivedV2Demo.ets`，PrepareReadiness在`ets/render`。其KEYBOARD_VISIBILITY_UNKNOWN是复合谓词：固定系统键盘声明、phone、windowMode1、四个合法WINDOW_PX avoid矩形、以及sessionEverEdited时曾见非零avoid。**仅凭UNKNOWN不能独立证明唯因“uitest inputText不弹IME”**；还可能缺固定键盘确认、窗口条件、注册/采样时序。inputText注入不是IME可见性的证明；absence avoid同样不能单独证明操作系统缺陷。必须保存完整PrepareSnapshot/输入焦点历史/API注册/错误原件，不能只保截断UI reason。

在同一windowId/windowGeneration内，observe真实非零avoid才设keyboardAreaSeen；窗口换代会重新核历史。不得native/UI直接写该位、用timeout抹掉sessionEverEdited、换页专门规避历史、假avoid矩形或用固定键盘按钮声明代替真实可见事件。固定键盘确认仅声明测试环境，仍须操作和观察证据。

### 可执行取证路径（tester，不先放宽门）

1. 串行使用T540正确签包；保PID/包/ELF、page/window generation和完整Prepare冷原件。先核固定系统键盘设置（非悬浮/分离、无硬件键盘隐藏条件）、窗口全屏phone、avoidAreaChange/keyboardHeightChange注册成功/实际回调及四rect原始值。数据缺项写NOT_OBSERVED，不能从未记录推API失败。
2. 先按本次bounds真实点击`t413-roi`（不是先inputText注入），观察onFocus/onEditChange及系统IME区域。保“未开→真实可见”的layout/原始avoid、keyboardHeight、时间和同window generation。需要实际屏幕触摸时由effector/真人执行同路径，不要求architect本轮用机；自动注入失败保自动设施边界，不假称所有路径不可能。
3. 若点击/焦点有效但仍无真实IME：同UIAbility内可用SDK26 `inputMethod.getController().showTextInput():Promise<void>` 显式请求。它要求已focus并绑定编辑框，不是在另一个测试runner进程调用就可控制目标app。不得凭Promise resolve置keyboardAreaSeen；必须仍收到真实avoid/height和系统区域原件。API的12800003/12800008/12800009及原错误完整保留。
4. 当前页无该显式请求入口时，交developer最小研究辅助入口 `requestImeForPrepare(index):Promise<ImeRequestReceipt>`，只在allowed/非HOLD的配置态、真实focus成功后调用上述平台API，输出page/window/input index/开始结束/原结果；隐藏时解除observer，不改变Prepare谓词/历史/预约门，也不绘制替代键盘。源码真源沿T540→T516→T469生成链，不能只修忽略目录生成件。
5. 真实IME出现后走既有`t413-prepare`：三TextInputController.stopEditing、FocusController.clearFocus，保实际blur/edit结束、avoid归零/height0/动画结束、后续didLayout/willDraw两帧锚及同代严格三框。仍在原3000ms准备观察界，不加时；只有原predicate全部过才READY→预约→HOLD→严格PNG→独立标定。

若同window真的可见IME但avoid/height无相应原件，architect先核observer绑定/坐标/生命周期再转developer；若API原件已明确平台不支持该模式，tester报精确环境/错误和真人路径结果，由manager决定用例支持域，不由architect把UNKNOWN改HIDDEN。没有真实可见历史而已编辑的attempt继续NOT_REACHED。

## 2. T537已含T536；实际原件不支持“后继root整体失败”

`test/t537/generate.cjs`直接调用`t536/generate.cjs`再overlay。逐位复算T537-m1-final签包SHA=`97e8e342d34431c64cc8c1286cc503271c1d61e82a062dc98f73c0f3cc24ae0b`；包内ELF SHA=`c65aab0751e68580ffe1e59ff11fcf06b50d9d3fe6a8912c57caafe8b8e2a6e1`，含T536_COLD_PREPARE_V1、全部八Q族和T537_INPUT_STAGE_V1。**本地实际包已包含T536，不再把“等待T536完整包”当本卡真实前置。** ELF包含代码不等实际设备该scope每个族都已成功执行；现场仍须加载来源收据和实际runtime阶段。

T510 inputStages同scope epoch7/run4/session2/permission4：Down到首NATIVE_OFFER为18,633,594ns；保留槽中确有五次NATIVE_OFFER与ENQUEUED_NOT_ACK回执。counts中的23是各阶段seen计数，不等于只发生五次；受保护的五条不能冒整次输入完整无丢失轨迹。保32×512B信用和dropped/retired计数，不扩环掩盖因果。

同一execution原件还有5153次ROOT_ACCEPT/ACCEPTED计数，最后一次同epoch7/run4 root5121接受、sourceCommit仍0；两个scope的首撤权分别是root33及root5122。故“root1成功后root都失败/尚未消费必因特化未交付”不成立。空场QUANTUM接受≠有沙水推进/30fps，但足以反驳笼统后继根不可达。

epoch7/run4最终firstCause=CLOCK_GAP，faultRoot5122：start87880695198752、deadline87880945198752、FIRST_REVOKE87880736579377，撤权距start41,380,625ns，**未越250ms根期限**。CONTROL_REVOKED原间隔34,047,344ns，超过33,333,333ns约714,011ns。它是有证据的GAP，不是有证据的GPU根250ms超时，也不是证明GL某调用吃满33ms。此撤权比Down晚约311.790s，缺少输入时段root/FIFO串联，不能让五分钟后的fault代替Down后首消费失败原因。原分组tail/firstQuantum有丢失和空列表，不能从全局first/last统计重建中间每根。

### 精确补证（T510原卡tester，一次保链）

沿正确T537包现有能力先导同一次attempt：操作前完整scope/ready/sourceOpen、FIFO head/count、sourceControlGeneration、输入代、当前plan/root kind；按一次Down/Up保raw32word、enqueueSeq/用户intent及具名回执；紧接收原boot/recovery/execution窗口，追同seq到G1 Claim/SourceRootOwner Begin→SOURCE completion/Accept或具名未入根原因。G1请求不能只用“ENQUEUED_NOT_ACK”当消费确认。保同刻允许谓词、fresh UI receive/sample、队头age、scope变更/归撤销/源码检查值、计划是否跳过SOURCE、SOURCE begin/retire及ledger。

优先复用已有T537 action/raw packet、T492 root/firstCause、quota/recovery和cold状态；若当前接口没携带FIFO head/intent→root关联，**只补定长观测缺口**交developer：`SourceClaimWitnessV1`含scope/intentSeq/enqueueSeq/queueCount/headAge/sourceGeneration/inputEpoch/planId/rootNonce/rootKind/eligibilityBits/reason及sourceCommit，输出一份现有诊断writer内的定长快照，≤256B且附加JSON≤2048B，仍守原2MiB writer/4MiB总信用、溢出完整INCOMPLETE。不得改变消费算法/队列、延长capture年龄或为日志唤醒owner；只有关联原件齐后architect才分source事务/调度/GAP/设备成本，不重开泛化日志卡或直接派shader拆分。

该窄见证按本次指定enqueueSeq锁定，保首次入队、首次claim或首个具名未claim原因及终态，不能被数千个后继空QUANTUM覆盖。只在scope退休或明确开启另一冷取证attempt时换代；不由原有Move尾环驱逐关键见证。取不到同seq原件仍UNKNOWN，不借最终CLOCK_GAP补链。

精确成本分开native API call墙钟、Submit/Poll/fence、GPU完成、UI pulse间隔、Swap前后和content commit；禁止相减跨域裸时间、用ASYNC pending当错误、用C2/别的scope推当前root。250ms与GAP不改，重试是新attempt不改旧开始时刻。

## 3. 严格PNG来源可行路径

snapshot_display在此次CLI仅导JPEG不等于设备所有截图API都只能JPEG。SDK26 `@ohos.UiTest.Driver.screenCap(savePath)`及带displayId重载明确是系统屏幕PNG接口（测试API，需实际UiTest runner环境）。tester先在已有ohosTest/UiTest设施用 `Driver.create().screenCap(context.cacheDir+'/t553-'+captureId+'.png', actualDisplayId)`，串行await，落原Promise结果/BusinessError、实际sandbox路径和displayId；再pull原文件。没有runner则准备测试runner/构建设施，不在业务页用FBO/readPixels冒系统截图。

实际成功资格仍是文件魔数`89504e470d0a1a0a`、PNG解码/CRC、真实尺寸/display与被测surface坐标、原始SHA/字节数及完整捕获区间；路径后缀或API文档不是运行成功证据。JPEG转PNG不会恢复无损源，严禁作为本项成功。错误401/17000002、权限/测试设施不可用具名保留，不能试无限种命令后放宽容差。

复用T406 capture.py封装截图命令与原始文件传输，保host调用前后与既有设备STEADY_NS映射证书/误差区间；整个保守区间必须在同token native hold内，不能用Date.now/mtime/两裸ns差配对。T537诊断hold仍2s、不续期；若文件获取过慢导致区间不合，优化预置runner/传输流程并作新attempt，不延长原hold。单沙继续T413既有maxHold/预约合同，不改其时钟/许可。禁止在截图过程中改变layout、颜色管理参数或逐点拟合ROI。

T510 JPEG三域只登记：FBO/default的RGBA8原件同值、Swap声明成功、系统JPEG有相应三色区域；±1差不能给零容差无损PASS。真实PNG后按原固定坐标/色彩契约直接校验；若PNG仍差，不预设阈值，应保存decoder mode/ICC/sRGB/gAMA/bitDepth、display色彩/缩放/坐标及native RGBA、同hold时间，再回architect分流。若平台确实只供无法配对或不保精确RGBA的系统PNG，manager用例裁决所需材料是原API错误/支持矩阵、原PNG/JPEG及元数据、时钟/坐标链、固定ROI差值分布和原阈值来源；在裁决前继续NOT_MEASURED，不能把±1写成新允许值。

## 4. 接力边界

本卡完成首次归因/精确路径/必要工程缺口定义，T408位置已证域保留，T510输入已证域保留，不篡改原报告或批量更新队列。manager可交tester继续已有能力取证；确实缺同进程IME请求入口或SourceClaimWitness才交developer实现上述窄接口，不默认重写业务。developer不执行测试、不用真机，仅开发/编译/提交；tester/effector真机任务须保真机标签、设备串行。

所有接力prompt共同约束：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它可做项。具体阻断原卡给未完/原错/复现/证据/尝试/不能继续原因/前置及接手角色，失败本人行看评论，成功结束待验收。CPU停投，仅新XComponent+shader，FAULT/标明STALE上一完整有效帧，禁Canvas/软件GL产品回退；defaultOFF、250ms/GAP/120Hz配额/30fps显示、准备门和PNG零容差均不变。不新增manager验收门。
