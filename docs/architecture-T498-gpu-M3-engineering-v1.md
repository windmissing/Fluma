# T498 · M3首阶段工程闭包（2026-11-16 V2控制gate已闭合）

2026-11-14，architect，P5，不使用真机。消费T497正式算法/manifest/30条反例及实际M2 pass/布局。[工程机器稿](architecture-T498-m3-engineering-manifest-v1.json)展开四档SOURCE/QUANTUM stage、银行、四binding和资源，[静态核对](analysis-T498-T503-contract-checks-v1.json)保原件指纹及检查。**2026-11-16续办：T508正式补充已消费，完整工程gate为ENGINEERING_READY / 产品NOT_IMPLEMENTED。控制实现以[时间gate工程v2](architecture-T498-m3-blocked-control-v2.md)及机器稿V2为准。** 不编码产品、不删控制/守恒门，不将部分稿称完整可实施设计。

## 1. BLOCKED时间账已由T508补齐并工程闭合

2026-11-14发现的正常blocked无独立丢弃字段、pending/carry/等待pulse和真实新输入未定义问题，已由正式T508补齐。2026-11-16消费当前T497/T508 SHA，冻结独立GRS_M3_PROGRESS_CLOCK_V3和GRS_M3_BLOCKED_QUOTA_V1；完整方法/短事务/Run同步退休/producer意图key/reader与资源见[控制工程v2](architecture-T498-m3-blocked-control-v2.md)。本稿后续stage/银行/资源仍有效，旧阻断保留在原卡和9fbf164历史，不再作为当前设计前置。

G=A+X+P+O+R+B；120*creditedElapsedNs=1e9*G+C+Z+BC。入口active与pending丢弃分账、carry归BC，WAIT/REARM fresh归Bwait；新输入arm后第一个新pulse整段仍记等待，之后才可积可运行quota。三条必要expected和全部21补充fixture已静态核，局部carry123456789不可伪配整数elapsedNs。此为工程闭包，尚无产品实现/设备通过结论。

## 2. 模块、版本与完整交付范围

独立`gpu_resident/m3a/`实现，不对当前M2生成器做全局字符串替换。建议文件：`protocol_generated.h/layout_generated.h`、`state_banks`、`work_layout`、`scope_control`、`source_kernel/source_transport`、`quantum_kernel/pass_table`、`root_owner/run`、`completion_gate`、`scene`、V4冷reader/collector及同源host适配。具体语言/头源拆分可按工程惯例，模块职责不得合成host预演。

采用同一GPU DSO内独立m3a object/static模块及显式冷route适配，共用一个T475 store/file worker；M3 shader/layout/scene字节与ABI形成独立key。切route先完成旧owner/context退休，不能两个物理pipeline同时保默认运行，也不能另加载一个DSO在同cache目录启动第二writer。若将来选择独立进程/DSO，必须另冻结store目录/全局pin与内存预算，不套本次单store账。旧NAPI和默认route行为不改，新V4 API/能力前缀独立拒未知版本。

T497 wire/layout/algorithm/source/scene4、medium3、features15、channelBitmap7、单H支持域为固定输入；numeric/time/angle/motion1、water2、optics2按manifest。N128/Q20/CQ/Hq/T16/±30°、lambda R8/Q16、40轮waterLevel/64轮η、package/Bmax/连续ID、单来源向量原子性及全部原审核不变。必须提供omegaIndexPerSecond，缺值CONFIG_REQUIRED；waterRate8Q/s仍候选/UNCALIBRATED，不擅自当旧默认或已标定效果。

V4独立冷选择/新空run，旧M1/M2默认、wire、shader/cache key、reader不动。能力矩阵显式拒L/U、percolate/deep/relax/correction/lift/advection/S3/S4/45°等未支持通道，不空跑冒支持；不宣旧F64逐位等价。profile/mode/参数在开run前冻结，不能跨版本复用旧pub/lease。

