# T127 严格态逐核工程接力 v1

日期：2026-10-26。architect；原卡 `6adf424be4b0127832c267ff`。消费T110算法/证据/复盘、T104工程、T112 db1be20基础实现和T120位型勘误。**工程分阶段交付；纯核可实施，完整strict route仍NOT_IMPLEMENTED/禁止启用**。不使用真机。

## 1. 支持域与当前代码差距

T110已将通用ρ文字补为RN操作图、affine包络和分支域，可推进支持核，不再等待整个writer集合一次完成。严格H位型331c不改，真实旧raw3320超四ULP仍拒绝；不以T111合成3321代替它。T112已有定容dyadic comparator/CanonicalS/证书框架，不是完整writer certifier：`verifySandTop(scope=LOCAL)`仍检查传入连续数组，并未实现稀疏邻边/non-worsening/库存预算审计；身份字段/摘要框架也不自动绑定实际patch。不得把这个方法贴到旧writer之后便称接入完成。

| kernel registry | RAW | INWARD | ZERO/额外限制 |
|---|---|---|---|
| SETTLE_RN_GRAPH_V1，R>EPS且冻结dH/dL、stock合法 | 真实RN图＋全部审计 | affine包络/支持域证书后可做 | 直接无写，不执行零量split/库存±0；来源provenance纳patch。 |
| TOP_SPLIT_RN_GRAPH_V1/MIX | donor/receiver真实图 | 已证J⊆[0,min(M,qGranted)]且q>EPS可做 | 列d/r及所有邻边；q≤EPS不调split。 |
| TOP/MIX_LIGHT、MIX_LIGHT_HEAVY | 明确原分支图后可做 | **V1不实现**，没有RN逆像原语 | RAW失败退ZERO，标INWARD_UNSUPPORTED，不能盲二分。 |
| LAND_RN_GRAPH_V1，已CONTACT_WAIT单phase且合法request | full/partial实际patch | partial域0<q<m且RN(m−q)<m可做 | q=0无写；full点q=m仅RAW，不伪造非零域。 |
| AIRBORNE首次contact的LAND | 材料候选可在隔离测试研究 | 接口可复用 | CONTACT_HOLD_MOTION未证；若退ZERO需要改位置/state/vy，则**整条相应路径UNSUPPORTED**，不能穿床或强LAND。 |
| percolate/bottom/deep creep/correction/lift-seed/fallback/crossing/merge/advection/invert/外源/运行注排水与未登记writer | 不走旧核降级 | 不支持 | 实际可达即STRICT_WRITER_UNSUPPORTED；配置关闭且确实不调用才可跳过。 |

initialize/restore只有cold完整candidate＋FINAL通过可支持；旧缺revision/非法快照拒迁移，保留原件、不自动reset/微量修床。renderer同token重绘不创造新物理publication。阶段性局部支持不意味着step73或设备code5已修复。

## 2. 新增纯有界数值原语

文件建议 `physics/P011OutwardArithmetic.ets`，不引入BigInt生产依赖；复用T112数据位型和定容整数scratch。typed `Interval{lo,hi,valid,reason}`采用caller-owned固定槽，运算失败不回一个假0。环境前置同T104：RN64、subnormal/±0可存取及运算、无FMA重结合；不支持报ARITHMETIC_ENV_UNSUPPORTED。

`nextUp/nextDown`以DataView uint32高低字实现完整有限边界/正负/±0，不用`v+EPS*v`；向Inf取邻值意味着包络无有限表示时返回RANGE_UNSUPPORTED。add/sub先RN得到v，用compareSum精确判断v与精确a±b大小，必要只取相邻一个值包对应方向；mul用compareProduct(a,b,v)。divide端点先RN(a/b)，用exact(v*b−a)判方向，负b时符号反转；b=0/除数区间跨0拒绝。每端最多一个相邻修正，无逐ULP搜索。

区间加减取相应端，mul/div取四个端点组合的定向结果min/max，固定四次，零分母域拒绝。min/max只在已证单分支内选，跨分支不套同图；非法/NaN不能被clamp掩盖。η节点从精确值范围覆盖的舍入单元最大半宽向上表示，跨binade取最大；次正规half-ULP无法F64保存时取MIN_VALUE作为有证上界，不改H。节点记录range/exponent/eta来源，溢出/无法给界拒绝，而非宽容差。

