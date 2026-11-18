# T567：目标12水相与S/F/P的GPU工程闭包

> 2026-11-18 T580资源公式澄清：逐项父/G12事件信用、SOURCE64、七HYDRO合并摘要、含DM24576B的WORK基线、裸arena对齐及六trial/双冷槽并存见 `architecture-T580-resource-attribution-v1.md` / `architecture-T580-resource-formulas-v1.json`。全部四档既有数值/布局/6204调度不变；机器表与生成器均显式引用该公式合同，不能靠arena/resident反推当独立证明。

> 2026-11-18 T577单位澄清：见 `architecture-T577-water-level-attribution-v1.md` 及 `architecture-T577-geometry-clarification-v1.json`。无P完整两段闭式与C8一致，B须为Q20、H4/eta4/R4/W4为4Q格点；T568分歧来自独立测试参数/断言与报告误归因，不改本稿有界二分/精确等式、ABI/资源/dispatch或T576真实前置。

2026-11-17，architect，不使用真机，不执行GPU/产品测试。消费T559正式G12方案、manifest及C01–C20。工程状态ENGINEERING_READY_RESEARCH；产品NOT_IMPLEMENTED/NOT_VALIDATED。研究参数未批准、未标定，默认OFF；CPU停投，目标10暂停。机器合同 `architecture-T567-g12-engineering-v1.json`、完整 `architecture-T567-g12-dispatch-v1.json` 与有限审计为本稿配套单源，旧G9/M1/M3/G11 reader和资格不迁名冒用。

## 1. 真前置与支持域

唯一沙物理父为T554冻结G9-DM v2；需要其实际宽算术、源/触摸、真实gravity输入、六J试探/拒绝前缀/ACTUAL/replay、controller、cold、scene backend，不能把设计交付或局部编译当已实现。沿T560 COMPUTE/SCENE/PRESENT分层和packet整数属性，不因vertexSSBO0放弃可行compute，也不因compute过了就宣可呈现。

首版最小完整功能是**水平湿域M1**：H/J真实水库存、水谱系、孔隙几何、水动量/压力/双向阻力/水致动员、S/F/P/J全部转换、事务/独立replay和完整新shader；M0几何账本/干退化只是实施前置，不能称已双向耦合。wet定义为BASE全局sum(H4+J4)>0，湿域gx=0、非零角拒绝；首笔水SOURCE提交前也核角和全部P区间不重叠/不入床。干域H=J=0沿同版DM的±30°，投影沙库存/动量/谱系/accepted时间逐字段比较，只有明确列出的profile/config/输入身份元数据不要求字节相同。

N128，K192/512，Kw64/128，S/F各列64来源，沙/水各4096独立origin；phi/孔隙率各1/2，水/干骨架密度各1，非可调真实材料值。湿倾斜、同列重叠P、DEM、双沙、旧M3 BULK_WET_H与preview repair不支持；容量够不能放过区间非法。不设phi0，不删反冲，不以水特效替F真实q。

干分支的新增H/J/pW必须0，wet几何cache epoch0明确N/A，不拿0的PCount冒真实空P，也不执行湿版P重叠门；Scene/cold在此模式只读父沙几何、不重建非法的湿C8。首笔水必须先检查当前干P是否已满足湿支持域，不能通过给干域新增限制来伪造C01退化。

## 2. 身份、配置和模块

新 `G12_POROUS_SPF_Q20_V1` code=LE32('G12P')，state/input/event/path/config/completion/cold magic与rev1全独立，seal0xC0DE1201。新run冷配置，算法/布局/完整参数/输入/实际seed/能力/scene ABI SHA各自固定；旧CURRENT不原地导入。参数 `MaterialConfig128` 的0..63保冻结父材料Config64原字节，64..127为G12C/rev/profile/Kw/kappa/固定体积比/单位/支持域/设计、父参数、seedRecipe、fixture SHA及0保留字，精确word表在机器表。

**MaterialConfig128不等于旧GPUConfig128**：运行envelope64+新材料128=GPUConfig192，新增64word放已声明WORK runtimeConfigExtension64，成本256B已经在675840B新增固定预算内。parameterSHA覆盖完整128word LE原件；父SHA单独保留在88..95，旧参数批准身份不复用。recipeSHA不能冒实际seedSHA；CPU仅传合法冷seed原件/配置，GPU生成全部几何后完整audit，最后计算/核对实际上传/导出字节SHA，不在host求eta/阻力来供热步进。

