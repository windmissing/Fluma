# P-011 G1 interim 精确复核报告 v1（12-block J_E 重算+口径核对）

- **日期**：2026-10-23；责任：tester。对应任务【P-011·interim复核】（滴答 6adaf0d6）。
- **性质**：**INTERIM_NOT_FOR_DECISION 全程标注**——本报告任何数值不构成正式判定；不停批（27 block 照跑）；结果不回灌采集流程。
- **触发**：manager 初版 interim（stride=8 采样、手写 E 公式）得全负信号（median −266.6%），需精确口径复核判定真信号 vs 口径偏差。
- **工具**：`tools/run_p011_g1_interim_precise.py`（新，流式解析 48×2.3GB ledger，~29 min）；结果 `test/reports/interim_g1_je_precise_v1.json`。

## 1. 口径核对结论（三疑点全部解决）

| 疑点 | 核对结果 |
|---|---|
| a) thetaAccepted 应否取 slot 级不同值 | **context.thetaAccepted 即每子步接受角**，correction slot 作用于残差而非 theta——slot 前后 thetaAccepted 相同（D run slot 授予子步核验：substep 488-490 thetaAccepted 连续 14.8/14.8/14.7 为输入角序列）。slot 的效果落在 stateAfter.S（更多材料被搬运→S 更平），由 E 公式的 S 项捕获。**无需 slot 级修正。** |
| b) stateAfter.S vs stateBefore.S 取点 | 按 prereg §3 原文 `stateAfter.S`（步后实际达成状态语义）。dx 核验：engine `edge.d` 与 `(S_before diff)/1.0 − tan(theta)` 逐位一致 → **dx=1.0**（ledger S 有 ~5e-4 量化，stateAfter 口径与 dx=1.0 自洽） |
| c) hold 段与 conditioning prefix 窗口切分 | prefix=CND_* 六段（1431 条，65s），formal=H0..H8；E 按分段常值积分（真实 physicsTime 单调轴，hold 期间保留真实当前场）；J_E=积分/窗时长。formal 窗 205.2s |

其余核对：thetaAccepted==effectiveTilt 全 48 run 零失配；executedThresholds.slopeStop 全批唯一值 0.363970234（26/20 同闸门）✅。

## 2. 精确重算结果（12 block 全量、无采样）

| 量 | 精确口径 | manager stride-8 初版 | 判定 |
|---|---|---|---|
| gain median | **−2.7184（−271.8%）** | −2.666（−266.6%） | 方向一致 |
| gain 范围 | [−2.9305, −2.6297] | [−2.893, −2.568] | 一致 |
| gain stdev | 0.0811 | 0.084 | 一致 |
| gain≤0 / ≥10% | **12/12 / 0/12** | 12/12 / 0/12 | 一致 |
| bootstrap median 95%CI | **[−2.749, −2.678]**（10000 次，block 级重采样，seed=20261019） | 无 | CI 上界 <0 |
| A J_E 范围 | 0.007802–0.008309 | 0.0061–0.0064 | 同量级（精确值略高） |
| D J_E 范围 | 0.029294–0.030810 | 0.0222–0.0235 | 同量级 |

**逐 block gain 全列表**：b01 −2.741 / b02 −2.630 / b03 −2.792 / b04 −2.694 / b05 −2.738 / b06 −2.662 / b07 −2.747 / b08 −2.931 / b09 −2.699 / b10 −2.699 / b11 −2.752 / b12 −2.632。

## 3. 判定：①真信号（`INTERIM_NOT_SUPPORTED_SIGNAL`）

精确口径复核后 **gain 方向不变、幅度略增**（median −271.8% vs −266.6%）。口径核对未发现 manager 手写实现的系统性错误——三疑点解决后重算与初版差 <2 个百分点。**负信号是真实信号，不是口径偏差。**

### 3.1 关键增量发现（精确口径才能看到）

