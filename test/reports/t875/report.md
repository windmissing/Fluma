# T875 PC 异步 SOURCE D1 四臂实际取证

- 任务：T875 PC 异步 SOURCE D1 四臂实际取证
- 角色：tester（PC 实测）
- 日期：2026-11-23
- PE：`.codex/local-test/t860-d1-release.exe`（SHA256 `f41be707566830dfb13ab126b0e47b1ddea4175f67669ccbd7dd9086dfd20cd6`，卡面指定 T860 最终件）
- 三 compute SPIR-V 沿原 SHA（1912af9e/b8f0e327/caf0614c）
- 全程 GPU 驻留，无 Canvas 回退

## 四臂结果（每臂 52 动作）

| 臂 | 动作终态 | 热页 | Source | 真实退役 | 退出码 |
|---|---|---|---|---|---|
| OFF | 48 DELIVERED + 2 AWAIT_PULSE + 2 OFF_NOOP | —（具名空操作） | 受理 | PRESENT_RETIRED | 0 |
| ON | 48 DELIVERED + 2 AWAIT_PULSE + RESERVED + ENDED | root 15832683861715，safePoint=5，256 字 Completion | 受理 | PRESENT_RETIRED | 0 |
| REPLAY_OFF | 48 DELIVERED + 2 AWAIT_PULSE + 2 OFF_NOOP | — | 受理 | PRESENT_RETIRED | 0 |
| REPLAY_ON | 48 DELIVERED + 2 AWAIT_PULSE + 1×RESERVE/NOT_BUSY + 1×END/NO_PAGE | —（本轮忙点未命中，属合法终态） | 受理 | PRESENT_RETIRED | 0 |

## 核心验证

1. **52 动作原终态**：四臂动作表与 T865 `plan()` 逐项一致（id/kind/时序 17ms…769ms；scriptNs 仅字符串 vs 整数类型差）；每臂 52 个唯一 terminalId；actualNs 单调。
2. **隔离重放同根原字**：REPLAY 两臂共产出 27 个 `M1PROOT1` 根原件（62792/63048 B），全部 32 字节摘要验证通过、Completion SEAL 全过；ron2 批 9 件 nonce 逐一映射到同进程日志根（1 SOURCE 根 = m1Nonce + 8 QUANTUM 根 = m1QuantumNonce）——重放拷贝取自实际 bank/event/publication backing。
3. **真实退役**：四臂全部 M1_QUANTUM_PRESENT_RETIRED（off2/on2/replay-off2/replay-on2 复跑批）。
4. ON 臂忙点预约：RESERVED→ENDED + M1_D1_HOT_PAGE 256 字 Completion（热页归 T869 已验机制，此处为 D1 控制流实际取证）。

## 负证据（如实保留）

1. ON 臂首跑：SOURCE 失败 PERMIT_REVOKED（m1PulseFault=CLOCK_GAP），后续 32 动作 REVOKED；同参数重试成功。背靠背负载下调度脆弱性。
2. REPLAY_OFF 首跑：3 根采集后续帧 QUANTUM FENCE_OR_PERMIT_FAILED（已知 iGPU 250ms 续帧时限）；根采集本身完整。
3. D1 producer 的 offer 未以 M1_OFFER 行 terminal-drain（owner 内消费），T865 ingress/compare 完整四臂 join 仍需其交接声明的外部适配器（静态 plan/动作身份/热页/重放原件判据已独立全过）。

## 交付

- 证据：`test/reports/t875/`（四臂×2 批 JSONL + 27 个 .m1root + summary.json）
- 原件交 T496（52 动作原终态 + 重放同根原字 + 真实退役），热页部分沿 T869 不重复
