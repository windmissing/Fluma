# T308 · V2 watchdog 粒子路径首次归因与工程修订 v1

2026-10-30，architect；不使用真机。依据 `docs/test-report-T285-v2-cold-directed-v2.md`、`test/reports/t285/tb62689.log`、`tb64350.log`、T284 隔离签包源码 `.codex/local-test/t284-v4/entry/src/main/ets/physics/P011ParticleHot.ets`、`P011ParticleM2Sink.ets`、`P011ParticleM2Field.ets` 及 `docs/developer-T284-bounded-cold-v1.md`。当前工作树中对应两份生产物理源码 SHA256 与 T284 隔离源逐字一致；本裁定只针对 T285 所用签包及 CPU V2 路径，不改变 GPU 主线优先级。

## 已证事实与边界

T285 两次独立 run 的 `THREAD_BLOCK_6S` 均在粒子 `applyParticlePlanIntoBody → sink.applyInto → finishPatchHot → patchLeaseHot → fieldStorageValidHot → ownsBufferHot`；pid62689 发生于 preview/planning 调用链，pid64350 发生于 step 的 motion channel 调用链。主线程长时未响应并被 watchdog 终止成立。两个栈采样落在 `ownsBufferHot` 的线性比较，只能定位**被采时的执行点**，不能证明单次扫描本身耗时六秒，更不能证明它是唯一瓶颈。`GC Status` 最大 pause 分别 9.166/9.514ms；`DireAllocate` 约 215MB、SharedHeap Used 约 7.3MB 是压力线索，不能据此认定六秒全由 GC 或内存泄漏造成。故障日志没有每次 patch 数量、各阶段累计时间、绑定数或冷采调度首码。

两次栈均无 `t276Cold/T276Export`，本次无 `T276_COLD` 分片，支持“这两次死亡时的采样栈不是旧的每帧同步冷档导出”；它**不证明冷采在所有触发点已通过**。T285 的 FIRST_BIRTH 早期 CURRENT 证据可保留，首生后 ≥8 秒、源停后时钟、LAND→床量→publication→同 token 像素均 `NOT_RUN`；DD12/G5 未通过。T285 报告中“冷采阻塞已消除”只按上述有限含义登记。

## 绑定生命周期和累计工作界

`bufferBindings` 在 workspace 构造时注册 30 个固定工作缓冲、四组各 13 个粒子快照缓冲，以及 pool/plan/result/event/mergeIndex 缓冲；M2 sink 构造时继续冷态注册 budget/refund/guard 缓冲。`registerColdBuffers` 仅允许 `active===0`，并在注册前线性查重。T284 M2 路径没有帧内注册调用，因此无“每帧追加导致绑定表无限增长”的源码证据；接口仍可在非活跃态被再次调用，developer 应封闭注册期或将额外注册视为版本变化并拒绝热态漂移。设冷态完成后的绑定数为 B，至少 82（实际还包括多组容器与 sink 缓冲）；外部 field 有 F=8 个 buffer，`ownsBufferHot` 对每个 field buffer 最坏查 B 个，`fieldStorageValidHot` 单次最多 8B 次身份比较，另有 28 次 field 内互异比较与长度/绑定身份检查。`storageValidHot` 另扫描 B 个长度，并核多组 buffer 与 sink 内部绑定。

每个成功的请求行 R 经 `preparePatchHot`、`publishPatchHot→patchLeaseHot`、`finishPatchHot→patchLeaseHot` **至少三次** `storageValidHot + fieldStorageValidHot`，即仅 field 对工作区的线性身份比较上界为 `24BR`；整步入口另一次。`preparePatchHot` 的两种降级/剪裁重试可能额外调用；异常/回滚也增加工作，不能把 3 次当全路径上界。若 B 只取确定下界 82、R 取配置上限 2048，三次已对应最多约 403 万次身份比较的量级估算；实际 B 更大，且这是最坏比较次数而非 T285 实测次数。每次 `storageValidHot` 的 B 扫描、容器级校验和 `txD1.syncContext/trace` 另计。