SeedDescriptor原字节明确包括初始两种总量、按物理序排列的S/F/H来源库存、P/J位置/速度/id/gen和recipe revision，不包含派生cache；SHA即seedRecipeSHA。冷建景单独三dispatch=SEED_UPLOAD_VALIDATE→SEED_HYDRO_ENERGY_AUDIT→SEED_FINAL_SEAL，实际完成后四次full-state copy建其它bank，成本不计入SOURCE16但必须单列。实际GPU seed按既有cold caller-span逐页流式核SHA，≤4096B暂存复用16384B页信用，身份绑定放被hash的seed字节之外，不形成自引用，不另保一份未计费全seed。几何/audit/SHA绑定未齐不得授permit或CURRENT；真实seed总水已包含孔隙水，旧W不能自动补成H。

新增 `gpu_resident/g12water/{contract_generated.h,contract_generated.glsl,reader_schema.json,buffer_plan.h,hydro.glsl,water_origin.glsl,wet_drive.glsl,coupled_force.glsl,water_flight.glsl,water_source.glsl,water_energy.glsl,wet_schedule.glsl,transport.h,transport.cpp,scene.h,scene.cpp,cold.h,cold.cpp}`，独立生成器产上述字典/32pass/usedMask/native+GLSL常量。父g9dm数学模块在新生成路径内复用，旧默认产物与冻结测试树不覆写。bridge/研究页经既有UIAbility→native控制队列→单GL owner，ArkTS不逐粒子处理、不读库存算增量、不阻塞等GL或文件。

native接口：`ConfigureG12(ParentConfig64, MaterialConfig128, SeedDescriptor, Identity*)`冷配置；`BeginRoot(Permit, RootKind, Input128, RootTicket*)`冻结全局dry/wet与完整计划；`Submit/Poll/Accept(RootTicket, Completion256, Token*)`沿既有250ms/typed completion接受；`AcquireScene(Token, Lease*)/ReleaseScene(Lease,Fence)`pin全部沙水；`BeginCold(Token, Mode, ColdId*)/ReadCold(id,segment,wordOffset,Span<uint32_t>,PageMeta*)/EndCold(id)`只冷caller-span。U64/nonce/SHA在桥接保原宽度，ArkTS用字符串/原始typed words，不能经Number丢精度。父实现未齐返回NOT_IMPLEMENTED_PARENT及模块名，不拿M0假充M1。

## 3. state、WORK、单位与几何

沙量Q20骨架质量/体积，水H4/J4为Q22真实水量，水pW为S64 Q42；统一冲量审计=4×父沙Q40+新增水Q42，能量=4×父E2Q60+水Q62、24附加分数位。宽中间与累计账S128/U128 checked，不靠shader int64或float相消。

每完整state尾按父K stateBytes继续追加waterHeader64(256B)、waterColumn16×128(8192B)、waterOrigin4×8192(131072B)、waterSource16×4096(262144B)、waterPacket32×Kw(128KwB)，共401664+128Kw。五bank都包含这些字段。waterHeader存水初始/准入/H/J归约、ID/source序号和新增水源冲量/水法向重力/湿约束支撑/水源能量与功/湿触摸功/初始水能/耗散；父沙累计账仍保原单位，联合审计显式×4加湿修正，不能把旧字段暗改Q42。Source去重仍沙水共享SOURCE序列、IMPULSE另序，所有bank含同一完整原capsule。

waterColumn16精确含H4/pW lo-hi/J4/eta4/R4/W4/C_Q16/map数/P数/J数/wet flags/epoch/phase/status/0。waterPacket32含mass4、xQ20、**y4Q22**、vx/vyQ20、column/id/generation、独立origin/source、birth/age/contact/path/commit/保留/seal。J沿T237的**点水包接触**，glyph半径/半高不作占据或碰撞体积，只有H与真实P骨架进入C8。新y4让奇数eta4接触可精确表示，不能沿旧Q21 y2截半格；J势能权威为gn*J4*y4/2，等价于设计的gn*J4*y2，保留/2有理数至24位能量区间，不向上投影造功。J出生vy≤0、AIR重力有理轨迹，接触max(4B,eta4)，无旧单向wet终速；未到达只保容量，绝不抬eta/光学深度。

