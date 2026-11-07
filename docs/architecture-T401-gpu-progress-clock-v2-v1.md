# T401 · GPU 独立进度时钟 V2 工程合同

2026-11-07，architect，P5。不使用真机。manager 已在本卡批准 T395 §4 的隔离 V2 和候选 GPU 绝对期限 250ms。本文冻结可实施实验合同。正式产品阈值仍须按设备结果裁决。

## 1. T399 原档与 V1 的边界

核 `test/reports/t399/t399_verdicts.json` 的 36 个 SHA 前缀，按 serial 去重三次累计导出，得到 627 行、两个 TID。epoch/run=3/2 至7/6 各有一个 stage40 root、一个 code1 的 stage41 取消，无 stage36–39、46–48、50 和 Draw/Swap。Offer/Consume 也为零。首个 run 关闭热探针，后四个有 stage43–45、49、51、58，因此本轮已有可直接判读的调用区间。

| epoch/run | 控制 TID18338 的 anchor→PauseBegin ms | GPU TID18645 的第二批 flush ms | flush 返回晚于控制撤权 ms |
|---|---:|---:|---:|
| 3/2 | 39.593 | 热探针关闭，不判调用内分段 | 不判 |
| 4/3 | 45.040 | 53.993281（stage44 serial278） | 9.462 |
| 5/4 | 40.204 | 60.667187（399） | 21.007 |
| 6/5 | 48.736 | 56.420312（513） | 9.627 |
| 7/6 | 48.426 | 53.998438（620） | 6.184 |

stage58 cut3 的前后采样与同次 flush 相符。stage59 的旧 permitRev=8、新=10、剩余约31.56ms 记录换代拒收，不能当超期限拒收反例。旧报告称这些区间无可拆事件，本文补正。没有匹配调度轨迹，flush 墙时内驱动、排队和脱调度分量继续未测。

`app_bridge.cpp::ObserveControlLocked` 由 status、offer 和 GL 检查点调用，但它读的 `controlAnchor` 只随 GL owner 的成功 Advance 更新。`GpuResidentM1.tick` 有页面 FrameCallback，尚无独立 native 心跳。这证明 V1 已让 UI 线程能在 GL 阻塞时撤权，同时仍以旧物理 anchor 判 GAP。T399 不能证明页面心跳真的断了 33ms。V2 必须新增连续性证据，并把 root 期限从 Advance anchor 解开。

## 2. 身份、入口与状态

只在隔离包冷配置启用 `GRS_M1_PROGRESS_CLOCK_V2`，V1 可在新 run 冷选作对照。sourceCommand32、completion256、Q20 物理核、守恒/事务/lease 继续复用。页面显式核 `progressClockRevision`；不支持的组合拒绝进入 V2。

| 入口/对象 | 接口与约束 |
|---|---|
| 页面 session | `beginPageSession()` 返回 native 生成的 U64 session。show 后创建全新 session/FrameCallback 对象，hide/disappear 同步 `endPageSession(session)` 撤权。新 run 保留仍有效的页面 session，但撤销旧 run 的 worker/源/配额。 |
| 页面心跳 | `pagePulse(session,seq,callbackNs)` 仅由当前 active 页的 FrameCallback 入口调用，在 status/源请求和其它工作前执行。session、seq、callbackNs 用现有安全 U64 高低词或十进制桥表示，严格验范围/递增。接收时间由 native steady ns 当场记录。序号重复、倒退、旧 session、缓存批量重放显式拒绝。 |
| native 监督 | 专用监督线程按单调时钟检查最后一次有效页面心跳与 session/前台状态，只短锁控制状态，无 GL。可用2ms检查节奏；这只是唤醒目标，实际调度迟到量记录原值。任何 authorize/accept/swap 门同时即时检查新鲜度，不能依赖监督线程准点。 |
| 只读状态 | `status()` 返回最近 pulse 序号、UI 原时刻、native 接收时刻、实际 gap、许可/源代、队列和时钟账，可检查过期并撤权，但不更新心跳。sensor、触摸、定时器、GPU continuation 和后台回调也都不能续页面心跳。 |
| GPU job 许可 | `WorkPermitV2` 固定完整 run/owner/context/surface/geometry/config/input、control session/许可代、planId、root kind/nonce、绝对起点与期限，沿 T398 `RootIsolation` 的 cut 和短接受事务验证。 |

心跳首次只建立基线。后续先检查旧 native 接收点到当前接收点，以及相邻真实 UI callbackNs 的间隔。任一 >33,333,333ns、时钟倒退或session不匹配先撤权，不得先写新时刻而掩盖断档。UI/steady 原点无需相同，分别以各自相邻差判断，跨域端到端只在建立映射和误差界后比较。若两域任一不能得到可信间隔，停止实验并报告时钟证据失败。每次心跳都需真实新回调，延迟后补发多笔不能把断档切碎。

