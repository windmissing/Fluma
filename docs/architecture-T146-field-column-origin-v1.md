# T146 FIELD_COLUMN首错误归因与补证工程 V1

2026-10-27；architect；原卡 `6adf6d30e4b06ee357480fa9`。不使用真机，不运行测试，不实现业务。消费T119正式报告/两臂原始48片trace、T117/T118诊断、Hot/M2/WaterSandField实际实现。当前FIELD_COLUMN整段谓词与声明基线`2c024c8`逐文本一致；当前树有T123等后续修改，不能把它当原设备包完整源码指纹。

**部分交付／关键现场证据不足，原卡看评论。** 已确定检测位置及工程补证合同，未确定哪项分量首次非法、哪条writer造成；不授权猜测性产品修复。T119采证交付成立，但其“s0证明所有分量合法”“drift0证明审计正常”“唯一前序成功commit污染”的推论须按本文收窄。T95仍等具体机制及必要实际修复，不因诊断文档交付放行。

## 1. 原始证据与可以成立的结论

只读解析`test/artifacts/t119-first-error-capture-20261026/{C1,C2}-full.txt`，两臂分别48个信封，part1..48重组；各payload98155字符，traces各2条。文件SHA256：C1 `d9d6a179c3e996302e62e8e6c929683e3a7dc174ad567bbc86e99cef56ab5de2`（2043747B）；C2 `98437e02e19ccee9aeb679ded6ed4ecbad909f09e1931b63617001594cffeac9`（2260523B）。未改原始日志/报告，重组不是重跑设备测试。

| 项 | C1 | C2 |
| --- | --- | --- |
| run | p011-run-1793026036189-3 | p011-run-1793026082568-3 |
| 成功substep/commit | 565；574→575 | 437；442→443 |
| 成功digest | c2d13ced→e7944738 | e3f44aaa→6b56926d |
| 失败substep/commit | 566；575→575 | 438；443→443 |
| 失败digest | e7944738→e7944738 | 6b56926d→6b56926d |
| 首错 | PARTICLE_BEGIN / BEGIN_HOT / raw2 / M2 code3 / FIELD_COLUMN，列67 | 同类型，列46 |
| 候选 | REQUESTED42，COLLECT99，collectColumn=-1 | 同左 |
| 前一成功候选 | REQUESTED42和CLAMPED≈30均PLANS_READY，particleStatus0 | 同左 |
| s0 | 成功/失败各128个0 | 同左 |
| particleEventTail | 空数组 | 空数组 |

BEGIN_HOT前stepOpen/active/pageBacklog/budgetOpen均0，失败后epoch未推进（1699/1311）、rollbackCode0。可以排除**这次入口**的BUSY/残留pageBacklog分支；不能据此证明整个run从无其它错误。失败检测先于collect A/B、预算begin和本次粒子apply。当前失败点不是AUDIT origin。

成功trace最后digest恰与下一失败trace开头digest相等，只是状态相关性证据。32位digest覆盖7个field数组及部分pool内容，不含surface、完整pool身份/free-list等；相等不证明逐位相同，变了也不能指认写列或写分量。beforePool是LAST_PRESENTED，不能当紧邻失败的权威pool全快照。空event tail也不能证明没有粒子写入：step成功收尾在无naturalObservationProbe时会清endpointParticleEvents，预览restore也截断尾部。

## 2. FIELD_COLUMN的完整语义

`P011ParticleHot.ets:beginStepHot`先逐列从0到127检查，首个不合格列记failureIndex；该列是**当前扫描最先发现**，不是时间上最早非法写入列。同列OR短路不记录实际触发子条件。

| 类别 | 实际检查 |
| --- | --- |
| 8个有限性 | waterTop、bh、bl、mh、ml、suspH、suspL、surface分别Number.isFinite |
| 7个非负性 | 上述除surface外逐项 `<0`；**finite负surface本身不触发此条件**；-0不满足<0 |
| 1个派生有限性 | 左结合`((bh+bl)+mh)+ml`必须finite，哪怕4个输入各自finite |

因此“某个分量非有限/负”不是唯一可能，派生求和溢出也在覆盖内；不能从列67/46推具体bh、LAND或dry通道。sum是实际RN64求值顺序，诊断不能改成高精度实数求和或重排。

