# T420 · CPU 仿真全链复杂度重构

2026-11-09，architect，P4 CPU 参考线。GPU P5 继续优先。本稿定义一条以事务边界为中心的重构路线，包含性能上限的证据、接口和后续验证。使用已有真机原档和当前源码，不使用真机。

## 1. 决策

推荐 **单 owner 内按原事务执行，稀疏写入用局部原字节日志，稠密或覆盖未知的事务走原完整 bank**。在同一重构中处理行级复制、collect/审计重复、公共发布和画面消费。第一版保留真实同步入口及原公共边界全验，先建立覆盖完整的写入闭包和对照账。替代公共全量证明需要 designer 给出逐切点等价证明，再沿同一执行框架推进。单独移动渲染或把模拟移进 worker 只能改变谁被阻塞，无法降低现有物理工作量。

对 30fps 的当前裁定为 **实测未达，重构后可行性待证**。T352/T368 两个慢窗即使把每帧实际 `hotApply` 直接减为零，其余工作中位仍需 126/123ms。这已经否定“只改 apply 就能到 33.3ms”的同工作量方案。现有档没有跨所有实现成立的毫秒下界，也没有证据证明 CPU 经全链重构一定能达标。保留 CPU 参考资格。可交互备用资格由真实功能、速度和画面证据另判。

## 2. 数据、版本与整帧预算

核本地签包实际 SHA 与 `test/reports/t257/` 记录一致。T352 报告安装的 T351-isolated-final 为 `18399de86843f21780a9cf113ea086e656fca85f084bc4c7e1a47b96f68c976c`，T351 最终交付为 `70aec640eef4e0347748c7114d5a50b668c484b3e8a1d1ad38c2ed3b26c0edd4`，是两个包。T363 用私有文件、run/token、日志和生成时点把 `test/reports/t360/v2-budget-t352-era.json` 归到前包。文件本身没有嵌入包 SHA。原件 SHA `f320e6a6bfd4f7f22d14a3ab572075cdc39b00f830f4ee0d7eae569c537f0ee3`。

T368 报告安装 T366-delivery `b7b71466283b5d32a0a18f1a69543bcc3f587db78e24643e2131167d4a2d37fd`。原件 `test/reports/t368/v2-budget-t368.json` SHA `b1f4ff3aacb85fdde51d1e959c972705d90b41bfa5a5d3a155d5aad70d24f2e1`，与同 run 的 scene 日志身份吻合。原件实际 `{cuts,budget}`，`cuts` 为 `T367_STOP_COHORT_V1`，符合 T367 出口。T367-delivery 实测 SHA `e20d4dddfdc0215a127e4937dccd0b3743fd099a27342a4cc59c591435a66dad`。T366 出口为 `{budget,hotApply}`，六段细账未归档。结论为 `HAP_BINDING_UNCONFIRMED / HOT_APPLY_FINE_NOT_CAPTURED`。可使用 T368 窗口原有外层互斥时间，禁止据此宣称已测 T366 收益或已分出复制、校验、审计各份额。

T381-handoff `2189bf62e7ed33bd08d5521fae56cc53b95fb20e46437176f8642dc0ae1c4c3f` 与 T400-delivery `92f0d5f82c40c5975fda34cff37c1b328d2a306a17ea881cb073358e73ef3f9e` 的本地签包均核验一致。T381/T403 约束优化后的行为。它们的短窗功能结果和前述性能档属于不同签包、输入及工作量。

