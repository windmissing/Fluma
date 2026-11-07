# T137 GPU确定配对：整数分段索引、最小pair与串行事务 v1

2026-10-26，designer；原卡 `6adf6602e4b03273ad88f511`。路线 `P011_GPU_PAIR_PROTOTYPE_V1`，算法 `GPU_SEGMENTED_ID_PAIR_V1`，CPU边界 `CPU_RN64_MERGE_RANGE_V1`。**DESIGN_READY_FOR_ARCHITECT：允许据此落测试专用原型工程，不是生产迁移、设备正确性或收益通过。** GPU为当前最高调度优先，T123 CPU精确索引并行保留。本轮不使用真机、不改业务／测试阈值、不回退共享修改。

渲染仅新XComponent+shader，含背景前景，禁止Canvas上屏及失败回退；失败FAULT或显式STALE上一有效新管线帧。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己状态有限例外；不改其它行列／队列／滴答同名清单。失败先architect分流，manager登记不验收。

## 1. 消费的事实与不变量

以T130 §3方案、T116精确谓词和当前Hot实现为输入；指纹与算术／离散模型核对见 `design-T137-gpu-pair-evidence-v1.json`。T131原始probe是整数map，不含本算法的多层排序、查询、reduction；其报告的同步可承受／逐merge不可行概括不作为定理。收尾已读 `architecture-T136-compute-arrival-decision-v1.md`：设备基础能力正向观察成立，修正harness同步／错误消费／冷暖与样本归档合同后进入有效原型取证，生产迁移／收益仍UNPROVEN。本方案与该裁定一致，不重新索取已交GL版本或map能力。

原选择目标：active、`idA<idB`，完整group `(phase,sourceKind,sourceId,state,floor(x))` 相等，并且

`P(a,b) := Math.abs(RN64(y_a−y_b)) <= mergeHeightRadius`。

在全部合法pair中选字典序最小 `(idA,idB)`；无pair返回与旧查找相同的失败状态。不能只找“最接近”或“任一合法pair”。选中后只由CPU调用原mergePairHot，沿原F64加权顺序、存活aID、释放bslot、事件／lineage／free-list／预算／失败rollback语义执行。新查找不修正旧物理状态或调整半径。

支持域：当前scratch经过原有限、质量／状态／位置等校验；active ID是互异整数1..UINT32_MAX（0仍是非active／empty编码）；半径有限且非负，Y有限，原CPU binary64 RN和次正规语义保持。active重复ID即使在不同group也在CPU预检拒绝，不能靠slot消歧悄悄改变原ID身份。inactive旧ID不作为活动记录，但不放宽原snapshot自身的校验。非法输入输出INPUT_INVALID，不当成NO_PAIR。

## 2. CPU精确分组／Y边界：不上传F32高度

记容量Nmax、active数n。定容数组扫描active槽，按原group逐字段精确比较、Y数值、ID、slot形成全序，得到records[0..n)。±0按数值相等，由ID/slot破同高次序；NaN拒绝。sourceId等group字段保持其CPU原类型／值，不压缩成碰撞hash或32位截断。全group连续，记录group的半开边界[G0,G1)，全局Y秩是records的位置。

每个a先在其group二分 `mid = upper_bound(Y <= y_a)`，再以**原表达式P(a,b)**作两段二分：

```text
L = first true P in [G0,mid)          // false...true；不存在取mid
R = first false P in [mid,G1)         // true...false；不存在取G1
range = [L,R)                         // 半开，包含a本身
```

左段y_b≤y_a，精确差非负且随Y增大不增，RN是单调函数，abs不改非负顺序，因此P后缀为真；右段精确差非正且绝对差随Y增大不减，RN及abs保持该顺序，因此P前缀为真。有限输入减法可溢出成±Infinity，此时原P为false，仍保持单调；本CPU边界算法不因该差溢出把有限原输入偷偷量化。自点差为0且radius≥0，故两真段连续。二分O(log n)且使用原CPU求差，不以实数距离替换。

