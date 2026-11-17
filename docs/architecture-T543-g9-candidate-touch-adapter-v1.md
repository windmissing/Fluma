# T543：G9_BALANCED_R1 与一次UP拨动工程闭包

日期：2026-11-17。architect；不使用真机。ENGINEERING_READY_NOT_IMPLEMENTED。输入为T534正式MD/JSON、manager已批准的显式研究比较规格，以及T529 `e429ea7`源码/开发报告。本稿和 `architecture-T543-g9-adapter-manifest-v1.json` 是developer接力合同，有限整数/位型例见 `analysis-T543-contract-checks-v1.json`。没有运行产品测试、物理模拟或真机，不宣称标定、观感、30fps通过。

## 1. 现状与版本裁定

T529仅SYNTHETIC模型域：`InputGate::ModelEnvelope` 用native now写capture，`Pose`为native人工姿态，`Offer`只收clockDomain=2/source=2；不能把它们称系统触摸/传感器采样。`g9ResearchConfig` 的candidate1为g16Q、muStop16384、bandQ、settle384；bridge传空seed，得到空床。因此它既不是BALANCED参数，也不是128列S40初态。保留该显式旧研究模式及其独立收据，不能改标题冒充新候选。

新研究配置用独立 `G9_CONFIG64_V2` / 参数revision2；INPUT仍128word、物理state/event/包络大小不变，但输入语义升 `G9_INPUT128_V2`。冷payload/page/capability升 `G9_SPF_COLD_V2` / `G9_SPF_SEGMENT_PAGE_V2`。新layout/shader/engineering SHA使program-cache key自然换代，不复用旧binary冒新逻辑。默认OFF，只能显式选择研究候选并新建run；热改参数/seed/adapter/clock/坐标策略要撤销旧run/inputEpoch、队列、permit/lease。V1只由显式legacy研究入口读写，V2 reader可按版本解释V1 SYNTHETIC，但不能据V1证明真实adapter；旧reader遇V2必须拒收。V5候选与本卡无依赖，仍等T517完整，不复制V5 shake/E为G9力学。

## 2. 参数与唯一初态

G9_BALANCED_R1：N128、Q1048576、CQ256Q、K512、Ls/Lf64、origin上限4096；g536870912、muStart32768、muStop24576、band262144、entrainRate262144、shear65536、settleTicks576，Vmax1024Q、packetMax244666、J={1,2,4,8,16,32}不变。T534候选对象canonical SHA=`41192a7852e4f695cb961437dc72fc829ef19b754c6ef3820e31f8103bd2f925`。此SHA是设计对象身份；不得与native参数胶囊SHA混名。候选§6研究门用于测量，不抬旧门槛、不称产品PASS。

固定初态 `G9_SEED_S40_ORIGIN1_R1`：128列每列S=40Q=41943040，F/P/p/rest=0、bedDepth=1、flowOriginCount=0，每列第0个bedCohort=(origin1,hi0,sourceId1,mass40Q)，其余63层/全部flow/packets/命令胶囊为0。origin在run内分配，完整键=(run,inputEpoch,origin1,sourceId1)，不能跨run复用权威。sourceRow0=(origin1,hi0,sourceId1,SEED_KIND0,admittedLo/Hi=5120Q,seq0,input0,digest0,revision1,flags0)，其余4095行0；header originCount=1、nextId=1、totalMass=5120Q=5,368,709,120（64位，不截U32），全部ledger/消耗序号0。这是初始存量登记，不产生伪SOURCE事件或source commit。