WORK新增675840+128K+256Kw逐段为：hydroColumn128 65536B、waterFace64 33024B、四张finalShare4 plane各131072B（水左/右、沙左/右）、root control8192B、wetDrive32及wideColumn32各16384B、六trial水摘要1536B、prefixCompare8192B、config扩展256B、0保留2048B、P kick staging128K、J flight staging256Kw。总式与设计逐字节一致；具体offset由父workBytes追加，旧offset不移动。

hydroColumn128包含当前epoch/phase、B/P/H/J、eta/R/W、S/F浸没长度、C、计数/31轮次数/status、精确水势能及scaled区间、PiW/PiF/NF宽值、aP/gamma/gnRef/psi/probe/驱动与下一几何。P/J逐项浸没/原速度/请求实际j/路径/排序状态放128K/256Kw，不能把这些成本藏在父spare。布局中字典未分配的保留字保持0；lane临时变量/循环栈属于shader固定资源，编译反射的真实shared/register成本另报。

同列床区间[0,4B]，P区间[2(y2−m),2(y2+m)]，端点/排序/不重叠/不入床先核，不能取union悄丢质量。C8(z)=2z−min(z,4B)−ΣP overlap，target2H4；inclusive [0,4CQ] lower_bound固定最多31轮，inactive搜索lane不提前跨barrier退出，末核C8(eta)=2H4、overlap偶数、R4=overlap/2、H4=R4+W4；H0强制eta0。C_i在床/P内部为32768，纯水段65536；端点用向上开区间的右导数，Cface为两侧均值。H/J总数U64，全部source/flow/P/J移动/转换同时核bulk≤CQ与2bulk+H4+J4≤4CQ。

## 4. 数值接缝、writer与真实转换

水fmW只能**一次宽域RNE**：`RNE(((pWL+pWR)*65536−aQ20*CfaceQ16*(etaR4−etaL4))/(2*65536))`，不能漏Q16除数，也不能先RNE头差项再RNE整体改变tie。qW4再TZ除Q*120J。PiW=RNE(gn*eta4²/8)为Q62；PiF=RNE(gn*(32F²−FWet4²)/64)为Q60；NF=RNE(gn*(8F−FWet4)/8)为Q40，先后次序及分支不靠float。面波速用checked U32（可能超过signed32），aWater用宽非负数，超父驱动操作数域精确拒绝，不截低字。

阻力先约掉min质量，按设计114bit分子/正分母单次TZ，F实际j量化到4倍数、P到4m倍数；水扣同actual，法向/静止S反冲进“环境施给动态库存”的支撑账。水法向重力及相反静水约束逐事件登记，P浮力+j对应约束+ j；不从最终ΔP补环境。probe只读，gnRef在Eval(q)内固定，实际DM top-cut不改H/B，更新S/F wet长度后才真实drag，probe绝不二次施力。pressure不是外功，完整微步闭合Q62上界≤0；source/触摸正功只扣真实ΔE.upper，负功不能抵其它操作，SOURCE注孔抬势能需显式workCap。

Input128的SOURCE沙/水允许60..63的新source workCap（Q62+24区间单位），水模式78=DIRECT_H0/FLIGHT_J1、79=单位rev1；原SOURCE_SAND干域此cap仍0。IMPULSE的60..63保父Q60+24原单位，湿能量比较时checked×4升至Q62+24，不能把同raw cap直接当新单位。body40..127完整参与去重，未用字精确0，SOURCE沙/水共享同一单调序列和准入回执，water source mass从原binary64体积floor到4Q；完整mask在机器表。

32pass完整逐writer表在manifest，完整六J/拒绝前缀/ACTUAL/replay调度在dispatch JSON。七HYDRO只写derived，不是第二水量writer。FLOW水/F先共同冻结和全目标容量/来源预检；WATER_ORIGIN_RESERVE复用父Hamilton scratch后将**最终实际份额**存水左右plane，F_ORIGIN_RESERVE再复用scratch并存沙plane。COUPLED_FLOW_APPLY先读未变的C，利用已空闲父Hamilton scratch为全128列暂存new F/H maps（每列512word），全组barrier后才copy到C并合并实际q/jp/压力；禁止边写左列边把新map当右列入口。两种来源各保左先右后既有tie，不以镜像修正改物理。

湿TOUCH root阶段只消费一次外部胶囊：横向/招募按完整水沙能量与功预检，向上请求在wetDrive的pendingTouchJy保存。32pass中的LAUNCH仅**第一个实际micro**消费该请求一次（即使实际q0也登记reason并清pending），后续micro不重复施力；试探/replay各从相同BASE请求独立执行，dry分支仍父root TOUCH/launch图。LAUNCH时用当前几何/来源/实际功，同位置拆体积，不能沿旧root再launch一遍又在每micro加一次源。该湿profile的阶段语义显式独立，不宣称湿域与干父同一时间图。

