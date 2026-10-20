# P-008 解法设计：列标量 `f_sl` 偏析率闭合

**状态：`DESIGN_ONLY / NOT_CALIBRATED`。** 本稿把 [algorithm-P011.md §11.1](algorithm-P011.md) 的预研整理为可进入实现评审的设计方案；不实现、不改当前默认 `percolationRate=6/s`，不解除 P-008 挂账。本文使用的公式与代理量仍须完成来源确认、物理映射和验证后才能标定。

## 1. 目标与边界

P-008 仅替换 `percolateHeavyConservative` 内生成分选预算的常数速率 `6/s`，按列计算候选 `k_seg,c`。列标量状态、heavy 向下分选方向、q/r 等量交换、逐列库存/容量限制、总运动预算及守恒审计均沿用现有实现约束。不得把 `f_sl`（长度/时间）直接用作速率，也不得新增或重发任何预算。

本路线不改变 `settleSandV2`、`creepBottom`、`relaxAngle` 的物理定义或先后关系；若实现基线的冻结算子次序与本文不一致，先按评审 11.2 的冻结顺序更新接口映射，再进入实现评审，不得在本项中顺手重排算子。

## 2. 输入闭合与常数率替换点

### 2.1 每列计算

定义相邻表面相对重力目标坡度的残差：

```text
d_e = S[e+1] - S[e] - tan(effectiveTilt) * dx
chi_c = abs(d[c+1] - d[c-1]) / (2*dx)              // interior
```

边界列使用相邻单边差分；列数不足 3 时 `chi=0` 并记录低阶边界。令已提交的前一子步非渗滤表面活动量为：

```text
activityRate_c = max(
    abs(effectiveTiltRate_radPerSec),
    maxNeighbor(abs(nonPercolationDeltaSLastStep)/(dtLast*dx))
)
gammaDot_c = chi_c * activityRate_c                // 1/s
```

`nonPercolationDeltaSLastStep` 仅由已完成的 settle、creep、relax、lift 等非渗滤 writer 记录；不包含当前/前次 percolation 的变化，避免自激反馈。无有效历史、`dtLast<=0` 或 activity 为零时令 `gammaDot=0`。该闭合是工程代理；`chi` 单独不是剪切率。

定义论文颗粒身份：小/大粒直径 `d_S,d_L`，`R=d_L/d_S` 且 `1<R<=5`；大粒体积分数 `phi_L,c`：

```text
dBar_c = (1-phi_L,c)*d_S + phi_L,c*d_L
F_R,c = (R-1) + E*phi_L,c*(R-1)^2
p_eff,J,c = rhoEffBulk_c*g*ell_J*(1-exp(-z_c/ell_J))
f_sl,c = B*rhoStar*g*gammaDot_c*dBar_c^2
         / (C*rhoStar*g*dBar_c + p_eff,J,c) * F_R,c
k_raw,c = signHeavyDown * f_sl,c / dBar_c           // 1/s
```

常数候选为 `B=0.3744, C=0.2712, E=2.0957`。浅层 `z<<ell_J` 时压力项退化为 `rhoEffBulk*g*z`；深层 `z>>ell_J` 时趋于 `rhoEffBulk*g*ell_J`。`z=sandTop*lengthPerModelUnit`。不得对不兼容的方向取绝对值：如果 `k_raw<0`，该列/该材料映射标记 `routeIncompatible` 并拒绝分选，不可产生反向或伪正向通量。

### 2.2 接入点与调用时序

唯一替换发生在原算子 `percolateHeavyConservative(dt)` 的列循环中、计算旧 `budget=6*dt*(through+pair)` 的位置。事务采集列输入快照，逐列计算 `k_seg,c` 与 request；通过现有 percolation local/total budget 分配器取得 grant 后，才提交 q/r。调用时序冻结为：

```text
settleSandV2(dt)
percolateHeavyConservative(dt; rateProvider = k_seg,c)  // 本项替换 rate 源
creepBottom(dt)
relaxAngle(dt)
liftByShake(dt)                                         // 若基线顺序如此
```

