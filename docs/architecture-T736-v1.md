# T736 · RX / DM BOOT + SOURCE 接续工程合同

2026-11-20 · 目标20 · architect · 离线准备。

> 2026-11-21 T754 补充（DM）。T751 正式次以 T748 打包 DSO 运行，`DEADLINE=6/9` 落在第 12 次 dispatch/barrier 提交后、执行 fence 创建前。修订诊断按最后 Dispatch post-guard、Pump flush 前后及 pre-fence 分段，保同根 `now/deadline`、last pulse、API 原边、原始退役 fence 与终态。未到 map 的数值默认值保未测。部署须对实际 `driver` 和 `pairedDsoFromHap` 的文件字节、完整摘要与映射 BuildId，`linkedDso` 保为独立未剥离工件。冷 SOURCE build/reflect 单列时长。250 ms、33,333,333 ns、真实 typed gate 和原 12-pass 规则保持。 [T754 首次归因与复测合同](architecture-T754-v1.md)

在 T734 之后增加一个显式 `BOOT + SOURCE` 研究变体。每族独立验证一次原 12-pass 非零 SOURCE，以真实 GPU 尾、同根 typed proof 和正增量验收。新变体只允许 SOURCE，单体对照继续走同一族的原生产 math、布局和数值判据。RX 复用现存研究页和 XComponent，DM 首轮使用实际原生 Controller/Transport 入口。T736 未运行驱动或设备，以下输入和期望为实施合同。 [T733 拆分方案](architecture-T733-v1.md) · [机器合同与静态核对](architecture-T736-source-contract-v1.json)

## 真实入口与最小非零输入

| 项 | RX | DM |
| --- | --- | --- |
| 入口 | `GpuResidentG9Rx.ets` 的研究配置 K192，原 `g9RxConfigure` → 同 ticket `APPLIED` → session/active/pulse → 原 `g9RxSource` → `OfferSource/BeginRoot/PumpBounded`。配置成功后先确认可用 SOURCE 和原 scene/geometry 资格。 | `libgrs_dm_t647.so` 的 `Controller::ConfigureResearch` → `OfferModelCommand`、同 scope 的正常 `Pulse` → `BeginRoot/PumpBounded`。通过现有 native 构建入口链接真实 EGL owner。正式 DM 库的 legacy NAPI 未注册，独立 runner 需要绑定该真实 Controller；已有 pbuffer transport 差分仍单独保留。 |
| 配置与种子 | T641 GNR1，profile 827477575，INPUT magic 1481788999/rev3，用本族 `g9RxResearchConfig(192)`、`RESEARCH_ONLY`、S40 T641 K192。 | T554 DM2，profile 843924551，INPUT magic 843662407/rev2，`kConfig192`、`fullTrialTrace=false`、S40 T554 K192。此入口接受已具名的 synthetic model 输入。 |
| 输入 | 沿现有按钮的一条原始命令。新 session 中 intent 为首个有效非零序号，`sourceId=2,mass=0.2,x=64,y=200,vx=0,vy=0`，按原 JSON 字符串数值进入 NAPI。真实 commandSeq/receipt 由 RX native 分配。 | 同物理值用本族 `Input128` body，kind word40=1，首条命令 seq=1、独立 sourceIntent=1。经 `OfferModelCommand` 生成本族 pose/clock/receipt，再由原 InputGate freeze。 |
| 冻结内容 | words64–73 为五个 LE binary64，word74=2、75=1、76=63701。 | 同五个 LE binary64，word74=2、75=1，word76=0。 |

每个 arm 从全新 scope、已 GPU 审核的同族种子、K192、angleIndex 0、无触摸和前序根起步。实际 seed 为 591,872 B，模板和 Config64 SHA 已离线核对。模板启动后按本族原规则填真实 run/owner/context/inputEpoch 和 parameterSHA，再记录实际上传种子 SHA。其余 INPUT128 字、128-word Config、nonce/plan/几何/时间由现有入口冻结。合法零位保持原值，word40=1、root kind=1、ordinal=0。保留原件中的动态 scope，禁止跨族共享 Config 或把例示 seed hash 贴到实际上传。DM T718 用的 T712b 历史 `commands.u32` 实测字面是 word138=2、word40=0。可复用其合法模板、构建和 T670 runner，但 tester 须产生一份新的正 SOURCE 行，成功的 transport 差分只证明 GPU/typed transport 域。 [RX 页面](../entry/src/main/ets/pages/GpuResidentG9Rx.ets) · [RX native](../entry/src/main/cpp/gpu_resident/g9rx/bridge.cpp) · [DM 输入门](../entry/src/main/cpp/gpu_resident/g9dm/input_gate.h) · [DM 构建入口](../tools/g9dm/README.md) · [T641 配置种子](architecture-T641-seed-config-v1.json) · [T554 配置种子](architecture-T554-g9-dm-manifest-v2.json) · [T670 离线交接](test-report-T670-offline-handoff-v1.md)