developer整卡范围必须包含完整REQUESTED→CLAMPED→OLD→AUTHORITATIVE→CONSISTENCY闭环、普通源/WATER/湿向量源72 stage、occupied medium所有writer、完整trial/root replay、控制/身份/状态接受及scene/cold接线；不能把阶段日志或单个solver当交付。不先CPU实现、不hot读候选/seed计划。

## 3. 银行、WORK与四binding

五个完整state bank，root内固定角色：CURRENT只读；ACTUAL候选；SPARE独立replay；SELECTED和BRANCH各为一个combined GL对象（state＋一份e行semantic事件）。CURRENT/ACTUAL/SPARE使用原M2三state对象的存储额度，但**新M3绑定与shader独立**，不能假设M2私有candidate指针就是所有角色。旧M2 WORK中的spare/旧arena范围按保守账保留，不再额外扣一次节省。

只有通过全部根gate后native原子交换CURRENT/ACTUAL角色及对应元数据/lease；GPU名为promote的pass只使私有候选READY_TO_ACCEPT，不写物理CURRENT。失败trial/preview/late root不能改accepted bank、有效来源/dedup前缀或public token。所有候选从同S0/B0开始，branchEpoch U64 checked递增，role/trial/seed及完整root/base/SHA构成scope，恢复业务serial不复用branchEpoch。

SOURCE WORK在原M2完整范围后依次追加raw dedup slab8,388,608B、sourceMeta524,288B、staging2176B、scope scratch32,768B。QUANTUM WORK追加sourceMeta524,288B、scope scratch32,768B及SPARE期望e×256B；sourceMeta只从已接受有效前缀GPU copy，明确copy bytes/barrier，不在host解释来源物理数据。SOURCE的raw slab按dedup index，meta按sourceEvent index；二者不可混用。ordinary payload512word全零，BULK完整32+512word身份比较，不只比hash。

每WORK的32KiB新增scratch独立拥有，descriptor/求解向量/投影残差/first-error/计数分成定长段，保留段清零，禁止借旧活跃索引或第三份scope流内存。工程机器稿登记完整追加段；helper对所有地址checked乘4、16B内部对齐、半开区间不重叠/不越界作断言。combined event起点alignUp(stateBytes,实际SSBO alignment)，padding计实际分配；绑定整个combined/WORK对象时shader按字节/word基址索引，不能把不对齐逻辑段作为glBindBufferRange起点。

四binding逐pass冻结：

| pass族 | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| init/core/solver | CURRENT | 角色WRITE_BANK | PACKED_WORK | 该scope EVENT |
| replay | CURRENT或待核候选（按stage） | SPARE | PACKED_WORK | scope原EVENT |
| scope final/select | BRANCH combined | SELECTED combined | PACKED_WORK | control128 |
| AUTHORITATIVE finalize | SELECTED combined | ACTUAL | WORK期望流/控制 | ACTUAL frame EVENT |
| 最终semantic compare | SELECTED combined | BRANCH consistency combined | PACKED_WORK | ACTUAL |
| pack/audit/seal | CURRENT | ACTUAL | PACKED_WORK | PUB或COMPLETION |

AUTHORITATIVE finalize必须在CONSISTENCY复用WORK期望流之前，对实际semantic事件与SELECTED逐字比较并锁存scope身份/匹配结果；最终compare再比较CONSISTENCY/SELECTED及ACTUAL状态，核该锁存结果。否则最后一个pass无法用四binding同时读三个state和所有独立事件。此安排使用已有scope finalize，不新增dispatch或拿hash替代全流。

semantic比较默认全部物理word。仅design允许的role/branchEpoch/物理bank或arena地址等诊断表示可按显式schema规范化；每项先核自己scope合法。不能笼统排除header、sourceMeta、budget、ID/generation、angle、tau或event operands。机器稿要求developer生成逐word比较mask并对照T497/T499，不允许凭“各角色不同”扩排除集。

## 4. stage、提交与错误序