模板恰为K512 state673792B，沿T529 reader_schema的原offset。模板填确定物理参数/布局字段，run/owner/context/inputEpoch及parameterSha字段规范0；`seedTemplateSha256`覆盖整个LE模板。Config64纳入模板SHA，因此模板不得反过来含Config64SHA。native新run填真实身份与parameterSha后，**再**对最终实际上传字节计算 `actualSeedSha256`；修正T529现有在填header前hash的顺序，不把前像hash冒上传hash。五banks上传同一合法原件，沿已有冷seed上传数组，释放后再允许cold builder；不保第二份整state。首次CURRENT发布之前须有初始化GPU检查：沿已有ROOT_INIT，再单独 `INITIAL_SEED_MAP_MASS_AUDIT` 一次检查完整map/源ledger/初始总量/容量/规范零和实际参数，完成失败只FAULT，不先显示空床或非法候选。初始化共2 dispatch（比旧只ROOT_INIT多1），不计作仿真时间/接受quantum，不改普通root的2024/12调度。该初始化使用同4个SSBO和既有WORK，无CPU物理求解。

Config64全部LE U32，完整字段表见机器表。0magic=G9C2、1rev2、2profile、3K；4..10上述七参数，11parameterRevision2、12flags7（research+allowSynthetic+allowRealTouch，能力非已验证）、13seedRev1、14adapterCode53401、15policy1；16..23候选设计SHA，24..31adapter规格SHA，32..39seedTemplateSha，40..47批准T534规格文件SHA，48..63=0。`parameterSha256=SHA256(全部256B)`，不能继续用旧ParameterHash12word。任何单word变化换SHA/新run。NAPI请求64word，返回原样胶囊+hash+typed状态；GPU既有CONFIG128[64..127]原件复用，CONFIG[0..7]仍物理参数映射，16..23仍parameterSha，48..55 actualSeedSha。INPUT[25]=2，[26..33]=parameterSha。GPU核物理字段与胶囊一致，原scope/inputSHA/permit校验继续生效。

## 3. 真实坐标和帧身份

ArkTS只收集原API事件，不计算p/S/F/P；native有界input-only adapter做checked映射、整数配方和排队；GPU在已有CONFIG_INPUT_GATE复算后才允许物理消费。UI不得再经 `g9OfferModel` 给真实手势补now。新增typed接口 `g9TouchBegin(raw,expectedPresentedMapping)`、`g9TouchUpdate(raw)`、`g9TouchEnd(raw)`、`g9TouchCancel(reason)`；返回stroke/commandSeq、原始证据摘要与REJECTED/NO_IMPULSE/QUEUED，不把QUEUED当ACCEPTED或CURRENT。native持有唯一stroke/身份与16槽队列，不信任UI自称已验证的mapping/clock/scene token。

SDK26.0.0 `ets/component/common.d.ts` 的TouchObject x/y/windowX/windowY/displayX/displayY均vp；旧screenX/Y已弃用且是窗口坐标，不能当物理屏幕px。使用同一事件点的component-local x/y，加当前UIContext.vp2px的比例转物理px；保留window/display原位型作证据。Native XComponent入口的坐标若选用，须独立声明px来源/比例并绑定对应producer，不能与ArkTS vp混接。R1采用ArkTS DOWN/UP changedTouches中的同一事件点及同一BaseEvent.timestamp；不把touches中的另一硬件点与resampled changedTouches位置交叉配时。MOVE只审计/取消，**系统事件原值不等于硬件原始采样**，报告保留pointPolicy=DOWN_UP_API_EVENT_POINT_V1，不宣称未重采样。

仅新XComponent+shader成功Swap且完成同scope present许可的帧，才发布native PresentedMapping：沿T529 ControlIdentity原顺序的run/owner/context/inputEpoch/pageSession/permission/window/config/surface、geometry/viewport revision、参数/seed/adapter SHA、真实surface W/H、物理contentRect、完整scene token及commit/publication。G9沿run/inputEpoch表达reset换代，不凭空增加其它route的resetSerial字段。当前T529 uBox=(0,0,width,height)，即全surface，可各向比例映射；后续若加入黑边须用真实uBox并将GL下原点转为UI左上原点：top=H-(boxBottom+boxHeight)。不得用组件布局宽度猜已画区域。DOWN保存已呈现映射；UP要求同mapping scope、geometry、参数与rect（数值/位型完全一致），同时另存UP最新scene token/commit。普通同geometry的CURRENT提交允许更新，**不要求250ms手势期间每帧commit都不变**；失败STALE帧或换代时取消，不能从任一私有候选bank冒presented身份。

