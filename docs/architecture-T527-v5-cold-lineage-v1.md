# T527 V5 完整冷谱系、容量与兼容合同 v1

> 2026-11-18 T614切点修订：同dispatch首事务前入口保存在GPU cursor32不可变字段，不从dispatch后mutable反算；公开Meta2及FULL CONTROL容器revision2按 [T614合同](architecture-T614-v5-entry-cursor-v1.md)。ROOT CONTROL仍1、header2/payloadV5_2/目录/预算不变。

> 2026-11-18 T584实现闭包修订：FULL必须保执行root的真实BASE及source上下文、五根控制与scope双stream，不能用NOT_EXECUTED代替缺对象。正式61段、既有预算和V5_2字典不变；缺口、唯一链接实现、捕获时机、sealed语义与跨route双槽生命周期以 [T584工程稿](architecture-T584-first-attribution-v1.md) 为准。

> 2026-11-17 T552补齐：所有非prefix逐段kind/index+1、revision/身份、根槽引用/行计数、FULL scope入口与末尾cursor原件采集及内层wire差异以 `architecture-T552-v5-wire-and-g9-seed-v2.md` / `architecture-T552-v5-segment-wire-v2.json` 为准。原T527表的offset/容量不变；本文旧“通用seal/角色隐含规则”不得代替新逐段表。

> 2026-11-17 T535修订生效：本文旧header rev1/摘要隐含规则由 `architecture-T535-v5-prefix-digest-v2.md` 取代；同路径机器表已升ENGINEERING_V2、payload V5_2、header2/policy1。仅自身SHA字节[1116,1148)虚拟归零，其他字节全部覆盖。容量/offset/预算不变。历史共享V1生成脚本不可覆盖当前表。

2026-11-16，architect，P5，不使用真机。消费T512/T506 V5及T498控制闭包、T503合同和T507实际helper。**工程合同交付，V5 cold适配NOT_IMPLEMENTED**。不改物理和旧默认，不让T522/V4等待本卡。机器表 `architecture-T527-v5-cold-manifest-v1.json` 给完整offset/容量/检查项；静态核对见 `analysis-T524-T527-engineering-checks-v1.json`。

## 1. 版本与完整性的精确含义

新增冷能力 `T527_V5_BOUNDED_COLD_V1`，payload `GRS_M3B_LINEAGE_COLD_V5_1`，页面协议 `T527_SEGMENT_PAGE_V1`。物理wire5/features31/channel15保持；控制clock仍T508/T498 V3，与物理和冷协议分开。profile为K192/512 × Kw64/128，冷模式ROOT_COMPLETE或FULL_PLAN_BRANCH；默认不预约FULL，不自动开启lift。

ROOT_COMPLETE包含当前成功前缀的完整V5 state、完整来源/证书/aux及SOURCE_CONTEXT、所封存frame有效SOURCE128、QUANTUM64+PROVENANCE32、control128、原input64/源32+512word、completion、quota及配置。不是只导物理流；来源表及有效cohort/depth/cert/aux全域不能用digest代替。普通档保当前根的审核上下文，**不宣称拥有全run的原始历史或所有preview**。

FULL_PLAN_BRANCH是下一完整plan的显式冷实验，最多G1+F4、五角色×四Q=20个物理/附属scope pair，并保每根nonce/base/branchEpoch/role/trial、实际输入/期限与控制结果。若要求独立重放完整plan，还必须有初始BASE及各根base锚点，不能拿最终CURRENT倒推旧床层。故本工程在T512已有20流之外**显式增加5个完整state锚点、5份control arena的冷保留**，再配最后CURRENT；G1无源/不足F4时descriptor明确NOT_EXECUTED，不造根。失败/取消时可封FORENSIC_INCOMPLETE，但不能报完整plan成功。

这是一份plan档案，不是无限历史。ROOT请求在真实owner冷安全点采集；FULL请求须在plan开始前预约全部信用并冻结初始身份，不能plan中途打开再伪称前半完整。20流既有上界指每Q五role，SOURCE八trial已在SOURCE128连续流中，不再把它们假装五role Q。

## 2. 扁平owner block与分段schema

