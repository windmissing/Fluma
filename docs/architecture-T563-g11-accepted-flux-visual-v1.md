# T563：目标11已接受面通量视觉示踪工程闭包

2026-11-17，architect，不使用真机，不执行产品/GL测试。消费T558 `algorithm-G11-surface-lateral-rendering-v1.md` 与V01–V14 fixtures；DESIGN/ENGINEERING_READY，产品NOT_IMPLEMENTED/NOT_VALIDATED。目标10暂停，不等待或启动各向异性工作，只保原设计镜像定义和已知tie反例。CPU停投、GPU驻留、全前背景仅新XComponent+shader，禁Canvas/软件GL产品回退。

机器表 `architecture-T563-g11-manifest-v1.json`、完整两父版本dispatch表、`analysis-T563-engineering-checks-v1.json` 是唯一工程引用；本稿不把T555/T548局部进展当已可用实际GPU能力，不更改父物理配方/参数批准/旧通过记录。

## 1. 组合profile、模块和资格

两个独立组合profile：`G11_VIS_G9_R1_V1`、`G11_VIS_G9_DM_V2_V1`，组合code分别LE32('G11R')/'G11D'；各支持K192/512与V2048/8192，旧父物理profile code、输入body、material参数SHA不变，**组合pipeline/layout/shader/visualConfig/visualSeed/reader身份另立**，新run冷选且默认OFF。物理state保持父布局而不是偷偷塞视觉字段；native Permit/Completion expected contract显式持parentProfile、compositeProfile、parentParameterSHA、visualConfigSHA、pipelineSHA、baseVisualGeneration。旧run/cold不原地改名，新组合不套旧cache。

新增 `gpu_resident/g11vis/`：generated_contract、buffer_plan、marker/rank_map、material_plan_producer/consumer、visual_root、glyph、scene/lease与cold adapter；两个父pipeline各自通过新组合生成器包含精确钩子，不修改旧g9spf/g9dm默认或T549冻结树。桥接显式g11ResearchConfig/configure/session/offer/clock/quota/cold等入口及新研究页；错误返回沿T550/T546，无真实clock证据则标SYNTHETIC/拒真实输入，不由视觉提供资格。

设备资格沿T560 COMPUTE/SCENE/PRESENT分层，compute仍4SSBO、不要求vertex-stage SSBO。Scene用直接GPU buffer整数实例属性：State fragment@SSBO0、视觉glyph来自扩展WORK的GL_ARRAY_BUFFER，不在vertex新加SSBO。至少6个attribute locations、真实IPointer/divisor/反射/FBO/同token许可必须过；vertex=0不是自动整产品PASS。

`VisualConfig64V1`字段表见机器表：magicG11C/rev1、组合/父profile、K/V、marker32/glyph8/R65536、blendTicks256或0、可见深度Q/8、直径Q/32、速度倍率1/最小0、采样policy1、descriptor/marker/schema版本、父parameterSHA、T558设计SHA、视觉recipeSHA及reserved0。SHA覆盖64word原件，不含自身SHA。视觉seed由合法父seed的实际来源/库存GPU构造，`visualSeedRecipeSHA`是配方身份，**不是假装提前知道GPU marker原件hash**；首次冷导出可给实际visualSeedRawSHA。parent seed沿T552合法B缓存/hash时序，DM另沿T554独立header，旧R1初始化缺陷不能由视觉掩盖。

## 2. 三bank与32word marker布局

严格按设计 **marker32word=128B**，CURRENT/ACTUAL/REPLAY三bank；glyph8word=32B×V；plan65536×8word=2097152B；control/index65536B；四张map快照4×N×64×4word=524288B。总新增V2048=3538944B、V8192=6094848B，完整附在父WORK末尾，既有offset不移动。buffer_plan逐段生成native/GLSL/reader常量，不拿未声明spare偷藏。四个compute binding仍父B/C/扩展WORK/EVENT；视觉数组都在binding2，物理B/C保持原访问权限。