P捕获/J到达均先PLAN后APPLY，按真实column/y/id从低到高逐包重核；新H/F下一micro才面输运，后包被新界面覆盖须真实重分类。J到达须加H/pW/origin/支撑并过局部耦合能量，最后才退slot；容量/能量拒绝沿具名试验失败，不退J再补质量。P同位置并F、F同位置拆P保持固体区间，不把粒子图形圆盘当排水体积。

## 5. event、pending、replay与根事务

event64=256B；新kind32..47逐word/usedMask/flags在机器表，原父kind在新namespace中保payload；wet DM另用父reserved operands记录eta/gamma/gnRef/probe，干字段保持父可比。新增每micro≤2K+8N+4Kw+32，SOURCE另64；七HYDRO合为每列一条七epoch摘要，不按七次×128多写。water interior face≤127，两个墙反冲归global32槽，不能把129面全塞N槽。没有动作不造假事件。water/sand final share plane是同阶段实际值，cold末批不是所有micro份额历史。

P半步A跨DRAG_HALF_A→P_PRESSURE_A，B跨P_PRESSURE_B→DRAG_HALF_B：每次reserve一个含F/S列行及P行的**pending phase window**，先保存旧端点，最后pass补齐后全窗seal；中间不得用旧g9Reserve再开另一全局pending。首pass不提前推进sealed cursor，中间失败只保此前sealed prefix。

复合行不能当跨kernel的单个old→new水patch直接重放：P_KICK分别存drag和pressure各自pW before/after、旧/中间/新速度及两套请求/actual j；F/S drag也分别存F端点/S端点。phase reader按真实次序核：A=F drag→P升ID drag→S drag→P pressure，B=P pressure→S drag→P降ID drag→F drag。不拿一个包跨越其它包的总old/new pW当原子变化。F/S pressure与面输运也按已接受实际j、压力后pW中间值、实际面jp及最终列值核算，最终C只做一次联合apply。此复合语义不新增行数/dispatch，独立checker不能偷用generic逐行覆盖。

PATH按namespace选择父sand path或新J16词典，J的y4/时段/slot/id/gen/列/接触seal全部可查，≤16行/slot/micro，沙仍父≤8。超行数/unknown bit/坏seal/旧generation/任意欠flow来源均具名FAULT/试验失败，不能截尾。末audit核完整沙水origin、四套实际反冲/功、精确几何、pending空、所有phase/hash/输入/BASE身份。

五完整state bank、四正常arena、四compute bindings沿父。发现六J、独立拒绝前缀、ACTUAL、accepted replay都从完整同BASE出发，所有水/ID/去重/环境/缓存/时间同候选回滚；同输入逐字比较可比state与规范event，不用摘要hash授权。新audit bits18体积/水位、19水谱系、20实际反冲、21耦合能量、22湿域/source，既有DM位不删。explicit rootKind+BASE wet模式替代所有12/16/2404/6204隐式猜root分支；SOURCE零时间、micro31轮/重放不计accepted时间。尾1024B completion、单root fence及250ms不改；全部门过且native token仍有效才一次切CURRENT，水不能先发布、沙不能后补。

## 6. 资源、完整证据和复杂度

| K/Kw | state单份B | WORK B | 单arena B | resident B | ROOT冷单槽B | FULL_CORE单槽B |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 192/64 | 1001728 | 2028160 | 28774400 | 122134400 | 59621888 | 122204032 |
| 192/128 | 1009920 | 2044544 | 32968704 | 138968960 | 68026880 | 139038592 |
| 512/64 | 1083648 | 2071680 | 44584960 | 185829760 | 91406848 | 185899392 |
| 512/128 | 1091840 | 2088064 | 48779264 | 202664320 | 99811840 | 202733952 |

SOURCE16，wet Q6204，dry Q2404，G1+F4 wet24832/dry9632（新SOURCE是16，不把父12带来少算4）。所有pass单64lane组，空lane照样barrier。保守每micro几何检查7×[31N(K+1)+K(K−1)/2]，K512是15164800个容量扫描/区间比较单位，另有2套O(N64²)来源、O(Kw²)J定序、两次顺序阻力、父DM/轨迹与宽除limb循环。不能把串行次序/除法成本直接÷64宣实时。

