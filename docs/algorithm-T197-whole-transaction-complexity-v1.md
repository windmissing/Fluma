# T197 全事务复杂度整体方案 v1

2026-10-28，designer；原卡 `6ae152f0e4b0ab6770185aa5`，承接已关闭但未交方案的T194，不重开旧卡。路线 `P011_WHOLE_TRANSACTION_V1`，设计交付 **DESIGN_READY_FOR_ARCHITECT**，未实施／未验证。唯一失败归因基线为T116 [V3](architecture-T116-T156-whole-transaction-v3.md)及其analysis JSON；T123/T128/T130/T137/T183只按各自已证明范围消费，不代替V3。本方案覆盖现行双沙wet/dry与public source/step，**不是T196目标7方案的移植**。

不使用真机、不重跑产品测试、不改业务、不回退共享修改。compute目标6保持最高调度优先，严格O2隔离对照继续但不替代结构方案。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外；不改其它行列／队列／滴答同名清单。所有画面含背景前景只走新XComponent+shader，禁止Canvas上屏／失败回退；故障FAULT或明确STALE上一有效完整新管线帧。manager按完成通知登记，不加验收门。

## 1. 结论、版本和不可偷换的语义

推荐“**单owner整事务执行＋可失效的派生数据结构＋原审核切点／原RN求和＋原子工作量准入**”整体路线，分三层：

1. `TX_EXACT_STRUCTURE_V1`：保留输入相同且被执行的完整source/λ/preview/authoritative流程，改变查找／派生数据维护／物理复制方式，不改变浮点运算顺序、最小pair或审核结果。它仍有保留的线性fold与场C²项，不能声称所有成本都成为O(N+B)。
2. `TX_STAGE_CERT_V1`：只复用**相同read-set位型且无失效**的collect/audit/派生结果；每次原逻辑审核都必须被实际执行或同等证书回答。未证明的predicate保留原执行，不能“最后审一次”代替原中途失败。
3. `TX_OWNER_ADMISSION_V1`：整事务专属owner、有界输入/输出队列及完整工作量预留；入队ACK不是源成功，超包络拒绝／背压在任何材料修改之前。异步API／新增资源拒绝是显式行为版本，列为**DECISION_REQUIRED**后才能产品启用，不默改现行同步接口。此设计任务不代替该裁决。

三层一起处理调用放大、数据重复、UI调度和资源，不再派“排序换一个”“merge移循环外”作为整体答案。CPU算量、GPU成本、UI响应、档案完整性分别报告；搬线程不降低原算量，GPU微核不减少M/J/A。

强制保持：旧group五字段、原`abs(RN64(yA−yB))<=radius`、全局字典序最小(idA,idB)、同F64加权表达式、birth分包/尾包/抖动／随机调用顺序、nextId/free-list/来源epoch、300/28等既有预算及比例分配／refund、每个原错误cutpoint与首码／ordinal、事件顺序/lineage、source成功后step失败不撤源、各λ回滚与preview不污染authority、观察／发布身份。不得改半径/F32、批量预选pair、合并source请求、丢物理步／审核／事件，或以热点采样假定实际最坏输入。

T116确认R1/R2长同步任务复发；真实N/L/B/M/J/A/R/V等为NOT_CAPTURED而非0。R3新run先CLOSED，258行是旧step3资料，不签新注入稳定。设计不等待旧缺测，但不据此填写真实负载、唯一首因或设备毫秒收益。

## 2. 变量、边界和完整调用图

统一：C列、N池容量、L活粒，K=pow2ceil(N)只用于既有树/GPU；D为一次选对结果。B_req/B_ent/B_commit分别请求／实际进入birth循环／最终提交birth数；M_find含无配/失败，M_commit另列；J为seed调用；Q/H分别request/motion行；T为票据；A为逻辑审核切点、A_num为实际数值fold；P为全池/场复制；R为某外源真实λ尝试≤8；V为实际preview≤3；S为帧内子步≤2；U为实际索引key插入/删除/位移更新；E为事件、Lg为lineage字数、Z为归档字节；Fch为完整field链执行数。所有总量必须带scope，不再把N当B、B当M、V当实际lift次数。

```text
public source (DIRECT: 可能在frame之外；QUEUED: 每substep最多取1)
  input/run/geometry/time -> backup(sourceBefore) -> trial lambda r=1..R
    capacity / eta solve / original field checks
    ordered columns c, phase H then L, positive mass -> seed j
      begin/freeze -> collectBirthHeaders validation
      row apply -> birth k loop
        if freeCount==0: find current minimal pair -> original mergePair
        original birth arithmetic / ID / cache / journal
      row audit -> tail audit -> finish audit / events
    original success/failure check -> accept once or restore lambda/source
  source ledger/epoch, publication commit (only actual success)

frame envelope (DIRECT source calls charged separately + frame's queued sources)
  original clipped dt -> S=1..2
    queued source success/failure BEFORE substep backup
    moisture/consumed shake input -> backup
    planner: actual REQUESTED / CLAMPED / OLD previews
      restore -> collect and shadow allocations / particle predictions
      candidate evaluation -> discard or choose; no fabricated lift seed in dry collect
    authoritative collect -> original total/correction reservation
    restoreMotion -> particle rows/motions -> preview/commit consistency mirror
    advection -> settle -> percolation -> bottom -> deep/correction/relax -> lift
      lift apply positive per-column/phase -> ordered seed j as above
    original audit / drift -> failure rollback or physics commit
    endpoint/peak/natural observation -> immutable publication
  rendering + capture/encoding/message/write/fsync/fence completion

optional isolated GPU pair per actual find:
  freeze -> CPU exact sort/range -> canonical pack/SHA/identity -> bridge
  upload -> build/query/reduce dispatch/barriers -> fence -> map
  identity/current-state verification -> CPU original merge/ledger
```

湿source r每次≤2C个positive seed；J_source≤2CR仅是上界；lift collect只登记request不seed，lift apply≤2C且**没有自身8次λ**。普通collect当前A/B两遍完整collect，seed头当前一遍完整collect，不能混计。authoritative与consistency mirror仍是真实工作，不能以accepted preview代替；只有§6证明相同read-set时可复用纯结果。

