# T860 · PC 异步 SOURCE 和热页接线

Windows x64 Release 编译通过。原接线 PE 为 `.codex/local-test/t860-release.exe`，SHA256 为 `99181c704081acbc0194143941b84b23a02e2040cad567941762a90976c5c3ef`。补充忙点和 writer 的 PE 为 `.codex/local-test/t860-wsi-release.exe`，SHA256 为 `9e09ae80b4e02bb4c959bbc8509018bbef2344103f08b12e4cb707f388032b21`。M1 compute SPIR-V 沿 T856。

Win32 脉冲通过四槽 `OfferSource` 提交不可变输入，Vulkan owner 消费并生成原 32 字命令。收据保原 QPC、scope、坐标、输入及命令 SHA。撤权在短锁中失效旧项，原提交及尾接受检查继续生效。可选 `--m1-capture-hot` 由 UI 线程在 SOURCE、QUANTUM、Graphics fence 等待或 Present 调用的实际忙段预约。owner 从已验收的 1024 B 热尾封不可变页，UI 读出并结束读权。没有增加 GPU 热路 readback。

显式 `--m1-cold` 仍独立运行，保留首根 16 MiB 上限及封套。owner 完成 Vulkan copy、fence、map 和 lease 退役后，把封存 CPU 字节交给单独 writer；writer 复用 T859 的短写拒绝、flush 和原子落盘。首个 Present 的收据在 writer 完成后附文件信息，writer 不持有 Vulkan 对象。

`test/t860/ingress.cpp` 的离线检查通过，覆盖独立 producer、四槽背压、同键重复与冲突、水位过期、撤权、实际接口的忙点预约状态、热页不可变、独占阅读及预约超时。

本卡部分交付。完整 D1 仍缺独立 52 动作业务驱动、逐动作终态 sink 和原字对照/重放接线。D2 热页当前单预约；全量冷档仍缺独立异步预约和多次读页接线，包括跨根 backing 与 credit 生命周期。现有 T848 首根封套尚无后续任意根解释，不能直接复用为后续根的冷页协议。下一步由 developer 补上述接口、由 tester 承接对应 reader 与固定动作，随后按 T858 用真实 PC 交付对照和退役证据。
