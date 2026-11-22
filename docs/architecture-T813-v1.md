# T813 · G13 探针 EGL 与角色编译隔离备选合同

2026-11-22 · 目标20 · P5 · architect · 离线工程合同

**先用 T811 修订工件跑原 RESET。** 同代进度若已定位到具体调用或首个原始错误，直接按该边分流。只有进度原件完整到足以确认 attempt，却仍无法区分 `Open` 与角色准备，且一次原样补采仍存在同一歧义，才启用下面的最小对照。进度工件、发布或导出本身缺失时，先修该证据链。T813 已给备选接线，无需等待真机结果才完成本卡。 [T808 原归因](architecture-T808-v1.md) · [T811/T812 原任务](后续工作清单.md#可立即开工)

## 已核对的原路径

T770 v7 已签 HAP `141773f5…` 的映射 BuildId `30a1c65f…` 与原 RESET 快照匹配，variant 为 `DEPENDENCY_SPLIT`。T778 可信诊断未签 HAP `fd24d289…`、BuildId `e4bc6faa…`，它与 v7 是不同工件，两包均嵌有相同的 `OP_00` 完整字节。本合同以 T811 交付的实际诊断包为下一轮基线，下一轮 SHA/映射从新回执取得。原 `Open()` 在专用 worker 上创建私有 pbuffer、非共享 GLES context。原 `CONTEXT` 写入本地事件，外部的 `CREATED` 可以覆盖到 `Prepare` 内的调用。 [T808 原件摘要](architecture-T808-evidence-v1.json) · [实际 Open/Compile/Prepare](../.codex/local-test/t767-visibility-final/entry/src/main/cpp/gpu_resident/g13_visibility/probe.cpp#L214) · [T778 编译回执](<D:/hmprojects/HelloWorld-T778/docs/receipt-T778-compile-v1.json>)

| 路径 | 已核实际计划 | 供备选隔离的生成输入 |
| --- | --- | --- |
| RESET | Q5/6 为 OP12/13，Q410/411 为相同角色的另一 scope。首次 `Prepare` 编译，第二次在同一 context 内复用 role。 | `OP_00`，**48,695 B**，SHA-256 `1b8ce7f6cb7fc077e728dda15102f95d4125efb9cc21a6d95b434ee7146ed2a4`。首轮逐角色对照只取这个 role。 |
| 后续用例 | COMPARE 的 Q814 是 OP6，PACK 的 Q817/818 是 OP9/10，均落于 `OP_00`。黏性 Q26/27 与 Q431/432 依次为 OP33/48。 | 若后续真实首边落入黏性角色，才取 `OP_18`（48,639 B，SHA `04dc4b7f…`）或 `OP_48`（48,755 B，SHA `df71255d…`）。先在原前缀顺序定位，单独跑某 role 只说明该角色自身路径。 |

旧 G13 BOOT 暖命中和 BOOT 编译用时属于其自己的角色和启动样本。ROLE 对照沿当轮编译回执登记实际完整输入，逐 shader 检查 **49,152 B** 上限。优先复用包内原生成字节，避免驱动命中身份因改源码或编译选项而变化。 [原隔离合同](architecture-T764-v1.md) · [字节门](architecture-T727-v1.md)

## 触发与归因

T811 有界进度按 `PID/start + attempt/generation + nonce + owner TID` 联核，确认证据来自实际映射的同一诊断 ELF。取最近连续序号、最新发布水位、溢出标记及一轮两次独立读取。匹配 ENTER/EXIT 使用调用种类、实际对象/角色和同次调用 ID。此处的“未闭合”只陈述观测端点，保最后原始错误和最后成功。若稍后 EXIT 到达，更新原持续时间及结果。约 30 s 的第二次 CPU 取样可复用 v7 做量级对照，仅是采样点。实际 `stat`/tick 频率、`wchan` 和可用 PC/栈随原件保留，采样受限则留原错。

| T811 实际证据 | 最小后续 |
| --- | --- |
| 读取丢代、序号缺口覆盖可疑调用、环溢出、持锁取 report 超时或导出截尾 | 先由 tester 校验原件与导出、developer 修读出或发布，再在原 RESET 上补采。缺失的 ENTER/EXIT 不补构。 |
| 已有确切 EGL 或上下文查询边，例如 `eglInitialize ENTER` 后无同次 EXIT，或 `eglMakeCurrent EXIT` 返回失败 | 保原参数、句柄、原返回、EGL error 样本和实际后端。初始化阶段按 `GetDisplay / Initialize / BindAPI / ChooseConfig / Pbuffer / CreateContext / MakeCurrent`，上下文查询细分到实际 `glGetIntegerv / glGetString`。交 developer 针对这一个调用检查配置、display/context 生命周期或驱动。原边足够时无需新对照。 |
| 已见 `CONTEXT_READY` 完成且有确切 role/调用的首个未闭合边或失败返回 | 分开 `glCompileShader` 与 `glGetShaderiv(GL_COMPILE_STATUS)`、`glLinkProgram` 与 `glGetProgramiv(GL_LINK_STATUS)`，以及创建对象、挂载、状态查询、日志读取、反射和绑定。模块 SHA/长度和实参相符后，交 developer 对应阶段。某 status 查询慢只归其原调用，编译、链接或延迟驱动工作的进一步归属需原证。 |
| 同一源码循环有直接证据 | 重复的符号化 PC/栈与 owner CPU 增量对应同一已确认源码回边，或可复现的最小路径证实该回边无法退出，才按具体源位置交 developer，给原退出条件、取消检查点及回归输入。R 态、`wchan=0`、CPU 增长和长耗时只描述状态。若缺语义确定的源码证据，保未定位。 |
| 原 RESET 在新工件已有正常短路径和安全退役，旧停滞未复现 | 保新旧工件、设备和实际差别，继续原用例顺序；本备选保持关闭。 |
| 同代 RESET 重复停滞，进度与导出已验证，仍只有宏阶段或不可区分的边 | 先选能拆掉最多后续调用的最小对照。连 `CONTEXT_READY` 都无法判定时走 EGL-only。已证上下文完成而角色内不清时，可直接走首个实际 role，不必再跑 EGL-only。若明确卡在 `CREATED_PUBLISHED`/取消前检查且没有任何 EGL ENTER，则沿发布/锁/源码边取证。 |

## 可直接实施的同包对照

**诊断入口。** 触发后才在 T811 诊断构建的 `g13_visibility/bridge.cpp` 和 `probe.cpp` 接入。三条路径共用同一个新签 HAP、诊断 native 模块、页面及唯一 `ProbeOwner()`。在现有固定 RESET 请求之外，由显式诊断选项选择下表路径。允许值固定，原 16 个用例 ID、原请求和报告语义保留，默认仍跑原用例。开始时校验选项并把 mode/case/attempt 冻结在 lease，防止同 attempt 变更模式；相同请求只读旧报告，不同设置须用新 attempt。角色由编译期固定的原 ordinal/角色表得到，入口禁止任意 shader、路径、role 名、op、buffer 或日志驱动的临时注入。新增选项只在显式诊断构建可达，沿 T778/T782 的可信身份登记和默认产品负控。 [owner 与读取原代码](../.codex/local-test/t767-visibility-final/entry/src/main/cpp/gpu_resident/g13_visibility/bridge.cpp#L14) · [T782 身份验证](<D:/hmprojects/HelloWorld-T778/docs/test-report-T782-v1.md>)

| 对照路径 | 复用执行边和停点 | 可取得的结论 |
| --- | --- | --- |
| 原 RESET | 保 `CREATED_PUBLISHED → CheckCancel → Open → CheckCancel → Prepare(5) → Prepare(6) → 原各语义阶段`。优先第一条同包参考。 | 原用例执行与复现。若遇停滞，记录实际完成的最后阶段。 |
| EGL-only | 同前缀过 `Open` 和其后的 `CheckCancel`，实际 `CONTEXT_READY` 完成即记录诊断停点并进原退役路径。保原选择 config、2×1 pbuffer、无 share context、make current、实际 GLES 版本与 renderer 查询。零 shader/program、fixture、提交、map。部分初始化失败按已取得对象走原 owner 清理。 | 原 EGL 与上下文查询路径是否闭合。此对照若在与原 RESET 匹配的细粒度边重现停滞，支持上游归因。单次成功只证明该次 EGL 路径。 |
| 单角色准备 | 同前缀走原 `Prepare(5)`，从包内实际生成字节执行 SHA、同 `Compile/Link`（保 `glCreateProgram` 原顺序）、状态查询、反射、64×1×1 与 binding/layout upload。返回后记录诊断停点，进原退役路径。零 fixture、dispatch/draw、map。需要核 `Prepare(6)` 的 role 复用时，在同一个专用诊断 attempt 按原顺序执行一次，并记复用，不从既往 attempt 复制 program。 | 可定位为源码检查、shader 编译及状态、program 链接及状态、反射/绑定或清理。单角色准备成功后原 RESET 仍停滞，则回原路径定位 `Prepare(6)`、fixture 或后续真实边。 |

**匹配和调度。** 每个 attempt 仍由同一逻辑 `ProbeOwner` 独占，复用线程创建、调度、NAPI 路由与发布器。worker 是每 attempt 新建线程，TID、新 PID/start 与 context/对象句柄分别从原件登记。优先在同机同系统与驱动、同一个签后 HAP 和映射 DSO、同实际诊断启动入口，使用独立新进程串行跑所需对照。记录 EGL attrs/config、实际 GL/EGL 版本与厂商/renderer、线程调度、设备热态与负载、app cache 配置、driver cache 可见状态及执行顺序。无产品窗口或 owner 在该诊断进程中并发使用 display，避免清理动作波及产品上下文。进程换代并不能证明 driver cache 冷，状态未知即标未知。只在分析耗时收益而非分界时才需考虑顺序影响、等价冷热配对或反序复核。原 T811 与备选新 HAP 可作历史比较；用于逐调用因果对照的原 RESET 也要尽可能来自同一新签包。上一轮已有充分原件时直接归因，不为凑配对反复进入已知长调用。

EGL-only 通过而原 RESET 仍复现且证据止于 `Open` 附近时，可接单角色准备一轮。若单角色出现同一可重现原调用，即按该调用继续。若两种对照都通过，原 RESET 仍不明，则保存三条路径各自的最后实际边，回原 RESET 补 `Open→Prepare`、角色复用或后续用例的具体接线。诊断结果随各自 attempt 保存，严禁拼接为一次原 RESET 或以通过的前缀推断后续通过。

**退出和证据。** 新路径只追加独立诊断 mode、原调用和停点，沿 T811 同代进度，零提交用实际计数证明。只在同一 owner 上清理已分配的 program、私有 context、pbuffer、实际初始化的 display 及线程状态，保原返回。无实际队列提交时按未提交分支退役，fence 字段为空。原 RESET 真有提交则维持覆盖最后使用的同队列 fence/原隔离策略。Cancel 由同 attempt 发布请求并由实际检查点确认。长调用仍在途时保 `OWNER_BUSY`、最后 ENTER 及未确认的取消，UI 超时只停止当前观测，禁止跨线程强解 EGL、销毁或放行新 attempt。原退役未证则保不能重用的 lease；在授权的串行窗口内需要结束观测时把旧进程结束与原记录分开记，新进程是新样本。

## 独立判定与交接

- **Developer** 只有触发成立才实现上述停止点及诊断身份，保 T811 当前进度与产品路径。精确调用已经足够时，按上表聚焦对应工程处。切分/RTE/数值方面沿既有 shader 合同与真实数值门。
- **Tester** 复用 T812 读出，最小增加三类负控或合成检查。分别拒绝不同签包/代混用、冒名或错 SHA 的 role、先报诊断停点后仍出现禁止提交。检查同包同 lease 的 `OWNER_BUSY`，无提交时虚造 fence，取消未确认、退役失败或导出断尾均无法升级证据。T769/T782 只给原接口所覆盖的语义与身份结论。
- **Effector** 仅在触发后按 T777 原串行设备窗口部署，先取原 RESET 的实际身份、同代进度、CPU/必要 PC、首边和 retirement，再按最小对照补唯一缺证。原 `EGL-only` 或单角色结果只给隔离诊断，不计原可见性用例、产品 OP65/66、真实 SOURCE、产品 cache 命中或 XComponent 首个有效 Swap。原 RESET 完整通过仍按 T769 用实际原字和生命周期独立判定。GPU 驻留、CPU 停投及 XComponent+shader 唯一产品呈现沿原规则。 [T777 真机窗口](architecture-T777-v1.md#设备窗口备份与恢复) · [T769 原字验证](<D:/hmprojects/HelloWorld-T778/docs/test-report-T769-v1.md>) · [T741 G20 实证计分](architecture-T741-v1.md#g20-完成度口径)

本轮从生成计划核对上述 11 个 ordinal、对应角色 SHA/字节及 48 KiB 门，并从 T770 v7 已签和 T778 未签的实际 ELF 核对 `OP_00` 字节及各自 BuildId。未使用真机。