**不能用rounded(y_a±radius)作边界。** 最小反例：a=.2，b=−最小正次正规数，radius=.2，RN(a−b)=.2原P为真，但RN(a−radius)=0使b被简单下界遗漏。这正是CPU必须保留原F64谓词的原因。环境FTZ或跨语言运算不一致时先报环境不支持／差分失败，不用epsilon掩盖。

CPU上传：id/slot/valid/originalYRank及每a的L/R/G0/G1，全部uint32；不上传高度、半径、质量、速度的Float32近似。group相等与范围正确性由CPU生成器合同证明，不能只验某返回pair合法来替代范围完整性。CPU排序、ID唯一检查、边界生成每次O(Nmax+n log n)，全部计入GPU路线总时间。

## 3. 全局分段布局与padding

为使主规模与active稀疏对照可比，本版冷配置 `K=pow2ceil(max(1,Nmax))`，h=log2K，不随每次n偷偷缩K；n=0/1可CPU直接返回NO_PAIR并标CPU_EMPTY_SHORTCUT、GPU调用0。Nmax主档192/512对应K=256/512；大档仅诊断，创建前检查容量、索引乘法及SSBO限制。不是pow2ceil(n)版本的混合测量。

主表T共(h+1)层，每层K条，每条16B，std430以uvec4存 `{id,slot,valid,originalYRank}`。层l、全局segment `s` 的长度m=2^l，表偏移：

`base(l,s)=l*K+s*m`，覆盖原始Y秩 `[s*m,(s+1)*m)`。

level0前n记录为真实记录；余下pad[i]为valid=0，id/slot可0，但originalYRank=i保持唯一。全序键为 `(invalidFlag,id,slot,originalYRank)`，invalidFlag=1−valid，故所有真实记录先于padding；比较字段用无符号逐字段比较，**禁止id相减转signed**。UINT32_MAX是合法ID，不作sentinel。padding有独立valid，不存在“maxID等于空”的冲突。

表的segment可以跨group，但这是存储复用：查询只分解本group内的[L,R)，所选每个标准段都被该范围包含，不会把跨group段整块误取。originalYRank不随各层ID排序改变；查询的L/R永远是原Y序坐标，不是ID表内部的offset。CPU预检G0≤L≤aRank<R≤G1≤n，slot映射互异且对应原scratch。

## 4. 并行稳定merge的唯一写入证明

层l从层l−1构建，m=2^l。每个原层位置j（0..K−1）一个invocation，读元素x=T[l−1][j]，定位其左右有序半段。设x在自己半段的下标为rSelf，在另一半段做全序key的lower_bound得到rOther：

```text
segmentStart = floor(j/m)*m
halfStart = segmentStart + (j-segmentStart >= m/2 ? m/2 : 0)
rSelf = j-halfStart
rOther = lower_bound(otherHalf, key(x))
out = segmentStart+rSelf+rOther
T[l][out] = x
```

CPU已拒绝真实重复ID；即使padding，rank字段也保证完整key全局唯一。对任意x，rSelf恰是自己半段比它小的元素数，rOther是另一半段比它小的数，所以out是并集里严格小于x的总数。不同key的此计数不同，故同segment写地址互异，且恰覆盖0..m−1；不同segment地址域不交。无atomic抢赢家、无跨workgroup写竞争、无丢失／复制记录。归纳自level0单元素段，各层均是所覆盖原Y段的全序排列。

本算法只读前层、写后层，不能原地覆盖半段后让别的invocation读到新记录。每层单独dispatch，前后依赖用SSBO barrier，不在一个dispatch里自旋假装全局barrier。工作组调度顺序不影响唯一输出；整数运算在已证范围内，不能靠uint wrap修索引。二分 `mid=lo+(hi-lo)/2`，避免lo+hi溢出。

层l每元素至多l次另一半二分比较（半段长2^(l−1)），总比较上界 `K*h*(h+1)/2`，构建O(K h²)。允许额外固定边界校验，但不得退化成每元素扫另一半段的O(K²)实现。

