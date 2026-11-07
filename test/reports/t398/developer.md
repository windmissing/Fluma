# T398 · GPU 控制与提交隔离 V1

沿原 `grsm1` 入口。签包及 SHA256 见[签包回执](../t257/T398-handoff.json)。控制、GPU 进度和已接受物理时钟在证据中分列。V1 继续以原 Advance anchor 加 33,333,333ns 判 GAP，保留 120Hz/carry 与 8ms Pump guard。

控制线程使用既有 16 项 mailbox，锁内只转移控制消息、冻结许可或线性化接受。每根冻结 epoch、run、owner、context、surface、geometry、config、input generation、permission revision、nonce 与绝对期限。状态、后台和窗口撤销可在 GL 卡住时运行。真正的清理仍由原 EGL owner 完成。可执行续批不固定等待 8ms，BUSY、fence drain 有短等待。

Source/quantum 的 wait、1KiB map/unmap、原完成校验均在锁外。原 owner 的 bank、publication、event prefix、source ACK 和物理提交在复核后的短临界区一起接受。当前源 ledger/dedup 的可见前缀以接受 bank 长度为准，shader 仅在其尾写候选。候选事件用独立 SSBO 并在接受时换槽。窗口与截止点失效后，未封尾和带 fence 的工作分别收尾。GL 资源的回收仍取决于原 owner 返回。

独立事件缓冲常驻增量为 10,844,416 字节（K192/Kw64）、11,892,992 字节（K192/Kw128）或 27,785,472 字节（K512/Kw128）。

Swap 前后分别复核许可。迟到的实际交换单列，只有有效的交换和收据才可发布 CURRENT。上一已接受显示以完整 token 标 STALE。时间线沿原 4096 环，`T398_STAGE59_V1` 保留旧 1–57。58 的 carry 为 cut（1 staging、2 dispatch、3 flush、4 fence、5 wait、6 map、7 audit、8 accept、9 swap、10 batch），A/B 为前后剩余 ns，C 为许可代次。59 记录拒收时的 deadline 与新旧代次。58 的 swap code 0/1 表示有效的 native 成功/失败，2/3 表示换代后的 native 成功/失败。49 用控制实际观察撤权的时间及当时接受计数。

宿主实 GL 通过 5/40/65ms、阻塞期间独立撤权、Poll、map、审计与提交之间、Swap 进入前和返回后的换代。拒收保持旧账本，5ms 源 root 正常交换。合法 SOURCE 与 QUANTUM 对拍 bank、非空 event、publication、completion 和 owner header。已执行并封尾的同 arena 拒收负控保留所有已接受证据。普通运行、桥与冷导出兼容性随回归执行。Hvigor ArkTS/native 和签名校验通过。未使用真机，T399 负责设备取证。

旧宿主墙时用例在单次 GL 调用实际超过 8ms 时曾要求仍执行多个批次。用例现在检查该 Pump 必须在 guard 后保留未封尾状态，下一 Pump 接续原 nonce。受控时钟用例仍单独覆盖多批连续推进。

复跑：`node test/t398/build_host.cjs`、`cmd /c test\t375\build_host.cmd`、`node test/t398/cold_units.cjs`。