公共事务树用parentId区分source root、step root、preview、seed、row、birth、find、ticket、audit、copy、field channel、IO。frame是调度观测根，不把frame外source藏掉；一个source在queued/frame项已计，则direct项不再重复计。失败λ／discard preview／失败before commit的工作都在树中，不从成功计数倒推。

## 3. 完整基线界，补展开field及观测项

以下是源代码上界／条件界，不是设备实测。sort原实现heapsort有确定O(Llog(L+1))；内建sort／外部writer成本须按具体manifest登记，新路线改用稳定定容比较排序复现原tie次序，不让实现依赖的sort上界被写成O(1)。

| scope | 现行完整上界及易漏项 |
|---|---|
| 一次find | O(N+Llog(L+1))，每次当前scratch重建；满池birth支路M_find≤B_ent，显式MERGE另计 |
| 一次collect | scan/validate＋ID sort＋计数pass＋deposit/cross两phase pass＋plan写/比较：O(N+Llog(L+1)+Q+H)，不是单个predictor |
| 一次Hot audit | copy/capture、free/live/slot验证、ID sort、**slot递增**相质量fold、列／total／local预算检查：O(N+Llog(L+1)+C) |
| ID查槽/票据 | I次linear slot scan为O(IN)；reserve去重和token查槽为ΣO(T_current)，可O(Q²)，closed/refund也有真实成本 |
| 全池/场复制 | 每pool13 arrays＋标量，场/预算/观测前态等：O(P*(N+C))，rollback及discard也算，已在audit/collect内部计的copy不再加一遍 |
| wet水位 | getTotals O(C+N)，40轮×C目标水位二分，加donor×receiver循环O(C²)，故O(N+40C+C²)；dry确实分支跳过才为0 |
| source η | 构建Tarr为O(C)；objective最多64轮×C，加端点/终核O(C)；**不是**任意精度收敛或无限二分；source R重复计 |
| diffuse/advection/moisture | 读写若干C数组／边和固定phase，O(C)每次；启用/collect/apply各调用数单列，通量、矩和期末reconciliation均计 |
| settle/percolate | 主loop O(C)；若每col guard／recompute全S，保守O(C²)；当前只有loop末update的路径不多乘C |
| bottom/deep/relax | 固定phase/方向、配置pass数p_a、候选边≤C−1，单patch guard≤3边但完整S更新可O(C)：保守O((p_a+1)C²)；bottom split与反向实际值保持原图 |
| correction reserve/apply | reserve候选排序；`byRatio`内嵌rest在执行后立即break，**只触发一次**，自身为O(C)＋sort，不能凭嵌套语法判C²。apply/shadow每patch全S刷新才可有O(C²)保守项；不以单guard O(1)掩盖 |
| allocatePreviewPlans/consistency | shadow arrays copy O(C)，每channel实际顺序、shadowS重复更新／correction同上，可O((p_a+1)C²)每次；plan的delta arrays按实际channel数k_f×C计 |
| guard/angle/surface/drift | 局部guard边≤3，调用前internalParticleGuard全updateSandTop O(C)；全角合法检查／maxSlope O(C)，getDrift/getTotals可O(C+N) |
| endpoint/peak/natural观测 | 每column统计／ROI／界面角／peak/endpoint统计O(C)，真实pool summary/serialize可O(N+E+Lg)，含每次grant的slice/对象/数组；distinct-column/evidence构造次数乘实际scope，不算固定免费 |
| publication/render/IO | publication scalar epoch更新O(1)≠snapshot/packet copy；实际immutable pool/profile O(N+C+E)，shader像素Pxl及场景pass数/配置宽度、JSON字节Z、消息复制/write/fsync各计，writer等待无源码固定ms界 |

重要核证：当前`P011ParticleField.particleMassAt(c)`读取两列cache相加为**O(1)**，不是每列扫描N；因此WaterSandField逐列getWaterVolume/audit不凭此推成CN。pool.totals另有resting状态scan N，故getTotals是O(C+N)。上界必须绑定源码SHA，不沿猜想造新热点。

设a field子链启用pass上限p_a、k_f为配置冷冻结通道数。一个完整field链可用保守包络

`F(C,N) = O(N + 64C + (p_a+1)C² + k_f*C)`，

其中64是覆盖source η的最大固定轮次；step水位40单独计，source若没η不能再收一次。实际计量用每个具名函数次数，不把此统一包络同时加给子函数和其父链。自定义hook／新增未知writer不在manifest时拒新route配置，不能当O(1)。

整公共事务／帧必须通过**实际调用集合求和**：

```text
W_particle_base = Σfind (N+Lm log(Lm+1))
                + Σcollect (N+Lc log(Lc+1)+Qc+Hc)
                + Σaudit (N+La log(La+1)+C)
                + P_other*(N+C) + I*N + ΣticketOps T_current + B_ent+Q+H+E+Lg
W_public_base = W_particle_base + ΣfieldCalls F_call
              + ΣsourceEta (<=64C+O(C)) + guard_not_included + rollback_not_included
W_frame_total = ΣdirectSourceOutsideFrame W_public
              + Σsubsteps[W_queuedSource + ΣactualPreview W_candidate
                          + W_authoritative + W_consistency]
              + W_observation + W_publication + W_render + W_encodeIO
```

J/M/A/P等若已累计到frame，不再外乘R/V/S/2C。inclusive wall scope不能和子项exclusive累计二次相加；GPU fence CPU等待不是GPU timer，所有native/ETS/OS clock有各自domain。

## 4. 结构主轴一：持久派生pair索引，保留串行merge

### 4.1 关键引理：最小pair的a不需要每次枚举全部a

在同group中按(Y,ID,slot)排序。定义 `eligible(v)`：v与同group的紧邻前驱或后继至少一个满足原P(v,neighbor)。