| 窗口 | 原始分母与可用结果 | 缺项 |
|---|---|---|
| 空场无源 | T352 scene frame1→941，78.077s、940间隔，12.039fps。T368 `phase0_empty` frame1→1114，88.021s、1113间隔，12.645fps。同窗 commit 差分别1880/2226。 | scene 是节流日志。逐 callback 分布、Q/A/R、各段与实际成功物理秒缺档。`L=0` 仍可执行场、发布和渲染，不能默认休眠。 |
| 注入及随后过渡 | T352 `phase2_touch` 含前述同 run 空场前缀，末段 frame1394→1402 为8间隔/6.146s，约1.302fps。部分区间 commit 差大于2×frame差，支持发生外源提交。T368 报告亦见从约12fps 降到1.2fps。 | 原档无对齐的 DOWN/MOVE/UP、逐 callback A 与 λ/seed 账。区间跨注入和停源，不足以给「持续注入」的独立分段或 p95。 |
| T352/T363 停源慢窗 | callback1369–1496，128帧，`N512/L101/Q2/A0/R534–606`。raw interval p50/p95 **767.742/768.993ms**，回调内 **759/763ms**。 | 旧 schema 无子步行。Q 是成功子步数，内部实际 dt 与逐角色请求缺档。 |
| T368 停源高接触 | callback2017–2144，128帧、256子步，`N512/L157/Q2/A0/R942`，每子步 R471。raw interval p50/p95 **834.586/851.291ms**，回调内 **831/835ms**。 | 包身份缺口如上。细分只到 hotApply 外层，无法比较内部排序/复制/审计收益。 |

两个慢窗均 `open=false, invalid=0`，帧内互斥和闭合128/128。最近环已覆盖旧前缀，不能据末窗复原整 run。T368 的子步互斥和闭合256/256，子步累计 wall 105247ms，不能拿它当整帧分母106367ms。两个窗的 scene serial 比 callback serial 小1，按 run/epoch/commit/完整 token 关联，子步按 parentCallbackSerial。Date.now 段计时为整数毫秒，原回调时间戳为另一个测量域。

| 同帧互斥项 | T352/T363 p50 ms | T368 p50 ms |
|---|---:|---:|
| hotApply | 634 | 707 |
| preview、collect、field 合计 | 59 | 63 |
| fill、公开校验、结构、exact70、严格域、逐字节对拍、seal 合计 | 58 | 51 |
| consumer copy、scene setup、pack、native API 合计 | 8 | 8 |
| 同帧扣除 hotApply 的全部余量 p50/p95 | 126/129 | 123/127 |
| `interval[i] − wall[i−1]` p50/p95 | 8.038/12.014 | 4.600/18.291 |

组合项先在每帧相加再求分位数，各行 p50 不可相加反推整帧 p50。`hotApply` 80796/96896 = 83.38% 和 90526/106367 = 85.11%。T368 的 86.01% 使用子步分母。两窗均为每帧六次 apply、十八次 collect 和六次 hot begin，旧 `rollbackDelta=14` 包含预演恢复，不代表14次失败提交。native API wall 覆盖调用到返回，异步 GPU 与系统实际显示另计。相邻帧余差含调度、其它 UI、导出和时钟误差，无法从中单独扣出 GC 或 GPU idle。T308 的两次 watchdog 栈定位到热路径采样点，支持累计工作风险，无法将六秒归给一次扫描。T331 的 audit sort 改善和 T366 非机 alias 快路对照各按原样本成立。跨包、跨负载乘旧单行耗时的估计作废。

33.3ms 对照下，T368 当前帧内 p50 需要约 **24.9 倍**整体缩短。即使热段全部消失，观察余量 p50 仍需从123降至33.3ms，约 **3.7 倍**。这是保持其他观察项原样的反事实算术。两窗内公共边界组和 preview/collect/field 组各自的 p50 都超过整帧预算，必须进入同一套验证。空场 T352 区间约83.06ms/帧，未有阶段归因。以仅优化接触行推断空场达标没有分段证据。

## 3. 累计工作界与必留成本

`N` 为池容量、`L` 为活粒、`C=128`、`B` 为冷注册绑定数。`Q/A` 为成功 STEP/源公开提交数，`q/a` 为实际进入两类 root 的次数。按原入口记录 `q`、源 λ 尝试、preview 候选、权威、恢复、collect-only、失败和重试。每次 apply `i` 分列递交 `R_i`、实际处理 `r_i`、prepare `P_i`、full audit `F_i`、增量 audit `I_i`、出生/合并查找 `M_i`、实际事件/lineage `E_i/G_i` 与写入 `w_i`。`P=Q+A` 只计成功发布。失败边界和冷导出分别以实际次数入账。`R` 是 apply 行数，包含重复试算和拒绝行，不能替代活粒、成功 LAND 或质量。T381 路径热容量为 request2048、motion=N、event4096、lineage8192，属于单 workspace，不是整帧总量。

