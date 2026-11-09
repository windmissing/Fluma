# T441 · GPU 真机归档检查器格式差与事件证据裁定

2026-11-09，architect，P5。不使用真机。消费 T410 原卡最新进度、`v1-attempt2/v2-attempt2` 八件原档、T411 原检查器、T409 接口与 T421 对应源码、T422 报告和 harness。只交工程修订、原工具最小重放归因及后续任务范围，不修改工具/业务实现，不重跑设备。配套 [归因证据与原件SHA](analysis-T441-archive-schema-attribution-v1.json)。

## 1. 首次裁定

**主问题分流给 tester 修复独立归档工具。生产导出采用的 string[] 本身合法，不需要 developer 把生产协议改成旧 checker 的对象。** 当前不是已证实的“生产新旧schema升级冲突”。生产导出首次加入的 `9a8a563` 已是 `requiredEvents:string[]`，包含条件 FIRST_OFFER 的生成逻辑；T421沿用同一归档/page revision。T411 自造的 `{present,readySeen,...}` 是工具夹具格式，无已归档生产对象版的证据。

原工具重放两个真机档均仅在 `T10-REQUIRED` 失败，报 `缺: BEGIN,END`。实际 EVENTS 页里两个事件都存在，manifest.requiredEvents也恰为 `["BEGIN","END"]`。根因是 `t409_check.cjs:79–85` 将数组当对象读 `req.present`，取得空数组。代码未核实际事件kind，存在反向漏判。只把数组转换成 `present` 会消除当前误报，但保留假声明即可通过的缺陷，不能作为完整修复。

T410 关于单次预约链路已经跨过 BUSY 的窄证保留。其“非档缺陷”收窄为**当前T10误报不能证明原档缺BEGIN/END，已核部分也未见缺页/哈希差**。完整独立检查器修复前不宣全档PASS，双源、负控、lease与30fps仍需各自实证。不阻T410继续完成其他可做的真机项目，不要求重装当前已安装包来解决离线工具问题。

## 2. 两个真机原档实际包含什么

| 项 | V1 attempt2 | V2 attempt2 |
|---|---|---|
| scope | capture1 / epoch2 / run0:1 / session1 / GRS_M1_PROGRESS_CLOCK_V1 | capture2 / epoch3 / run0:2 / session1 / GRS_M1_PROGRESS_CLOCK_V2 |
| 预约回执 | request1，safePointSerial1，FROZEN/DURATION_REACHED | request2，safePointSerial2，FROZEN/DURATION_REACHED |
| native窗口ns | 3604984938442672–3604986940182047 | 3605793762313610–3605795763087985 |
| 页面与实际事件 | SUMMARY+EVENTS，rowCount2 / tokenCount0 / pageCount2，serial1 BEGIN、serial2 END | 同左 |
| requiredEvents与seen | string[]仅BEGIN/END，六个seen均false，sourceOfferCount两相均0 | 同左 |
| 引用页原字节/SHA | 两页2015+2452=4467B，声明与实际全部相同 | 两页2346+2422=4768B，声明与实际全部相同 |
| manifest原字节及总账 | 2454B，totalBytes6921=4467+2454 | 2785B，totalBytes7553=4768+2785 |
| 首末账/接受/显示 | BEGIN账等于initial、END账等于terminal；八字段全0，无token、新物理帧0 | 同左；renderTargetHz30只是配置值 |
| 预约时既有状态 | initialPhase NEW_RUN_REQUIRED、permission PAUSED_CLOCK、initialFailure CLOCK_GAP | initialPhase CLOCK_GAP、permission PAUSED_CLOCK、initialFailure WAIT_PAGE_PULSE |
| 窗口pulse | pulseApiCalls180，acceptedPulses0，rejectedPulses180 | pulseApiCalls180，acceptedPulses0，rejectedOtherPulses180 |
| 详细行统计 | suppressedDetailedRows0 | suppressedDetailedRows1169，dropped0，非丢关键事件的同义词 |

两档包SHA均为 T421-release `5450ad3052654de11994e180870e0a6b9e0db7c80d2a6e95aa572003367ae751`。四个page文件与各自manifest引用的原字节和SHA逐项吻合，manifest原件SHA及独立预约回执SHA见配套JSON。本轮只核归档声称的包身份与已交签包记录，不重装/重新证明设备实际安装身份。

