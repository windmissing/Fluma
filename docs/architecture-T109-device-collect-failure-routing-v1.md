# T109 设备COLLECT99首失败事务工程分流 v1

日期：2026-10-26。architect。原卡 `6adf3613e4b0127832c06d35`。不使用真机。状态：**证据层级纠正与诊断工程接口交付；BUSY/粒子失败/audit之间尚不可唯一归因，不授权盲修物理**。

## 1. 原始分片核对

消费T103 `16a37d6`正式报告和 `test/artifacts/t103-first-code5-capture-20261026/{C1-full.txt,C2-full.txt,run-console.log}`，对照c0c58c3固定源码的事务/错误路径及T102开发报告。本轮直接从WS_EVENT first_code5_trace的JSON解析分片，按run和part重组，不运行产品测试，不改变原始日志。

| 臂 | 实际分片/载荷 | run与子步身份 | 结果 |
|---|---|---|---|
| C1 | 3/3片，6105字符/UTF8 bytes，JSON可解析 | `p011-run-1793012705902-3`；仅a675/substep675 | REQUESTED42→COLLECT99；beforeCommit=afterCommit=682。 |
| C2 | 48/48片，97121字符/UTF8 bytes，JSON可解析 | **同一个**`p011-run-1793012755607-3`；a563及a564两条trace | a563 REQUESTED42/CLAMPED30均PLANS_READY、result=clamped；a564 REQUESTED42→COLLECT99；beforeCommit567→after568。 |

两臂均complete=true/dropped0的原观察保留；C1不是97KB，C2的两条trace不是两个run。C1/C2确有不同run，证明两次独立触发；**C2内部reset又产生第二个run的说法没有这些分片证据支持**，不据此签reset重布防全过程通过。

外层dt=1/30，内部每条trace dt=1/60，一次tick可做两个子步。C2 first substep已提交，第二个失败，因此整tick前后commit+1不等于失败子步半提交，也不能给它标“整个tick逐值回滚”。beforePool是LAST_PRESENTED publication，不是紧邻失败attempt的权威全池快照；last presented可能早于inter-step外源。C1 commit没变也只证明成功commit身份未推进，仍缺完整数组逐值rollback证明。

## 2. 42°候选与30°effective的含义

raw的thetaBefore约29.999999999999996，target约79.75/79.84，maxTiltOmega720、substep dt1/60，故REQUESTED=min(target,before+12)≈42。30是上次planner已接受的effective，不是T50把target钳到30；实际input.targetTiltDeg仍约79.8。C2 a563重算30计划后clamped成功，a564发生另一候选失败。不得用effective30替换真实requested42复现，也不把两候选/子步的pool状态合成一个静态点。

## 3. 关键纠正：COLLECT99不等于beginCollect非零

固定c0c58c3的 `WaterSandField.previewChannelPlansAt` 在调用**整个substep(dt)返回后**写trace.stage=COLLECT/collectCode。substep内链是：updateSandTop→（非dry水通道）→saveMotionSnapshot/beginCollect→advection→settle/percolate/creep/relax/lift请求收集→updateSandTop→audit(true)。因此该stage是大阶段，尚非beginCollect返回的直证。

已走读两条99来源：

1. saveMotionSnapshot调用M2.beginCollect，非零只写lastError=UNKNOWN；substep看到particleBinding不active时返回UNKNOWN。这一分支需精确M2状态、active以及内部Hot状态才能裁定。
2. substep末尾audit(true)的getDrift检查达到原DRIFT_LIMIT同样返回UNKNOWN=99。该处COLLECT99可以发生在beginCollect及双collect均成功之后。S0几何可行不证明H/L/水审计通过。

M2.beginCollect自身也折叠多个来源：stepOpen非0→BUSY1；attemptToken不安全整数、beginStepHot非0、collectA/B非0、compare非0→STEP_FAILED3；外层再折为UNKNOWN99。Hot begin还含configured/active/pageBacklog、storage/field输入/medium、pool transfer、budget.beginHot的返回；双collect/compare也可能失败。**不能从99直接选BUSY，更不能默认是重入**。

poolProgress的firstError0/rollbackCount0是现有摘要，未携带上述逐分支原码，且部分诊断在begin/checkpoint或清理过程中更新；不足以排除所有粒子/预算/容量/journal/审计错误。live36或21、merge0说明该次导出没有MERGE计数，不能证明所有容量检查都不会失败，也不构成T78全链与本故障绝对无关的证明。两条线已有不同观测应独立处理，不用本卡重新撤销T96/T97交付。

## 4. 事务生命周期走读与当前裁定

`restoreBackup`包含particleBinding.rollback→pool.restoreSnapshot→discardCommittedPage；preview入口先restoreBackup，finally再restore scalar/sliding/request budget。M2.apply成功finish清stepOpen；collectA/B/compare失败会rollback并清stepOpen；M2.rollback对work.active非0调用Hot rollback再清stepOpen。Hot rollback也可能自身返回错误，当前M2.rollback未把它作为只读首错误暴露。

由此**源码有清理路径**，不能仅凭“预览调用真实substep”认重入。另一方面清理调用存在不等于每次都成功，更不证明stepOpen、work.active、budget epoch/pageBacklog始终一致。需要同一次candidate的entry/exit/cleanup原值与原返回码；不能通过无条件清零stepOpen、work.active、budget epoch来修一个尚未证实的状态问题。

当前分类为：**确定的工程可观测性/错误折叠缺口＋未知业务首失败**。先实施下面最小诊断工程修订并独立取证，回architect再分流具体业务修复；不等T98/T104整套严格态writer启用，也不向designer泛派数值边界设计。本次不是未实现预测试RED，设备真实code5仍FAIL/invalid。