**邻接见证引理**：v存在任一合法伙伴，当且仅当eligible(v)。因有限Y下原RN差的abs在Y两侧分别单调，若更远伙伴满足P，则中间更近的紧邻也满足。即使±0/同Y/溢出差，原比较仍按同一侧单调；padding不参与，radius有限非负沿原配置。group边界绝不能跨越。

该证明使用原binary64最近偶数环境下`abs(RN(a−b))=abs(RN(b−a))`的符号对称性（±0取abs后同0，溢出为两侧Infinity）；不能在另一个FTZ／非原舍入或F32环境直接沿用。

**最小端点引理**：全部eligible顶点中ID最小的a，就是全局字典序最小pair的第一端点。任何边的较小ID端点也是eligible；若a还有ID更小的合法伙伴，该伙伴将比a更早eligible，矛盾。因此找到a后，在完整合法Y区间排除自身作range-min-ID，就得到最小b，且b必>a。**不能只从a的邻接见证取b**；紧邻只证明存在，不一定是最小ID伙伴。

### 4.2 定容AVL／rank范围最小值

新增派生 `PAIR_ADJ_AVL_V1`：全active节点按完整 `(phase,sourceKind,sourceId,state,floorX,Y,id,slot)` 全序存定容AVL。每slot固定一个node，left/right/parent/height/size、prev/next、eligible、subtreeMinEligibleID/slot、subtreeMinID/slot及valid标志。ID=UINT32_MAX合法，0/最大数均不作真实最小值哨兵；查询用hasValue。重复active ID在独立ID索引／原审核cutpoint拒绝，不以slot掩盖。

一次cold build O(N+Llog(L+1))，邻接见证与augmentation线性构建。find先取root.minEligible确定a；用subtree size和select(rank)二分本group两侧的P区间，保守O(log²(N+1))（二分每次select O(logN)），再对[L,rank(a))与(rank(a),R)做O(logN) range-min，选最小b。边界继续用原`abs(RN(yA−yB))`，不用rounded y±r。合法区间存在性证书和最小值读取都绑定当前key revision。

mutation只更新真实改变的node：birth插入；retire删除；cross/state/source/phase/Y/floorX变化时先删旧key再插新key；merge执行**原F64图**后重读a全key，删b，必要时重定位a，不能假设加权x的floor必不变。纯mass/vx/vy改动不影响pair eligibility，但仍更新数值审核／predictor revision，不能让整个stage证书误复用。

工程write barrier须在物理key写入前按slot-node直接摘除旧node、保存旧邻接，再运行原F64 writer、最后按实际新key插回；不能先让node引用的Work.y变值而仍按旧树位置查询。一个logical patch内不开放find或外部callback；若原writer失败，按其原cutpoint恢复／毒化派生状态，不先抛一个新索引错误覆盖原码。

删除只改变其旧前后邻接，插入只改变新点和新前后邻接；最多固定几个eligible重算，每个augmentation向根修复O(logN)。AVL旋转不改变中序或原始Y值／ID，重算integer size/min仅对派生数据，无F64加法重结合。指针／height／valid错误是派生错误，必须在新route校验中发现，不能返回随便一个pair。

得到：

`W_pair_new = O(N+L0 log(L0+1) + U log(N+1) + M_find log²(N+1))`。

没有一次选多对：每次find→原mergePair→新质心／删除／birth→更新树→下一find，归纳每步pair与旧全扫描完全相同。失败无pair也O(log²N)，不是重排全池。索引损坏/不受控写失效时只允许从**当前正确snapshot**有界重建并显式计rebuild，或报原型错误；不能静默降级全扫描后仍记新复杂度。bulk motion改很多key可一次build O(NlogN)，但仍在下一find之前完成；其次数单列H_bulk，不每merge误触发。

可供work计量使用的宽松结构计数界：AVL高度取`hA=ceil(2*log2(N+1))`，rank二分轮数`hR=ceil(log2(N+1))`；一次find预留≤`6*hR*hA+4*hA+固定头开销`个树节点访问（group界／Y分界／两侧P二分／range-min），一次原子key插删按`64*hA+固定头开销`预留，重定位按删＋插计。此为结构操作包络，旋转/比较/encode的真实指令与CPU时间需另校准，不是设备耗时承诺；具体AVL实现若超过此包络，先证明新界／修manifest，不悄悄放宽运行统计。

### 4.3 失效、rollback、ID与ticket查找

索引只属于一个owner/viewBranch（public root、λ、preview、authoritative）、poolContentRev、params.mergeRadiusRev；反复用同mutation号但内容不同无效。所有写入口必须声明write-set。rollback将branch/view换回对应持久根或从恢复数组重建一次，旧根不能贴新revision继续用；generation计数不回绕，耗尽fail closed。跨参数/route/restore/newrun失效；GPU结果也不得消费旧节点身份。

连续seed是同一个已准入public root里的**逻辑savepoint**，必须继承当前已正确的派生索引，不在每个seed的beginStep重新cold-build。可用持久节点COW，或对AVL指针/height/min/eligible及root做同样的嵌套undo；子seed成功后父view接管其新树，失败恢复该seed入口树，外层λ失败再退到λ入口。frozen collect只读原frozen位型，不允许因共享树而读到后续Work key。若工程暂只能每seed重建，明确`H_bulk=J+...`代入上界，不能报告本版预期的少量branch-build收益；整套结构未到位前不称复杂度迁移完成。

ID→slot采用第二定容AVL（整数ID全序）或经证明的现ID有序活动链加查询树；不用大小2^32的稀疏数组／概率hash。lookup O(logN)，motion/非birth事件总O(IlogN)，并核slotGeneration、live和ID一致；旧ID退役后同slot再出生不能命中。collect按同ID顺序遍历树，无每次heapsort，仍保留原slot-order验证pass与duplicate错误结果。

票据采用两个定容整数树 `(ordinal→row)`、`(opaqueToken→row)`，保持原allocateParticleHotLease的值／次数及safe整数耗尽规则；reserve仍逐原行执行同F64 debit/scale，duplicate/conflict/closed/epoch错误相同。O(log(Q+1))查询／插入代替O(T)扫，close不删除历史duplicate约束，reset/rollback按branch失效；不把token重编码成索引改变wire身份。