新增**有限白名单扩展** `compareAffineBound`：至多一个两输入乘积＋七个带符号binary64标量，总≤8项，对齐2^-2148，沿T104272×16bits容量；积仍只53×53位sig，不允许标量自身是未展开无限乘链。max有限积<2^2048，七标量和远小于它，八项上界<2^2051，对齐后最高位<4200<4352；subnormal积最低2^-2148。每项归一整数carry仍<2^53。超项数/非有限报ARITHMETIC_EXPRESSION_UNSUPPORTED，不把这个API变通用表达式解释器。

此扩展用于精确`r0+a*q+b±B`端点确认；r0=Sright−Sleft−u不先round。B=H时一个积＋五标量足够；违规入口B=|r0|时不得把B近似为double，可按残差符号代数消去同r0，另一侧最多2*r0+a*q+b（r0重复两次，七标量＋一个积），精确比较0。**不新增第二个积或未界定长和**；每个midpoint/coeff需额外结构则本版UNSUPPORTED。

T112方法返回可复用result对象，必须在下次compare前复制其code/sign到标量，不能保存两个引用后误读同一最终结果。Outward模块定容warmup，非重入，按attempt owner串行。

## 3. 图、affine包络与确定q选择

新增 `P011WriterEnvelope.ets / P011WriterKernels.ets / P011WriterRegistry.ets`。每核显式编写定长node表，不用动态AST/字符串表达式执行。每node有op、输入槽、RN实际值、ideal interval、error interval、branch约束和nodeId；max node数由实现冷表精确计数，超容量拒绝，不热增长。独立oracle按实际node表审查，不用随机样本代替证明。

T110误差递推保留：加减传播E；乘法包含a*Eb+b*Ea+EaEb及η；冻结正分母除法E/D+η。所有bound运算向外。CanonicalS再按三次RN求和，δ0=C0−精确四库存和不可漏；相同输入重复乘法共享node位型/误差依赖，不能假装独立噪声抵消。affine系数用已存binary64区间，无任意精度有理数生产库。

INWARD先由明确branch域J得到上下affine余量；冻结writer入口r0、u与bound，RAW/INWARD/ZERO共用，不因失败改变基准。解一次约束按coeff正/负/0分类；分母不跨0，端点用Outward原语得到**充分条件的保守内域**，再取向内ceilRep/floorRep且用compareAffineBound复核。涉及非表示r0/B的中间numerator，先构造向外数值范围，取保证约束的保守端，不能取round值冒精确商。选择内域上端唯一最大可表示q；若舍入/域证书不能确定，INWARD_NONE，不无限邻点搜索。最大性只对这个保守内域，不对真实物理可行域。

settle的R来自collect，apply冻结dH/dL，A=(dH+dL)/max(R,EPS)不假定1；top M=RN(MH+ML)与精确MH+ML不同，MIX有A，后两段有常量b，不能忽略确定性偏差。INWARD必须对**整个J**证明分支条件，不能仅检查RAW点；MIX最小J按T110给定并与q>EPS/stock/grant相交，后两段逆像未证因此不解。

实际patch证书是独立防线；若包络宣称包含但实际落在外，报ENVELOPE_CERTIFICATE_MISMATCH并中止该候选/回architect，不偷偷再减q走ZERO遮盖证明缺陷。普通RAW不合法且包络无可行q才允许ZERO。每笔至多RAW、INWARD两次非零shadow图＋一次无写ZERO，绝不在权威库存上试写。

## 4. 真实patch与证书接口

`freezeWriterInput(kernel,branch,identity)→FrozenWriterInput`冻结run/attempt/candidate/writerOrdinal/config/snapshotSerial、theta/u/H位型、原库存、水/来源、预算actualDebit、pool/request/ticket身份，单核changedColumns≤2、受影响边最多4去重（现相邻top核≤3）。池slot/相态/来源/ID、free-list和cache前值必须在shadow可核对，不能只存pool质量摘要。

`buildRaw / buildInward→KernelPatch`严格保留T110实际RN操作序，qControl/h/l/phaseSum/库存stored-change/CanonicalS变化分列。字段包括node manifest revision、envelope/domain证明、actual material/pool/provenance/water patch、localCertificate、raw/actual debit-refund、request/grant event草稿。T112七通道beforeBits/afterBits作为存储基础，另加pool/water/provenance显式字段，不把预算变化挤进自由文本digest。

新增真正 `verifyLocalPatch` 只核changed columns及相邻完整边，读取未改邻居权威输入；每边按入口合法H或违规|r0|精确比较。LOCAL non-worsening可通过而全场仍不合法，FINAL始终全边≤H且原stock/capacity/finite/审计通过；两种证书字段区分boundMode/identity，不调用全场verifySandTop却伪称局部实现。digest与serial在最终交付边界计算，热单核不复制/扫描整池。

