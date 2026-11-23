# T890 · PC 观测、Vulkan owner 与退役时间戳

2026-11-23 · developer · Windows x64 Release。编译与链接通过。三份离线自检通过，范围为 ROI 边界、时间戳位宽回绕、同 grant 冻结角度、两族派发表与原档范围。硬件路径交独立 PC 取证。

| 分项 | 工件 | SHA256 |
| --- | --- | --- |
| A/C 独立开关，共用 Win32 PE | `.codex/local-test/t890-observation-timing-release.exe` | `1e49f2a6f7aadef3bd9b047c647ba21cccf57de23a5fd8809dd01f771c69671c` |
| B · DM2 | `.codex/local-test/t890-dm2-release/pc_sand_dm2.lib` | `cab9d2cc53446886efa681bd0625a178a8ca46938bcc6a1fbf68877d961d9ed0` |
| B · R1 | `.codex/local-test/t890-r1-release/pc_sand_r1.lib` | `ec2f433ab5f82c34126fdd1a989c3f577346557a78da9b95646871a9c0108a5c` |

## A · 空箱、重力和同根观测

- 在原 `--m1-window SOURCE_SPV CANDIDATE_SPV COMMIT_SPV` 增加三个可分别启用的选项。 `--m1-empty-preview` 先以实际初始化 fence 和 shader 呈现发 `EMPTY_READY`，根为空、沙水库存为零。首张空窗进 WSI 队列后才开启 pointer READY 和真正 SOURCE 接受链。同一 WSI 循环续接业务 publication。
- `--m1-manual-gravity` 启用左/右键每步 15°、Home 回中，限定在 ±30°。自动驱动可在同窗口发 `WM_APP+3`，WPARAM 为有符号角度索引，LPARAM 为窗口 geometry。+15° 为 3840，−15° 为 −3840。 `M1_GRAVITY_INPUT` 记录真实 QPC、序号和越界/旧代拒绝。新 grant 一次冻结，每个 QUANTUM 连同原 `angleAcceptedIndex` 记到账。
- `--m1-observe REQUEST PREFIX` 支持同一接受帧的定点取证。REQUEST 是文本，先写从 1 开始的业务呈现序号，再依次写床、粒、背景三组 `x y width height`。每组最大 16×16，使用窗口实际像素坐标，左上原点。 `m1Observation` 含同 root/slot/generation、geometry、imageIndex、graphics fence/serial、实际格式、颜色编码、原字节及 SHA。像素由同一绘制命令缓冲复制，等待图形 fence 才读。随后在新根提交前捕获同 nonce 的状态、有效事件、publication 和 Completion，输出 `PREFIX-<nonce>.m1root`。 `M1_OBSERVATION_COMPLETE/NOT_CAPTURED` 分别记录原件或终态。取证开关会增加诊断复制与 I/O，长窗吞吐另用普通模式。

## B · DM2 和 R1 的独立 Vulkan owner

`pc/vulkan/sand/owner.h` 是 native 接口。宿主提供真实 device/queue、冻结输入和实际 buffer/memory，保队列串行及对象代。两个库都执行照原角色表校 SHA、创建 pipeline、实际 BOOT 派发、同 fence 验审计原字。接受根在实际尾 fence 与映射后进原 T647 typed gate，保 250 ms 根期限和短轮询。宿主在短锁下提供当前 scope/freshness 守卫并依原 typed proof 处理 bank 晋升和 publication 生命周期。

- DM2 交付 45 个计算模块，对应 12/2404 原槽。 `Run` 可预留并录制六个 FULL discovery 区。 `CaptureDmArchive` 从同一已接受根的真实 BASE、FINAL 和 arena 另排复制，等待真实 copy fence、按对齐失效，再写原 `G9_SPF_DM_COLD_V2`。两种档分别约 26.8 MB / 103.4 MB。
- R1 独立交付 49 个计算模块与 6 个图形模块，对应 19/2543 原槽。 `Run` 根据实际 frozen Common 的视觉 bank 及所选批次录入 WORK， `ReadR1Witness` 同 fence 校计划 seal、nonce、micro、phase、role 和 typed proof 后借出原字，供宿主写 witness 封套。沿 T886 的 `R1SceneGate` 联接同代 shader lease。两个 native 库以独立 CMake 目标供 Windows 样机宿主链接，常用 M1 PE 的命令行仍走 M1 业务入口。
- 角色清单与编译模块在各自的 `sand_assets`。下一轮用实际 GPU 验 BOOT 数值、真实接受根、设备和资源代。DM2 取 ROOT/FULL 原件，R1 取同根非零通量 WORK 与窗口回执。静态 manifest 的硬件资格为 `NOT_EXECUTED`。

## C · G13 拒绝根与冷复制

沿原 `--g13-diagnostics full|hot` 采原始 GPU query。保 root start、business copy、独立 cold copy 与原 ACTUAL、REPLAY、COMPARE/PACK 标记。查询池、written mask、queue/device、run/nonce 及 fence generation 跟随根。只有同一 fence 真正确认后才以 availability 无 WAIT 读，缺点输出 `UNAVAILABLE` 或空 span。 `LATE_RETIRED_DIAGNOSTIC` 保业务拒绝和 `acceptedRoot=null`，冷复制迟退另报 `LATE_COLD_COPY_DIAGNOSTIC`。业务截止、写集与原 850 派发保原合同。

Win32 PE 复用了 T882 既有 G13 构建输入，已核快照八项 SHA 与当前输入匹配。[T882 复建基线](../t882/delivery.md)。
