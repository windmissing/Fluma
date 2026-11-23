# T872 · PC M1 有序脉冲与许可退役交接

Windows x64 Release 编译通过。PE 为 `.codex/local-test/t872-release.exe`，SHA256 `6ec0ec5c9e401ff4f7000e305f4acc989c2e5f9db35b8e6bfc97474ec8669871`。M1 SOURCE 和 QUANTUM 仍加载原三份 SPIR-V。

- Win32 callback 逐拍登记 callback QPC 和 native receipt QPC。32 槽脉冲队列独立于 SOURCE 四槽，按 run、geometry、session 和重新锚定的 QPC 边沿隔离。owner 消费保存的实际时间，只用当前 QPC 复核 fresh 和 deadline。真缺拍、反序、旧 scope、溢出均可区分。
- SOURCE 与 QUANTUM 以实际许可、submit、短轮询、owner 接受及退役边沿出收据。失败保首个原因和真实 VkResult。普通收据保最后一轮 wait，`--m1-source-trace` 额外输出成功根的有界逐轮记录，失败根始终带有界记录。Win32 与 owner 通过定容队列把输出交给 writer，输出溢出另报。
- 拒绝的已提交根先 tombstone，再据同 fence 退役或实际 DEVICE_LOST 失效。退役未闭合禁止复用。首帧和续帧均关旧 offer。可恢复的缺拍或溢出在同代、已退役条件下等待新真实 baseline 与新 Down。续帧等待报告 STALE，首帧等待报告 FAULT。已有接受 bank 保留。

离线自测使用生产脉冲队列与 `ProgressClockV2`。owner 相隔 39 ms 消费时，实际 16 ms 连续脉冲保许可，真实 callback 缺拍、receipt 缺拍及无新拍超龄仍撤权。覆盖旧代、新 session、反序、溢出和取消，以及 tombstone 期间拒绝重提、同 fence 退役后迟到不接受。T860 SOURCE 和热页离线回归通过。

33,333,333 ns、250 ms、120 Hz 及四份配额保留原值。双源续帧、同场景沙水与实际 Vulkan 退役证据按 T867 交独立 PC 复核。