这些文件证明一次预约已在真实owner检查点被消费并自动冻结归档。它们不包含活跃物理接受，不能用窗口内短gap直方图、零配额Balanced或页面标V2推导真实控制已健康。现有故障在arm前已存在，窗口内没有新READY/源offer/接受/首错/撤权时，对应事件缺席可以合法；不能为补齐“好看链路”补写或强制要求本窗重放旧故障。

BEGIN/END里的permit字段是捕获瞬间的控制快照，可为旧permit或全0。V2页的permitEpoch0与归档epoch3不应被一条“所有permitEpoch必须等于capture epoch”的规则误拒。完整捕获scope必须一致，实际root接受/呈现的有效permit另按语义验证。BEGIN真实行时刻略晚于startNs是采样开销，不要求两者数值相等。

## 3. 当前工具缺口与最小证据

| 落点 | 已复现或源码确定的问题 | 精确分流 |
|---|---|---|
| T10 required | 读对象present，真机string[]误报BEGIN/END缺失；从未查EVENTS实际kind | tester重写为实际页面扫描与独立required推导 |
| 自带t409_good夹具 | manifest声称present含BEGIN/END/FIRST_ACCEPT，实际行是BEGIN/PULSE/PULSE/PULSE，旧checker仍PASS | 该夹具不符合产品V1，不能保留为产品正例。PULSE不是本版native事件kind，周期pulse存统计 |
| INCOMPLETE/退出状态 | 将该合成夹具置INCOMPLETE且dropped1，原API输出state PASS、unjudgable=true，CLI退出0 | 完整性失败、不可判和可消费PASS必须分开，非可消费结果不返回成功退出 |
| 页数公式T12 | 使用 `1+ceil(rows/4)+(tokenCount>0?1:0)` | 正式是 `1+ceil(rowCount/4)+tokenCount`，每TOKEN页一个token，合法2–8token现被误拒 |
| T13事件序 | 用文件名是否含SUMMARY/TOKEN猜页，汇总serial后排序 | 应按cursor/type读取，沿实际存储顺序核1..rowCount；排序会掩盖调换行/页 |
| scope/账/首末 | 注释声称核每页scope、END账，实际主要查manifest账字段类型和serial | 必须核所有页重复scope、SUMMARY与manifest、真实BEGIN/END账、游标/type/complete及token数量/身份 |
| 数值/未知输入 | 多处 `|0`截32位、宽松十进制，未知revision被SKIP且空目录可成功结束 | 严格范围和U64解析，未知revision/无输入不输出PASS，不能靠转换吞非法类型 |
| 入口说明 | T411报告称 `archive_check.cjs <dir>` 可直接消费，实际该脚本没有T409分派 | 显式修文档和CLI路由；旧timeline与新T409保持各自parser，禁止互相追认 |

前两条设备重放及两个最小合成反例已用原模块导出函数执行，未改原工具或原档。反例目录在 `.codex/local-test/t441-checker-*`，仅是合成诊断，配套JSON记录调用结果。复现可调用 `require('./test/t411/t409_check.cjs').makeT409Fixtures(<独立新目录>)` 后检查 `t409_good`，即得缺真实END/FIRST_ACCEPT却PASS；再对独立副本设置INCOMPLETE/dropped1并正确重算manifest总字节，CLI仍退出0。不能把这些反例的PASS写为产品通过。

## 4. 冻结的独立检查合同

新工具输出自身 `checkerRevision:T441_ARCHIVE_CHECK_V2`，输入协议仍为已有 `T409_EVIDENCE_ARCHIVE_V1` / `T409_EVIDENCE_CAPTURE_PAGE_V1`。不因修工具强迫T410更包。旧 `T306_TIMELINE_COLD_V1` 归档仍由旧timeline parser按其原字节语义复放，不能用T409补字段追认。生产requiredEvents对象版没有证据，不默认接受；旧对象夹具转为明确SYNTHETIC/INVALID反例。如未来确有合法历史对象生产档，先给原件、来源版本和合同再建专属adapter，不能仅按JS类型猜版本。

### 4.1 原文件、页序和scope

