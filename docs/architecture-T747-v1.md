# T747 · G13 位精确 GPU 算子可行性架构

2026-11-21 · G20 · P5 · architect · 离线交付。
+**纯 U32 的 add/sub/mul 和带二进制尺度的整数转 FP32 已通过 GLES 静态编译、反射和 Vulkan SPIR-V 校验。** 如果 935F 必须支持，按 T744 先用这套算子做设备对照，再决定是否值得接入完整 G13。正常数快路只在每个运算都可证明处于安全范围且原生舍入已获资格时参与对照。保留现有 T741 原生资格门和失败结果，当前没有选定后端。 [T744 裁定](architecture-T744-v1.md) · [T741 原资格](architecture-T741-v1.md) · [T747 离线证据及 44 条向量](architecture-T747-evidence-v1.json)

## 可实施接口

| 接口 | 运算及表示 |
| --- | --- |
| `addBits(a,b)` / `subBits(a,b)` | 输入输出为 highp `uint` 原 FP32 位型。先处理 NaN、Inf 和零，按绝对值选大数，把次正规有效数左规到 24 位并调整指数。指数差 `d≤25` 时在 low-first `uvec2` 精确对齐，加减至多 49 位。`d≥26` 时小数严格小于大数下侧最窄半 ULP，原大数为 RTE 结果。减法先翻转第二操作数符号，符号零与精确对消按 RTE 处理。 |
| `mulBits(a,b)` | 有限非零输入以整数求 24×24 位完整 48 位积。`umulExtended` 取 hi/lo，指数求和后同一出口舍入。保 ±0、逐级下溢和跨过最大次正规/最小正常的进位。 |
| `packRte(mag, scale, sign)` | `mag` 是无符号整数幅值，数值为 `(-1)^sign × mag × 2^scale`。正常数保 24 个有效位，次正规直接在 `2^-149` 格点做一次舍入，留 guard、sticky 和截断低位的奇偶。上溢按 RTE 包装；移位显式区分 0、32、64 及跨 limb。探针覆盖 U32/S32 幅值和两 limb 的 Q32/整数输入。 |
| 现有数学转换的接缝 | 保留 `SoftBinary128` 的构造、级数和数学域。新增实验接缝应直接接收/返回 `uint`，沿 `wshl/wshr/wbit/wrne` 扩到原 U128，并在最终尺度上舍入一次。`softF32(float)`、`softFloat(Soft)`、`sfloat`、`floatFixed` 是现存转换入口。逐点核原来的零规范化、RNE/RTZ、非有限入口、量化和越域处理。仅复用 limb、carry/borrow 和 rounding 规则，24×24 乘法用两 limb。F32↔Soft/完整 S128、反向定点转换及 binary64 入口仍需各自覆盖，探针当前只实现取位、转出和算术。 |

