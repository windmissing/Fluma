# T744 · G13 次正规资格失败与后续路线裁定

2026-11-20 · 目标20 · P5 · architect · 离线交付。

**实测 Maleoon 935F 和该驱动组合，G13 两条原生路线均未取得原数值资格。保留当前 MUST-PASS 合同。优先用可用的合格设备继续 G13，RX/DM 拆分按各自证据独立推进。** 当前原件里没有已合格的替代设备。若 935F 是必需交付机型，先评估 GPU 位精确等价路径。只有用户明确接受缩小数值域，才另立版本化合同。 [T741 原资格](architecture-T741-v1.md) · [T744 联核证据](architecture-T744-evidence-v1.json)

## 能力声明、实测与未知项

| 证据 | 已确定的结果 | 推进边界 |
| --- | --- | --- |
| Vulkan T743，ticket 1 | loader 和物理设备报告 1.3.309，实际 instance/effective 为 1.2.0。真实 OHNativeWindow surface 创建成功。设备 Maleoon 935F，vendorId 6629、deviceId 537137152、driverVersion 240715635。三项 FP32 属性 DenormPreserve/RTE/SignedZeroInfNanPreserve = **0/1/1**，诊断 FP32_PROPERTIES_MISSING → NO_ELIGIBLE_PHYSICAL_DEVICE。 | 该分支在创建逻辑 device 和计算管线之前返回，op65/66、GPU fence、交换链及 Present 均未到达。该驱动未宣告支持原 Vulkan 模块要求的 DenormPreserve 32。 |
| GLES T730/T741 | 同一 BOOT GLSL 的 195/250/196/180 四次真实回读，op65 +53 应为 0x00000001 而为零，+59 RTE 正常。op66 两轮次正规输入保留和结果生成均失败，掩码各 0x0c。T739 attempt 180 的实际加载、原输入、GL 对象和完成 fence 有关联证据。 | 被测程序和 GLES 环境复现清零，原资格拒绝成立。 |
| 综合 | 同机两条路径都未满足现行合同。Vulkan 的缺项和 GLES 的实测失败同属次正规保留。 | Vulkan DenormFlushToZero 属性未采，Vulkan 算术未测。芯片执行单元、编译器、驱动及其组合的内部成因仍未知。 |

