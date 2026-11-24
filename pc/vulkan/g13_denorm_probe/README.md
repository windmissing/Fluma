# G13 FP32 v2 · Windows 独显短测

整包解压到可写目录，在 RTX 3090 Ti 的 Windows x64 环境双击 `Run-Test.cmd`。回传最后一行 `REPLY G13-FP32-v2`。完整日志在本机自动留下，无需发出。脚本优先使用 Vulkan 枚举中的独显，并在摘要中打印真正使用的设备名。OP_48 调用观察约 60 秒，全进程上限 120 秒。

`probe=PASS` 表示 GPU 上的位型、小量边界、正常数舍入和非有限值探针通过。`OP48=OK@…ms` 表示默认次正规行为的 OP_48 模块成功创建计算管线，只运行独立探针与建管线。这一版无完整沙水或流场业务入口，F03/F05 和最终物理资格需另取证。

证据区分为 `-Verify`（本机散列检查）、`-ProbeOnly`（独立小量 GPU 探针）和默认短测（探针加管线创建）。新程序没有独显次正规保留、清零的资格门。保留独立的 RTE、SignedZeroInfNan 能力检查。

## 构建复核

1. 用 `python build.py --glslang <glslang.exe> --out <build>` 从已封存的 T916 角色源码生成头和两个 SPIR-V。已复核的编译器是 glslang 16.6.0（报告版本 11:16.6.0）。脚本按冻结摘要确认 T916 模块，并要求默认模式的源码重新编译、验证结果与仅去除 DenormPreserve capability/mode 的原 SPIR-V 逐字一致。
2. 从 x64 VS 开发者终端用 `cl /EHsc /std:c++17 /O2 /MT /I<含 vulkan/ 的头目录> /I<build> main.cpp /Fe<build>/G13-FP32-Probe.exe` 编译。
3. `python package.py <build> <output.zip>` 生成工件。运行端通过系统 Vulkan loader 选择 GPU，应用无需图形 SDK。

本轮 Windows 集显工位使用 Intel UHD Graphics 630，Vulkan 原始驱动号 1656924。实际探针通过。OP_48 已进入 `vkCreateComputePipelines`，在 60 秒观察窗内未返回。RTX 3090 Ti 待复测。
