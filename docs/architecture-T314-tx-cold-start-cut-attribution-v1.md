# T314 · 事务 runner 冷启、阶段分母与故障终态分流 v1

2026-10-30，architect；不使用真机。依据 T304/T297 测试报告、T299 归因、T303 开发报告，`test/reports/t304/` 六份归档及 worker/launcher 阶段文件，隔离 T303 `TxGpuPageLauncher.ets`、`TxGpuWorker.ets`、`TxGpuFixture.ets` 与 T250 BEFORE v10 字典。此裁定只涉及隔离事务 runner；不阻 GPU 驻留主线。

## Want 与冷启

T304 独立冷启的 AVL8（pid43489，10 roots）、AVL32（pid46597，34 roots）、GPU_PAIR32（pid47686，34 roots）均 `SEALED/COMMITTED`，与各自 backend/fixture/steps 身份一致。AVL32 的 74 行、GPU32 的 77 行 worker 阶段文件有 ROOT_ENTER/EXIT、ARCHIVED、编码与写盘闭合，未见原错误；这推翻“这些 backend 必然在首启死锁”的旧诊断，**不能倒推 T297 每个旧 run 的唯一原因**，因为旧四个 AVL/GPU 档缺 Want/PID/worker 接收原件。

T304 同 PID 第二 Want 无新文件/日志。T303 隔离源码的 `T165PageLauncher` 是模块单例，`started` 一旦在 `start()` 置 true，后续 `configure()` 即使换 Want 也直接返回；它在 `pageReady()` 后只启动一次。该可达机制与同 PID 复用现象相符，但旧 T297 还缺 onNewWant 路由原档，不能宣所有旧失败已还原。**runner 使用合同**：每个独立 run 使用不同 runId 和新的进程/launcher 冷启，保留原 Want 字段类型（尤其 `txSteps` 为 number）、包 SHA、实际 backend/arm/fixture/steps/PID/TID；同 PID 二次 Want 不计一次执行，不用缺文件记 backend FAIL。若产品要求同进程连续执行，这是另一个明确需求，须先改 EntryAbility→页面 Want 序列与 launcher 的一次性消费、worker 退休/归档边界并交 developer；当前卡不暗改产品接线。tester 只需在 T297 报告勘误“无档/未执行”的范围，已有冷启无需重跑。

## 三个补档的原码和前缀

三档命令均 `suite=false, fixture=WET_192_64, backend=LEGACY_SCAN, steps=32`，各自 PID 与 faultPlan 的 PID 一致；cut20/cut5B 为 `COMMON_FREEZE_OFF`，cut9 为 `COMMON_FREEZE_ON`。它们的 `result.faultPlan.hit=false` 是旧 cut1/6/18 专用接点；**T250 BEFORE v10 应读 `beforeEvidence.hit=true`**，三档均有 BEFORE_INPUT/INJECTED_READSET/ORIGINAL_CHECK_RETURN 原行。

| 档 | 原始终态 | 可登记的语义 |
|---|---|---|
| cut20/root3/ord1 | `FAULT`, `ENDPOINT_COMMIT_KEY_MISMATCH`；rootRows 先 SOURCE1、SOURCE2，再 STEP3 `EXCEPTION:ENDPOINT_COMMIT_KEY_MISMATCH`；原 consumer `P011NaturalObservationProbe.stampEndpointObservation` 同异常 | 命中观察提交校验；T250 字典规定“已提交状态保留”，不是物理 rollback。须用 before/after/commit key 原字节核其特定不变量。 |
| cut5B/root3/ord2 | `FAULT`, `TX_GPU_PHYSICAL_FAILED:5`；原 consumer `collectParticlePlanInto B` 返回 **4**，STEP3 `firstCode=5`，有两条 ORIGINAL_BACKUP_RESTORE | 原 consumer 码4与父 step 码5不可混写；backup 行只证明执行路径，逐字节 rollback、事件 lineage 与成功前缀仍须 checker 对账。 |
| cut9/root2/ord1/ON | `FAULT`, `TX_GPU_ADMISSION_FAILED`；原 `selectPair` 抛 `TX_GPU_NOT_REUSABLE:STALE`，原 caller 映射 **13**；rootRows 两条 SOURCE | admission 失败是夹具“全量要求”外层码，不可当原 consumer 码；SOURCE1 成功前缀和 rollback 仍须核字典预期。 |

`hit=true`、`SEALED`、最后 FAULT 均不能替代原码、前后状态、rollback 与已提交前缀的独立判定。T299 已核 T297 cut1–19 的字典 root 命中范围，T304 补齐 20/5B/ON9；未因此得到双 freeze 臂全部 20 cut 或三 backend 故障矩阵 PASS。针对缺项由 tester 用现有归档按 T250 字典作**定向 checker/报告勘误**，不足的原字节再列精确补证，不重跑整 20 矩阵。若原码与字典冲突，先回 architect 分工程/设计；若字典把观察异常误作 rollback，由 manager 裁用例口径。

## 时间分母

T304 AVL32 的 root ENTER→EXIT 每 root 约 195–372ms，GPU32 有同构闭合；阶段文件每个 ROW 的写入都同步 fsync，观察成本进入其所在运行墙钟/`physicalMs`，不得当无观察的物理性能。`TxGpuFixture` 的 `physicalMs` 从 fixture 内首尾计时；`TxGpuWorker` 的 `serviceMs` 从 worker 收到命令到归档/写盘之后，`queueMs` 从 post 到 worker 起点，`archiveEncodeMs` 和 `archiveWriteFsyncMs` 为额外分段，worker/launcher 阶段 fsync 还会叠入各段。不能把 physical/service/单个 stage fsync 简单相加，也不能把 AVL8、AVL32、GPU32、不同 PID/不同 backend 或 freeze OFF/ON 混成同负载配对收益。T304 无同负载双 freeze 臂的有效重复分母，driver/GPU timer 仍未量测，收益保持 `NOT_PROVEN`。

本卡分流以**runner 使用工程合同 + tester 报告/原档 checker 补证**为主；当前三补档没有已证物理实现缺陷，不派盲修。若要改变同 PID 多 Want 产品行为，再交 developer 按上述版本化合同实现，保留原子性/成功前缀和 XComponent+shader 渲染限制，禁止 Canvas 回退。无 manager 复验门。
