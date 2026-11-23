# T860 · PC 异步 SOURCE 和热页接线

Windows x64 Release 编译通过。PE 为 `.codex/local-test/t860-release.exe`，SHA256 为 `99181c704081acbc0194143941b84b23a02e2040cad567941762a90976c5c3ef`。SPIR-V 沿 T856。

Win32 脉冲通过四槽 `OfferSource` 提交不可变输入，Vulkan owner 消费并生成原 32 字命令。收据保原 QPC、scope、坐标、输入及命令 SHA。撤权在短锁中失效旧项，原提交及尾接受检查继续生效。可选 `--m1-capture-hot` 由 UI 线程在 SOURCE 或 QUANTUM 提交后的实际忙段预约，owner 从已验收的 1024 B 热尾封不可变页，UI 读出并结束读权。没有增加 GPU 热路 readback。

`test/t860/ingress.cpp` 的离线检查通过，覆盖独立 producer、四槽背压、同键重复与冲突、水位过期、撤权、实际接口的忙点预约状态、热页不可变、独占阅读及预约超时。

本卡部分交付。完整 D1 仍缺独立 52 动作业务驱动、逐动作终态 sink 和原字对照/重放接线。D2 当前覆盖 compute fence 忙段的单一热页，仍缺 Graphics/Present 忙点，以及异步冷预约、GPU staging credit、冷页谱系和独立文件 writer 的生产接线。既有显式单次冷档仍可由 T859 路径独立复采。下一步由 developer 补这些接口，随后由 tester/effector 按 T858 取真实 PC 对照和退役证据。