## 5. 范围查询、最小b与全局最小pair

每个真实a一个invocation（pad invocation显式写invalid候选）。用标准二叉区间分解把半开[L,R)分为互不相交的对齐2幂段；无需叶树存储，临时树索引仅计算段号：

```text
lo=L+K; hi=R+K; level=0; best=INVALID
while lo<hi:                          // 至多h+1轮
  if lo odd: inspect(level, lo-(K>>level)); lo++
  if hi odd: hi--; inspect(level, hi-(K>>level))
  lo>>=1; hi>>=1; level++
inspect(level,segment):
  base = level*K + segment*(1<<level)
  // 表已按invalid/id/...升序，不计算idA+1
  pos = first position with (valid==0 OR id>idA)
  if pos within segment AND valid==1:
      best = smaller unsigned-id candidate(best, record[pos])
```

分解循环不变量：已取段与剩余祖先表示的区间不交，二者并集正好是原[L,R)；取奇端点剥离一段，再上移父层保留覆盖。最多2h+1段（K=1特例一段）；每段完整包含于原范围，所以仅取合法group／Y候选。ID表排序保证“invalid或id>idA”是单调后缀，其首元素若valid就是该段最小的合法大ID；若invalid则段内无合法更大ID。idA=UINT32_MAX时全部真实ID≤它，只得到invalid／段尾，从而无b，**不会idA+1溢出成0**。

各段候选取最小idB，得到a的最小合法b。a本身和所有较小ID都被严格>条件排除，无需在GPU上修改表删除a。定义候选32B为两个uvec4：`{valid,idA,idB,slotA},{slotB,aRank,bRank,reserved0}`。合法输入ID唯一，id对唯一对应slot；invalid字段清零但不以id0决定valid。

全局reduction：比较valid优先，再无符号(idA,idB)字典序；其余rank/slot仅用于相同候选的确定诊断，不改变选对。固定workgroup W=64，每组64候选共享存储、二叉min归约，由lane0写唯一组输出；输入尾pad置invalid。所有lane都参与各级barrier，不能因超n提前return后让其它lane卡在barrier。组间分多dispatch、A/B双buffer轮换，直到1条。每轮每组唯一写者、不同组输出不交，输入不与输出别名；无跨组同步假设。

证明：每a返回最小b；对这些候选取字典序min首先最小化可匹配a，再最小化其b，恰等原全扫描目标。min在固定全序下满足结合／交换／幂等，故合法调度顺序与归约树不影响结果。无pair当且仅当所有a候选invalid；它与GPU错误／未完成／过期结果是不同status。

## 6. 同步、身份与CPU串行事务

原型使用独立EGL compute context及独占owner线程，不借XComponent renderer当前context。资源warmup时编译／分配，运行一次只一个request；双slot是失效隔离与资源预算，**不是同时跑两个依赖merge**。

顺序：上传header／level0／query、清result为NOT_READY → h层build（每层后SHADER_STORAGE barrier）→ query（barrier）→ ceil(log_W K)轮reduction（依赖轮间barrier）→ 最后输出status/identity → BUFFER_UPDATE barrier → fence／flush → 有界完成等待 → READ map／检查unmap → CPU核身份与pair。单纯barrier不等CPU完成，fence完成也不替代可见性和身份。

此处shader存储下一pass与CPU map读的barrier类别依据[GLES3.1 §7.11与§17](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)：前者用SHADER_STORAGE，后者用BUFFER_UPDATE。具体GL错误／超时／恢复与读取实现由architect落T130，不从T131 map成功推多pass自动正确。不得每层都CPU读回／fence；只在整条链后一次读结果，也不省层间GPU可见性。

身份必须覆盖run/state/config/attempt、candidateRole/Ordinal、substep、poolMutationSerial、contextEpoch、payloadSerial、algorithm/CPU range/layout revisions、Nmax/n/K/h与原F64 snapshot及上传bytes摘要。CPU保留完整身份manifest，GPU只回显明确编码的固定uint字段／摘要；64位serial拆hi/lo，不能转float。摘要用于关联／差错诊断，不是用hash碰撞概率代替不可变snapshot与owner世代检查。上传和执行期间输入不可变。