现行入口为 `P051DerivedV2Demo.tick → P051DerivedDemoV2.tick → facade.inject/step → WaterSandField`。成功外源独立发布，随后 STEP 的备份不会撤销成功源。`stepWithTiltTrace` 每回调 0–2 子步。每个子步先定本次状态与预算，再预演、权威校验/collect、原序 apply、末端复核和公开提交。`P011ParticleM2Field` 的 collect 仍可重复排序和组装行。`P011ParticleM2Sink` 依序处理 guard、reserve、refund、publish、audit、close 及事件。`P011DerivedCommitV2` 对每个公开边界准备 inactive bank，验证结构、exact70、严格域和 authority 字节，再 seal/commit。画面读已发布副本，走 `P013SingleSandPreparation/Controller` 或实际场景消费及 native。只有同 frame identity 的互斥 leaf 时间可以闭合。

| 层 | 现有可审计累计界。所有和按实际调用及失败前缀计算 |
|---|---|
| 源与 preview/权威 | 按外源 λ×实际列/seed×birth/merge，及每 STEP 的实际 preview/verify/authority 次数展开。T197 历史全功能上沿 λ≤8、preview≤3 只对其声明入口成立。当前干态各入口计实际次数并登记上界，不能把三次 apply 直接认定为三次 preview。每次 collect 成本 `O(N + L log(L+1) + R_col + C)`，当前 audit sort 的省法不自动覆盖 collect。全路径还计 motion/结果比较、保序 fold、备份与恢复。 |
| 行事务 | 静态绑定/lease 上界 `O((P_i+r_i)B)`，预绑定命中仍保动态身份和失败次序。`V=55N+16C` 为 SoA 字节，`D` 为本次需要的派生/field 副本字节。主要复制项 `O((P_i+r_i+F_i)(V+D))`，额外 before/restore/场副本按实际边计费。一次成功、无重试的当前行有 authority→before→scratch→after→authority 四次整池 payload 运动。T368 counter19 每帧 116,723,712B，约3864份 V(512,128)。这是逻辑 copy 字节，不能和 counter9 直接相加当硬件带宽。 |
| 审计、索引、预算 | `O(Σ_full(N+C+IDcheck+flush)+Σ_inc(C+j_i+t_i)+Σ_i W_ticket/merge/event)`。`j_i` 是增量审计实际重放记录，`t_i` 为触列。当前增量实现仍清零并比较整 C，并重放自基线的日志。正常每16行重验及 attempt 尾全验保留，坏证书时最坏每行全验。`IDcheck` 有效索引可为 O(L)，失效排序 `O(L log(L+1))`，当前数组删除最坏搬 O(L)。pair/ticket 的线性 fallback 和出生全池候选搜索按真实实现分列，复用已通过对应入口证明的 T197/T205 索引，不将实验结构计作现路径常态。 |
| 场与派生 | 逐实际通道计 `O(C)` 的全列遍历、每 patch 全场刷新乘实际 patch 数、执行到的 donor×receiver 等项最多 `O(C²)`。T197/T205 的湿态二分及通道保守界不得直接收费给未走该分支的干场。重构仅移除有闭包证明的重复更新。必须保留的 ordered F64 fold、可影响读写的阈值/顺序与现行 correction 语义保持。 |
| 发布与显示 | 成功 P 及失败边界各自付 `O(N+C+4L+2C·70+L·70)` 的最坏公开全量验证工作，并加实际副本、字节比较、seal。渲染每次消费的 slot copy、pack/可见粒子扫描、native 上传/GL wall、Swap 与系统显示单列。UI 调度按真实 callback 墙时和相邻帧余差另计。 |

当前冻结合同下的工作量下界可具体验证。①对真正被访问的非空行/事件和活粒最少有 `Ω(Σr_i + ΣE_i + ΣG_i + Σ实际访问L)`，原次序及首错要求禁止跳过其中任一可区分检查。②保留全验时每个实际 `F_i` 至少扫描 `Ω(N+C)`。③每个成功公共边界至少核 N 槽、128列、`2×128×70=17,920` 个 exact70 limb，另有每活粒编码。N512 的成功边界实测 fill 37,376B、派生写3,072B、authority 对拍38,400B。两个慢窗每帧有两次边界，另读一个42,496B 消费副本。上述都是原实现工作量下界，字节和 limb 操作没有足够的目标机有效带宽/服务时间校准，不能换算成跨实现的毫秒下界。今后若改变证明合同，经批准的增量证明可以改变其扫描下界，仍需证明相同的可观察结果。

