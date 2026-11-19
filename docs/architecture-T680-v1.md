# T680 · 单沙 raster / packet geometry 续归因 v1

2026-11-19，architect。仅解析已保存原件与核对真实生成源码，不使用真机，不运行产品、测试、生成器或构建。机器证据为 `analysis-T680-evidence-v1.json`，分流合同为 `architecture-T680-contract-v1.json`；静态复核入口 `tools/architecture/analyze_t680.py`。本卡完成工程归因与交接，不等于 T592 产品通过。

**9/7 是一个早期快照中真实的 UI geometry 与 ArkTS packet geometry 差值，不是已证明的 native ack 缺失。唯一已证工程缺口是 FAILED 冷档摘要丢失配对证据；先保留有界关键字段，再按同 scope 的真实失败分流。** 目前不授权 native ack、geometry 推进或算法猜测性修复。

## 1. 三份证据分别能证明什么

| 原件 | 原始身份、阶段与时间 | 结论边界 |
|---|---|---|
| `test/reports/t491/t592b/t540-PREPARING-18.json` | gate session=`s1795089517841-1`；pageGeneration=1 / prepareId=1；owner=null / ownerSerial=0；subphase=DISMISSING；observation=null；gate observationSerial=18 / inputSerial=6；startedMonoMs=284251072.706096；window=649 / generation=1；snapshot controller/session/surface/context=1/1/2/1；run=`p011-run-1795089517855-3` | authority 完整、无 detailTruncated；状态 PREPARING，reason=WAIT_OBSERVATIONS；只有 BEGIN、INPUT、CHANGE 三个事件，**没有 FRAME/FAILED 判定**。文件名的18是 gate serial，不是 raster observationSerial=306 或 didLayout=301。 |
| `test/reports/t491/t592b/t540-FAILED-25.json` | pageGeneration=1 / prepareId=1 / ownerSerial=1；state=FAILED；reason/firstCause=`PREPARE_TIMEOUT:KEYBOARD_VISIBILITY_UNKNOWN`；detailTruncated=true；authority 整段不存在。外层 observation 保留 window=649 / generation=1 / branch=1、layoutGeneration=2、serial=11；POST 首时284251095.69344，最后 SAMPLE=284253209.115158，frameNs=284252159365832 | **session、startedMonoMs、gate observationSerial、完整 owner、context、snapshot/native、FRAME 原因均未保留。** 本地短文件名25不能冒充 payload 中已核实的 gate serial。observation 里的 serial11也不能顶替 gate serial。 |
| `v3b-field-lock.json`、报告 v3b 与旧 fix9 JPEG | field-lock 对 PREPARING 的9/7、1154×1644、didLayout301>300等抄录可核实；“native confirmed/ack lag”是作者解释。旧 JPEG 的 plan generation=9 / geometry=11；新 PREPARING 的 plan generation=7 / geometry=9 | 不能把 generation7误作 geometry7，不能把 geometry9冷档贴到 geometry11截图的失败时刻；JPEG未显示具体native字段。 |

报告称 FAILED 来自“另一次运行”，但原件本身不能证实或否定：短ID、window相同，FAILED最后 SAMPLE 距 PREPARING start 2136.409062ms，时间相容；关键 session/owner/context 仍缺。**登记 NOT_ESTABLISHED，既不强拼同一次，也不把报告措辞当作已证不同一次。** 优先以原名、WS_EVENT t426_prepare 的 name/bytes/SHA、设备进程与加载包收据补配对；时间接近、prepareId=1或同window都不是充分身份。

## 2. 十项比较成立九项，不等于已走到这个门

实际路径 `readiness.authority.snapshot.raster.native`，当前 geometry 来自 `readiness.authority.snapshot.geometry`，不是协议字符串。下表只表示保存数值的比较，**不是运行产品测试 PASS**。

| native 字段 | actual | expected | 静态比较 |
|---|---|---|---|
| controller | 1 | 非零 | 满足 |
| session | 1 | 非零 | 满足 |
| surface | 2 | 非零 | 满足 |
| context | 1 | 非零 | 满足 |
| run | p011-run-1795089517855-3 | 非空 | 满足 |
| geometry | 7 | 当前 geometry=9 | 不等 |
| packetWidth | 1154 | rasterW=1154 | 满足 |
| packetHeight | 1644 | rasterH=1644 | 满足 |
| viewportWidth | 1154 | rasterW=1154 | 满足 |
| viewportHeight | 1644 | rasterH=1644 | 满足 |

