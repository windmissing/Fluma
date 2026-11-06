# T374 · CPU 粒子 apply 累计复杂度与取证合同 v1

2026-11-06，architect。不使用真机。按当日排期，GPU compute 为主线，T374 是 P3 CPU 参考工程。依据 T361、T366 交付、T368 复跑报告及原档，并对照 T331 的实际审计实现。本文交可派工合同，设备与产品性能仍按实测登记。

## 1. 原始计量和版本纠偏

`test/reports/t368/v2-budget-t368.json` 实核 234,877 B，SHA256 `b1f4ff3aacb85fdde51d1e959c972705d90b41bfa5a5d3a155d5aad70d24f2e1`。`budget.revision=T351_EXCLUSIVE_FRAME_BUDGET_V1`，run `p011-run-1793931455921-3`/epoch2 与 hilog 317 条 CURRENT 的 run 一致。128 个帧收据及256个子步连续，`open=false, invalid=0`。`incomplete=true` 来自环覆盖（帧2016/子步4030），未保存早段阶段账。callback 2017–2144 与 render token 2016–2143 有一帧编号偏移，使用完整 token、commit 和 parentCallbackSerial 对接。首末帧提交4048→4302。256 子步父帧对齐，每帧 Q=2/A=0、N512/live157、每子步 R471、每帧 R942。R 包含预览，不能当实际批准/沉积粒子数。

| 统计口径 | 互斥闭合 | wall 合计 | hotApply 合计 | 份额 |
|---|---:|---:|---:|---:|
| 子步 `sum(row[24:44]) == row[6]` | 256/256 | 105,247 ms | 90,526 ms | **86.013%** |
| 帧 `sum(row[24:44]) == row[19]` | 128/128 | 106,367 ms | 90,526 ms | **85.107%** |

帧 callback interval P50/P95 834.586/851.291 ms，wall 831/835 ms，hotApply 707/710 ms。子步 wall P50=411 ms、hotApply P50=354 ms。前帧 wall 对后帧 rawInterval 的127个配对中，累计占比99.155%，剩余中位4.600 ms。它是跨两时钟的占用近似。停源慢窗约1.2 fps 的 FAIL 保留。每帧扣除 hotApply 后余量中位123 ms，是观察分账，不能当作优化后速度预测。pack/native API 的128帧累计675/269 ms，异步GPU耗时未测。每子步3次 apply 与9次 collect，每帧6次 apply、18次 collect 和6次 hot begin，derived rollback counter14含预览恢复，不作14次事务失败。T352/T361 另包慢窗 N512/L101/R534–606、hotApply中位634 ms。两档负载、包和诊断身份不同，无法作严格的优化 A/B。T312 的旧每行斜率及旧全验口径也不能外推此窗。

**T368 归档与报告所列的 T366 出口不一致。** 本地签包实核 T366=`b7b71466283b5d32a0a18f1a69543bcc3f587db78e24643e2131167d4a2d37fd`，T367=`e20d4dddfdc0215a127e4937dccd0b3743fd099a27342a4cc59c591435a66dad`。T366 签包输入树 `.codex/local-test/t366-final` 的导出固定为 `{budget,hotApply}`，页入口打开 `configureParticleFineReceipt(true)`。归档却只有 `{cuts,budget}`，cuts revision=`T367_STOP_COHORT_V1`，与 `.codex/local-test/t367-final` 的页面出口一致。文件没有内嵌安装包 SHA，同 run 日志也不能证明装了哪一个二进制。以 `T368_RUN_STAGE_EVIDENCE_VALID / HAP_BINDING_UNCONFIRMED / HOT_APPLY_FINE_NOT_CAPTURED` 处理。不得把外层86%称为 T366 六段细分或宣布其 alias 优化的设备收益。tester 如能从既有装包/导出回执补证，先追回真实 `{budget,hotApply}` 和 install SHA，给出文件名、run/token、包身份。原细档缺失就登记缺证，在下一 CPU 参考工程的新诊断包取样，禁止重建旧档或占用 GPU 的真机优先窗口重跑同旧包。

