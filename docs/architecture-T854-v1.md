# T854 · G13 OP17 建管线停点与私有写集

2026-11-23 · architect · P3 · Windows x64 Vulkan · 仅 PC

**把问题定界到 Intel Vulkan 建管线中的编译器执行。** 在冻结的 `OP17_DRIVE_CONTEXT` 中，移除 `entryLedger()` 后仍两次超过 60 秒未返回。后一轮的两个原始 dump 均在同一 owner 的 Intel `igc64.dll`，采样落在不同偏移。当前证据只定位到模块和调用，具体编译 pass、触发指令与最终是否返回仍未知。 [T833 阶段交接](../test/reports/t833/t849-handoff.md) · [本轮核验收据](architecture-T854-evidence-v1.json)

**11 月 23 日更新。** T833 的 motion、MMS、ledger 已在开发机真实建管线并通过 OP66。私有三段链在一个真实槽上通过 ACTIVE、ENTRY_NOOP、MMS_FAULT 的独立写集及首错判定。集显开发机与目标独显机统一使用拆分路线。拆分版按独立数值参考、事务不变量和完整业务根验收，当前工作可继续接入可验证的完整候选。单派发整段只提供可选逐字回归，缺其输出时逐字等价标待证。原版建管线停点保留在历史诊断记录。 [拆分版实际交付](../test/reports/t833/t854-handoff.md) · [独立判定](../test/reports/t857/handoff.md)

## PC 原件核对

冻结 Release PE `49d4f25e…` 与四个角色的 GLSL/SPIR-V SHA 逐一重算，每份 SPIR-V 在该 PE 内恰有一份。四个角色沿同一 layout SHA、`flags=0`、空 pipeline cache 在 Intel UHD 630 上执行。下表的建管线耗时用宿主实际调用计时，未返回时用同域 QPC 从 ENTER 到监督器截止。 [窄臂原件](../test/reports/t833/t849-qualification/narrow-result.json) · [原 OP66 字序](../entry/src/main/cpp/gpu_resident/g13flow/boot_contract.h)

| 实际角色 | 建管线 | 同 owner 后续 |
| --- | ---: | --- |
| `OP7_MOTION_ONLY` | 59.156 s | 创建、submit、fence 均为 0。双轮 OP66 原字逐项对应，掩码 `[0,0]`，正常退役。 |
| `OP7_MMS_FORCE_ONLY` | 9.470 s | 创建、submit、fence 均为 0。双轮掩码 `[0,0]`，正常退役。 |
| `OP17_LEDGER_ONLY` | 78.824 ms | 创建、submit、fence 均为 0。双轮掩码 `[0,0]`，正常退役。 |
| `OP17_DRIVE_CONTEXT` | 60.059 s 后收回诊断子进程 | shader module 创建成功，目标 compute pipeline 只有 ENTER。无本角色 `VkResult`、submit/fence、OP66 或正常退役。 |

复测仍在 `vkCreateComputePipelines` 的 ENTER 后 60.085 s 被监督器收回。对两个 `.dmp` 重新解码 `ThreadList`、x64 `CONTEXT`、`ModuleList` 与 `MiscInfo`，核对 PID `9016`、owner TID `8916`、进程创建时间和同域边沿。进入后 20.085 s 及 45.059 s 的 RIP 分别为 `igc64.dll+0xae55e8` 和 `igc64.dll+0x4a17de`。版本 `31.0.101.2140`，两份 dump 还均列出同版本实际 Vulkan ICD `igvk64.dll`。栈样本窗口的 owner CPU 用量约为单核 90.8%。这说明采样时线程正在 Intel 编译器模块中执行。两点采样和无符号栈不足以还原编译调用链或判定死循环。收据保存两个原始 dump 的 SHA 和路径、模块基址、RIP、栈区长度及本机 DLL 的 SHA。 [复测原收据](../test/reports/t833/t849-qualification/OP17_DRIVE_CONTEXT-stack.receipt.json) · [Minidump 格式](https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/)

本轮再次运行 `wpr -start CPU -filemode`，`CPU.Verbose.File` 返回 `0xc5585011` 和 `Failed to enable the policy to profile system performance.`。前后 `wpr -status` 均未录制，未取得 ETL。后续复现可以继续使用同 owner minidump。具备现成采样权限的 PC 可另采 WPR 和符号，不新增此项为 T833 开工前置。 [复核原错](architecture-T854-evidence-v1.json)

## T833 先接真实 OP17 入口的两项资格