评审 11.2 的全通道冻结顺序优先；P-003/P-005 §20.1 明确的局部次序是 `percolateHeavyConservative → creepBottom → relaxAngle`。故本项只保留原渗滤槽位，即在 `settleSandV2` 之后、`creepBottom/relaxAngle` 之前；`liftByShake` 及其它 writer 不由本项重排。活动率使用前一已提交子步的非渗滤表面增量，因此本步较早执行的 settle 不可被临时计入本步输入；这样输入快照不依赖算子遍历顺序，并避免 percolation 与本步 settle 间的循环定义。

## 3. q/r 拆分与守恒/防过冲证明

对每列保留既有定义：

```text
through = min(mh, bl)
pair    = min(mh, ml)
M       = through + pair
budget  = k_seg * dt * M
request = min(0.5*M, budget)
q = min(through, 0.5*through, request)
r = min(pair, 0.5*pair, max(0, request-q), mh-q)
```

要求 `k_seg=max(0,k_raw)` 且必须为有限值。实际通过预算分配器的 `grant<=request`；q/r 依据 grant 而非 request 计算，即将上式 `request` 替换为获批 `grant`。既有提交为：

```text
mh -= q+r;  bh += q+r
bl += r-q;  ml += q-r
```

重相有序/混合总量变化：`ΔVH=Δbh+Δmh=(q+r)-(q+r)=0`。轻相变化：`ΔVL=Δbl+Δml=(r-q)+(q-r)=0`。逐列成立，因此全域和也为零；不触及悬浮相。

由于 `grant<=request<=0.5*M`，且 `q<=0.5*through`、`r<=0.5*pair`，有：

```text
0 <= q+r <= 0.5*through + 0.5*pair = 0.5*(through+pair)
```

此外 `q<=through<=bl`、`r<=pair<=ml`，并由 `r<=mh-q` 得 `q+r<=mh`，故不会从任一 donor 扣成负库存。即使 `f_sl` 推导出的 `k_raw` 很大，`request` 的闭式上限仍把列总交换限制在现行 `0.5*(through+pair)`；通过预算器后 `grant` 只会更小。该 cap 是数值/守恒防过冲上界，不声称它标定了合理物理幅值。非有限输入、负压力、无效粒径或方向不兼容均 fail-closed，不进入 q/r。

## 4. 关闭/开启 A/B 验证设计

比较三臂，除 rate provider 外固定种子、初始材料场、子步 dt、倾角/摇晃输入、writer 次序、预算和设备。OFF 臂关闭渗滤 grant，但保留同一预算账本和其它通道的预注册额度/优先级；不得把未用的渗滤额度临时转给其它 writer，否则该 A/B 不能归因于渗滤律。

| 实验臂 | percolation rate | 用途 |
|---|---|---|
| OFF | `k_seg=0` | 静态关断基线；验证没有活动时分选不发生 |
| BASELINE | 常数 `6/s` | 现行行为参照 |
| FSL | 上述逐列 `k_seg,c` | 检验剪切响应和压力/粒径代理的行为差异 |

**静置关断实验：** 构造至少三类初始场：均匀分层、带静态弯曲但无时间变化的表面、完成摇晃后进入静置的平台。倾角固定且传感器静止；待所有非渗滤 writer 的前步活动率降至零后，记录 `gammaDot/k_seg/request/grant` 与 `VH/VL`。FSL 必须 `gammaDot=0 → k_seg=0 → q=r=grant=0`，各相逐列守恒误差在既有 epsilon 内；BASELINE 仍按 6/s 演化，OFF 不演化。另设倾角恒定但静态 `chi>0` 的对照，确认不会因静态曲率被误判为剪切。

**剪切响应实验：** 固定相同混合初态，施加无剪切静止段、低活动摇晃段、高活动摇晃段、停止摇晃恢复段；记录每子步 `gammaDot`、`k_seg`、request/grant、单位时间 heavy-down 量和相守恒。FSL 应在静止段归零，在活动段随 `gammaDot` 增长而响应；压力项与粒径项通过分层场景比较其抑制/增强方向。BASELINE 对相同库存/深度/摇晃的 rate 与 activity 无关，故静止段仍可持续分选，活动强弱只通过其他通道间接改变库存。

