# T742 · G13 Vulkan BOOT 资格探针

2026-11-20 · 目标20 · developer · 离线开发和编译。

隔离 HAP 从 `G13VulkanBootAbility` 打开 BOOT 页。原生 owner 从 XComponent SURFACE 回调取得真实 OHNativeWindow 引用，用 `vkCreateSurfaceOHOS` 建 surface，记录实际 compute queue 对该 surface 的支持。具备 compute、graphics 和该 surface 呈现支持的队列优先。探针以一个 compute pipeline 完成资格回读，交换链、首根和实际呈现属于后续阶段。后台、销毁或几何代变化撤销当前票据，已提交的工作由 owner 等待实际完成再回收。

设备侧按实际 loader 和物理设备 API 走 Vulkan 1.2 core，或在 Vulkan 1.1 device 创建时启用 `VK_KHR_shader_float_controls`。三项 Float32 属性均为 `VK_TRUE` 才创建 shader module 和 pipeline。最终回执带选中的设备、队列、扩展、原始 VkResult、actor PID/start tick、owner TID、窗口代、映射 ELF BuildId 和实际模块 SHA。缺能力时保留具体项，设备属性、管线结果与数值资格分列。[T741 架构裁定](architecture-T741-v1.md) · [Vulkan FloatControls 属性](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html) · [Vulkan OHOS Surface](https://docs.vulkan.org/refpages/latest/refpages/source/VK_OHOS_surface.html)

| 原资格 | 实现与回执 |
| --- | --- |
| REST 64×64 | 从现有 ArkTS 配置生成原 Config128 和 seed SHA。GLES 与 Vulkan 共用 BOOT 布局、探针操作数、状态头字段及双轮预期位型。 |
| BOOT 64/65/66 | 三 bank、两 pack 的 scope 为 `0/1/1/2/2`，随后原位覆盖 bank0 状态头，再执行 `65:2` 和 `66:0/1`。整个序列绑定同一 pipeline，五组 descriptor 在记录前写定。 |
| 完成与原位回读 | 命令显式建立 transfer→compute、compute→compute、compute→transfer 和 transfer→host 依赖。等待真实 `vkWaitForFences` 成功后读取 CTRL、两组原输入和 WORK。非一致内存按整段映射失效。 |
| 首拒和独立判据 | CTRL 尾封印或 error 失败时 OP65 保持首拒优先级。OP66 两组各保八字、封印和失配掩码，可独立定位。原输入回读和五项 WORK 证据随结果提交。 |

## 离线工件

- Vulkan BOOT 和 GLES SPLIT 均通过 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`，错误 0。沿用了现有未签名隔离构建方式。GLES SPLIT HAP SHA256 为 `37d4aacc9aed3560cf301ed0922fd100fede717ccc92ebf92c6486293493956f`。
- Vulkan 输入 GLSL 为 24,160 B，继承 T741 已核对的 BOOT 生成字节，只在四处原探针结果声明加 `precise`。glslang 16.6.0 编译并经随附 SPIRV-Tools 校验，通过三项 32 位执行模式、五个 set 0 binding、六个 push offset、UBO stride 16 和 local size 64 的复核。SPIR-V 为 116,688 B，ARM64 链接 ELF 与 HAP 内 ELF 各精确包含一份。完整 SHA 和 BuildId 见[编译收据](../test/reports/t742/compile.json)。
- 工件为 `.codex/local-test/t742/entry/build/default/outputs/default/entry-default-unsigned.hap`。开发交付阶段未使用真机。设备支持、真实管线返回、fence 与数值结果留待 T743 取证。

从仓库根目录生成隔离项目，随后在输出目录运行上述 Hvigor 命令。

```powershell
node tools/g13/prepare_build.cjs .codex/local-test/t742-rebuild --vulkan-boot <glslang.exe>
python tools/g13/record_vulkan_boot.py .codex/local-test/t742-rebuild <receipt.json> --readelf <llvm-readelf.exe>
```

收据工具读取与项目同名的 `.log`，其中保留该次完整 Hvigor 输出。T743 使用实际签包启动后点击「运行资格探针」。终态在页内可复制，也可从 HiLog 的 `G13VkBoot` 按同 PID、ticket 和 `receipt <ticket> <序号>/<总数>` 拼回一次完整 JSON。成功状态 `BOOT_NUMERIC_QUALIFIED` 对应本卡 BOOT 数值门。
