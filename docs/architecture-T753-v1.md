# T753 · G13 ACTUAL / REPLAY 绑定与可见性工程合同

2026-11-21 · G20 · P5 · architect · 离线交付。

> T764 补充。T760 首拒 OP65 后未达到 T756 生产稳态角色与隔离可见性验证。隔离真机验证改由独立诊断 HAP、owner/context 和固定 pass 入口执行，复用生产绑定、barrier、map 和顶点属性配置，保原数值门和实际场景发布路径。 [T764 门前独立探针合同](architecture-T764-v1.md)

**把调度阶段、shader 索引和物理 bank 分开。** 现有 QUANTUM 的 ACTUAL 和 REPLAY 均写 REPLAY bank 与 CTRL word 10240。REPLAY 成功封口还可能将选定 J 写进原根 opcode 的 `CTRL[520]`。本卡锁定映射、GPU 可见性及发布边界，供 developer 实施。T730 原 FP32 数值门继续生效。 [T750 原定位](architecture-T750-v1.md#完整根前置--actualreplay-作用域映射) · [T753 可复核证据](architecture-T753-evidence-v1.json)

## 实际根因与修正位置

T739 签包 `9e35e801…` 的 ARM64 module BuildId `16704862…` 与已归档加载样本匹配。本轮核签包 SHA、ELF、六个实际生成角色字节，以及 T739 构建输入 `transport.cpp` 的绑定路径。原表 820 条 QUANTUM 行的逻辑 scope 为 0/1/2。`Transport::Bind` 用 `p.scope ? replay_ : candidate_` 选择 binding 1，并将 `p.scope` 直接上送。`traceBase()` 将所有非零 `uScope` 映射到 10240。`sealMicro()` 写 `c[518+uScope]`，原 2 会写 520，`rootInit` 与 `completion` 用 520 存取 root opcode。ACTUAL 不能设置应有的 `c[518]`，两段共 14 个黏性微步的 block0 在原路径只产生 7 个不同地址。该错位可从源码和已签生成物确定。T730/T739 当时停在 BOOT 原数值门，现场完整根尚未到达。 [宿主绑定](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L395) · [封口代码](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl#L7) · [根初始化与 Completion](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp#L51) · [T739 实际加载证据](architecture-T741-v1.md)

### 绑定表与校验

在 pass 规划和后端绑定间加一份共用的只读映射。按 root kind、原 ordinal 和原行 `op/scope/J/micro/block` 校验，再输出逻辑 phase、shader scope、绑定角色及实际 buffer/range。`uIndex` 仍为原 ordinal，保持 820/16 行、dispatch/作用域标记和 ABI。role lookup、program/context、动态 bank 和选择的 descriptor 提交前必须匹配冻结计划。未退休读者、错代、错误逻辑 scope、写入对象被重叠绑定均拒绝提交，禁止把非法索引 clamp 成可执行。只读的 BOOT 别名沿原用法逐 op 核实际无冲突访问。

| 原路径和 ordinal，闭区间 | `uScope` | SSBO 0 / SSBO 1 | 证据目标 |
| --- | ---: | --- | --- |
| Q 0–4、409、815–816、819 | 0 | CURRENT / CANDIDATE | 外层按原 op 写。409 封 ACTUAL 前缀，815/816 只审计和准备 CANDIDATE，819 从 CANDIDATE 封 Completion。 |
| Q 5–408，表 scope=1 | 0 | CURRENT / CANDIDATE | ACTUAL，从同一 BASE 重置并推进。CTRL trace `[2048,10240)`，selected word 518。 |
| Q 410–813，表 scope=2 | 1 | CURRENT / REPLAY | REPLAY，从该 BASE 独立重算。CTRL trace `[10240,18432)`，selected word 519。 |
| Q 814；SOURCE 10（op60） | 0 | CANDIDATE / REPLAY | state 全字比较，两个范围须独立。按原规则比较两组物理 trace，只有每个 1024-word 块的 scope 字 +4 排除出值对比，仍须独立验值。 |
| Q 817–818；SOURCE 13–14 | 0 | CANDIDATE / PENDING_PACK | PACK_VIEW / PACK_AUDIT 只消费已比较的 CANDIDATE。 |
| SOURCE 0–6、11–12、15 | 0 | CURRENT / CANDIDATE | 54–56 为 ACTUAL，其他是准备或外层。施源通过且 `quanta` 不增。 |
| SOURCE 7–9（op57–59） | 0 | CURRENT / REPLAY | 保原专用绑定与原源码 `which=1` 的 source 封口。诊断逻辑 phase 由原 op 推导，SOURCE 的旧 `uScope=0` 语义保留。 |
| BOOT 64–66、私有 66 | 原值 | 原冷初始化与 probe 绑定 | op64 三 bank scope 0/1/1、双 pack scope 2。op65 保原冷入口和绑定。op66 两轮 0/1，私有 66 同样由各实际链接角色执行。 |

两根 scope 的局部索引 `s=0/1` 同步用于 `selectedAtEntry`、`c[518+s]`、`c[521]` trial 位、`c[768+s]` forced skip、`c[776+s]` begun、`c[780+s]` micro mask、`c[784+s]` visc 和 `c[788+s]` pressure。`traceBase+1024*(J+micro)` 使用 `[1,2,3,4,5,6,7]` 七个 micro 槽，OP48 block 的 16 字记录从该槽 `+384+16*block` 起。ACTUAL/REPLAY 第一个 J1 block0 起点为 word **3456/11648**。`c[515]`、Completion[122]、`c[829]` 和 MicroTrace +4 按规范化 0/1 原字登记，逻辑 phase 和原表 scope 放证据侧车。`c[520]` 仅存本根 opcode，Q 为 0、SOURCE 为 50。历史 trace +4=2 或 root opcode 异常据实判为旧工件或坏证据。

### WORK、CTRL、SC 和原 ABI

- **State / WORK。** binding 0 的 BASE 是既有 CURRENT 的只读引用。ACTUAL 和 REPLAY 的写目标为不同分配或物理不重叠范围，始终从独立的 BASE 恢复。binding 2 仍为一份 GPU WORK，包含 face ENTRY/ADV/KICK/CORIO、黏性 RHS/三槽、Q32 压力及残差、诊断归约、系数和持久化控制。两 scope 串行复用，不能被解释成两份同时在场的 WORK 原件。`TRIAL_RESET_FROM_BASE/INIT` 重置状态和根暂账，ENTRY 和 `VISC_SETUP` 生成当次输入，`PRESSURE_INIT/RHS` 重建压力相关 region。逐角色核 mutable range 的先写后读，replay 读只允许原冷常量或该 scope 上游已产。WORK[19] 的冷系数/数学资格和 `entryLedger` 重建的 per-micro 暂存分开核。特别验证失败 J→下一 J、ACTUAL→REPLAY、已收敛/PROJECT_NOOP 后的未执行 region 没有被当成实测结果。
- **CTRL。** binding 3 保 18,688 words。`[0,1024)` 是配置/permit/quota/全局控制，`[1024,2048)` 是不可变原始/规范输入与回执。`[2048,10240)` 和 `[10240,18432)` 是独立 ACTUAL/REPLAY trace。`[18432,18688)` 为 Completion256，读回 offset **73,728 B**、**1,024 B**。`ROOT_INIT` 清当前根 GPU 暂位/trace/tail，`ROOT_REPLAY_BEGIN` 先封 ACTUAL 并保可比较摘要，再执行 REPLAY。不得把根摘要头、全局计数、回写到两段的 Completion 副本用作独立试算的唯一证明。Compare 前核两段 nonce、根/J/micro scope 及采样封印，独立检查轨迹覆盖、首错及可比原字。禁止在热路径添加逐 pass CPU 回读。
- **SC（现有 Scene 读取和发布）。** 仍用 3 个 state、2 个 RenderPack，WORK/CTRL 不给 Scene 当输入。只租同一已接受的 Field/Pack 和原 profile。PACK 作为 GPU 输出，首批顶点属性从 pack header 后 byte 256、stride 64 读取，tracer 属性从 state 的原 tracer byte offset、stride 32 读取。保原 XComponent 和 offscreen 目标。FULL 冷档固定 `PREFIX/BASE/CANDIDATE/REPLAY/WORK/CTRL/PACK0/PACK1`，记录 `BeginRoot` 快照的实际 bank/range/代号，释放 pin 前完成同根读回。ROOT 冷档缺 REPLAY，需要独立双 bank 证据的用例取 FULL 或现有独立 fence 后的有界诊断记录。冷读 WORK 是最后阶段的实际 scratch，逐 scope 的 WORK 可见性依赖当时独立取证。

GLES 保四个 std430 binding 与 linked roles 的每程序反射。Vulkan 保 set 0 的 0–3 SSBO、4 Layout std140 UBO（80 项、stride 16、1,280 B）与六个 U32 push values（24 B），不增加 descriptor/存储字或产品输出字段。已生成的 layout/math/algorithm/gates 合同保持，新 scope 路由与同步纳入生成清单和独立实现身份。 [原布局与冷档](architecture-T607-g13-engineering-v1.md#9-冷档与原始证据) · [Vulkan ABI](architecture-T731-v1.md#实施面角色及相同计算) · [Scene 顶点读取](../entry/src/main/cpp/gpu_resident/g13flow/scene.cpp#L136)

## 两后端同步边界

| 边界 | GLES | Vulkan Core 1.1 路径 |
| --- | --- | --- |
| 同 workgroup 内 | 保 `memoryBarrierBuffer → memoryBarrierShared → barrier` 和全 lane 一致到达。对有跨 lane 读写的 Candidate、WORK、CTRL 显式 `coherent`。例证为 copyBase 同次跨 lane 写后 lane0 修头、OP48 读取邻 lane 上一轮结果、lane0 写 c[729] 后其他 lane 读。共享树归约仍用原 `shared`。 | 同源显式成员可见性与组内 barrier。对实际模块验 memory model、storage member `Coherent`、execution scope Workgroup、buffer/shared memory semantics 等价。核当前工具链与目标设备已启能力。API barrier 只放 dispatch 边界。 |
| GPU 生产后给下一 dispatch | 原每 producer `GL_SHADER_STORAGE_BARRIER_BIT` 保留。把 actual→replay、compare、PACK/AUDIT 纳入顺序，核切换 program 后实际重绑定与 uScope/ordinal。 | 同 queue 用 `COMPUTE_SHADER/SHADER_WRITE → COMPUTE_SHADER/SHADER_READ|SHADER_WRITE` 覆盖实际重叠 ranges。上传路径按实际 API 处理 host flush/submit 与 `TRANSFER_WRITE → COMPUTE_SHADER`。每根使用冻结的 descriptor tuples 或等价不可变范围，避免命令录制后改写仍在使用的 descriptor。走 T731 同一 graphics/compute/present queue 优先，不隐式跨 family。 |
| 根尾、故障和冷读 | 当前 root submit 只给出 `SHADER_STORAGE`，`Poll/CopyBuffer` 紧跟 fence 后 map，原根路径没有 `BUFFER_UPDATE`。在最后相关写之后、尾 fence 前记录 `GL_BUFFER_UPDATE_BARRIER_BIT`。故障前缀与冷读欲取 GPU 写字，同样在实际 fence/map 路径保证这一可见性。map/unmap/首 GL 错和 fence 原返回需同时成立。 | 实际 CTRL→staging 用 `COMPUTE_SHADER/SHADER_WRITE → TRANSFER/TRANSFER_READ`，copy 后用 `TRANSFER_WRITE → HOST_READ`。提交 fence 成功且同代后读取，非 coherent host 内存按 atom 对齐 invalidate，保 offset/返回值。FULL 的其他 segment 逐真实 range 走同规则。 |
| State/Pack 给绘制 | 当前 Scene 直接顶点取 buffer，root 路径缺 `GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT`。最终 compute 写之后、首消费之前补该 bit，可与根尾 buffer-update 合并。保已有 offscreen 顺序和独立 draw fence。 | `COMPUTE_SHADER/SHADER_WRITE → VERTEX_INPUT/VERTEX_ATTRIBUTE_READ` 针对实际 Field/Pack；按 T731 对 offscreen `COLOR_ATTACHMENT_OUTPUT/COLOR_ATTACHMENT_WRITE → FRAGMENT_SHADER/SHADER_READ` 和 image layout 作转换。acquire semaphore 等待实际依赖阶段，draw/render-finished 与 present 分别取证，按原 WSI 退役。 |

旧声明保作可追溯对照。两算术臂统一同步与 qualifier 后才比较成本。本轮将三块声明加 `coherent`，在 T739 六个角色及 T750 隔离 OP48 上重跑 GLES 静态编译、反射和 Vulkan validator。最大输入为 GLES **48,786 B**、Vulkan GLSL **49,080 B**，Vulkan 尚有 **72 B**。OP62 的 WORK 在 GLES 静态反射被优化掉，仍以生成 ABI 和该角色实际必需访问为准。隔离 OP48 的 SPIR-V 在 `Logical GLSL450` 下保 Workgroup 控制屏障，及 UniformMemory / WorkgroupMemory 的内存屏障。原生 FP32 执行模式仍在。每个实际新增/改动单元过 **49,152 B** 完整输入门，若继续加探针导致超限，按 T750 拆角色。静态校验覆盖语法、反射和指令，运行时可见性由实际跨 lane 和跨 program 探针证实。 [GLSL ES 内存限定 §4.9](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf#page=79) · [OpenGL ES 可见性 §7.11.2](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf#page=133) · [Vulkan 同步和回读](https://docs.vulkan.org/guide/latest/synchronization_examples.html)

## 发布、原装包证据和负控

1. **根的许可与晋升。** 根开始时冻结 nonce/plan、run/owner/context/surface/geometry/config/session/permission/inputEpoch/control、BASE commit/quanta、实际对象与 range、role map 和 cutoff。每 Pump 依原 10 pass/8 ms 和 **250 ms** 绝对 deadline 检查，fresh pulse 仍为 **33,333,333 ns**。只在完成 fence 成功、原 Completion 封印和审计齐全、同 scope、计数/根类型、选中双 scope、租约代号和 permit 重新有效时，owner 原子轮转 current/candidate、pack/pendingPack、commit/quanta 与 SOURCE FIFO。原取消/超期走废弃或撤权账。完成 fence 只给自己的对象退役信用。fence 创建/等待失败或上下文丢失保根/scene 资源不复用，沿 owner 退役或上下文销毁链收口，不能把 null fence 当退休。 [原完成与许可](../entry/src/main/cpp/gpu_resident/g13flow/controller.cpp#L403) · [当前根 Poll](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L571)
2. **Scene 发布。** 完成提交后才签 Field/Pack 同代租约，绘制前、Swap 前及实际返回后核同 lease/token 和许可，实际成功才推进 `PresentedMapping/lastGood`。先保一份明确覆盖相关 Draw 的 scene fence/WSI 退役证据再释放复用资格。现有 `ReleaseScene` 可收到 null GL fence，实施须把“未发 Draw”和“Draw 后 fence 创建失败”区分记录，后者留资源待退役。迟到根、迟到 scene fence 或旧 Swap 只入旧代记录，状态保 FAULT 或具名 STALE。仅 XComponent+shader 上屏，保持 30 FPS 的有效帧口径。 [Scene lease](../entry/src/main/cpp/gpu_resident/g13flow/controller.cpp#L620) · [原绘制与 Swap 逻辑](../entry/src/main/cpp/gpu_resident/g13flow/scene.cpp#L93) · [Vulkan WSI 退役](architecture-T731-v1.md#真机最小探针和安全退役)
3. **原装包及修订身份。** 原件保 HAP 或实际驱动签包/包 SHA、实际进程 PID/start 和 BuildId/DSO SHA、gen、设备驱动/API、物理 program/module/descriptor、逐角色源码 SHA/bytes、cache route/key/binary SHA。根侧车保来源与实际映射 policy hash、根 nonce/plan 和 scope、begin/end monotonic ns、GL/Vk 值、实际 SSBO 或 buffer allocation/range、bank/pack generation、role+ordinal+uScope、barrier 类型/范围、fence handle/generation、尾/冷 raw SHA 与 commit/scene token。GLES 绑定查询作为隔离核对或边界采样，Vulkan 记实际更新和记录的 descriptor tuple。对照臂为同 capture scope 配对，历史标签/源码 SHA 严禁补写成新臂。cache ABI 与角色源码区分两臂，原端点字段和 cold offsets 保留。在 `read_archive.py` 增连据核对时按 policy 区分历史结构解析与新工件的独立 bank/trace 证明。现存 alias oracle 固定读 CANDIDATE，修正版依 `c[829]` 和实际绑定选 bank，ROOT 缺所需 REPLAY 时给无法观测并取 FULL。 [冷 reader](../tools/g13/read_archive.py#L178) · [T727 证据 schema](architecture-T727-measurement-v1.json)

| 隔离负控 | 应见原证和结果 |
| --- | --- |
| 两相同绑 REPLAY、只换 bank 或只换 scope、把 descriptor range 在同一分配中重叠 | 根开始前校验实际对象/range 与期望 tuple，命名错位并拒绝。保底另测同地址 trace 和两次写出的独立序关系。即便两份 payload 相同，绑定重叠也必须失败。历史 `uScope=2` 负控应见 `518+2=520`、七个 trace 地址复用。 |
| 仅给 REPLAY 增一个原位差、调换两个 scope 证据或省略一个 phase seal | 真 `ROOT_SCOPE_COMPARE` 对实际 REPLAY 差异走原 18/audit bit13 缺失，commit/quanta/pack 不晋升。仅 scope/nonce/封印错即便 data 匹配也拒绝作为独立证明。同步通过的同输入正控须先成功。|
| 同组跨 lane、跨迭代、跨 program 与 pack 顶点读取 | test-only 输入使用 lane/迭代/root 可区分的值，f=63→64、反向回读、三槽旋转、CTRL[729] 和 `c[802]` 各有对照。跨角色读取有效值并保 marker 原字、shader 及 barrier 事件。分别走 buffer map 和 VertexAttrib 消费，看真实生产位。缺 barrier 的对照即便因设备偶然可见仍不能证明合同，通过结构检查识别缺边。 |
| stale WORK 与迟到完成 | 在合法测试入口对上一 scope scratch 放标记，下一 scope 的有效消费者只能读本次生产值。另测根发出后 pause/resize/rebind/stop、过 250 ms 后仍得到 signaled、错代 fence/scene lease、fence null/WAIT_FAILED、Vulkan device/surface lost 或 WSI 仍未退休。预期原件可归旧 scope，当前无新 commit、quanta、pack、Present 或 FIFO 消费，资源按真实 fence/代号退休。 |

**实施触发。** scope 路由与消费者可见性修正由静态原件确定，developer 可按合同开发、编译和离线核对。tester 补 836 行映射、错绑及冷读夹具。隔离可见性探针由 effector 按设备队列取真实原件，可先于数值资格做隔离验证。完整 G13 看 T741/T744 实证前置。935F GLES 原 FP32 失败、Vulkan 原模块缺 DenormPreserve32 均继续登记。合格替代设备可复用 T739/T742 资格链。若 935F 仍是必需且选择等价路线，先过 T747/T750 的真实位证、可见性、成本及完整算子覆盖，再接完整根。所有分支保持原算法、layout、原数值 oracle、250 ms 及有效 30 FPS，GPU 驻留、CPU 停投，禁止 Canvas 回退。 [T744 数值路线](architecture-T744-v1.md) · [T750 切片与成本](architecture-T750-v1.md)

离线复核 `python tools/architecture/analyze_t753.py .codex/local-test/g13-glslang/bin/glslang.exe`。本卡未修改产品代码、未使用真机。**G20 仍约 33%（4/12）**。DM SOURCE 已开发编译，RX BOOT 已补原身份，SOURCE 真机与优化臂首个有效 Swap 待证。 [T748 交付](developer-T748-v1.md) · [T749 补证](test-report-T749-v1.md) · [T741 计分](architecture-T741-v1.md#g20-完成度口径)
