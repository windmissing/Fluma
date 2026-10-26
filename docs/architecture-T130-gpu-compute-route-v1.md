# T130 GPU compute性能路线、事务与收益判定 v1

日期：2026-10-26。architect；原卡 `6adf42e9e4b06ee35744ccae`。消费T116精确选对/计算量、T92/T96/T97与现行F64权威pool/预算/ID/preview/ledger合同。**独立路线方案完成；能力/收益待T131，不是GPU生产实现或已加速**。不使用真机。

## 1. 要解决的工作与不变边界

用户要求评估compute解决卡死，目标是降低粒子合并/候选搜索的端到端成本并避免UI同步长任务，不是把画面render shader换名为compute。T94可证R1/R2同步merge→seed→lift阻塞，真实负载乘数仍不全；T123 CPU O(NlogN)精确索引继续，不能等GPU评估才修当前卡死。

原配对谓词：active、aID<bID、phase/sourceKind/sourceId/state/floor(x)相同，`abs(RN64(yA−yB))≤mergeHeightRadius`；选全局字典序最小(idA,idB)。每次成功merge会改变质量/质心/速度、slot/free-list，下一次pair必须根据**新scratch**重算，不能一次GPU批量选互不冲突pair后声称与串行等价。预算/退款/ID/lineage/事件/rollback与CPU原次序不变，T98/T104严格态未整体启用，GPU不得另设精度/容差路线绕过。

GLES highp float不是CPU binary64；不将y/radius或质量转Float32再做匹配，不把GPU atomic竞争赢家当最老pair，不把GPU side effects写进权威pool。初版GPU仅返回**只读索引建议/确定pair**，CPU精确验证与提交所有材料和账本；选对正确性仍须证明，CPU只验某pair合法不足以证明它是最小pair。

## 2. 能力取证T131与收益是两件事

T131 `6adf42e9e4b03273ad8646a7`独立真机卡已要求实际GL/GLSL/context、compute编译link、SSBO/workgroup/shared-memory上限、整数kernel/同步/恢复及微基准。当前gl3.h、EGL major3请求不能证明真实ES3.1可用；本卡没有设备结果，不声称支持。依据[Khronos GLES3.1规范第17章与shader memory同步章节](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)，dispatch、GPU内存可见性、fence完成与CPU读回是不同阶段。

T131整数map/reduction通过只放行“可考虑GPU原型”，不放行粒子事务迁移。能力UNSUPPORTED记录该设备/context限制，不能换设备/模拟器冒通过；能力支持但同步/读回慢仍可能不采用。不同GPU/driver/温控/图形负载身份分别报告，不能只用kernel submit耗时当完成时延。

## 3. 最小可比较原型：CPU精确边界＋GPU整数范围索引

候选A：CPU沿T116按group/Y/ID排序并用**原RN64高度谓词**二分得到每个a的合法Y rank范围[L,R]，保留binary64和原始scratch在CPU。输入GPU只有uint32 group内rank、ID、slot、valid、L/R及payload identity，不上传float替代高度。CPU计算O(NlogN)预处理计入总耗时，不隐藏在“准备一次”。

GPU在group/Y序列上建立确定的分段排序ID索引：pad至K=pow2ceil(N)，每层K个(id,slot,valid)；第l层对应长度2^l段的ID升序表。可用每个输入元素在另一有序半段做lower_bound求唯一输出rank的稳定merge，valid+ID+slot/原rank全序，padding在末；不得竞争写同地址。每层dispatch并建立SSBO可见性，层数log2K。查询每个a时把[L,R]分解成O(logK)标准段，在每段的有序ID表找首个ID>idA，取最小b；再对所有a的有效结果做确定字典序min reduction。总工作O(Nlog²N)、存储O(KlogK)，不原封扫描N²全pair。

这比CPU T123的O(NlogN)单merge工作量可能更大，优势若有只能来自足够规模下并行/批量摊销；**N512不预设胜出**。CPU排序/范围生成或GPU层间dispatch可能已超过CPU完整选对。原型不得通过改成扫描所有区间元素退化O(N²)来省开发却称本路线已测。

上面GPU索引构造/查询是新增并行算法，需要designer给确定性/无写冲突/复杂度与边界证明后再派原型实现：完整group/Y/ID映射、稳定merge rank、合法padding与uint32最大ID、重复ID拒绝、range分解正确性、结果最小性、每层同步/epoch及数值谓词CPU边界来源。没有该证明前，T131只做既定整数微probe，不让developer自行把顺序merge批量化。此为明确算法前置，不阻塞T123。

