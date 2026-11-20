# T750 · G13 黏性位精确切片工程与成本预案

2026-11-21 · G20 · P5 · architect · 离线交付。

**OP_VISC_BLOCK 可以单独拆出，现有派发顺序和相邻角色切换位置都可保留。** 从 T739 实际签包核对原字节后，生成的单 opcode 对照约 **16 KiB**，GLES 静态编译与反射、Vulkan SPIR-V 校验通过。先做五原字和单块的隔离验证。另发现当前 QUANTUM 的 ACTUAL/REPLAY 映射把两套试算落到同一 bank 和 trace 区。完整根对照须先闭合这个映射。 [T750 机器证据](architecture-T750-evidence-v1.json) · [T747 位精确前置](architecture-T747-v1.md)

## 五原字和边界

1. **固定取值。** 诊断行保存 `[q,L,R,B,T]` 的原 FP32 位型，另记 `f`、U/V、`(x,y)`、boundary、`homogeneous`、当前 WORK 槽和实际四邻地址或 ghost 来源。`q` 直接取 `w[array+f]`，L/R/B/T 按原 `fieldRead` 顺序取有边界语义的值。`f<nuFaces=(Nx+1)Ny` 为 U，余下为 V；lane 仍按 `f=lane; f<nf; f+=64` 消费。五原字阶段先只替换 stencil 中的原位加减乘，随后才将实际访问器和同块消费者接上 U32 原字。
2. **固定舍入顺序。** `s1=add(L,R)`，`s2=add(s1,B)`，`s3=add(s2,T)`，`m=mul(4,q)`，`s=sub(s3,m)`。每个步骤保一次 RTE，保原符号零和次正规，禁止重结合或把 `4q` 与减法收缩。若 `homogeneous && U && !periodicY`，首/末行分别再减 `mul(2,wallLow/High)`。完整 `viscApply` 保留 `sub(q, div(mul(mul(nu,dt),s), mul(h,h)))` 和 `dt=div(1,mul(120,F32(max(J,1))))`。`h` 与 `nu` 来自 `c[10]`、`c[18]`，墙速来自 `c[32]`、`c[33]`，用实际 Config 原字。
3. **固定 ghost 处理。** 先周期规范，再被指定的法向壁面取 `+0`，最后切向镜像。非周期法向越壁作奇反射。U 的非周期 Y ghost 按原 `-inside + 2*wall`，V 的非周期 X ghost 按 `-inside + +0`。保原 `sign*word + offset` 的符号零行为和中间原字。CLOSED 两轴闭，TEST_PERIODIC_XY 两轴周期，TEST_PERIODIC_X_CHANNEL 仅 X 周期。原 `activeFace` 决定下一状态和残差/能量计数，周期重复槽在原 `ENFORCE_NORMAL_FACES` 填写。`viscosityLedger` 另有 `viscApply(...,false)` 消费，连接完整根时须核非齐次分支和该消费者的同域等价。
4. **可核的负控。** `[q,L,R,B,T]=[0,3f800000,33800000,bf800000,0]` 左折叠输出 `00000000`，重排后先让 L 与 B 的 ±1 相消会错误得到 `33800000`。两个正常数的对消样本输出 `00000001`，`q=00000001` 且四邻为零时输出 `80000004`。通道下/上壁对照用相同 `q=3e000000` 与异号壁速，ghost 分别为 `3ec00000`、`bf200000`，齐次修正均应得 `be800000`。机器表保每一步原字。加 ±0、qNaN/sNaN/Inf 分类、周期 alias、墙法向 +0、交换低高墙、lag 一代的邻值和绑错槽负控。探针私有 NaN 规则沿 T747，完整块的非有限/范围故障仍由原 `checkVelocity` 输出 16/35，禁止发布候选。

原生数值对照用已获资格的设备和实际算术证据。仅凭 GLES `highp` 与括号无法证明 RTE 或禁止收缩，GLSL ES 3.10 的 `precise` 是保留字。Vulkan 若声称禁收缩，核实际 SPIR-V 数据流及 `NoContraction`。已知 935F 失配的原生输出作为负控保存。 [原算式](../entry/src/main/cpp/gpu_resident/g13flow/viscosity.glsl) · [原 MAC 访问器](../entry/src/main/cpp/gpu_resident/g13flow/mac_access.glsl) · [GLSL ES 3.10 规范 §3.7、§4.10](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf)