## 5. 结构主轴二：共享不可变前态、原写顺序、撤销日志

公共事务owner保留canonical，候选／seed/row在有界branch view上执行。先建立一份不可变public入口；原source成功是独立public commit，后续stepBefore必须在其后。原source λ与step preview各有逻辑savepoint，原有行/seed审核和事件page边界不合并。

保留F64数组语义的两种可实施后端：

- **首阶段定容整bank**：每公共attempt/λ/candidate按原需要复制，原row/sink对同owner私有Work使用受控视图，省13buffer来回搬运；alias/长度／buffer绑定检查仍在原cutpoint执行。raw typed array逃逸或未知外部写存在时，不允许此优化，原copy路径继续计费。
- **后续copy-on-write page/undo**：只有能证明地址域和bit模式不变的字段走write barrier，记录原32/64位字、logical frame first-touch；读取仍是原值/原运算次序。seed内每次merge/birth不做整池copy。row失败按原rollback域恢复；nested parent/λ/source/step savepoint必须各自保留first-old-value，不能只保最外层undo而无法复原内层。日志容量预留，写前检查，无热扩容。

日志最坏不是O(N)：同一个slot跨B次birth/merge可以反复写。用w作为**实际原字写次数**，时间O(w)、nested undo存储≤d*w记录（d冷冻结最大savepoint深度），加完整数组快照O(N+C)。COW每页复制一次只在相同逻辑branch内成立；新λ/preview要新版本，不能拿一次COW摊掉全部R/V。事件/lineage/票据/计数器/source状态也进入对应write-set，不能只回滚pool。

原seed从begin→collect校验→row→tail→finish→page导出的顺序保持；允许使用同owner共享底层库，不把多个seed压成一个budget denominator／一个audit。`beginDryBatch`现有双phase批次已经是一种独立语义，只在原路径复现，不能自动扩成wet lift按列全批。原birth `floor(mass/pkg)`、tail、random/nextId/freeSlots及每一次find仍原顺序。

## 6. 结构主轴三：审核和collect证书，不跳audit

证书必须保存**read-set、原运算图版本、结果／首失败位置、副作用清单、已覆盖buffer所有权**。任何依赖位型或结构generation改变立即失效；只比较一个mutation整数、短hash或“质量总量没变”不够。

| 可复用对象 | 明确依赖／动作 | 不允许的捷径 |
|---|---|---|
| field CanonicalS／dry waterTop | bh/bl/mh/ml及dry需U/P cache，dirty column按原左结合式算 | 用S+=delta或将T196的IDfold搬进旧audit |
| pair eligibility/ID uniqueness/free membership | 受控writer＋AVL/slot generation，delete/insert更新邻居 | key未变就让mass/predictor证书一起有效 |
| 局部finite/域检查 | 受影响slot／列、原predicate参数，按原序报告失败 | 不知write-set仍沿用旧PASS |
| numeric audit | 精确覆盖field/pool cache/budget/before/external等read-set，完全未变才可重用 | F64总和“可结合”假设、仅因最终总量相近跳中途审核 |
| frozen predictor | frozen slot/field S/waterTop、mode/params/dt/θ、原seed及算法图；构建A/B只读输出 | 用预览角的轨迹替代authoritative角或忽略原溢出/容量失败 |
| headers-only collect | 原collect所有验证、计数、total finite等仍按原次序；只省随后被完全覆盖的无观察plan数据写入 | 因为motionCount会清0就不验证旧粒子/predictor |
| planA/B纯复制 | 同完整immutable frozen输入、确定纯collector且storage/identity校验都执行，复制相同结果到各自bank | 修改A以后让B指向A形成alias、任一cutpoint丢错误身份 |

### 6.1 明确保留的数值成本

旧Hot audit从slot0..N−1累加每列phase质量，随后从column0..C−1折叠before/after并执行原预算audit。**这与T196按ID折叠的A路线不同**。没有本篇外另给出的浮点函数组合证明，不能用树形sum、exact accumulator后一次RN或总量+=delta替换。

因此dirty audit仍执行该原有ordered fold，代价O(N+C)；结构树可免每次duplicate-ID sort，但不是免数值检查。SINK_ROW后ticket.close可能改变audit不读取的closed标志，也可能有读到的budget值变化：只有逐predicate read-set一致时，APPLY_TAIL/FINISH才命中证书。若motion/field/budget/stepExternal任一改变，必须再fold。保留A_logical切点，日志记`AUDIT_EXECUTE`或`AUDIT_CERT_HIT`且两者之和=原逻辑调用数；新路由最坏A_num=A_logical，**不承诺每public只audit一次**。

### 6.2 数值证明边界

结构证书按归纳：从原入口已执行的验证及受控状态建立证书；每受控writer核其修改集合并更新相应结构树／dirty集合；无变化的域沿已验证值；原数值fold遇dirty完整重跑。因此每原cutpoint的PASS/原错误code、stage、ordinal及rollback选择相同。**不得在public入口额外先跑catch-all audit并立即返回13**，从而抢在原INVALID_BUFFER／collect失败之前改变首码。发现无法建合法树的原输入，只标UNCERTIFIED，在原对应cutpoint执行原检查／原查找并返回原结果。重复ID时原particleSlotHot取最先physical slot而find/后续audit拒绝的先后关系同样保留，不用ID树提前去重报错。

未知writer、外部可写alias、异常callback、NaN中间图不支持时，禁复用相应证书，执行原验证，不能凭本方案抹掉失败。若为保错语义走原执行fallback，其全部legacy工作必须单列并纳入W_upper；新树界只对经支持的合法受控域主张，不能把fallback代价藏掉或在性能报告中伪称全域新复杂度。生产配置不支持某writer时应显式配置拒绝／保留有名legacy分支，不能省做该通道。

内部原生copy/排序调用次数会改变，旧耗时／诊断比较次数显然不会逐字相同；统一新性能schema，保留旧业务首码与逻辑event顺序，不能把计数变化包装成旧run。任何旧契约将诊断heap比较次数作为业务断言的场景都须显式兼容层／新版本，不偷删测试。

