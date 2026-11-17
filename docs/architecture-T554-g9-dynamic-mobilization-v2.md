# T554：目标9动态动员与中点积分v2工程闭包

> 2026-11-17目标12独立湿域工程：见 `architecture-T567-water-sfp-engineering-v1.md`，新G12 profile/配置/单位/6204pass与孔隙水量合同。父DM干域/参数/旧机器表不变，不将旧M1/M3/G11或父2404计数迁名冒湿域支持。

> 2026-11-17工程修订：设备资格与无vertex SSBO场景沿 `architecture-T560-stage-capability-and-lab-triage-v1.md`；可选目标11仅按 `architecture-T563-g11-accepted-flux-visual-v1.md` 独立组合接入。父DM物理数学/参数/机器表不变，新组合的视觉资源和dispatch不能复用父计数冒充。

2026-11-17，architect，不使用真机。消费 `algorithm-G9-dynamic-mobilization-v2.md`、正式manifest/fixtures；产品NOT_IMPLEMENTED/NOT_VALIDATED。工程机器表 `architecture-T554-g9-dm-manifest-v2.json`，仅局部数学/表式检查见 `analysis-T554-engineering-checks-v2.json`。这是目标9内部升级，不新立目标10，不恢复CPU投入。

## 1. 独立命名空间与实施落点

新route=`gpu-spf-dynamic-mobilization-v2`、profile=`G9_SPF_DM_Q20_V2`，profileCode=LE32('GDM2')。新增独立 `entry/src/main/cpp/gpu_resident/g9dm/`（schema/配置/seed、dm_request/apply、midpoint、surface_flux、friction、audit/replay、scheduler、transport/controller、scene、cold及reader），与g9spf旧R1并列。只复用不改动的宽整数、program-cache、控制时钟原语；不能原地替换旧activation_deposit或让T549冻结头源/fixtures追随HEAD。bridge/CMake/类型声明新增dmResearchConfig/dmConfigure/dmSession/dmOfferModel/dmPulse/dmQuota/dmCold*及独立显式研究页，默认OFF，原入口和参数不变。

DM自己state/event/path/input/completion/cold magic分别LE32('GDS2'/'GDE2'/'GDP2'/'GDI2'/'GDC2'/'GDT2')，结构revision2，seal=0xC0DE0922；对应profile、layout/algorithm/shader/cache/reader/capability身份一起换代。时钟仍原GRS_M3_PROGRESS_CLOCK_V3，不把算法v2冒时钟v2。旧reader拒未知profile，DM不原地导入旧run，不只改旧state.profile再宣迁移成功；SOURCE零时间/去重规则与物理字段语义保留，嵌套capsule也必须满足新INPUT/profile身份。

Config64独立magic LE32('G9D2')/container2、parameterRevision1（独立profile命名空间，不是R1的rev1）、word2=新profileCode。0..15其余含K、g/muStart/muStop/touchBand/touchRate/kShear/settle、flags、seedRev1、adapter53401、坐标policy1。16..23为新研究构造对象SHA，24..31保T534一次UP配方SHA，32..39为DM自身seedTemplateSHA，40..47为本次正式设计manifest文件SHA（**不是T534批准标记**）；48..55依次七个DM参数及policyCode2=DM_IMPLICIT_POSTMIX_V2；56..63=0。完整64word LE SHA为parameterSHA；new HEADER106..113同这八word，SRC/Q/五bank/SPARE/重放/冷reader全部逐字保留并审计。

`G9_DM_RESEARCH_R0`仅未批准、未测的显式研究构造：参数值/支持域逐项在机器表，K512为默认构造，协议允许显式K192。旧G9_BALANCED_R1已批准身份不复用，不称产品标定或性能通过。

## 2. 合法seed与R1边界

DM模板从T552 R2的合法S40结构派生：每列S40Q、F/P/p/rest0、depth1、**列9缓存B40Q**、单origin1/sourceId1和U64 M0；加DM独立magic/profile及HEADER106..113参数，全部未用word0。T552 R2 cache修复不等DM算法升级：DM另算template/config/parameter/实际上传SHA，不复用T543或T552三种SHA。T548旧seed helper/真实adapter的局部编译不代表可直接给DM资格，T549原冻结基线不改。

唯一构造顺序沿T552：完整模板（动态身份/参数SHA0）→模板SHA→Config64/参数SHA→真实run身份和参数SHA→最终实际上传SHA→同一原件五banks→首次GPU完整审计→CURRENT。K192/K512模板字节数及示例SHA都在机器表。未经审计不得先显示空床；不能hash后GPU补缓存冒同一原件。初始两dispatch和seed临时数组沿父合同，不计模拟时间。

## 3. 原子根与12段表

