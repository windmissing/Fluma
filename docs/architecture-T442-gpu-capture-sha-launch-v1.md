# T442 · GPU 预约签包身份冷启动入口

2026-11-09，architect，P5。不使用真机。依据 T410 attempt2、T421 源码及 T438 显式体验入口，裁定诊断预约的 SHA 从实验构建的冷启动 Want 注入。

## 归因

T410 三份成功档分别为 V1/epoch2/run1、V2/epoch3/run2、V2/epoch4/run3，均归档了对应 captureId、FROZEN 预约回执和 T421 完整 SHA。此后报告中的整串、分段、回车和逐键注入仍有 `INVALID_CAPTURE_CONFIG`，部分框内文本据报正确。成功档证明该路径曾传入有效配置。失败档缺按键当时的 ArkTS `onChange` 序列和九字段校验首错，无法复原实际 SHA 状态，也无法判定早期成功来自 IME 竞态。 [T410 阶段报告](test-report-T410-device-v2-progress-v1.md)

`GpuResidentM1.ets::armCapture` 用 `packageSha.trim()` 和当前 status 生成九字段 config，唯一 SHA 写者是诊断 TextInput 的 `onChange`。框显示和调用参数之间没有独立回执。 `evidence_capture_napi.inc::EvidenceConfigArg` 同时验证 JSON、九字段类型与 `EvidenceConfig::Valid`，在调用 `ReserveEvidenceLocked` 前便可返回该错误。因此这个错误不能单独证明 SHA 失效，也不能归为安全点、租约或 GPU 期限拒绝。工程缺口是实验取证身份依赖实时编辑回调，且原错误无法区分字段。

## 实施合同

1. 在实验构建的 `EntryAbility.onCreate` 冷入口接收 `grsm1=true` 与字符串 `gpusha`。仅在明确允许诊断采集的构建 profile 装配此入口。复用 T438 的能力边界。解析要求恰好64个 ASCII hex 字符，不接受空格、换行、截断、Unicode 或数字型参数。通过后存 lowercase 的冷启动声明。页面取得其只读快照，TextInput 降为只读展示。诊断时间线与预约共享该声明。 `onNewWant`、其它路由、前后台、重建窗口或页面输入均无权覆盖它，另一包或另一身份须冷启动新进程。
2. 宿主先对**实际用于安装的最终 signed HAP**计算 SHA256，与签包记录逐位比对。归档文件名、字节数、SHA、成功安装回执、目标 bundle/version、设备及启动记录。已装包来源无法对应时，先由 tester 用同一核验文件重装。Want 字段是宿主声明，应用只证明传递、冻结和消费了该值。不能把它当设备自行测得的 HAP 哈希。禁止把待签包的自引用 SHA 硬编码进 HAP，也不以版本号或源码 SHA 代替最终包哈希。
3. 读取一次当前状态，核 activeRevision、epoch/run/session 可用且是本页代。构造原九字段 config 后保原 native 验证和当前 scope 安全点复核。缺/坏 SHA 在调用前给可区分原因。其他配置错误给有界的失败字段/类别诊断，保留旧顶层错误码。冷回执包含来源、校验结果、实际生效 revision/run/epoch/session 与该次 requestId。冷诊断不得伪造 READY 或消费预约。有效 SHA 与实际安装 SHA 的一致性由宿主侧 checker 核验，任何合法64hex都不自动获得包身份可信结论。
4. 接受后按 requestId 保存本次 SHA 和 scope。只让同 request 更新该 captureId 的导出身份。取消、换代、旧回调、无效参数或新请求失败保留上一冻结档与原回执。晚旧状态仍由现有 native 门拒绝。原2000ms配置、1s安全点窗口、GAP、250ms和采集预算沿 T417/T421 执行。成功启动或填好身份都不触发自动预约。

## 交付与续测

developer 在隔离构建中独占 `EntryAbility.ets`、`GpuResidentM1.ets` 及最小实验接线。T438 的 `GpuSandWaterExperience` 是独立页面，保留其常规用户入口，诊断取证沿 `grsm1` Want 进入 T421 预约页。实验配置可复用 T438 命名 profile，但须在实际生成来源核路由，不能凭同版本号假定带 T434 复制改动。T437 若需要此入口，取同一冷注入改动叠到 T434 双路生成源后签独立包，同包内做完整/前缀对照。既有签包和原件保留原身份。实际页面路径做非机正反控，覆盖 cold Want→原 NAPI、普通构建拒绝、无 SHA、63/65位、空格/大小写/非ASCII、合法但与宿主哈希不符、onNewWant 试图替换、旧 session/run、失败不调用或不改变预约/冻结档、同包冷启重复可用。产出新签包 SHA、可复跑安装与启动参数、非机结果。开发任务不使用真机。tester 另卡按真机串行核冷启动身份和新 run，随后续 T410 双源、六负控和30fps。工具格式修复 T443 可并行。

执行者完成本卡范围和本角色必要检查，局部受阻先完成其余。失败报告原错、证据、已尝试及真实前置。不允许修改后续工作清单或滴答同名清单，本人行状态是唯一例外。共享文件串行集成。渲染保持 XComponent+shader，故障为 FAULT 或完整旧帧标 STALE。