物理坐标(px,py)到模型的唯一binary64运算顺序：`x=((px-left)/width)*128`，`y=((top+height-py)/height)*256`；先各减/除/乘独立round-to-nearest ties-even，禁fast-math/FMA融合；delta=UPmodel-DOWNmodel也独立binary64舍入。保存DOWN/UPmodel和delta原位型，native复算并逐位相等后封INPUT。width/height必须有限正；screen点按 `left<=px<left+width, top<=py<top+height` 半开矩形，故模型x∈[0,128)、y∈(0,256]。上边y=256合法，下边y=0从API像素端排除；无接触位置clamp。另对UP x沿既有GPU接触位置RNE量化，若`RNE(x*Q)>=128Q`，即使像素仍在右边最后半个Q20格内也明确 `ADAPTER_TOUCH_GRID_OUT_OF_RANGE`，不排队、不clamp或等GPU才发现。T534闭区间是数值支持包络，本工程收窄到实际像素及既有接触格范围；不改变有效内点增益。DOWN/UP/MOVE任一点出界、覆盖控件/黑边、未知变换/旋转/缩放未体现在映射或surface尺寸不符均取消，不把屏幕无效区投到墙边。

## 4. 时钟与手势状态机

SDK BaseEvent.timestamp单位ns、系统启动以来；nativeXComponent timeStamp是int64 ns。ArkTS路径原timestamp必须Number.isSafeInteger且>0，立即拆U64低高word并保留binary64位型；超2^53-1不能用已舍入Number伪造精确值，返回INPUT_TIMESTAMP_PRECISION_UNSUPPORTED（未来可加独立native整数producer，不静默替代）。DOWN/UP必须同source/pointer、同已验证时域；UP-DOWN做checked U64，8,333,334≤dt≤1,000,000,000，不能0除、取绝对时间差或钳成假dt。

固定clock profile `OHOS_TOUCH_MONOTONIC_NS_R1`：event timestamp映射与native根用的monotonic clock同域、offset=0。实现层须以实际平台API/源与构建信息声明producer clock proof；SDK的“since system starts”一句不能独自证明steady_clock epoch。native应把项目Now/Stamp统一核对为CLOCK_MONOTONIC ns，记录原采样、回调native receipt、root freeze三种时间。上游参考实现中MMI actionTime使用CLOCK_MONOTONIC微秒、ArkUI转换至time_point（参考链接见机器表）；这是设计依据，**不能冒当前设备已验证**。启动/恢复/permission或context换代撤销proof；新proof generation绑定scope和平台source SHA。没有proof回INPUT_CLOCK_UNVERIFIED，禁止native now覆盖DOWN/UP，禁止从两次回调延迟拟合未知epoch或引入任意容差。若平台真实接口不是本profile，须先升版clock映射方案，不能猜秒/毫秒/纳秒。

每次收件都要求0<sample<=receipt；UP到Offer/Freeze/Claim的年龄均≤250,000,000ns且非未来（等号合法）；DOWN可比UP旧至1s，不能错用pose250ms把全部长手势取消。pose角度仍是clockDomain=2/source=2的native人工控制，独立新鲜资格；真实命令capture是UP的原系统timestamp，标记commandFlags.REAL_TOUCH，不能据此宣称pose来自sensor。人工pose是native持有的当前控制状态，允许在已授权fresh page pulse/Freeze时重新观察并stamp now，明确标MANUAL_STATE_OBSERVATION；不得修改target/control generation、制造meaningful事件或rearm BLOCKED，不得刷新排队命令UP时间。sensor输入没有此例外。传感器shake不接G9。