```text
for each original merge call:
  freeze current patchScratch & request identity
  CPU prepare exact groups/Y/ranges/unique IDs
  GPU rebuild all levels -> query -> min reduction -> return
  CPU verify identity, current scratch unchanged, ID-slot, full original F64 predicate
  // 原型每次同时与独立CPU全扫描／T123做最小性差分
  if valid found: original mergePairHot(...)
  else if valid NO_PAIR: original no-pair error path
  else: abort original transaction, report compute error; no partial authority write
  on successful scratch merge: mutationSerial++, invalidate all old GPU data
```

CPU运行时重检“合法”只是一道防线，最小性来自本算法证明及独立差分，不假装重检一个pair就证明没有更小pair。原型差分开销与非诊断执行耗时分开列，不能从最终收益静默删掉将来生产仍必需的校验。

**每次成功merge后重建**：a质心／质量／速度变、b退役／free-list变，后续birth亦变；新scratch重新CPU准备／上传／build／查询／readback。不能一次上传得到一批pair按旧质心顺序合并；即使pair互不共享slot，也可能不是串行下一次最小pair。每个λ、seed、preview/candidate、authoritative apply调用按原次序独立计费。snapshot有任何mutation都不能仅因n没变复用索引。

GPU不写权威pool／质量／nextId／预算／refund／rollback／ledger，CPU merge与失败audit仍原合同。错epoch、迟到、context loss、fence timeout、map/unmap失败、GL错误、摘要或输出错全部丢弃，保留错误status及输入；owner取消后结果不可应用新run。即使shader还在执行，失败slot也不能立即覆写／复用，须等待完成或销毁失效context按工程规则隔离。资源不足报CAPACITY_UNSUPPORTED，不偷偷降容量或改F64。

## 7. 最小原型接口与定容预算

建议新增独立测试harness／shader／manifest，不接生产route，所有权避开T123 Hot修改。接口划分：

```text
prepareExactRanges(snapshot, radius, identity, fixedCpuBuffers)
   -> status, Header256, Leaf16[K], Query32[K], manifest
executeIntegerPair(prepared, fixedContextSlot)
   -> Result128, stage timings, counters/errors
verifyAgainstOracle(snapshot, result, oldFullScan, cpuExactIndex)
   -> pair/status and optional subsequent-transaction equality record
```

query32字段 `{id,slot,aRank,L,R,G0,G1,valid}`；各层16B record、32B候选均以uvec4避免std430 struct歧义；header256与result128是本版固定载荷预留，architect须冻结逐字段offset／endian／长度，超出要版本化，不能工程自行truncate。result至少status/valid/idA/idB/slots/ranks/身份回显；checksum非正确性替代。可选逐节点诊断、GL计时query、完整表读回另预算并分列耗时。

本版shader数据payload每slot：

`16K(h+1) + 32K(query) + 64K(candidate A/B) + 256(header) +128(result)`。

| 配置Nmax | K / h | 主表bytes | slot载荷bytes | 双slot载荷bytes | 初始上传载荷上限bytes | dispatch / request |
|---|---|---|---|---|---|---|
| 192 | 256 / 8 | 36,864 | 61,824 | 123,648 | 12,672 | 8 build＋1 query＋2 reduce＝11 |
| 512 | 512 / 9 | 81,920 | 131,456 | 262,912 | 24,960 | 9 build＋1 query＋2 reduce＝12 |

上传上限=16K＋32K＋384（含result reset）；不上传整个(h+1)层表。每request结果回读128B，完整诊断读回若开启另计。CPU原pool／sorted indices／ID sort／range arrays／staging、GL对象/align/driver allocations、shader共享存储不在表内，architect逐项补总峰值。reduction64×32B=2048B共享载荷／组，W=64须核当前context的invocations/size/shared限制；不是仅凭设备名硬编码可用。