marker32逐word：0owner（0 INACTIVE/1 S/2 F/3 P）；1column；2cohortRank；3packetSlot；4packetGeneration；5packetId；6/7 originLo/Hi；8sourceId；9rank；10massSnapshot；11/12 parkedXi numerator/denominator；13oldXQ24；14oldYQ23；15newXQ24；16newYQ23；17/18 phaseStartTicks U64；19phaseSpanTicks；20/21 visualId U64；22flags；23sampleGeneration；24appearanceSampleKey；25/26cohortMapEpoch U64；27lastPlanOrdinal；28lastMaterialPhase；29phaseOldKind；30reserved0；31seal0xC0DE1101。完整表及known flags在JSON。F rank/m保持精确整数，S/P不适用字段0；不用visualId占physics id或origin计数。

parked ξ保持有理分数，F显示中心=(2rank+1)/(2mass)，全量U64/U128先算再投影Q24/Q23，只在最终shader投像素用float；不每帧舍入rank。S垂向rank是本cohort底部起整数，真实top-first切取区间[m−q,m)；cohort数组下标不是永久身份，按old/new map映射更新cohortMapEpoch，未知origin/非法range是VIS_FAULT。

control65536B细分：三个bankHeader128word=1536B；Pstyle512×4word=8192B；column索引128×2word=1024B；markerIndex8192word=32768B；258条batchDigest16word=16512B；planHeader128word512B；VisualConfig64原件256B；两个scope运行计数256B×2=512B；其余4224B=0。小V/K未用部分0，不因缩水改变wire。batchDigest只是定位/成本索引，不以hash授权marker/材料正确性。Pstyle是视觉字段，旧物理state保留字不写。

## 3. 已封材料计划与R=65536证明

每个**材料阶段一个批次**。批次键=完整scope+parent/composite/configSHA+rootNonce/baseCommit+semanticMicro+phase/opOrdinal+执行role+generation，seal必须在全部物理预检/实际patch和来源份额确定后、视觉更新前完成。不能拿最终S/F差反推经过的路径，不能消费预测器、qPhysical/WAIT请求、未封pending或回滚列。

快照四张均定长N×64个cohort4：OLD_BED、NEW_BED、OLD_FLOW、NEW_FLOW。VIS_PLAN_BEGIN在材料writer前保存旧两张map，清本阶段plan header/slot；生产writer按原数学运行；VIS_PLAN_APPLY在真实成功分支后保存新两张map、完成计划、核所有range，再更新marker。内group同步用memoryBarrierBuffer+barrier，所有lane统一通过barrier，不能提前return；跨dispatch原API shader-storage barrier继续。新快照/记录都GPU内，CPU不读库存求gain或推进marker。

八worddescriptor通用联合：0type、1ownerRef、2originRef、3oldRef、4newRef、5amount、6rangeStartOrEventRef、7seal。引用U32高4bit为表tag/低28bit为index，0xffffffff为NONE，不强塞64位origin；originRef指向完整cohort4或真实packet/event，包含originLo/Hi/sourceId。所有typed ref须先校tag、index<count、同批次generation及key，不把匹配颜色hash当来源。

固定partition：0..32767四张map的MAP_REF，(table*N+c)*64+rank；32768..49151为两个donor/转换plane，32768+side*N*64+c*64+rank；49152..(49152+K−1)是包动作引用，剩余全0。最大有效记录≤4N64+2N64+K=49152+K≤49664<R65536。每phase各自复用，不能同时累积整root/全部J记录；type和phase矩阵见机器表。记录count仅有效数，不因固定槽留空就把count当连续前缀迭代；reader按partition/known type/valid seal解码。

MAP_REF只引用实际快照原件。FLOW两个plane存**原Hamilton最终left/right份额**和旧origin mass/range：在`g9FlowReserve`每个side的商/余数tie分配完成、从remaining donor减份额之前，把该origin真实share写进本side固定slot；后续不得从最终new map重新算比例。左side处理后右side基于剩余库存正是父实际顺序，不因目标10暂停而“修对称”。FLOW_PAIR_APPLY全成功/事件已seal才给计划seal；失败候选不发布视觉。

