# T407 · GPU V2 双源缺证、心跳断档与 30fps 工程修订

2026-11-09，architect，P5。不使用真机，不实施业务代码、不重跑测试。依据 T401、T404 开发报告、T405 正式报告与原始归档、当前 bridge/UI 源码作首次归因。本文是 T401 的后续工程修订；未修订部分继续有效。

## 1. 结论与用户补充

用户本轮明确「30帧率就可以，不一定要120帧率」。本合同将**显示与有效新内容帧目标改为 30fps，不再要求每个物理量子都 Draw/Swap 或达到 120fps**。固定物理子步 `1/120s` 与整数配额账保持 T401，30fps 时可以合并显示多个已接受物理子步的结果。降低物理积分频率会改变物理含义，不能由显示目标自动推导。

真实页面心跳仍独立于显示节流。继续使用 `>33,333,333ns` 的真实控制 GAP、单 root 自提交意图起算的绝对 250ms 实验期限、原守恒/事务/身份门。不能把 FrameCallback/pulse 本身节流到 30Hz：精确 `1/30s` 已在边界，整数取整成 33,333,334ns 即超窗，实际抖动更无裕量。保留真实逐回调 pulse，例如设备实际提供的 60Hz 回调；不保证设备调度必定满足，也不允许定时器或补发续命。

T405 已完成部分取证；**尚不能证明 V2 同窗真实成功推进 325/508 个量子，也不能证明 V2 与 V1 同因、非回归，更不能确认 V2 卡死由 GL 封尾造成**。已找到可直接修复的状态/观测工程缺口，唯一下一路由为：developer 按本文修复有界观测、失效幂等和 30fps 绘制调度，完成开发检查及签包后，tester 按第 5 节独立补证，再回 architect 分流。此刻不依据旧调用墙时派 GPU kernel 或 flush 性能重写。

## 2. 包、窗口与证据边界

签包 SHA256 `e72786aa8d621b8b40342639f9cdfcd051f6a26b886e0843c3632f4e1ecac095`，与本地 `T404-complete-signed.hap`、签包记录及 T405 manifest 一致。T405 归档 51 个文件的 SHA 前缀与 verdicts 一致；这只证明归档完整性，不证明每个标签的语义和窗口匹配。

| 原档 | 可支持事实 | 不能支持的推断 |
|---|---|---|
| `l2` 的「下轮 V2」/`PROGRESS_V2_NEXT_RUN` | 冷配置选择的页面回执 | 当前 run 已是 V2；必须核实际 `progressClockRevision` |
| `l14` 显示 physicsCommit `0:325`；`l17` 显示 `0:508` 且状态 STALE | 页面在不同采样点显示这些文字 | 没有配对 native run/session/epoch 快照，不能将其与最后零准入/零呈现拼成一个 V2 窗口 |
| `v2state1.png`、`l4/l5` | 实际为 CPU 单沙数值 V2 页面；截图已目视核对 | 不能当 GPU V2 状态证据 |
| `v2state2.png` | GPU 页面显示 STALE 和「下轮 V2」，缓存 commit 为 `0:1` | 不能替代原生当前版本、成功量子或可见物理内容证据 |
| `tl1-manifest.json` | `INCOMPLETE / PACKAGE_SHA_REQUIRED`，0 页/0 行 | 不是成功导出 |
| `tl2-manifest.json` 的 statusBefore/After | **实际 V1**、epoch47/run16、accepted/physicsCommit/sourceAdmitted 均 0，`clockDeltaNs=48613281`，GAP/NEW_RUN_REQUIRED | 48.6ms 是 V1 的旧 Advance anchor 差，不能作为 V2 页面 pulse 独立负控 |
| 同一 status 中 pageSession4、quotaBalanced=true | 代码对 V1 也追加 V2 字段，且该账全零 | 不是 V2 生效或非零配额正确消费证明；跨 session 的 maxGap 也不是本次首错间隔 |
| `tl2` serial3667919/dropped3663823，缓存前缀1–4096 | 数据截断且首四行属 epoch1/run0；拒判应保留 | 当前 `BridgeTimeline::Add` 满后丢新行，并非保留最近事件的循环覆盖；不能还原晚期 V2 因果 |
| `tl2.files` 列 31 个 NDJSON 分片 | manifest 声称导出4096行、1024页、1543343字节 | 这31个分片均不在本次归档，不能复核4096行；`missing=0` 只属于导出器自己的声明 |