以 T368 的 `R=942` 做界限校验。若这942行均走成功且无重试的四份 payload 路径，`4×942×30,208 = 113,823,744B`，与 counter19 实测的116,723,712B是同量级。两者之差不分摊给某个具体阶段。若六次 apply 都走 T331 正常基线、每16行重验、两个终端全验且无 fallback，该子集的全验上界是 `3×6+floor(942/16)=76`。这不是实测全验数。fallback 上界回到逐行，再加真实终端/其它 full cut。对照需逐 role 记录这两个工作界的命中与偏离。

## 4. 路线选择和最坏成本

| 路线 | 保留的证明 | 时间、内存与退回成本 | 裁定 |
|---|---|---|---|
| 原完整 bank | 已有 rollback、公开全验、首错与冻结字节 | 维持上节每行、每边界乘数，存储为原 bank 与固定证据容量。旧代码直接可回放。 | 留作 oracle 与不支持分支。 |
| 保序等价批处理 | 只合并无外部 writer 间隔的静态校验/索引准备、只读快照、传输。逐行计算、真正票据 reserve/refund/close 与事件顺序保留。 | 降低固定成本。若仍每行快照与全量验证，最坏 `O(R·(V+D+N+C))` 保留，新增内存限预注册 bank/视图，配置撤回即可退回。 | 纳入推荐路线的第一阶段。单独交付不足以解决全链。 |
| 私有事务视图、触碰日志和逐切点证明 | 失败前缀、场/池/预算/事件完整写入闭包。每逻辑检查实际执行或用同等证明回答，保留 T331 重验以及未经批准的公开全验。 | 稀疏快路消去行内整池复制乘数。最坏证书全 miss、写集稠密、故障/不支持时支付原完整 bank；预检有独立上界。每写记录旧位型可逆，内存与真实写数上界成正比，固定容量超界在事务前选完整 bank。 | **唯一推荐**。 |
| 物理与呈现解耦 | 每个成功 STEP/source 仍提交完整权威状态。展示只取已封签版本，保持 lease、token、epoch 与失败首码。 | 可省未消费版本的画面复制/pack/Swap。物理工作界保持，异步需要固定队列、版本和最多占用数。回退于新 run 选择原同步入口，清理旧 owner/lease。 | 同步安全部分接在推荐路线上。独立 worker、自动丢输入、改时钟或背压属于另版本裁决。 |

推荐路线上，正常稀疏事务改成每 root/分支的有界 bank 准备与收尾，加逐写 `O(w_i)` 日志、逐访问 slot/column、原顺序工作及实际检查。保守式 `O(W_select+Σ_scope(B+V+D)+Σ_i(w_i + touchRead_i + orderedRow_i)+Σ_full(N+C+IDcheck)+Σ_miss(originalCutWork)+Σ_actualPublic(originalPublicWork)+field+display)`，scope 含实际 root 与需要独立备份的子分支，`W_select` 含配置和闭包检查。失败回滚成本 `O(w_rolled + frozenBytes)`，向证据读者物化原快照的成本按实际发生计费。若某个常用 cut 仍每行读整快照，它的 `O(R·(V+D))` 残余必须列明，不能把工作挪去物化层再记作已省。证书允许 miss，最坏路径为预检加原完整 bank 及真实证据物化成本。获准的隔离重放至多额外执行一次已回滚候选前缀，须单列上界。设 `U_cap` 为每 owner 同时未提交 scope 的逐写日志容量。现有 `TxWordUndo` 六个 U32 数组实际为 **24×U_cap B**，未包含 DataView、标量登记、owner/分支元数据、完整备份、发布 bank 和 ring。只有全部写路径可静态覆盖时，才可用每活跃 scope 写上界 `w_hi` 与嵌套上限 `D_hi` 配 `U_cap=D_hi×w_hi`。批量写按 word 数计，日志槽不能只按唯一粒子数分配。继承全部原数组及辅助索引按 T205 allocation ledger 同时存活数累加。T381 已登记的额外 typed bytes 3,494,516B 保留在峰值预算，除非按替代后真实生命周期证明释放。