WORK/STATE/CTRL 和跨程序边界沿原 32 位字及原地址。搬运、哈希、比较位型、有限性分类和冷读可直接消费 `uint`。潜在次正规的值保持该表示，进入任何浮点消费者前须有该运算的安全证明或等价实现。审计先保原错误与无提交语义。探针把无效算术的结果记为 `0x7fc00000`，同时原始输入照存。正式入口仍按既有阶段拒绝非有限，T628 的公共 17、mathStatus 和首因由原调用点决定，也沿用已有明文零规范化。未覆盖的生产消费者列为缺口，含 `/`、sqrt、比较、选择/钳制、坐标求整、dot/归约和渲染 pack。 [现有 limb 与定点](../entry/src/main/cpp/gpu_resident/g13flow/wide_q32.glsl) · [SoftBinary128](../entry/src/main/cpp/gpu_resident/g13flow/math_soft128.glsl) · [T628 首错与数学域](architecture-T628-v1.md) · [GLSL ES 3.10 整数内建函数 §8.8](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf#page=128)

### 正常数快路的逐运算边界

两输入先用原字证实均为有限正常数。`E` 为无偏指数，A/B 代表按绝对值排序后的两个数。减法使用翻转第二个符号后的条件。以下是保守充分条件，边界外由纯 U32 路径覆盖。

| 运算 | 允许的条件及边界 |
| --- | --- |
| 同号 add | `max(Ea,Eb)≤125`。绝对值之和至少是最小正常数，小于 `2^127`，没有对消。 |
| 异号 add | `Ea≥−124` 且 `Ea−Eb≥2`。小数严格小于 `2^(Ea−1)`，差保留大数符号且仍在有限正常区间，舍入不能跨到零或次正规。相近指数会对消，一律走 U32。 |
| mul | `−126≤Ea+Eb≤125`。乘积在 `[2^(Ea+Eb), 2^(Ea+Eb+2))`，舍入后保持有限正常。 |
| convert | 先保留纯 U32 转出。将来仅在确切可表示且目标指令的有符号/无符号语义及位型证明都成立时使用原生转换。 |

这些条件保证范围，RTE 等价还要锁定实际后端、程序、编译模式、禁止收缩和运算顺序。Vulkan 快路要求实际 RTE32 声明与模块模式、已审查的 `NoContraction` 和实机位证。GLES 要核实际作用域及正常数舍入，不从 `highp` 或一次 tie 推导全域严格 RTE。若无法建立该段语义，只比较纯 U32。分支计数记录逐 op 命中、fallback 和混合 wave 情况，耗时臂单独关闭计数。22,860 个正常输入运算的主机有理数抽样交叉核了范围边界，设备出字按下节另验。 [T741 同机能力与收缩](architecture-T741-v1.md) · [Vulkan Float Controls](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html)

## 实机最短证据链和独立 oracle

1. **先跑独立探针。** `t747_u32_probe.comp` 以 SSBO 0 读每条 16 B 输入，SSBO 1 写每条 4 B 输出。输入 `[op|sign, low/a, high/b, exponent+32768]`，opcode 0–4 分别为 add、sub、mul、convert、原字 relay，`sign` 和尺度只由 convert 消费。沿原 native owner 提供数据与 64-lane dispatch，保诊断私有结果及首错。两轮用不同作用域和动态输入，先派发算子，再经真实 barrier 把出字接到第二程序的原字 relay，至少让一个输入在 GPU 上产生供次轮计算，保输入、算术结果和重读结果。两轮都从 fence 后实际 buffer 回读。记录实际 bundle/DSO、PID/start、context/queue、program/module/role SHA、交换的 buffer 范围/屏障及原 GL/Vk 返回。缺证或任一原字不同即停在该层。probe 进程保原生 `CAP_FP32` 拒绝记录，纯 U32 Vulkan 模块走独立整数能力与真机资格，模块没有声明 `DenormPreserve 32` 的原生 FP32 使用权。 [离线探针源码](../tools/architecture/t747_u32_probe.comp) · [原 BOOT 证据链](architecture-T741-v1.md)
2. **原字判据。** `00000001 + 00000000 → 00000001`、`00800000 × 3f000000 → 00400000`、`00800001 − 00800000 → 00000001` 均须保留。反号输出应为 `80000001`/`80400000`。`3f800000 + 33800000 → 3f800000`，`3f800001 + 33800000 → 3f800002`，`00ffffff × 3f000000 → 00800000`，`80000001 × 3f000000 → 80000000`。`−0 + −0 → 80000000`，`+Inf + −Inf`、`Inf × 0`、sNaN/qNaN 的 probe 运算结果是 `7fc00000`，原字 relay 可保存 NaN payload。机器表含 44 条完整输入与期望，精确有理数解码后一次取偶复核。Tester 扩成边界组合和固定种子随机样本，用独立原字 oracle 核首个失配，且加错绑定/错作用域/陈旧读回负控。 [全部向量及 oracle](architecture-T747-evidence-v1.json)
3. **保持原门。** op65/op66 的原偏移、五项 WORK、两组八字和封印仍按 T741 读取。后续等价角色逐个以自己的实际链接程序、生产算子路径和动态输入执行对应样本，并附 store/load、重放、首错；孤立 probe 只证明最小算子。复用 T631/T721 的原 F05 速度/压力与分裂误差、F06 黏性/剪切、F07 流量/剪切，以及 T628 exp 总相对误差门。`exp(-90)` 对照位 `0008ec28`，置零不满足 `2^-20`；`exp(-103)` 最近位仍为 `00000001`，舍入误差超原门，沿原首错记 public17/detail4。补关键字节截断、域拒绝与同 scope 回放。 [T631 独立公式](test-report-T631-v1.md) · [T721 真实生成物](test-report-T721-v1.md) · [T744 exp 原字](architecture-T744-evidence-v1.json)

## 体积与成本量测

| 测项 | 已有数据 / 下一次量测 |
| --- | --- |
| 独立构建 | GLES 送编译源 **4,078 B**，反射 local size X=64、bindings 0/1。Vulkan GLSL **4,075 B**、SPIR-V **15,624 B**，glslang 和捆绑 validator 通过。IR 无 float type、Int64 capability 或 RelaxedPrecision，`OpUMulExtended` 与 carry/borrow 实际保留。核心 3,198 B，未计生产接线。 |
| 实际角色 | 当前 T728/T741 的 GLES/Vulkan 余量分别为 OP_00 **393/99 B**、OP_18 **540/246 B**、OP_37 **707/413 B**、OP_48 **575/281 B**。整体拼入核心即超过 49,152 B。优先按原 opcode 闭包重生独立角色，例如提取 OP_VISC_BLOCK 的真实依赖，逐项剔除已替换的浮点闭包。优先保持原 dispatch 数量、绑定、barrier 和原事务，仅改变角色映射。每个完全拼装单元重报 UTF-8 bytes/SHA 与实际反射，超过 48 KiB 构建拒绝。Vulkan 源与 SPIR-V 分列，直接生成模块时按模块本体过同一 cap。增角色后量测切换、ABI 反射及冷暖编译和缓存开销。 |
| 真实切片 | 先抽取 `viscApply` 的有序四邻和减 `4q`，用实际边界处理后的五个原字核原有次序和结果。然后测同一 `OP_VISC_BLOCK`（48）的一块真实热循环，保其场、ghost、16 次迭代、共享屏障和原残差。该切片外的除法、谱界、归约和检查只有闭包等价完整后才能放行此角色。64×64 的 MAC 有 8,320 个面，每块 16 次循环对应最多 133,120 个面迭代槽。取实际 active/no-op、迭代和分支比例，禁止用槽数推算设备实耗。 |
| 同条件 A/B | 相同后端、队列、场/Config/J、负载、驱动与温控档。合格正常域原生、全 U32、通过范围证明的混合臂各重放同一正常输入。次正规域以原字 oracle 和合格环境结果对照，935F 已知失配原生值单列为负控。前后独立标记纯计时和收据臂。报告提交 CPU 墙时、GPU 时间、从最后提交到 fence、barrier/切换、内存及可获 occupancy/spill，给样本数、p50/p95/max 和无效样本理由。GLES 在查询有效且 `GPU_DISJOINT_EXT=0` 时记 GPU 时间。Vulkan 用队列的 `timestampValidBits`、`timestampPeriod` 和结果 availability，处理计数回绕。查询不可用时只报真实单调时钟和 fence 墙时。 |
| 完整验证 | 沿 T727 对完整根和可见页面记录角色编译/链接/缓存、实际根启动至原 fence/验收/提交、场与 PACK/AUDIT、同 token 有效 Swap，以及 30 FPS 内真实 accepted Q 和有效画面。所有臂保 **250 ms 根绝对期限**，不靠重置 permit、缩短迭代或 CPU 接力。即使切片收益成立，完整 G13 的 820-pass QUANTUM/16-pass SOURCE 和稳定 30 FPS 仍要实测。 |

原 OP_VISC_BLOCK 含高频每面算术和原屏障，是测 U32 成本的较严入口。若位证通过但任一闭包超限，先重分角色。若重分后冷启成本、250 ms 或 30 FPS 仍不成立，保可测失败点，按 T744 使用合格设备路线或继续比较快路。 [实际黏性核](../entry/src/main/cpp/gpu_resident/g13flow/viscosity.glsl) · [T607 调度和期限](architecture-T607-g13-engineering-v1.md) · [T727 量测合同](architecture-T727-v1.md) · [EXT_disjoint_timer_query](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt) · [Vulkan timestamp](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html)

## 放行与进度

- **研究实现触发。** 935F 仍属必需交付机型且本路线进入离线验证计划时，developer 可实现独立 probe 构建并编译，不接产品 Configure/READY。tester 沿机器向量、T631 和现有离线设施维护 oracle。effector 用可用构建执行同机原字和计时。最小精确位证、跨角色回读、可复现生成和体积过门后，再用真实黏性切片比较纯 U32 与有资格的混合臂。可复用合格设备的 T739/T742 原生路线独立推进。
- **产品实施触发。** 取得可控切片成本并闭合实际所需算术、转换、审计及上屏消费者的逐项等价证明后，才考虑完整研究变体。继续用原 Config/seed/input、layout、方程、系数、压力 Q32、op 顺序、域和阈值。缓存、程序/驱动、等价实现版本及冷档工件的身份另记，旧原生拒绝仍可追溯。已有契约足以承载等价实现，若任何候选要求缩域或改变原舍入/误差，须另由用户裁决版本。真机数值、首根、首 Swap 和 30 FPS 均依原门。GPU 驻留优先，CPU 停投，前景背景仅 XComponent+shader，禁止 Canvas 回退。

离线复核 `python tools/architecture/analyze_t747.py .codex/local-test/g13-glslang/bin/glslang.exe`。本卡未用真机，无实际 GPU 耗时或算术通过结论。G20 仍为 **4/12，约 33%**。T738 v5 的 DM 同次 BOOT 已补 PID/start、实际编译约 185.0 ms、链接约 510.5 ms 及真实 8 B 尾，仍为 BOOT 阶段半分。该轮没有 DM 暖二进制加载证据，三族仍缺优化臂首个有效 Swap。 [T738 v5 原件](../test/reports/t491/t738/v5/dm20-v5-final.jsonl) · [T741 计分口径](architecture-T741-v1.md#g20-完成度口径)