shader uint索引所有 `l*K+...` 和byte计数在CPU用checked wider整数先验，满足uint32及GLsizeiptr与SSBO上限后才dispatch；GPU按已验证范围运行。不得用0xffffffff做合法ID比较哨兵。冷配置边界含K=1/0active/1active，Nmax≤已配置限额；不允许运行热扩容、new Map/Set、每merge编译program／创建buffer。

总工作CPU O(Nmax+n log n)，GPU build O(K log²K)，query O(n log²K)（至多2h+1段、每段≤h+1次二分比较），reduction O(K)，空间O(K logK)；主配置下K=Θ(Nmax)。最坏单merge不是O(N²)，但**M次串行merge为M倍，不是整步一次O(Nlog²N)**。若实现用group全pair扫描／表全扫描取代分段二分，即使返回正确也未实现本路线复杂度。

## 8. 192/512同步成本模型与可行边界

一次request真实端到端关键路径：

`TgpuPair = Tfreeze+TcpuSortRanges+TbridgePack+Tupload+TdispatchBarrierChain+TfenceWait+TmapUnmap+TidentityPredicateCheck`。

TdispatchBarrierChain是11/12次CPU提交与barrier调用的实测区间；TfenceWait是整条GPU链剩余完成等待，不等于kernel时间，与CPU提交可能有重叠，不能再次叠一个“GPU timer总时长”双计。完整attempt以一个monotonic起止测量，分项用于解释，不用分项p50相加冒总p50。

T131汇总以“hot”标签报告192上传／提交／等待／读回p50分别 .005/.008/.295/.002 ms，p95 .006/.014/.722/.004；512分别 .004/.005/.271/.002，p95 .005/.007/.322/.003。按T136核对，旧hot只是标签，各bench仍gen/glBufferData，摘要五轮样本及p95尚未从现归档完整关联；上述数字仅为**待关联的报告统计**，不能当持久资源warm基线。旧probe还缺shader写后map的barrier及wait返回值消费，因此不把旧计时升级为修正同步后的成本。这些统计均不是本11/12 pass链的预测、下界或性能保证；本路线payload、barrier、driver排队、graphics竞争均不同。尤其不把11次dispatch乘11次fence等待：本算法只有链尾一次fence，但每次串行merge都要完成一次链尾读回。

若一attempt确实有M次非空配对请求，则本版调度计数精确为192档11M、512档12M个dispatch及M次结果完成／读回；M须按实测子步/seed/λ/preview/commit累计，不能凭容量推M，也不能从其它run或只写到的空池ledger假造冻结负载。n≤1短路单列，不混入GPU收益样本。timeout与失败保留，不能只计成功最短请求。

与T123比较，CPU已能在一次O(NlogN)中完成整个选对；本GPU路线CPU准备也O(NlogN)，另加O(Nlog²N)并行工作与同步，**没有复杂度优势保证**。同设备同输入同构建实际满足 `TgpuPair < TcpuExactPair` 才有单次查找收益；采用还要完整attempt p50/p95改善、最坏/UI/资源不触原门、重复批次足以区分噪声，沿T130不捏造比例门槛。共同merge/账本成本在双方相同，但仍须整attempt实测，不能只用代数消去后声称UI已不卡。

对N192/512目前建议**进入正确性＋收益原型，不预判采用**。更大N可作诊断但不能以4096收益替代主档；原型若主要耗在CPU边界生成、层提交或每merge同步，结论可为不采用当前GPU算法，CPU T123仍保留。不能为了赢改成一次批量merge、F32量化、减少preview、改变随机输入或将UI等待移出计时。独立物理线程可减少UI阻塞但属另工程合同，不等于GPU降低总计算时延。

## 9. 独立oracle与原型可测合同

本轮 `design-T137-gpu-pair-model-v1.py` 是**designer离散算法核对器**：模拟CPU F64边界、整数层构造、唯一写入、区间分解和最小pair，对比小规模原全扫描；输出到证据JSON。它不是GLES执行／生产测试，也不替代tester独立实现oracle。实际原型与后续全事务等价、设备收益均待另卡。