## 5. 工程落点、接口与写入闭包

以一个隔离 V2 实施系列交付。沿用 T197/T205 的 `TxBranchView`、`TxWordUndo`、地址登记和工作量收据概念。当前基础实现并未证明本页所有 writer 已接齐，复用前须对照当前版本。以下接口用显式 ArkTS 类型，句柄由 owner 冷分配。

```text
CpuTxOwner.selectRoute(profile, bound, input): CpuTxRoute
CpuTxOwner.begin(kind, frozenInput, parent): CpuTxView
CpuTxView.mark(cut, domain): CpuTxMark
CpuTxView.beforeWrite(addressId, byteOffset, width): void
CpuTxView.beforeWriteRange(addressId, firstElement, count): void
CpuTxView.materialize(cut, viewId, target): void
CpuTxView.check(cut, originalOrdinal): CpuTxCheckResult
CpuTxView.commit(mark): void
CpuTxView.rollback(mark): void
CpuPublication.acquire(run, epoch, commit, geometry): CpuPublicationLease
CpuPublication.release(lease): void
```

`selectRoute` 在 root 首写前核 owner/buffer/shape、writer 闭包 revision、所需容量和 hook 能力。只返回 `LEGACY_FULL` 或 `LOCAL_UNDO_V1`。旧路径失败码、容差与结果相同，预检拒绝增量资格只写诊断。`mark` 绑定 owner/generation/root kind/attempt/branch/cut/ordinal。`beforeWrite` 保留原 1/2/4/8 字节位型，F64 保 `±0`、非有限值原位型与原拒绝。旧地址数值与每代登记明确区分，禁止 typed-array alias 重复注册漏恢复。`commit` 内层保留父回滚记录，`rollback` 逆序恢复、截断并更新控制代号，已发布成功前缀不在该日志域。容量上界证明失败则在开始前整 root 走原路。隔离验证遇到未登记写或运行期日志界违例，禁止执行漏记写、封存首差、恢复本 root、停用该资格。只有完全无已发回执/外部副作用、原冻结输入和重放一致性已证明时才允许有界重跑一次。原业务失败保留原失败，不因性能选择再跑变成功。对外可见的新增拒绝语义需独立裁决。

当前 `TxBranchView` 的 WHOLE_BANK 子分支与日志回滚不同，不能直接调用现有 `commitChild` 代替新日志域的父回滚。开发需对照 mark 栈、子提交、父撤销、已发布前缀及控制代号，复用登记形状及原位型工具。实际 `fork/mark/sealRead` 仍构造句柄，若复用结构则给出固定热分配上界或冷预分配实现，不能将已有 API 直接记为零分配。

| 写入域与责任文件 | 最小闭包和生命周期 |
|---|---|
| `WaterSandField.ets`、`P051DerivedFacadeV2.ets` | 外源 admission/λ、STEP 子步、preview 候选与权威的 root/mark。湿干场数组、倾角、预算累积、端点 ledger、原 substep/attempt 身份、source/elapsed、观测与 freeze。先保留原 backup 公共故障出口。分支只共享冻结输入及确定等价的静态读集，任何可变预算或库存单独持有。 |
| `P011ParticleM2Field.ets`、`P011ParticleHot.ets`、`P011ParticleField.ets`、`P011ParticleM2Projection.ets` | collect/计划/验证及 patch 生命周期。粒子 SoA、free list/slot generation/ID、merge 与排序索引、首错、行结果、event/lineage、派生缓存、动/静场回写。seed/RNG、pair 最小 ID 谓词和浮点表达式顺序原样。T381 零获准 T4 保源列并纵向推进，尾按源列终床复核，下一成功子步 T3 原序沉积和退役。 |
| `P011ParticleM2Sink.ets`、`P011ParticleHotBudget.ets`、`P011ParticleHotEvidence.ets`、`TxWholeTransactionCore.ets` | guard、票据/退款、源回执、全局与局部预算、sink checkpoint、事件和投影发布。登记显式下标写、`.set/.fill`、标量字段、原 hook 与 native 入口的读写影响。原冻结读者要求完整字节时，用同一 cut 的原视图物化后再恢复。不得以成功后状态、JSON 数值或摘要补造。 |
| `P011DerivedTupleBarrierV2.ets`、`P011DerivedCommitV2.ets`、`P011DerivedCheckpointV2.ets` | 登记 exact70 limbs/count/cache/dirty、结构顺序、field marks、before/after、verify serial、inactive/active bank 与 seal。原失败顺序含 binding→structure→exact70→strict→authority bytes。保原失败 raw 和成功 prefix。控制代号单调不回绕，业务 commit/receipt 仍按原规则。 |
| `P051DerivedV2Demo.ets`、`P013SingleSandPreparation/Controller.ets` 与对应 native | 独立画面消费时钟和 slot。先做同帧内一次消费，合并经证明等价的重复数据准备。后续展示可合并多个已经独立发布的版本，按原 seal、run/epoch/geometry/optics、完整 visible key 取最新合法值。旧 token 明确 STALE，失败 FAULT，禁止在 reader 持 lease 时复用 slot。记录被跳过的展示机会及消费延迟，不能因此跳过任一物理步骤/源回执。仅 XComponent+shader，禁止 Canvas 回退。 |