## 7. field、preview、η与观测的结构安排

1. **dirty-column更新**：在全部write入口齐全时，guard前只计算受影响S，重算式原样；其它未脏列值与原全遍重算相同。dry waterTop依赖pool col cache，cross/birth/LAND同样置相关列dirty。whole-field读／audit在读之前flush所有dirty；flush顺序按原column顺序，保留原局部物理运算先后，不用ΔS增量。
2. **shadow共享纯核**：preview allocator、consistency与authoritative使用同一**逻辑原顺序**的纯通道算子及不同branch view；不因为同函数就直接采用preview库存，保留原consistency比较。能复用的是同read-set派生量，不是合并preview/authoritative的物理提交。REQUESTED/CLAMPED/OLD不同θ一律重新生成其依赖结果；collect只记录riseRequest的路径不额外算seed。
3. **field循环仍有上界**：dirty更新去掉每grant完整C重算后，大部分通道变为O(C+numberOfPatches)；但water donor×receiver仍保留O(C²)、p_a次relax仍计。correction byRatio/rest只执行一次内扫，保留O(C)+sort而非误记新的平方热点；若其apply仍走whole-S更新，再按真实更新次数加C。η objective保持原二分轮次、端点检查和RN列累加，不改成解析水位或用F32 GPU近似。未来替换为排序水线/receiver集合必须另证F64和错误边界；本版不虚报已成为ClogC。
4. **稳定排序**：field候选内建sort要迁移时用固定scratch的stable mergesort，调用原纯比较器、把比较为NaN/±0按原tie等价处理，稳定原index破同序；字段算术不变。新性能scope记录比较数。未证明比较器纯／有原外部副作用，则原sort执行并保留其测量，不能换后声称全域等价。
5. **观察／发布**：endpoint/peak/natural同原触发点读已接受或原失败attempt状态，source/commit/page键原样。物化／编码移到读immutable sealed capture之后；所有原字段都有copy/lease预算，无screen Canvas路径。不能合并掉被验收要求保留的substep记录。改成异步消息或截取“最新帧”是owner版本行为，不能在精确物理层冒相同同步API。

这是一条统一read-set/write-set/事务框架，非分别打几个不相干的热点补丁。保留的C²与ordered-fold是显式预算项，并由owner全链准入控制最坏服务工作，而不是靠平均FPS遮掩。

## 8. 改造后完整上界与迁移可行边界

记H_bulk为必须whole-build索引的独立branch入口／bulk mutation数量，A_num实际原顺序fold数，H_hdr为headers-only验证pass数，P_bank为剩余whole-bank copy，w为undo字写数；所有均按实际调用树统计。

```text
W_new_particle <= H_bulk * O(N+L log(L+1))
                + U * O(log(N+1)) + M_find * O(log²(N+1))
                + I * O(log(N+1)) + ticketOps * O(log(Qcap+1))
                + Σcollect_numeric O(N+Q+H) + H_hdr*O(N)
                + A_num*O(N+C)
                + P_bank*O(N+C) + O(w+B_ent+Q+H+E+Lg)
W_new_field <= ΣfieldCalls O(N+64C+(p_a+1)C²+k_f*C)
W_new_public/frame = SAME executed-source/λ/preview/authoritative call-set sum
                    + observation/publication/IO/render/GPU optional costs
```

不把H_hdr与其内部collect重复收取，cache hit也计validation固定成本。最坏所有证书miss，J/λ/preview乘数仍在，**未宣称整个frame线性或已消灭所有O(JN)**。本结构实际收益是：重复merge不再每次NlogN rebuild；ID/token查找不再N/Q线性；可证明read-set未变时免重复full sort/copy/fold；field不因局部patch反复全场刷新；调度在完整包络内。最大主工作量剩余项可能变为A_num*(N+C)或field C²，方案不会把它们藏起来。

baseline与new必须在相同N/L/B/M/R/V/S、同状态／失败输入上比较。T116同M分母：旧扫描O(MNL)，T123 O(M(N+LlogL))，新树O(H_bulk(N+LlogL)+UlogN+Mlog²N)。早命中旧scan可能O(MN)，常数和输入敏感差异不由上界决定。本次设备实际N/M未知，禁止拿合成M64/128外推复发事务。

## 9. 工作量预留、背压与完整owner调度（显式行为版本）

### 9.1 三类预算不能混淆

物理预算保持既有300/s/28/s及bottom等；工作量预算W计计算原子块／复制字节／验证／GPU请求，不改变物理grant；IO/内存额度另计。W不能通过“剩多少就做多少birth／物理dt”执行。source和step各是独立原子public root，不能用一个frame预算撤销已成功source。

新增producer-facing状态 `PENDING / WOULD_BLOCK / REJECT_WORK_ENVELOPE / COMPLETED(originalResult)` 需要 `TX_OWNER_ADMISSION_V1` 裁决。不是给旧WaterSandError编号塞一个“成功0量”，也不是把资源拒绝当原物理失败。源成功epoch/ledger仅在真实执行并通过原检查后推进；入队ACK、排队占额、失败attempt不得冒源成功。

预检以freeze输入＋cold配置计算**整根最坏**工作/undo/event/encoded-output上界；包含source R≤8、每次2C seeds、B_req及可能M≤B_ent、每seed原cutpoint、V≤3/S≤2及fieldpass/η。B_req按原F64 floor/tail图做一次受控checked计算，不先进入birth循环才发现量巨大。derive未支持／safe整数溢出／无有限上界时在材料修改前拒准入。原event容量逐seed重置不推出整public E≤4096，需跨seed/λ累加；容量表见§10。

先预留完整W_upper、undo、事件和IO；预留失败则WOULD_BLOCK/REJECT，保持请求身份、未成功、可显式重试。执行过程中每个有上界的原子块（AVL操作、slot/column fold、64字节SHA块、固定copy chunk、GL wait slice等）检查实际预算**覆盖**；超出预留意味着BOUND_VIOLATION，完整回滚当前public root并保留错误原值，不能当普通局部clipping继续。外部真实异常按原错误cutpoint优先处理，不能让性能计量抹首码。