`trace.s0=Array.from(substepSandTopBefore)`，只记录`bh+bl+mh+ml`的沉积总高度；不覆盖waterTop、悬浮、各沉积分量或field.surface绑定身份。即使总和为0，各项仍可能有正负抵消；且s0在规划前保存，不是在失败OR执行处读的8分量。不能用s0=0排除负分量/非有限的其它数组或中途变化，不能把它叫“完整S0合法快照”。

`recordCollectFirstFailure('PARTICLE_BEGIN', hotCode)`未传drift对象，函数默认把driftWater/Heavy/Light写0；本分支未调用getDrift。它们是**NOT_MEASURED的默认填充值**，不是实际测得0或审计PASS。原码origin已够区分检测分支，无需用这三个值进一步“排除全部漂移”。rollbackCode0在BEGIN_HOT失败时也来自默认返回参数，不证明执行过一次真实rollback并逐位恢复。

此外WaterSandField.audit及firstInvalidColumn对负分量用`<-EPS`（EPS=1e-9），Hot begin用严格`<0`；firstInvalidColumn还不检surface/派生sum。故`collectColumn=-1`与FIELD_COLUMN列67/46不矛盾：可能是[-EPS,0)微负，也可能是遗漏的检查项/取样时刻不同。**这是已证实的合同差异，设备触发是否属于它尚未证明。** 不因此统一改阈值、epsilon或clamp。

## 3. 实际现场路径、所有权和回滚边界

M2构造将`surface`绑定到WaterSandField.sandTopScratch，同一8数组由Hot读取；M2 buffer保存readonly引用，数组元素仍可由场/sink写。storage/binding检查通过不等于全部元素合法。

失败前的实际路径如下，存在多个可观察写点，不能仅保留“前一commit”一项：

1. 前一子步原apply/通道/审计/commit/收尾，随后可能有本子步`consumePendingGestureSand`的seed事务、moisture派生刷新。
2. step保存backup，再`updateSandTop`，保存substepSandTopBefore并进入规划。`updateSandTop`写sandTopScratch；DRY还会重写waterTop=`sand+悬浮+particleMassAt`，它并非纯读缓存。
3. REQUESTED preview进入`restoreBackup`：还原7组field、shake状态与pool，调用粒子rollback/discard，末尾`updateSandTop`再派生水面。接着preview显式update，再`substep`入口update；不是只读取trace.s0。
4. DRY不走waterLevel/diffuseSuspension；`saveMotionSnapshot`复制7数组，再M2.beginCollect→syncSources→Hot.beginStep。当前失败发生在这里，尚未执行本次advection/settle/percolate/creep/relax/lift及本次particle apply。前次成功的这些路径仍属候选，不能反向判它们唯一。
5. 成功候选内restoreMotionSnapshot/particle apply/projection/allocate、restoreBackup会多次写共享场；必须区分preview与authoritative。M2Sink的LAND增加mh/ml、LIFT按原比例扣层、其它分支增悬浮；dry会按pool重建waterTop，皆是待关联路径，不是已证故障writer。

saveBackup/restoreBackup和motionSnapshot覆盖7个field数组，不单独冻结surface；surface经update重算。Hot成功begin才冻结8数组及fieldBefore，失败FIELD_COLUMN发生在冻结之前，所以旧`work.frozen*`/fieldBefore可能属于更早step，不能作为这次失败的before。失败后WaterSandField.restoreBackup、M2 rollback等清理可能改变现场，补证必须先锁定原始失败值再清理；不得为了取证再调用audit/派生更新或修正值。

所有权要求保持原field/particle/budget事务及preview finally恢复，不改变对外commit/elapsed/粒子发布合同；失败时不能把污染值发布成成功新帧。现有digest不能证明全部恢复，T146不签逐值回滚PASS。

## 4. 先交developer的最小诊断工程修订

版本化新增内部`P011_FIELD_COLUMN_EVIDENCE_V2`，原FIELD_COLUMN/raw2/外层code5及阈值不改。只补诊断，不把产品writer修复混进本步骤；不使用真机，离线开发/编译/自测后交tester定点验证，再effector串行设备采样。修改范围Hot诊断槽、M2首错透传、WaterSandField trace与有限窗口观测；不改生产算法/renderer/通道算术。

