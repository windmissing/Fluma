# P-011 v2.13 slip-overshoot 根因复盘

> 独立缺陷 route：`slip-overshoot-v1`；状态：`BLOCKED`；最高优先级；证据 run `p011-run-1792397031250-3`。

## 结论

2,512 条子步记录中发生 484 次坡度 residual range failure（正向 overshoot 268、正向 return 5、负向 overshoot 206、负向 return 5），超过 `tan30°=0.577350`。首次 crossing 不是某个非 slip 材料通道造成：被测 `maxSandTopInvariantError` 是 `|S[c+1]-S[c]-tan(effectiveTilt)|`。首个越界正向边 `c=57` 的原始材料坡度约 `-0.003118`；effectiveTilt 更新至 36° 后、任何 material grant 前 residual 已为 `-0.729661`。负向首个越界同样由实际角度推进到 -36° 后触发，边 `c=117` residual 为 `+0.798533`。

ledger 的 context/stateBefore 在 `substep()` 之前采样，`updateTilt()` 则在 `substep()` 内先执行；所以首个 failure 的 context 标 24°，而 audit/grants 对应更新后的 36°。该对齐问题会让事件标签落后一子步，须在下一次观测中修正/独立核验。

## 两个问题的证据回答

1. **哪个通道推过线？** 首个 crossing 不是材料通道。角度参考更新单独就把仍近乎水平的沙面推过 residual 硬线。之后 settling 可局部放大既有超限：正向峰值 sequence 1724 的边 `c=3`，settle 将 residual 从 `-1.647012` 加重到 `-1.659934`；slip 随后反向修正到 `-1.647218`。所以 settle 是后续放大项，不是起因。代码中的 range 检查是 post-step audit，未提供 pre-commit admission guard。
2. **relax 为什么没追上？** 孤立边 `excess/2` 在首个越界样本中请求约 `0.329867`，数学上足以将 residual 从 `0.729661` 拉到 `tan(4°)`。但 127 条边同时申请近似均匀通量，内部列入/出抵消，实际坡度更新取决于 grant 的空间差分。再加上该步 total request `52.3944` 对 `5.0` 上限，slip aggregate 申请 `40.8176`、实际仅 `3.8952`（9.54%）；相邻边约 `0.03148` 的同量级 grant 使最大 residual 只改善约 `0.0000646`。因此主因是倾角参考推进与材料 response 不匹配，repair 同时受总预算缩放和空间通量抵消；不是 isolated `excess/2` 公式太小。bottom budget 只用了 `0.0159/0.1667`，不是限制源。

全正/负 overshoot phase 的 slip applied/requested 分别约 5.77%/5.69%。所有正向越界子步、全部负向越界子步至少 90% 使用 total cap，显示该问题持续存在。visible `surface-slope.png` 的整体线性坡角与局部 residual 不是同一指标，不能用前者替代后者。

## 路线判断

双角度滞回不能单独修复本问题，因为它没有改变 effectiveTilt 更新，也不能保证被限流后的离散 grant 足以降低全场 residual。后续单独设计须覆盖 tilt update 可行性、改变 `sandTop` 的所有通道之 pre-commit hard guard、现有 total/local/bottom 预算竞争，以及观测 context 与 grant/audit 子步对齐；不得增加预算或放宽 `tan30°`。当前 `slip-overshoot-v1` 保持 BLOCKED，不进入 developer/tester。此复盘只更新设计文档，没有修改代码或测试用例。

## 证据摘要

- `summary.json`: 首次 audit failure recordIndex 1456 / sequence 1457，error `0.7295962158`；484 range failures；无预算超额。
- `ledger.ndjson`: 2,512 records；首次 crossing 首边 grant/request、stateBefore/After 和 phase 汇总见 algorithm-P011 §8.12。
- source manifest 中 WaterSandField SHA256 与本次复核源码一致；观测实现的 begin-snapshot / updateTilt 顺序见 `WaterSandField.ets`。