1543343 字节还超过 native 声明的 1048576 字节 JSON export budget。T405 的「重导成功」只能表示结束了导出流程，不能改写 INCOMPLETE 结果。缺失分片若能由 tester 恢复应保留原 SHA，不能重新生成替代旧原档；即使恢复，截断和错窗限制仍在。

T401 已补正 T399：部分第二批 flush 确有 53.99–60.67ms 调用区间，且晚于控制撤权返回。它们是 V1 旧包证据；无匹配调度轨迹不能细分驱动/排队/脱调度，更不能移植为本次 V2 的 UI GAP 原因。T405 没有同窗工具调用起止与双域 pulse 证据，`dumpLayout 导致断档` 当前只列待验证假设。

## 3. 源码首次归因

1. `GpuResidentM1.ets::tick` 先 `model.refresh()`，正常分支 `model.tick()` 又 refresh，形成每回调两次大 JSON 读取/解析，并更新多段响应式文本。pulse 不接受时提前返回，旧 receipt/memory/gravity 文字没有同时更新，状态与计数能来自不同采样。证明显示缺少原子身份，尚未测得这部分是否造成 >33ms 停顿。
2. 同页 V2 失效分支每次回调调用 `model.stop(pulse)`；`GpuM1DemoController::stop` 即使已停止也每次 `setSource(-1)`。`app_bridge.cpp::SetSourceLocked` 每次推进 boundary、清未发源并写 SourceBoundary。闭会话的 `PagePulseLocked` 每次失败还会写 PagePulse fault，尽管 `ClockGapLocked` 有阶段幂等保护。这是可确定的重复工作/诊断噪声路径，但没有完整事件流，不能将 3663823 条丢弃全归于它。
3. 4096 行日志只保留最早前缀，无显式 run 采集窗口；每 root 详细 GL 事件密集，长会话很快失去首错/晚期事件。32 个 token receipt 同样有界，不能覆盖长时间每帧记录。现行「每16 pulse聚合」没有解决生命周期日志、逐root密度和终态保留问题。
4. 实际 V2 已将旧 Advance/post-Pump GAP 门隔离到 V1，配额接受在短接受事务内；不能因 V1 accepted=0 断言 V2 分离时钟无效。T404 脚本心跳/ANGLE 65ms 成功为 SOURCE，acceptedQuantumCount=0，也不能充真机 QUANTUM/PRESENT 证明。
5. T405 未具备双源首次失败的原子 down/permission/bounds 记录。可确认 `down()` 必须 Ready；不能断言每个触摸都发生在不 Ready 的窗口。诊断面板展开占据视口，命中标签本身不足以核实际 XComponent 的可用位置/面积和源坐标。

归类：已确定的是**工程观测/状态路径缺陷**，根因产品故障仍缺同窗证据；当前没有依据改物理设计、放宽时限或判测试用例不合理。双源/负控/30fps 均未获产品 PASS。

## 4. developer 唯一实施合同

不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部分支受阻先完成其它可做部分。不得修改后续工作清单，除本人任务行状态有限例外；不得更新滴答同名清单。前景背景只用新 XComponent+shader，禁止 Canvas 上屏或失败回退。

### 4.1 UI 与失效幂等

- 文件：`GpuResidentM1.ets`、`physics/GpuM1DemoController.ets`、`gpu_resident/app_bridge.cpp` 与最小关联状态类型。每个真实 FrameCallback 先送 pulse；不要用 status、sensor、renderDue 或旧 callback 更新 pulse 时间。合并同回调重复 status 读取，snapshot 显式携带当前 revision/run/epoch/session/sampleSerial；页面按同一个 snapshot 更新计数/许可。失效分支统一显示旧计数身份及 STALE，不能将缓存显示成当前成功。
- `stop` 与 native 关源在**相同 source boundary 已关闭且无待撤旧源**时幂等；首次 UP/STOP/故障/换代必须完成撤源，不能仅凭 JS pointer=-1 跳过 native 操作。对旧会话重复拒绝计数可聚合；仅首次 fault/状态转换写关键事件。不能吞掉另一个 run/session/源代的新首错，也不能自动 reopen 闭会话。
- 显示文本最多按30fps刷新，展开诊断低频刷新；输入控制仍按真实回调和现有 no-catchup 时间规则运行。native 最终许可检查不能依赖缓存 UI 就绪状态。触摸 down 以一次有界诊断记录其 session/run、局部坐标、实际 viewport、Ready/permission/source代、接受/拒因，避免只有无因的 false。

### 4.2 绘制按 30fps 尝试，物理账独立

