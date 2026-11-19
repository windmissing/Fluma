# T552：V5逐段身份和G9规范seed补齐

> 2026-11-19 T619生效：同名segment-wire JSON已同步T527 parent SHA及T619公开身份表引用；root cursor64/token/capsule按 [T619](architecture-T619-v1.md) 生成和读取。段地址/ROOT7/FULL61/CONTROL revision例外和G9 seed均不改；不得复制T614私有cursor32作公开root64。

> 2026-11-18 T614生效：真实opcode切点、GPU不可变入口cursor32、Meta2及FULL CONTROL revision2以 [T614修订](architecture-T614-v5-entry-cursor-v1.md) 和同名JSON为准；G9 seed无变化。

2026-11-17，architect，不使用真机。消费T517 7dd6e58阻断回执、T548 cdb4c77局部交付、T527/T535/T543及实际shader。两项均是此前工程合同遗漏，本次分别给出可独立实施的完整机器表；不让developer猜值、删审计或用helper冒完整功能。未运行业务测试/真机。

## 1. V5非prefix数值合同

权威机器表 `architecture-T552-v5-segment-wire-v2.json` 完整展开四profile×ROOT/FULL的每一段，不改原offset/byteLength/stride/容量。**正式确认kind=descriptorIndex+1**，它是该mode目录内的编码，不是跨mode恒定的物理事件kind；reader必须同时核mode/目录。prefix仍kind1、segmentRevision2、header2/policy1和T535固定mask。非prefix容器默认segmentRevision=1，T614例外FULL CONTROL_ROOT_0..4为2；内嵌SOURCE/QUANTUM/control/input各行自身magic/revision/seal按原V5协议，不从容器revision推断。旧helper的kind算法至此才成为正式合同，旧header1/错误版本Validate不得据此继续使用。

内层数值也明确：SOURCE128 magic876950349/rev5、seal在word31为3235774468；CONTROL128 magic876825680/rev5、seal同word31/3235774468；PHYSICAL64 magic1163088455/rev5，**没有通用末word seal**（word63依kind可能是segment等operand），以原kind usedMask及paired planned/sealed游标授权；PROVENANCE32 magic894456396/rev5、word31 seal3235774469。不能把外层descriptor seal或“版本5”机械写进所有行尾。现m3b/protocol_generated.h的source/quantum/control revision仍4而实际V5 writers写5，是待T517同源生成器对齐的已见实现缺口；SOURCE/CONTROL旧seal数值保留是协议事实，不一律改成aux seal。

逐段固定身份规则如下；机器表把每行的固定数值和动态绑定分别列出，动态值不能用0代替：

| 段 | ROOT rootIndex/role/trial | FULL rootIndex/role/trial | branchEpoch |
| --- | --- | --- | --- |
| prefix | 0/0/0 | 0/0/0 | 0 |
| FINAL_STATE/FINAL_SOURCE_CONTEXT、三种FRAME | 0/0/0（当前档关联，FRAME是聚合容器） | 0xffffffff/0/0（全plan聚合） | 0，不假装某单role |
| CONTROL_ROOT_i | ROOT i=0 | FULL i=0..4 | 0；内部每行自己有branch |
| BASE_STATE_i/BASE_SOURCE_CONTEXT_i | 无该段 | i/0/0 | 0 |
| Qq_ROLEr_PHYSICAL/PROVENANCE | 无该段 | q+1/r/0，q=0..3、r=1..5 | 同root同role CONTROL128行10/11 |

role数值沿现有协议：0 ROOT、1 REQUESTED、2 CLAMPED、3 OLD、4 AUTHORITATIVE、5 CONSISTENCY、6 SOURCE_TRIAL。SOURCE_FRAME是聚合八trial原件，其descriptor role/trial为0，不把全流冒成某单trial；真实SOURCE128逐行校原trial/branch字段。QUANTUM五个固定role不是SOURCE八trial，descriptor.trial=0且control行8应为0。ROOT FRAME可能保已接受frame前缀，descriptor关联当前档而非声称每行均由当前root产生；普通ROOT不冒完整plan重放。