1. 固定明确manifest输入；目录无manifest或未知revision返回UNSUPPORTED/INPUT_ERROR，非成功退出。每个引用name唯一、文件实际存在、bytes为合法整数、SHA64hex与原字节一致。文件名限制同目录，不能把重复引用同一页计为两页。
2. dataBytes核实际页字节和，totalBytes核实际页字节+**原manifest UTF8字节**，同时核声明预算及上限1MiB。禁止重新格式化原件以匹配SHA/总字节，不将manifest自述COMPLETE视为校验结果。rowCount2..4096、tokenCount0..8、pageCount按正式公式及≤1033，maxRows/maxBytes与SUMMARY一致；数值禁止`|0`、NaN、负数、隐式字符串转换和U64溢出。
3. 解析页面后用cursor唯一定位0..pageCount−1，核next=cursor+1和complete只在最后为true。第0页SUMMARY，接ceil(rowCount/4)个EVENTS，之后恰tokenCount个TOKEN；token页为单个`token`对象。文件名只作路径，不推断type或顺序。
4. 每页核revision/state=FROZEN、captureId、activeRevision、packageSha256、epoch/runLo/runHi/pageSession、start/end、row/token/pageCount、incomplete/native reason及evidenceSource/clock/uiClock/probeClock。manifest使用nativeClock对应page.clock；export reason与native freeze reason是不同字段，不强制两者相等。SUMMARY与manifest重复summary逐字段相等，不能被篡改其中一处。
5. 按页cursor及页内原顺序核事件serial为严格十进制字符串1..rowCount，不排序修复。每行captureId匹配、kind为已知字典，ledger每字段为规范U64字符串。核首BEGIN、末END唯一，BEGIN账=initialLedger、END账=terminalLedger；页/行缺失、乱序、重复和串窗各有具体错误码。
6. 显式预约回执作为独立侧证，若提供则核T417 revision、request/capture与scope/startNs、safePointSerial和armByNs关系，不混入旧T409页面必填字段。缺预约回执影响预约专项证明，不将旧的合法T409归档一律打成缺页。

### 4.2 事件字典、推导与缺席

本版native字典为 BEGIN、READY、SOURCE_ATTEMPT、FIRST_OFFER、FIRST_ACCEPT、FIRST_FAILURE、REVOKE、END、GL_DETAIL。stage0是合法关键kind，不可当“没有GL stage”而丢弃。未知kind/混合或对象requiredEvents、非字符串、重复、缺BEGIN/END均明确拒绝，不静默忽略未知。

| 来源 | 独立推导和实际事件义务 |
|---|---|
| 恒定 | BEGIN和END始终必需，真实行在首末，不能只读manifest或present声明 |
| summary.readySeen | boolean true必有READY；flag与实际READY存在性须一致，false不强制补历史READY |
| summary.sourceAttemptSeen | true必有SOURCE_ATTEMPT，保存down.phase与原UI/原生身份；仅有boolean不能证明沙水两相都发生过DOWN，按实际phase逐项报告 |
| summary.sourceOfferSeen及sourceOfferCount[0/1] | flag应等于任一相计数>0；每个offerCount>0相别各需一条FIRST_OFFER，phase来自该事件code0/1。sourceEnqueuedCount≤对应offerCount，FIRST_OFFER拒绝结果也必须保存。FIRST_OFFER与SOURCE_ATTEMPT不可互替 |
| summary.acceptedSeen | true需FIRST_ACCEPT并有实际接受token样本；样本条数不等总接受数。tokenCount>0而false、true但token页缺失均拒绝或依原incomplete结果拒判 |
| summary.faultSeen/revokeSeen | 分别要求FIRST_FAILURE/REVOKE。首次故障导致冻结的原事件和END保留，不能由当前实时status补造 |
| GL_DETAIL | 按detailPolicy选择的明细，不属于每root/每pass都必需。suppressedDetailedRows不是dropped，不据聚合制造缺失的精确GL事件 |

从SUMMARY的严格boolean/相计数计算required集合R，核manifest.requiredEvents为对应已知字符串集合，核R中的实际事件全部在场。还要反查实际事件与seen/counter一致，禁止把两个地方都删掉required来漏检已发生事件。FIRST_OFFER规则在T409初始实现及接口补充中已存在，两个attempt2并没有实际FIRST_OFFER；不能写“这两档新增FIRST_OFFER丢失”。

合法空闲档可只有BEGIN/END，但其语义活动必须报告NOT_EXERCISED。若独立动作日志声称在同窗发生源事件而页中无对应证据，即便seen与required都被一致清零，也不能判产品通过，转生产观测路径补证。工具无法从两个全零快照证明“窗口没有发生任何事”，不能自造该证明。

### 4.3 TOKEN、账与结果消费