- 首次合法 publication 可立即呈现；后续由 GL owner 使用单调时钟的30Hz节奏检查是否到绘制时点。未到时点的 accepted root 正常计入 SOURCE/QUANTUM、守恒和配额账，**只跳过 Draw/Swap**；到时点时绘制该时刻最新已接受的 publication。首次实现不新增延迟 render 队列/线程，不保留未 pin 的 publication 指针，不在后续 root 许可下重画旧 root。
- 仍用该 accepted root 的原始 permit/nonce、publication slot/generation、绝对deadline、owner/input及生命周期门检查 Draw/Swap；不为绘制重新起250ms，不以尚未接受结果覆盖 latest，不在跳绘制时释放仍在用的 lease。若当次到期/换代，按现有规则拒绝；下一次有合法新结果再尝试。
- 每次绘制只在合法新结果到达时尝试，因此30fps是目标，**不是节流器保证**。GPU服务慢时不能重复交换旧token填数。记录 `renderSkippedByCadence`、成功不同物理commit的新帧数、source-only帧数、STALE时长与实际Swap间隔；native Swap成功与系统实际显示分开报告。低频绘制省去的wall时间需设备测量，不能承诺修复40–60ms单量子服务时间。
- 原子接受、物理120Hz生成/消费及 pending≤4/丢最新策略保持 T401。报告模拟秒/墙钟秒、过载/撤销丢量，不能将30fps显示通过当实时模拟速度通过。此实现修订仅作用隔离V2；V1对照保留原语义并报告其绘制配置。

### 4.3 显式有界采集窗口

2026-11-09 T417 修订。采集入口改为 [有界预约与 owner 安全点消费](architecture-T417-gpu-capture-reservation-v1.md)。T409 同步 arm 和 `CAPTURE_BUSY` 语义保留于下文作为历史合同。新签包按 T417 处理请求、时限与身份。实际窗口、原时钟与配额账、证据预算和冷档校验沿本节。

- 在现有冷诊断接口旁新增隔离 `beginEvidenceCapture(configJson:string):string`、`endEvidenceCapture(captureIdDecimal:string):string`、`evidenceCapturePage(captureIdDecimal:string,cursor:number):string`，不改变 sourceCommand32/completion256。开始配置字段固定为 `packageSha256`、`expectedRevision`、`epoch`、`runLo/runHi`、`pageSession`、`durationNs`、`maxRows`、`maxBytes`；SHA为64位十六进制，U64用严格十进制字符串，run词为U32，duration限2–5秒、maxRows≤4096、maxBytes≤1048576。返回JSON的 `state` 为 `ARMED/FROZEN/REJECTED`，含native生成captureId、实际身份及reason；活动版本/身份与期望不符拒绝arm，隔离包的V1/V2均可采集且不改其控制语义。只在worker无活动root且接受/呈现无在途的安全冷边界开启，等候不持控制锁；忙则返回 `REJECTED/CAPTURE_BUSY`，不能阻塞心跳。armed期间不重置采集，最早到期/首错/显式end冻结；重复end幂等，旧id拒绝。page只读已冻结窗口，cursor按现行有界分页规则验范围，返回next/complete和相同身份。禁止导出隐式清环、热路径写盘/JSON/无界分配。
- 窗口数据全部绑定 captureId、实际activeRevision、run/epoch/session、native起止、UI时钟域及TID。旧session全局maxGap单列；窗口内重置的是诊断统计，不能重置物理配额或模拟时间。配额首末/终态必须是锁内原子快照，reset先冻结旧账。
- 固定预算内保留「开始/Ready/第一次源尝试/首次接受/首次失败/撤权/结束」关键事件与终态，单独预留容量；周期pulse原间隔做固定直方图/计数/最大值，首错保存前后seq、两域原时间、实际gap、deadline、许可身份和job/root。详细GL追踪限定前1–2个指定root，其余固定计数/总时/最大时；token样本限定≤8组且保留沙、水及对应QUANTUM/PRESENT完整token，逐帧只累积计数。不能以静默采样掩盖 required 事件缺失。
- 本轮继续按总冷档≤1MiB预算设计，事件及manifest预算一起预估；超出立即标INCOMPLETE而非超预算继续。关键事件溢出、丢失、缺分片/不匹配SHA一律拒判；有意未采样明细与实际 dropped 必须不同字段。不靠继续扩4096容量解决长期问题。
- 冻结后导出全部manifest引用分片和SHA、终态、token样本到同一报告目录；离线检查实际文件存在/字节和/hash/序号/身份/required事件。前台操作、工具和导出各自起止时间单列；没有可比较时钟映射则不作跨域绝对时刻相减。