用冻结 `role_OP_00.comp` 的 `split_group` 重新生成 `OP17_DRIVE_CONTEXT` 并与 T833 实际编入 PE 的 GLSL 逐字节相等。在此基础上，只在 `drive()` 的 `uvec4 fq=uvec4(baseState[18],baseState[19],0u,0u);` 处分界并重取调用闭包。第一臂保留从 `if(lane!=0u)return;` 起到该标记之前的完整运动块。第二臂保留 lane guard、从标记开始的 MMS 块和原末尾 `failMath`。两臂均逐字保原 `main`、OP17 入口 guard/prologue/trace 与 OP66，均无 `entryLedger()`。各自独立只用于定位建管线。OP7 已过资格的模块使用槽 815 的时间分支，OP17 使用原 scope、J、micro 和实际 `acceptedTime()`，所以 OP7 的结果不能覆盖这两个新臂。 [角色生成器](../pc/vulkan/build_roles.py) · [拆分器](../tools/architecture/analyze_t750.py) · [原 `drive`](../entry/src/main/cpp/gpu_resident/g13flow/drive.glsl#L7)

| 离线已验证诊断臂 | 最终 GLSL / SPIR-V / 指令 | SPIR-V SHA 前缀 |
| --- | ---: | --- |
| `OP17_MOTION_CONTEXT` | 18,369 B / 84,732 B / 5,202 | `c3062c6d…` |
| `OP17_MMS_CONTEXT` | 15,758 B / 66,844 B / 4,020 | `ece1ae31…` |

两个 `.comp`、`.spv` 与反汇编已生成在本机 `.codex/local-test/t854-architecture/`。按原 `build_roles.py` 的 PC 转换、glslang `--target-env vulkan1.1 --spirv-val` 和反射规则，保五个 binding、Layout UBO stride16、24 B push、64 lanes 与三项 FP32 模式。两臂只有离线编译证据，尚未跑 Vulkan 建管线和 OP66。正式变体同样遵守每个最终 Vulkan GLSL ≤49,152 B。 [完整指纹](architecture-T854-evidence-v1.json)

T833 先为两臂各跑一次独立进程，复用 `qualify_role.py` 的 60 秒单次调用/120 秒进程限额，用已通过的 `OP17_LEDGER_ONLY` 作同机正控。记录实际 PE/SPIR-V、adapter/LUID 与 ICD、layout/flags/cache、QPC、返回、OP66 原字和真实退役。59.156 s 的 OP7 motion 距限额只有 0.844 s，若同一臂试次跨限额，保实测分布并复核，不能把一次边缘通过当成稳定时间。若一臂超时，保持其余字节、输入和 API 条件，在同 OP17 上只更换一个子闭包，motion 先隔离原 `acceptedTime` 与角化简/三角，MMS 先隔离窗判定与原 Soft128 指数。可以另用可运行同工件的 PC Vulkan 驱动作同条件对照。保有界 QPC 与两点 owner 栈，未经实测不判定为特定算子或厂商内部缺陷。 [监督器](../pc/vulkan/qualify_role.py)

## 首个私有等价候选

先落 **OP17 motion → MMS → ledger/trace**，维持原四个数据 buffer 和原逻辑槽。原有计算块之间仅新增两处同 queue 的 compute write→compute read/write 屏障，沿 T849 的 128 B 私有 WORK 段传递阶段和数学状态。保持原数值顺序与 RNE，不从舍入后 CTRL/FP32 值重建 Soft128。T854 两个单臂各自会走原入口和 trace，接续时必须按下表收敛到一个入口和一个 trace。 [私有段既有布局](architecture-T849-v1.md) · [Vulkan 同步](https://docs.vulkan.org/guide/latest/synchronization_examples.html)

| 阶段 | 等价边界 |
| --- | --- |
| OP17 motion | 只在本阶段运行原入口快照、`c[752]`/`c[676]` prologue 和原逻辑 active/NOOP 计数，保 trial trace 的计数单元。锁存原 NOOP 判定，写 `c[624..641]`、`c[820..827]`，运动产生的 `mathDomain` 当场沿原代码记录。NOOP 路径只在入口写原 `traceStage(true)`，后两段按该锁存作无业务写的消费。 |
| OP17 MMS | 用同一六 push 和 BASE/CANDIDATE 重算原 `acceptedTime()`，保 `[start,end)`、`outState[204]==3` 和两个 `softExp` 的顺序。力窗外或 forceType 不符时 `c[642..643]` 保原字。保 `softDomain`、`localMathStatus`、`localMathFunction`、`localMathObserved`、`localRounded`、`expContext` 和 3×4 个原始诊断字，原末尾 `failMath` 只在此处。 |
| OP17 ledger/trace | 沿入口 active 锁存运行，即使 motion/MMS 新写 `c[514]` 也继续原 `entryLedger()`，之后才 `traceStage(false)` 一次。`entryLedger` 从 `work(0)` 进行原 64-lane 归约。相对 `b=work(19)+256` 只写 `w[b+0..48]`、`w[b+64..83]`、`w[b+96..127]`。在原 `uMicro==0` 且整秒谓词满足时写 `c[544,574,576,578]`，保先抄旧账本再更新 CTRL 的顺序。OP17 trace 为 `microBase()+520` 起 8 字，保原 active/NOOP、错误、计数与 seal。 |

每次原槽继续用实际 `{uOp,uScope,uJ,uMicro,uBlock,uIndex}` 和 root nonce，私有段仅允许预期生产者驱动下一阶段。新槽先失效旧阶段，消费前核该段范围、nonce、六 push、阶段、owner/候选代及实际 buffer 身份，按原 owner 同队列推进。坏段、接错根和撤销使候选失效，沿既有故障/退役收尾，不产生可发布 Completion。完成/退役通过原 fence 路径，私有段与诊断全缓冲读取不进入正式热路径或 wire schema。 [原入口及首错](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp#L20) · [ledger](../entry/src/main/cpp/gpu_resident/g13flow/ledger.glsl#L5) · [trace](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl#L5)

## 差分与放行

- **已有 OP7 原字。** 重读三份各 5,343,360 B 的原始归档，以 BEFORE/FIELD/MOTION/MMS/OP8 的实际 staging 分段逐字比较四缓冲。隔离执行的 BASE、CANDIDATE、WORK 全部逐字相同。BOOT 种子 FIELD 的 CTRL 仅字 `522,752,755` 改变，audit 从 0 到 6。MOTION 的可见变化是 `624,625,630..633,637`。MMS 仅将 `642/643` 从 `7fc00042/7fc00043` 改为 `3e7de023/3dfbc4c9`。OP8 改 `522,752`，活动逻辑计数各截面为 `0→1→1→1→2`。两份早期种子字段失败的 `[op,error]` 为 `[7,35]`、`[7,26]`，首错头和 seal 均保持至 OP8。它们证明了同输入这组写集和保留边界，尚未覆盖后续实际抛出竞争数学错误。 [原始段长、摘要和文件位置](../test/reports/t833/t849-qualification/private-write-result.json) · [本轮差分逐字表](architecture-T854-evidence-v1.json)
- **拆分版原字与参考。** 延用 T844 的冻结输入与分段机制，在实际 OP17 scope、J=1/2/4 和 micro 槽采 BEFORE/MOTION/MMS/LEDGER。核 `c[624..643]`、`c[820..827]`、`c[751]`、WORK 边界及未写区域，保原 OP7→OP8 的独立时序回归。用独立参考复核运动、MMS 输出及账本归约。含 REST、匀转、RAMP 两支与时间边界、MMS 窗内外、分别触发的 motion/MMS 数学错、整秒/非整秒 ledger、已选 trial/forced skip/已有首错导致的入口 NOOP。加入 active 后新数学错仍执行 ledger/active trace、OP7 field 首错后再次实际发生数学错、错 nonce/槽/旧段和中途撤销。核原 `atomicCompSwap(c[514],0,e)` 的首次获胜、`c[513..517]`、`c[526]`、`c[733..750]`、`c[751]` 的累计诊断位及 `c[766]` seal。首错比较保真实首次发生阶段，原 `mathDomain` 可先于 `drive` 尾检查发布错误。若有可运行单派发原模块的 PC，追加同一输入的全四缓冲逐字对照并单独登记。 [T844 判定夹具](test-report-T844-v1.md) · [T857 独立判定](../test/reports/t857/handoff.md) · [Soft128 原诊断](../entry/src/main/cpp/gpu_resident/g13flow/math_soft128.glsl#L5)
- **下一步接入拆分业务候选。** 当前冻结映射仍是 SOURCE 16、QUANTUM 820。原表 QUANTUM 含 14 个 OP17 和槽 815 的一个 OP7。原逻辑槽数保留，OP7/OP17 三段均过资格后的物理派发候选为 `820+2+28=850`，copy/barrier 独立计。`820+2+14=836` 作为后续 MMS 与 ledger 同臂的优化候选，须单独过建管线、OP66、独立参考、首错及完整业务验证。生产放行照 T828 核非零 SOURCE/QUANTUM、双 scope、250 ms/33,333,333 ns、一枚尾 fence 与 1,024 B 热回读、同根 shader 画面及可信速度/压力/黏性，沿原 T606/T631/T721 的独立参考及数值门。目标 14 沿原前置。开发机实测功能与正确性，目标独显机使用同路线的业务构建实测性能。GPU 驻留及 shader 渲染，禁止 Canvas 回退。 [原派发表](architecture-T607-g13-dispatch-v1.json) · [目标 13 PC 验收](architecture-T828-v1.md)