状态：IDLE→DOWN_ACTIVE→（MOVE采集计数/检查，零力）→UP一次排队→IDLE；CANCEL/第二指/重复DOWN/丢pointer/不同timestamp域/过期/rect或身份换代→CANCELLED→IDLE，无force。UP的active touches允许0（抬起后），changedTouches必须恰含原pointer一次；不能用UP touches.length==1作为错误前置。所有MOVE检查单指、坐标/时间非递减和区域；最多256次，溢出明确取消。保DOWN/UP全原件和MOVE count/滚动原件hash，不声称完整MOVE轨迹重放。单次strokeId=IMPULSE commandSeq为native U64单调非零，溢出要求新inputEpoch，pointer ID不代替strokeId；SOURCE序号另域。

## 5. 精确整数配方与GPU复算

仅有效UP建立一次IMPULSE；radius原binary64=4.0，referenceMass=Q/16=65536。对binary64 delta**精确解码整数位型**，`dxQ=sign(dx)*floor(abs(dx)*Q)`（dy同理），-0统一+0，subnormal为0，NaN/Inf、|dx|>128或|dy|>256拒收。不能先floor负数引入偏差、也不能以epsilon比较。dxQ²+dyQ²<(Q/4)²为NO_IMPULSE（相等进入配方）。

`vx=sign(dxQ)*min(24Q,RNE(abs(dxQ)*10^9/(4*dt)))`；`vy=min(32Q,RNE(max(dyQ,0)*10^9/(4*dt)))`。RNE使用整数商余数、2r与d比较、相等取偶数。`Jx=(Q/16)*vx`、`Jy=(Q/16)*vy`；`workCap=8*(Q/16)*(vx²+vy²)*2^24`。raw合法快滑速度饱和分别写SAT_X/SAT_Y（仅未限值RNE结果>cap才置位）；Jx/Jy全0也为NO_IMPULSE，无命令无consumption，故纯向下不变向上，斜向下仍可保水平冲量。只限input速度增益，不clip物理F/p或particle速度。

所有中间值用checked128 limbs；|Jx|≤1.5Q²=1649267441664，Jy≤2Q²=2199023255552，workCap≤800Q³*2^24=15474250491067253436239052800（<2^120）。SOURCE质量、能量、deterministic Hamilton分配、逐列功拒绝/回滚、CFL/选择/authority/replay不变。GPU既有CONFIG_INPUT_GATE/INPUT_AND_CFL利用raw model bits[50..53]与duration[54..55]按上式重新计算dxQ/dyQ、J、cap和饱和标记，与INPUT[56..63]/flags**逐word**核对，radius必须4.0；不接受只信native的物理J。GPU无FP64要求，用既有binary64位型解码和128位整数，不在GPU重新计算px仿射浮点。像素→model真实性由native/离线reader与原证据复算，只有两层都一致才可称真实adapter链。GPU rawDelta/J/cap任一坏值在修改候选物理状态前拒绝，旧CURRENT保留；不添加另一root fence/readback或第五binding。

## 6. INPUT128、去重与控制消费

V2完全保留原word位置，38/39与78..127仍0；不能偷用SOURCE64..76保存pointer/px/timestamp/scene，也不能借非本kind的word。NONE的40..127全0；SOURCE仅40..45、64..76有效，46..63/77=0。真实IMPULSE仅40..63及77有效，64..76=0，77=53401，45 bit0 SAT_X/bit1 SAT_Y/bit2 REAL_TOUCH（其它0）；43/44=真实UP timestamp，46=UP modelX，48=4.0，50/52=modeldelta，54=真实dt，56 signed64 Jx、58 unsigned64 Jy、60..63 U128 cap。V2 envelope[3]=0、[22]=2、[23]=2仍表示native人工pose；真实command身份通过77/45识别，不谎称整个envelope来自sensor。合成显式J仅独立入口V2 adapter0/flags0（仍明确SYNTHETIC）；新配置内不接V1 capsule冒真实。冻结nonce/receipt/freeze仅写原[10..11]/[18..21]；Pose/Freeze不得覆写command[43]。

