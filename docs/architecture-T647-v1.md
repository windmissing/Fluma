# T647：DM 父接受证明与双父组合接缝工程修订

2026-11-19，architect，不使用真机。结论 **ENGINEERING_DEFECT_CONFIRMED / CONTRACT_READY_FOR_DEVELOPER**。T639的真实父合同阻断成立；安全修复ede5891保留，编译脚手架不是产品包。正式机器表`architecture-T647-acceptance-v1.json`、有限源码/表式证据`analysis-T647-evidence-v1.json`与只读复核脚本同名PY。未构建产品、运行shader/物理/设备；修订不等产品PASS。

## 1. 首次归因：不能只宽松 gate

T639 blocker中各源码SHA与本轮实际源一致。DM Transport::Common/Pump与完整表确为SOURCE12、QUANTUM2404；CompletionGate::Common却强制12/2024和0xffff。shader `g9EndAcceptedMetadata`仅在2024时增加acceptedQuanta，PACK_SCENE无条件写65535，故真实2404既不能正确报接受时间，也不能证DM物理bit16/17。G11两父gate无可信ExpectedContract入口，直接套默认gate必拒19/2543或19/2923；CPU改尾/count/审核位来通过同样违规。

进一步核到与新增审核位直接相关的**缺实现/工程接线**：

- `dmPredictor`仅保存pEntry；`dmMidpoint`读`uColumnStage+c*64+52`的pEnd却没有对应真实写入。pMid写到+56，CORRECTOR_FACE仍调用原`g9FaceFlux()`读C物理p，不消费中点；没有完成私有完整摩擦预测或正式DM_PREDICTOR事件链。
- `dmWord/Put`占旧ColumnStage+32，而正式T554要求在父WORK尾追加DM48/列；当前字段会与预测临时区重叠。不能用错位或旧残值生成审核PASS。
- `dmEval/Request`多处用`C[uConfig+...]`取参数，uConfig是WORK偏移；当前RootConfig上传也没有完整DM Config64。必须改真正W配置访问及正式字段，不能只在外层检查值后让shader继续读错buffer。
- qNext/nextFeasible当前写常量或由qPhysical<upper推断，没有正式q+1边界复核；T554要求最大可行q、来源连续前缀和实际patch联证。仅authority/replay重复同一错误并不足以证明物理审核。
- 组合Common当前`uVisTicks`按(quanta+kindDelta)×32，正式G11 accepted quantum为256 visual ticks；这与rest32/J混单位。T639续实施应改正式256并checked乘加，不将动画时间错误掩成物理quanta修复。

这些已有明确算法§4/§6.2和T554字典，不需要developer另定物理。它们属于**补齐既定DM实现**，不是授予改积分/摩擦/能量或自行裁阈值。若按原公式仍遇未定义量/矛盾，保具体公式/反例/原错回architect再转designer；不默认放宽。

## 2. 五个独立 family / 明确 rootKind

| family | SOURCE count/mask | QUANTUM count/mask | 完成尾身份 |
|---|---|---|---|
| R1_DEFAULT | 12 / 0xffff | 2024 / 0xffff | 原R1 magic/profile/rev1，完全保留 |
| DM_DEFAULT | 12 / 0xffff | 2404 / 0x3ffff | 正式DM2 magic/GDM2/rev2/seal，修正未对齐实现 |
| G11_R1 | 19 / 0xcffff | 2543 / 0xcffff | 新G11C magic/rev2/seal0xC0DE6471、G11R profile |
| G11_DM | 19 / 0xcffff | 2923 / 0xfffff | 同新G11C、G11D profile，物理profile另核GDM2 |
| G9_RX | 12 / 0xffff | 2404 / 0x3ffff | DM2完成magic/rev2、GNR1 profile，T641新输入/配置域 |

SOURCE=1/QUANTUM=2始终取冻结permit/rootKind，不从任何总数猜。`uRootKind/uRequiredPasses/uRequiredAudits`分开；所有input kind、outcome、SOURCE admitted、acceptedQuanta和seal分支都依显式kind，count只校实际完整dispatch。两组合表去VIS投影仍等各自正式父表。SOURCE不设置16/17伪N/A通过；新域只允许该root的精确mask，未知bits拒绝，不用“至少包含”接受多余未登记审核。

