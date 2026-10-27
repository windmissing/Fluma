# T181：T177 跨run失败混入勘误与首错补证工程 V1

2026-10-27；architect；原卡 `6ae09174e4b0ab67700f8a2f`。不使用真机、不重跑测试、不修改业务/测试。消费T177 `267f519`正式报告、A/B/B2/B3 dump、实际run-console.log、执行脚本、DEMO/page/传感器代码。**B3“新冷启318ms内12次失败后自行恢复”已被原身份反证；旧run首个失败原码仍缺，本卡部分交付看评论。**

## 1. 关键分流：12条属于旧进程，不是新冷启失败

| 原文件/位置 | PID / run | 原始观察与结论 |
| --- | --- | --- |
| A-static.txt:121起 | 21223 / p011-run-1793100180197-3 | 19:23:00.211新段开始；后续B1继续同run |
| B2-handheld.txt:2至3099 | 21223 / 同上 | commit一直2890，重复page-physics-failed **1593条**；4条input.START均FAILED，没有dry.receipt；实际首错发生于更早未归档窗口 |
| B3-handheld.txt:2至29 | **21223 / 同上旧run** | 19:29:26.337–26.657，单调时间2493303095.862006–2493303413.143020，共12条page-physics-failed，间隔317.281014ms；commit仍2890，frame仍1450 |
| B3-handheld.txt:125起 | **24096 / p011-run-1793100569325-3** | 19:29:29.341新run从commit2/frame1/sequence1启动；已采有效段内page-physics-failed=0、input.START 5/5 ACCEPTED、dry.receipt15笔、admitted总量11.209761119842529 |

旧段最后失败比新run首tick早约2.684秒，console 19:29:24执行cold restart、19:29:36记录ready；先清hilog再force-stop会把停进程前的旧日志重新写入缓冲。新run的成功不能覆盖旧run失败，也不存在“同一个failed对象自行恢复”的证据。B3最早旧段dropped=1748，缺原始首错并非无故障；12条是重复停止通知，不是12次独立物理step失败。

源码 `P051EmptySandDemo.tick` 在field.step返回false后设置sticky `failed=true`；后续tick在调用step前直接return false，page每帧又调用stop('page-physics-failed')。failed只在reset初始化中清false；force-stop重启创建新model/run。故原日志跨PID结果与该状态机一致。不能把普通aa start当reset，也不能因同文本把旧故障直接归T146 FIELD_COLUMN、shake冷启敏感或T129双场卡死。

B1零位移swipe没有START/receipt只能说明本执行器未观察到实际按住注入，不代表物理入沙失效。B2重启Activity而不重置run用于新姿态取证无效，**但其中旧run已失败仍是真实观察**；不能把未找到初始失败全涂成执行器问题。B1已采尾部19:24:00还成功，B2首采19:26:57已停2890，原首错发生区间没有完整原dump，时间/原因不可猜。

## 2. 原码、trace及身份：实际缺什么

四份dump均无 `first_code5_trace` 分片，也没有包含数值getLastError/失败stage/attempt的原始行。`page-physics-failed`是page层通知，不携带错误数字；不能由它还原code5/Hot2/M2 3或AUDIT。page的emitFirstCode5仅在getLastError===5时导出，且受一次锁存/旧trace截断影响；没有trace既不能证明不是code5，也不能证明一定是code5。

最小可分事实：旧run commit2890停止，B2触摸拒FAILED，新冷启run推进到commit1047且有准入。首错的物理before/after、raw code、attempt/preview、输入与field原bits都**NOT_CAPTURED**。相关包声明沿s150 f28ec28e；installTime相同不能独立证明包SHA/source。若把1792130688871解释为Unix毫秒，UTC为2026-10-16 06:04:48.871、北京时间10-16 14:04:48.871，**不是报告写的10-27 16:31**；若系统该字段另有语义需保留原返回/单位，不自改时间换算迎合结论。

本轮明确读取了被默认文件列表忽略的 `test/artifacts/t177-real-handheld-20261027/run-console.log`。另T95同名console实际存在，本轮已核其R1/R2末PID与动作时刻、R4 CLOSED NOT FOUND / R1 store path not found，并补正T176旧“未见console”的描述；该更正不补出T176缺失的材料bits/内因。

## 3. 手持方向证据保留范围与符号勘误

新PID24096的8条60帧摘要（文件行403/585/786/950/1126/1306/1555/1885）中target→effective：35.365→30、-30.046→-30、-34.226→-30、-16.445→-16.445、41.785→30、-4.640→-4.640、50.737→30、57.009→30。它们支持传感器目标变化、物理已接受角跟随与既有±30钳制；原shakeInput摘要均0，但这些稀疏摘要不能排除两采样间shake。

adapter使用平滑后的x/y：`atan2(-filteredX,filteredY)`，不是raw x直接作角。报告“seg11–13 x≈-5.3对应负左倾，seg18–20 x≈+7.4对应正右倾”与该符号规则不一致；新段负target附近raw x约+4.6/+6.0，正target附近约-5.05/-6.11/-7.26，符合代码的负号。不得按截图/console中的LEFT/RIGHT文字重标原传感器轴。物理低端方向还需要设备屏幕坐标/姿态与渲染坐标一致性证据，不能以此直接认定“沙粒向低端流动”产品PASS。