更大的放大来自**每请求全池工作**：sink 入口、prepare、publish、sink 行审计各有一次 `copyPoolDiagnosticHot`；prepare 另做两次 `copyPoolHot`。行审计 `auditParticleHotBody` 再扫描空槽/全部 N 个槽、对活动粒排序 `O(N log N)`、扫 C=128 列并做 budget 审计/derived flush。T285 配置 `N≤512, R≤2048, M≤512, eventCapacity=4096, lineageCapacity=8192`。成功路径仅上述可见部分的累计界为 `O(R·(B + N log N + C + derived_flush + budget_audit) + M·lookup + plan_compare + step_tail)`，至少有 `4R` 次全池 transfer 和 `2R` 次全池快照拷贝；对 N=512、R=2048 的容量上沿就是数千次全池复制/排序。`R`、活动粒数、重试数在 T285 故障窗口未记录，故不能把容量上限写成实测工作量，也不能从单一栈顶排除排序、copy、derived flush、预算审计、preview 重算与事务跟踪的累积成本。缩小 dt 或仅替换 `ownsBufferHot` 不能作为本轮完整修复。

## 分流：工程问题先交 developer

现有每行多次重复验证整套**冷态不变的 buffer 身份与大小**、每行全池 copy/排序，是可由源码确定的工程复杂度放大；先按工程修订开发，不转 designer 调物理参数。developer 在隔离 T284/T285 续证分支及对应生产物理模块实施一个**整体有界**版本：

1. 明确冷注册完成时点和 B 上限，固定字段/工作区的身份、byteLength 与别名校验。把静态绑定表检查移到每个 step/attempt 的可信入口；每个 patch 仍逐次检查 token、epoch、state、请求身份及可变 field/typed-array 引用，禁止用缓存把跨入口非法替换、detach、长度变化、字段互 alias 变成成功。若运行环境允许外部在同一 JS 同步调用栈内改写 buffer，应说明可达性和保留的必要逐 patch 检查。拒绝坏输入的码、无部分提交、rollback/事务原子性与确定性保持。
2. 对整条行路径定预算，而非单点盲优化：列出 prepare/lease、pool transfer/copy、audit 排序、budget、derived flush、preview 重算、trace 的逐步调用次数和 `R,N,B` 依赖；减少每行重复全池工作时保留每个请求的守恒、容量、身份与异常回滚语义。若某次全池审计无法安全合并，报告下界与主线程预算冲突，回 architect 做设计分流，不以跳过审计换速度。不得修改物理 dt、落床阈值、endpoint 协议或用 Canvas 回退；渲染仍只走新 XComponent+shader。
3. 加小而固定的标量阶段收据：同 run/step/attempt、preview 或 commit 身份、B、R、N/live、重试数、每类 validation/copy/audit/derived 调用次数与各阶段累计及最大耗时、冷采排队/触发/完成/超预算状态、GC 可得计数。收据输出频率和字节总量必须有界；重日志不得每帧同步导出大对象。正常帧无冷分片不等于冷采成功，触发锚点要能区分未到点、排队、跳过、失败。

developer 的本角色检查：编译签包；非机覆盖注册期/别名、detach/替换、错误码与 rollback、重试、请求顺序、守恒和结果位型；用 R 与 N 的多个容量档核调用次数及累计增长，验证修订前后同输入提交/事件/粒子结果一致。开发交付后 tester 再持新签包真机续证，不把设备执行加作开发关闭门。若需要先补精确设备规模，只取同一原故障窗的 `R,N,B,attempt,phase` 与阶段累计/最大时长的有界标量；现有日志缺这些，不要求重复采巨型冷档。tester 应核两个栈路径、watchdog、首生后成功物理秒和指定锚点完整性，若仍失败携最小阶段收据回 architect 首次归因；effector 后续按独立任务验观感。

## 接力结论

T308 首次归因为**工程复杂度与证据出口缺口**：重复绑定校验和每行全池工作有确定源码证据，哪项占六秒尚未量测。给 manager 派 developer 整体修订，再派 tester 真机续证；T285 的失败不记产品 PASS，也不阻塞 GPU compute 主线。若 developer 证实在保持事务/守恒下无法满足主线程预算，再由 architect 明确设计约束转 designer，不能先调 dt。无需新 manager 验收门。