T743 签包 SHA 前缀 bdc2a7bc，包内 ARM64 ELF BuildId e7b5586e… 与实机报告匹配。已离线核 NO_ELIGIBLE 的真实代码分支、BOOT GLSL 24,160 B、SPIR-V 116,688 B、模块 SHA cb82cd06… 与包内唯一副本。模块声明 DenormPreserve 32、SignedZeroInfNanPreserve 32、RoundingModeRTE 32，T742 已为四处结果声明加 precise。T743/T739 的 REST Config、seed 和 BOOT 父源 SHA 相同，两后端生成物各自记录。属性为 0 时，该模块不进入 Vulkan pipeline。 [T743 原始回执](../test/reports/t491/t743/vk-diag-1.json) · [T742 资格分支](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp#L268) · [T742 编译原件](../test/reports/t742/compile.json) · [Vulkan SPIR-V VUID 06297](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html) · [两种次正规属性](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html)

## 保持原合同的推进顺序

1. **优先在另一台可用设备上验证原合同。** 当前没有已取证的合格备机。先从现有设备中筛候选，复用 T742 探针与 T739 GLES SPLIT。取实际 loader/API/driver、物理设备和签包 SHA/映射 BuildId。若 Vulkan 三项 Float32 属性、资源与队列资格齐全，只建一个 BOOT pipeline，验证原位。候选 GLES 也可用同原位合同认证。仅在实际试验通过后把候选称为合格设备。
2. **保留 935F 原生路线的具名拒绝。** Vulkan 报具体属性缺失，GLES 报原位资格失败，同时展示已确认的程序与驱动身份。仅当系统、驱动或实际编译方案改变，再按新指纹复验。调度可复用已知拒绝以免重复长编译，产品能力仍按该次设备、驱动、程序和合同判定。GPU 名称不足以作为跨版本拒绝依据。
3. **若 935F 必须支持且原合同不能缩域，先验证 GPU 位精确等价的可行性。** 比较 U32 原位型 add/sub/mul/convert 与有保守范围证明的原生正常数快路。保次正规原字经过 GPU 计算、存取和角色边界，含输入分类、正常数下溢、对消、RTE 和工具链编译核对。探针须执行实际算术路径，保单独的软件后端身份及原生能力记录。可复用 SoftBinary128 的 limb 工具和部分标量构造，主速度的 advection、viscosity、force、reduction 与定点压力接口仍需逐项覆盖。先用最小算子验证正负次正规、正常数生成、对消、tie、±0/NaN/Inf、同条件重放和首错，再择一段真实生产算子测 49,152 B 门和成本。取得等价值与可控成本后才考虑扩完整求解，仍受 250 ms 根期限及 30 FPS 约束。当前没有这一实现可交 T730。 [现行压力与速度合同](architecture-T607-g13-engineering-v1.md) · [原生黏性算子](../entry/src/main/cpp/gpu_resident/g13flow/viscosity.glsl)
4. **RX/DM 按各自资格接续。** RX 消费现有 BOOT 原件，按 T736 进入本族非零 SOURCE 前置核对。DM 已复现 BOOT 返回，下一步补同次 driver/DSO 映射与真实 8 B 尾。完整 QUANTUM 各依 T740。 [T736](architecture-T736-v1.md) · [T740](architecture-T740-v1.md) · [T738 真机报告](test-report-T738-v2.md)

## 若要定义支持 FTZ 的新合同

替换 +53 或 op66 探针会失去原资格覆盖。只检查输入为正常数仍会漏掉真实中间结果。两个正常 FP32 数 0x00800000 × 0x3f000000 得 0x00400000，0x00800001 − 0x00800000 得 0x00000001。两个算式都无乘加融合，却都得到应保留的非零次正规结果。

离线复算了 T628 数值反例。exp(-90) 的 RTE 位型为 **0x0008ec28**，仅舍入误差约 4.80×10⁻⁷，低于原总相对误差门 2⁻²⁰。静默写零的相对误差为 1。exp(-103) 虽可舍入到最小次正规，仅舍入误差已约 24.34%，按原门仍须拒绝。原合同因此需覆盖可舍入为非零次正规且舍入误差低于门槛的区域，替换为正常数测试会改变覆盖。该反例仅验证数学条件。产品 F05 和实际设备运算仍需原 oracle。原失败继续按 T628 public17 + bit4 和首错原件处理。 [T628 数学域](architecture-T628-math-domain-v1.json) · [实际力项与标量消费](../entry/src/main/cpp/gpu_resident/g13flow/drive.glsl) · [复算原件](architecture-T744-evidence-v1.json)

若用户接受缩小支持域，再书面裁定三个点，随后给 profile、能力、缓存和冷档重新版本化。

- **设备与范围。** 确认 935F 是否必需，以及产品是否允许缩小数值域、限制输入和推进时长。声明 config、seed、驱动及中间值的可证范围，明确 RTE、非有限、零符号和重放行为。
- **可观察行为。** 受限正常数域须保证所有相关角色的输入、转换和中间结果在证明范围。潜在次正规可具名拒绝，保留原输入与首错。若允许在明确场景清零，则要重定义误差与状态重放判据。覆盖正负次正规、正常数输出下溢、对消、舍入 tie、零与非有限、exp 原域、拒绝后无提交。
- **损失与验收。** 列明确被排除或改变的有效输入、制造解、连续时间、守恒与能量、散度以及旧档可读性。独立 oracle 验新域正例和原域拒绝迁移，至少重走 F05 压力/速度及分裂误差、F06 黏性/剪切、F07 流量/压力证书、同一非零首根、同 token Swap 和 30 FPS。新版本通过只覆盖其声明域。 [T631 Oracle](test-report-T631-v1.md) · [T721 取证](test-report-T721-v1.md)

## T730 接续与当前进度

前置为实际候选设备或明确授权后的等价实现，以及其可部署真实工件。优先复用 T739/T742 签包，固定同一配置、seed/输入及作用域。最短补证取设备原属性、队列、实际 pipeline 返回和 fence 后 CTRL/WORK 原字。原 BOOT 须 ctrl7=0、有效 seal、五项数学样本合格、op66 双轮原字吻合，再推进逐角色、原 oracle、完整首根和同一已接受 root token 的 XComponent Present。冷暖要有实际包映射、PID/start、初始缓存清单及编译、链接、加载原时点，保 250 ms 根门与 30 FPS。当前组合原生两路线拒绝继续登记，READY 和首个有效 Swap 未达。GPU 驻留优先、CPU 停投，仅 XComponent+shader，禁止 Canvas 回退。 [T727 量测合同](architecture-T727-v1.md) · [T731 后端工程合同](architecture-T731-v1.md)

G20 按 T741 的 12 点口径仍为 **4/12，约 33%**。RX BOOT 新进程暖轮已有证据，原日志核得 637.3 ms 冷、186.3 ms 暖，仍属 BOOT 阶段半分。DM 复现 BOOT 返回，G13 Vulkan 在设备预检停止。三条优化路线仍缺首个有效 Swap。本卡不使用真机。离线复核运行 python tools/architecture/analyze_t744.py。 [RX 冷暖原日志](../test/reports/t491/t738/v3/h-warm.txt) · [DM 复跑](../test/reports/t491/t738/v3/dm20-v3-final.jsonl) · [G20 计分口径](architecture-T741-v1.md#g20-完成度口径)
