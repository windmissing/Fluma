# T465 · 独立GPU测试证据边界与最小补证

2026-11-10，architect，P5，不使用真机。不改tester报告/实现，不重开T453/T459旧卡，不重跑产品测试。按已交原件和断言实现首次分流，冻结可复用资产和后续独立补证范围。原件/源码哈希及raw统计见 [证据索引](analysis-T451-T462-T463-T464-T465-evidence-v1.json)。

## 1. T453逐项裁定

T452开发host声明四档80次root执行；`SourceFixture::complete`仅在accept分支写raw，fatal不写。现存**60个raw**按kind/outcome为SOURCE接受32、SOURCE业务拒绝16、SOURCE重放8、QUANTUM接受4。不能写“80 raw / 60 accepted + 8 replay + 16 reject”。16个拒绝reason为PAYLOAD_CONFLICT=8、COLUMN_LIMIT=1、ID_EXHAUSTED=3、input reject=65539，各4；没有reason23–26的fatal原档。SOURCE host直接调实际GLSL和RootGate并手工派发13 pass，属于实际kernel/短接受层，不能覆盖应用Run/bridge/lease接线。

| T453断言 | 实际执行 | 可保留结论 / 缺口 |
|---|---|---|
| M203 32/32 | `h[141]==c[169]` | 两者都是sealedRows投影一致；**没有读h142/143或c170/171 mutationSerial**，不能记checked U64 +1已测 |
| M205 48/48 | accepted stop属于0..9、BIRTH_LIMIT时rows=K+1，replay跳过 | 停止码域/一种行数边界可复用；未证明firstId/birthCount连续区间或全部首达停止优先序 |
| M206 32/32 | accepted SOURCE事件kind15/16计数等于h137/138 | merge/birth摘要计数一致可复用；不是runtime per-origin liveMass审计 |
| M207 5712 rows | magic/revision/rootKind、ordinal、rowSeal、END尾、usedWords保留零 | 保留这批原档SOURCE128结构结果；未因此验证snapshot数学、pair最小性、RNE、来源隔离 |
| M208 “16 fatals” | `c[28]==2`业务拒绝，reason取宽并集且rows≤1 | 是业务拒绝结构；不覆盖fatal、23–26首错、已接受bank/pub/ledger全量不变或迟到尾退休 |

原档例 `raw-1` h141=c169=193，而mutationSerial(h142/143)=192；`raw-3` 行数5，serial196；`raw-11` 行数383，serial574。mutationSerial是跨本run累计，不能用单root行数代替。正确独立判据读取pre/post U64串号，对每个MERGE/BIRTH checked+1，水包BIRTH也计；DIRECT/END不加、replay/拒绝不重复计，按T445冻结数学。需同时核header与completion投影、跨低字进位及耗尽fatal，不把行数当变更数。

T446的31项独立枚举/RNE/事务oracle是**语义层**可复用资产，不需要重写或重跑CPU仿真。它曾列五项待V3字段的NOT_IMPLEMENTED，本次M207实际补到结构层，M203/M205/M206/M208其余不能随“5/5”一起关闭。现功能已有T452实现，此处应登记**独立测试未覆盖/证据不足**；不能再把实际缺证标为产品NOT_IMPLEMENTED。只有尚无运行时scene token攻击通道等确未实现的测试设施，按设施NOT_IMPLEMENTED登记并结束该项，不误派产品缺陷。

## 2. T459差异与lease用例问题

`runArm`的pulse线程在capture-on臂 `captureCommittedEpoch==0` 时直接continue，capture-off臂不等该条件。两臂脚本时间每一次真实wall调度回合才加100000ns，终点也按接受数量动态停止。该脚本不是相同的绝对时刻/调用序列，transport JIT/线程调度会影响pulse数和baseline；on臂还额外执行reserve/end并continue。这足以构成**测试输入不对称**，并不证明差异只来自JIT，也不能排除采集路径的真实扰动。

最终原件quotaCarry off=436000000、on=460000000，差24,000,000，按120×gap只对应总积累时间差200,000ns（两个100000ns步长），与不同pulse数相容；但没有完整pulse/baseline日志可唯一归因。正确状态是**完整无干扰未证**。可保留：这一次运行accepted/generated各8、revoked/overload/clearedCarry/pending/active相等，8份completion256逐字节相同，on臂RESERVED→ARMED→EXPLICIT_END→FROZEN，窗口accepted前缀0→6、业务后来到8。未实际比较两臂state/event/token序列、未执行完整首错矩阵；firstError实现还把END reason当错误，不能据EXPLICIT_END宣称已比较首错。`offerWords`未被调用，当前hook未注入注释所称seed/mid SOURCE，该次主要为空场QUANTUM。

lease段180s守护终止只证明**NOT_MEASURED**，没有阶段enter/exit原件可确定超时点一定是Create/JIT，不能把它当根因。用例还存在预期错误：ReleasePublication仅进入releasePending，Retire在fence signal后清租约，但保留available publication；该generation在尚未被新接受覆盖时仍可再次Acquire。T459立即Release后Acquire就期待STALE缺少“已退休/替换该publication”的前置，可能得到BACKPRESSURE或OK。T434/full_host在release+glFinish后又执行下一QUANTUM，才检查旧代STALE，该测试不可省去中间步骤。

