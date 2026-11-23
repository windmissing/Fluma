# T860 · PC 异步 SOURCE 和热页接线

Windows x64 Release 编译通过。原接线 PE 为 `.codex/local-test/t860-release.exe`，SHA256 为 `99181c704081acbc0194143941b84b23a02e2040cad567941762a90976c5c3ef`。补充忙点和 writer 的 PE 为 `.codex/local-test/t860-wsi-release.exe`，SHA256 为 `9e09ae80b4e02bb4c959bbc8509018bbef2344103f08b12e4cb707f388032b21`。M1 compute SPIR-V 沿 T856。

Win32 脉冲通过四槽 `OfferSource` 提交不可变输入，Vulkan owner 消费并生成原 32 字命令。收据保原 QPC、scope、坐标、输入及命令 SHA。撤权在短锁中失效旧项，原提交及尾接受检查继续生效。可选 `--m1-capture-hot` 由 UI 线程在 SOURCE、QUANTUM、Graphics fence 等待或 Present 调用的实际忙段预约。owner 从已验收的 1024 B 热尾封不可变页，UI 读出并结束读权。没有增加 GPU 热路 readback。

显式 `--m1-cold` 仍独立运行，保留首根 16 MiB 上限及封套。owner 完成 Vulkan copy、fence、map 和 lease 退役后，把封存 CPU 字节交给单独 writer；writer 复用 T859 的短写拒绝、flush 和原子落盘。首个 Present 的收据在 writer 完成后附文件信息，writer 不持有 Vulkan 对象。

`test/t860/ingress.cpp` 的离线检查通过，覆盖独立 producer、四槽背压、同键重复与冲突、水位过期、撤权、实际接口的忙点预约状态、热页不可变、独占阅读及预约超时。

## 续接交付

Windows x64 Release PE `.codex/local-test/t860-paged-release.exe`，SHA256 `b02294dcdd5daa88c434cb444f0ce48882a21bd44a51908fcf9ad89f96e8c7e4`。

- `--m1-source-trace` 导出外部提交回执、待消费撤销及 owner 终态。固定环保序号和真实根身份，溢出显式报告丢项。
- `--m1-cold-on-busy FILE` 由非 Vulkan owner 的窗口线程预约首个 QUANTUM 的真实 fence 忙段。owner 只为同一已接受根认领 copy，现有 16 MiB 上限随 credit 保留至 writer/reader 退役。writer 封存原字后提供单读者、64 KiB 顺序页、范围校验以及 60 秒 TTL。文件沿 T848 首根格式，页偏移从原 payload 起算。未预约到首 QUANTUM 时明确报告未采。
- C++ 离线检查覆盖终态日志、过期/背压、同根认领、封页后错游标与重复读取拒绝、顺序完整读取及旧代拒绝，编译和自测通过。

仍属部分交付。完整 D1 尚需独立 52 动作驱动与 OFF/ON 原字对照/重放，D2 后续任意根预约仍需版本化的 backing/arena 和 reader 协议。当前 T848 首根封套只解释首 SOURCE→QUANTUM，继续用于首根证据；跨根重用须由 architect 与 tester 明确新档案语义后再接生产读取。