DM默认profile保正式GDM2的物理意义，但本次实现修正必须新algorithm/implementation/layout/pipeline/shader/reader与缓存身份，冷新run创建且实际seed/config字节SHA重算；旧错误2024/rev1/0xffff产物不能热续接为正确DM。旧R1和T549冻结源/fixture不改。两组合保父state/input/物理参数身份，完成尾是新组合身份；GNR1保T637/T641域，不借REAL_TOUCH资格。新magic/seal/rev和值域均在机器表生成，旧reader应明确拒收。

## 3. DM 配置与真正物理审核 producer

DM默认RootConfig128：前0..7物理参数、16..55各SHA不变，64..127上传**完整正式T554 Config64**，新增八参数位于112..119（64+48）；GPU从W读取且核与state header106..113相等。默认DM parameterSHA=正式Config64全部256B的SHA，不再用当前20标量ParameterHash冒同一身份。GNR1的完整Config64不同，八物理参数在12..19，因此RootConfig为76..83；生成不同accessor，不互用112/76。旧Config/seed实物必须新身份重建，不拿旧SHA补标签。

WORK仅使用T554每档`appendedWork.byteStart + c*192`的DM48原字典，32..47存真实post-FOLD入口/预测/S64中点及支持。S64落盘先checked收窄，四limb计算留寄存器，不把每个S64槽当128位写跨下一字段。旧ColumnStage仍负责corrector暂存，face区先预测后corrector完整覆写；不加buffer或CPU场演化。新增参数不授权改变已定值。

bit16按真实DM_REQUEST/APPLY链核：冻结S/F/p/B、完整宽head/H/omega及最大整数q；q可行且q+1不可行或被S/U界排除，32轮闭式/隐式/计数与诊断一致；来源top-first连续可表达前缀、qActual=min(qPhysical,qOrigin)，实际S/F、map/来源迁移、p不变、q0/WAIT不伪扰动；kind15字段/保留零/ordinal/seal来自本次真实写入。q+1有域时必须真实Eval，不能把常量nextFeasible当证据。不能用重复同一结果的hash代独立谓词。

bit17按真实post-FOLD私有预测链核：pTrial0含一次原HR/床修正/gxδ，完整δ入口mode摩擦prox→pEndPredict；预测阶段不写C物理p/质量/来源/反冲/耗散/rest/time；pMid精确RNE且域/CFL合格；corrector确用同S/F和pMid求q/jp、实际p基底仍pEntry、静止面mask核预测与corrector支持；真实FRICTION/32/J rest/能量账只一次。CORRECTOR支持结果已知后写真实kind16事件，physicalDeltaIsZero/predictorRole1/完整字段及role规范化按T554。停止剪切必须读实际post-friction newP，不再读旧band/p。

根时间核不能只看某个计数：MICRO_AUDIT对每个实际执行micro检查δ=1/(120J)、rest tick=32/J的实际更新/扰动规则，scope总δ=1/120；私有预测不加tick。根被接受QUANTUM只在candidate metadata增加acceptedQuanta一次；SOURCE不加。相同修正同时应用六J发现、authority、独立拒前缀、accepted replay，保全部质量/来源/动量/功/严格能量/CFL门。

拒绝码仍用正式T515 enum，不能加宽Blocked范围吞硬错。当前dmMidpoint把中点F0非零/速度越界写`g9Reject(28)`，而正式28是PATH_BOUND；应按真实谓词分别用7=NEGATIVE_OR_EMPTY_WITH_MOMENTUM、6=VELOCITY_DOMAIN、5=CFL，保持试验可细分语义；S64不可表示按设计速度域拒绝，S128算术溢出硬29=OVERFLOW。正式19..29协议/来源/replay/信用/path/算术错不能转正常BLOCKED4；30撤权/上下文失效按owner取消处理。Blocked proof只准正式1..18正常族且满足六试验拒前缀/无提交，不能为容纳错误28直接放宽gate。DM局部ORIGIN_WAIT/PARTIAL仍列级q0/部分准入，不提升为根BLOCKED或fault。机器表保存完整enum与中点映射，T639/T644共用。

### 有界审核闭合（无新 dispatch）

在新DM工程revision显式登记已有RootAndStageControls的word120..127为机器表八字段；旧shader未用这些slot，容量和偏移不增、原completion reserved统计位仍零。ROOT_INIT清零并设protocolRevision1；scope进入与结束记录真实seen与certified16/17位，不靠mask常量初始为PASS。

