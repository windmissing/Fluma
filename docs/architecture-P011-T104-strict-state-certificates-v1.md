# T104 严格合法态、真实patch证书与writer工程合同 v1

日期：2026-10-26。architect。原卡 `6adf3250e4b06ee35743a638`。依据T98全稿/数值证据/复盘、T100 adf416d raw与T90；路线 `P011_STRICT_REPRESENTABLE_STATE_V1`。**工程接口交付；精确谓词/证书基础可开工，writer生产启用有具体设计证明前置；NOT_IMPLEMENTED/NOT_VALIDATED**。不使用真机。

## 1. 到件裁定与不可绕过的前置

T100真实θ5已补齐：旧revision同材料起点在target5重算出不同delta与非空区间；raw的setTilt前后effective均0，不采纳文字“effective发生变化”的说法。θ0边差超H四ULP、旧初态非法的事实保留；新严格revision不能原字节续跑并宣称修好。设备code5仍由T103首失败证据另判，不凭同码承诺修好T95。

T98给出严格状态定义、动作最多三候选和安全归纳，但**尚无各writer的实际操作图、ρ_e推导与分支范围**。工程不能用“通用4ULP/固定epsilon”代填。对真实库存patch可设计接口，对未证明的内缩量不可写入生产。下文分基础建设、明确设计缺口和实施顺序；不是只删preexisting容差提前blocked的授权。

## 2. 唯一数值对象与有界算术实现

`canonicalSandTop(bh,bl,mh,ml)`固定按现有 `(((bh+bl)+mh)+ml)`，每步binary64 RN生成，逐位核对缓存。缓存不同报CANONICAL_S_MISMATCH，不能反写缓存到库存。uTheta由同一次物理 `Math.tan(theta*Math.PI/180)`生成并保存Float64位型，predicate/preview/commit复用；不另求一次tan，不宣称跨设备libm位型恒等。

新增纯模块建议 `physics/P011ExactDyadic.ets`、`P011StateCertificate.ets`。为避免EFT在subnormal/FTZ上的假证明，**优先采用固定容量整数dyadic比较**，不升级物理库存格式：DataView解码binary64 sign/exponent/significand；有限数表示为带符号整数×2^-1074，乘积对齐2^-2148。实现只需有限输入的加减、两输入乘积、比较，不提供任意精度循环或泛用乘链。

固定272个16位limb=4352bits，最低指数−2148；最大有限binary64积绝对值<2^2048，最多8项求和<2^2051，因此最高用位<4200，容量有余。残差比较四项、角门的差与二输入积均在该表达式白名单内；超出白名单报ARITHMETIC_EXPRESSION_UNSUPPORTED，不能悄悄截位。只将53位significand拆成至多4个16位limb相乘，单个累加器每次乘加后归一，整数上界<2^53；不对272长数组做未界定累积，不使用JS位运算截断高位。移位索引均有静态范围，循环固定上界。

`compareResidual(Sleft,Sright,u,H)`精确比较已存数的 `Sright−Sleft−u` 与±H；`compareRate(theta,before,omega,dt)`精确比较差的绝对值与输入omega×dt；定向门直接按规范化±0后的binary64顺序比较。显示用rounded residual/atan不参与最终合法决定。非有限/库存负值/容量异常沿原检查拒绝；dyadic模块不能让非法输入靠符号位通过。

环境自检验证RN64基本运算/乘法、signed zero、DataView字节序及subnormal存取/算术，禁fast-math/FMA重结合。整数比较可精确保留subnormal位型，**不等于writer运行时算术自动支持它**；实际RN条件未成立则严格route报告ARITHMETIC_ENV_UNSUPPORTED，不给证书、不切旧route伪成功。native若另实现同模块，须关闭contract/fast-math并独立对照，不把C++默认编译选项当证明。

## 3. 类型、身份与证书生命周期

以下为需新增的typed contract，不是现有已实现API：

- `StrictInputIdentity`：legalStateRevision、decisionArithmeticRevision=`DYADIC_SIGN_V1`、runId/stateEpoch/attemptId/substepId/configRevision、snapshotSerial、thetaBits/uThetaBits/HBits、writerKernelRevision。config/trace/快照及账本配置身份显式携带，endpoint/peak业务字段/公式不改义。
- `ActualMaterialPatch`：定容changedColumn索引与bh/bl/mh/ml/suspH/suspL/waterTop before/after位型、pool patch/ID/free-list/lineage摘要、budget实际debit/refund、qGranted/qActual、分支及操作图revision；引用只读冻结输入与当前candidate scratch，不引用可被下一preview覆盖的数组。
- `StateCertificate`：identity、canonicalS1/patch hash、fullStateDigest、scope=LOCAL/FINAL、legal、firstEdge、精确符号/诊断残差、reason、certificateSerial。LOCAL只覆盖受影响列和全部相邻边；FINAL必须全边/完整原不变量通过，LOCAL不得被当作发布证书。
- `WriterCandidate`：caller/branch、固定输入摘要、实际patch、actualDebit/refund、候选序号RAW/INWARD/ZERO、envelopeRevision/证明范围、localCertificate；动作量不是替代patch的证据。