prefix arena/generation仍0。**非prefix arena=descriptorIndex，generation=header.traceId**，是不可变cold block内的保留段身份（traceId非零U64且不得复用），不是GLuint/现场GL arena或可写权限。新增明确此表示层语义，live producer owner/context/permit和真实GPU arena资格必须在采集前独立核验；不能把cold arena当热lease。完整权限键为header完整scope+traceId+descriptorIndex+rootIndex/role/branchEpoch，reader不得跨档或跨role拼段。同physical/provenance pair拥有各自段号，但root/role/branchEpoch和ordinal区间必须一致。

## 2. 根槽、引用、计数和未执行

ROOT header.rootCount=1；FULL=5，表示**已分配ROOT_RECORD128槽数**，不是声称五根都执行。FULL槽0固定G1/SOURCE、槽1..4固定Q0..Q3；无SOURCE时槽0显式NOT_EXECUTED，不把Q0移入槽0。ROOT槽0的rootKind来自最后真实已提交root（1 SOURCE/2 QUANTUM）；尚无任何提交root时返回NO_COMMITTED_ROOT，不造空成功档。header.flags仅bit0表示FORENSIC_INCOMPLETE，其它0。

RootRecord.flags在本合同具名：bit0 NOT_EXECUTED、bit1 FORENSIC_INCOMPLETE、bit2 SEALED_COMPLETE，恰选一；其它0。NOT_EXECUTED槽保固定rootKind和静态segment引用，其nonce/时间/commits/输入SHA/结果及scope动态身份为0。它是已分配但未执行槽，不是rootCount以外全0的未用槽。执行槽nonce/plan/base/proposed/input/sample/deadline等取同代原件；rootRecord的sealed不等于publicationEligible，失败前缀不得冒提交。

ROOT record0：baseStateSegment/baseSourceContextSegment/firstScopeSegment为0xffffffff，scopeCount=0；currentState=1/currentContext=2/source=3/control=6。FULL record i：baseState=11+2i/baseContext=12+2i/currentState=1/currentContext=2/source=3/control=6+i；槽0 firstScope=0xffffffff/scopeCount0，槽i=1..4 firstScope=21+10(i−1)、scopeCount=5（pair数，每对占两个相邻segment）。prefix中completion/input/quota/cursor指针分别为2304+256i、3584+64i、3904+256i、5184+64i（U32 word，ROOT i=0）。所有公式和引用均在机器表逐槽展开，0xffffffff只表示不适用，不能当数组下标。

| 段/状态 | plannedRows / sealedRows / validRows | body/ordinal |
| --- | --- | --- |
| prefix | 8192/8192/8192 | T535 fixed self SHA |
| 已执行STATE/完整SOURCE_CONTEXT | 容量word数三者相同 | 完整原始字节含规范保留0；ordinal全0 |
| SOURCE_FRAME | owner.sourcePrefixRows三者相同 | 仅已提交可读前缀；容量尾0；SOURCE的physical/aux区间0 |
| 普通PHYSICAL/PROVENANCE_FRAME | accepted state header.eventUsed（38）/V5 provenanceRows（179）分别三者相同 | 非零前缀first1/last=count；两种各自区间，另一种0 |
| CONTROL_ROOT | 完整Q在ROOT为6/6/6、FULL为8/8/8；SOURCE为0/0/0 | 前6个CONTROL128；FULL再含后5个Meta32，不能把Meta当CONTROL128；SOURCE无preview表，body0、flags2；容量尾0 |
| FULL Q scope pair | 本role实际planned/sealed，相对本scope起点相减；valid=sealed≤planned≤容量 | 两descriptor都绑定同physical/aux的first/last；first=start+1、last=start+sealedCount，空则0/0 |
| NOT_EXECUTED | 三者全0 | flags=1、branchEpoch0、所有ordinal0、整段body0；固定kind/目录/role/root/arena/generation/rev仍保留 |
| 失败已进入scope | 保真实planned/sealed/valid | flags=FORENSIC_PENDING=4；只sealed前缀可消费，不能把pending补seal；body超过planned仍0 |

