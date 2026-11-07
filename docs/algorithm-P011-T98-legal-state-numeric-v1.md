# T98：严格合法态与可表示提交修订 v1

2026-10-26，designer；原卡 `6adf27a0e4b017ab340363c8`。路线 `P011_STRICT_REPRESENTABLE_STATE_V1`。状态 **DESIGN_READY_FOR_ARCHITECT / NOT_IMPLEMENTED / NOT_VALIDATED**。不使用真机，不改业务、原始证据或冻结测试。所有渲染仅新XComponent+shader，禁止Canvas上屏／回退（含背景前景）；失败FAULT或显式STALE上一有效新管线帧。

## 1. 裁定与不可兼得条件

消费architect的 `architecture-T90-tilt-interval-routing-v1.md` §3。推荐**统一严格合法态谓词＋真实候选状态证书＋writer在提交前向合法域内舍入**，不把preexisting的1e-12传播到candidate，不扩大H、不事后改高度、不绕planner。

旧失败状态的边差为0.5773502691896262，H为0.5773502691896257，差4.440892098500626e-16，等于H处4个ULP。它确实存储在硬界外；不是只因计算区间端点而凭空出现的差异。以下三项不能同时成立：①原字节不变；②仍以原H严格约束；③把该状态在θ0判合法。扩大容差、把残差吸附H或把target0偷偷移为非零都不选。

因此本方案明确：**原始非法快照不直接接受；修复发生在产生该快照之前的授权writer候选内**。新轨迹必须从合法初态重放，使松弛、LAND及其它改变床高的writer只提交可表示的合法产物。若只改preexisting删除epsilon而不改writer产物认证，会把失败提前，不能算本设计实施完成。若只改精确比较而不修writer，也不会消除已经存储的超限。

本修订更严格而非扩大允许域；它保证安全性及合法no-op不会被区间计算噪声误拒，**不保证所有动态输入都有可行计划或所有粒子都会按某时限LAND**。设备code5同根因未证，不能宣布本案自动修好T82/T95。

## 2. 输入、数值对象与版本

输入：固定S0／完整材料库存、pool与预算快照、候选计划、θbefore/target/maxOmega/dt、原H和配置／run／attempt身份。H沿现行 `Math.tan(30*Math.PI/180)` 的binary64值，证据hex=`3fe279a74590331c`；不替换成较大的近似常数。

新增 `legalStateRevision=P011_STRICT_REPRESENTABLE_STATE_V1` 与 `decisionArithmeticRevision=DYADIC_SIGN_V1`，进入config/trace/ledger/快照身份。原物理route、endpoint/peak窗口、外源与质量预算不换义。旧混用preexisting1e-12、slope严格、angle1e-9的判据保留历史标识，不能给旧raw套新revision再改判PASS。

定义本revision的状态对象：

- 原库存数组仍是权威binary64；`S[c]` 用既有左结合顺序 `(((bh+bl)+mh)+ml)`、每步RN64生成。缓存若与此位型不一致，报状态证书错误，不拿缓存凑合法。
- θ为已存binary64角度；`uθ=tan(θ*π/180)`按既有物理计算路径算一次，记录位型并在该次判定／提交复用。本文不声称库tan跨平台正确舍入；不把超越函数的不确定性伪装成新的物理容差。
- 对给定已存S与uθ，谓词使用这些数作为**精确二进制有理数**：`R_e=S[e+1]−S[e]−uθ`；合法 iff 每边 `−H≤R_e≤H`，且所有量有限、库存／容量原检查通过。
- rounded residual可作诊断；最终符号用error-free TwoSum/TwoDiff固定短expansion，或等价有界精确dyadic比较。不是全库存升级成任意精度，也不把S改成另一套隐藏高精度库存。

