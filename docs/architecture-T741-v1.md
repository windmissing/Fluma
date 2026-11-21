# T741 · G13 次正规精度与后端资格

2026-11-20 · 目标20 · P5 · architect · 离线裁定。

> T783 补充。T760 的 `G13_COMPUTE` 落盘原件对应 T756 的 BOOT 角色，key `bacbc911…`。按 T783 用同签产品包保数据换进程，联核实际 cache 文件、加载、反射和原 FP32 终态。旧 T730 的 `856a…` 仅证旧 key 加载。BOOT 缓存窄证可独立取，完整角色和首 Swap 继续等原资格与根。 [T783 暖进程合同](architecture-T783-v1.md)

**保留 T690/T607 原次正规、RTE 和数学资格。下一步优先在同一台 Maleoon 935F 上跑 T731 的 Vulkan 资格探针，只创建一个 BOOT pipeline。** 先核 Vulkan 32 位浮点属性，再用实际同源 BOOT 跑原 op64/65/66。待两轮原始位型和原数学尾均通过，再接实际 XComponent 和完整首根。已有 T739 GLES 样本可作比较基线。设备侧尚未验证 Vulkan，也尚未裁定切换产品后端。 [T731 同机方案](architecture-T731-v1.md) · [T741 原件与静态证据](architecture-T741-evidence-v1.json)

## 已定位的拒绝

离线核对不同 boot 下的 **195、250、196、180** 四份真实回读。均为同一 23,834 B BOOT 源码，完成 fence 原值 37146，首次 GL 和附加 WORK 回读 GL 均为 0。有效 CTRL 封印 0xc0de1301，尾部 error=27。op65 先拒绝，op66 的两组提交也已实际执行，分别保有正确封印但独立失败。T739 的 attempt 180 另带实际映射 libgrs_g13 的 BuildId 16704862…、文件 SHA 1a7784f4…、PID 24462/start tick 985718、EGL scope、program 2、CTRL 7、WORK 6 和源码路径 SOURCE_COMPILE，已和 T739 SPLIT 已签包内 ELF 联核。可据此认定这份 GLES 源码在该驱动运行环境重复出现次正规数清零。更早样本的程序加载身份按原件可用范围登记。 [T737 首因](architecture-T737-v1.md) · [195](../test/reports/t491/t730b/v4/l3.json) · [250](../test/reports/t491/t730b/v5/l2.json) · [196](../test/reports/t491/t730b/v6/l2.json) · [180 和输入回读](../test/reports/t491/t730b/v7/t739-diag-full-0.json)

| 原资格点 | 应得原始位型 | 四次实测 |
| --- | --- | --- |
| op65 WORK +35/+41/+47，sin(0)、cos(0)、sin(π/2) | 0、1、距 1 ≤ 2⁻²¹ | 00000000 / 3f800000 / 3f800000 |
| op65 WORK +53，最小次正规数 2⁻¹⁴⁹ 加零 | **00000001** | **00000000** |
| op65 WORK +59，1 + 2⁻²⁴ 的半 ULP tie | 3f800000 | 3f800000 |
| op66 每轮索引 2 与 3，次正规输入保留及结果生成 | **00000001 / 00400000** | **00000000 / 00000000** |
| op66 其余索引及封印 | 3f800001 / 3f800000 / 00000007 / 00000000 / 00000000 / 46503332 | 均匹配，两个失配掩码均为 0x0c |

T730 v7 汇总把 +53 写成门要 1.0，并与半 ULP tie 混写。实际要求是原始位 1，代表 2⁻¹⁴⁹。+59 才是 tie 且已经通过。op66 原封印为 0x46503332。T739 同一个 CTRL 对象回读 [744..747] 为 `[0x1, 0x33800000, 0x3f800000, 0]`，另 [18648..18652] 为 `[0x34000000, 0x00800000, 0x3f000000, 0x7f800000, 0x7fc00000]`，原输入落位、尾部和两轮输出互相印证。op66 索引 3 由两个正常数 2⁻¹²⁶ × 0.5 生成次正规结果，输入已经确认，因此两处清零分别覆盖输入保留和输出生成。暂无理由把现象归回偏移、签包或数值比较阈值。 [T690 原门](developer-T690-v1.md) · [T739 诊断与签包](developer-T739-v1.md)