设计的FULL字节式resident+65536meta+4096input命名为**FULL_CORE**，含五bank/四当前role arena/WORK，不能假称保了六次发现试验的全部原件。需全部历史则明确预约**FULL_TRIALS**：增6个GPU discovery arena（每个J执行后copy当次完整A，六次实际buffer copy单列非dispatch），每个冷slot再加6A。四组合FULL_TRIALS单槽分别294850432/336850816/453409152/495409536B。max512/128 resident+双CORE+22528输入快照/16384页为608171136B；含六GPU历史及双TRIALS为1486197888B，driver/JIT/host固定对象/FBO/JS/文件/对齐仍另计。未预约不分配历史；受信用拒绝就报告NOT_MEASURED，不能在CORE里补造历史。

上面6copy为上界；较小J成功后未执行的后继试验目录必须标SKIPPED/validCount0，不能把旧arena残留当该试验原件，actual copy次数单列。几何epoch只在本root/scope内编号，完整有效键带parent header的run/context/root nonce、J/micro/phase；不把可wrap的全局U32 epoch当长期身份。所有cache被writer失效后，下一消费者须核正确phase/epoch或重算，不凭字段非0冒新鲜。

`G12_POROUS_COLD_V1`/`G12_POROUS_SEGMENT_PAGE_V1`只读新schema，metadata前缀65536B、输入4096B、目录≤32段、每页≤1024U32/16KiB JSON、typed行完整、两slot单builder/reader、TTL60s不续。ROOT/FULL模式及实际global累计capacity按表，旧route被pin则WOULD_BLOCK直到真正free，不在逻辑退休时开第三slot。caller-span逐页取，不在JS整档聚合。独立reader重建C8/水势能/水沙来源/actual反冲，与cache核对，不能读旧eta=S+W或截图恢复物理。FULL_TRIALS的无效区只按原cursor/mask解码，未seal字节不是完整原件。

## 7. scene、检查和交接

全部前景背景新XComponent+shader。fragment只读同CURRENT State，P/J各独立整数实例属性VAO，P沿T560，J用mass4/xQ20/y4Q22，shader/cache ABI/反射独立；compute→SSBO/attribute的实际consumer barrier与scene fence lease保留。水光学从同token eta、真实固体区间/自由水求，不把孔隙水全部画在床上、J抬水位或光流当F搬移；foreground在同完整FBO合成后present。FAIL为FAULT或标STALE上一完整兼容帧，不用wall时间续播，禁Canvas/软件GL回退。

旧G11只支持干父profile，不能直接开湿版；本卡只冻结G12实际沙/水share plane与生成钩子，湿G11组合须另具profile/marker-map生命周期/dispatch/资源/scene光学合同后才能打开。当前M1可以完整显示真实沙水底材/物理P/J，不因此冒G11目标已经湿适配。

`python docs/analysis-T567-T570-engineering-checks-v1.py`只生成本轮工程资产，核四档布局和设计参考值、完整32writer/6204表、event64 masks、128材料/192运行配置、Q16压力消元/114bit界、150个静态水容积格点、历史原件SHA及T570因果域。没有时间积分、GL、真机或新CPU物理oracle。标准湿例是否能推进、能量拒绝分布、资源实际可分配、250ms、材料/视觉阈值仍UNMEASURED。发现实际标准例闭合反例保原seed/全部actual j/能量区间/最小phase后回designer，不能删反冲/改phi/强裁速度/延时限。

交developer：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿/机器表完整做独立模块/配置/seed/H/J/origin/32pass/实际反冲/事务replay/新scene/cold并编译提交，先M0再M1，父实际后端缺项具名报告；仅开发编译提交、不执行测试。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它可做项，具体阻断原卡写未完/原错与复现/证据/尝试/不能继续原因/输入前置与接手角色，失败本人行看评论、成功结束待验收。

tester独立构造C01–C20合法seed，不导入设计脚本当oracle、不新建CPU物理stepper；保沙水分账/实际q/j/每phase同输入反冲/完整回放/冷页/设备资源与时间，不把守恒拒绝冒效果完成。effector按accepted时间比较启动/沉降/堆积/水面/双向反馈和NO_VISIBLE_LAYER，必须同token真实shader画面。M2真机/独立验证不作开发卡关闭门；失败首次仍architect归因。CPU停投、GPU驻留、250ms/GAP33333333ns/120Hz配额/显示30fps、默认OFF均保留，不新增manager验收门。
