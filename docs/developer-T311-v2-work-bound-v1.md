# T311 CPU V2 累计复杂度修订与未闭合检查

2026-10-30；developer；不使用真机。消费 T308 工程；GPU M1不改、不阻断。**实现、运行自测和两工程编译/签包已交；旧零分配静态门仍未闭合，本卡状态看评论，交architect首分流。** 不以代码/软件测试宣告T285 watchdog已消除或设备PASS。

## 已实施

- workspace首次attempt入口封闭冷注册，B≤512；原M2实际B=399，N各档一致。绑定表为私有冷注册状态，关闭后追加注册明确拒绝，不用ArkTS不支持的Object.freeze。入口继续完整workspace/field长度、身份与别名校验；保存该epoch可信的8个field buffer。每patch仍查真实field引用、byteLength、fieldBinding和epoch；容器/工作缓冲的可变引用、detach、lease token/state/request校验全部保留。移掉的是已证明互不相交的field对冷B表的重复membership扫描，不缓存整套storageValid结果。
- M2 sink容量预读只需两项列质量。将原transfer的**完整原顺序preflight**抽为validateHotTransfer，仍先检查patch忙、输出shape、canonical引用和13×13别名；随后直接读canonical H/L列缓存，不再为了这两个标量复制整池。prepare/publish/audit的拷贝、回滚快照、原阶段顺序与原错误码保留。没有跨host guard、budget回调复用候选，也没有把audit snapshot改为canonical alias view。
- 对storage、field validation、prepare、lease、read/restore transfer、snapshot copy、audit、sort、budget audit、derived flush、collect、apply、sink、trace sync、容量预读、retry、rollback/finish记固定计数、累计/最大inclusive耗时、真实成功复制bytes、B/R/N/live/run/step/attempt/phase。20个stage；current3×20 F64及32×80 F64首存行，typed总20960B；溢出/时钟异常显式，最新current摘要仍可冷取。GC计数NOT_AVAILABLE。
- 热阶段无JSON/日志/格式化；Date.now采样仅测量，绝不作为物理dt。它是粗粒度wall ms，负差/非法差计clockInvalid，不能冒STEADY_NS。显式冷timer每秒至多一条≤1800字符、每workspace最多32条/57600字符；整环只显式cold export。CPU stage嵌套inclusive，不把各列简单求和当端到端成本。
- T284/T285续证版本由test/t311/generate.cjs沿T276→T284实际生成链重建，调用同一生产物理代码；50ms诊断timer调用有界1Hz阶段出口，不放进frame callback。冷采另有signal/select/queued/emitted/completed/overBudget六个固定scalar，原T284状态/预算/延后排队、INCOMPLETE规则保持。

## 整链工作界与保留的下界

令C=128、N=容量、L=live、B≤512、R=本attempt请求数、P=prepare尝试数。两种retry按kind互斥，每行至多一次，故本sink P≤2R；外层planner重新attempt须另计ΣR、ΣP，不能只报一次成功子步。

| 阶段 | 修订后界/调用数 | 未跳过的工作 |
|---|---|---|
| entry/field membership | 每attempt完整O(B+8B+28)；bound阶段每次8项真实引用/长度 | workspace可变引用和B个byteLength仍逐lease检查，O(BR)不能说全变O(1) |
| capacity pre-read | 每行一次原preflight O(13²)，两缓存读取O(1) | alias/detach/忙码与原次序；省一全池transfer |
| prepare/lease | prepare P、成功行两lease；原token/ordinal身份 | 每prepare before/候选快照与edit/merge；失败重试仍计 |
| full pool movement | 无retry成功夹具transfer=3R+5，snapshot copy=2R；原为4R+5及2R | 每次payload V(N)=55N+16C；失败按实际已发生的copy计 |
| per-row audit | R次；apply-tail与finish另2次 | 每次O(N+C)+ID heapsort O(L log L)、budget；保留所有原守恒/库存/容量/重复ID/有限值 |
| preview/collect | 原双collect及比较 | 每轮扫描/排序/预测/merge index依真实live/请求；不删第二跑或偷用旧plan |
| derived/budget/trace | 原调用点、次数可由stage与既有derived/trace counters对齐 | 不把flush跨copy/guard移动；budget闭合与事务同步均保留 |
| 冷采 | 原4锚点/全run1MiB等T284预算、固定6计数 | 未触发、排队、完成、超预算分别记录；无锚点不冒成功 |

真实M2多行夹具五档N/R：8/1、8/6、64/16、192/32、512/64，提交/事件/粒子结果逐位相同。N512/R64：transfer261→197，少64次/1,933,312B；bound membership调用1552→0（entry仍8次），避免重复619248次B身份比较；逐行audit依旧66次。复制总payload在此夹具从(6R+5)V降至(5R+5)V。N512/R2048上沿算术为371346944→309480960B，**这是容量界，不是T285实测请求量/流量**；retry/失败/merge/field复制另计。

完整审计仍有Ω(R(N+C))读取、上界O(R(N log N+B+C+budget+derived))，双preview、merge M·lookup、外层attempt累计另加。因此本版不是次线性/整步常数化，也不能离线保证6秒watchdog消失。要把逐行全池审计安全合并为增量证书，必须另定不变量证书、可变callback/派生缓存/重放与原错误顺序的设计；不能在本卡自行跳audit、改dt/阈值或把预测成本从分母删掉。若T312仍超预算，请architect据阶段计量决定工程/设计接力。

## 检查与唯一未闭合门

运行自测97项通过：新增authority/绑定/时钟行9、多R/N及失败rollback7、原hot sink17、M2 13、M2 transactions14、gesture18、T284 cold15、reassembly4，共97（以delivery.json逐项计数为准）。全部不使用真机，原codec/ROI设备证据不冒充。N512/R64夹具为真实M2 sink、预分配plan，host仅明确限定SPAWN不使用budget/guard；生产WaterSandField另五档整步对照覆盖。新旧对照基线为本轮开工前完整ets目录，源hash在证据清单。

**静态门未通过：**既有run_t168_rev2_effect_units.cjs本轮初版输出17 PASS/16 FAIL；开工前源码独立OFF_ALL审计已经NOT_ZERO_ALLOCATION/146，本轮当前为NOT_ZERO_ALLOCATION/164。两个版本的显式allocationSites集合没有新增；新增的是Date.now及receipt/调用上下文未解析证明缺口，不能据此声称实际已分配，也不能写ZERO_ALLOCATION通过。已尝试移除热JSON/回调时钟、移除不支持的freeze、固定trustedField冷分配；运行与编译均通过，但旧模型仍不闭合。没有修改静态工具/测试断言或增加无依据白名单。原始输出、前后层表、差异保存在test/reports/t311。

请求architect首分流：审查新增计时原语和静态模型缺口，明确是否工程收口或派tester/工具模型补证；在其裁定前不授权自己将164项改记PASS。未完成的是形式静态门，不是真机等待；本卡本人行看评论，提交可复核实现/签包不伪记产品通过。

生产根工程和隔离T284续证工程均hvigor PackageHap成功、签名验签通过；路径与SHA以test/reports/t311/delivery.json为准。未安装设备；背景/前景始终原XComponent+shader，无Canvas回退。