设计上可在私有branch里**协作让出CPU**，但只能在持有完整预留/owner、无可见半提交时；不消费更新一半的输入，不改逻辑θ/dt/队列顺序，不能把半步publication发下一帧或把让出当少算。保守首版不跨UI frame驱动同一物理事务，直接由独立owner完整跑至commit/rollback；调度preemption本身不变成物理分片。

### 9.2 单写者与队列界

推荐现有数值运行时的独立物理owner（具体Worker/NAPI平台可用性由architect探针确认），UI只发送有界immutable命令；native整事务候选另做libm/Math差分，不能仅`-ffp-contract=off`就声称sin/exp与ArkTS逐位相同。原代码路径的准确数学先留同运行时；GPU context owner另隔离或明确独占，不能跨线程借renderer context。

示例冷资源profile：输入队列Kq=16、每命令头≤256B＋最多2C个F64（C128时≤2304B），队列payload≤36,864B；不接受无限变长trace/字符串，identity长度冷限制。profile是提案非既有已批准上限。停源/reset/config等控制命令同FIFO，不抢占半事务；紧急取消只在原子根边界生效，当前未commit的根可按显式取消合同完整回滚，不能撤前一成功根。

UI时间参数在接收时冻结：按原规则只裁一次dt≤1/30，保留样本顺序、source clock/targetTilt/shake输入身份；不能owner排队后用执行墙钟重新算dt、按最新角覆盖历史角、把多个source量相加或只保最新command来“提速”。若产品另选sample coalescing／丢旧tick，必须新语义裁决及计量，不属于精确层。

owner同时仅执行一个public root；root之间发布不可变commit序列及回执。双publication槽/双IO封存槽均有lease/代身份，UI落后或writer占用时背压，不覆盖仍被读的槽。渲染可以继续显示标明STALE的上一有效新shader帧，但不能把过期帧标当前成功，也不能退回Canvas。enqueue返回有限固定开销，不允许内部等待service完成把卡死搬回UI。

命令头至少含`runGeneration/enqueueSerial/kind(SOURCE,STEP,INPUT_UPDATE,RESET,CAPTURE_FINISH)/producerTime/acceptedDt/inputGeneration/configRev/payloadLength`，producer先占槽才复制bounded payload，owner验证FIFO身份。capture启用／计量schema握手必须在首step前完成；CAPTURE_FINISH在同FIFO排到位置后，只对该run已接受末key发唯一finish/fence，不借历史First1日志返回新run CLOSED。finish之后SOURCE明确CAPTURE_CLOSED/未执行或按新capture会话协议处理，坐标gate接受／入队ACK不能写成成功准入。这把T116身份故障也纳入整体owner/IO边界，但不代替T204旧资料勘误。

原每root输出先按schema保守估计编码bytes并占Zio信用：数字/字段数、有限F64的最大编码长度、受限身份字符串的UTF8最坏转义和所有E/Lg字段都计，不以平均行长预留。完整输出超过空闲credit就不启动该root，不截字段。独立writer按序消费sealed immutable block并归还credit；write/fsync失败明确CAPTURE_INCOMPLETE/首原码、不能签COMPLETE。若故障发生在物理commit之后，保留物理成功及source ledger，不能把归档失败反写成已成功源被撤销。

### 9.3 排队时延与CPU/GPU响应分列

以Wop为确定操作计数、ρ为待设备校准的服务率，不称Wop/ρ是硬实时保证。若每root实测/配置的服务上界为Tsvc且调度延迟≤Tsched，则FIFO队列m个在前的条件等待界≤m*Tsvc+Tsched，完成界≤(m+1)*Tsvc+Tsched；m最多Kq（含正在执行者的定义必须固定）。OS可无限延迟、磁盘或驱动可timeout时，没有无条件墙钟有限界；必须有显式deadline／timeout结果、有限队列和不会被UI同步等待的协议。

工作抵达率λwork高于服务率μwork持续时，稳定无丢排队不可能；必须真实WOULD_BLOCK/拒绝并记录。接受率／拒绝率／source实际准入体积与相同offered输入一起报告，不能降输入后把完成样本p95变好称算力收益。已接受请求不可“过期后默默丢掉”；deadline到若尚未启动需明确CANCELLED_BEFORE_START，若根已执行则按完整原子取消合同，不发半成功。

wall watchdog timeout/GPU fence timeout是可观察新失败行为，需owner版本记录；其物理回滚域／首故障GL码／资源quarantine保持真实。W保证无无限算法循环，不保证driver响应；GPU结果迟到必须身份拒绝、失败资源不能立即复用。

## 10. 内存、负载包络与设备校准

基线主档C128/N192与512，Hot Qcap2048、Ecap4096、Lgcap8192、Hcap=N、R≤8、V≤3/S≤2均按配置/源码身份核；这些只是**单Hot work/每scope容量**。不能说整个source或整帧只有4096事件。

新route需要cold `TxEnvelopeProfile{C,N,Qcap,E_hot,Lg_hot,Rmax,Vmax,Smax,BpubMax,JpubMax,EpubMax,LgpubMax,WpubMax,undoWordMax,depthMax,Kq,ZioMax,leaseSlots,stepDeadline,clockRev}`。数值关系必须相容：J_source≤2C*R、M_birth≤B_ent、U≤a*(B_ent+M_commit+H+Q)，a为已证明每动作固定key变更数；所有aggregate计数checked safe integer。**WpubMax／deadline／服务率／可接受峰值常数 PENDING_DEVICE_CALIBRATION**，缺实际毫秒不阻设计，但未校准不准宣产品性能或私填量级。

对于B_req超profile／越日志容量的原请求，owner准入新版本明确拒绝，保留offered输入分母；不改成更小birth执行。即便生产Wpub上限选得很保守，也不能把“拒了大部分负载”当原算法变快。精确层的correctness fixture必须以资源足额配置覆盖完整原路径。