snapshot在attempt开始冻结，preview/每个candidate有独立serial。全量state/pool/budget digest在候选终结/提交边界计算一次O(N+pool)，局部writer用冻结输入身份＋patch位型、revision关联，不每笔全池hash。摘要用于身份，真实逐位比对仍是提交一致性依据，不能用hash碰撞或相同sourceEpoch替代patch核对。热缓冲冷configure/warmup定容；容量不足报CERTIFICATE_CAPACITY，不热扩容。

## 4. planning与真实执行的接口改造

`P011TiltPlanningPass.ets` delegate从仅`previewAt→plans`升级显式V2：`previewCandidate(theta, role, identity)→{status,plans,actualPatch,canonicalS1,certificate}`；原V1保留历史，不缺字段默认V2。`WaterSandField.previewChannelPlansAt/allocatePreviewPlans`必须在隔离完整shadow里以真实apply顺序执行所有writers/粒子/预算，再计算S1，不能拿S0+sum(deltaS)盖证书。原plans差分保留诊断/预算用途。

精确区间端点为已存S1的精确差±H，用有界dyadic求交；最终点证书同时检查合法态、定向门与精确角速度门。atan只作提议。工程固定代表点集合最多三个：seed、nextDown(seed)、nextUp(seed)，去重/非有限剔除，按距requested的binary64展示距离排序、同距取数值小者，再用精确硬门筛；不能因展示距离或angleEps接受。此集合找不到合格点可保守返回NO_CERTIFIED_REPRESENTATIVE，**不保证**它覆盖所有可行表示值。

REQUESTED→至多一个CLAMPED→OLD的实际preview上界仍3；代表点检查不暗中重跑多个物理preview。选定一个CLAMPED角后必须重新真实preview认证，失败走OLD，不能换另一个角继续无界搜索。requested速率提议沿既有浮点顺序生成但精确硬门再验，不合格不能以1e-9修正通过。target=before=±0、dt或omega为0只许0/旧角。需要更宽代表搜索或保证进展须另回designer，不由developer放大候选次数。

commit在原事务锁/串行边界使用同candidate输入和冻结配置，以同序真实执行并比对每笔qActual/patch、pool/预算/事件、最终S1与FINAL证书。任何位型/配置变化INVALIDATED，丢弃并原样rollback；不能只提交缓存S1或跳过库存writer。成功才推进commit/elapsed/publication，失败不推进成功身份、不重复退款，不永久冻结field。已有T102首失败导出复用，不建立第二套账本。

## 5. writer覆盖清单与真实patch接入

| 责任文件/hunk | 必须认证的真实变化 | 特别风险/依赖 |
|---|---|---|
| `WaterSandField.settleSandV2` | suspH/L扣除、mh/ml增加、shake provenance分摊；中央及两邻边 | scale=allowed/max(request,EPS)、各相乘法及库存加减的舍入不能由“ΔS=allowed”代替。 |
| `relaxAngle`、top split及pair move路径 | donor四库存split、receiver各相增加、全部外邻边 | donor分支、余量、H/L扣入次序和sliding滞回均进入patch；不能仅证中央差。 |
| `creepBottom/creepBottomRequests/splitBottom*` | 原两阶段bottom exchange及donor/receiver | 分相比例与reservation/refund映射不同，不能套top split包络。 |
| `liftByShake/splitTopV2` | 场库存扣除、seed/merge或wet悬浮fallback、dry等待/回滚 | pool事务失败必须按原wet/dry语义回退；源/预算记录只取真实q。shake0不抹残余能量。 |
| `P011ParticleM2Sink` LAND/LIFT及Hot patch | pool减少/合并/库存增加、guard host接口与lineage | sink目前LAND直接f.mh+=gh/f.ml+=gl；在写入前建真实候选，不能field先写后由证书补救。 |
| percolate/phase重排及其它同列writer | 即便实数总S不变，分项舍入可能改变canonicalS | 必须枚举并认证；不能因理论ΔS=0免证。 |
| initialize/restore/invert/external admission/其它直接库存入口 | 全状态合法性、外源发布和独立commit身份 | 不能只包step内writer而遗漏inter-step publication；原行为有实质冲突先报告，不私造新准入拒因。 |