T366 离线 8/8 和字段/计数定义保留。细账 `T366_HOT_APPLY_EXCLUSIVE_V1` 每16帧采样一次、64 apply 最近环，6段为 applyTail/planIdentity/prepareLease/transferAndDerived/rowAudit/budgetGuardEvents。已有 per-attempt role、request kind、full/incremental/fallback、实际复制与 clip 计数。其离线快检350–534次/apply、完整 alias 比较0来自固定夹具，L101 与原 R534–606 未同时覆盖。当前原档缺细账，复制、证书、审计和校验各自的设备时间份额仍未知。T368 回执中的 `5.45ms/R×404–536≈2.2–2.9s/帧` 是跨版本估算，且大于本窗约0.835s的实测帧间隔。404–536来自 T366 离线夹具，本窗每帧实际 R942。该估算不能作为本次审计份额。live157与CURRENT只作池计数及渲染身份证据，守恒需库存/事件独立对账。

## 2. 当前工作界与可复用范围

令 `A_sub` 为每成功子步在 preview 与权威中的 apply 次数，`R_i` 为第 i 次 apply 递交行数，`P_i` 为含重试的 prepare 次数，`N` 容量，`L_i` live，`C=128`，`B` 已登记缓冲，`K=16`，`V(N)=55N+16C` 为粒子 SoA payload，`D(N,C)` 为一份派生视图 exact70、count、dirty、slot 元数据的复制界。`M_i` 表示实际 touched slot/lineage/merge 搜索规模，所有量逐 attempt 累加。主项是

`W_sub = Σ_i [planCheck(R_i,L_i) + leaseCheck(P_i+R_i,B) + copyTraffic(P_i,R_i,V,D) + auditWork(R_i,K,N,C,L_i) + budget/event(R_i,M_i)] + collect/compare/restore/field + publicBoundary(N,L,C,70)`。

- `preparePatchHot` 在每次有效 prepare 内 authority→before→scratch→after，publish 再 after→authority。无 retry 的成功行可见4份全池 payload 运动，还包含对应派生或取证边界。`copyPoolDiagnosticHot` 和 `copyPoolHot` 所在的 derived counter19 实测每帧 **116,723,712 B**（N512等量3864份），此为**整 callback**累计。942行×4=3768 份仅是成功无重试主链计数模型，余量不能无细账分给某阶段。counter9 的167,211,488 B属派生记录的另一账，不能与counter19简单相加成设备内存流量。需要把每笔 copy 的边、有效载荷与发生阶段分别归属。
- T331 首次全验建基线，之后 `SINK_ROW` 尝试增量，按 K16 周期、attempt 尾及 fallback 做完整检查。`APPLY_TAIL/FINISH/OTHER` 仍走全验。设 `F_i` 为实际完整审计次数，正常周期与 `ceil(R_i/K)` 同阶，尾审计和 fallback 额外计数。审计项按 `F_i·(N+C+IDcheck)` 和增量 touched/journal 工作分列。证书失效可回到每行全验，不能把旧 T311 的逐行完整排序当当前常态。当前有序 ID 索引也保留实际插删成本。
- T366 快绑定减少重复交叉比较，仍有逐调用的 busy/shape/身份/byteLength 检查和失败全路。快绑定只认证已登记 buffer 关系，不能认证内容、预算、合法性或跨 attempt 状态。把单次全 alias 比较变快不会消除 `Σ R_i` 放大。
- preview 候选和权威在角、budget shadow、before bank、step/attempt 与写入后状态上各自独立。权威输入证据必须来自实际 authority。双 collect 与真实重验保持，同角或同 R 不能共享可变计划/配额/结果。分支数、重试、恢复和外层校验计数一并进入累计预算。T353 的零 grant 横跨与纵向停滞是行为分流，不能借跳过行、延迟落床或改变运动规则换取计量收益。

## 3. P3 工程实现顺序及接口