NATURAL S→F的plane0按旧bedRank保存实际top-cut mass/range，plane1=0；DM只qActual，WAIT/no-q无转换。TOUCH批次plane0同S→F实际片段，包partition指向原LAUNCH sealed EVENT64，按原事件顺序先recruit再launch；被原touch逐列功拒绝/回滚的列，其计划slot清0且不生密度。DEPOSIT plane0按实际F-origin→新bed cohort区间，保旧top合并偏移，不用新的数组下标冒永久key。CAPTURE包partition指向真实按排序捕获的sealed事件，含q/slot/id/generation/x与origin；source新包同样只指真实已接受birth记录，SOURCE零时间不移动旧marker。

包动作≤K，不展开路径行；保留packet/action事件原件直至本批视觉消费结束，禁止在CAPTURE/LAUNCH后因packet已retire就从复用slot猜旧身份。NEW/OLD map快照在本批核/更新完成前不覆盖；包x/ID/gen来自事件而非已复用packet。不能表达的未来merge/多相动作具名UNSUPPORTED_VISUAL_PHASE，先升版，不用链表或截计划。当前G9无packet merge，该域不虚报支持。

## 4. marker映射与相转化

FLOW完全按T558：同origin左出a右出b、old mass m；r<a去左邻末尾rightIn区，r≥m−b去右邻leftIn区，其余加本列leftIn再减a。新r/m来自真实leftIn/stay/rightIn，old/new key与质量和逐项审计，mNew0不得绑定F。全q0无转化时rank/m和x精确不动，jp/p变化不播放；同m均匀流每步有q/m列累计位移，subpixel不清零。

S→F：只被实际top-cut选中的S marker离床，parked ξ→floor(ξ*mNew)；已在F同origin按转换前ξ重定rank，不给新质量凭空速度。TOUCH先recruit再launch，同批两转换不能直接投影到最终mNew：逐origin先用mIntermediate=oldFlowMass+本批实际S_CUT合计重投影，再按真实LAUNCH来源前缀[0,q)选P，留存rank减q并核最终map；即使最终该origin F=0也能合法S→F→P。中间质量只由已封转换记录求和，不引入物理预测/新快照。F→S：只有实际DEPOSIT才停车；按真实来源入床顺序、old top合并偏移给cohort垂向rank，横向ξ保留。垂向分位由appearanceSampleKey固定编码M和pair生成（η=(2pair+1)/M），不是用horizontal rank给床建立物理颗粒ID，流动前后不更换垂向配方。量化重置误差≤1/(2mNew)列照实VISUAL_REQUANTIZATION，不作为真实q或放宽镜像阈值。

P→F：真实CAPTURE后按事件中捕获x取ξ、映最终同origin流质量；同列多包按物理capture序建立计划，已有F在本批混合按旧ξ保位，新包按真实x映到最终map。F→P：LAUNCH的真实origin前缀区间筛marker，绑定真实slot/id/generation。P owner仅引用相符物理包，不产生第二物理P；packet generation换代/退休必须有合法owner转换或置INACTIVE并报告，不能跟新slot漂移。

phaseBlend候选8个accepted quantum=256 TICKS（32ticks/quantum），0为独立无淡化对照。根内时间由baseAcceptedQuanta*32+(micro+1)*(32/J) checked U64计算，SOURCE不加tick；只有真正提交root才发布该tick，trial/Draw/wall不推进。F→P旧表面anchor固定淡出，真实P点淡入；P→F旧P anchor固定淡出、新F点淡入，不画连接两点的假飞行线。S/F混合重投影不沿屏幕平移播放，量化误差单列。

同marker在前次淡化未完又发生真实转换时，有限两anchor合同选择**以本次转换前实际owner点为旧锚**重建本次相位，不以加权中点造不存在轨迹，标PHASE_REBASED并计数；不声称多次快速相转化无感知跳变，effector必须记录。v1不叠无限历史。旧锚被当前物理遮罩裁掉可直接不可见，标CLIPPED_ANCHOR，不抬厚度保好看；这不是丢失物理库存。

Pstyle每slot四word={physicsId,generation,opacityQ16,representativeMarkerIndex}，在已有control预算内。原P body始终仅一个K实例：launch共享同一真实phase时间，Pstyle给真实P淡入权重；无marker样本的真实P按父正常不透明绘制，不能因视觉池满隐藏物理粒子。capture后旧P ghost只选该包最小marker slot的代表画一次（同包旧锚/颜色一致），其它marker保新F点而不重复画同P body；确定min reduction非抢首winner。Pstyle生成/聚合O(V+K)，不能为每fragment扫描V。

