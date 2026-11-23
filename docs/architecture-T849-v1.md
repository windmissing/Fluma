# T849 · Windows G13 `drive` 与 OP17 建管线停点及工程分流

2026-11-23 · architect · P3 · Windows x64 Vulkan · 目标 13

**当前最窄的现场停点是只含 `drive()` 的稳态模块。** OP7 的字段审计、OP15 和另外四个角色已通过本机双轮 OP66。`OP7_DRIVE_ONLY` 在没有 `entryLedger()` 的情况下仍未返回。共同优先核 `drive` 的 Soft128 路径，另测 OP17 的独立账本归约。 [T833 八臂报告](../test/reports/t833/t841-qualification.md) · [本轮复核](architecture-T849-evidence-v1.json)

## 实测原件及归因边界

复算八份原始 JSONL、角色模块和实际 x64 PE `ad40c7ce…`。各角色 SPIR-V 在 PE 中只出现一次，最终 GLSL/SPIR-V SHA 与 T841 逐项相同。八臂均用同一 Intel UHD 630 硬件条目、LUID/device UUID、驱动 31.0.101.2140、Vulkan 1.2、queue family 0、同一 pipeline layout、`flags=0`、`pipelineCache=null`。每个子进程的实际 BOOT OP65/66 通过，随后稳态 `vkCreateShaderModule` 返回 0。 [汇总收据](../test/reports/t833/t841-qualification/build-and-result.json) · [T841 基线](architecture-T841-v1.md)

| 实际臂 | 观测 |
| --- | --- |
| `OP7_FIELD_ONLY`、`OP15`、`OP6`、`OP8`、`OP12_14`、`OP16` | 建管线约 307.067 / 330.819 / 35.347 / 52.138 / 58.748 / 102.199 ms。真实 submit/fence 返回 0，两轮 OP66 原位比较 `[0,0]`，正常退出。 |
| `OP7_DRIVE_ONLY` | PID/TID 96972/83232，`moduleSHA=158cd36f…`。从 ENTER 到 supervisor 截止的同域 QPC 差 60.003 s。只见 ENTER，无 VkResult。进入调用后的样本显示 owner 平均占单核约 91.3%。 |
| `OP17` | PID/TID 85708/11824，`moduleSHA=cb5b3825…`。从 ENTER 到 supervisor 截止为 60.100 s。只见 ENTER，无 VkResult。进入调用后的样本显示 owner 平均占单核约 90.9%。 |

两条未返回臂均由 supervisor 收回其诊断子进程，未执行该臂的 submit/fence/OP66，缺正常 owner 退役。60.003/60.100 s 是观察下界。T841 的关闭优化对照针对原完整单 opcode 7，本轮新臂使用默认 flags。两者的共同可达 `drive` 含角化简、三角多项式、Soft128 基础运算和条件 MMS 指数。证据已排除 `entryLedger` 是 OP7 复现的必要条件，尚未隔离 OP17 的叠加影响，也没有驱动用户栈能判定 ICD 内部阶段。 [驱动臂原件](../test/reports/t833/t841-qualification/OP7_DRIVE_ONLY.receipt.json) · [OP17 原件](../test/reports/t833/t841-qualification/OP17.receipt.json) · [创建 API](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateComputePipelines.html)

## 冻结源码的最小可执行对照

从 T833 实际生成的 `role_OP_00.comp` 走 `t841_arms → split_group → build_roles` 的原变换，按下表对调用体做最小替换，再取传递函数闭包。已逐个通过离线 glslang/SPIR-V 校验，保原 private OP66、64 lanes、四 storage binding、Layout UBO stride16、24 B push、三项 FP32 执行模式及 OP66 的原 `NoContraction`。新增最终 GLSL 每单元均小于 49,152 B。以下是诊断模块，新模块暂未运行实际 Vulkan，当前 production opcode 映射保持原版本。 [实际角色生成器](../pc/vulkan/build_roles.py) · [驱动原文](../entry/src/main/cpp/gpu_resident/g13flow/drive.glsl) · [Soft128 原文](../entry/src/main/cpp/gpu_resident/g13flow/math_soft128.glsl) · [账本原文](../entry/src/main/cpp/gpu_resident/g13flow/ledger.glsl)