TOKEN页面数量和root唯一性、scope、完整rootToken/接受身份、kind/phase/receipt必须真实可解析；已有预算max8保持。`NOT_ATTEMPTED_NO_SCENE_TOKEN`是合法跳绘制标记，不能强制每root都draw，也不能补requested/rendered。ATTEMPTED保原token和current/swapSucceeded真假，失败呈现是**合法失败证据**，档本身完整时不把它误判成文件格式损坏。物理接受、配额oracle和显示链判据由T412/T416按其合同消费。

真实缺页/错SHA/未知schema/乱序/必需事件或首末账不符输出FORMAT_OR_INTEGRITY_FAIL。manifest INCOMPLETE、任一page.incomplete或dropped>0输出UNJUDGABLE，CLI非0，不能以`state:PASS,unjudgable:true`作为下游成功。具体编号可由tester沿既有T1–T14扩展，但stdout必须有稳定机器结果，失败码与原始错误定位保留。完整且全检查通过才输出ARCHIVE_INTEGRITY_PASS，并附activity分项。有效失败档、零接受档和HOST_SCRIPTED_OFFLINE均不能冒设备功能/30fps PASS。

T412的整数配额恒等式与逐序列差分独立保留，archive checker仅核账的类型、scope及首末一致，不从直方图逆造pulse序列。T416只消费由该checker通过且manifest/page哈希绑定的具体原档；不能用另一个窗口的PASS或手工BEGIN/END替代完整性结果。这是现有证据消费合同，不增加manager批准/验收门。

## 5. tester工具修复任务书与正反fixture

不使用真机。所有权限于 `test/t411/t409_check.cjs`、必要的主CLI分派/README和独立fixtures/报告。冻结本稿及原件SHA后修复第3–4节各项，保持独立实现，不调用产品断言作唯一oracle。不改业务导出、设备安装或T410原档，不删除必需事件。旧timeline parser和真实旧T399/T405拒判语义继续保留。

| fixture | 输入与预期，不允许被更早错误遮住目标门 |
|---|---|
| 当前真机两档 | 原件不变，应能在新工具完成所有结构/身份/页序/事件/账检查；活动分项为零接受/无token，不产生双源或30fps通过 |
| 正常活跃、合法失败与无呈现样本 | 真正符合V1的SUMMARY/EVENTS/TOKEN、非零接受、两相offer/后继Q；分别含ATTEMPTED成功/失败与NOT_ATTEMPTED，完整档均可消费，功能结果分别判断 |
| tokenCount0/1/2/8及边界行页 | 核一token一页、公式及四行分页；合法2–8token不能误报页数。超8/4096行/1033页精准拒绝 |
| 声明代替真实行 | required/present声称BEGIN/END/FIRST_ACCEPT但实际缺行，更新SHA/总字节使文件门合法后仍在事件门拒绝。旧t409_good转反例 |
| required类型/字典 | 对象、混合数组、未知kind/required、重复字符串、漏BEGIN/END，不能容错吞掉。缺FIRST_OFFER分别与sourceOfferSeen=true、两相count非0搭配精准拒绝 |
| flag/计数矛盾 | seen=false而事件在场，seen=true而事件缺失，offerCount有沙水但仅一相FIRST_OFFER，enqueued>offer均拒绝；DOWN不能代offer或ACK |
| 页/行篡改 | 缺页/重复引用、SUMMARY与EVENTS置换、同serial行顺序交换、错cursor/next/complete、跨capture/epoch/session/SHA/clock，分别命中目标门。调换后重签fixture哈希但不重排修复serial |
| 首末账与数字 | END账≠terminal、BEGIN账≠initial、只改manifest或page.summary；数字型serial、前导零/超U64、负数/2^32回绕字段、预算/UTF8字节差拒绝 |
| 不可判和CLI | INCOMPLETE或dropped>0即使所有物理字段平衡也非成功退出；空目录/未知revision不SKIP成功。suppressed>0且dropped0的合法有界采样不误拒 |
| 旧档回放 | T306真实T399仍走旧parser，T405缺31分片/超预算/截断继续拒绝；T409不得借旧parser避开新页scope和required校验 |

每个反例写来源、唯一变更、重新计算的fixture字节/hash、期望错误码和实际结果；全部标SYNTHETIC_PROTOCOL，不当真机。复用已有T409真实App/ANGLE mixed/late档作为附加实际格式对照，保持HOST_SCRIPTED_OFFLINE标签。报告原/新checker版本、两个原件manifest/page SHA不变、全修复项映射和CLI退出码。工具非机检查完成即可交付，不等T410剩余真机项目作关闭前置。