已完成段flags=SEALED_VALID=2，未执行=1，失败前缀=4，未知/组合拒绝。失败已进入但尚无sealed行仍属FORENSIC_PENDING而非NOT_EXECUTED；不得用q0/事件0反推未执行。BASE锚点属于root实际进入时的完整前态；一旦该根进入，即使该根失败，BASE仍完整合法、flags2；仅该根从未进入才是零段flags1。FINAL_STATE/CONTEXT始终取真实CURRENT成功前缀，不取最后失败私有bank。

CONTROL的128word stride是容器分页粒度，不意味着每128word都用旧control128字典。ROOT完整Q只有前768word，后面全0；FULL完整Q前1024word整体封存，Meta末尾到1024的96word和1024..2047全0。已进入但控制表未完整封存时保守planned6（ROOT）或8（FULL）、sealed0/valid0、flags4，body可保原始forensic字节，不能据其未封行授权重放。SOURCE control固定全0但其实际输入/八trial在SOURCE128和SOURCE_CONTEXT内，SOURCE完成与未执行通过descriptor flags和rootRecord区别。

FULL身份/游标必须在**本role SCOPE_FINALIZE/原保留copy切点**锁定，不能等CONSISTENCY覆盖WORK后才从“当前cursor”取AUTHORITATIVE身份。control role行槽为r≤3?r−1:r（0/1/2/4/5），PLANNER_END为3；branchEpoch=row10/11、nonce=row4/5、role=row7。physical/aux起点取该次v5BeginScope的实际参数；planned/sealed终点取同次v5Cursor[0..3]；不能拿最后一次compound的cursor5/6当scope起点。AUTHORITATIVE可从frame非零起点复制，descriptor仍保原ordinal区间，reader据此归一化而非私改行内ordinal。

FULL在原已预留8192B/root的GPU control保留尾部存**每role入口/末尾原GPU cursor32各一份**：word768起，slot=r−1，每slot64word，入口在768+64slot、末尾在800+64slot，结束word1088。T614具名cursor32的9/10为不可变入口、11为valid、12..17为身份/revision2、31为seal，0..8仍原mutable字段，18..30零。SCOPE_INIT先清无效原件，INPUT_E_REQUEST内部BeginScope在首credit前封入口；host在INPUT_E_REQUEST dispatch后及SCOPE_FINALIZE后copy，不能把前者mutable0..3冒首事务前值。保原barrier/owner许可；无新增分配/物理dispatch/热map/完成尾/第五binding。最多10次copy/Q、1280B/Q，四Q40次/5120B；内部重放只重置mutable，不覆盖逻辑scope入口。

冷采集统一fence/安全点之后，builder用这两份原cursor和本role control行构造公开CaptureScopeMeta32（root/role/nonce/branchEpoch、pStart/pPlannedEnd/pSealedEnd/aStart/aPlannedEnd/aSealedEnd、entered/complete/firstError），只做字段搬运/checked减法，不预测执行。公开控制段前6×128word不变，word768起改为5条Meta32（r−1索引），结束928，928..2047为0；内部raw布局不作为公开wire。完整字典见机器表，builder写body后seal。原control copy3072B不变，入口/末尾raw快照copy额外如上；不存在“需使用未证明空闲的原WORK尾部”前置。ROOT模式不申请该FULL保留集，也不伪造五scope Meta；其公开control尾部全0。

每scope pair的CaptureScopeMeta与control的nonce/role/branchEpoch相互核；完整control明确NOT_EXECUTED的role其Meta全0，不能因dispatch发出就伪造scope执行。若硬错导致control/入口/末尾任一缺失，则标META_INCOMPLETE/FORENSIC，不能猜未执行或报FULL完整。T614字典标识`T614_CAPTURE_SCOPE_META_V2`，Meta自身revision2，FULL CONTROL父container revision2；ROOT CONTROL仍1，原control128行格式未改。