## 方案裁定

| 路径 | 能保留的资格和推进条件 |
| --- | --- |
| **Vulkan 显式 FP32 模式，首选对照** | Vulkan 1.2 core，或 Vulkan 1.1 上实际启用 VK_KHR_shader_float_controls。由 vkGetPhysicalDeviceProperties2 读取 `shaderDenormPreserveFloat32`、`shaderRoundingModeRTEFloat32`、`shaderSignedZeroInfNanPreserveFloat32`。三项成立才试含对应 32 位执行模式的模块。设备属性、实际 pipeline 返回与原位测试分别记证。 |
| GLES 的 `highp` / `precise` | `highp` 对次正规数仍允许清零。受支持的 `precise` 约束运算顺序和不当融合，无法声明该浮点模式。当前失配包括孤立乘法的次正规结果，新增限定词没有足够理由解决。若厂商提供可核的每程序保留模式，另以源码、驱动能力和同一原位型验证。 |
| 用正常数替换探针或跳过 +53、索引 2/3 | 正常数只可作 ULP/tie 对照。原路线支持的次正规域仍由原向量取证，保持原资格和错误优先级。 |
| GPU 显式软件兼容 | 若同机 Vulkan 数值资格失败且项目仍要求支持该设备，再另行比较 GPU 位精确算术或选用有资格的设备。既有 U32-limb SoftBinary128 只覆盖标量构造与函数，主速度仍 FP32、压力仍 Q32。局部改探针不能覆盖实际计算的下溢。正式替代需证明 GPU 计算全链输入、算术、存取、重放和审计等价，再量测 shader 体积与 250 ms/30 FPS 成本。 |

[Khronos GLSL ES §4.5.1](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf) · [Khronos precise 语义](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_gpu_shader5.txt) · [Vulkan Float Controls](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html) · [T607 数学工程合同](architecture-T607-g13-engineering-v1.md)

## 离线建立 Vulkan 产物的资格形状

从 T739 已签 SPLIT ELF 核实六个 compute 源原件，保原 op、浮点表达式和所有可执行依赖，应用 T731 push constants、layout UBO 和原 storage binding。用 GLSL intrinsics 在各 entry point 请求 `DenormPreserve 32`、`RoundingModeRTE 32`、`SignedZeroInfNanPreserve 32`。固定 glslang Vulkan 1.1 目标执行编译及随附 SPIRV-Tools 校验，六角色全过。反汇编复核实际 execution modes、set 0 binding 0–4（4 SSBO + 1 UBO）、六个 push offsets 0/4/8/12/16/20、uL stride 16 和 LocalSize 64×1×1。六份反汇编均无 RelaxedPrecision 与 NoContraction 装饰。目标模块的原规定运算顺序及数值行为仍由逐角色和原 oracle 判定。