GPU配方复算仅适用于kind=IMPULSE、adapter53401且REAL_TOUCH置位的完整合法组合。adapter0/flags0走原显式SYNTHETIC J边界及work审计，不把它反解成Flick；NONE/SOURCE不执行拨动配方。53401缺REAL_TOUCH、adapter0却置REAL_TOUCH/饱和位、未知adapter/flag，均拒绝，不凭flags单独旁路真实验证。SOURCE R1 commandFlags=0，其sourceAdapterRevision沿原显式SOURCE合同。

去重沿原正文逐word：40..127及run[4..5]/inputGeneration[12..13]/parameterSha[26..33]，不以bodyHash授权，不比较nonce或pose sample；INPUT revision/adapter模式额外核准。native证据按(strokeId,commandSeq,run,inputEpoch,parameterSha)强绑定，不能用另一pointer或另一raw record替换同序号正文。重送原命令只ACK，换nonce不能重复施力；已消费即使各列全unapplied也消费。BLOCKED无提交不消费，保原body与UP时间，仅在原250ms年龄内可重试，过期明确STALE并丢弃；不持续改capture续命。

IMPULSE enqueue**不rearm BLOCKED**，继承T524/T508/T529；只有原合同已有真实新target/mode/source控制事件可解锁。静床上只有UP而无合法控制唤醒时UI明确BLOCKED/STALE，没有凭空加force或假target。G-C3等研究测试须先声明实际RUNNABLE/合法控制前置，不能隐藏控制事件；若产品希望静床UP直接唤醒，那是后续designer语义修订，非本卡偷改。输入排队成功不等于root消费；独立报告记录入队、根资格、选择、提交及unapplied原因。

## 7. 冷证据与成本

新cold仍32768B prefix，不为prefix建descriptor（沿T529，不混T535）。header revision=2；prefix原word3072..3583放两份TOUCH_EVIDENCE256：CURRENT_FROZEN_TOUCH、BASE_LAST_CONSUMED_TOUCH；3584..3647放Config64，3648..3711放SEED_META64，余3712..8191保持0。原completion/input/config/quota/summary/descriptor区不改。两份证据缺席须全0且有明确kind，真实command缺证据不能报完整真实adapter。BASE旧consumed与当前重复命令可能同序号，仍分别绑定对应capsule。root当前为NONE时也保BASE_LAST_CONSUMED，不能只留最近随意UI值。native在BASE冻结前保同代证据，commit后才替换已消费记录；失败候选不能覆盖旧记录。

TOUCH_EVIDENCE256逐word表见JSON：完整scope、pointer/stroke/command、DOWN/UP原系统timestamp与binary64位型、receipts/freeze、vp原screen/window/local点、vp→px比例及contentRect、DOWN/UPmodel/delta、duration、原场景DOWN/UPtoken身份、config/adapter/seed/clock/platform来源SHA、frozenInputSha及J/cap。self evidence hash不嵌自己；raw record SHA仅外部manifest/回执。SEED_META64保存template/actual/parameter SHA、M0/origin/source、上传字节数和版本，其余0。外部manifest含新revision、两record raw SHA和存在性、真实与合成域标签、clockProof状态/来源；prefix原始SHA在外部复算。reader先核版本、零位和SHA，再重建raw映射/时长/配方/INPUT原件与BASE去重，不把外部摘要当原件。不能以T529 V1 cold或仅UI日志证明真实拨动。全部页仍≤1024word/16KiB，逐页文件不在JS聚合；TTL60s、两slot/单builder/reader及共享跨route信用不变。