每slot一块精确capacity U32[]，无自动增长。固定32,768B prefix含header256、64条descriptor32、最多5份completion256/input64/quota256/rootCursor64、完整source命令32+bulk512、配置与收据；所有实际段布局由机器表生成，保留/未用内容0。大state和各流在prefix后，offset checked U64→size_t；逻辑连续不要求另造第三个vector。

descriptor32字段冻结于机器表：segmentKind/rootIndex/role/trial、branchEpoch、arena/generation、byteOffset/byteLength、stride/validRows/plannedRows/sealedRows、physical/aux ordinal区间、flags/revision及SHA256八word。header256为独立V5C1 magic/rev2（T535，word15=policy1），绑定完整scope、request/trace、参数/layout/shader及外部包声明摘要；U64均low-first。ROOT_RECORD128共最多5条，关联scope根索引/nonce/base/source与physics commits、input/参数SHA、root期限与样本资格检查时刻及outcome，不能仅凭scope全局一个nonce解释五根。8,576B configAndRootRecords段中前2,560B为5条记录、其后4,096B完整cold配置胶囊、余1,920B为0。机器表逐word列明，保留位0。对象号不代替权限，跨root相同ordinal不互引。包/ELF摘要只是声明，必须与外部签包收据核对后才是加载来源证据。

| 段 | stride | 核心校验 |
|---|---:|---|
| FULL_STATE / BASE_ANCHOR | U32 | 完整V5布局及cohort/depth/cert/aux、reserved0、origin/cycle/U/A |
| SOURCE_CONTEXT | 各子段字典 | 完整dedup/ledger/raw512/sourceMeta和有效前缀身份 |
| SOURCE | 128U32 | 全trial有效连续前缀，原始source/body与metadata一致 |
| PHYSICAL | 64U32 | root/scope/ordinal连续、role合法、V5 used-mask/seal |
| PROVENANCE | 32U32 | 附属自身ordinal/seal及对应physicalOrdinal，同scope交织切点 |
| CONTROL | 128U32 | preview/planner终态、实际role/branchEpoch、未执行原因 |
| INPUT / QUOTA / COMPLETION | 各自schema | input64原位型、clock V3全部B字段、物理completion V5不能混reader |

ENERGY/REQUEST的physicalOrdinal0合法，其余必须引用同scope真实物理事件；BED片按color/edge/fragment，LAND/LIFT按真实cut消费。actual frame prefix和20scope pair各自descriptor，不能把一个frame ordinal硬套五条流。planned可大于sealed，但成功档只公布共同sealed prefix；pending body只在明确FORENSIC段中保原错误/信用事实，不计有效行。每复合操作physical和全部aux共同seal后才推进prefix，只有physical seal不够。源/根失败仍保原首因，不补造证书或来源片。

## 3. profile容量与显式峰值

SOURCE_CONTEXT每份D=10,224,000B：96word控制基段、4096×64word原dedup胶囊、4096×16word来源ledger、4096×512word bulk原始位型、4096×32word sourceMeta。原WORK spare/scratch不冒权威来源；只打包已接受有效前缀，未用槽规范化0，段内布局见机器表。完整state未包含这些WORK权威域，不能漏。FULL每个base锚点需配当时同代SOURCE_CONTEXT，最多5份，最终再1份；这是重放必须的真实前态，不用当前ledger倒推。

设S为完整V5 state，D为上述完整来源上下文，A_s为SOURCE128容量，e为每scope物理行数，Ea为附属行数，P=(K+4e)×256，A=4Ea×128，C=8192，H=32768。

`Troot=H+S+D+A_s+P+A+C`。

`Tfull=H+6(S+D)+A_s+P+A+5C+20*(256e+128Ea)`。

ROOT容量保守含完整SOURCE_CONTEXT、SOURCE和整frame，即使当前kind只使用其中一部分。实际valid数据可小于capacity；不得把分配上限当已封事件数。FULL的5个base锚点加最终state共6S及6D，不把T512仅追加的20流额度冒完整重放输入。T512常驻input256原件已纳入cold prefix；本工程FULL GPU保留另保守预留256B输入副本，不能把旧机器表仅20流的增量误读成已包含所有锚点/来源上下文。