### SOURCE-only admission 和 DM 退役顺序

配好 SOURCE program 后，RX 研究变体在 session 激活后、首个合格 pulse 建立 baseline 时发送原页命令。通过原 NAPI 方法和 native clock 顺序，避免依赖人工在一个刷新间隔内点击。DM runner 同样在真实 clock/baseline 下排入命令。真实 scene/owner 准备好后再武装诊断。保 FIFO 队首、prior root 未发生、中性角、非零 intent、fresh pulse 与 `pending==0`。持锁在 `ClaimLocked` 前检查这些条件，同时在 `Transport::Begin` 防御性拒绝 kind≠1。短臂只放一次 claim。若首次 claim 之前已积攒 QUANTUM 配额，保真实原因，按原 Stop/retirement 开新 scope；不强清 quota 或执行未装载的 QUANTUM。claim 后真实 pulse 可继续，新增 pending 与本根 `activeRemaining==0` 分别记账。收完本根 cold/呈现证据后按原 Stop 收口。

T748 已按下列顺序修订 DM 零配额提案。T736 初审发现旧 `BeginRoot` 把 `run_.workspaceRetired` 置 false，而 `PumpBounded` 在设置 true 前，对 copy 的 `nextRun` 调用 `RetireSourcePlanLocked`。该函数要求 workspaceRetired 为 true，旧路径会将合法输完的 SOURCE 作为 `STALE_REJECTED` 撤权。实现沿 RX 现有处理，只在 fence/读回、typed proof 与 fresh permit 验毕后，在**局部提交提案**上置 `nextRun.workspaceRetired=true`，完成零配额 plan 退役预检。随后原 `CommitAccepted` 再核真实时钟/permit 并原子晋升，成功才发布 nextRun/nextClock。失败继续按原 fence 退役真实对象。T751 超期时还未到该预检，零 quota 提交留待实际 Controller 验证。单跑 T670 `Transport::CommitAccepted` 不能覆盖这一调用。 [DM 调用处](../entry/src/main/cpp/gpu_resident/g9dm/controller.cpp#L98) · [原退役先决](../entry/src/main/cpp/gpu_resident/g9dm/progress_clock_v3.cpp#L107) · [RX 参照](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L575)

## 角色、反射和绑定

从真实最终拼装 GLSL 分别生成两族 BOOT 与 SOURCE。复用 T733 opcode 闭包和原生成器，计算正式 `glShaderSource` 字节时包含全部拼接内容，并在构建时拒绝大于 49,152 B 的单元。T733 的 SOURCE 离线原件 RX **41,767 B**、DM **41,802 B**，重取签包内 shader 后再生成，两者 glslang 链接反射都得到 40 个 active uniform 和四个 block。编译器输入 SHA、完整 uniform 名/类型/元素数在机器表。实际 GLES program 在真实 owner 下用本 role 本 context 反射和赋值，支持 T475 load 后同样校验；验证 `std430` block 0–3、1 个全局组 × 局部 64 lanes。缺实际必需 uniform、类型/维度或 block/binding 不符时拒绝 admission。仅允许静态证明无影响的 inactive 声明消失。SSBO 名/别名沿 ROLE 绑定表，宿主 `Common`/typed setters 使用该反射表并按每个根刷新。

资产生成、native transport/controller、私有 completion gate 与 cold manifest 按族接线，RX 页面只显式选择研究变体并复用原输入。BOOT 资产沿 T734，T733 参考 RX 22,318 B、DM 22,353 B，取实际构建结果重新登记。Vulkan 候选沿 T731 共用算法输入、pass/binding 和 oracle，将 copy/dispatch/readback 顺序映射到实际后端同步，缓存和已链接程序身份分后端验证。 [T731 同机对照](architecture-T731-v1.md)

下表 slot 是 owner 真实 buffer 数组。默认新 scope 的 CURRENT=0、CANDIDATE=2、arena=0，按冻结 permit 替换动态索引。B=ReadState/read-only，C=WriteState/coherent，W=Workspace/coherent，E=EventPath/coherent。原 op0–11 均装在同一个 SOURCE program，`uPass=uOp=ordinal`、`uRole=0`、`uJslot=0`、`uMicro=uint(-1)`。这 12 次之间没有计划角色切换。

| Ordinal | 原 opcode | B / C / W / E | 既有消费与宿主动作为准 |
| ---: | --- | --- | --- |
| 0 | SOURCE_INIT_BASE | 1 / CAND / 5 / 6 | dispatch 前 CURRENT→1 完整 state copy；清 controls/summary、建 pass 游标 |
| 1 | SOURCE_VALIDATE_INPUT | CURRENT / CAND / 5 / 6 | GPU 并行复制、清私有状态，lane0 核 Config/Input 与初始几何/质量 |
| 2–3 | SOURCE_DEDUP / SOURCE_DECODE | 1 / CAND / 5 / 6 | 原命令 capsule 去重与 binary64 解码、净空/容量/质心裁决 |
| 4 | SOURCE_CREDIT | 1 / CAND / 5 / 6 | 原 marker，保通用 pass 校验 |
| 5–7 | SOURCE_PRIVATE_BIRTH / SOURCE_ORIGIN_CERTIFICATE / SOURCE_LEDGER | 1 / CAND / 5 / 6 | private birth 与前缀 sealing、来源/动量审计、命令/提交元数据 |
| 8 | SOURCE_AUDIT_REPLAY | CURRENT / 4 / 5 / 6 | 从同一 CURRENT 独立重放，复用 E，保游标一致与真实重放审查 |
| 9 | SOURCE_COMPARE | CAND / 4 / 5 / 6 | 比完整状态；dispatch 屏障后完整 E 6→7 和 6→8+arena 两次原 copy |
| 10–11 | SOURCE_PACK_SCENE / SOURCE_SEAL | 1 / CAND / 5 / 6 | 原 audit mask，再由实际 SOURCE program 写 256-word tail，seal 最后 |

每次 dispatch/copy 后沿用 `SHADER_STORAGE | BUFFER_UPDATE`，保 shader 内同步及宿主 `Allowed()`。原 `NativeEvidence.barriers` 只计 12 次 dispatch barrier，3 次 copy barrier 另记。宿主 `Begin` 各上传一次 512 B frozen INPUT 与 512 B Config。每 Pump 最多 10 pass 和原 8 ms 检查点，沿原 Flush、250 ms deadline、33,333,333 ns pulse freshness。第 11 ordinal 成功后单根只建一次原 fence。实际 fence success 且 scope 仍当前才从 `WORK profile.work[8].begin` map 1024 B。原证据另核 unmap、GL/EGL 无错和 fence/commit 时间。owner 之外无 GL 调用，运行时无角色编译或跨线程等待。 [RX 原传输](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L402) · [DM 原传输](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L403)

## 逐根身份和失败事务

- BOOT 和 SOURCE 使用独立可缓存工件。冻结 T733 定义的 canonical 计划 `roleSetSHA`，声明只支持 SOURCE，绑定后端、族、变体、原 pass、binding 以及两份 shader 字节。Owner 在 admission 将真实链接/反射合格的 SOURCE GLuint、来源 SHA 和 context 绑定进私有 flight。每 ordinal 验映射、实际绑定、游标与 dispatch/barrier 结果，SEAL 必须是本根同一 program。记 scope 为 run/owner/thread/context/surface/geometry/config/inputEpoch/session、ticket、nonce/plan、bank/arena generations、输入、参数、seed、layout、roleSet 和实际 shader。现有 `Permit.shaderSha`、Config[32..39] 与 GPU Completion[80..87] 使用实际 SOURCE SHA。DM 仍由 T647 比 permit 和 observed program，并把 split `activePipeline` 独立绑定实际计划；RX 由现存私有 CompletionGate 增加等价 plan/role 收据校验。导出时按变体选 trusted schema/manifest。冷 reader 从实际版本化 manifest 校验 SOURCE 和 roleSet，原单体 reader 继续校自身 SHA。 [T647 接受合同](architecture-T647-v1.md) · [RX 冷读](../tools/g9rx/read_cold.py) · [DM 冷读](../tools/g9dm/read_cold.py)
- fresh scope、原 queue head 和原完整命令须在 commit 前同短锁重验。所有可拒绝的队首/配额/代号预检先完成，才调用 `CommitAccepted`。outcome1 晋升实际 CAND，记录提交序和消费一次；outcome3 合法 q0 可提交元数据/消费一次但无新质量、来源或 packet。两者 quanta 不增。outcome2 仅允许原精确重复语义，保原 bank/commit/quanta，无新有效内容。普通入口可能在 CPU FIFO 去重即返回原结果，只有真实 GPU 重放收据才记 GPU duplicate。SOURCE 不能借 QUANTUM 的 outcome4 证明正常 blocked，FAULT5、坏封样、错 program/pass/hash、容量拒绝、时钟/owner 撤权以及迟到完成按真实错误退役，禁止以任何失败证据晋升候选或消费命令。原明确的过期丢弃单独记取消，保留失败前后队列/commit/arena 证据。RX 若曾呈现，保上一已有效帧并标 STALE。
- 不同 A/B 进程允许不同合法的 runtime ID 与绝对时间。每个 arm 内逐根与**自己的**实际映射、frozen INPUT/Config、permit、fence、tail、cold、commit 和原 scene token 精确绑定。跨臂对齐同物理 seed 内容、Config64、角度与命令，逐字对比值域和事件有效载荷。受动态身份影响的 header、nonce、scope/gl handles、源签名、时戳和包含它们的摘要按原字段单独验证，保留原件再规范化对照，禁止用 SHA 全相等替代字段账。错 scope/错 build/同形同值不同 nonce 反例必须拒绝。T475 单角色 cold/warm 计时与 native fence 证据分开。

## 原 oracle 与观察结果

T524/T515 的 SOURCE birth/胶囊/守恒/event 规则与 T554/T641 本族身份一起校验。最低正控从合法 S40、空 packet 与 origin1 出发，binary64 `0.2` 按父规则 `floor(mass×2²⁰)` 得 **209,715 Q20**。x 为 67,108,864 Q20，y2 为 419,430,400 Q21，column64，列净空和质心均合法。预期 slot0、packetId1、generation1、origin2/sourceId2，零速度与零新增动量，总质量由 5,368,709,120 增至 5,368,918,835 Q20，S/F 不变，源功/能量和派生缓存按原精度验。真实封样应见 outcome1、commit+1/quanta不变、Completion[62]=209715、[39..41]=12/12/12、[42]=0xffff、SOURCE J/trial 集全零。核 `ROOT_BEGIN → SOURCE_BIRTH → ROOT_END` 的封存有效前缀、证书字段/path零和实际 cold 区段。SOURCE_AUDIT_REPLAY 复用 E，op9 的两个 arena 副本不能冒充两份独立物理事件原件。原 GPU full-state compare 与逐事件 oracle 各自保留。

Tester 复用 T641/T554 配置/reader、T524/T515 原字典、T652 负控与 T670 原 transport 设施完成实测校验。最小负控应覆盖重复/改 body、合法 q0、坏 hash/签名或错 role、12 次中断/迟到与撤权，分别看读回、typed commit、FIFO、bank 和 cold。T556 的 DM 事件 15 公式只适用于 QUANTUM，本轮正控用 SOURCE_BIRTH；无事件15属于 SOURCE 预期。RX 有真实 scene 时另关联同 SOURCE commit 的有效 XComponent Swap，标明 SOURCE 零物理时间。DM pbuffer 只交 real GPU/Controller 证据；真实页面及 Swap 要有后续实际入口。GPU 驻留优先、CPU 停投、XComponent+shader 唯一上屏，禁止 Canvas 回退。 [T524 SOURCE 工程合同](architecture-T524-g9-spf-engineering-v1.md) · [T515 事件字典](design-T515-three-state-manifest-v1.json) · [T652 负控](test-report-T652-v1.md)

## 实施触发

按族独立推进。T734 构建完成后，由 tester 签包、effector 用实际包/映射确认本族 BOOT。放行证据须将同 ticket、同 PID/start/owner/context、实际包和 BuildId、实际 BOOT GLSL SHA/bytes 与 role 绑定，并附原 Init 完成等待后、`WORK work[7].begin` 的真实 8 B `[0,0x641647]`，及 map/unmap、首 GL/EGL 错和 `BOOT_AUDIT_OK` 时刻。驱动 compile/link/build 或缓存 PERSISTED 单独成证。缺同 scope 原尾或失败时保持该族未放行，报告停在哪个真实阶段。T730 的 G13 数据不能代本族 BOOT，另一族同理。暖新进程、完整 QUANTUM、目标帧率按各后续臂验证，当前交付是离线合同。 [T733 BOOT 定义](architecture-T733-v1.md) · [T727 量测合同](architecture-T727-measurement-v1.json)

重核用 `python tools/architecture/analyze_t736.py .codex/local-test/g13-glslang/bin/glslang.exe`。读取 T733 同一签包源及实际 schema，执行静态 GLSL 检查并复算输入/种子和 pass 表。未执行 SOURCE、未使用真机。