写入登记表每行含 `file/symbol/writeOp/addressId/elementType/width/owner/view/domain/earliestCut/latestReader/invalidates/restore/escape`。逐 stage 对照 T205 manifest 和本分支的真实 writer，加入隐式 `.set/.fill`、复用 scratch、影子预算与故障回调。未知写入、跨 owner 别名、detach 或重入按原优先级触发原全验证，禁止视作证书命中。记录原物理 `code/stage/ordinal/source/attempt/row`、commit/elapsed、事件顺序及冻结原字节，不能只比最终床量或 checksum。`Q=2` 第二子步失败必须保留首步及外源的成功前缀。

### 证明交接与迁移顺序

1. **阶段 A，统一基线和切点。** developer 沿原同步页及 T400 参考语义建立同一个隔离构建的原路/候选选择。复用 T351 `FrameBudgetV2` 和 T366 `P011HotWorkReceipt`，扩展按 role/cut 的计数、完整 copy 边、collect 检查和未闭包 writer 清单。在原路上先得到高接触匹配负载、空场与注入的离线可复跑基线。把私有原样 `.set` 的必要读取保留，等价化只提交已证的部分。此阶段不独立开一串热点卡。
2. **阶段 B，整事务闭包与局部日志。** developer 在原 cut 对比完整 bank 与日志视图，先覆盖干态落沙及 T381/T403，再扩实际覆盖明确的稠密/异常场景。复用已证索引或选原索引路径。每个成功/失败边界仍实际执行原全量公开证明，保 T331 K16/尾审计/回退。仅当每一域可被物化成原 cut 字节且独立差分全过，才将该 profile 作为可选新路。把 full-bank、日志、回退和冷档峰值纳入单预算。
3. **阶段 C，闭包上节省剩余整帧成本。** 在同一架构中用所有者、版本化依赖和纯只读切点消除可证明相同的重复准备。若需要减少公共全量 exact70/结构扫描，或以触列审计代替原有全局有序 F64 fold，先由 designer 证明并给反例。必须证明逐 mutation 的 slot/generation/ID/free、两相列 limb 溢出与 round-to-nearest-even、`±0`/subnormal、原顺序 RN64 fold 与每个 prefix 的首错、全局 strict、脏列传播及跨分支回滚。依赖闭包失配在原 cut 跑原验证，`serial` 或 hash 本身不足以推出等价。C² 场算法、触床判据、步长/源量、新增队列拒绝或改观测语义，若现合同内无等价证明，同样转 designer/manager 裁决。A/B 的可做工作不等待此证明。
4. **阶段 D，画面解耦测量。** 先保持同步物理 clock/输入队列，按独立呈现时点消费已接受的发布版本，实测 pack、提交、GPU 与合成等待。若确需 worker，另给单 owner、有界输入/输出、顺序取消、未完成 root 归属、背压与用户可见反馈版本。线程拆分后的新 ACK 语义须审批后实施。不能把帧率抬高与模拟变慢或重复显示一并计为收益。