建议新增派生结构的payload上界（不包含原Hot几十数组与所有branch，它们由allocation ledger从实际字段累计）：

| 项 | 定容提案／计量 |
|---|---|
| pair AVL与ID AVL | 每节点预留16个32位索引/位域＋4个F64/uint键摘要位槽≤96N B；键原值读取对应immutable/Work slot，不量化Y/source；两工作view按branch数乘，不拿一树跨branch假共享 |
| ticket ordinal/token双树 | 每ticket预留10个32位结构＋原token双精度读引用，新增≤48Qcap B/view；原ticket数组/请求/receipt另计 |
| dirty／certificate arrays | 上界32N＋64C＋64k_stage B/view，保存完整所需rev，不用短hash代替内容身份 |
| undo word log | 每条addressIndex/old64/frameId/reserved预留24B，最多undoWordMax；savepoint栈depthMax×64B；当多个层各需old值时24*d*w上界，不虚称一次写一条够所有层 |
| input queue | Kq*(256+16C) B，提案16/C128=36864 B |
| event/lineage封存 | EpubMax*原事件逐槽bytes＋LgpubMax*实际元素bytes，双buffer／owner/NAPI/IO副本按真实数量乘；原seed page可流入根私有arena但未commit不能对外当成功 |
| publication/IO | 固定bank/leases包括pool13buffer＋field全部state、source/预算/metadata、ZioMax；cold snapshot与上下文恢复重叠另外累计 |

例如仅一view的新增pair树N192/512为18,432/49,152B，双ticket树Q2048为98,304B，**不是全程序内存**；W/undo/Epub容量未选定时不报一个虚假的总MB。architect生成实际全数组分配ledger，含旧canonical/frozen/scratch/step/source/preview banks、Map对象替代、endpoint/peak、GPU staging／SHA输入、renderer/FBO/旧帧重叠以及snapshot持有者。任何OOM／定容超限在修改前预留失败或整根rollback，不热扩容／偷驱逐幂等或归档。

时间目标分三个口径：①输入enqueue固定开销／UI响应预算沿现有硬门，待设备实测；②相同offered/accepted输入上的public root/整frame service p50/p95/最坏，比较新旧同分母；③命令接收→物理commit→UI显示→归档fence的端到端，queue/setup/encode/fsync分列。不能以host模型运行秒数、O2二进制或GPU request变快直接签任一目标已达标。

## 11. GPU、SHA与计算资源的明确边界

T137只读整数pair算法与T191/O2隔离对照继续；新CPU邻接索引是完整方案的确定性控制/对照，不自动取代GPU目标或宣GPU无价值。原T137模式每merge当前scratch重新CPU sort/range/hash、h层build/query/reduction、一次尾fence；不能把本篇AVL更新当GPU旧ABI也自动增量了。

GPU分支工作项必须完整追加：CPU冻13buffer/identity验证、sort/range、canonical LE pack与**真实SHA256块数**、context字符串／bridge、upload bytes、dispatch/barrier、fence CPU等待／map、消费校验、CPU原merge与ledger、reply与外层archive。T183已给O0 prepare/hash占比的限定证据，O2收益待对应新包实测，不从旧分项相减构造反事实收益。

不可跨mutation复用池SHA或GPU range，仅因generation相同不够；身份header里不可变配置SHA片段可configure期缓存，但payload变化必须重新hash，保持原SHA字节／padding／wire。FNV/短摘要不取代全内容比较。若将来做GPU增量索引／GPU驻留权威事务，需要另版可见性、串行依赖、rollback与完整收益证明；本篇不默授权。

## 12. 全链计数字典、恒等式与独立规格（交T198）

新计量 `P011_TX_WORK_TRACE_V1`：每scope `ENTER/LEAVE/FAIL/CANCEL`唯一id、parentId、run/attempt/λ/candidateRole/substep/origin(DIRECT/QUEUE/SHAKE)、before/after视图revision、logical/error/physical原码、配置flags、offered/accepted体积和时间。计数安全整数，溢出或ring截断标incomplete，不以0代未知。FIRST_LONG是部分进度快照，不能当事务入口回放原件。

必须具名：SOURCE_ENTER/R_LAMBDA/J_SEED；B_REQ/B_ENTER/B_COMMIT；M_FIND_ENTER/NO_PAIR/MERGE_COMMIT；PAIR_BUILD/KEY_INSERT/ERASE/RELOCATE/ROTATE/NEIGHBOR_TEST/RANK_SELECT/RANGE_NODE；COLLECT_A/B/HEADER_VALIDATE/PLAN_BYTES；AUDIT_LOGICAL/EXECUTE/CERT_HIT/FOLD_SLOTS/FOLD_COLUMNS；ID_LOOKUP_NODES/TICKET_SEARCH_NODES；COPY_POOL_BYTES/COPY_FIELD_BYTES/UNDO_WRITES/UNDO_RESTORES；每field函数及eta/water迭代／donorReceiver／guard／dirtyFlush；preview/authoritative/consistency；事件/lineage字数及stage；publication/encode/message/write/fsync bytes；GPU prepare sort/range/SHA_BLOCKS/pack/upload/dispatch/fence/map/consume及ownerqueue时延。

恒等式／边界必须按每个scope检验：

```text
enteredScope = completedScope + failedScope + cancelledScope + stillActiveScope
B_ent <= B_req ; B_commit <= B_ent             // scope匹配，discard不算commit
M_find_birth <= B_ent                         // 显式MERGE不能混入
M_find = found + noPair + error
AUDIT_LOGICAL = AUDIT_EXECUTE + AUDIT_CERT_HIT // 不存在无证skip
sourceSuccess = actualAcceptedSourceCommitCount, never enqueue ACK
physicalCommitCount = accepted authoritative roots/substeps only
copyBytes(root) = sum exclusive child copyBytes
fieldVisits = sum actual channel invocations*their own visits
ΣframeWork = direct-source scope sum + queued-source scopes + step stages
             + observation/publication/render/IO exclusive totals
```