状态顺序为 `WAIT_WINDOW/PREPARING→WAIT_PAGE_PULSE→READY→SUBMITTING/FENCE_PENDING→ACCEPTED→DRAW_PENDING/CURRENT`。冷初始化期间允许真实页面持续送 pulse，但不生成模拟配额。资源、owner、独立页面心跳和现行 sensor/manual 许可齐后才能 Ready，首个 Ready 后的新 pulse 建模拟计时基线。`down()`、sourceOpen 只依赖这些就绪条件及源控制代，不依赖已有 accepted 或 CURRENT，避免首笔源的循环前置。

## 3. 三个时钟及有界配额

**控制连续性**以真实 pulse 为准。心跳过期、后台、窗口失效、几何/配置/owner 换代和真实控制错误立即短锁撤权、停源、清未发队列，唤醒 worker 排空。真实 GAP 记录 `CLOCK_GAP` 和 UI/native 两域原间隔。需新 session 或显式新 run 再入，不接收补发旧 pulse 自动恢复。传感器新鲜度继续按原250ms sensor合同独立检查，不能由页面pulse覆盖。

**GPU 工作期限**从 worker 把任务从有界队列选出并创建提交意图起算，`deadline=start+250,000,000ns`，加法溢出拒绝。未消耗 nonce 的 BUSY 重试、续批、flush、fence、Poll、map、audit 和 Swap 都不重设。期限等号可通过，`now>deadline` 拒绝。控制撤权即使未到期限也拒绝。SOURCE 和每个 QUANTUM root 各有自己的任务许可，不能把后一个 root 续在前一个的到期许可里。SOURCE FIFO 按原16项上限，另保入队时间，过期源不重放。每个计划最多取一笔有效 source 后推进已授予的量子，禁止不断进源使量子饥饿。GPU BUSY 用有限事件唤醒/Poll等待，不自旋。控制锁不得覆盖 GL 调用，GL owner 返回前已发命令与 pinned 资源按 T398 隔离目标安全排空。

**模拟配额**由 Ready 后连续有效 pulse 的 native 接收差生成。以下全部整数计算，当前固定120Hz不变：

`u=carryBefore+120×deltaNs`，`newQuanta=u div 1,000,000,000`，`carryAfter=u mod 1,000,000,000`。

合法相邻 pulse delta 已经 ≤33,333,333ns，乘加做溢出检查。先累计真实 newQuanta，再将待发队列限制在4个。实验明确采用 T395 §4 的**丢最新超额配额**策略，`keep=min(newQuanta,4-pending)`，余下记 `overloadDroppedQuanta`，carry 保留完整原计算值。不得删旧已计划/在途量子挪出位置，也不得通过假 delta 调原 `Fixed120Clock::Plan`。需严格不丢配额的后续产品策略交 designer/manager 单独裁决。

新增 native 内部 `PlanGrantV2`，含planId、grant时刻、当时carry、冻结角度/输入控制身份、0–4个量子。一份计划在 GL owner 使用，最多一根在途。提交期间新心跳只入有界 pending，不修改活动计划和 root。worker 空闲时原子领取至多4个 pending 形成下一计划，授予数从 pending 移到 activePlan，root 的 carry 按该计划冻结。source-only 可有零个量子；若无源且无量子则不建空 root。按 source ACK 和实际接受的 quantum 分别推进，取消不计成功模拟秒。

在同一 run 的整数账上始终满足：

`generatedQuanta = acceptedQuantumCount + activePlanRemaining + pendingQuanta + overloadDroppedQuanta + revokedDroppedQuanta`。

activePlanRemaining 包含在途那一个量子及该计划尚未执行的量子。SOURCE 不占该账。领取、接受、撤销、超期和reset逐笔互斥线性化，持锁读取同一快照。模拟秒按 acceptedQuantumCount/120 另以整数 tick 归档，carry不足一个量子单列。真实 GAP 清配额时，将 pending和活动未接受量子移到 revokedDroppedQuanta，fractional carry清零并登记，已接受前缀和源终态保留。reset先输出旧账终态再建全新run账。每个 pulse 的原始 delta、carry、生成、过载/撤销丢弃、延迟和旧帧比例保留冷档，以便确认V2是在正确退化，还是达到产品性能。

## 4. 源控制、接受和呈现