## 原 16 次 block 的交接

| 边界 | 需保持的合同 |
| --- | --- |
| 入口 | `VISC_SETUP`（33）先填 `work(4)` 的 RHS 及 `work(5/6)`，原 `c[525]` 与 `c[729]` 管收敛和迭代计数。私有运行从真实 SETUP 或经过审计的一次性原字快照起步，保存 `uOp=48/uScope/uJ/uMicro/uBlock/uIndex`、`uL[80]`、Base/Candidate/Work/Control 的绑定/range。`uBlock>0` 消费真实上块状态或全同输入快照，保当前、上一步、RHS 及计数，不能拿零块初态跑出后块。原入口先判当前错误、已选 trial、强制跳过、PROJECT_ONLY 和已收敛，再进入热循环。 |
| 局部递推 | 恰好一个 64-lane workgroup。`upper` 保原 S/U128 limb 算法和向上舍入规则，`cc=0.5*(upper+1)`、`dd=0.5*(upper-1)`，`rho` 及 `alpha/beta` 按源码从头复演到 `g=16*uBlock+k`。记录初始与末次 `upper/rho/alpha/beta` 原字。每次循环分别读 `work(5+(g+1)%3)` 和 `work(5+g%3)`，写 `work(5+(g+2)%3)`。块 0 第一次读 6、5 写 7。保 `res=RHS−viscApply(cur,true)`，`next=(x+α*res)+β*(x−old)` 的顺序及原 ν=0、inactive 分支；inactive 复制 `x`，其原行为和 active 数分开核。每次写后所有 lane 同步。 |
| 收敛与账 | lane0 的 `c[729]=16*(uBlock+1)` 同步后才用 `work(5+(c[729]+1)%3)` 作结果。残差、能量按 active 面和原运算顺序，再按 lane→树层 32/16/8/4/2/1 做两次原归约。保 `sqrt(sumSq/Nc)`、`RMS≤2^-16/J` 与 `max≤2^-14/J`，第 7 块未收敛沿原错误 13。读 `c[720/721/525/729/676]`、首错和 `traceBase+1024*(J+micro)+384+16*block` 的 16 字记录，含原 flags、迭代数、RMS/max、参数和 energy、封印。`traceStage` 对 op48 早返回，block 证据取 `traceVisc`。已收敛的后续派发复制原记录并置零活跃迭代；selected/故障/PROJECT/forced skip 各留其原原因。 |
| 同步 | 活跃 OP48 源码每块有 **33 次**全组 `syncAll` 到达，含入口 1、迭代 16、更新计数后 1、两次各 7 的归约、末尾 1。`syncAll=memoryBarrierBuffer→memoryBarrierShared→barrier`。错误由原原子首错和全组一致的分支传播，禁止 lane 私自返回跳过 barrier。后端保每次生产 dispatch 后相应 compute/SSBO 可见性，copy/回读另保对应依赖，最终读实际 fence 后的数据。切片时可采私有中间字，真实根仍沿原单尾 fence / 1024 B Completion。 |

