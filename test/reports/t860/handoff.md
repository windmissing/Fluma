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

## D1 四臂与后续根归档

Windows x64 Release PE `.codex/local-test/t860-d1-release.exe`，SHA256 `f41be707566830dfb13ab126b0e47b1ddea4175f67669ccbd7dd9086dfd20cd6`。

- 在既有 dry `--m1-window` 加 `--m1-d1 OFF|ON|REPLAY_OFF|REPLAY_ON`。Win32 owner READY 后启动独立 producer，使用与 T865 相同的 52 动作，记录声明时刻和实际 QPC 时刻。SOURCE 动作由下一次实际 pulse 通过同一四槽提交，按 actionId 联接 producer 回执和 owner 终态。QUANTUM 收据保实际 UI ns 以回配 pulse。OFF 的 capture 动作为具名空操作。收尾保真实 fence、lease、offer 和许可状态。
- REPLAY 两臂额外传 `--m1-d1-replay-prefix PATH`，每个已接受 SOURCE/QUANTUM 在原 owner 上从实际 bank、event、publication backing 复制原字，先取 fence 与 invalidate 结果，再允许后续写者。累计原载荷上限 16 MiB，耗尽时保前缀并报码。OFF/ON 主臂没有这条 readback。
- `--m1-root-on-busy FILE` 在第一张业务图像后预约后来任意 QUANTUM；实际认领根与忙点 nonce 匹配。独立封套 `M1PROOT1` 为 8 字节 magic、8 个 LE U32 长度、32 字节 payload SHA、4 段原载荷（Completion256、已接受 bank、实际 arena 的 `[0,eventEnd)`、同代 publication）。后 4 个长度为零。Completion 绑定 eventStart/end、bank、slot、generation 与提交根；JSONL 补真实 backing 和 fence。T848 的 `M1PCOLD1` 首根路径仍保原解释。

编译及离线调度、队列、封页检查通过。四臂的实际时域等价、原字对照和后续根 reader 由 T865/T869 独立取证。交付的 JSONL 是原始边沿，T865 比对器仍需将其按 schema 联接成四臂证据。