SOURCE严格复用T497 0..71表：0初始化，1preflight/dedup，8trial×8stage，66终态/ledger、67root audit、68私有promotion、69pack、70pub audit、71seal。普通点源/WATER只首trial槽运行原单源prefix规则及occupied/J净空修订，所有λ/η及其它槽uniform no-op，trialsUsed0；不能误成湿向量all-or-none。BULK每seed共用一个trial的来源/ID/预算/驻留索引，任一未完成整向量回滚；fatal不降λ隐藏。

QUANTUM以实际M2 BuildPassTable为依赖：删六water-head stage2/3/4两色，换三个GLOBAL_WATER_LEVEL pass；删原28/25/26/27四根尾，得到core=D2−7。每scope加init/finalize，scope长度L=D2−5；排序网络的k/j/颜色/dispatch顺序仍逐条展开。根序列：

```text
PLAN0（初始化/预存guard/jRequested）
REQUESTED[L] → PLAN1（选中或构造CLAMPED）
CLAMPED[L]   → PLAN2（选中或转OLD）
OLD[L]       → PLAN3（选中或BLOCKED）
AUTHORITATIVE[L] → CONSISTENCY[L]
SEMANTIC_COMPARE → PRIVATE_PROMOTE → PACK → PUB_AUDIT → SEAL
```

五scope固定提交，即使未执行角色也运行统一GPU no-op；不能host读branch结果再选择跳过。Dq=5(D2−5)+9，四档549/619/594/664；G1＋F4上界2268/2548/2448/2728。机器稿按四档展开索引/角色/原stage/排序k/j；产品未实现，表不是执行通过证据。完成计数包含规定no-op提交槽，实际语义执行计数单列，不能把no-op叫物理加速。

64lane persistent solver/index/replay只能单workgroup，所有barrier前分支uniform；大copy/audit可按不相交域多组，无跨组自旋。每batch沿当前最多10顶层op及8ms提交墙钟上限，在每dispatch/续提/Flush/fence/accept/Swap检查原permit；算法内有界循环仍受设备watchdog，资源/次数界不是250ms证明。正常一root一尾fence/map1024B，一G1+F4至多五尾/5120B，不能逐seed/preview map。

首错顺序为实际执行pass index+1→role/trial/seed→scope ordinal→确定性检查word，锁存原reason/GL/EGL；后续固定stage仅诊断/no-op/seal。firstError完整tuple在定容WORK及control终态保，completion原firstError字段不借保留位扩装。未执行角色写NOT_EXECUTED原因，不让旧scope descriptor冒本scope成功。

## 5. 原子接受与版本化reader

V4 completion97/98为完成pass数，99/100 required=0x7fff/high0；不再读成旧64位stage mask。header145..160和completion173..188严格按T497命名映射；旧completion144水账、160..172 M2来源描述符不覆盖。三个8KiB control arena，16×128word，预留唯一PLANNER_END/首错终态信用，不能满后丢首错。

gate先核seal/revision/features/完整root和base身份/permit/真实GL结果，再核outcome：

- SUCCESS：required审核、实际stage表与counter、全部库存/事件/pub验证成立才原子接受；SOURCE后Q失败保成功prefix。
- METADATA_REJECT/DEDUP_REPLAY：按设计单次metadata接收及delta0，不增物理/source成功量，不凭零库存放松身份。
- FATAL_ERROR：保首错、隔离candidate/晚尾，FAULT或合法STALE，不fallback到OLD掩盖audit/GL。
- PLANNING_BLOCKED：保base commits、pub资格0、只控制结果，**不能走旧AcceptQuantum/成功bank分支**；其时间账和再入gate严格按§1及控制工程v2，走BlockPlanningControl独立事务。

Scene新token/revision4读取column16的T/eta/R：自由水深clamp(eta−max(y,T),0,W)，LAND不再额外抬eta；所有源/运动/床/CROSS/ARRIVE均用occupied面和J净空。背景前景全部XComponent+shader，同代真实lease/Swap前后许可，失败不Canvas/软件GL回退。