| K/Kw | Troot B | Tfull B | 两FULL slot B |
|---|---:|---:|---:|
|192/64|28,524,416|157,537,024|315,074,048|
|192/128|29,577,088|163,853,056|327,706,112|
|512/64|47,166,336|255,871,744|511,743,488|
|512/128|48,219,008|262,187,776|524,375,552|

单App全route共用两slot、一个builder、一个reader，实际capacity+reserved统一信用上限 **524,375,552B**，而非每route各开两slot。每请求还核本profile/mode上限；异profile旧FREE backing计账，必要时先实际释放再分配。小profile不声称可保留两份大profile档还符合其同档pair表。旧M2仍用原55,410,688B政策，切V5前不忽略其reader pin；跨route新请求先等旧slot可用或WOULD_BLOCK，不扩第三slot。业务物理无须等冷导出，新冷请求可拒。

FULL在GPU额外保留 `20*(256e+128Ea)+5*(S+D)+5C+256` B（256为本工程独立输入副本保守信用）；既有V5常驻payload另加。5锚点只做只读buffer copy，最多5个锚点/plan，共5(S+D)字节（分段buffer copy次数和descriptor实际另记）；不新增物理dispatch/热map，不跨binding偷偷塞第五个buffer。control在根尾原点复制至其冷保留区，原已map completion/控制输入写prefix不用第二次热读回。FULL新增的GPU保留、完整最大SSBO对象/实际alignment及冷复制墙钟必须单列冷capability/资源表。没有GPU信用/能力则FULL_UNSUPPORTED_RESOURCE，ROOT仍可按自身能力提供。

只允许一个GPU FULL builder保留集。完成caller-span采集并恢复后释放该集，READY/reader只pin native slot；若旧GL尚未安全退休，新FULL不得占第二保留集。正常resident、FULL额外GPU、FBO≥8wh、cache/JIT/driver退休、两native slot、diagnostic副本/writer、JS页与已存文件分别记账。K512/Kw128已知GPU payload+FULL额外为366,564,432B（不含alignment/FBO/driver），两native FULL524,375,552B；二者在旧READY reader pin＋新builder抓取时可实际同时存在，合890,939,984B，再加允许并行diagnostic副本/writer3,501,224B和页缓冲16,384B为894,457,592B已知payload包络，仍排除JS/固定对象/FBO/cache/driver。不得只取其中max冒并存峰值。这是上限设计，不是设备已可承受或RSS已回收证明。

## 4. 请求前信用、pin与生命周期

复用T503/T507状态机与单调ID/32terminal/60s request起TTL，但**不能只改MaxTraceBytes常量**：实际T507 `TraceManager.Begin/Layout/TracePage`硬编码route=m2、wire3、单state+单events及SOURCE128/Q64，V5必须独立descriptor适配器并移除该路径的错误假设。保原M2 adapter，限制由route/profile/mode policy取，不全局抬高旧限。

```text
BeginV5(expectedScope, profile, ROOT|FULL, replaceId)
  -> 校schema/scope、两slot信用、builder及FULL GPU保留信用
  -> reserve requestId+traceId+exact block + bounded descriptors
  -> ROOT等owner安全点；FULL等下一完整plan边界
  -> owner写caller-owned spans -> unmap/restore -> 验布局/共封前缀
  -> 锁内重核scope/取消/TTL -> 原子published替换
PageV5(traceId, segmentPageIndex) -> 一个有界页
EndV5(traceId) -> 撤资格/取消builder；已有旧published按原资格保留
```

保旧published直到候选全部成功；分配、捕获、校验、取消、超时失败不覆盖旧档。读pin跨替换/End/TTL/reset进入RETIRING，native block不能复用；容量按FREE/BUILDING/READY/RETIRING及锁外owner旧块一起计。writer/reader异常NAPI/取消均RAII释放。TakeOld→实际free→扣capacity→新分配严格顺序；不把指针移到局部就当free。

实际T507 `Trim` 先把slot.capacity置0再返回局部unique_ptr；V5迁移不得把这个返回点当实际free证明。适配统一保定长`detachedBytes`信用直到局部owner析构/显式reset后回执，再归还容量；Begin/Capability统计包含该项，任何trim新分配交错也不能形成未计第三块。这是V5具体所有权修订点，不因旧helper单元通过就跳过。

