# T882 · PC 双源预约与 D1 退役

2026-11-23 · developer · Windows x64 Release。

| 工件 | 源码提交 | PE | SHA256 |
| --- | --- | --- | --- |
| A · T880 双源预约 | `d6395752` | `.codex/local-test/t882-a-final.exe` | `86e8ad81779157a94558bea64e067984045ed0a3801e1a2c098335f7180e06fc` |
| B · T881 D1 退役 | `b683d90a` | `.codex/local-test/t882-b-final.exe` | `12d454b11469160ef2ff5f29dd9aaf8905a2d512d95bf070dc391d00ea44db28` |

A 可单独复验，B 源码叠在 A 上。上表为同一构建基线复编的最终工件。[A 初次编译工件及行为说明](source-reservation.md)另存。两个最终工件的 `pc_steady_spirv.h`、`vk_boot_spirv.h`、`pc_view_spirv.h` 和 `assets_manifest.json` 均逐字节相同。

## B 原始证据

- `M1_D1_READY` 给出实际 READY 锚点与 QPC 频率。52 条 `M1_D1_ACTION` 保留计划 ns、实际 callback、native receipt、session、序号及原始结果。 `M1_D1_PULSE_OWNER` 给出对应 owner 消费或撤销原码。
- `M1_OFFER_RETURN/TERMINAL` 的 `actionId` 为合法 JSON 数字。SOURCE 保留实际 Δ、过期年龄、动作与根身份。A 的普通交互记录使用 `M1_RESERVATION_RETURN/TERMINAL`。
- `END_INTAKE` 记录末次真实脉冲 session/seq、offer 序号及取消的 capture request。封口与新根提交互斥。已经提交的根仍按原许可和截止时间判断，tombstone 经实际同 fence 信号和 owner Poll 退役。封口后最多一秒的清理账单独记录 owner fence、WSI retire、offer、lease、剩余配额、carry 和 trace loss。迟到根不会成为接受根。真实设备丢失和 drain 超时保留失败原码。
- 唯一 `M1_D1_DRAIN` 是业务退役原始记录。 `M1_D1_CLOSE` 记录其后的实际窗口销毁入口。四臂对比须用真实时标和根字。先做两次 OFF，再按 T881 合同决定是否进入四臂比较。

## 编译与离线检查

两个 Release 均完成编译和链接。T882 A/B C++ 离线自检、T860 ingress/hot page 和 T872 pulse/tombstone 回归通过。pending 与已 signal 的路径用离线 owner 测试覆盖。PC 双源续帧、四臂时域等价和实际 Vulkan drain 由独立复验采证。

纯 Git 检出缺既有冻结输入及 `g13flow/root_binding.h`，且 G13 角色生成对产出的 `OP_62` 报断言，当前 PC 构建要求 `OP_61`。双工件使用同一份既有 G13 工作区构建输入，T882 提交未纳入这些变更。依赖快照 `.codex/local-test/t882-shared-build-inputs.zip` 的 SHA256 为 `bee27cfa64e3eb291f580c626c9a9a999d55764f454d74976cfdc41f88d1fe30`，内部逐文件记录 SHA256。三份批准设计原件和一份文档行尾按 T607 manifest 指纹核对。两个构建日志分别是 `.codex/local-test/t882-a-clean-build.txt` 和 `.codex/local-test/t882-b-clean-build.txt`。