## 6. developer与T410的分流边界

**当前没有证据要求developer修生产requiredEvents格式或重签包。** 现有数组/页游标/token页布局可由独立工具正确消费。只有新工具和独立动作原件表明以下生产问题时才另派developer，并保精确复现与source符号。

- `CaptureOfferLocked`已记某相offerCount却未保存该相FIRST_OFFER、`CaptureAcceptedLocked`与token/seen矛盾，且非显式INCOMPLETE/预算拒判。修对应capture写入与临界预留，不移除required。
- `GpuEvidenceCaptureCold.collect`保存了串scope、错字节、缺页/首末账不符却输出COMPLETE，或native `EvidenceCapture::Page`游标/数量/revision不自洽。修唯一生产写者，不后处理篡改归档。
- 真实动作/根接受确实发生却capture没有观察到，需绑定同run/capture/action和原时刻证明后查观测接点。T422的Balanced不能替代这类证据。

T410继续当前T421包上其他可做范围，采真实健康新run/pulse、空场QUANTUM、双源ACK→后继Q→真实token Draw/Swap及六负控，冻结后保全部原页/manifest/预约侧证。修复后的checker可离线消费已有原档，无须为T10误报重抓同一空闲窗。两attempt2所有acceptedPulses为0的事实带入后续问题报告，不能把预约成功当执行健康。未能继续时写具体页面/输入/时钟/原错误与未完成项回architect，不先改GAP或GPU期限。

T410报告分别列ARCHIVE_INTEGRITY、RESERVATION、QUANTUM、SAND/WATER SOURCE/继Q、负控、lease/呈现、30fps和模拟速度。工具待修期间可以继续收证，但手工核验不得代填完整性PASS。新任务不得修改当前已装包或占用T410在途设备窗口。

## 7. T422证据边界补正

T422已交的五预约场景按其实际层次成立。S1确实使用控制线程预约和生产owner循环安全点，S2–S5为生产函数上的合成控制场景，不是所有场景均端到端真实GL循环。harness 的 `progress.accepted>=1 && Balanced()` 证明该时点整数账自洽，S5还核显式end不改变accepted；它们不构成capture-off与capture-on的逐root/逐字节等价实验。

因此T422报告中“capture off、业务前缀、lease、真实GAP、GPU截止和归档由Balanced间接覆盖”收窄为**未由该断言证明**。Balanced既不读取publication generation/lease/release fence，也不核完整manifest/page原字节、物理state/event、首错冻结或系统显示。全0账同样Balanced。不能从它补出完整lease、归档或无观测扰动的PASS。

现有五预约场景不推翻、不重复整卡。若后续需要采集非干扰结论，给独立tester明确追加范围，使用同输入/同脚本时钟capture-off/on的生产路径，逐root核accepted state/event/completion/token、成功前缀和首错，观察Acquire→release fence→Retire代际，deadline前/后和真实GAP按独立场景，归档由新checker独立核。脚本时钟实验仍标HOST_SCRIPTED_OFFLINE，设备时序另归真机；没有证据就标NOT_MEASURED，不把完整补测加作T421开发或T410开工门。

## 8. 执行约束与本卡检查

后续执行prompt必须包含持续完成约定。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先完成其他可做项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其他行列/队列或滴答同名清单。接单原卡已开始、本人行进行中；成功回执、本人行结束待验收；失败/关键缺证原卡写未完成项、复现/原错、证据、已试办法、不能自行继续原因、所需前置与角色，本人行看评论。共享树不回退他人。CPU停止投入，既有oracle仅复用。非机卡明确不使用真机，设备卡真机标签且串行。所有前景背景仅XComponent+shader，禁止Canvas上屏/回退，失败FAULT或明确STALE上一有效完整帧。

本卡已完成两真机档原件/哈希/总字节/实际事件和首末账、原checker重放、两个工具最小反例、生产初始格式与事件字典、分页/状态/CLI缺口及T422断言范围核对。证据JSON可复算，不是完整新checker。工程修复由tester实施，生产问题仅在第6节条件被证明后转developer。本稿选择保留已有生产V1和原档、版本化独立工具，未改生产协议、阈值、安装包或产品结果。