开发必要检查限编译和非机自测：重复stop/闭session不重复推进边界、不丢首次撤权；同窗状态一致；原120Hz整数账/源与量子身份回归；30Hz跳绘制不跳接受、到期/换代/lease不漏门；满预算及缺分片失败；250ms等号/超1ns、真实双域GAP、旧代拒收保持。交付开发报告、原始结果及新签包SHA；不以另卡真机作为开发结束前置。若冷采集无法安全插入，应给具体接口/竞态证据回 architect，不删减首错证据约束冒充完成。

## 5. tester 后续精确补证任务书

**真实前置是第4节开发交付及配对签包，不是已有 T404 包再盲跑。** 真机标签，设备串行；先离线核签包、manifest分片检查器和窗口配置。执行要求与清单/渲染边界沿第4节；功能、性能、设备负控分开判定。所有补证完成后回 architect；失败保留原始错误和完整未行使项，不默认转 designer。

1. 同机 V1/V2 分离进程或明确全新run/capture，核actual revision及完整身份。每个健康窗口预先设置采集、手动重力、关诊断面板，记录viewport；先空场成功QUANTUM，再沙、水分别一次真实DOWN→Offer/Consume→SOURCE ACK→后继QUANTUM→对应Draw/Swap完整token链。每窗口先限2–5秒，首错即冻结，成功按预算冻结。分开窗口不能冒称单一连续run；若卡要求同run双源，则另做有预算的同run窗口。
2. 健康窗口内不运行dumpLayout、status轮询工具或导出；由已arm的native固定统计与关键事件取证。结束冻结后才抓屏/导出。要验证工具扰动另开匹配配置窗口，记录工具起止及UI/native间隔；没有事件相关性不判dumpLayout因果。Screenshot要核真实页面与同token身份，诊断面板遮挡另算可见性失败，不能计新内容帧。
3. 独立控制GAP：从健康V2会话有意停止真实pulse，保持status/sensor等其它入口运行，核真实两域间隔、首次撤权、配额清账与新session/显式newrun恢复。最后V1的48.6ms不能替代此项。
4. GPU绝对超期：健康pulse持续，先空场非零配额的QUANTUM即可建立任务；用隔离已有cut或最小专用注错保持同nonce跨250ms，验证不续期、晚返回不接受。无需先成功SOURCE；SOURCE专用超期另需其真实准入。若缺可在设备触发的注错入口，报告具体缺入口回architect，不能用自发GAP替代。注入点/等待不得持控制锁阻断pulse。
5. 分清满队列：pending4量子过载可在空场健康pulse+慢worker触发，核丢最新与整数账；SOURCE FIFO16满另需Ready和有效source代，不可将两者混用。旧代迟到需先有真实在途root再显式换run/owner/window等指定身份，核旧任务排空/拒收与新代隔离；不以空队列换代充迟到拒收。
6. lateSwap 必须先有合法accepted publication、可到Draw/Swap的许可，再在Swap前后指定cut注入超期/换代，区分Swap调用前拒绝、native成功但晚返回、系统实际显示。没有到达该cut就记 NOT_EXERCISED，不能借source准入失败笼统声明所有负控均不可行。
7. 30fps性能另开预算允许的健康持续窗口：固定墙钟区间内统计系统实际显示的不同物理commit有效新内容帧/间隔、nativeSwap、重复/STALE比例、acceptedQuantumCount/120模拟秒、generated/overload/revoked/carry终账。以30fps目标报告实测及差距，不再要求120fps显示；若只能提供nativeSwap证据，系统显示指标记缺证。短窗口功能链通过不能代替持续性能结论；不得删掉失败帧或仅选快区间计算。

## 6. 交付与变更记录

本卡已完成原档/源码静态核对、签包和归档hash核对、截图页面核对、证据降级及唯一工程后续合同；未用真机、未重跑测试、未改业务代码。T405原报告保留，由本合同正式补正其V2在位/同因/非回归/完整导出/负控推断，不覆盖测试原始记录。

2026-11-09：新增本修订；纳入用户30fps显示目标，独立保留120Hz物理子步、33.333ms控制GAP与250ms实验deadline。architect本卡成功交付不等于产品双源、卡死修复或30fps通过；后续由manager依据原卡完成通知登记并派developer，不新增manager验收门。