## 5. 初始化、密度和不变账

初始化全GPU、一次冷初始化pass，按每列M=V/128=16/64固定候选slot成对分层采样：横向ξ_j=(2j+1)/(2M)；垂向/来源分位η_j=(2min(j,M−1−j)+1)/M。镜像(c,j)→(127−c,M−1−j)，η同值、ξ反转；在真实累计S/F/P来源质量域定位样本，不按颜色替来源。初始无库存则INACTIVE；physics packet本身不因未被marker抽中而不画。visualId为独立单调U64、sampleGeneration有界，超限具名VIS_ID_EXHAUSTED，不wrap冒旧粒子。

真实SOURCE/新暴露材料可按同配方在INACTIVE槽补样，确定slot/候选次序、同rootBASE counter在ACTUAL/REPLAY分别推进，不能用全局counter让replay生成不同ID。无真实新材料/转换/暴露批次不周期补样、不随机重生；q太小仍保原rank累计，不能独立加速。池满为VIS_DENSITY_LIMITED，记录各列样本数/NO_VISIBLE_LAYER，停止新增样本但不拒物理源/q/包/ID；已有marker非法plan溢出才FAULT。关闭视觉与开启的同输入物理账/轨迹须逐字段一致（scope声明差异单列），marker数量不是质量。

## 6. root事务、完整dispatch和资源

root开始VIS_ROOT_COPY一次dispatch，将同一VIS_CURRENT复制到VIS_ACTUAL/VIS_REPLAY及各自header，BASE bank在root寿命内只读；发现六J/独立拒绝前缀/预测器不写marker。仅AUTHORITATIVE(role2)和ACCEPTED_REPLAY(role4)实际路径有视觉批次，root内按真实微步累积，不能每微步从CURRENT重启；父plan多个QUANTUM时下一子root从上一已提交VIS_CURRENT接力，而非重回整plan起始。

QUANTUM每个实际authority/replay scope：TOUCH_ATOMIC前/后一个批次，micro内NATURAL_APPLY、FLOW_PAIR_APPLY、FRICTION_REST_DEPOSIT、ORDERED_CAPTURE前/后各一个。FLOW批次BEGIN必须在FLOW_ORIGIN_RESERVE之前，APPLY在FLOW_PAIR_APPLY之后，避免丢掉Hamilton原始份额；其它批次包住对应材料writer。P FLIGHT移动仅改变真实包位置，既非材料owner转换也非视觉播时钟，不加独立材料批次。单scope H=1+4J≤129，实际+replay合H≤258。SOURCE实际birth和SOURCE_AUDIT_REPLAY各一批H≤2；replay BEGIN的旧map读取B/BASE而不是刚完成authority的C，内部g9CopyBase后writer产出的新map读取C，两个视觉bank均从同一BASE独立推进。SOURCE的COPY放SOURCE_VALIDATE_INPUT后，完整计数同下。每批VIS_PLAN_BEGIN/PLAN_APPLY两dispatch；scope对应的32micro预提交完整表保uniform no-op，不按成功J临时缩表。

新增root dispatch为COPY1+2H+AUDIT1+GLYPH1。R1 Q上界2024+519=2543；DM Q=2404+519=2923；SOURCE12+7=19；G1+F4最坏10191/11711。两个完整表逐项给parent op、新VIS op、触发role、micro及batchPhase，不能只写2H+3少算replay。Q实际J小于32时仍记录最大提交表/实际活跃批次数，发现/拒绝scope不产生视觉副作用。

VIS_ROOT_AUDIT逐word比较ACTUAL/REPLAY全部marker、规范headers/ID/相位时刻/诊断，独立核每plan范围/来源/粒子代/容量。两执行role的batchDigest仅定位，不能以hash代替逐marker对照。AUDIT后GLYPH构建并逐项核bounds/known fields，设置visualReady；最终**父ROOT_SEAL在这两者之后**，completion既有1024B尾携带组合required mask和总pass数，native只在完整组合typed接受后同时推进物理CURRENT与VIS_CURRENT/generation。SOURCE分类必须显式rootKind，不能沿旧`uRequiredPasses==12`猜SOURCE（现SOURCE19），所有硬编码count/检查表同步版本化。