**BOOT 编译输入 24,128 B，SPIR-V 116,592 B；最大源码为 OP_00 的 49,053 B，距 48 KiB 上限 99 B。** 实际构建须按完全生成字节复核。汇总 extension/capabilities 的模块级声明，保三项执行模式，避免重复注入把 OP_00 推过门。新增长度超门则重划依赖闭包，并复核原 dispatch。SPIR-V 字节量另报；这一路的输入为 GLSL。当前结论只涵盖离线编译、SPIR-V 结构和 cap。 [逐角色 bytes/SHA 与编译证据](architecture-T741-evidence-v1.json) · [GLSL intrinsics](https://docs.vulkan.org/glslext/latest/glslext/ext/GLSL_EXT_spirv_intrinsics.html) · [SPIR-V 模式](https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_float_controls.html)

## 最短设备闭环与采用门

1. **先查能力，再建一个计算 pipeline。** 同一台 935F、同系统记录 Vulkan loader、instance API、物理设备 vendor/device/driver、queue family、已启用扩展、三项 FP32 属性与拒绝原值。如果运行版本为 1.1，创建 device 时启用上述 KHR 扩展。缺任一属性即保明确未达条件，禁止创建声明该模式的模块。探针先用实际计算队列，资格阶段无需先建交换链。T731 的窗口/呈现探针随后独立验证。不同候选绑定真实包/映射、PID/start、owner、模块 SHA、pipeline layout、条件和原 VkResult。 [Vulkan SPIR-V 运行约束](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html) · [扩展与核心版本](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_shader_float_controls.html)
2. **同源 BOOT，真实输入和同步。** 固定原 REST 64×64，保原 Config128/seed SHA 和 probe 原字。以同一 pipeline、稳定 descriptor 组派发 op64 的三个 state banks 与两个 render packs，保原 scope 0/1/1/2/2。每组在记录后的执行中绑定实际目标。先完成 op64 对 bank0 的写，再原位覆盖种子头，使用 compute write→transfer write→compute read 的实际依赖，然后 op65、op66 scope 0/1。保 shader 内的同步及两次探针写序。最终 fenced readback 由实际 GPU CTRL/WORK → staging → host，同队列提交时保 shader write→transfer read→host read。非一致内存按 atom 对齐失效后读。取 CTRL 尾 byte 73,728 的 1,024 B，检查封印和 error0、两轮各八字、masks[0,0]，以及 WORK byte 583,296 的原五项和读回的两组输入。阶段区分 pipeline fail、fence/readback fail、op65 首拒和 op66 独立拒绝。 [GLES 原 owner 顺序](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L318) · [T731 映射与同步](architecture-T731-v1.md)
3. **资格失败按实际阶段分流。** 缺属性或实例/队列能力就保该设备/后端的具体缺项。属性齐但管线或同步失败先归工程。原输入与映射可核、管线与回读无错而原位仍错，则该 Vulkan 程序未通过原资格，保原 word、绑定与 loader/driver 来源。现有 attempt 180 已能作同源 GLES 失败比较，避免仅为同一现象反复跑长编译。需要报告 Vulkan 暖收益时才另测实际新进程、正确 cache identity 和独立路径，用 T727 计时。
4. **BOOT 正确后才推进原 oracle 和呈现。** 逐计算角色跑同样两轮资格，正式路径沿 G13 原布局、16 SOURCE / 820 QUANTUM、PACK/AUDIT、许可和真实 XComponent。复用 T631/T721 的解析速度、压力、黏性基准。F05 独立核物理压力及分裂误差，F06 速度/壁剪切和黏性残差，F07 流量及压力/速度证书，按原量纲、边界、阈值和冷档原件验，保首个非零根及同 token PRESENT 的真实返回与画面。原计时预算下取全路径冷暖和 30 FPS。原位、原 oracle、有效首帧与成本均过后，再以真实证据裁定后端。GPU 驻留、CPU 停投，XComponent+shader 唯一渲染途径，禁止 Canvas 回退。 [T631 原 oracle](test-report-T631-v1.md) · [T721 产品侧证据合同](test-report-T721-v1.md) · [T727 冷暖/Swap 量测](architecture-T727-v1.md)

本卡不使用真机。复核命令为 `python tools/architecture/analyze_t741.py .codex/local-test/g13-glslang/bin/glslang.exe`。

## G20 完成度口径

按当前三条实施路线 G13、RX、DM 各看冷编译、链接、暖缓存和首个有效 Swap，共 12 个等权证据点。全链同域实证给 1，仅 BOOT 窄证给 0.5，缺设备证据给 0。G13 与 RX 各有冷编译/链接及部分缓存证据，DM 仅有 BOOT 成功返回支撑编译/链接窄证，三条线尚未取得优化臂首 Swap。当前 4/12，即 **约 33%**。这是按实验结果估算的进度，后续随同身份证据更新。 [G13 T730](test-report-T730-v2.md) · [RX/DM T738](test-report-T738-v2.md) · [T740 对原件的核对](architecture-T740-v1.md)