候选B（后续研究）：将纯排序/范围预处理也放GPU，需要完整binary64高度比较/舍入差界的整数模拟或经证明CPU生成的排序/域键，不允许F32量化。候选C（不选）：GPU N²每对判断并reduction，纯搬循环没有降低总算法复杂度且引入同步成本，不作为卡死修复结论。

## 4. 原型输入/输出与CPU权威事务

定义测试专用 `P011_GPU_PAIR_PROTOTYPE_V1`，与生产route分离。输入描述符含run/state/config/attempt/candidateRole/candidateOrdinal/substep/poolMutationSerial、contextEpoch、payloadSerial、N/K/count、所有版本、group/Y-order/range/ID/slot bytes hash、CPU原F64 snapshot digest。所有输入在该request期间不可变，uint32 ID不把MAX当empty（单独valid），rank/slot边界严格检查。

输出固定：same request identity、status、found、idA/idB/slotA/slotB、index/range/reduction计数与checksum。无pair不是提交成功；GPU错误不能返回id0假pair。CPU先核identity和snapshot未变，再核pair完整原F64谓词、ID/slot仍对应；最小性用独立CPU oracle差分证明，不仅运行时合法性重检。CPU调用原mergePairHot/preparePatch/预算审计及后续event顺序，GPU不写pool/库存/nextId。

CPU合并一次后poolMutationSerial++，旧GPU结果/索引失效，必须对新scratch重建并重新请求。对多个birth/λ/preview重新请求的上传/dispatch/readback都计入同完整事务；不能只测一次pair却外推整步有B次。ZERO/失败/rollback不发布新权威状态，candidate代/owner结束立即丢弃未消费结果，不让迟到结果应用到新run。

初版限定同步测试owner一次只一个request，预分配最多两slot用于失败隔离，不开放无界并发。未来异步生产若要避免UI等GPU，必须把**完整物理事务**交给独占物理owner线程：UI只提交有界顺序输入、消费不可变publication。事务可在owner等GPU但不拆成多帧半提交；touch/sensor/config按原消费身份排序，checkpoint/取消/epoch/队列满策略另案明确。线程迁移不是本卡授权实施，也不等于GPU必有收益。

## 5. context、SSBO/barrier/fence/readback与资源

测试原型使用独立EGL context及其单owner线程，不偷用XComponent renderer当前context；生产共享上下文/共享对象若另选，需额外资源可见性/排队/竞争证明和graphics干扰实测。不能在UI线程调dispatch后glFinish掩盖整帧卡死。

每层写SSBO后以正确shader-storage barrier供下一层/查询读取；reduction后建立所需buffer可见性，glFenceSync标记GPU进度。CPU通过有界轮询/等待至完成后按GLES合法buffer读取映射路径取得结果；记录map与unmap错误。barrier不证明CPU已完成，fence也不代替选对/数据身份校验；禁止在fence前读后把旧值当新输出。GPU timer若可用必须归档支持/有效/disjoint，不能以CPU dispatch返回时间填GPU执行。

K层索引用每record16B（id/slot/valid/rank）预算：K*(log2K+1)*16，加N个query范围/映射及输出/reduction临时O(K)；双slot显式乘2，CPU排序/原F64 snapshot/上传staging另计。N512主索引81920B/slot、N192取K256主索引36864B/slot，仅主表理论载荷非实测内存；实际align/padding/driver/query/fence必须单列。限额与SSBO上限/整数乘溢出在创建前核，失败UNSUPPORTED；不降N或改capacity让探针绿。

缓冲program在context准备/warmup阶段分配编译，热循环无每次gen/compile/无限cache；维护buffer/fence/query live count及峰值，release/context loss销毁/失效所有epoch结果。上传失败、GL错误、timeout、checksum/identity错误均丢弃建议，不提交CPU事务；原型报告FAIL/UNSUPPORTED，保留原始输出。生产若将来授权CPU精确算法重算，只能在尚未提交的同snapshot重算并标compute路径失败/CPU执行身份，**不能伪称GPU成功**。画面仍仅新XComponent+shader，错误FAULT或明确STALE旧有效新管线帧，无Canvas退路。

