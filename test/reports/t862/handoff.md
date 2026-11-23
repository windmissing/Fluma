# T862 · PC M1 双源首阶段接线

PC x64 Release 编译成功，PE 为 `.codex/local-test/t862-release.exe`，SHA256 为 `9fc0985c6e03984f29275da1deb9ecc1469a0b87e02283c64ce9c27bcc185877`。

沿 T856 的三个已固定 SPIR-V，使用 `--m1-window SOURCE CANDIDATE COMMIT --m1-dual-source`。键 `1` 选沙，键 `2` 选水。切换撤销原 hold 和待消费项，重新按下后走同一 Vulkan owner、原许可和原 SOURCE V2 命令。水源使用 FullM1 原有的到达分流及 `16 × 2^20` 发射速率规格。双源 profile 保持同 run 的已接受库存和 QUANTUM。

| ABI | Windows Vulkan 接缝 |
| --- | --- |
| SOURCE V2，128 B | command32 的 kind 2、sourceId 14、坐标/量 16–21，Completion256 保原 admitted 和 mode |
| K192/Kw128，publication 31,744 B | sand16 起始 word 0，water20 起始 3072，column16 起始 5632，header 起始 7680 |
| 同代 SceneLease | 由同一已接受根和 publication lease 发出沙水库存、来源累计、活跃数量及 WSI 身份，场景 shader 读取相同 GPU publication |
| 图形模块 | 双源 SPIR-V 9,004 B，SHA256 `d7d73a46ac11532bc04cb63ea08c80c1cbb98b7b90e8db446b6d3a4c6a88a151`；干沙 SPIR-V 的 SHA 与 T860 相同 |

离线检查验证了新 hold 保留 run、非法材料和越界输入被拒，双源片元通过 SPIR-V 验证。T852 的真实 PC 两相、逐列来源账与像素仍待 tester/effector 取证。当前窗口沿 T856 首根后呈现，空箱独立呈现、完整水面光学、扩散和性能证据各沿后续目标验证。首阶段 shader 显示现有 ABI 的床、在途沙、水体和实际飞行水包。显式 T848 首根冷档保持原干沙解释范围。
