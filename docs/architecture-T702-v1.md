# T702 · DM CREATE 与 V5 NOT_READY 分流

2026-11-20，architect，不使用真机。DM 已有独立入口实际启动及 shader 路径失败，交 developer 修 DM 生成链。V5 已有 ROOT Configure 接受与后续三次 controller 缺席观测，首个初始化阻断由 effector 用现成 T653 ticket 档定位。两条链分别登记。[机器裁定](architecture-T702-contract-v1.json) [原件复核](analysis-T702-T705-evidence-v1.json)

## DM · 实际 CREATE 失败，修订独立生成链

- T569 v2 的 `dm20-execution.jsonl` 仅两行，实际 `g9_dm_differential` 到达 GL，Maleoon 935F、K192、resident 55,339,648B、fullArenaBytes 0，返回 CREATE result 8。T670 DM 实际 `transport.cpp` 把该返回映射到 `programcache::Build(1, PipelineSource(), …)` 失败，枚举为 SHADER_FAILED。此轮 DM20 已失败，DM22/29/31 保 NOT_RUN。T685 的 EntryAbility 错入口是另一轮旧证据。
- T569 报告和收据记同窗 hilog `0:1728 S0022 Symbol 'slot' redeclared`。当前交接有该签名收据，缺独立原始 hilog 与进程加载映射。两份实际签包 T569-dmfix/T670-dm-final 均嵌相同 DM ELF f63f4cc9…，ELF 内部包含完整生成 GLSL 字节 a85f6da8…。该源码在 `g9Capture` 1725 用 `slot` 清槽、1727 用单语句 `for(uint slot…)` 扫描、1728 紧接声明 `uint slot=best`，与报告行号一致。证据支持优先修 DM 编译器兼容性；加载 ELF/源码的设备原件仍缺，原始编译/链接返回与日志需随复测。
- 工作区同路径的 `g9_dm_differential` 实测 SHA **7c8284b9…** / BuildId **bf832f6f…**，T670 原收据为 **db325543…** / **ee6dead0…**。T569 报告声明旧标识并提到 INPUT_LENGTH 调试 throw。无法从现有原件判定实际跑的是哪一份 ELF；包内私有库字节与独立 ELF 分开登记。tester 在复测前重出 ELF 及独立部署收据，并对照原 `DT_NEEDED`、argv、配套库的 loaded SHA/BuildId、进程起点和原输入。优先核 `dm20c-input` 的 591,872B seed 和原 manifest，`dm20b-input` 的 37,584B seed 是旧设施失败候选，不能复用为正确输入。

**developer 实施面**为 `tools/g9dm/generate_assets.py` 中写出 `pipeline.comp` 前的 DM 变换。对 `g9Capture` 的清槽、候选扫描、选中项分别使用作用域唯一的 `clearSlot`、`scanSlot`、`selectedSlot`，给扫描循环显式块，完整替换对应槽引用。保持候选集、best 搜索及平手规则、初始与更新次序、per-ordinal 控制、内存屏障、coherent、ABI、config/seed 和 DM 物理语义。变换须对预期旧片段唯一命中，冻结的 corrected_reference 与 parent_input 保原字节。通过已有生成器、CMake 打包和 cache 身份绑定同步派生 comp/header、DM identity/manifest、reader 字典和源码 hash；编译后核独立 ELF 与配套 DM 库实际嵌入字节及 program-cache 输入。沿 RX 的名称兼容经验，变更只落 DM 生成链。T691 RX coherent 是独立失败证据。

**tester / effector 接力**沿 T670 的独立 DM 可执行文件与原权限、预算、输入协议。tester 离线准备生成、签名、reader 和逐文件身份清单，effector 串行真机从同一 DM20 条件取启动命令、加载身份、stderr/stdout/exit、execution.jsonl 和当次原编译/链接错误或成功回执。随后按原矩阵实际执行 DM22/29/31。仅通过 CREATE 支持启动层结论，DM_NATURAL 的接受/拒因、冷档和物理差分仍用原 oracle。