实现时以实际库存赋值搜索＋调用图补齐writer registry，列每条路径是否授权/未支持，不以此表宣称已经穷尽代码。`updateSandTop`在dry还更新waterTop，canonical计算应与该物理副作用分开，调用只读证书不得触发水位重写。

## 6. 明确退designer的可落地证明缺口

由manager派designer补充，不重开已完成T98、不要求manager验收。所需增量文件可为T98 writer-kernels补篇：每个§5核给**实际RN操作图**、分支条件/数值域、ρ_e上下界推导（包含库存S求和）、q可表示内域取法及分支跳转处理。至少先交settle、top relax、pool LAND三个产生原失败床沿的真实内核，随后其余publication writers闭合。不能仅复述ρ来自误差传播。

还需明确：pool LAND减少q后保留CONTACT_WAIT的H/L/ID与request/grant关系；pair split舍入造成qActual与各相实际转移总和不完全相同如何记录而不改质量容差；原grant=0及tiny阈值下ZERO候选如何保证不执行有舍入副作用的“零量split”；旧违规内部stage的non-worsening bound用哪个固定快照；每核是否能证明INWARD后局部合法，不能则如何退ZERO/整候选blocked。包络可保守，但不能用无终止逐ULP/二分搜索弥补非单调性。

严格old S0不合法时不得用修复事务自动清理；若初始化场景/外源/invert与严格态无法同时保持现合同，须designer给显式支持域/版本或另案迁移，不能architect悄改场景/姿态。以上属于**生产writer实施实质前置**，不妨碍纯精确算术/证书框架和独立oracle测试先行。

## 7. 分阶段developer派工书与失败处理

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身状态有限例外；不改滴答同名清单，不回退共享工作。所有渲染仅新XComponent+shader含背景前景，无Canvas回退，失败FAULT或标明STALE上一有效新管线帧。

阶段A：新增纯ExactDyadic/CanonicalS/certificate类型及测试接口；旧production route不切换。先由tester建立独立精确dyadic oracle与边界用例，再developer实现定容纯模块、编译/离线验证。没有全部writer包络时交付只称基础模块完成，不能标step73已修。

阶段B：设计补篇逐核到件后实现真实patch builder/certifier、refund映射与V2 preview，使用feature/config明确标识严格route；尚不支持的writer/初态显式STRICT_WRITER_UNSUPPORTED，不能运行旧writer再贴新revision。代码范围WaterSandField相关局部核、M2Sink/Hot patch及Planning delegate/types，T96热点等价优化保持独立。

阶段C：writer registry与所有发布入口闭合、FINAL证书与commit比对通过后启用新route重放合法初态；旧快照retain历史，不直接迁入。旧非法快照返回兼容原TILT_PLANNING_BLOCKED code5＋新reason PREEXISTING_STRICT_INFEASIBLE，非有限/配置/证书容量按原对应错误类别并明确stage；不伪造source成功或更改elapsed/peak公式。文件恢复失败保留原件，不静默reset。

每核最多RAW/INWARD/ZERO三个真实patch候选，只有证明范围内INWARD可用；qActual≤qGranted，退款不超过实际debit且每笔一次，event/ledger/provenance写真实转移。不原地试写权威库存；candidate失败不能partial commit。证书失败本帧物理无新成功，renderer按已有FAULT/STALE合同展示，不拿旧frame冒充当前物理成功。

## 8. 客观回归与设备边界

独立测试覆盖精确±H与nextUp/down、四ULP旧raw拒绝、dt/omega0和速率/定向外邻值、非有限/次正规/最大指数算术、真实S1与g0+delta舍入不同、零patch边界no-op、每核split分支/邻边/预算退款/质量/stock/capacity/ID/lineage。从合法初态重放T86/T100两target，记录首次q/patch变化与所有发布证书，不要求旧非法S0被接受。preview纯度、同序commit逐位、错误回滚/非冻结、外源inter-step身份及endpoint/peak原用例全留。

算术独立oracle可在测试端用任意精度有理数；生产仅白名单定容模块，不将测试oracle引入热路径。耗时/缓冲/裁剪量与原性能合同独立报告，安全不等于LAND时限通过。设备T103不同机制取证后再决定T95相关修复/复测依赖；本稿不解除设备前置。

T104工程交付完成但生产启用仍依赖§6，原设计NOT_IMPLEMENTED身份保留。本轮无业务/测试变更、无测试重跑/真机，原卡报告完成与具体设计前置，不改他人队列。