该快照更早的门并未全满足：inputs[0].focused=true；keyboardConfiguration=UNKNOWN；**snapshot.keyboardHeight=1099，不只是 allocation 旧值**；avoid[2]为1256×1099非零区。raster.firstDrawSerial=0、frameAnchorNs=NOT_OBSERVED；layoutAcknowledged=0、snapshot.layoutGeneration=0，零值相等仍不满足非零 ack。valid=true、didLayout301>300仅证明其中两个条件。

真实 base predicate 顺序：window/input history → input焦点/编辑 → 固定键盘资格 → 键盘遮挡 → component → raster gate → nativeReady/current geometry。若仅对保存数据做 base 条件检查，首个未满足的是 INPUT_EDITING_OR_FOCUSED；跳过输入条件也先遇键盘资格/遮挡。raster 子谓词先遇 RASTER_WAIT_DRAW_PAIR，之后还有非零layout ack，才轮到十项 native。V4 owner 尚未绑定时 frame 甚至不会进入 base predicate。因此本档的**实际 recorded reason仍是WAIT_OBSERVATIONS，不能把静态候选原因回填成已经执行的失败首因**。

FAILED原件只证键盘可见性超时；不能由其恢复 UNKNOWN 的具体子条件（确认按钮、设备/模式/avoid格式或历史），也不能把 PREPARING 的键盘值搬过去补齐。

## 3. 字段生产链与合法时序

核对的是 T659 真实生成树 `.codex/local-test/t659-delivery-source/.codex/local-test/product/entry/src/main/ets/`，同时核真源模板；各文件SHA及与前次归因的实际源码对比见机器证据。

- `render/P013SingleSandController.ets:64–69` 的 `prepareReadinessCold()`：geometry、packet宽高、run来自 **this.preparation.packet**；surface/context/viewport来自 native.getState；controller来自 controller.session。`render/P013SingleSandPreparation.ets:70` 仅在真实 pack 路径以 `context.geometryEpoch` 生产 packet.geometry。这里没有读取 native geometry ack 的接口。nativeReady=true只验证该 packet 与 native 状态及 lastReceipt 的CURRENT/token/swap，不保证它等于当前 UI geometry9。
- 页 `publishRaster540:477` 先推进 UI geometry、设置当前 layout/captureGeometry并 schedule，**没有同步 pack 或 draw/ack**。`prepareSingleSandFrame:273` 先检查 raster/node geometry，通过后才 pack，再查询/准备真实 EGL surface、检查px尺寸并绘制。因此 UI9与上一 packet7可以是合法的未配对中间态；READY不得放行，但差值2本身不证明漏两次确认，也不证明会一直不收敛。
- `RasterGeometry.ets:54–72` 的 observe 总会复制最新 native evidence；geometry-only 不在 actual change-key，因此不会仅为追赶 packet 再 advance。`snapshot(currentGeometry)`把**当前 geometry与上次观察 packet**装进一个深拷贝，本就能出现9/7。协议名T532_WINDOW_HOST_RASTER_V3、plan.generation=7、当前geometry=9是三个域。
- 页 `readPrepareSnapshot:281–313` 先 refreshRaster，再从实时controller `raster.snapshot(g.geometry)`取快照，同时另读prepareReadinessCold；真实 willDraw:213–230逐回调新读→observe→frame，并核 owner/window。**不能仅凭冷档就宣消费者一直读取 layout.raster 的静态旧副本。** layout.raster用于 draw/capture的缓存与gate中的实时snapshot也不能混称同一对象。
- prepareUpdate:95–110仅在状态身份变化时自动冷导出。PREPARING档通常是开始阶段，并非每FRAME或失败瞬间持续更新。后续是否完成pack、刷新evidence或发生新allocation，现有原件未覆盖；不由静态调用存在推断真机必然执行成功。

分类：早期合法中间态**可解释、尚未观察恢复**；持续发布/pack缺陷**未证**；静态snapshot长期陈旧**未证且有实时读取路径**；采集证据关联**不足且有不同generation/阶段混用**；native ack缺实现**不能从该字段推导**。无需designer改仿真算法。

## 4. 已证工程问题：冷档截断丢失全部配对 authority

真实 `render/PrepareReadinessV4.ets:77–82` 先生成完整 `authority=super.report()`，UTF8超过14336B后只保state/reason/prepareId/pageGeneration/ownerSerial/首末原错等摘要，**整个snapshot、gate session/start/serial和owner均丢弃**。原FAILED档已经触发这条支路，而非采集者查错JSON路径。继续把同一截断文件当作完整配对证据读取，无法补回数值；不再要求tester对undefined填0，也不盲目重复同条件prepare。