ZERO是单独`NoWritePatch`：不执行±0、不重算dry waterTop/cache、不reserve/close新成功ticket、不改pool state/y/vy/ID/free-list、不发成功LAND；保存同位型。若需要停止运动，返回CONTACT_HOLD_UNSUPPORTED给上层而不是把原disposition6包装ZERO。

## 5. 文件/hunk及逐核接入

| hunk | 实施要求 |
|---|---|
| `WaterSandField.settleSandV2`纯核提取 | 冻结depth/speed/dH/dL/R图，再shadow库存与shake来源分配；选择patch前不写场数组，保持collector/apply区别。 |
| `splitTopV2/relaxAngle`纯核 | donor mixed乘法与返回h/l顺序不变，receiver只加返回各相；中间updateSandTop在dry的waterTop副作用纳shadow，不通过调用现有有副作用函数“计算证书”。 |
| `P011ParticleM2Sink` kind2及`P011ParticleHot`DEPOSIT | reserve后的grant在shadow选择；池remainder/cache RN、full释放一次、partial同ID/contact、来源预算与landed事件同一patch；guard host/HOT mirror读取同snapshot，禁先改field再让pool prepare看半新状态。 |
| `P011StateCertificate`与new local verifier | 入参版本/owner/serial、固定边集合、精确bound、库存/预算审计接口、零副作用；原基础helper身份保留。 |
| planning delegate V2/真实preview | 只为registry支持的隔离测试路径接实际patch；完整产品路由暂不切。未来按T104同序commit/FINAL闭合后才能发布。 |

退款只退原已debit的qGranted−qControl一次，qControl不是h+l或真实ΔS，不能根据phase差再退一笔/改最后一相补账。三候选仅shadow ticket结算，最终选择成功后原子发布；失败恢复ticket、budget、库存、source、pool/ID/freeSlots/lineage与原attempt身份，不吞原错误/非冻结语义。实际质量仍按原容差审计，不宣称同q加减逐位守恒。

LAND若原request是AIRBORNE而q缩0需CONTACT_HOLD，整条严格路径不在本版运行支持域；已CONTACT_WAIT的ZERO可无写保留。partial正q需RN(m−q)<m，反相库存无需+0写；full q=m释放slot一次不动nextId。几何/运动writer未证不可由本核偷改。

## 6. 分阶段任务书、独立测试与剩余设计

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身状态有限例外；不改滴答同名清单，不回退他人修改。任何画面仅新XComponent+shader含背景前景，无Canvas回退，失败FAULT或显式STALE旧有效新管线帧。失败/证据不足原卡评论并自己行看评论，成功结束待验收。

阶段1 tester先补Outward/affine白名单独立有理数oracle与支持图用例，developer实现定容原语/node/NoWrite/local verifier；T125独立基础验证结果可复用，不能以T111弱断言替代。阶段2逐settle、top/MIX、CONTACT_WAIT LAND纯shadow核接入，后两top分支RAW/ZERO；阶段3仅受控测试driver真实preview/commit差分，不开启完整strict产品。各阶段开发/编译/离线自测可独立交付，不要求真机为关闭前置。

独立测试逐node包含普通/端点/相邻ULP、EPS/M阈/跨binade/subnormal、分母0、除数过零/overflow拒绝、δ0非0和settle A≠1、top M舍入b≠0、中央安全外邻越界、RAW/INWARD同基准、ZERO完全同位型、CONTACT_HOLD拒绝、full/partial LAND的ID/free/cache、audit失败退款/rollback、preview/commit同序。包络覆盖须有node域证明，随机差分仅辅助；无授意通过更大H/epsilon。

剩余实质设计：CONTACT_HOLD完整运动准入/事件/位置与材料ZERO关系，top后两段RN逆像支持，所有其它writer/publication/invert/外源闭合。按具体kernel新设计补篇，不重开T110，不由manager填公式；这些**不阻塞本版已支持纯核**，但持续阻止完整strict route启用。若某核实际源码操作图与T110证据版本不一致，developer停该核并回architect/设计明确差异，不自行换公式。

本轮工程接力交付完成，未实现业务/测试、未用真机。T104全route的NOT_IMPLEMENTED与设备独立code5故障边界不变，自己的行结束待验收、原卡说明支持域和依赖。