以上阶段是同一架构的证明顺序。developer 交付 A/B 和 C/D 的可证部分时，若发现公共边界仍是硬预算障碍，提交明确的 cut、计数、原始残量和需证明的命题，由 architect 转 designer。新证明未齐时保留原检查及真实性能结果。生产路由切换由 manager 裁决，GPU 主线优先。

## 6. 交付验证、预算门与后续

开发不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止，局部受阻先完成其余项。不得修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，除本人行状态有限例外，不改滴答同名清单。编译、非机对照及签包即开发交付。tester 使用独立 baseline 和 oracle 复核，真机由后续独立任务按 GPU 优先串行排队。

- **匹配负载。** 同输入 tape/seed/reset、初始 checkpoint 和实际 dt/姿态序列逐条回放。覆盖 `Q=0/1/2`、`A=0/1` 与实际 λ 重试、N192/N512、L0/低/101/157/满池。高接触至少对齐 N/L/Q/A/R 的分布与角色、attempt 数，参考 R534–606 及942。离线夹具若只能分别匹配 L 与 R，分表报告，不能冒同条件。空场需分别测活跃无源与真实休眠。注入从 DOWN 到 UP 精确独立取窗，停源窗固定成功物理时间并另报实际墙时。全额/部分/零授予、容量边界及接触事件混合均入矩阵。
- **等价回归。** T381 受阻原列下落、目标高床、源列终床变化与失败恢复。T403 0° 的三笔正授予及 `0→0.3`，受限臂的 `0.13212158388108966` 与零获准臂保持。核原 admission/particle IDs、位型、事件序、exact70、source/commit/elapsed、原始 code/cut/ordinal、失败封存与 LastGood。注错覆盖每个 savepoint 之前/之后、旧回执、外部 writer、同地址多次写、票据回滚、K16、index/cert miss、日志边界、密集旧路及 second-step rollback。设备“同 publication CURRENT swap”和系统实际显示分别判定。
- **工作量先行。** 每段输出 p50/p95/max 与配额、role、attempt、N/L/R、实际 copy/undo/物化 bytes、full/inc/fallback、sort/collect、场 dirty/全量次数、内存峰值、scheduler 与 cold 导出相邻标记。要求稀疏快路有实测删除的复制边与 `R` 扩展斜率变化，分别列稠密 fallback 和失败回滚开销。未命中样本计入同一负载总体，不把失败过滤成更快样本。正常路径和诊断 ON/OFF 都要逐位一致并报开销。没有公开基线缺项时才谈优化比例。
- **真机 A/B。** 使用同签包冷选两路或同一源码/参数的成对签包。记录安装 SHA、app/PID/设备、build mode、V2 motion revision、窗口/分辨率、renderer、传感器、温度与热/冷状态。场景按预登记负载顺序交替 A/B，独立重复，冷启动和稳定窗分报。每个场景建议预登记至少三对窗口、每窗至少128个连续 callback，以有界 ring 捕获。若原始失败先到则封存真实前缀，保留配对失败理由。三窗口均要有子步/帧/呈现对应与可核原件，异步 GPU 或系统呈现读不到就单列缺证。
- **33.3ms 判定。** 参考 30fps 显示目标，报告真实逐 callback p50/p95、工作线程实际服务墙时、有效新内容帧率与间隔、输入延迟、积压/掉帧/STALE、成功模拟秒/墙钟秒。观察负载达同条件的稳定新帧吞吐与物理推进两项，禁止以重复 Swap、旧图或减少 Q/A 当通过。同步实验如回调内 p95 高于33.3ms 直接登记未达。异步实验还要查真实展示和物理积压，UI callback 快不足以判整体。若证据分账仍有一个必执行阶段自身在目标工作量下超预算，保留其条件下界并归因。若 A/B 后剩余组仍超预算，先处理 C 段的证明/算法前置，CPU 继续参考线。无需再按单热点重复派工。

本卡产出为整链工程方案和角色接力。已经按实际签包、窗口和原始互斥账核对 T308/T331/T349/T351/T352/T361/T366/T368/T374 证据，T381/T403 为行为回归基线。当前缺少空场/注入阶段账、T368 细项与包绑定及优化后匹配负载证据，按上文一次补齐。下一步由 manager 按 GPU P5 优先派 P4 developer 实施该系列，安排独立 tester 消费。若需要改变证明或物理合同，按阶段 C 明确分流 designer。