SOURCE仍12pass；QUANTUM对原T524每scope微步的10段变12：ACTIVATE_REQUEST/APPLY替为DM_REQUEST/APPLY；原FACE_FLUX_MOBILITY替为PREDICTOR_FACE、MIDPOINT_BUILD、CORRECTOR_FACE，其余顺序不变。机器表从固定父表完整展开2404项，另列opcode数值；native/shader/reader由同一表生成，不能各写一个stage enum。

scope根控仍8段；六J发现804、权威392、拒绝前缀重放804、接受重放392、根12，共2404；G1+F4最坏9628。六J的uniform skip、第一成功选择、独立拒绝前缀、authority、完整accepted replay、首错/取消/回滚/late token均保。五state、四事件arena、4SSBO、单64-lane组、既有root fence和1024B完成尾不变；新scope每次完整初始化DM WORK，不跨trial残留。

DM_REQUEST先把**所有列**入口S/F/p/B及gx/gn写私有WORK快照，然后memoryBarrierBuffer+group barrier；64lane各处理两列，任何lane不得提前return绕过组barrier。几何用邻列冻结B=S+F及有向gx，墙外方向忽略。再各列做整数求解/来源前缀预检；DM_APPLY前不写权威库存、p、计时、ID或来源。全部阶段失败仅私有candidate可有诊断，不发布半root。

## 4. 动员请求、32轮和来源前缀

权威算术完全按设计§4，所有乘加/符号比较S128/U128 checked、Q20与Q16单位独立。RNE带符号，非负floor；head/Hs/chiS在定义的H/omega截断之前不得夹CQ。F0要求p0；gn≤0或坏库存是硬错，不返回“无动员”。几何阈严格相等仍chiG0，无epsilon/min1。

Eval(q)使用冻结F_entry、p及r=max(F_entry+q,hReg)，`u=RNE(p/(F_entry+q))`；H=min(B,max(Hg,F_entry+Hs(q)))，omega按设计截上限，`q*(120J*65536)<=omega*max(0,H-F_entry-q)`。取最大可行整数，不取试探中的某次q。Eval(0)算U上界；p0或kShear0用闭式U，其他用lo0/hiU+1、32轮gap≤1时uniform no-op；最后再核q可行和q+1不可行或被S/U上界排除。

冻结成本计数：Eval0一次，活跃二分轮每轮一次，最终q再一次；只有q+1同时≤S和≤U时再Eval一次，否则由上界证明。qNext记录q+1（包括S+1哨兵）、nextFeasible=0；不以0哨兵混淆真实qNext。qPhysical处H/omega/u复用最终Eval，禁止为event重算出另一版值。故实际Eval=2+activeIterations+I(next需Eval)≤35，保设计声明≤36的保守界；32轮不因早点收敛减少dispatch。每Eval的除以ONE/Q用精确移位/余数RNE实现，其余最多3次宽除（u/head/chiS）。记录活跃轮数，可离线复算操作计数；失败未封前缀只能给成本下界，不冒完整测量。

闭式分支也规范写finalLo=U、finalHi=U+1、activeIterations0、closedFormBranch1；隐式分支closedFormBranch0，32轮后finalHi必须finalLo+1。均独立复核q及边界；no-bed等q0仍按同字段规则给出确定记录，不能留上个微步的solver值。

qOrigin只从床top-first连续前缀到F稀疏来源表，遇第65种不可表达来源即止，不跳层；坏origin、质量溢出、事件信用错仍FAULT。qActual=min(qPhysical,qOrigin)，不得再乘旧band/rate或套V4床搬运额度。预留事件/来源写信用后原子迁移同q的S/F和map，p逐位不变、B/占据/势能不变、混合动能不增。qActual>0才清rest/标扰动；纯请求q0不伪造扰动。

qPhysical>0而qOrigin0是局部DM_ORIGIN_WAIT，保原列、继续FLIGHT/FLOW等其它阶段，让已有F可以离开释放来源空间；它不是root BLOCKED，不自动撤quota。前缀部分可准入，reason顺序严格NO_BED→NO_DEFICIT→RATE_ZERO→NUMERIC_RESOLUTION→ORIGIN_WAIT→PARTIAL→ADMITTED。每微步每列一条DM_NATURAL（q0也有），旧自然kind4 writer在DM profile关闭，旧kind4只留触摸专属band/rate激活；不重记同一次S→F。

## 5. 中点预测与真实更新唯一落点

DM_APPLY后核实际F波速CFL，失败回完整BASE再细分，不能clip q。然后FLIGHT_PATH/PARTICLE_FOLD；**在这之后**冻结预测器S/F/p/mode，与此前DM入口不是同一个时刻。P捕获在微步末，其反馈下微步才入新DM。