| 对照 | 精确切口/实际经过的调用 | GLSL B / SPIR-V B / 指令 |
| --- | --- | ---: |
| `OP7_MOTION_ONLY` | OP7 drive 前段，原 `acceptedTime`、区间化简、RAMP/匀转、三角与重力，截止于写完 `c[820..827]`。 | 16,535 / 73,916 / 4,774 |
| `OP7_MMS_FORCE_ONLY` | OP7 drive 从 `uvec4 fq=…` 到末尾，保原力窗、`acceptedTime`、两次 `softExp` 及原受控输出，无角化简/三角。 | 13,924 / 56,028 / 3,582 |
| `OP17_LEDGER_ONLY` | 保原 OP17 prologue、入口 early guard 和 `traceStage`，删除 `drive()`，只留 `entryLedger()`。 | 8,937 / 28,852 / 1,809 |
| `OP17_DRIVE_CONTEXT` | 保原 OP17 prologue/guard/trace，删 `entryLedger()`，独立核 `drive` 在真实 OP17 push 下的闭包。 | 19,976 / 92,592 / 5,988 |
| `OP7_REDUCE_ONLY` | 仅在私有 OP7 lane0 对真实输入 `outState[26]` 执行原 `softF32→softReduce`，冷取 `quadrant` 与 `mathRaw`。不含 trigonometric polynomial、`softDiv` 或 `softExp`。 | 11,175 / 45,548 / 2,927 |

先用现有监督器测 `MOTION_ONLY`、`MMS_FORCE_ONLY`、`LEDGER_ONLY`，各一独立进程、原 60 s / 120 s 观察预算，取得实际创建返回、submit/fence 和原 OP66。运动臂未返回再测 `REDUCE_ONLY`。若两臂分别能创建，执行下述私有连续写集对照。账本臂正常时仍按需用 `DRIVE_CONTEXT` 测 OP17 入口/尾跟踪与 drive 同模块的影响。诊断 guard 和输入从冻结原字取值，严禁把缩减臂结果作为正式根发布。单个臂停点只缩小候选；例如 `REDUCE_ONLY` 能返回，不能排除化简与多项式组合仍触发编译负担。 [监督器](../pc/vulkan/qualify_role.py)

## 通过隔离后可接续的等价方案

只有对应新臂均完成建管线、OP66 和数学资格，才做下列候选。**GPU 调用顺序、逻辑计数、数学和首错保持原义，物理派发数量另计。** 若仅 MMS 指数路径仍不返，先保其它阶段证据，并用替代驱动/堆栈取证。不得删除 forceType=3、把实际输入锁成 REST、改 `exp` 精度或以 CPU 计算补回。