scope证书14bit：六DISCOVERY=0..5、AUTH=6、六REJECT_PREFIX=7..12、ACCEPTED_REPLAY=13。若选中J=2^k，expected=发现0..k、拒前缀7..(7+k−1)、AUTH6、REPLAY13；全拒expected=六发现+六拒前缀。只在真正enabled的scope begin/ seal维护，预排uniform no-op无证书。每scope要求/已访问stage mask由真实op/micro推进，拒绝处核首错与已seal合法前缀，禁止把后续未执行预测声称执行；soft reject的证书只证明**该拒绝前缀的完整合法性**。硬fault或少scope/stage不能给完整根审核位。

MICRO_AUDIT与SCOPE_SEAL在已有pass里完成对应谓词，PACK_SCENE/COMPARE_REPLAY前核seen/cert16/cert17严格等expected、原first-choice/summary/replay与真实接受时间，才可写16/17。BLOCKED无需虚构authority或完整预测，须六试验独立拒绝重放均完整才有正常拒绝控制证明；它不增加接受时间。SOURCE只核原SOURCE16族，无新DM证书。新controls读取写入/reader字典、生成布局SHA必须同步。

原bits0..15沿T515具名族一项不少（表含原名）：输入身份、质量、来源、map、slot/ID、几何、环境动量、转换能量、微步能量、外功上限、CFL、rest、第一成功选择、完整state replay、event/path封前缀和candidate scene/accept一致性。DM不能再无条件写0xffff；由真正检查成功/该root明确N/A条件汇总。最后一族GPU只证明candidate场景元数据与拟接受state一致，真实present仍由native联合提交/场景lease/Swap另证，不能预报已上屏。源码未完成审核producer时保持未实现/拒绝，不CPU补PASS。

同样核`g9RootEnd`事件audit operand及所有SOURCE/Q汇总切点：只封该scope已实际闭合的具名审核，不能遗留旧65535常量冒完整DM证书；root期望/实际mask和scope原件含义分开，尚未执行的最终commit/present不预报。DM48、8word控制登记和G11新control保留区覆盖均只在本次新layout/reader revision生效，原T563/T554旧reserved合同由本稿明确版本化修订，不静默写旧区。

## 4. NativeEvidence → 私有 validated proof

各父保默认`CompletionGate::Accepted/Blocked`入口，默认R1只12/2024，不给调用方传任意count/mask的宽松重载。新增受信工厂`ExpectedForActivePipeline(ActiveProgramIdentity, frozenPermit, parentKind)`，从生成表产生**不可变私有构造**`CompositeExpectedContract`/`RxExpectedContract`；包含family/kind、父物理profile、tail magic/rev/seal/profile、精确count/mask、state/work/容量、物理参数/seed/layout、实际shader/pipeline/contract SHA与冻结输入身份。JS、raw尾、测试boolean不能构造授权合同；生产test seam禁用。

父内部公共语义核验核心可复用，输入上述受信Expected。对R1/DM分别提供`AcceptedComposite`/`BlockedComposite`的类型化入口并返回私有`ValidatedCompositeParentR1/DM`证明；GNR1生成专用DM validator也可，但必须执行同一完整真实DM谓词/时间/bit16/17合同。**不是把组合尾改成父12/2024/2404再调默认门**，也不声称默认门本来能接受组合尾。物理proof只能由对应父完整核验核心构造，不由视觉包壳自行PASS。

NativeEvidence来自唯一owner的真实Transport dispatch/barrier计数、fenceNs、首个原GL/EGL错、context/owner/映射尾身份；它不是从uRequiredPasses或尾[40]复制的声明值。生产可保现有原始struct用于采集，但对外只交Transport内部sealed observation/capture handle；controller从同一个mapped flight获取，不接受NAPI自报。根begin≤fenceNs≤deadline且commit时仍在deadline/fresh/current scope，copy/scene barrier和warmup次数分账，不混充每physics dispatch的一barrier。

Common严格核T524全256word语义：magic/rev/words/seal/profile/rootkind；nonce/plan/run/owner/context/surface/geometry/config/inputEpoch/session/baseCommit；bank/arena代及范围；[39]=[40]=真实dispatch、[41]=真实barrier、[42]=精确requiredMask；event/path planned=sealed≤容量，selected/eligibleJ/完整trial mask与首错、全部参数/layout/shader/inputSHA、合法outcome/保留零。Accepted另核actual proposed commit/quanta、SOURCE admitted/去重语义及fault字段全零；BLOCKED另核六拒绝/第一失败/无物理提交/无接受时间，不拿fatal5转正常4。原阈值/能量/守恒/replay不降。