冷trace新V4描述SOURCE128/QUANTUM64/control128及root/role/trial/seed/branchEpoch、有效行/保留位/seal；旧V2/V3 reader拒V4，新reader不把旧字段改名复用。普通只保SOURCE全trial、ACTUAL流及preview摘要；FULL_BRANCH_TRACE在plan开始前预留所有20e行（20个独立e流block以免单SSBO超限）及descriptor，未足额WOULD_BLOCK。消费者退休前不得复用，完整/摘要schema不同。M3不借T503的M2冷payload协议解释V4，也不借旧4MiB timeline预算装完整branch流。

## 6. 四档资源与合法峰值

M2基线保守保留。QUANTUM frame arena行数为 **K+4e**，e=48K+16Kw+382；旧K SOURCE余量不抵新增，三份各比旧多512行，合393216B。不是误写成max(旧行数,4e)。新增2S state、三份新SOURCE128、3e scope流、raw slab/meta/staging/scratch/control照T497逐项计。

| K/Kw | payload B（无alignment/driver） | 单新SOURCE arena | 单Qframe arena | FULL_BRANCH额外 |
|---|---:|---:|---:|---:|
|192/64|61,257,552|2,638,336|10,926,080|54,384,640|
|192/128|65,229,392|2,638,336|11,974,656|59,627,520|
|512/64|129,767,504|5,259,776|26,736,640|133,027,840|
|512/128|133,739,344|5,259,776|27,785,216|138,270,720|

combined padding加`2×(alignUp(S,A)−S)`；WORK按整体binding地址0和显式word偏移，新增段内部16B对齐。GLsizeiptr/offset/count/单block、四SSBO/fragment能力、64lane/shared资源逐对象实际冷探测。FULL_BRANCH不能盲分成一个138MB binding；20段分别探测，逻辑总量仍照表并计descriptor/padding。program/JIT、FBO8wh、host/cold副本及driver退役另账。

同T500相位原则：新context完成旧retire后才建；冷warmup额外E只在Create、早于FBO，不把它与本代FBO同时累加。若M3采用新的预热pass序，需重新核其readonly/无quota和真实暂存，不擅用旧M2冷fixture当已适配。旧M1/M2与M3跨profile不同时拥有两份完整默认pipeline；如明确双实例实验，按实际并存另算，不偷扣基线或凭机型猜内存不足。

## 7. 接力与必要检查

T508正式设计与本卡控制工程v2已闭合，完整ENGINEERING_READY。developer按机器稿V2一次完整实施；不得把设计检查冒产品运行通过，不自动切产品默认。

后续developer，不使用真机。消费已闭合版本的独立manifest，全stage/角色/银行/shader/reader/controller同源生成，必要非机检查含：T499三十反例、8trial/seed全成全退、η/40轮残差/净空、medium全部writer、role/epoch污染、完整semantic比较、四binding执行、stage计数/审核、blocked补充状态表、四档资源能力与错误清理、晚尾/lease/Swap、旧reader拒未知；host GL仅离线设施。编译/签包及本角色自测即可交付，独立tester/设备另卡，不附加开发门。

T499独立输入/整数期望可继续，不新建CPU业务参考实现，复用既有oracle；未实现产品RED登记NOT_IMPLEMENTED。真机卡在实现/签包后带“真机”标签串行核真实root成本、offered/admitted、模拟速度/30fps/系统像素/内存峰值，不假许120fps或放宽250ms。所有失败补证先回architect分流。

共同prompt：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投。全部前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。GAP33,333,333ns/250,000,000ns绝对root期限/120Hz quota/30fps目标不改。成功原卡回执并本人行结束待验收；失败/关键缺证留未完项、复现原错、证据、已尝试、不能自行继续原因、前置/接手角色，本人行看评论。

2026-11-16续办已完成§1时间gate与全部本角色范围，独立静态核对见analysis-T498-control-gate-v2.json。本卡回原卡并本人行结束待验收；未实现/编译/运行产品或设备，不冒产品PASS。
