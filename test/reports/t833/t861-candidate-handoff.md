# T833 · T861 Windows Vulkan 拆分候选交接

Windows x64 Release 构建通过。PE 为 `.codex/local-test/t833-candidate-release.exe`，SHA256 `78ee42e95543189ef4c376b4b8dfec1df648c9d0b5953d91a49595a28bf93773`。生产计划继续用原 16/820 映射。新候选在独立入口按槽选择原已生成角色，每角色先过真实 OP66 资格门。旧单派发参考登记 `MISSING_PC_REFERENCE`。

| 入口 | 行为 |
| --- | --- |
| `--g13-candidate BASENAME` | 原闭盒 REST BOOT → RAMP SOURCE → QUANTUM。使用真实 Win32 pulse 和原 250 ms / 33,333,333 ns 许可。 |
| `--g13-revoke-before` / `--g13-revoke-after` | 分开运行。前者在提交前撤掉 owner 权限；后者在已经提交后撤权，实际等待该 fence，保私有原字并拒绝接受。 |
| `--g13-fixture F02 CONFIG STATE` | 绑定经原 SHA 和 ABI 校验的 Config128/State 原件，PRIVATE_PROJECT 结果保 `acceptedRoot:null`。 |
| `--g13-fixture F03 CONFIG STATE --g13-trial 1` | 从原始周期剪切初值采首个物理 QUANTUM，trial 亦可选 2/4。 |
| `--g13-fixture F05 CONFIG STATE` | 绑定非零闭盒原初值，保 SOURCE 和物理 QUANTUM 的原场。 |

编译期约束固定为 SOURCE 16 次与 QUANTUM 850 次派发，对应 16/820 个逻辑槽。QUANTUM 的 14 个 OP17 各保 GPU 阶段 3 锁存和 CTRL 首错前缀；每条锁存核 nonce、scope、J、micro、ordinal，保实际 active/NOOP。冷证据单列拷贝计数。Archive 的原字排列为 Completion256、0/14 条槽快照、BASE、CANDIDATE、REPLAY、WORK、CTRL、PACK，JSONL 保偏移、长度、SHA、绑定对象、layout 以及实际角色模块 SHA。额外场拷贝属于取证入口。

本轮验证到 Windows 编译、SPIR-V 生成及静态 16/820、850、14 槽约束。实际角色创建、GPU 提交、撤权和 F02/F03/F05 数值由独立 PC 验证产出。原始候选档仍需映射到 T828 ROOT/FULL 与只读 GridView，F03 首步不能覆盖 30 步振幅或收敛，F05 完整场误差仍待 T866 核对。本交付不能解锁生产候选或目标 14。