PREDICTOR_FACE复用原HR/床修正/gxδ算pTrial0，再按入口mode做完整δ摩擦prox得pEndPredict；仅WORK，不改物理p、反冲、耗散、mode/rest或模拟时钟，不执行真实来源迁移。MIDPOINT_BUILD做S128加法再RNE除2，pMid需F0→p0及|pMid|≤F·Vmax；中间pTrial/pEnd写S64前做checked收窄，超可表示/中点速度/CFL域按原试验可细分拒绝，128算术/坏来源等硬错保持FAULT，不静默clip。

CORRECTOR_FACE以同一S/F、pMid算新面质量q/动量jp/压力/床修正；真实p候选的基底仍pEntry，不能从pMid再加全步力。真实质量和动量必须来自同一组corrector faces，q0仍可传压力。静止mass mask每列为(pEndPredict==0 && 原modeEntry0且corrector pTrial在静摩擦预算内)，两侧都成立才把该面的q置0；墙面仍q0，压力不删。预测supported与correctorSupported均在WORK保存，不能把预测mode写回改变muStart/muStop历史。

FLOW_ORIGIN_RESERVE/PAIR_APPLY使用corrector面及原pEntry基底；随后原真实FRICTION_REST_DEPOSIT只执行一次、计一次支持/耗散、推进一次32/J restticks。停止判断改用**实际post-friction newP**的剪切及同一几何超阈，不读旧p/band；H降低、omega0不强清p/强制F→S。ORDERED_CAPTURE、MICRO_AUDIT_SEAL保全部来源/动量/局部及root能量门。没有无条件熵证明，整数位移0而能量升的场景仍拒绝；严禁加无对应势能的gravity外功、ε放行或删除审核。

停止几何项用FLOW_PAIR_APPLY完成后的实际邻列B，公式与DM相同但不能复用已过期DM_REQUEST几何快照；剪切的newP显式传入判定函数，不能在g9PutMomentum之前又从列中取oldP。F→S不变B，故各列停驻/沉积顺序不改变这份几何域。

## 6. WORK、事件及replay

新增WORK仅在父work末尾追加128×48word=24576B，不插入/移动旧offset；机器表逐字定义：前32word为DM冻结/求解/来源前缀，后16word为post-FOLD真实入口及预测动量/支持。无新物理库存/history/CPU sidecar。原face129×32word在预测时可暂用，MIDPOINT已存入口/结果后CORRECTOR覆写全部face；真实pair/apply只读corrector，不能误用预测face。U128临时值保寄存器/局部定长limb，禁止另开未计buffer；编译器spill/设备成本独立实报。

新增EVENT64 kind15 DM_NATURAL、kind16 DM_PREDICTOR完整word表/usedMask沿正式设计。DM_NATURAL stage=DM_APPLY，每列一条；flags的role位原样，qActual>0加compoundMutation(4)，q0加normalDenial(8)，未知位拒绝。Hs0Bound/HsPhysicalBound仅min(HsWide,CQ)的诊断值，不反用于物理计算。q/前后S/F/p、mapdepth、solverUpper/lo/hi/activeIterations/closedForm等由实际冻结数值填入，不拿CPU算值回填shader原件。

DM_PREDICTOR在**CORRECTOR_FACE支持结果已知之后**按列顺序封存，不先seal再补字段。字段physicalDeltaIsZero=1；predictorRole=1固定表示“私有动量预测阶段”，**不是uRole**，实际DISCOVERY/AUTHORITY/REJECT_PREFIX/REPLAY仍在原header/flags规范化路径验证，防止额外operand随执行role改变而破坏逐word重放。stage=CORRECTOR_FACE、trial=同原语义trial index，entity/from/to均列号，id/generation及未用字为0。预测事件不带物质mutation/denial位。

replay先验证各自event magic/revision/seal/role/trial/usedMask/新stage和kind适用，再仅按原规则清flags bit0/1比较全部语义word；predictorRole固定1照常比较，不增加“忽略operand27”捷径。自然/预测均从完整前态和同参数GPU重算，不能只比hash或让authority/replay仍走旧零速度flux。预测事件序在corrector后、真实FLOW之前；每scope所有ordinal连续，planned/sealed错不补尾。

QUANTUM requiredAuditMask=0x3ffff：新增bit16最大可行q/来源前缀/实际patch，bit17私有预测/entryP校正/一次力与时间；SOURCE仍0xffff，两新族N/A不作为QUANTUM的PASS。SOURCE复制/校验八个新header字段但不跑DM/推进时间。首错stage/NOOP/marker按新12段表，不借原10段下标。completion结构256word/读回1024B保持，新profile/revision/mask改变；原reserved统计位仍0，不为性能计数偷偷写旧保留字。