视觉新增审核bit18 PLAN_MAPPING、bit19 VIS_REPLAY_GLYPH。R1 SOURCE/Q required=0xcffff；DM SOURCE=0xcffff（DMbit16/17 N/A）、DM Q=0xfffff。父物理audit位一项不少，故障/Stop/晚token/plan坏seal不切任一CURRENT。视觉准备失败不得先提交物理再补视觉，也不为保物理吞掉组合错误；研究组合清晰FAULT，旧默认父profile仍独立可用。

三bank当前索引只是表示层，publish时同native mutex更新(parent state bank,vis bank,tick,generation,configSHA/token)，锁内无GL或等待。Scene lease pin两者和glyph所在WORK，单owner/单scene fence未退休前不允许复写WORK/开启新root或冷builder；glyph只有一份，依此显式背压成立，不能另偷偷双buffer以提升FPS。背景/lease/context变更按原retire/quarantine规则，逻辑取消不冒实际free。

payload三bank/plan/快照按设计固定。每个父K/V档native/GL计算buffer实际大小、binding offsets、最大SSBO和总并存资源在机器表展开；多出视觉WORK与其冷热副本都记账。普通readback仍1024B、root fence数1；额外storage/vertex-consumer barrier/实例fetch单列，未测性能不声称免费。

## 7. Scene组合、遮罩与vertex=0设备路径

glyph8word={oldXQ24,oldYQ23,newXQ24,newYQ23,kindFlags,blendQ16,oldPacketMassOr0,appearanceKey}。实际owner/精确rank和视觉ID保marker32，不把投影glyph当权威。坐标物理域x[0,128)、y[0,256]；F可见层band=min(F,Q/8)，GPU y2取2B−1−floor(η*(2band−1))，保证S≤y<B（F0无glyph），不因薄层亚像素而抬高度；S仅真实可见顶带，埋藏样本保身份不画。两anchor包围quad只在各锚点圆盘内着色，不在两点之间填线；旧F/S锚若越当前真实轮廓则clip，P ghost仅按真实旧位置，不假装新物理包。

vertex shader无SSBO：glyph两路uvec4 locations0/1、stride32/divisor1，GL_ARRAY_BUFFER为同扩展WORK的glyph区；gl_VertexID生成6顶点。fragment只读State@binding0按同uBox/N/真实S/F轮廓和前景mask裁剪、线性色±0.08/alpha≤0.5候选的premultiplied合成；静床/流层基材继续父shade，不让所有F变异色。重复同token必须同图，STALE/FAULT无tick/相位续播。

P draw使用T560 State属性mass/x/y2，另加真实packetId/location4、generation/location5和WORK Pstyle uvec4/location3；各自divisor1、State stride256/WORK Pstyle stride16。反射至少6location、相同id/gen核准，不匹配FAULT而非使用新slot旧style。glyph与packet各自独立VAO，fullscreen另VAO。绘制顺序为底/静床/F基材→S/F及转换glyph→真实P/单ghost遮挡策略→同一完整FBO的前景→present；Pghost作为glyph类别同真实P层规则，不双画。每帧最多V个glyph实例+K个真实P实例，6V+6K顶点另加fullscreen；原两FBO/RGBA8≥8wh保留，无新增纹理。大跨越quad的overdraw另测，不能只报顶点数冒fragment免费。

compute→SSBO及attribute使用 `GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT`（T560/Khronos）；同group同步和跨dispatchbarrier分别保。glDraw没有推进视觉、没有CPU map/每帧库存copy、没有按wall时间UV滚动。FBO/屏幕三域证据与真实accepted物理/视觉token配对后才判效果，单诊断条纹不冒G11通过。

## 8. cold、独立客观与效果接口

新增组合cold schema `G11_VIS_COLD_V1`/page `G11_VIS_SEGMENT_PAGE_V1`，保父完整ROOT/FULL原件并附精确visual payload（全部三bank/glyph/plan/control/四map快照），目录注明逻辑BASE/CURRENT/REPLAY索引及parent/composite/visualConfig/seedRecipe/pipeline SHA、tick/token。ROOT/FULL每slot都要在父容量上加本V全部视觉bytes；最大DM K512 V8192两FULL=388308992B。旧reader拒新组合，逐页≤1024U32/16KiB、TTL60s/两slot单builder/reader与实际free信用保留；不开第三未计slot、不在JS聚合整档。