1. **归属闭合与证据版本。** developer 复用 `P011HotWorkReceipt.ets` 的已有六段和 `FrameBudgetV2`。新冷档按包构建标识、run/epoch/frame/substep/attempt、role、before/after commit、copy edge 与 first error 对齐。冷出口保留 `{budget,hotApply}` 版本及 schema，若诊断包兼有 `cuts` 则同档显式包含三者，不能覆盖已有字段。包 SHA 由实际安装回执及归档 manifest 关联。相同帧记录 aggregate apply 时间，只有被采样的子段才计算内部份额。导出列出细计时 ON/OFF/抽样率、invalid、覆盖和丢样。保留对账计数，避免每行新增时钟/日志，诊断波动单列。
2. **共享不可变准备。** `P011ParticleField.ets::validateHotTransfer/transferHotState`、`P011ParticleHot.ets` workspace storage 和 lease、`P011ParticleM2Sink.ets` 热回调按原 owner 建冷注册表。每 attempt 验证固定 storage 形状、绑定代、无 alias/detach；后续校验可复用明确不变的项。逐行仍按原优先级验证 busy/lease、step/attempt/plan/ordinal、动字段和可变回调状态。任一可变/未知、fail injection 或证书失配走原完整路径。跨 preview/权威只共享不可变 buffer 布局和静态索引，不能复用已消耗的 lease、派生审计结论和 reserve/grant。M2 事务与真实 guard 调用顺序冻结。
3. **减少整池复制的第一档。** 在 `preparePatchHot`、`editPoolHot`、publish/restore 与 `P011ParticleHotEvidence/P011ParticleM2Projection`/`TxWholeTransactionCore` 逐边列出 writer、reader 和覆盖时刻。候选优先验证“直接将 before 复制到私有 after 并在 after 上编辑”以消除成功路径的 scratch→after。准入前提是 scratch 作为失败证据、derived view 身份、T331 基线/ID 索引、候选重试与 frozen raw 原字节均可保持或等价物化。原始实现有独立 before/scratch/after 读者，禁止直接别名合并。不能证明该候选则保持 bank，选其它被证明重复的传输。每次优化须给一个实际 copy edge 或检验项减少量，不能把同一工作搬到未计量段。K16、fallback、attempt 尾与 public boundary exact70 全部保留。
4. **整事务的第二档仅凭证明推进。** 若需要将每请求 O(N+D) 快照降为触及 slot/列的工作，先建立 attempt 独占 before bank、私有 writer 集合和逆序 journal。逐行保留预算/来源/guard/allocator/merge/事件/lineage 实际顺序、事件容量与最早错误。明确 journal 覆盖、别名、溢出、capture 字节、checkpoint 何时物化、每项恢复原位型及 K16/full fallback 重验；公用 bank 在成功提交前不可见。若替换证书或更改 failure evidence 口径，返回 architect 定接口和不变量，交 designer 证明后再实现。无需先做第二档才执行可证的第一档。

## 4. 验证、交付与失败边界

同输入做基线与新包非机差分，至少覆盖 Q1/Q2、空场、N512/L101及L157，采 R≈606/942 和逐 role，含全额/部分 LAND、跨列零 grant、容量不足、分支退化、dirty/merge、同一 attempt K16边界、fallback 与撤回。若同 N/L/R 无法同时复刻，按实际归档多个轴，不能把近似档当同条件。故障矩阵覆盖替换/alias/detach、请求/lease stale、epoch/reset、stage 抛错、prepare/publish 后回滚。逐 bit 比对权威 pool/field、source/elapsed/commit、exact70 acc/cache、事务 ID、票据、事件、first error 及冻结原字节。T366 已知 refund 源码哈希与 full-lift-degrade 的基线红项保留原证据并报告是否新增偏差。按 `(role,Q,R,attempts,F,P,N,L)` 分层检查各边复制和审计计数，使用受控 fixture 比较真实壁钟，检测诊断开销。保持原物理 dt、守恒、严格域、事务和显示合同。

developer 本角色编译、签包和上述非机检查后交付。独立 tester 在 GPU 队列允许时用同设备同脚本的标识包采对照，按帧和子步分别对账。设备性能判据沿逐 callback p50/p95、真实阶段与完整身份。33.3 ms 参考仍保留，先验证 CPU 参考减少实际累计工作，原档非 hot 余量123 ms 的量级说明单一优化不足以承诺流畅。若新证据显示 hotApply 内另一子段主导，带独占计量回 architect；工程修订不以盲调参数掩盖负载。渲染前景背景沿 XComponent+shader，失败 FAULT 或完整上一有效 token 的 STALE。

本卡 P3 复杂度复核与工程合同已完成。T368 的性能 FAIL、T366 装包身份差异与细分未取到分别登记。后续 CPU 参考是否排入实施由 manager 按 GPU 主线调度。
