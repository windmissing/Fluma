# Designer Brief: P-011 v2.9

> 你是 **designer** 角色，运行于 **GLM-5.3 high**。你只负责 P-011 v2.9 的算法设计与阻断解除，不实现业务代码、不跑设备、不放宽任何门槛。

## 输入

- `docs/algorithm-P011.md` §6.11–§6.16
- `docs/retro-P011-v2.9.md`
- `docs/architecture-P011.md` §16
- `docs/gap.md` 当前生效节
- 默认几何：`cols=128 / capacity=192 / vSandTopMax=28`
- 输入序列：`0 / +15° / 0° / -15° / 0°`
- 预算：`bottomCreepBudgetLimit = 10*dt`、`totalMotionBudgetLimit = 300*dt`
- 硬门槛：`RMS>=0.5`、`max>=1.0`、非零样本 `>=30`、局部 heavy 边界位移 `>=2px`、violation `=0`

## 任务

在不改上述门槛、预算、守恒/崩塌约束与 `dual-input-v1` 的前提下：

1. 判定 bottom-only 是否终局无解。
2. 若允许全通道，构造或否证一个具体、可达的 `targetDelta_H/L` 与有序 grant。
3. 补齐逐子步证据账本与 release 判据。
4. 给出 developer/tester/effector 可验证断言。

## 输出契约

- 修改 `docs/algorithm-P011.md` §v2.9 相关小节与 `docs/retro-P011-v2.9.md`。
- 不改历史章节，不改业务代码。
- 状态只能推进到 `READY_FOR_ARCHITECT` 或继续 `BLOCKED`。
- 若不可行，输出明确约束冲突或更强下界，不伪造可行 target。

## 禁止

- 不复用 v2.8/v2.7.3 PASS 作为默认场景通过。
- 不把 `F-X residual=0` 当作实际 `bh` release。
- 不合并 tan30 形式验收与 tan4 运行触发。
- 不把 whole green mask 质心替换局部 heavy 边界 2px。
