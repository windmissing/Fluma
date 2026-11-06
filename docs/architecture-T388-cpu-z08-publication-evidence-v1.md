# T388 · CPU 源列沉降发布身份补证分流

2026-11-06，architect，P3 CPU 参考。不使用真机。补注 T378 §4–5 与 T386 Z08 判读，复用 T381 的已发布 companion 和现行 renderer 收据。

## 1. 两档实际证明的范围

核 T386 五份文件的登记 SHA256 前缀、T381-handoff 签包 SHA256 `2189bf62…c3f` 和 T386 原卡。两份成功收据同 run `p011-run-1793958125594-3`、owner 1、stateEpoch 2，substep/commit 分别为 842/860 和 1444/1462。原始 F64 大端位型逐行解码，以 `slot/id/sourceId/column` 对齐 240/240 行，各粒质量逐项相同，`floor(finalX)==column`。这是两时点和观测行的结论，间隔有 602 个成功子步序号差；该档没有逐步 dt、实际接受角或 UP/首拒绝的锚点，不能从序号推物理秒或连续路径。

| 子项 | 裁定 |
|---|---|
| 零获准 T4、源列下降 | 基线 240 行低四位 reason=2（`CROSS_BUDGET`）、finalState=1、目标列均为源列+1，grant=0。240 行 `finalY<y0`，范围约 12.4871–78.4277，保留在源列。 |
| 接触与 T3 | 沉降档 240 行 reason=3（`CROSS_LAND_REMAINDER`）、targetPlusOne=0（无跨列目标）、finalState=2、Sfinal=0；finalY 原位型 `3eb0c6f7a0b5ed8d` 即 `1e-6`，是床面加 contactEps。源码只在 kind=2 且 granted<requested 给 reason=3，说明该子步已实际走原列 T3，240 行 grant=0，余量全部仍在原粒。两档未覆盖从 AIR 到首次接触的确切子步。 |
| 实际 LAND 与退役 | 零获准 T3 的全额余量已解释，属于保留状态。当前无正授予 LAND 质量或全额 LAND/退役证据。T381 host 近床夹具已有正授予及退役，单列离线通过。 |
| 预算和质量 | 两档各 240 行 reserve=refund>0，累加均约 5.0（相同 F64 累加值 5.000000000000005）。同 cohort 两档粒质量和均 23.085180362701475。这是已观测 cohort 质量与本批票据证据，整体库存守恒须用权威审计/外部质量账。 |
| 回滚 | 两个失败文件逐字相同，`failed=false, rolledBack=false` 且身份为空/零。只证明两次读数未见当前失败回执，`beginAttempt` 会清标志。设备注错及失败恢复是 NOT_RUN，T381 的九切点和保成功前缀继续单列离线结果。 |

全额余量和实际全额沉积须分项记账。T386 已有数据支持 CPU 源列下降和进入原列零获准 T3。旧报告把 1e-6 写为 0，并把全额余量与全额 LAND 的证据界线写得过宽，以本稿的位型和代码判读为准。

## 2. 现有 publication 通道与最小接口

T386 目录只有两份 companion、两份独立失败文件和 layout。按该 run 搜索 `test/reports` 未发现同包同提交的 publication 或 renderer 原回执。T381 隔离页先导出成功 companion，跨 `await` 后才读取失败文件。T386 的两份失败档没有可配对身份。P051 的帧预算通道可带请求 token，P013 有限频的场景日志与 `lastReceipt`，此轮都无同提交原档。单沙材质按钮属条件分支，不能替常规 T381 P013 场景作证。拼接 token 或点击时读取另一个提交的 `lastReceipt` 均不能补回这两次。

现有链已具备身份，无需改 publication wire 或增加第二套 token。

1. **导出接口**。T381 生成器加一份隔离、定容、一次预约的冷归档。物理侧复用 `DerivedCommitV2.copyPublishedInto` 所验证的 ACTIVE publication seal 和 `DerivedPublishedSlotV2`，只补只读身份快照（kind、run、stateEpoch、physicsCommitId、ledger key 的 substep/attempt/committedOrdinal、seal serial/certificate/content serial）。配对已成功 companion 的 owner/run/epoch/substep/attempt/commit，同一同步回调冻结，不跨 `await` 再读活动槽。失败单列，使用其真实失败身份和 activeCommit。沿 `DiagnosticColdFiles` 异步写同一冻结归档及 SHA。
2. **呈现身份**。预约后在正常 `copyPublicationInto→drawNewPipeline` 的同次同步调用截取 publication 与该调用直接返回的 `P013SceneReceipt`。同一 commit/ledger key、source epochs 和帧序对应，`requestedFrameToken==renderedFrameToken` 且非空、CURRENT、`currentFrameSucceeded`、`requestedSwapSucceeded` 皆成立，才记同 token 交换。用 `renderedSource` 对齐请求，LastGood、STALE、异代/空帧/错 commit 明确拒配。每帧可有多个成功子步，只有实际被 draw 选中的 ACTIVE publication 可以计 PRESENT；被后续子步覆盖的中间提交保留物理证据并标未呈现。交换确认和系统显示分列。无 Canvas 上屏或回退，前景背景走 XComponent+shader，故障走 FAULT 或完整旧 token STALE。
3. **LAND 观测**。隔离捕获用固定少量预选 ID，复用真实 Hot 事件、成功 substep 现有提交边界及 ACTIVE seal。保留源列/目标、slot/ID/sourceId/generation、event ordinal、kind、requested/granted/remainder、床量与票据变化、最终权威粒子位型或 retirement。现行 companion 只保留 reason 非零且仍有粒的行，不能用空行推断全额退役。若需补最小采集点，在已存在事务路径暂存对应事件及最终状态，成功发布才可导出，失败丢弃暂存。固定容量满或阶段缺行报告不完整。常态无额外池扫描、读回或逐帧归档，无物理写路径和 endpoint wire 变化。

## 3. 执行分流

- **developer 离线**。先完成隔离归档和配对反例（晚一 commit、同 commit 错 epoch、STALE、两子步只呈现最后一笔、失败保持 LastGood）。复用 T381 近床、故障切点及已成功前缀夹具，冻结前后权威、票据、seal/active commit 位型；核正授予部分 LAND 和全额 LAND/唯一退役事件能从新档直接判读。编译与签包，设备另卡。
- **tester 定点续证**。等归档签包，按 P3 串行使用设备，GPU 先行。同 run 记录 UP、首零获准、原列下降、首触床与 T3，记录逐个有效 dt 和实际接受角。保留原受限余量臂；再用同一路径的少粒近床夹具取得正获准及全额 LAND，或明确记录受限原因、grant 和 remainder。至少配到一个相关 CURRENT publication 的请求/实际 token 及交换。受限余量与全额 LAND 分开判定。设备回滚如另需验证，使用显式隔离注错单独取真实失败 cut 和前后状态；两份 false 回执不承担注错证明。

T388 归因与补证接口已裁定。旧 T386 两次同 token CURRENT、实际全额 LAND 和设备注错仍缺对应原证。