`g11vis::CompositeGate::Accepted`消费对应父proof再核组合扩展、真实18/19和视觉generation/tick，才构造私有`ValidatedCompositeAccepted`；Blocked同理只生成控制proof、不能切bank。最终`CommitComposite(proof, liveGuard)`核proof确来自当前mapped raw tail/同flight与Expected，再在同一非GL提交锁切物理bank、VIS bank/generation、accepted time/publication。默认DM/RX `CommitAccepted`亦只走真实typed proof路径；raw permit+tail的旧低层Commit不得成为可绕入口。校验后撤权/过期/Stop/错context必须拒绝，无先物理后视觉、无静默重试重加力。

## 5. 新组合完成尾扩展及时间

仍256U32/1024B，0..155原父语义保留；word4明确组合profile，word72..79为物理父layoutSHA，word80..87为**实际组合shaderSHA**，根permit不能仍填旧父shader。仅新G11C/rev2在156..239登记扩展，240..255零，完整逐段见机器表：contractRevision、parentKind(1R1/2DM)、physical/composite profile，staticContractSHA、visualConfig/recipeSHA、physicalPipelineSpecSHA、compositeLayout/PipelineSHA，base/proposed visualGeneration、base/proposed acceptedVisualTicks、physicalSeed/actualVisualSeedSHA及frozenExpectedSHA。默认DM2/RX的156..255仍全零，不能用G11扩展冒通用DM尾。

staticContractSHA是本合同原件SHA；实际pipeline/shader由生成产物原字节另算。frozenExpectedSHA是claim时规范冻结的**期望描述**SHA，内容含本表选定常量、原permit完整身份、全部参数/seed/layout/实际产物SHA和base visual代/tick；不含自身或尾字节，不制造自hash循环。生成器须产唯一LE字典和长度给reader；该hash只连接原件，不能代逐字段比较/授PASS。新扩展原件由GPU从冻结配置写入，native逐项对Expected比较，不能map后补。视觉审核位18只来自成功材料plan/mapping审查，19来自完整逐wordACTUAL/REPLAY及glyph校验，均在父seal前；CPU或缺writer的空表不能置位。

期望前像完整冻结为机器表`FrozenExpected224`：896个LE字节，GREX/rev1、family1..5、rootkind/tail/count/mask/capacity、完整permit及9×U64 scope、base/proposed视觉代/tick、13个具名SHA、clock/lifecycle/domain与参数/implementation revision、保留零。静态合同SHA取`architecture-T647-acceptance-v1.json`原字节；缺的视觉字段/V及非RX receipt专属字段按表零，不从尾反推期望。采用已有冻结permit/config/产物身份的只读view，逐word流式SHA，不另分配896B常驻副本；GNR1用已有hash workspace，40960B不增，组合Expected对象/引用固定helper实际sizeof另报并计入controller账。当前mapped tail/evidence只与该view比较，不能边验边改expected。

组合只在新工程layout中登记原64KiB VIS control保留区相对byte61312..62239为Expected224+其SHA8（共928B，digest相对word15552）；62240..65535仍零。冻结后在Begin、首dispatch前以一次glBufferSubData上传928B，根寿命只读，GPU seal从该具名区取扩展字段；非G11默认/RX无该上传。此处明确增加组合每根928B上传/一次subdata，计成本但不增buffer容量、物理dispatch、fence或读回；不得继续宣旧reserved全零/零额外成本。host可用≤928B固定栈序列化暂存，不再长期复制；同flight、GPU审核及字段比较才授权，传入期望hash本身不授PASS。FrozenExpected的next visualGeneration在claim时冻结候选base+1；若GPU SOURCE确为duplicate2，尾和proof的proposed必须等base，不把候选值当真实已提交；Expected/hash始终不回改。