**硬断言（所有实验臂）：** 每子步 `ΔVH=ΔVL=0`；非负与容量不变量通过；每列 `q+r<=0.5*(through+pair)`；通道 request/grant 计入原预算且 grant 不超预算；dt 减半后累计分选量收敛差满足现有 P-008 数值容差。A/B 只接受“相同场景、相同预算”配对结果，不按视觉主观差异替代账本。

### 预期差异

| 条件 | 现行常数 `6/s` | 候选 `f_sl` 闭合 | 判别观察 |
|---|---|---|---|
| 静置且历史活动为零 | 仍可能分选，直到库存/半量 cap 限制 | `gammaDot=k_seg=0`，停止分选 | 静置窗口逐列 q/r 是否归零 |
| 同深度、不同剪切活动 | rate 不随活动变化 | rate 随 activity proxy 变化，最终受库存、半量 cap 与预算裁剪 | 高活动段单位时间 grant 是否高于低活动段；裁剪饱和时只要求请求响应正确，不能要求 grant 单调 |
| 同活动、浅/深层 | 深度不显式影响 rate | Janssen 压强抑制随深度增加并趋于饱和 | 深度曲线先增后趋平台；浅层线性极限正确 |
| 同活动、不同粒径比/大粒体积分数 | 不区分粒径组成 | `F_R` 与 `dBar` 改变 rate | 与文献映射方向一致且不超半量上界 |
| 剪切停止后的恢复 | 仍可继续分选 | 前步活动代理衰减到零后关断 | 停止延迟不超过一个已定义的 activity-history 更新窗口 |

不得把“FSL 比 6/s 更快/更慢”写成先验通过条件。量化差异须由有效参数与 A/B 账本给出；预算饱和时 request 行为和 grant 行为分开报告。

## 5. 前置缺口与处置

| 缺口 | 决策 | 完成标准 |
|---|---|---|
| 重沙是否等于大颗粒（以及偏析方向） | **实现前必须完成** | 由材料定义/显微或供应数据确认 heavy/light 与 small/large 的映射、`signHeavyDown`；映射不兼容则本路线停止，不取绝对值掩盖方向。 |
| `phi_L` 体积分数换算 | **实现前必须完成** | 确认每相质量/体积字段语义及固体密度；给出从模型库存到大颗粒体积分数的可复算转换。无这些信息不得直接用 `bh/(bh+bl)` 充当 `phi_L`。 |
| Janssen `ell_J` 测量 | **实现前必须完成**（若实现压力项） | 测得槽体横截面/湿周、壁摩擦 `mu_wall`、侧压比 `K_J` 和长度标尺；至少报告历史/含水状态变化。不能测得时只允许保留 `UNIDENTIFIED_BOUNDARY` 作为研究支线，不可上线任意代理 `ell_J`。 |
| `d_S,d_L`、bulk/有效密度、孔隙率/饱和状态 | **实现前必须完成** | 明确 SI 标定及有效压力闭合，避免重复浮力扣减；粒径比范围需落在论文适用域 `1<R<=5`。 |
| 剪切代理 `gammaDot` 与活动历史 | 可先作为**仅供离线原型验证的代理值**；上线前必须完成 A/B、静置关断及 dt 收敛验收 | 明确 `nonPercolationDeltaSLastStep` 的 writer 聚合、缺失样本和停止后归零规则；不得宣称代理等于实测物理剪切率。 |
| 系数 `B/C/E` 及论文变量映射 | **实现前必须完成** | 对照论文 252 原文/适用范围确认符号、系数与 `phi_L/R` 定义；现有笔记候选值在确认前不能作为生产标定值。 |

因此，当前可供实现评审的是事务接口、守恒证明和验证矩阵；实际替换生产 6/s 的实现前提尚未满足。完成阶段一闭环后，需携带上述缺口证据、A/B 数据和预算/守恒断言结果进入实现评审。阶段状态继续为 **`DESIGN_ONLY / NOT_CALIBRATED`**。