上述M_find分类恒等式用于**已闭合scope**；FIRST_LONG／运行中快照另加`findStillActive/findCancelled`，不得把尚未返回query硬塞noPair/error或填0。partial trace缺尾部用INCOMPLETE，不声称完整恒等式已检验。

耗时inclusive parent与exclusive child并列，不能直接将CPU/GPU异时钟相减；sum child work计数与wall-clock critical path不是同一量。失败／超时／拒绝／预热outlier全保留。诊断OFF也需低扰动固定计数用于包络；详细trace可关闭但其开关本身绑定schema，不能无日志就填0或拿不存在的array推设备PASS。

独立规格至少包含：

| ID | oracle／反例及上界 |
|---|---|
| TX-01 | C/N/L/B独立变化、空槽F、B>N、full no-pair/early/late配对；同M分母旧scan/T123/new树逐pair、F64/ID/free/ledger完全一致 |
| TX-02 | 邻接只证明eligible，b可能为非相邻最小ID；signed-zero/subnormal/rounded y±r边界、MAX_ID、跨group；质心改变／group floor变化后重定位，退役复用slot拒旧ID |
| TX-03 | 连续birth/merge的每个原audit、budget denominator/refund、first code/stage/ordinal、失败λ恢复与成功source前缀保留；不能只比final pool SHA |
| TX-04 | 所有read-set字段各翻一位／同rev异内容／alias或pointer改变／budget.close／motion后缓存miss正确，cert hit和原重算结果一致 |
| TX-05 | slot-order与tree/ID-order求和反例；dirty numeric audit仍原fold，证书不允许结合律／近似epsilon更改 |
| TX-06 | SOURCE R1..8、lift collect无seed、apply多C/phase seed、V0..3/S1..2和authoritative/consistency、frame外DIRECT入口；计数不二次乘、不遗漏失败 |
| TX-07 | 全field通道／water40／eta64／correction C²/dirtyS/观测/发布负载，证明pair热点降低后未把未计链藏掉 |
| TX-08 | undo nested同地址多写、源成功后stepfail、plan页/事件/lineage/票据溢出与partially-applied局部错误，回原逻辑savepoint逐字节一致 |
| TX-09 | 工作量/内存/IO不足在root修改前背压；已开始bound violation整root rollback；offered与accepted分母、原始请求重试身份不伪成功 |
| TX-10 | owner FIFO/停止/reset/cancel/deadline/队列满／publication lease/IO慢/迟到GPU；不丢已接受source、不半提交跨帧、UI旧帧显式STALE |
| TX-11 | GPU prepare/SHA/bridge/fence/consume与archive全项，O0/O2/CPU/GPU同fixture同包身份，旧证据范围不扩大 |
| TX-12 | capacity N192/512及C/B/Q/E independently sweep，全root/frame峰值内存／work上界，source拒绝率与UI/物理/归档时延分列，不只单merge曲线 |

原失败首码/现场N/B/M等缺证由后续精确设备卡按T116核证路径采，不阻本设计。T198是独立规格／oracle，不能复制本设计模型当独立实现。未实施预测试RED登记NOT_IMPLEMENTED结束归因，实际失败先architect分流，不删旧硬门。

## 13. 候选、迁移及architect任务书

| 候选 | 裁定 |
|---|---|
| 只调heapsort/O2/换native | 不作为整体答案；O2留隔离实验，线程改善UI单列，均不消乘数 |
| 同时预选多pair／跨列批量seed一次audit | REJECTED为等价优化：质心/来源/预算/错误先后不同；若新物理版本另行裁决 |
| 本篇派生结构＋原数值cutpoint＋完整owner/work包络 | RECOMMENDED；等价层与资源/异步新行为分版，保留残余成本，具可测完整上界 |
| 全GPU权威事务／F32 / 去SHA或audit | 本卡不选；改变数值／信任域或事务需另证，不能据微核吞吐或O2完成自动迁移 |

迁移按**同一整体方案的阶段**实施：

1. architect先修订完整工程图/manifest（消费T116 V3、本篇和T198规格），冻结所有原logical cutpoint、read/write-set、offered/accepted/API版本、资源全表与branch深度；不能直接派某栈热点返修。owner/admission显式裁决与结构纯等价任务分别登记，不新增manager验收门。
2. 隔离实现动态邻接pair AVL＋ID/ticket整数树，独立逐次差分／mutation与rollback oracle；实现同owner view/whole-bank，先保留原全部numeric fold和原错误域，不先删验证。此阶段已可测整链W，不宣最终UI目标达成。
3. 证书和dirty-column逐predicate接入，命中／失效透明可查；headers-only保留所有原验证／finite total；按实际write-set选择COW/undo，无法证明原错误次序的路径保留原完整执行并计费。trace规格与完整frame oracle同版。
4. `TX_OWNER_ADMISSION_V1`经显式行为裁决后接完整物理owner和bounded input/publication/IO队列，冻dt/输入顺序、不半提交。目标设备校准W/service/deadline，独立测试正确性/背压，再设备完整链稳定／收益。GPU O2对照并行，不把它当本结构阶段替身。
5. 所有原source/step/通道矩阵实际支持并有同状态差分后，才由后续工程确定生产开关和回退合同。未支持writer保留明确legacy whole-execution成本或UNSUPPORTED，不混报“全链已迁移”。

所有权建议新增独立TX owner/core/index/cert/trace模块及隔离harness，必要Hot/M2/WaterSandField接点由architect按hunk列出，避免GPU/渲染在途共享修改被回退；编译／离线自测为开发卡交付，独立tester／设备effector另卡。产物包括具体opaque-handle/branch/cache接口、完整allocation和work-envelope manifest、错误与capture状态、同source/step输入输出fixture及故障注入边界；不在designer卡实施业务。

本篇已交整体方案、局部确定性证明、公共事务/帧聚合界、原子预算和迁移合同。参数校准／owner新行为裁决／实施／设备证据是后续真实工作，不伪装已完成。本轮模型与指纹见 [设计算术证据](design-T197-whole-transaction-evidence-v1.json)，不是产品/设备测试。