工程修订限于**既有冷导出中的有界配对摘要**：不新增产品协议、native接口、日志线程、每帧全量导出或第二准备权威；保留authority原路径及现有revision，新增具名诊断块 `pairingEvidence`（内部revision=`T680_PREPARE_PAIRING_EVIDENCE_V1`），在完整与detailTruncated分支均保留；消费者先识别authority完整与否，再按显式来源解析摘要，不静默拼字段。

developer交接边界：

1. 真源 `test/t426/PrepareReadinessV4.ets::report` 从**同次super.report()的已生成数据及同次owner副本**构造摘要；若使用小型纯值DTO/helper，须由现有 `test/t574/t574_prepare_v4_overlay.cjs`/`t659_owner_glue.cjs` 和 `tools/singlesand/prepare_owner_build.py` 真正带入T598固定基线生成树，不能只改生成缓存或工作区另一页。缺少生产时刻字段不得自行造时间/序号。
2. 摘要最小字段：gate.session/pageGeneration/prepareId/observationSerial/inputSerial/startedMonoMs、state/reason、ownerSerial与完整十字段owner或显式null；snapshot windowId/generation、controller/session/surface/context/run、geometry/nativeGeometry/nativeReady、viewport、layoutGeneration；raster revision/generation/geometry/W/H/valid/reason、didLayout/requestedAt/layoutAcknowledged/firstDraw/frameAnchor；raster.native十项；三输入created/editing/focused、keyboardConfiguration/deviceType/windowMode/avoidUnit/四个avoid rect/keyboardHeight/animation及keyboardAreaSeen/sessionEverEdited；diagnosticOverflow；最后一个**已有**同prepareId/pageGeneration FRAME的serial/frame/now/reason（无记录明确NOT_EVALUATED）。这正好覆盖门顺序和本轮缺失，不复制node raw/ancestry、全snapshot事件或图像。
3. 使用已有authority数据/最多256个已有事件提取last FRAME，不重新执行predicate，不改 reason/firstCause，也不把最新snapshot的值冒充last FRAME的值；标记两者observationSerial是否一致，跨serial只作候选观察。摘要不声称包含 native ack；十项geometry源标识为ArkTS packet，viewport源为native.getState。
4. 摘要建议UTF8≤4096B，仍守readiness14336B、外层16384B硬上限；**实际序列化后计数**，不能按字符估算。不回归首末rawError保留；字符串有界且逐字段标明截断/缺失，截断identity不得用于精确配对。若摘要与原错合并仍溢出，输出显式INCOMPLETE/PREPARE_COLD_UTF8_LIMIT及字段缺失，不伪称完整或放宽上限。外层observation即使被丢弃，摘要中仍保window与gate scope；branch仅从本次owner取得，无owner标为未知。
5. 不在report执行refreshRaster、bindOwner、advance、pack、frame、gate.check、reserve或任何native调用。导出必须无授权/状态副作用；不为观测修改三秒期限、两帧permit、owner/ticket或RESERVED后才consume的既有合同。

诊断接口保持 `PrepareReadinessV4.report():string`。内部可采用 `buildPreparePairingEvidence(authority:PrepareAuthorityReport, owner:PrepareAttemptOwner|null, ownerSerial:number):PreparePairingEvidence` 纯函数，具体DTO须具名、无方法、无Record/动态any授权调用。输出具名字段固定为：`revision/status`；`gate`（session/pageGeneration/prepareId/observationSerial/inputSerial/startedMonoMs/state/reason/diagnosticOverflow）；`ownerSerial/owner`；`snapshot`（上述window/native/geometry/layout字段）；`raster`（上述raster标量和native十项）；`inputs`；`keyboard`（configuration/deviceType/windowMode/avoidUnit/avoid/height/animation/areaSeen/sessionEverEdited）；`lastFrame`（status/observationSerial/frame/monoMs/reason/snapshotSerialMatches）；`missingFields/truncatedFields`；`sources`（geometry=ARKTS_PACKET、viewport=NATIVE_GET_STATE）。诊断status为COMPLETE或INCOMPLETE，不作门禁PASS标志；lastFrame无已有记录时status=NOT_EVALUATED，其余值为null；未知/截断数值同样null并列出路径，不用0代替。实际合法的0（例如firstDrawSerial=0）保持0，与未知区分。键盘四rect与输入三项保持原顺序。owner=null在开始阶段是合法观察值，不自动标产品身份缺陷。

需要私有base状态时优先用已生成authority的具名纯值DTO；不得为摘要重建第二套predicate或票据权威。原firstRawError/lastRawError仍在原路径，新增摘要字段不顶替raw异常；旧读取方仍能解析原revision，T671按新增诊断revision显式识别。所有未知字段/截断身份均不具备same-scope join资格。

