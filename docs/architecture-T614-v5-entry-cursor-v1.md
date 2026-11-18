# T614 V5同dispatch入口cursor修订

2026-11-18，architect，不使用真机。输入为T596原评论6afd53cce4b0d2e2f7786bb2、646af44回执、T552/T584/T527和当前canonical shader/transport。交付为**工程合同修订，产品未实现/未验证**；原T596续办，不另建同一开发卡，FULL继续关闭/META_INCOMPLETE直到完整实现。

## 切点裁定

不能在一次invocation中插host copy。`v5InputEnergy`在`v5BeginScope`之后同invocation执行credit/WriteAux/Seal，dispatch后cursor[1/3]确实已不是入口aux。另纠正具体opcode：当前`profiles_generated.h`先SCOPE_INIT（枚举9），后INPUT_E_REQUEST（15）；真正调用v5InputEnergy/BeginScope的是15，9仅role初始化。T596目前在9之后copy还可能取上一scope残值。文档中“scope初始化”不能替代实际枚举。

采用**现有cursor32内具名不可变入口字段**，不移动物理运算、不新增dispatch/GPU对象/热map/第五binding。不是反算末尾减1，也不是默认入口0。新增字段通过本合同正式分配，不私占reserved；全部新shader/生成资产SHA必须变化，旧二进制不能声称生产新wire。

## 私有GPU cursor32 revision2

字典及usedMask在 `architecture-T614-v5-entry-cursor-v1.json`、T552机器表cursorCapture中一致。0..8保原mutable含义：pPlanned/aPlanned/pSealed/aSealed/pending/pendingPStart/pendingAStart/firstError/auxWriteCursor。9=pEntry、10=aEntry、11=entryValid、12/13=rootNonce、14=role、15/16=branchEpoch、17=entryRevision2，18..30必须0，31=entrySeal3235775745。usedMask=0x8003ffff。该字典不修改prefix rootCursor64的未知保留字；不要将私有32word整块随意塞入公开prefix。

在既有SCOPE_INIT role初始化处由唯一lane清cursor32/entryValid/entrySeal（无论role将执行还是跳过，避免残值）；原来全量初始化也保持零。INPUT_E_REQUEST通过真实门并进入首次`v5BeginScope(physicalStart,auxStart)`时，写0..8原初态及9/10真实参数，12/13取uNonce，14取uM3Role，15/16取`W[m3RoleAddress(role)+5/+6]`的真实GPU branch。写完17=2、11=1，memoryBarrierBuffer后**最后写31 seal**，此后才允许第一笔credit。入口计入Begin已发生，即使随后请求/首aux失败也不擦除。

scope内已有consistency replay会再次调用v5InputEnergy。须拆出`ResetCompoundForReplay`：只重置mutable0..8，核同nonce/role/branch及起点，保9..17/31不变；不得把内部重放当新logical scope/新入口。无capture开关也维持同一物理运算；新增观测字段不参与物理选支/事件算术。下个scope重用必须先经过上述清零。

## 原件保留与公开wire

保留原8192B/root控制块内部布局：raw入口块768+64*(role−1)，raw末尾块800+64*(role−1)，每块32word，结束1088。把入口copy触发由SCOPE_INIT改为**INPUT_E_REQUEST dispatch后**，此时0..8允许已经前进，入口只从9/10读；API shader-storage+buffer-update barrier后copy。同role SCOPE_FINALIZE之后、下一role复用前copy末尾。单Q仍至多10次/1280B，四Q40次/5120B，0新增dispatch/热读回/分配；retain仍原5×8192B。没有成功进入Begin时入口块无valid/seal，不能取0..3冒原入口。

公开CaptureScopeMeta32升**revision2**，字段位置保持T552：8/11为p/aStart（来自入口9/10），9/10/12/13为末尾planned/sealed（来自末尾0..3），nonce/role/branch与入口、末尾、真实CONTROL128行共同匹配；entered/sealedComplete/firstError保原语义，reserved19..30全0、word31 seal最后写。真正完整共同前缀须pending0、p/a planned=sealed、firstError0及scope/control全部审计封存；entryValid只证明起点被写过，不授予消费/发布。

FULL的CONTROL_ROOT_0..4容器segmentRevision升2（包括未执行SOURCE的零body槽）；其它非prefix仍1、prefix仍2。ROOT模式CONTROL保持1/尾全0；header2、payloadV5_2、pageV2、T535 mask、61段目录/offset/容量/行stride、内嵌CONTROL128 rev5及事件wire全部不变。descriptor版本足够区分此扩展，**不占header reserved打隐含标记**。能力响应可具名报告captureScopeMetaRevision2。新reader必须按mode/index例外严格核版本，FULL CONTROL旧rev1不得冒新完整锚点；历史离线reader可按旧schema保留历史解释，不能自动升级。

公开control前6×128word不变，5×Meta32放768..927；928..2047清0。**内部raw快照区不能直接全块复制成公开control**，builder先搬前768word，再构造Meta、零尾；否则内部两份raw和公开5条Meta互相污染。SOURCE无preview表，CONTROL_ROOT_0 body仍全0，是否执行由真实rootRecord/input/sourceContext决定；不造SOURCE scope Meta。

所有末尾均来自同role原件。未进入role须有真实GPU控制跳过证据才NOT_EXECUTED；已进入但Begin之前错误、缺任一copy/身份/封存皆META_INCOMPLETE/FORENSIC，不猜未执行。首aux成功的例子：入口(0,0)，dispatch后mutable(0,1,0,1)，不可变入口仍(0,0)；AUTHORITATIVE可入口(p0,a0)非0，禁止套此例减1。首compound pending的失败保planned和sealed差异，只sealed双流共同前缀可消费。

## 原T596接续与检查边界

开发在canonical lineage_events/lift_writer/quantum_v5及**实际启用的**生成链同步上述逻辑，退役迁移脚本不恢复。更新cold_contract、cold_capture、collector/reader对CONTROL版本例外、Meta2、同源身份与usedMask的校验；同一提交更新机器表生成header/程序SHA，不能只加字段却仍绑定旧核。其余逐根INPUT/completion/quota/rootRecord、真实SOURCE跳过、首根前GPU/native联合预约及真实GPU退休票据、V2全链仍原T596责任，不能把本裁定当已实现。

本轮仅核源码顺序、字段分配、32word边界、40copy/5120B及既有offset/预算不变，无产品测试。独立tester后续补入口非0/首aux成功或pending失败/Begin前失败/跳过/内部重放不改入口/跨role残值/错rev与raw泄漏等反例；不把独立测试作为developer关闭门。developer仅开发编译，不使用真机；已有签名部署执行采证归tester/effector。CPU停投，渲染仅XComponent+shader，禁止Canvas/失败回退；原1–11和10暂停、水线13→14→12及旧五卡暂缓不变。

执行prompt包含：不允许修改`D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态例外）或滴答同名清单；要么完整完成当前范围和本角色必要检查，要么遇到确实无法继续的具体问题才停，局部受阻先做其它可做项。阻断原卡写全部未完/原错复现/证据/尝试/不能继续原因/前置及接手角色，成功结束待验收，失败/关键缺证看评论；不新增manager验收门。