1. **非 D 特异**：B（median 0.02984）/C（0.02957）/D（0.02967）三修正臂 J_E 紧聚集，A（0.0078–0.0083）单独低——**所有 correction 臂相对 A 一致抬高 J_E ≈3.7×**，D 的 e²γ 加权既没有更差也没有更好。这改变了归因方向：问题在 correction 通道本身（B 层），不在 e²γ 加权（D 层）。
2. **超出量分解（b01 精确）**：D 的 excess² 总量 245.8 = active 边 171.2（A 的 7.4×）+ inactive 边 74.6（A 的 1.7×）。**J_E 抬升主要来自 active 边**——correction 在 active 边搬运材料压低该边 r 的同时，材料堆积使相邻边/后续子步的 active 边残差重新超过 stop 线；inactive 边仅贡献次要份额（A 自身 inactive 占比 65.6% vs D 30.3%，D 反而更集中于 active）。机制与"清 active 假降 E"相反——E 公式无 active 遮罩且按 stateAfter 计，correction 的搬运在 next-substep 的 state 中留痕。
3. per-phase：D hold 0.0424 vs A 0.0134（×3.2）、leg 0.0250 vs 0.0059（×4.2）——hold 段差异绝对值更大，leg 段相对倍数更高。

### 3.2 非劣门三件同查（同 interim 口径）

| 项 | 结果 | 判定 |
|---|---|---|
| 帧时 | A/B/C/D wall sec mean 389/390/387/388（n=12 各，min 383–385 max 391–403） | 无单项炸掉 ✅ |
| 普通通道挤占 | D correction requested 191,205 / granted 187,609 / rejected 3,596（拒率 1.9%，B 2.0%/C 2.0% 同级）；totalBudgetExcess>0 子步 2,049 个（量级 e-14，预算守恒无实际侵占） | 无挤占证据 ✅ |
| 输入可达代价 | thetaAccepted 序列正常（倾斜 ramp 连续），D 无输入不可达征状；代价主表现为 J_E 抬升本身（效果代价非成本代价） | 登记，非成本类失效 |

## 4. 复核结论（对 manager）

1. **`INTERIM_NOT_SUPPORTED_SIGNAL` 成立**：精确口径下 G1 12/12 block gain 全负、median −271.8%、bootstrap 95%CI 上界 <0，且 B/C/D 紧聚集——不是采样或手写公式伪影。
2. **27 block 不必停**（任务红线遵守：未停批）：负信号已稳定，后续 block 更可能确认而非翻转；正式判定等全量。
3. **归因方向修正**：J_E 抬升是 correction 通道整体效应（B 已显现），非 D 的 e²γ 加权缺陷——designer 的后续机制研究应从"B/C/D 共同的 slot-apply 与 E 累积交互"入手，而非 D 算法本身。
4. **可能的关键问题（供 designer 参考，非本报告结论）**：E 公式按 prereg 用 stateAfter.S **无 active 遮罩**——correction 搬运材料在物理上确实降低了 active 边残差（rAfter<h），但搬运的副作用（邻边变陡、材料重新分布）全部计入 J_E。若 designer 认为"对系统总残差的净效应"不该由 correction 臂承担搬运副作用，那是 estimand 层面的复议（须新 revision），不能临场改口径。

## 5. 红线自查

NOT_FOR_DECISION 全程标注 ✅；不停批、不提前裁决 ✅；interim 结果未泄露给采集流程（本报告完成后才落盘，且采集脚本不读取）✅；冻结锚点/口径零改动 ✅；纯离线（48 ledger 流式解析 29 min）✅。

## 6. 制品

- `tools/run_p011_g1_interim_precise.py`（精确 J_E 重算器，流式、bootstrap 内置）
- `test/reports/interim_g1_je_precise_v1.json`（全量逐 block 逐 variant 数值+sanity+bootstrap）
- 本报告：`docs/interim-g1-review-P011-v1.md`