所有非prefix SHA仍覆盖完整byteLength，含零尾/NOT_EXECUTED零body，prefix只按T535固定32B虚拟归零。两slot/TTL/GPU保留/原四profile最大524375552B native及894457592B已知并存包络不变；源数据/record缺失报INCOMPLETE，不能选择方便的0补身份。机器表包括有限正例与wrong kind/rev/root/role/trial/branch/range/count/seal/未执行非零反例规则。

## 3. G9合法S40 seed修订与SHA链

T548报告属实：T543模板列9为0，但g9AuditMassGeometry要求列9==S+F=40Q。这是本角色漏填派生缓存。**不删审计、不在首次审计前偷偷修缓存后仍宣旧SHA。** 本次新增独立 `architecture-T552-g9-seed-manifest-v2.json`，旧T543机表及T549冻结资产原样保留为历史；新交付引用R2表。

新seedId=`G9_SEED_S40_ORIGIN1_R2`、seedRevision2：128列word0=41943040、word6=1、**word9=41943040**；其余列word0，完整bed/source ledger/总量/物理参数与R1保持。allOtherWordsZero只在排除全部显式赋值后适用。M0仍5120Q（U64），不生成SOURCE事件。所有其它派生缓存逐字段按现有审计复核，不把列9这一处修正当作未来可以省略cache审计的许可。

Config64容器仍G9C2/rev2，profile仍G9_SPF_Q20_V1；**parameterRevision升3（word11/INPUT[25]），seedRevision(word13)升2**，word32..39引用新templateSHA，完整parameterSHA重新计算；候选物理设计SHA和adapter SHA不变，研究默认OFF。旧parameterRev2/template旧SHA不能用于新R2，readers严格核一致；不是把旧R1测试/签包追认为新身份。template header的动态run/owner/context/inputEpoch/parameterSHA规范0，静态字段含正确缓存；配置SHA不写进自己的template前像，避免循环。

唯一时序：生成合法模板→hash模板→组Config64/计算parameterSHA→新run填最终身份与parameterSHA→**hash最终实际上传字节**→上传同一原件到五banks→初始GPU完整map/质量/来源/缓存审计→确认通过后首次CURRENT。其间不能在hash后再改header、以GPU g9Derived事后补缓存冒原始上传hash。初始audit可以只读复核或写回相同派生值，不能改初始物理/ID/时间/commit或以空床先发布。原single-vector暂存、两次初始化dispatch、INPUT/cold V2双证据及信用不变。

机器表给完整模板稀疏赋值、Config64全部64word、template/parameter/示例actual三种SHA及示例动态身份；生成器引用明确指R2。T543旧检查程序只代表旧历史，不能运行后覆盖本表。已有T548局部helper须同步这三层身份和冷SEED_META、UI声明，不能仅patch column9后保旧hash。

## 4. 接力与v2边界

T517对应分支现在可独立继续完整ROOT/FULL/五BASE与SOURCE_CONTEXT、collector/reader/跨route信用；T548对应分支现在可独立继续完整adapter/初始audit/cold证据/签包，不等待另一分支。T554动态动员独立profile必须从R2的**合法缓存与hash时序**派生新模板、再加八个新header参数和新profile身份，不原地覆盖R2，不污染T549/R1冻结基线。

交developer：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿及两机器表完成原卡当前完整范围；职责仅开发/编译/提交、不执行测试，不将独立tester/设备加作开发关闭前置。所有前景背景新XComponent+shader，失败FAULT/标明STALE上一完整有效帧，禁止Canvas/软件GL产品回退；CPU停投，250ms/GAP/120Hz配额/显示30fps不改。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它项；具体阻断原卡报告未完/原错/复现/证据/尝试/不能继续原因/前置与接手角色，失败本人行看评论，成功结束待验收。架构补齐不冒原卡已实现，不新增manager验收门。