plan与map是**最后实际批次**原件，非所有micro历史。完整历史物理EVENT/PATH及BASE视觉用于独立重建/调试，但不声称末批plan或batchDigest证明整根全部来源传输；独立测试可预先指定单个(root,micro,phase,role)抓原件。`G11PhaseWitnessV1`只允许一个预预约GPU witness：完整plan2097152B+四map524288B+old/new marker各128V+header4096B，V2048=3149824B、V8192=4722688B。在目标VIS_UPDATE前copy旧marker，后copy新marker与sealed plan/maps；同root仍照250ms/撤权，不加CPU等待。只读回caller-span在root退休后，带根真实outcome，失败私有资料不冒CURRENT。

该可选witness GPU保留+native单份块各按上值额外计账，3次数据copy（oldmarker/newmarker/plan+maps可分段且报告实际次数）、header填写另记，无新增物理dispatch/binding；capture未预约无额外成本。max视觉cold已知并存包络另加两份witness容量，driver/FBO/JS/文件等仍排除。witness读取不可用时精确NOT_MEASURED，不按hash补造来源原件。

tester接口交付raw parent/marker/glyph/plan/maps/完整字段字典、固定同输入视觉ON/OFF、authority/replay逐word、V01–V14负控、singlePhase witness及设备payload/dispatch/时间分账；独立期望自己实现，不导入developer输出当oracle，不新建CPU物理stepper。主要精确关系包括rank全域、q0冻、均匀q/m、r镜像、来源left-first事实、P/S/F唯一owner和id/gen、mix误差上界、Stop/late无发布、视觉池满不变物理hash；没有最终感知/性能新PASS阈值。

effector仅比较同实际accepted时间区间的底材版本与组合版，保真实新帧/token/流动可见区域/纹理位移、弱流累积、停流/反转、clip/过渡重基/重投影闪动及NO_VISIBLE_LAYER/VIS_DENSITY_LIMITED。录屏漂亮不能替物理正确，marker数量不能代质量。目标10已暂停，已知tie只隔离报告不自动修物理。

## 9. 实施prompt

交developer：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿及机器表完整新增独立g11vis/组合生成、真实阶段sealed plan/份额与map寿命、视觉三bank原子root/replay/lease、资源dispatch表、无vertexSSBO的scene/遮罩、cold/单phase witness与独立收据，保父物理数学/参数/输入消费/旧baseline。沿T560能力分层而非删门冒支持；父R1/DM实际实现未齐时具名真实前置，不拿设计/partial冒可运行。仅开发/编译/提交、不执行测试、不使用真机，独立tester/effector不作为开发关闭前置。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其它项。具体阻断原卡报告全部未完/原错/复现/证据/尝试/不能继续原因/输入前置/接手角色；失败本人行看评论、成功结束待验收。全部前景背景新XComponent+shader，故障FAULT或标明STALE上一完整有效帧，禁Canvas/软件GL产品回退；CPU停投/defaultOFF/250ms/GAP/120Hz配额/显示30fps不改，不启动暂停的目标10，不新增manager验收门。

## 10. 精确实施接缝与有限检查

文件归属：新增 `gpu_resident/g11vis/{generated_contract.h,generated_contract.glsl,reader_schema.json,buffer_plan.h,material_plan.glsl,marker_map.glsl,visual_root.glsl,glyph.glsl,scene.vert,scene.frag,visual_transport.h,visual_transport.cpp,cold.h,cold.cpp}`；新增组合生成器同时产native/GLSL/reader/dispatch/cache ABI，不手抄offset。父 `g9spf/{surface_flux,activation_deposit,touch_launch,capture,root_schedule}.glsl` 和DM对应writer仅在**新组合生成路径**注入生产钩子；旧生成产物/旧T549 baseline不覆写。研究桥接置于新g11vis adapter，研究页冷配置走既有UIAbility→native session→GL owner消息机制，ArkTS不读marker/库存来推进。