这是真实**诊断工程修订**，可交developer按本合同实施及编译；不是承认产品缺native ack，也不增加另卡设备/测试为开发关闭门。若现有设备仍有完整同scope原件，直接复用，不等这项形式关闭。

## 5. 现有任务的最短续证与分流

**T671 tester离线**继续既有parser/签包任务，不重复建脚本：先核各原件字节SHA、原完整文件名与WS_EVENT收据、实际包身份；输出每字段value/source/status（PRESENT、TRUNCATED、NOT_CAPTURED、NOT_EVALUATED），保留gate serial / raster serial / frame-observation serial三个域。对本次两档只登记上表，不将9/7填作FAILED首因。新摘要实现后支持其显式revision及大小/缺失判定；测试脚本/夹具与host验证仍由tester负责。

**T592 effector真机串行**先取现有原名冷档/WS_EVENT及同scope的完整snapshot；不必重复已经有文件的prepare。准确目标是旧 geometry11 的 RASTER_NATIVE_MISMATCH失败所在attempt，或明确标注新的包/attempt。新attempt须真实确认固定系统键盘并观察失焦/收起后的实际回调，不能写fixed=true/keyboardHeight=0绕门。文件已丢失则记录NOT_CAPTURED和真实路径/读取错误；旧FAILED仅有截断摘要时承认不可恢复，先用已有完整档或上述有界摘要产物，不反复运行同条件空证。设备窗口等待不是实阻塞；摘要缺实现/包未齐是相应采证的真实前置。

需要的最小同scope序列：开始/发布时的gate与raster身份 → 首次满足输入/键盘/layout/draw的真实snapshot及已有FRAME reason/time → 下一次实际FRAME/snapshot及最终失败或READY。优先复用已有自动冷档/冷观测出口和最后FRAME；只有已验证配对且仍缺那个时刻，再由architect指定那个有界只读点，不泛加日志。没有记录过的pack生产时间或publication serial明确NOT_CAPTURED，不能从render generation差值换算。已有报告/native draw失败码与receipt若可取同时保存，不以nativeReady代替新geometry成功绘制。

| 续证结果 | 下一步 |
|---|---|
| 输入/键盘未通过，或FAILED仍键盘域 | 该attempt按具体首门处理；配置/采集问题由tester/effector纠正，产品回调/状态问题保原错回architect。不能转成raster ack修复。 |
| 同scope UI先发布9、packet暂7，后真实pack/draw配对9且门收敛 | 合法暂态；不改geometry推进，不据此修native ack。继续T592原HOLD/严格PNG矩阵。 |
| 前门全满足、同scope多个真实frame到期限仍packet旧代 | 对照真实prepareSingleSandFrame首失败/pack/draw/receipt与native身份；architect只对已定位的生产或调度分支修文档再给developer，不强写新geometry。 |
| 同scope packet与native viewport均新代，gate raster.native仍旧 | 核观察序号与读取顺序；证据成立后仅修采样/缓存更新，不因evidence-only刷新advance或进入allocation key。 |
| geometry连续增长 | 要给真实allocation/actual key差异与配置事件；区分真实布局变化和自激反馈，单个9/7或数字差2不证明循环。 |
| 仍截断/无同scope字段 | 用本节精确缺失状态和第四节最小摘要；不冒PASS、不直接派native/designer修复。 |

现阶段T592真实HOLD与严格PNG仍NOT_REACHED，原JPEG不能转码冒严格PNG。本architect交付不负责修改T592/T671状态、创建/关闭manager卡或整表更新。

## 6. 执行边界与必要检查

本卡必要检查：原件JSON解析与原字节SHA；十项actual/expected完整；前门/owner/截断与三个序号域；实际源与真源模板匹配；静态发布→pack→读取链；机器合同与文档一致；只暂存本人交付。没有产品测试/设备PASS声明。

后续prompt须包含：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅执行者本人状态有限例外），不得改其它行列/队列及滴答同名清单。**要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。** 局部受阻先完成其它可做项；失败在原卡保原错、证据、未完成项、尝试、不能自行继续的原因、输入与接手角色并本人行看评论；成功结束待验收。developer仅实施+编译；tester离线，effector承担全部真机且串行，不增manager验收门。

十九目标；目标10暂停，CPU停止投入，新增仿真GPU驻留优先。水线及后续优先13→14→12→15→16→17→18→19，真实技术前置另判。全部前景/背景仅XComponent+shader，失败FAULT或明确STALE前一有效帧，无Canvas/软件GL产品回退；不改endpoint协议/协议名/generation含义，不把native.geometry放回allocation/actual key，不放宽尺寸容差/3000ms/READY/预约/票据门。