将全局生命周期许可与 source boundary 分开。正常 UP/STOP/切源关闭旧 sourceBoundary、丢未发旧源，撤销未接受的旧 SOURCE 任务；已接受的 source ACK 保留。它不撤销与源无关的活动 QUANTUM，也不刷新 GPU 期限或 pulse。新 source generation 必须等原 SOURCE 排空后应用到 owner，旧命令按原 packet 代次拒绝；不能只改CPU标签而让GPU使用新代通过。后台/GAP/窗口等则撤销所有任务。触摸先于Ready时记录原拒因，不能伪入队；Ready后真实新DOWN可以启动首笔SOURCE。

所有切点复用 T398 的检查和 wait/map/audit 后的准备结果。短接受段同时核当前控制新鲜度、任务deadline、plan/nonce、完整owner及输入身份，原审核通过才一并提交bank、事件前缀、publication、accepted、source终态或成功量子。换代、deadline和GAP拒绝保留首码与旧账。持续不返回的GL调用只承诺独立控制可撤权，设备资源由原owner或设备失效路径清理。

配额账的成功计数和activePlanRemaining消耗须在同一个 `RootIsolation.accept` 临界区完成，不能等 `Run::Pump` 返回后再补记，也不能返回后重复消耗。页面status只读这份原子快照；GL线程局部receipt可以在返回后同步，但不作为公开守恒账的第二来源。

Draw/Swap绑定真实已接受的publication slot/generation及完整token。仍检查任务deadline和全部控制身份，Swap前后记录结果。Swap返回后已过期或换代的native成功只计lateSwap，不能宣CURRENT或系统显示验证。旧有效完整帧在等待时标STALE，无可用旧帧显示FAULT/等待状态，前景背景都走XComponent+shader。不得用重复交换充新物理帧或以source-only root充成功量子。

## 5. 代码归属、观测和负控

developer 修改隔离 `GpuResidentM1.ets/GpuM1DemoController.ets` 的session/pulse入口，`app_bridge.cpp` 的监督/配额/许可，M1 `Run` 增 `AdvancePlanV2` 而保V1入口。复用 `RootIsolation` 和 T398 已隔离的接受/事件资源，shader和wire布局沿现有版本。冷状态增加V2版本/250ms配置，旧stage1–59保留语义，新心跳、计划授予、配额账、任务意图、GPU超期和源boundary事件版本化；4096有界环/分页/截断拒判继续适用。周期pulse可按固定频率聚合原始间隔计数/最大值，首错及每次撤权不得丢。读状态不能承担心跳续命。

V2 的所有控制新鲜度读取改看lastPagePulse，GPU worker不写该值。现行loop中以last成功Advance判delta和post-Pump GAP的两处逻辑仅留在V1分支；V2用真实pulse的控制门及意图起算的GPU门替代，不能仅改 `FreezePermitLocked` 的期限。物理配额从新的grant入口领取，不把GPU忙碌形成的长wall再喂旧 `Fixed120Clock::Plan`。

离线需覆盖以下实际入口和竞态。

- 连续16ms真实pulse +5/40/65/200ms GPU任务在实验期限内可接受；250ms恰边界与250ms+1ns分别判定。BUSY/续批不续期，SOURCE和QUANTUM独立意图计时，计划≤4/在途≤1/FIFO≤16。
- GPU卡住时停止真实pulse，定时器/status/sensor/旧FrameCallback持续运行仍须在检测时撤权；GPU晚返回不接受。hide/resume、窗口换代、native接收正常但UI原时刻断档、反向断档、乱序/重复/旧session、积压pulse重放均覆盖。
- 撤权和wait/map/audit→commit之间、source UP/STOP与SOURCE接受、QUANTUM不被正常UP误杀、Swap前后换代/超期，核成功前缀/lease与迟到原码。每个合法root与V1原输入对拍bank/event/completion/publication。
- 快pulse、慢GPU、源竞争、长时间过载及reset用精确整数oracle核上述守恒式，含carry边界和溢出。SOURCE不增加模拟秒，重复CURRENT不增加新帧，丢配额不改变库存。监督线程延迟时接受门仍不能漏掉控制GAP。

编译/非机自测和签包即开发交付。tester在同机对照V1/V2和探针开关，先空场成功quantum，再真实沙水分别准入→成功量子→同token Draw/Swap，报告控制pulse原值、调度/GL分段、GPU期限、丢配额和实际成功模拟秒/墙钟秒。独立制造控制断档、GPU>250ms/无返回、旧代迟到和满队列负控，不能用自发超时替独立注错。250ms是本隔离实验候选，平均服务跟不上120Hz或有效新帧不足时仍登记性能失败；T380收益前置继续按真实结果。