| 资源/工作 | 冻结额度及增量 |
| --- | --- |
| 普通GPU常驻K192/K512 | 51,120,768 / 93,803,648B，不变；BALANCED使用K512 |
| FULL额外GPU K512 | 133,718,016B，不变 |
| G9最大ROOT/FULL单slot | 45,953,024 / 179,671,040B，不变；两slot359,342,080B |
| 两record+Config64+SeedMeta64 | prefix原保留区2,560B，新增分配0；实际原字节参与外部prefix SHA |
| native输入证据信用 | 16队列×1024 + 活动stroke/当前freeze/BASE cached/lastConsumed各1024 =20,480B；Config/SeedMeta/ClockProof/PresentedMapping各256=1024B；一个1024B序列化临时块，合22,528B，不含原对象锁/容器开销 |
| cold seed上传 | 原单673,792B vector暂存，hash/填充原地；不得另保第二整state；在cold builder前释放，不冒设备RSS已回收 |
| 每root host→GPU / completion | 既有INPUT512B+CONFIG512B，不增传输；completion1024B/单根fence/读回不变 |
| dispatch | Q2024、SOURCE12、最大plan8108不变；在已有gate增加有界decode/两次整数RNE/平方乘法，无新增Q dispatch；初始化2次单独记账 |
| binding/workgroup/shared | 4 SSBO，64 lanes，1 group/pass、shared≤4096B不变；128limb局部值计寄存器不能冒免费性能 |

新增native信用在收DOWN/入队前保留，满16槽返回QUEUE_FULL/取消本stroke，不覆盖旧命令、不无限增长。冷slot pin住额外证据不另保一份整个prefix；重导沿原slot复制。JSON页数/UTF8归档界沿T524，不因prefix填保留位增长。编译报告须实报生成资源表、shader/cache key、冷热分配各项，实际allocator/alignment/栈/驱动/FBO另记，不以本表当设备峰值证明。根绝对250ms/GAP33,333,333ns/配额120Hz/显示30fps不变，无CPU业务物理/优化/新oracle。

K512允许FULL保留与两native slot并存时，本表已知payload小计为93,803,648+133,718,016+359,342,080+22,528+16,384=586,902,656B。seed上传数组须在该阶段前释放；若实现改变生命周期而并存，另加673,792B，不能用逻辑释放冒RSS回收。该小计尚不含诊断独立信用、JS运行时/页落盘缓存、固定对象、栈/编译器spill、alignment/FBO/cache/driver；不把小计报成全部峰值或设备资格。

## 8. 实施和独立验证接力

交developer的prompt：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿/机器表一次完成BALANCED Config64与规范seed、真实ArkTS/native stroke/clock/PresentedMapping adapter、GPU配方复算、INPUT V2/独立旧SYNTHETIC兼容、cold V2双record及collector/reader，修订canonical generator，不运行迁移脚本覆盖已交业务源码。编译签包/身份收据明确不同于T529；初始化audit与新clock不偷换旧时间合同。所有渲染仅新XComponent+shader，失败FAULT或标明STALE上一有效完整帧，禁Canvas和softwareGL产品回退。开发职责仅开发/编译/提交，不执行测试、不使用真机；不将独立物理/设备/效果测试加作开发关闭前置。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它可做范围；具体阻断依用户规则原卡报告全部未完成项/原始错误/复现/证据/尝试/所需前置与接手角色，自己行看评论。完成自己行结束待验收。

交manager：T543交付即工程前置已齐，可派独立developer；不自行建卡/改他人行。T541/T542可继续原T529独立物理/资格取证，不能把本稿当已实施。T544需本adapter完成且正确签包后才开展真实拨动效果，非等本稿就放行。独立tester在新版本上测dt两端±1ns、原时钟未来/回退/epoch/2^53、半开边界、-0/NaN/Inf/次正规、RNE半偶/符号、饱和/0冲量、取消/多指/UP零active、rect/identity换代、GPU错J/cap、reserved/body污染、重送/nonce变更、全部unapplied消费与BLOCKED不消费、冷双证据/旧版拒收；设备检查实际系统clock与px/PresentedMapping、首帧seed audit、时间与GPU成本，effector按T534已批准研究门测P05/P06/P07。失败先回architect；未实现登记NOT_IMPLEMENTED，不假PASS，不新增manager验收门。