**同组跨 lane 可见性要作为单独对照变量。** 已签 OP_48 的 WORK/CTRL 声明没有 `coherent`。下一轮相邻 lane 会读上一轮写的邻值，残差读也消费 lane0 更新的 `c[729]`。保历史原字节作对照。新增的两个算术臂使用同一 `coherent Work/Control` 声明和上述原屏障，或在该后端证明等价的 shader 内存语义。Vulkan 核实际成员修饰、scope 和 memory semantics，保两个层次的依赖。离线已核这一声明变体可编译。用跨 lane（相邻 `f` 与跨 64 的 stride）、跨迭代、跨程序读回及多次 hash 复现确认可见性，记录错误槽和写者。原声明缺少显式保证属于工程证据缺口，目前没有设备数据把它认定为某次实际失败的首因。 [原屏障和归约](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp#L19) · [原记录公式](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl#L1) · [原调度](architecture-T607-g13-dispatch-v1.json)

### 完整根前置 · ACTUAL/REPLAY 作用域映射

原 820 行调度语义为 root outer `scope=0`、ACTUAL `scope=1`、REPLAY `scope=2`。生成的 `k_quantum` 原样保留这些值。T739 的传输源和当前源都通过 `p.scope ? replay_ : candidate_` 选 binding1，并将 `p.scope` 直接上传给 `uScope`。签包内的实际 OP_48 以 `uScope==0` 选第一 trace 区。结果是 ACTUAL 与 REPLAY 均选 REPLAY bank、trace 基址均为 word **10240**，两者按相同 J/micro/block 写到相同位置。OP48 `c[784+uScope]` 也被推进到 785/786，偏离两套索引 784/785。`ROOT_SCOPE_COMPARE` 仍读取两个不同 bank 与两段固定 trace。这是可由原件复现的静态接线错位，当前 G13 原生 BOOT 仍在更早的 FP32 资格拒绝，现场未执行这些根。

隔离切片可以使用受审计快照和显式 `shaderScope=0/1` 独立完成。完整根测试前，另行修正逻辑阶段到 bank、trace 和 shader 索引的显式映射。ACTUAL→CANDIDATE / `uScope=0` / trace 2048，REPLAY→REPLAY / `uScope=1` / trace 10240。保外层 scope=0 的逐 opcode 绑定、SOURCE 显式 REPLAY override、BOOT 探针的内部 scope 参数。修正前逐项审查 `uScope` 数组索引、选中/跳过位、firstError、cold 物理字段及 reader，保原逻辑阶段身份，避免把 BOOT 或 SOURCE 一并减一。双臂同映射后实证对比两份来自同 BASE 的独立 bank 与 trace，错绑/同地址/同次试算被覆盖须拒绝，才允许用完整根证据判定算术等价。 [出错绑定](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L395) · [生成派发表](../entry/src/main/cpp/gpu_resident/g13flow/contract_generated.h#L3962) · [作用域原证](architecture-T750-evidence-v1.json)

## 生成、体积和实际量测

用原角色生成器按 opcode 取可达闭包，只把 48 单独分出。原 `OP_48` 剩余 `49,54–61` 的顺序及源函数保持，其他角色、布局、调度、820/16 次提交不动。这里的两个 OP48 静态对照均由实际 T739 签包的 **48,577 B** 源重新生成，拆前可逐字重构。各角色继续保私有 66 和独立反射。原生 66 门仍拒不合格驱动，U32 研究角色需在自己的生产算术路径上复测原样本，连同真实 GPU 写读跨角色证据独立判资格。

| 对照角色 | GLES 最终 GLSL | Vulkan 最终 GLSL | Vulkan SPIR-V |
| --- | ---: | ---: | ---: |
| 原 OP48 纯切分 | 15,931 B | 16,225 B | 71,172 B |
| OP48，WORK/CTRL coherent | 15,949 B | 16,243 B | 71,228 B |
| 剩余原角色 | 43,307 B | 43,601 B | 227,312 B |

均已离线通过 GLES 原 0–3 绑定、64 lanes 和作用域 uniform 反射，Vulkan 保 set0/0–3 SSBO、binding4 Layout UBO、24 B push，并通过 validator。这三个源码对照的 SPIR-V 均未生成 `NoContraction`，原生运算资格另验。单块角色有 45 个可达函数，含 `viscBlock/viscApply/viscSpectralUpper/fieldRead/reducePair/checkVelocity`、原 trace/66 及所需 limb，已移除压力 V-cycle、drive 和 ledger 闭包。用 T747 的 3,198 B 核作保守筛查，coherent 对照直加后还有**30,005 B（GLES）和 29,711 B（Vulkan）** 余量。完整 U32 块还需原等价的 div、sqrt、min/max/abs、定点转换和归约的实现及尺寸原件，当前只验证原生闭包形状。新增/重构各个完整编译器输入逐个按 **49,152 B** 构建拒绝超限，GLSL 路线另报 SPIR-V，直接生成 SPIR-V 的模块本身受同一上限。记录 role/opcode map、最终 source SHA、生成版本、binding/同步、两后端模式和各自 cache key，禁止跨臂直接标 cache hit。 [实际角色证据](architecture-T750-evidence-v1.json) · [原尺寸规范](architecture-T727-v1.md)

- **先量工作量及切换。** 每个 QUANTUM 固定派发 112 个 OP48，分 14 个 trial micro、每 micro 8 块，最多 1,792 个 scheduled 迭代槽。64×64 每块最多 133,120 个面迭代槽，CLOSED/双周期/X 通道的 active 面分别 8,064/8,192/8,128。实际记录 submitted、active、converged、skipped、每块实际迭代数和残差归约的 active 面。离线遍历原 pass，拆前后所有相邻角色切换位置相同。在连续提交模型中 program use 为 99→99，固定 10 pass/Pump 模型为 170→170。源码每次 Pump 重置 `lastRole` 且可能因 8 ms 提前退出，真机以实际 Pump 与 `UseProgram`/pipeline 事件为准。另报新增 cold 编译、链接、反射、初始化、暖真实加载和缓存工件；计算程序数由 6 增至 7。
- **成对对照。** 冻结设备/驱动、Config/seed/输入、J、真实 block 序列、资源、热度、角色和 qualifier。依次记录原签包历史臂、同 qualifier 原生对照（限合格域）、纯 U32 与通过 T747 逐运算范围和 RTE 的混合臂。首对先报方向，收益裁定沿 T727 的重复配对。冷热使用不同进程和经证 cache 初态，缓存以 PERSISTED→同 key 的真实 BINARY_LOAD+reflection 为准。计时样本关闭逐面取证，另跑同输入原件臂。分报真实 API 编译/链接、切换数与 CPU 提交时间、设备有效 GPU query/timestamp、fence 及实际墙时，缺 GPU timer 时只报墙时；同时报告字节峰值、采样数、p50/p95/max、错误与截尾。
- **可验收的等价和时限。** Tester 复用 T747 原字、T631 F03 离散黏性衰减、F06 启动和稳态及墙剪切、F07 流量/剪切，完整根再保 F05 压力/速度与分裂误差及 T628 首错。私有切片收完真实五原字、每块首错、迭代与残差，从真实 fence 后核查。完整臂比较同一实际 owner 根的入场至 fence/接受/提交，保 **250 ms** 许可绝对截止和原拒绝。用实际 accepted root/scene token 对齐 XComponent 首次有效 Swap 和持续有效 **30 FPS**，另记 accepted Q 与重复帧。隔离切片的快慢只支持该块成本，原 820-pass 完整根结果须实测。 [T727 量测合同](architecture-T727-v1.md) · [T631 独立 oracle](test-report-T631-v1.md) · [T721 实际字段](test-report-T721-v1.md)

**放行顺序。** 935F 仍是必需交付机型且按 T744 选择验证 U32 备选时，developer 可先接独立五原字探针与成对控制臂，交同一构建链编译和身份收据。tester 维护独立 oracle，effector 先取得 T747 双轮真实 GPU 位证、跨程序/跨迭代读回和设备原件，再跑单块成本。整根候选需再闭合 ACTUAL/REPLAY 映射、未覆盖原算术、同步、边界和原 oracle；角色并入正式 Configure/READY 依这些真实前置。目标域、原数值门、压力 Q32、布局和算法保持。GPU 驻留优先，CPU 停投，前景背景仅 XComponent+shader，禁止 Canvas 回退。 [T744 后端裁定](architecture-T744-v1.md)

复核命令为 `python tools/architecture/analyze_t750.py .codex/local-test/g13-glslang/bin/glslang.exe`。本卡未用真机，GPU 算术与耗时待实测。**G20 仍为 4/12，约 33%。** 同期 T749 增加了 RX 同次 BOOT 身份证据，三条优化路线仍缺首个有效 Swap。 [T749 补证](test-report-T749-v1.md) · [T741 计分](architecture-T741-v1.md#g20-完成度口径)