T436四档full/prefix逐字节结果、T434 lease与14类故障的已执行原件可以引用其**具体范围**，不能代替采集on/off对lease时序的差分，也不能证明M2新增origin/回滚。若后续要改变“未覆盖publication可再次Acquire”的产品语义，交manager裁决合同变更，不能为满足T459错误预期改产品。

## 3. 最小后续独立tester补证

不使用真机，复用现有GPU host/GLSL及T446独立oracle，不新增CPU仿真或优化。分为可先读既存raw补断言、以及必须执行实际生产路径采新原件两部分；缺设施不要求developer补完真机才可继续其它部分。

| 包 | 输入和最小执行范围 | 必须落盘、判定及负控 |
|---|---|---|
| A 既存raw再判读 | 全部60个raw，固定wire/hash与每个root的scenario/predecessor映射 | 修正数量/outcome；真正读取serial U64及逐rootdelta、firstId/birthCount区间、header/completion投影；无法从raw获得pre-state/命令时精确标缺，不从输出反推oracle |
| B M2实际语义 | 生产GLSL/RootGate与独立T446预先冻结期望，至少K192/Kw64覆盖tie pair、nearest≠最小b、非eligible插中间、origin不同但sourceId同、radius±1、正负RNE tie/抵消/极限乘积、merge后重选、部分准入/NO_PAIR | 保存输入command、BASE前态、候选/事件/接受后态，独立重算每步pair与RNE；不能只比较开发assert或同kernel self-replay。四档资源/stride可复用既有结果，新增数学负控不要求每档穷举 |
| C V3五缺口 | 既存host故障hook23–26及generation溢出，SOURCE成功→QUANTUM失败，来源U64聚合、serial进位/溢出、ID末端部分成功 | 故障completion必须输出，即使不接受；保留first stage/row/word/expected/actual及pre/post accepted bank/header/pub/ledger/hash，证明整root拒绝、成功前缀保留；WORK control/originU64段由测试读回，或有等价可检验原字节证据，不能以开发assert替代 |
| D capture开关 | 生产App bridge两臂同固定输入、同绝对native/UI脚本、同baseline/终止屏障，on臂在两臂共享安全点reserve/END，off臂同点空操作 | 逐pulse记session/seq/两域stamp/ready/baseline，逐root state/event/completion/token/首错与成功前缀；账含carry/pending/active/cleared和守恒式逐点比较。不能按wall轮询数推进脚本，不能只等accepted=8即stop |
| E lease配对 | 同一已创建真实transport，on/off各做Acquire→release fence→未signal背压→signal/Retire→合法再Acquire，及新接受替换后旧generation拒绝；另测带lease的撤权/换代晚尾 | 每阶段enter/exit、slot/gen/token、owner/context、retire条件、原始返回码。GL阻塞有外部守护，超时输出当前阶段后NOT_MEASURED；不能180s后只写推测Create，不把守护超时改成业务期限 |

D至少一条正常SOURCE+QUANTUM前缀及一次相同故障切点，实际scope/token按T441分别规范化诊断capture身份与业务身份；业务字节和接受序不能随capture变化。将初始化/JIT完成与业务baseline分开，两个臂在相同脚本barrier进入baseline，完整保存喂入序列及hash以证明同输入；固定有限总脚本/最终排空边界，保留所有账与carry。若真实同输入仍出现差异，以具体第一个分歧回architect，不能继续标环境噪声。

C/E中的WORK/lease若现host无读回接口，tester可在测试编译域读取现有friend/helper，不改产品在线接口或接受数学。所需新测试设施及准确缺字段作为补证前置写清；没有设施时相应项NOT_IMPLEMENTED/NOT_MEASURED，不能伪PASS。T462真机Create诊断、T464恢复和本离线补证互不增加关闭门；各自有结果回architect再分流。设备能力、实际250ms/30fps依旧未测，不从离线推断。

执行者要么完成本卡约定范围与本角色必要检查，要么具体不可继续才停止，局部受阻先完成其它包。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态例外，不改其它行列/滴答同名清单。CPU停投。全部背景前景只XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT/标STALE旧完整有效帧。成功交原卡且本人行结束待验收；失败/缺证留未完项、复现原错、原件、尝试、不能继续原因、所需前置和接手角色，本人行看评论。

## 4. 归因交付

T453断言映射错误和T459输入/lease前置问题归tester修订独立测试；不是已确认产品数学缺陷。原T446语义oracle、T453结构子集、T459事务子集和T436运输对照按上表复用。新补证结果先回architect判断产品工程/设计/用例或仍缺证，manager仅登记派工，无重测或新增验收门。本轮只读核对已存原件和实现，未执行产品或设备测试，归因范围已完成。

## 2026-11-12 T478/T484 实际补证工程接续

T471线程/守护/lease用例首次归因与D1+D2+E可执行合同以 [T478](architecture-T478-symmetric-driver-lease-v2.md) 为准，保生产异步预约，不把worker单线程差分当完整覆盖。T479出生y/origin/断言解释纠偏及现有host受控BASE方案以 [T484](architecture-T484-m2-controlled-base-fixtures-v1.md) 为准，可直接交tester，无需线上注入接口或CPU仿真。旧卡报告保历史范围，未覆盖项不随其PASS关闭。