FULL等待plan期间也计request起60s、不因轮询续期。Stop/owner/context/input/session/run/config/permission等换代撤read/publish资格，旧builder归来先完成unmap/restore/quarantine，再丢候选。控制线程不强杀GL，不更改250ms/GAP。错误保first failed stage、原GL/EGL、request和原scope，导出失败不能覆盖业务firstCause。

## 5. 分页、文件和诊断fallback

page0只返回有界摘要/scope/revisions/counts/限额，不把64条展开descriptor塞进一个16KiB header；完整prefix作为普通数据页。其余页按**segment内部**切≤1024U32，SOURCE128/Q64/aux32/control128均整行，不跨段拼半行。state/input/capsule按各自word表；reader先核stride、counts、offset不重叠、checked乘加和有效封前缀，再按显式page目录访问。每页实际UTF8≤16,384B，未知/错行立即拒，不静默改stride。

至多64段，`pageCountMax=1+ceil(Tmode/4096)+64`（header加每段取整余量的保守界），包括最后文件manifest的UTF8归档信用为 `(pageCountMax+1)*16384`。最大FULL页上界64,076、归档1,049,837,568B；JSON大于原始U32正常，必须以实际落盘UTF8计。collector逐页write/fsync/hash，不在JS聚合完整数组或整段JSON；caller同时只保当前页，慢读不延长60s，不能导完大半就写COMPLETE。

独立大trace预算与4MiB timeline+diagnostic分开，不借用后者，也不因新trace抬其限额。诊断继续copy前gate1、Capture≤1,404,072B、writer≤2MiB、小fallback≤16KiB；使用新schema adapter附V5的模式、root/branch、paired sealed信息和输入/参数SHA。复用旧四scope首因/faultRoot/原GL/EGL/stamp，附加摘要预算≤2048B，T507实测旧fallback13,218B+2048=15,266B。实际实现仍须用真实writer量最坏escape/U64验证，不以此设计加法冒运行PASS；超信用保完整INCOMPLETE envelope，不能截断JSON或以cold错误替换首因。

成功manifest要求所有segment/page序号、scope/traceId、hash/bytes/seal、expected/actual段全集相符，缺任何谱系段即INCOMPLETE。保存原拒页/错误和最后manifest失败事实；finally End自己的trace和snapshot。JS/文件历史不pin native slot，仍需另外记录实际内存/磁盘/时限。

## 6. 旧档与实现接力

离线按archive schema+payload revision显式dispatch：旧T445/V3→原V3 reader；V4→其原独立V4 reader；V5→本V5 reader。V4 reader若尚未实际交付明确READER_NOT_IMPLEMENTED，保字节不转成V5或伪判通过。旧V3/V4 reader必须拒V5；新reader也不能给旧无来源床补造cohort/cert。在线先协商coldCapabilities和profile/mode；旧requestM2Trace/readM2Trace在T507新构建仍升级拒绝，新M2能力不接V5 id，V5缺字段/未知schema不降级旧API重试。

T517完整实施前置：T522可复跑完整V4、T512 V5全writer/state/semantic gate、T498 V3时间gate、T503/T507实际bounded机制及本V5 span/descriptor/reader/collector适配。T522/V4不依赖本V5合同；旧V5文档中的T508“待交”已由T498当前闭包解决，不能继续把它冒未解前置。

实现至少含V5 profile-generated布局/独立payload adapter、FULL预预约与锚点/20流保留、caller-owned span、同源NAPI/collector/离线reader。检查四profile ROOT/FULL、每段最大行/坏seal/错ordinal/缺cert/错cohort、同physical ordinal跨role污染、planned未封body、pin中替换/失败保旧/两slot耗尽/slot锁外旧block、TTL等号/ID耗尽/End pending、GPU保留未退休、超过raw/UTF8信用、I/O/NAPI/最后manifest失败finally。导出完整只是档案完整，不是物理或30fps通过。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投。前景背景全部XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。GAP33,333,333ns/250,000,000ns绝对root期限/120Hz quota/30fps目标不改。