| ID | 必须核对的证据 |
|---|---|
| GP-01 | group全部五字段逐项不同、同组与跨组；ranges与原全扫描每a合法Y集合完全相同 |
| GP-02 | radius=0、±0、等Y、左右相邻ULP、次正规、有限相减溢出；rounded y±r反例不能遗漏 |
| GP-03 | n=0/1、稀疏active、非2幂192、满512、单group/多group、首/末group/range边界；0/1短路身份清楚 |
| GP-04 | UINT32_MAX及0x80000000跨signed边界，无id+1/相减溢出；真实重复ID跨group也拒绝；ID0 active拒绝 |
| GP-05 | 每层每segment结果等于独立CPU排序、所有输出恰写一次；padding不丢／不当真，originalYRank保持映射 |
| GP-06 | 每[L,R)标准段无交／无漏／无越界，查询等于原候选集合最小大ID；不能只核返回pair合法 |
| GP-07 | 全局pair／NO_PAIR与原全扫描及T123逐次一致；无匹配、晚配对、相同候选重复归约仍确定 |
| GP-08 | 连续merge质心改变与tail birth：每次新scratch重建，逐pair及最终质量／位置／速度／ID／free-list／lineage／events／budget/status逐位或沿原合同一致 |
| GP-09 | λ/seed/preview/authoritative apply完整序列，身份、真实M、dispatch/readback次数及rollback一致，不省失败请求 |
| GP-10 | 注错run/context/serial/slot映射、迟到、超时、map/unmap/GL错误、owner取消、context restore：无权威写、无旧结果被新请求消费 |
| GP-11 | 检查build/查询二分计数与O(Kh²)/O(nh²)上界、冷暖资源峰值／泄漏／热分配，不能O(N²)替换而报本路线完成 |
| GP-12 | 真设备全11/12 pass链和128B结果，实际层间barrier／尾fence／读回；固定样本矩阵、原始每次时序、p50/p95/最坏/超时、图形竞争与输入/UI延迟 |

tester独立oracle尽量不共用GPU key比较／range分解代码；旧全扫描是选对基准，原F64表达式次序是数值基准。perf诊断与正确性oracle模式分别报告实际开关及开销，冷编译／配置与warm分列。尚未实现原型的预测试RED按NOT_IMPLEMENTED登记，不误派业务缺陷。首版主规模192/512缺任一完整正确性／总时延证据时保持UNPROVEN，不据微kernel或本离散模型签设备PASS。

## 10. 反例、选型与交接结论

候选A：本版CPU精确范围＋GPU稳定merge分段ID索引，**推荐受控原型**；能给确定性／无冲突／最小性证明，代价是多pass和每merge重建。候选B：GPU atomic抢pair，拒绝，调度赢家不等最小pair。候选C：GPU N²全pair/reduction，拒绝作为本路线实现，未降低总工作。候选D：GPU F32高度／bucket，拒绝，原边界已被改变。候选E：一次选互不冲突pair批量执行，拒绝，旧质心不等下一串行状态。候选F：完整binary64整数模拟及GPU权威事务，延后，超本最小原型且需另证明。

必须保留的反例：真实MAX_ID与padding同值、重复ID跨group、signed排序把大ID提前、idA+1溢出、把Y秩当ID表秩、查询跨group段、同dispatch原地写／全局自旋barrier、早return导致组内barrier不齐、质心变后复用旧范围、只验pair合法就放过更小pair、旧payload读回当新success。所有这些都不靠浮点容差解决。

本方案已给T130要求的算法／证明／版本／最小接口与可测合同，回architect落实原型工程（结合T136能力／成本裁定）。原型开发与tester收益另派；不自动迁移生产、不改T123、不宣布当前卡死已修。无新性能阈值、无未经测量的采用结论。独立双测完成后designer再按此路线验证；失败仍先architect归因。
