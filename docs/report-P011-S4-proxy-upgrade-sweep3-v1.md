# P-011 S4 proxy-upgrade 三轮 sweep 记录（2026-10-25）

状态：**BLOCKED_MISSING_PRESSURE_DEFINITION**。依据 [S4 代理升级域建议](architecture-P011-S4-proxy-upgrade-domain-recommendation-v1.md) §3，先冻结量纲再扫 μ。

当前实现的 `pEff` 是相邻列 `sandTop` 的几何平均，约为长度/材料高度代理；代码将 `deltaBh` 与 `muY*pEff` 直接比较，缺少有效压力 `pEff(Pa)`、有效密度 `rhoEff` 及固定参考压力。两边量纲不能闭合，因此不得从现有 μ=0.1 旧实验推新候选网格。

| 轮次 | 计划 | 状态 | 记录 |
|---|---|---|---|
| 1 | 冻结量纲并执行 μ=0 对照 | BASELINE_ONLY / 未执行 | 只有在正确模型实现后 μ=0 才可作为物理闭合的对照；现有几何代理代码不构成该基线证据。|
| 2 | 从固定非均匀场驱动比 `P_drive/p_eff` 分位数生成候选 μ | BLOCKED_MISSING_PRESSURE_DEFINITION | 无有效压力与密度字段，分位数不可计算，候选网格留空。|
| 3 | 在 2–8px 域复验稀疏低/中/高 μ | BLOCKED_MISSING_PRESSURE_DEFINITION | 依赖第 2 轮候选值，未执行。|

三轮均没有改 `k≤0.2`、能量式、预算、对称/守恒或零通量闸门。Node 结果不是效果 PASS，也未使用真机。机器可读记录见 [s4-proxy-upgrade-sweep3.json](../test/reports/s4-proxy-upgrade-sweep3.json)。恢复 sweep 前须由 designer/manager 登记 `pEff/rhoEff` 的物理来源和单位。