## V5 · ROOT Configure 接受后，先找同 ticket 首个停点

- T669-v5 实际签包 ce9ba9af…，包内 T653 libgrsm1 3,201,504B，SHA 35ac238c… / BuildId 7f31c1de…。实际 V5 页面接受 ROOT Config32，`Uint32Array` 32 元素/128B、offset 0、word15=0。T582 原件对应 boot **331203009245315**、diagGeneration **2**、m5Configure attempt **2**、requestTicket **1**。`v5-m3b-first` 的 NO_FIRST_FAILURE 仅属于这笔输入 API；last/last2 归同一请求。工作区根也保有 `v3-v5-entry.json` 执行摘要；t594b 下的 `v5-boot.json` 为 0B。
- 布局 cB/cC/cD 各自保同 boot/gen 的 `m5ColdCapabilities` API_SEMANTIC/M5_NOT_READY，调用 attempt **3、4、5**。这些只读调用的编号不同于初始化 ticket。对应 T653 bridge 分支在读取时没有 controller；已有 controller 的能力序列化若不支持，走 UNSUPPORTED_M3_COLD。当前可判三次采样时 controller 缺席，首次何时进入、是否短暂发布后撤销、哪个原生阶段阻断、是否由页端终止仍未知。FULL 资格、本轮物理运行与冷档完整性保未测。
- 冻结包对应 `GpuM5Init.wait` 使用 `m5Session()` 的 JSON 等待值，再读初始化页。源码正常未就绪返回 `{"state":"M5_NOT_READY"}`，可正常解析。真实创建链在原窗口/EGL、Configure、Scene、同代发布后才进入 `M5_WAIT_PAGE_PULSE`，session 后 attachFrame。页面真实启动失败或超时会先调用 `GpuM5Init.archive`，随后 `setActive(false)` 可能取消当前 ticket。30s 后的无 controller 也可能处于该 cleanup 之后，现有原件无分流证据。

**现有出口续取，优先恢复原进程文件。** 按 T650/T653 查应用 filesDir 中 `m5-init-1-*.json`、客户端 `INIT_*` 回显及当时真实 session/清理原件，绑定实际模块和进程起点、boot/diagGeneration/configAttempt、一份 Config32 SHA、ticket、epoch/window/context/owner。若旧原件确实取不到，用 T669-v5 真实 GpuResidentM5/XComponent 页新一次 ROOT 正控，记录 Configure 准入、原 session、真实 `m5InitDiagnostics(ticket,0..7)` 回包及原有客户端 fault/timeout/cancel。保原 450 polls/15s 行为与既有窗口，观察一次自然运行及正常清理，禁止手动 reset、route switch、代发 pulse 或延迟清理来补证。先利用现成 archive；实缺导出时，tester 只把现有只读出口接入同页冷采集，固定原 native，effector 执行。每页 2048B 原数据、原响应上限 4096B，完整档需同 boot/ticket/snapshotRevision，变更或过期逐页保原件。

**分流判据**：最后窗口/退休切点开放，则核同代 owner/窗口与真正 retirement；Configure/程序/Scene 有首个非零实际返回，按准确原返回和对象交 architect 修该段；已发布再被同代或更高代撤销，查 cancel/退役因果；仍处开放阶段时输出截尾。页面终止后仍无可验证的 ticket 档则保 `INIT_EVIDENCE_INCOMPLETE`，上报实际出口错误与加载身份再决定是否要产品观测修订。T541 SPF WAIT_RETIRED、T593 正式 M1 CURRENT、T594 RX 各保原 scope。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。GPU 驻留、CPU 停投、仅 XComponent+shader 渲染，故障保 FAIL/明确 STALE。developer 开发、编译通过，tester 离线，effector 真机串行。