## 6. 规模、oracle与端到端计时合同

T131先按其既定N192/512整数map/reduction能力微基准，不能追加未证明pool算法负担。能力到件后manager可派designer上述算法证明，再developer测试专用原型、tester独立差分/收益卡：

- N16/192/512为主，1024/2048/4096只作扩大量诊断；active稀疏/满、早匹配/晚匹配/无匹配、group分散/同组/高度边界、重复ID拒绝、连续merge质心改变、tail births、8次λ及多preview完整序列。真实T94冻结pool未到件则明确工作负载合成，不能假称精确重演。
- oracle比较原全扫描（小规模基准）与T123精确CPU索引：逐次pair、无pair/error及后续完整pool/事件/预算/ID/free-list/hash完全一致。CPU F64高度条件与GPU整数索引各自验证；probe注入故障/迟到/错run/context restore/owner取消不能污染权威态。
- 同设备/同构建/同snapshot跑旧CPU、T123 CPU、native CPU可得对照、GPU方案；冷编译/创建首帧与warm样本分列，固定输入矩阵与重复次数事先登记。冷失败/timeout/所有outlier保留，不删失败只报暖态平均。
- 总时延由真实CPU准备/排序/范围生成＋native桥封送＋上传＋dispatch各层＋barrier/fence等待＋映射读回＋CPU校验/merge/账本＋跨线程排队组成，按monotonic clock端到端量，GPU timer作子项，不把重叠阶段简单相加制造更小“总计”。报告总p50/p95、最坏、超时/错误率、完整attempt吞吐、UI阻塞/输入延迟和render竞争影响。

吞吐异步多snapshot微基准可以另列，但不同物理run并行不等一个必须串行merge的事务快了；不能用大量独立GPU任务摊销出的吞吐替代N512单次完成延迟。CPU preparation和GPU共享graphics竞争都必须进对照。

## 7. 收益判定与T131到件后的路由

安全/确定性/质量/预算/容量/原性能硬门原样保留，原型不是放宽的替代测试。采用必须同时满足：目标设备能力与恢复可用、实际pool差分正确；在相同主工作负载上**完整attempt**总p50/p95比T123精确CPU基线改善且重复批次误差范围支持该改善，最坏/UI阻塞不触原硬门；全部准备/同步/回读/资源成本已计；未靠改变输入规模、F64或事务顺序取得结果。尚未冻结的性能“值得采用”比例由manager据测量决定，不在本卡捏造10倍/20%门槛或升产品PASS。

不采用条件：ES3.1/SSBO/kernel或恢复不支持，或完整事务正确性/确定性不能保持，或N192/512的端到端同步成本抵消收益；记录原因，继续T123/native线程候选，不将GPU能力失败当所有渲染失败。补测条件：只有kernel计时没有总时延、缺真实负载/CPU索引基线、timer无效、context/热态不一致、样本无法区分噪声，则保持UNPROVEN，给精确缺项而非默认采用。

T131结果回T130做增补裁定：能力通过只推荐是否进入受控原型，不声称已GPU加速；实际原型完整收益到件后才有采用/不采用结论。当前方案可以交付，依赖未到属于**约定后续评估**，不伪造能力事实或自动建立生产GPU任务。若结果是FAIL/UNSUPPORTED/关键缺证，按原卡失败规则回报，不让manager验收代替技术分流。

## 8. 文件所有权与派工边界

仅测试原型建议新增独立 `test/p013_compute/` harness、CPU oracle、GLSL kernel/manifest、单独CMake target；不改T123 Hot索引/M2Sink/WaterSandField或renderer共享hunk。能力探针T131独立，不先升级产品renderer context。若后续采用，architect另补生产物理owner/NAPI typed buffer/完整transaction/资源与错误接口，再developer分阶段实施，device tester/effector独立验证。

下游任务通用约束：不使用真机的设计/开发卡明确注明；需要设备的probe/复测卡打真机并串行。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己状态有限例外），不改滴答同名清单，不回退共享修改；失败原卡评论问题并自己行看评论。所有渲染只走新XComponent+shader含背景前景，禁止Canvas上屏/失败回退，故障FAULT或标明STALE上一有效新管线帧。

本轮完成工程路线/原型/事务/收益合同，无GPU业务实现、无能力实测/测试重跑/真机；自己的行结束待验收、原卡说明后续T131和算法证明依赖。