console三姿态动作窗口和有效tilt变化大体可关联，但文件未包含每个用户动作的精确设备时间戳；没有逐传感器过滤全状态或三轴z完整流用于精确重放。保留用户实际手持、五次START和15笔准入进展；目视流向/跟手/沉积仍NOT PASS，材质故障沿T176→T178/T179/T180独立处理。不能调T50钳制、重力强度、guard/预算/守恒去迎合“跟手”。

## 4. 最小工程与补证接力

### 4.1 已确认的执行器工程问题，先给采集方

effector负责 `tools/t177_*` 后续版本和证据包，不改既有raw。冷启顺序应先冻结旧PID/run并归档旧尾/退出，再force-stop；记录命令stdout/stderr/退出码；启动新包后立即读新PID、model/run/config/build READY。建立新PID+run过滤窗，**只把归属已证明的新进程事件计入冷启失败率**。hilog reset不是归属证据。普通aa start未产生新run则报REUSED_RUN；没有START不能假定长按有效；旧failed状态直接报PREEXISTING_FAILED，不反复运行无效触摸宽矩阵。

同一进程newrun也要按run分界，不仅靠PID；保留两段物理commit与diagnostic sequence。更正T177派生报告的“B3瞬态恢复”与installTime论证，原观察/交付不删除，不重开manager验收门。按原报告当时真实用户授权记录手持；不把脚本LEFT/RIGHT当用户姿态测量值。

### 4.2 首错误原码的工程补齐（developer，不使用真机）

模块所有权：P051EmptySandDemo模型失败锁存、EmptySandDemo页面小摘要导出及现有P051DiagnosticBuffer旁路。只读锁存**首次field.step实际失败**，新增 `P051_FIRST_PHYSICS_FAILURE_V2`，不改变物理/重试/reset策略：run/modelGeneration、before/after commit、dt/单位、raw field code、stage/Hot/M2码、attempt/substep/候选身份、source成功前后、已接受θ、input sensor/stale身份、last good publication key；未知值null+NOT_CAPTURED，不以默认0冒实测。

在`failed=true`之前、原返回后清理/下一frame之前存固定摘要；不得为了拍before再次snapshot/updateSandTop、step/audit/rollback。失败帧可借现有精确trace但明确捕获时点/完整性；旧 `beforePool=LAST_PRESENTED` 不是实际step before，不改名冒充。旧emitFirstCode5仍保留，V2覆盖所有实际raw code，重复page stop只累加repeatCount并关联firstFailureId，不覆盖首错也不复制一次“新失败”。固定首错槽+最近8条生命周期摘要，按run/reset清理；旧失败导出前遇reset须保留已冻结旧run身份，不能染到新run。

身份/renderer信息通过诊断sidecar新增，不更改endpoint/particles协议。开发完成编译与离线自测即可交付，不以真机为开发关闭前置；与T178材质首错及T174共享hunk协调、不回退他人。

### 4.3 独立tester与设备最小补证

tester不使用真机：一个非5失败、一个code5失败、sticky连续三次false、reset后新run、进程/日志交错、缺首错/trace截断/导出异常夹具；断言firstFailure只记真实step失败一次、repeat不重跑step、raw code/status未知不填0、新旧run不互相覆盖。注错只在隔离夹具，不放产品自动恢复接口。验证“相同installTime而不同包”不能被身份检查误认同包，原数值边界不变。

先请原执行者提供**已存在**的19:24–19:26旧run首错日志/页面原码/包source manifest（若仍有），无需重跑。确实缺失就写NOT_CAPTURED；不要求追溯构造旧原bits。新诊断到件后设备只做一次冷启静置+一次有界真实姿态/触摸窗口，锁定新PID/run，若首错命中立刻取V2并回T181；未触发诚实记录一次未复现，不跑宽矩阵或伪造同源重现。真机标签、队列串行，GPU前置齐时最高优先、不打断在途run。

若新证据指向真实工程错误，先修本文对应路径再developer；算法问题交designer，用例争议带证据交manager。诊断通道尚未实施的预期RED登记NOT_IMPLEMENTED，不把它冒新物理缺陷。旧真实失败仍未定位，不改成“仅执行器错误已全部解决”。

## 5. 当前结论与任务边界

已明确：B3新冷启瞬态失败/自行恢复是跨进程报告归属错误，路由采集报告修订；旧run停止是另一条真实但缺首错的物理失败，不认同T146/猜shake因果。T181完成这次路径级分流及补证规格，**关键首错原码仍缺，状态看评论**。派生证据 `analysis-T181-run-identity-evidence-v1.json` 保留四份raw/console hash、分PID/run计数/首尾与原位置、摘要角度及包声明边界。

不调整T50参数、不做生产物理修复、不把新run准入成功覆盖旧失败、不宣双场卡死已修。T182需材质修复/独立证据及实际效果前置，等待设备本身不算blocked；本session不改队列。

所有下游prompt：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人状态有限例外），不更新滴答同名清单、不回退共享修改；离线不使用真机，设备卡真机标签和串行前置。所有前景背景仅新XComponent+shader，禁止Canvas上屏/失败回退；故障FAULT或明确STALE上一完整有效新管线帧。manager按交付登记/调度，不新增验收门。