## 5. 最小工程接口：P011_COLLECT_FIRST_FAILURE_V1

在原有T85/T102 typed trace中新增可选的有界firstFailure对象，不另建第二套物理账本，不改变现有外部错误码/endpoint/peak公式：

| hunk所有权 | 必须记录、读取时点 |
|---|---|
| `P011ParticleM2Field.beginCollect` | 接收dt/tilt/substep/attempt/run/seed/revisions；entry的stepOpen/work.active/stepEpoch/pageBacklog/budget epoch-openCount；每条实际分支M2返回、Hot原返回、phase=ENTRY/TOKEN/BEGIN_HOT/COLLECT_A/COLLECT_B/COMPARE；未执行项明确NOT_RUN。 |
| `P011ParticleHot`与Budget只读诊断 | beginStep失败原status/具体校验stage、首无效列/slot及谓词、owner/run/token/field binding身份；compare失败首字段/row/index/两侧位型与count，不能全量打印两plan；budget begin/rollback原status及epoch关系。 |
| `WaterSandField.saveMotionSnapshot/substep/audit` | 明确firstFailure.origin=PARTICLE_BEGIN/CHANNEL/AUDIT等；collect已成功但audit失败时记录三类getDrift值、原阈值、initial与source累计、waterTransactionNet、H/L/水/pool总量及首相关列；原方法只求一次实际值，不为诊断多跑audit/collect。 |
| preview入口/退出及restoreBackup/rollback | 记录candidateSerial、role/theta、parent attempt及checkpoint serial、清理前后stepOpen/active/budget/pageBacklog及rollback返回；**首错误在cleanup前冻结**，后续恢复/成功不得覆盖为0。 |
| `EmptySandDemo` T102导出 | 复用首code5一次导出和最多64×2048字符，含首错误摘要及两个子步各自commit/rollback身份；field configRevision与T50 input configRevision分列，poolProgress字符串明确解析层次。 |

新增对象用冷配置固定容量/预分配标量槽，热循环不拼无界string、不保存全plan；必要完整失败输入快照只在首次失败冷导出，长度超限标TRACE_INCOMPLETE并给独立artifact出口，不截掉关键字段后仍complete。导出字段包括同attempt权威before/after库存/完整池和预算digest；若没有完整回放输入，只称路径诊断，不冒称可精确离线重放。JSON byte/chunk校验独立，不能以complete标志代替part唯一性/连续性/最终JSON解析。

firstFailure记录不修正返回值、不清除lastError、不推进commit或读有副作用getter；诊断异常只记incomplete，不让物理失败变成功。原公开step code5、collect99历史语义保留，同时携带新的origin/rawCode。若未来要细分公开错误需另版，不在补证中顺手换码。

## 6. Developer与tester精确任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外；不改滴答同名清单。共享代码不回退他人改动。全部渲染仅新XComponent+shader含背景前景，禁止Canvas上屏/失败回退；失败FAULT或显式STALE上一有效新管线帧。

developer仅在§5局部hunk接只读诊断、错误原码留存及现有导出扩展，编译/离线自测。不得直接重试beginCollect、跳过audit、清active/epoch、增容量或将REQUESTED失败强行改为OLD可接受；这些均改变物理/事务合同，须实际根因到件后先修工程文档再实施。

测试先行明确区分三类99：真实M2入口BUSY、内部begin/collect/compare原码折叠、collect成功后audit drift失败。用真实接口可达状态构造反例；必须使用桩时标测试边界且不可当设备首因。覆盖正常REQUESTED→CLAMPED→OLD、双子步前成功后失败、preview异常finally、rollback异常留原错误、诊断开关同输入场/pool/预算/ID/账本序列等价。保留原守恒/容量/有限值/阈值，不造“所有99必须是BUSY”的镜像断言。

独立tester先补T103已有日志重组报告：C1/C2 part唯一连续、实际chars/bytes、run/PID/trace attempt和外层commit匹配；不重复整段设备取证。若缺必要业务输入，给明确MISSING字段后manager派真机串行窄采证卡，用同dry30°→requested42真实target/输入序列和新版firstFailure导出；该卡须真机标签。结果回architect，tester/effector不承担整体根因猜测。

## 7. 到件分流与T95前置

- 原码BUSY且entry/checkpoint证明残留事务：工程修复只改对应begin/finish/rollback所有权与preview隔离hunk；须证明正确owner负责关闭，不靠强制清标志；旧pool/预算/外源精确回滚。
- BEGIN_HOT/collect/compare失败：按实际status及首谓词定位输入、计划缓冲/类型、预算、pool或身份；若输入合法却实现不符，architect落具体最小hunk→developer；真正非法输入按原fail-closed保留，不调阈值。
- audit drift99：对同attempt实际库存与预算/外源/池汇总顺序归因，不能先改baseline/容差；若设计合同问题再designer，输入/观测错则精确工程修订。
- 仅日志/状态身份混用：修tester采集与诊断身份，不改生产动力学；用例主张争议交manager，不私改原期望。

T95整卡继续等首失败机制裁定及必要修复/开发离线交付；窄取证是实质前置工作，不把整卡因可做局部准备放行。T109工程文档交付不等设备code5修复完成，manager据报告派诊断/补证并推进，无新增验收门。T98/T104离线严格态路线独立，不阻止先修已确定的事务或audit工程问题。

本轮仅归因文档、固定源码走读及既有日志重组，无业务/用例/阈值修改、无测试重跑/真机；自己的行结束待验收，原卡报告完成与未知首因。