native接口（类型沿父Status/Permit/Token/Lease实现）：`Status ConfigureG11(const ParentConfig&, const VisualConfig64V1&, CompositeIdentity*)`只冷创建；`Status BeginG11Root(const Permit&, RootKind, const Input128&, RootTicket*)`只向现有GL owner排完整组合表；`Status AcceptG11Root(const RootTicket&, const Completion256&, CompositeToken*)`在同一提交锁核父/视觉完整身份并双切换；`Status AcquireG11Scene(const CompositeToken&, CompositeSceneLease*)`/`ReleaseG11Scene(lease, fence)`持真实GPU退休；`Status BeginG11Cold(token, ROOT_OR_FULL, ColdId*)`、`Status ReadG11Cold(id, segment, wordOffset, Span<uint32_t>, PageMeta*)`沿既有单reader/caller-span；`Status ArmG11PhaseWitness(const Permit&, MicroPhaseRole, WitnessId*)`必须提交前预约单份，`ReadG11PhaseWitness`只在退休后。U64/nonce均保原宽度，ArkTS用十进制字符串，不经Number丢精度；缺父实施返回具名NOT_IMPLEMENTED_PARENT，资格拒绝保COMPUTE/SCENE/PRESENT分域，不把partial对象发布。

GLSL接口按表生成 `visBeginBatch(role,micro,phase,oldBank)`、`visRecordFinalShare(side,c,oldRank,share)`、`visRecordConversion/PacketEvent`、`visSealApplyBatch(role,micro,phase,newBank)`；共享同WORK binding2，无新增SSBO。VIS dispatch走独立switch，复用原pass计数但不能被父unknown-op分支吞掉；rootKind显式替代全部12/2024/2404隐式识别（含accepted quantum更新、input kind、outcome、completion metadata）。父材料请求/积分/舍入/来源tie保持原数学，新增钩子不回写父q/库存。

引用与联合细则：新增tag7 COLUMN，ownerRef为COLUMN(c)；MAP_REF的oldRef/newRef指物理writer实际保留/移动的同一cohort行，不能按origin相同就把床内两个分离同源cohort合并，缺对应侧用NONE。originRef指非空一侧的完整cohort4。S_CUT的oldRef=OLD_BED原行，newRef=NEW_FLOW最终行或NONE（TOUCH全部launch）；amount=cut，rangeStart=oldMass−cut。DONOR oldRef=OLD_FLOW、newRef=相邻NEW_FLOW、amount=实际share、rangeStart为左0/右oldMass−rightShare。DEPOSIT oldRef=OLD_FLOW、newRef=实际NEW_BED落点、rangeStart=落点原有top质量（不合并则0），amount=实际入床量。Packet记录originRef=EVENT64，old/newRef为实际owner侧或NONE，amount=event q，rangeStartOrEventRef=EVENT64索引；LAUNCH实际来源前缀0、id/gen/x全读原事件。MAP_REF的amount为该侧cohort质量、rangeStart=0；其它不适用字段NONE/0，所有unused slot整32B零。TYPE+phase+tag矩阵不符立即VIS_FAULT。

初始化每列用真实库存累计域顺序S由床底到顶、F按原map顺序、P按真实slot顺序（物理x所在列），以η选择owner/来源与S垂向rank；F横向用ξ，P绑定真实slot/id/gen。appearanceSampleKey编码(M<<16)|pair，垂向η在初始化和后续投影始终相同。初始候选slot=c*M+j；补样只在真实SOURCE/新暴露阶段，按实际事件序/列序，再按INACTIVE slot序使用同j=slot%M配方，ID/counter基于各自BASE独立推进。不得借补样改已有marker或物理库存，补样导致的稀疏/镜像采样差单列而不冒物理tie修复。

`python docs/analysis-T560-T563-engineering-checks-v1.py`只重建本轮新工程表。52,650个小整数rank分区/镜像关系、8,192个重投影误差关系、全部8个父/K/V资源档及完整父dispatch去钩子等价检查通过；这是有限规格代数/布局核对，不是GPU实测、物理oracle、产品PASS或新CPU投入。最大的DM K512 V8192已知GPU FULL+双cold+input/page包络632474752B；预约GPU/native witness后641920128B，另有driver/FBO/host等排除项。
