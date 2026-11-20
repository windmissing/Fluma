# T708 · G11 Worker 装载与首帧分流

2026-11-20，architect，不使用真机。**T687-g11 内有可执行的 G11 Worker 模块。真机确有独立条目查找失败，最终模块装载与归档首次失败仍未知。STOPPED 应沿独立的首帧配置链保原回执定位。** [原件复核](analysis-T708-evidence-v1.json)

## 工件与真机原件

- 核到实际 T639 unsigned SHA 74ae28e15b01d4db170e6a7c8970327ed6365dc68c051b37c58af8e745577d77 和 T687-g11 signed SHA e237f6b671fad40a5d718839dff5130ad37c7a6cc76d59e62005709573ecc22b，均通过 ZIP 内容检查。签名前后 ets/modules.abc 的 2,805,992B 同一 SHA 18490ca7…，libgrs_g11.so 同一 SHA 36b72b1e…、BuildId bbfe2dfc…。这是包内身份，T600-v2 未归档同进程实际 loaded-module 映射。
- 两包均无独立 ets/workers/*.abc。原隔离工程的 sourceOption.workers 与实际 loader 都列入 G11ColdWorker，loader 模式为 esmodule。直接反汇编签包中的 modules.abc，确认含 com.example.helloworld.entry.ets.workers.G11ColdWorker 模块记录、可执行入口、workerPort.onmessage 安装和 BEGIN/CANCEL 回调实现，host 字节码使用 entry/ets/workers/G11ColdWorker.ets。独立条目缺失不足以证明 Worker 漏编、漏包或被签名剥除。官方示例也以模块名开头使用 .ets URL。[OpenHarmony Worker API](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-arkts/js-apis-worker.md)
- T600 原 hilog PID 48183、Worker TID 49307，09:26:22.147 开始创建。09:26:22.154 另见 EAWorkers attach limit。09:26:22.160 的 stageModule=1、三条 GetEntry/GetSafeData/CreateFileMapper 和 null safeData 确证这次独立路径读取失败，09:26:22.170–.172 有宿主终止与 Worker 退出。缺 Worker 入口/BEGIN 回执、原终止原因或归档首次拒绝，设备是否执行合并模块及两条诊断的因果仍未知。开源运行时对此类独立项读取有明确失败分支。项目 T232曾记录独立条目查找失败而 Worker 后续实际执行，只作独立历史对照。[OpenHarmony 的 Worker 资源读取](https://github.com/openharmony/ability_ability_runtime/blob/master/frameworks/native/runtime/js_worker.cpp#L407-L430) [T232 原设备报告](test-report-T232-gpu-resident-probe-v1.md)
- 包内 module.json 的 EntryAbility 源为 G11ResearchAbility，实际直接 loadContent 到 GpuResidentG11，页面用 grs_g11 真 XComponent。原件 l1–l4 保 STOPPED、无 presentedMapping、两个 seed 按钮禁用、冷档可点。按报告属于 R1/K192/V2048/ROOT 配置后观察窗口，缺该次配置原 API 参数/回包、owner 执行结果及 scope。l5 以后属于其它页面和时段。

## STOPPED 归因与工程修订

页面构造的是冷档帮助对象。只有点击 seed 原件或冷档调用 collect 才实际构造 ThreadWorker。首帧走 configure → preset → 原 64 元素 typed config → NAPI → owner 队列 → real WINDOW/EGL → loaded-module 校验 → Controller::Configure/GPU Create → bootstrap fence/raw ready → **用户封存 seed，之后再激活** → 真 session/active/pulse → accepted root → XComponent 实际 swap。导出 bootstrap 可在封存前读取四段供 W12 核验。封存/激活的原 gate 无 Worker 成功前置；配置后空等 40s 也不会自动封存或激活。现有原件的 seed 禁用无法证明实际已到 raw-ready。

已确认两个丢失首错的接点。页面每 250ms refresh 把 configure 的即时 QUEUED 或前置拒绝、归档异常覆盖为 controller 摘要。owner 的 lastReply 又会被后续窗口/归档命令替换。另有实际静态分支，owner_.Configure 非 IN_PROGRESS 后执行 Destroy(true) → Release → Stop，因而可以稳定显示 STOPPED，同时另有 CONFIGURE_RESULT.code。窗口 RESIZE 或生命周期撤权亦有各自 Stop。当前无同次配置原件在这些分支或 typed/窗口/EGL/映射/资源间取舍，先补配置可见性。

**developer** 按以下工程范围开发、编译。

1. GpuResidentG11.ets 分开保当前 controller/presented 摘要、配置回执和归档终态。配置只发一次，保档位、typed view 元素数/字节数、实际同步回包或异常。刷新时保持该次首拒。展示 STOP/resize/hide/cancel 的实际来源，QUEUED 仅代表入队。档位更换、二次配置和旧回调各带自己的尝试归属。
2. g11vis/bridge.cpp 已有 g11Status 冷读中补有界的最近配置记录，保原字段与调用方式。为同一次合法配置关联入队与 owner 进展/结束，独立保存第一个实际失败/原返回及 cleanup 后状态，用原窗口回调或原 owner 命令、EGL 创建原返回、loadedModule、CONFIGURE_RESULT 和 bootstrap 状态。入队前尚无 window/context/run 时标未知，绑定后携真实身份；晚回执不得覆盖新请求。lastReply 继续表示最近任意命令。只观察原阶段和已有返回，无额外 GL 轮询、同步等待、自动重试或新增公开端点。
3. GpuG11Cold.ets 与 G11ColdWorker.ets 的内部消息增加可验证的启动握手。同一真实 Worker 先回带请求标识的入口 ACK，早于文件/NAPI 归档操作，随后分别留 BEGIN 到达、原 native 首拒或实际文件封存。保构造、post、onerror/退出和有界启动等待的原错误或 UNKNOWN，终态只清理本请求。保持单 Worker 及原 CANCEL、End/refund、60s 冷资源界，loader 失败只影响归档状态。当前 COLD 无 token 或 publication 时可被原 native 门拒绝，文件路径尚未分配，亦可表现为空目录。

**tester** 沿 T687 离线生成和签包，核当前 SDK 的有效 loader workers、实际 ABC 可执行入口/导入、host URL 与 moduleName、签前/签后 ABC 与 native ELF。merged 按相符的可执行记录与内容检查，独立 asset 模式按实际文件路径检查。离线覆盖刷新和冷命令插入后仍保首个配置失败、旧响应不能挂到新请求、Worker 有 ACK 但无 token 导致原 native 拒绝、没有 ACK 的启动终态。若后续真机无入口 ACK 且现存原件可定位 resolver 拒绝，再隔离同 SDK、API、真实模块名的最小 Worker 装载对照，定位应修的 profile/编译模式或 URL/入口。保留 SDK 正式生成和签名路径，禁止将整份 modules.abc 改名当 Worker 文件。仅改 ArkTS 时可固定原 native；桥改后交新的真实 ELF/BuildId。

**effector** 串行真机。优先恢复同进程历史证据。缺件时先用原工件可读出口，或者已交付的诊断包，在实际 G11 页先取一次完整配置及原始 g11Status，包括 lastReply、scope、loadedModule、scene、bootstrap/presented 及实际支持的配置诊断。这段先不点击归档。保首个拒绝/开放阶段、实际回调和窗口时序。实际 raw-ready 后按原序导出 seed、封存、再激活，记录 session/active/首 pulse/同 scope 首 accepted root 与 swap。独立 Worker 启动握手不触发归档、无需假 token；真实归档另保 Worker 回执和 native 首拒。W02–W12 仍按原矩阵执行，物理和像素 NOT_RUN。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（本人状态有限例外）。本卡不使用真机。GPU 驻留，CPU 停投，目标10暂停，前景背景仅 XComponent+shader，故障报 FAULT 或保上一有效帧并明确 STALE。
