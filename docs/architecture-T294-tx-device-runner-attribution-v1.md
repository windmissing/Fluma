# T294 T251 事务设备 runner 缺口首次分流 · 核证修订 v1

2026-10-30，architect；原卡 `6ae3fc72e4b031ea82c40fda`。消费 T290 正式报告、T250 BEFORE v30/T278 v8 交接与签包源树、T245/T256；不使用真机，不重跑测试，不改业务代码。本稿勘误 T290 §二第 3 点的“App 内无任何页面/Ability 入口”，不否定其全部设备字段 `NOT_RUN` 的结论。

## 原包核证

T250-before-v30 的签包 SHA `3b87eee5e12c2429fdc930d4b5af58338cf5fb96d2d36c537b19f72975a85cfd` 包含 `ets/modules.abc`；ABC 含 `TxGpuPageLauncher`、`TxGpuWorker`、`txFaultCut`、`TX_GPU_COMMAND_V1` 字符串。对应构建源树中 `EntryAbility.onWindowStageCreate→pages/Index→onPageShow→t165PageLauncher.pageReady()` 与 `EntryAbility.configure()` 汇合到 `TxGpuPageLauncher.start()`；Want 的 `txBackend/txRunId/txUnsignedSha/txFixtureId/txSteps` 校验后创建 `ThreadWorker('entry/ets/physics/tx/TxGpuWorker.ets')`。Worker 调 `runTxGpuFixture`，GPU 路径用 `libp011_gpu_pair_prototype.so` NativeTransport、AVL 路径用 TreeAdapter，结果写 `context.filesDir/txgpu-<run>.json`、fsync、回报 `TX_GPU_ARCHIVE_V1` 和真实 `process.pid`；`txFreezeArm`、`txSuite`、`txFaultCut/Root/Edge/Ordinal` 已有入口。T278-v8 源树和签包 ABC 同样存在这些接点。

因此 T290 从“Node transpile 自测使用 fixture”直接推出“设备 runner 不存在”**不成立**。包内静态可达和编译成功不等于真机启动、native GPU 成功、归档可拉取；T290 的 `device=NOT_RUN`、`sameBinaryDevicePid=NOT_RUN` 仍完全有效。当前分流是**既有 runner 的设备执行/证据缺口**，并非可证实的“从零开发 runner”工程缺口。T251 设备工作应按原前置继续，由 tester 在真机串行窗口执行；若首启出现具体 Want、worker 加载、签名/ABI、文件导出或成本字段故障，保存原错误回 architect，再精确修工程文档→developer。

## T251 可用入口与核验界线

使用同一签包/进程，Want 给合法 `WET_192_64` 或 `WET_512_64` fixture、三 backend 之一、唯一 runId、签包交接中的 build-input/unsigned SHA，并显式给 `txFreezeArm`。`txSuite=true` 在**一个 Worker/PID** 内顺序跑两 freeze 臂×三 backend；但其 suiteRows 不含故障计划，20 BEFORE/NEXT 应按各自 `txFaultCut` 独立新 Worker/run，按 T250/T278 字典 root/edge/ordinal 执行，不能把一次 suite 当 20 cut 完成。每次保存 Want、签包 SHA、PID、Worker `SEALED/FAULT`、拉出的 JSON 原字节与 SHA；先核 `config/source/build`、真实 native shader、offered/admitted/accepted 和冷档完整性。`process.pid` 是进程 PID，单 Worker 只消费一条命令；多次运行可复用同一进程并新建 Worker，须逐次核 PID 未变，不能靠签包文件名推断。

现有 Worker 的 `queueMs/serviceMs/physicalMs/archiveEncodeMs/archiveWriteFsyncMs` 和 scope/ledger 是可用原入口，`gpuTimer=NOT_MEASURED`、`runtimeNativeDriverBytes=NOT_TOTALLED`、`externalCopies=UNBOUNDED_EXTERNAL` 保持缺测；不得以 `serviceMs` 单值冒同负载 p50/p95，也不能把 CPU double 的离线 399/0 或 12/0 冒真 GPU。20 cut 须分别核真实 hit、原 firstCode/receipt/events/lineage、authority rollback/成功前缀与资源退休；AFTER OBS/NA 不混进 BEFORE 分母。设备端能跑多少按实录，设备导出/成本分段若在首次运行证实缺接点，精确回报字段和触发条件，再交 architect 分流。T245/T256 的同 binary/PID、两臂、满池 synthetic、五段成本与资源口径仍生效；外部 driver 不可见标 NOT_MEASURED，不伪造总峰值。

此项无需先新建 developer runner 卡，也不新增 manager 验收门。T251 真机可开工但目前未执行，产品设备性能与完整事务设备通过均未证。任何 UI 呈现仍只新 XComponent+shader，含前景/背景；故障 FAULT 或明确完整旧 token STALE，不走 Canvas。