1. **跨派发只交接未舍入的私有上下文。** 运动臂写原 `c[624..641]`、`c[820..827]`，未新增 `c[642/643]` 写。MMS 臂同样使用原 BASE/CANDIDATE、原 `uOp/uScope/uJ/uMicro/uBlock/uIndex` 重新执行原 `acceptedTime()` 和 `[start,end)` 测试，维持原两次 `softExp`、诊断设置顺序、受条件保护的 `c[642/643]` 及原 `failMath`。各臂边界保存 `softDomain/localMathStatus/localMathFunction/localMathObserved/localRounded` 和 3×4 原始 `localArg0/1/localSoftResult`，`expContext` 状态照原值接续。现切口没有跨臂存活的 `Soft` 数值，后续如再在 trig/ramp 内切，必须用 `{sig[4], signed exponent, sign}` 保存原私有值、阶段和异常上下文，禁止 F32 舍入后再展开。保持原 113-bit significand、RNE、常量、多项式顺序和原 exp 证书。 [T721 数值合同](test-report-T721-v1.md)
2. **仅 PC 候选记录一个有界 GPU 私有段。** 复用同一 WORK binding，在原 WORK 后申请 16 B 对齐的 128 B 资源。words 0/1 为 schema 标记和阶段，2–3 为实际根 nonce，4–9 为原六个 push，10 为入口 guard 标志，11 保留零。words 12–17 按顺序保存 `softDomain/localMathStatus/localMathFunction/localMathObserved/localRounded/expContext`，18–29 为三个 4-word 原向量，30–31 保留零。新 PC UBO 在原 80 项 layout 数组之后给实际 word offset，原字段及 push 偏移照旧。首次写先失效旧阶段，按同 root/ordinal/角色及实际 range 校顺序，消费者与下一写者都在原 owner 同 queue 上。每次记录真实 compute write→compute read/write 屏障。独立核实际内存和 fence，过期或缺子阶段使整个根候选失效并报告具名平台故障。私有段放在旧 STATE、Completion256 或 ROOT/FULL schema 之外，按诊断需求冷采另报。 [Vulkan 同步](https://docs.vulkan.org/guide/latest/synchronization_examples.html)
3. **OP7 保字段在前，OP17 保入口判定。** OP7 保第 815 槽 `field → motion → MMS`，原字段阶段只计一次，已存在首错时驱动仍沿原顺序运行，保持 first-error 原子语义。OP17 在 motion 前执行一次完整原 prologue、计数及 NOOP 判定，把是否进入 active 路径随私有阶段封存。后续 MMS 和 ledger 消费此 latch，禁止读更新后的 `c[514]` 把原 active 改成 NOOP。原 active 即使 `drive` 产生新的域错，仍按旧顺序执行 `entryLedger → traceStage(false)`。原 NOOP 只写一次 NOOP trace。`entryLedger` 从原 `work(0)` 计算 `kinetic/reducePair`，保 64-lane 归约顺序，以及 `work(19)+256` 的真实写范围和原首微步/整秒分支。其原读取无 `c[624..643]`，但 trace 读取当前错误和账本状态，不能先于 drive 或提前 seal。 [原入口/守卫](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp) · [原 trace](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl)
4. **计数与触发。** 原 `16/820` 逻辑槽、尾 `[39..41]`、`OP_ROOT_TIME_PREPARE` 与有效帧 gate 保持。表中 OP7 一处、OP17 十四处。若 OP7 三阶段、OP17 两阶段且 `MMS + entryLedger` 的生产变体也建管线及原字验证通过，则 QUANTUM 实际派发为 `820+2+14=836`。若 OP17 的 motion/MMS/ledger 各自分派，实际 `820+2+28=850`，两者均固定排每个子阶段并由 GPU 保留原 NOOP，copy/barrier 另计。等价候选采用实际已验证拓扑和新 pipeline set SHA，SOURCE 不由本合同自动迁移。其 55/56/58/59 另按原 `acceptedTime` 和 writer/audit 顺序取真实角色证据。未通过私有原字门时保原 820 物理映射冻结。 [原派发表](architecture-T607-g13-dispatch-v1.json) · [T753 资源](architecture-T753-v1.md)

## 补证、验收及恢复

- **取证优先级。** 若最小臂仍复现，从同一诊断进程 ENTER 进入后，在约 20 s 与 45 s 取实际 owner TID 的用户栈或 WPR CPU sampled stack，保存进程创建 tick、QPC、ICD/loader 的实际模块路径、版本、模块偏移、采样覆盖与符号可用性。另保 supervisor 截止同域 QPC。环境缺调试器/权限时登记不可采原因，可换已具备采样条件的同驱动 Windows 工位。另一实际 Vulkan 硬件/驱动以相同 PE、SPIR-V、layout、flags、API 资格和至少一个已通过的轻角色成对验证，明确 adapter/driver 身份。替代设备结果只覆盖其真实适配域。 [Microsoft WPR/WPA 采样说明](https://learn.microsoft.com/en-us/troubleshoot/windows-server/support-tools/support-tools-xperf-wpa-wpr)
- **位级等价。** 复用 T844 的冻结 BASE/CANDIDATE/WORK/CTRL，新增对照点为 motion→MMS→ledger 前后。核 `c[624..643]`（力窗外 642/643 必保原字）、`c[820..827]`、`c[751]`、首错字段及 seal、WORK 最终快照、OP17 active/NOOP trace、OP8 真实下游；核其它字不写。覆盖所有合法运动 modes、RAMP 两个时间分支和边界、MMS 窗内外/两级 exp 原错、字段先失败后数学失败、OP17 入口 NOOP 与进入后数学失败、错 nonce/序号/旧私有段和中途撤权。优先用能运行原模块的合格硬件作冻结输入的同后端差分，缺原输出时显式留缺，原 oracle/正式物理证书仍逐项核验。 [T844 夹具](test-report-T844-v1.md) · [T837 原判定](../test/t837/README.md)
- **业务条件。** T833 可先接小臂诊断与私有写集。只有所有实际生产角色过原 OP66 和各自数学/接口资格，才推进非零 SOURCE/QUANTUM 执行、双 scope 重放、原 250 ms/33,333,333 ns、单尾 fence/1024 B、ROOT/FULL 与同根 `GridView` 原始场。目标 14 等目标 13 速度、压力、黏性的可信 PC 定量场。GPU 驻留，shader 上屏，禁止 Canvas。 [目标 13 PC 定量验收](architecture-T828-v1.md)