- 在原FIELD_COLUMN同一执行点，清理前冷分配槽记录首失败列全部8值、各自IEEE754原64位hex（包括NaN/Infinity/-0，不经JSON数字吞成null），以及3个左结合中间和的bits。输入bits从原Float64Array的buffer+byteOffset直接读原始两个word，不先读JS Number再写回而丢NaN payload；派生和bits标明是本次计算值。记录8个finite标志、7个<0标志和sum finite；诊断badMask bit0..7为非有限（水、bh、bl、mh、ml、悬H、悬L、surface），bit8..14为前7项负，bit15为最终sum非有限；全mask用于诊断，不改原短路/首列选择。
- 关联run/stateEpoch/configRevision/substep/attempt/candidateRole/callOrdinal/theta、M2 attemptToken/Hot stepEpoch前后、sourceEpoch、commitBefore/当前commit、field buffer identity/byteOffset/length及surface是否确为sandTopScratch。失败快照独立于旧Hot frozen缓存，写定后M2.collectExit/trace只复制，不再次读取现场。
- 原firstFailure的V1值保留兼容，新增driftMeasured=false/rollbackExecuted=false及取值来源；只有真实audit传drift时标true。不能把所有rollbackCode0一律解释为未执行，应按真实分支设置。digest标注覆盖schema/相关性用途，不更名为全场SHA。
- 仅诊断开关开启时做有限阶段观测：前一成功apply结束/审计前/commit收尾、本子步gesture前后、backup前后/派生update后、preview restore前后、substep update后、beginCollect/Hot检查点；记录preview/authoritative与writerStage，不只记录substep编号。每检查点沿同一精确谓词找异常。cold两份8×128×8B场快照（合16384B）保存窗口内最后一次全合法/首次非法，另32条固定metadata ring，满时记dropped，不无限日志。
- 首次观察已非法则标PREEXISTING_IN_WINDOW，不能冒首次写入；阶段间从合法变非法，只能定位**该区间**。若区间含多个writer，先就该区间定点细化到源语句/列/操作ID，不把最后标签当唯一writer。pool派生参与时附该列particleMassH/L原bits与其pool epoch，并核完整冻结pool身份；不混入LAST_PRESENTED。
- 只读观测不调用updateSandTop/getDrift/额外rollback，不增业务校验调用次数，不变field/pool/budget/事件。关闭诊断时无热分配/记录；开启时固定存储，采集开销另报。失败信封保留原分片规则、首错误不可覆写，V2有独立revision/完整性/dropped及包/构建SHA身份。

## 5. 最小验证/补采与下一次分流

tester（不使用真机）仅针对V2诊断：独立构造覆盖每一有限性/非负性、sum溢出、-0与finite负surface不触发、同列多异常mask、首列顺序、NaN位型保真、先捕获后cleanup、旧frozen不误作本次before、未测drift/未执行rollback标记；验证诊断开/关field/pool/预算/事件逐值一致。[-EPS,0)案例同时展示Hot拒绝与既有audit容忍，不把既有判据差异偷偷改掉。无须重跑全部物理验收。

effector（真机标签，同机串行）待V2有效包后沿T119同款最小4 swipe复现，先取一条完整失败链；现有T119两run/版本/map等无关证据不重索。包/源/诊断版本、run/commit/候选/窗口关联必须齐；字段不足或未复现如实报告。只采最后合法/首次非法列分量、精确mask和对应阶段窗口；不重跑T95 LAND长窗/手持全套来代替首因补证。失败值的原64位、操作前后状态及归属到件后回architect。

到件路由：若同一原输入被不同校验合同接纳/拒绝，architect先明确数值支持域与调用边界，涉及算法合法域/舍入裁决交designer，不由developer自行放宽。若某确定writer超出已定域、错误回滚/绑定，architect补该writer的最小修复、所有权与精确回归合同再developer。若测试构造/报告解释争议交manager；未实现功能预期RED登记NOT_IMPLEMENTED。当前无足够证据指定LAND或dry writer返修。

下游通用prompt：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己行状态有限例外），不改滴答同名清单，不回退共享修改。开发/离线验证明确不使用真机；设备采样打真机并串行。所有渲染含背景前景仅新XComponent+shader，禁止Canvas上屏或失败回退；故障FAULT或保留标明STALE的上一有效新管线帧，不能伪装当前成功。manager按交付登记推进，不加验收门。

本轮未改业务、未跑离线/设备测试；只读解析原始分片、比对代码与版本、制定工程补证。机制仍UNPROVEN，状态看评论；采证交付不等产品通过。