物理时间严格quantum=1/120s；rest tick=1/3840s，每micro32/J，和视觉tick不是同一单位。G11视觉acceptedTicks=checked(acceptedQuanta×256)，仅真实accepted Q加256；SOURCE为0时长，显示30fps不改120Hz配额。SOURCE duplicate outcome2不加commit/quanta/visualGeneration且不发布新内容；非重复SOURCE含合法q0回执依父合同commit+1、quanta不变，组合有新成功publication时visualGeneration+1；QUANTUM commit/quanta/visualGeneration各+1。BLOCKED/FAULT/late不改published各值；U64溢出拒绝，不wrap/clip。native ledger只在typed真实commit后同步，不能CPU加quanta掩shader没更新。metadata在candidate先写，失败整体不发布；authority/replay从同BASE独立构造，不双加。

## 6. 文件归属与续实施

| 工作 | developer明确文件范围与结果 | 真实后继 |
|---|---|---|
| DM父默认修复，可独立实施 | `g9dm/{pipeline.comp,completion_gate.*,research_config.h,transport.*,controller.*,schema/生成器,冷reader}`及其构建输入。落实完整DM48、配置accessor、q边界、预测/校正/事件/审核、accepted time、DM2身份和默认typed门，生成新SHA/cache/seed。旧g9spf/T549不改。 | 独立tester沿T556父DM用例核真实12/2404、16/17负控、SOURCE零时/接受时间/全部replay；编译交付不等设备通过。 |
| 双父可信接受接缝 | 新共享**内部纯验证语义核心**或各父`completion_gate`专用Expected入口；R1默认API、常量和行为不变，只新增受信组合入口。返回真实私有父proof，不cast父Config/Permit或强改尾。 | tester默认R1回归与组合错family/kind/count/mask/尾版本/nonce/late负控。 |
| T639原卡续实施 | `g11vis/{parent_adapter,visual_transport,controller,publication,bridge,scene,cold}`、组合生成器/新研究页/reader。保ede5891/W01–W12已交安全部分，完成T563/T634全部真实材料writer、marker/双提交/场景/cold，不只填Commit壳。 | 真产品路径/非空联合帧后T600/T601客观/主观；原编译脚手架不可直接当产品包。 |
| T644新RX实施 | `g9rx`自有生成变体按T641/T647同一正确DM物理/审核/时间实现，选GNR1受信Expected；可复用修复后的共享数学或同源生成，无需等默认DM设备PASS，也不直接copy错误旧gate。 | T642独立期望+T645新域完整独立GPU验证；不冒REAL_TOUCH资格。 |

manager可将DM父默认修复独立developer卡，与T639不依赖的材料/scene/cold范围并行；T639真正切bank与完整产品闭包须父**代码/接口**修复或同源专用实现齐全。若同一developer接手可在本工程授权范围直接完整续作，不能把“等manager再批准”加作新门。default修复与组合接缝共同占`g9dm/completion_gate`时显式单owner，组合开发者只消费，不互相覆盖；共享工作树不回退他人修改。本architect不建卡、不改他行队列。

原T639解除的是工程合同缺口，不是立即宣已开发；T600/T601仍等完整产品、T645仍等T644。签名/部署/脚本/夹具/真机和采证由tester/effector负责，不加开发设备/集中准备门。根正常读回1024B/1 fence/4SSBO、SOURCE/Q固定表、250ms/GAP33333333ns/资源容量不增加，不另起CPU演化或默认CPU参考投入。

## 7. 必要检查与执行回执

本卡有限检查确认10个family/root行、正式完整dispatch计数、20审核族/精确mask、新尾156..239无重叠与保留零、DM/RX不同Config偏移，以及T639阻断源码SHA与当前一致；**不模拟物理、不运行产品、不证明审核producer已部署**。独立客观后继需核真实审核缺一必拒、坏qNext/错来源/中点未消费/重复力与时间/漏micro/伪scope证书/错Expected/CPU补位/旧缓存混版、全拒BLOCKED与accepted全unapplied、父ON/OFF物理逐字段、视觉同token/真实cold。失败先architect分流。

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部分支受阻先完成其它可做项；阻断原卡含未完、复现原错/证据/SHA、已试/不能继续原因、所需真实前置与接手角色。成功结束待验收、失败看评论。不允许修改D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不改其它列/他行/队列或滴答同名清单。GPU驻留、CPU停投、前景背景仅XComponent+shader；FAULT或明确STALE，禁Canvas/软件GL产品回退。目标1–11不变、10暂停，水线13→14→12、旧湿M1暂缓；无manager新验收门。
