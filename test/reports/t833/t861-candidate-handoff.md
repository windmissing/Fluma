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

## F03 时序与 T607 导出

新版 Windows x64 Release PE `.codex/local-test/t833-series-release.exe`，SHA256 `99994ce278f945d5ed3caa8da7440579e77c99bb8070509824bec47b7c1a8809`。F03 入口连续推进 30 个物理 QUANTUM，每步沿真实 Win32 pulse、预算和许可，前一根接受后才交换 bank/pack 进入下一根。每步另存 `BASENAME.quantum.N`、GPU Completion256、原生接受时刻和绑定。首错保留已生成前缀。F05 可通过 `--g13-f05-rest` 选择 REST 取样入口，默认仍采 RAMP SOURCE 路由。

`python pc/vulkan/export_g13_candidate.py RAW RECEIPTS_JSONL OUTPUT --mode FULL` 将 V2 原档按 T607 长度、目录和 SHA 生成分段 JSONL。使用 `--mode ROOT` 导出 ROOT；随档给外部 SHA 收据及 `.grid.json`。GridView 的 u/v/piQ32 取 CANDIDATE，φ 取同一根 GPU PACK 第 3 字段，形状、nonce、geometry、commit、slot 和 Config SHA 必须匹配。两份原 PACK 都写入 FULL；OP17 私有尾段保在原档。场导出不声明窗口呈现。

验证记录为 Windows x64 Release 编译通过，ROOT/FULL 合成封套均由原 reader 判 `STRUCTURALLY_COMPLETE`，错 PACK nonce 被拒。仍需独立 PC 跑出 F03 的实际 30 根和各病例数值矩阵。T837/T866 完整物理入口还需 checker 对齐原字语义：G13 GPU 原 Completion256 的 QUANTUM opcode 在 word 4 为 0，word 32–33 保留零，实际接受时刻取同根 `nativeAcceptedNs`。现有 T837 在这两处消费合成 fixture 的 opcode 2 和尾字时刻。交接件保留全部实际原字，需 tester 更新消费接缝；原数值阈值和生产 16/820 映射保持冻结。