精确比较的依据参见 [Shewchuk的浮点精确谓词方法](https://www.cs.cmu.edu/~quake/robust.html)。工程须验证RN、无fast-math重结合、溢出／次正规数处理；不能把flush-to-zero环境当成已证明实现。非有限、unsupported算术条件均明确失败。

新增接口输出建议：`StateCertificate{revision,attemptId,thetaBits,uThetaBits,stateHash,legal,firstEdge,residualExpansion,compareSign,reason}`。证书只描述实际候选，不修改数值；绑定全材料／pool／预算配置摘要，不能用一张证书证明另一个preview。

## 3. 统一preexisting、candidate和角区间

### 3.1 preexisting

初态在θbefore下用§2同谓词。旧状态若只因1e-12通过而实际在H外，返回 `PREEXISTING_STRICT_INFEASIBLE`（旧numeric code兼容映射由architect定义），保留attempt／rollback／non-frozen，不重置run、不偷改S。

普通writer的历史non-worsening分支仍可作为单笔guard诊断／独立明确repair事务的局部限制；它**不能**把违规初态升级成合格planner状态。旧快照升级需从已知合法快照重放；若必须支持原地迁移，另案定义预算内材料修复事务，本案不授权“微量高度清理”。

### 3.2 candidate必须认证真实可提交状态

现有 `S0 + sum(deltaS)` 与 `(S0[e+1]-S0[e])+(deltaS[e+1]-deltaS[e])` 只保留计划差分，浮点求值顺序可能与实际库存writer不同。新接口必须返回preview在隔离shadow中按**同一执行顺序**产生的最终库存patch、规范S1及证书；不能只对旧差分做更精确求和就声称提交安全。

固定一个候选计划的数学区间由精确G_e=`S1[e+1]−S1[e]`构造：`u∈[G_e−H,G_e+H]`，全边求交端点／大小比较用expansion。精确±H的两边在θ0时交集为[0,0]；本次旧raw的±(H+4ULP)则仍为空，保持真实差异。不同θ会重新生成不同plans，不能把θ0的G_e复用为θ5的可行性证明。

**区间用于提议角，最终以对应候选的点证书为准。** `atan`结果是角度种子，不是精确边界证明；不可用 `lo≤hi+1e-9`接受已越界点。可对种子的相邻表示值做有限的点校验，找到符合角门的代表值，再按既有CLAMPED角色重跑计划。每个真正提交角都要以该角的真实重算计划及uθ认证；不得沿用REQUESTED计划给另一个角盖章。保持REQUESTED→至多一次CLAMPED→OLD的既有候选角色上界，不能增加无界搜索。

若反三角种子/相邻值不能给可认证的可表示角，明确 `NO_CERTIFIED_REPRESENTATIVE`，走OLD／blocked，不把窄区间扩宽。工程若改搜索策略，须先补对应有限上界与接口，不自行扩大候选次数。

### 3.3 定向和角速度独立硬门

定向区间始终 `[min(θbefore,target),max(θbefore,target)]`。`target=θbefore=0`（含−0规范化）时只有0；不借angle epsilon接受±1e-15，不把0变成避免空区间的偏移角。`dt≥0,maxOmega≥0`且有限，最终比较 `|θcandidate−θbefore|≤maxOmega×dt`，对已存输入的差／积做精确dyadic比较；展示用rounded端点不决定接受。dt=0或omega=0只许保持旧角。

requested先按既有速率提议，再投影到真实定向／速率域；每个提交点都重验两门。exact产品允许的角速度边界闭合，相邻ULP外拒绝，不沿用1e-9的接受捷径。target相反方向、零、±30邻域同样处理；外部双角／大倾角新路线不能借本卡默认启用。

**合法no-op证明**：S0严格合法，target=before=0，所有writer实际patch=0，则S1=S0且u=0；逐边点谓词同preexisting，定向／速率均含0，所以REQUESTED必可接受。该性质不要求所有非零计划也可接受，更不适用于本次已在硬界外的旧S0。

## 4. writer产物：提交前内缩，不能把数值误差加给H

每个改变S的既有writer／事件（含relax、settle、LAND/lift等实际会影响S的路径）必须在真实材料patch完成前生成候选；central与两外邻边按真实影响覆盖。旧闭式guard仍给原预算／stock／容量内的q上界，新增表示认证只可能减小允许量，不增加请求或预算。

给定固定writer分支，理想残差 `r_e+k_e q` 与实际库存运算产物残差之差界记为ρ_e。ρ来源是**具体浮点操作的定向舍入包络**：每次add/sub/mul的有效范围、ULP和error-free误差，沿真实split／库存求和图传播；不是手写全局epsilon。可使用区间外包求ρ，但不能用“附近看着差4ULP”定义统一容差。需涵盖最终S求和和三项residual；overflow/underflow不明就失败。

保守内域为 `−H+ρ_e≤r_e+k_e q≤H−ρ_e`（旧违规边non-worsening分支用原bound且不获全局合法身份）。取所有相关边与 `[0,qGranted]`、stock、capacity相交，将q向区间内侧的可表示数舍入。**缩的是动作域，不是把H变成H+ρ**。

固定最多三个候选：原闭式q的真实patch认证；必要时上述已证明内域的q认证；若两者都不可得则q=0实际认证。不能假设split取整后的整体可行性对q单调并盲二分，不能用多次试写碰运气。若分支变化使ρ界不再成立，重算明确分支包络或拒该动作，不外推旧界。q=0走真实零patch、精确谓词，不用保守区间宽度误拒边界不动状态。

单笔q=0不代表整个step合格；最终step全部材料／pool操作结束仍需全状态证书。某个writer无法生成合法候选时，按其原退款／保留在途库存语义处理；若整候选不合法，仍丢弃该preview，不能以“最大程度放行”越H。这里不引入额外松弛迭代，不额外搬移材料，也不把tiny q阈值改小。

### 4.1 守恒与预算证明边界

新逻辑只在**授权事务内**选择q_actual≤qGranted：pair move的H/L从donor扣多少就向receiver增加同相多少；LAND从pool减少同相量再向床增加，外源仍独立记成功量；拒绝时不执行任何库存patch。不得直接写sandTopScratch到nextDown(H)，那会跳过材料账本。candidate角改变时，未发布的内部stage可暂时出现相对该新角的违规边；仍沿原non-worsening限制认证各动作，不能把它混成θbefore下初态合法检查，也不能提前作为合格状态发布。

理想事务有逐相ΔM=0；binary64库存更新仍须通过原冻结守恒阈值／审计，本文不谎称“相同q加减”自动逐位零漂移。可用精确和诊断检查实际存储增减是否闭合，但原门槛不放宽。stock/capacity／pool lineage／ID和事件都用真实q_actual，不能只改deltaS遥测。

退款=实际已扣预算中未实施部分；沿原四层扣账／退款映射，不超过actualDebit，不重复退款，不形成下一步源债。local/total/bottom used与remaining维持原关系；q缩小不新增任何通道预算。活动状态仍沿请求/滞回合同，不把量被截为0伪造stop。

### 4.2 事务归纳证明

合法初始化→内部动作保持原合法边不越界／原违规边non-worsening且真实表示认证→最终candidate全边严格认证→原子提交／发布，所以每个发布态严格合法。**不声称**新角尚在规划中的内部stage始终全边合法；它不能发布，也不能用non-worsening替代最终严格证书。preview使用复制的库存／pool／预算／滑动状态，所有表示裁剪结果写入计划；commit逐项同序复用／核对q_actual和patch。preview/commit精确一致失败仍原样rollback／code，不给近似相等；失败不推进成功commit、时间、ID/free list或预算，仍保持field非永久frozen的原合同。旧step73快照拒绝不构成推翻此归纳，因为它不是新revision生成的合法态。

## 5. 方案比较

| 候选 | 结论 |
|---|---|
| preexisting/candidate都加1e-12或4ULP | 允许实际在H外的存储态；扩大guard，拒绝 |
| slope交集非空后以angle epsilon认requested | 可能接受定向/速率外或最终residual外的点；拒绝 |
| 精确谓词但writer不变 | 揭示真实非法，可能提前blocked；必要但不充分，不作为完整修复 |
| 发布后把高度吸附到H／nextDown | 无材料事务、破坏H/L账本／预算，拒绝 |
| 全库存改定点／高精度并迁移 | 改全部writer和状态格式，范围远大于局部问题；不选 |
| 严格同谓词＋动作内缩＋真实patch证书 | 不扩大H，合法no-op闭合，局部接口清楚；唯一推荐，可能更多保守裁剪，需观测损失量 |

## 6. 证据到件及未决范围

本轮读到T100续证：θ5真实计划deltaS[63..65]=[0.0013123359580052485,−0.04505666772096747,+0.04505666772096747]，交集约[−0.0463690,0.0901133]，与θ0的零delta不同；stepAfter.ok=true。按architect既定分流将其作为不同候选必须重算的输入，不把T86旧错误文件覆盖。T100报告文字的setTilt摘要与JSON若不一致，以其原始分栏值为证，不用文字反推状态被搬动。**θ5成功是旧revision事实**：其旧S0在严格初态门下同样不合格，新revision不直接接续该快照；新合法轨迹重放后的θ5需重新独立验证，不承诺与旧路径相同。补证回architect的正式处理不由本设计代替。

`design-T98-numeric-evidence-v1.json`记录本轮只读算术：H/相邻ULP分类、旧S0超4ULP、精确[0,0]与旧raw空交集、T100引用及SHA。它是设计推导，不是新产品测试。将S64换nextDown仅展示表示格点位置，**没有授权在业务中单独改这一高度**。

设备T82同code5是否同因仍等T102→T103首失败证据。若是另一collect/particle问题或真实不可行输入，按原流程回architect，不用本修订掩盖。

## 7. 回architect的最小接口和客观验证合同

不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md` 其它内容，仅自身状态有限例外；不改滴答同名清单，不回退共享修改。不使用真机；仅新shader渲染、无Canvas含背景前景回退，FAULT/STALE身份真实。

architect先修订P011合法态／guard／planning对应工程文档，明确：CanonicalS/ExactResidual比较接口、preview真实patch/certificate、writer动作包络及退款、候选点认证／有限代表值提议、revision与trace。developer只按其hunk实现；不由本设计直接派业务代码。T96等价热点优化与本数值语义修订分开，不能给语义改变仍标“逐位等价优化”。

必要客观用例：

1. 抽象g=±H与θ0精确交集[0,0]接受；g=±nextDown(H)接受；单／双边g=±nextUp(H)严格拒绝，真超限同拒；旧raw超4ULP不能直接PASS。
2. target=before=±0、dt/omega=0，接受0且拒定向外邻值；角速度恰界接受、相邻表示外拒。非有限值报错；libtan位型与物理使用同源。
3. 合法边界no-op经过preexisting/candidate/commit一致；真实S1与g0+delta不同舍入的反例必须按实际S1认证，不假装位型相同。
4. 从合法初态重放T86原轨迹，writer保持局部合法／non-worsening合同，最终提交／发布态必须严格合法；未发布的新角内部stage分列。trace说明首次不同q/状态/退款，不能只要求旧非法step73被接受。完整theta0/5各计划独立，不将θ5结果移植给θ0。
5. pair及列型writer覆盖中央／邻边、H/L split与pool LAND实际patch；零量、库存耗尽、capacity与tiny阈值、包络分支变化、预算退还；守恒／预算原线全保留。
6. preview纯度、同输入确定性、commit逐位一致、失败回滚／非冻结；原endpoint/peak身份不回退。边界安全不等于动态目标达成，后续T95按原长窗产品指标验证。

初始全边认证O(N)，每局部writer认证O(受影响边数×固定运算图)，最终全边O(N)。短expansion长度由固定表达式限定；精确积/次正规支持须有明确上限，禁止无界大数或逐帧扩容。动作最多三个候选，不加物理子步／全局迭代；trace仅首反例／固定容量。端侧新增耗时和裁剪量须测量，不承诺本设计自动改善帧率或消除所有hold。

设计的安全与no-op性质已给证明，实施／效果未验证；新revision与旧测试冲突必须按原流程显式登记，不静默删旧测试／改阈值。交回architect落地，manager只登记派工，不新增验收门。