## 7. 资源、冷档与兼容

机器表完整重算两档state/work/event/path/resident/FULL/ROOT及页目录。N128，e=6N+2K，E=32e+4N+K+16，PATH=32×8K；每arena比父版本多1048576B，四arena增4194304B、WORK增24576B，state不增。K192/K512 resident分别55,339,648/98,022,528B；FULL额外六arena76,603,392/140,009,472B，单最大buffer12,767,232/23,334,912B。真实GL alignment/对象/FBO/cache/driver/旧profile overlap单列，不冒RSS。

DM冷capability=`G9_SPF_DM_COLD_V2`、page=`G9_SPF_DM_SEGMENT_PAGE_V2`，32768B prefix沿T543/T552双触摸record/Config64/SeedMeta，版本/全部SHA换新，不为prefix建self descriptor。BASE/FINAL及全部所选/权威/六试验的EVENT+PATH按新E重算；不能只扩shader后仍用旧cold预算。K192 ROOT/FULL=26,750,976/103,354,368B，K512=48,050,176/188,059,648B；新两slot最大376,119,296B。原来ROOT+2MiB/FULL+8MiB、两FULL+16MiB的增量由实际父版本复算，而非套旧V5上限。

每页仍≤1024U32/16384 UTF8，按segment整页/行解码，EVENT64与PATH16边界由新布局核，不把PATH误作event。保守pageMax=1+ceil(modeBytes/4096)+32，含最后一条manifest的文件UTF8上界=(pageMax+1)*16384；实际页数另算。单builder/reader、两slot、TTL60s不续、caller span、pin/End/ID耗尽/晚归来丢弃保留，collector不在JS聚合整档。旧G9 reader只读原版本，DM reader拒R1 header保留字段混版；旧R1输入body只有在新run/新profile/新参数身份下重新封装才可用。

切DM route先退休旧GL资源、撤旧scope并实际释放旧route native冷slot；旧reader/pin/builder未退休则WOULD_BLOCK，不强拆、不将锁外旧block逻辑释放冒信用回收。DM两slot守自身376119296B，不能借V5/M2信用额外并存旧profile；旧SOURCE/QUERY接口继续原预算。K512 DM FULL GPU+两FULL native+T552输入证据22528B+当前页16384B已知小计614,190,208B，仍不含上述driver/FBO/JS等。seed临时673792B应在cold阶段前释放，若实现改变生命周期须另实报并存，不虚报free。

最坏scope micro=190、每列Eval≤36，总≤875520 Eval、最多2626560次宽除；若128轮limb除实现，内循环上界还要乘128，不能只报2404 dispatch。p0/kShear0闭式路径实际不做32次Eval，但不能减预声明pass或关闭replay。root绝对250ms、双GAP33333333ns、120Hz配额/显示30fps不改；失败真报BLOCKED/FAULT及成本，GPU实时性未测，不以本设计承诺实时。

## 8. 开发与独立验证合同

T555 prompt：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿/机器表完整实现独立g9dm：新profile/config/合法seed/48wordWORK/12段、隐式后混合求解+来源WAIT、中点与真实一次力更新、审核与逐字replay、独立native/页面/cold/reader/cache/完整资源账，更新新生成器，不修改T549冻结R1资产。继承T550安全入口/错误契约；若尚未实施，具名拒绝而不放宽NAPI门。新touch仍原body配方，真实clock资格未证不冒真实输入；SYNTHETIC标域。developer仅开发/编译/提交，不执行测试、不使用真机，不将T556/设备/效果另加开发关闭前置。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它项。阻断依用户规则原卡写全部未完/原错/复现/证据/尝试/为何不能继续/所需前置与接手角色；失败本人行看评论，成功结束待验收。

独立tester（T556）先设计本profile专属D10起动—位移反例、DM maximal q/q+1、来源64满WAIT/部分前缀/硬错、p0/k0/32轮/舍入/宽head、p不变与混合能量、entry/mid/actual一次力、post-friction休止、全部SOURCE/QUANTUM/replay/Stop/迟到/坏seal与冷热身份用例；机制基线可先离线定义，GPU执行等T555正确包，不把局部公式检查当独立产品oracle。适用旧43语义/原touch负控保留，变动事件/质量量级另写新期望，不覆盖R1结论。设备真测GPU内成本/driver/fence/root/GAP/有效新帧，效测只看真实接受S/F/P新帧；触发真机的任务保真机标签/串行。失败先architect，不直接交designer或关能量门。所有后续仅新XComponent+shader，故障FAULT或标明STALE上一完整有效帧，禁Canvas/软件GL产品回退；CPU停投/defaultOFF/旧产品门不变，不新增manager验收门。
