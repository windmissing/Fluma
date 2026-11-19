# T606 v1 · 目标 13 纯流场独立基准与可信性用例（T605 交付消费）

日期：2026-11-19，tester，不使用真机，无 CPU 流体 stepper。
依据：`algorithm-G13-pure-fluid-gpu-v1.md`（T605 设计，READY_FOR_T607_T606）+
`design-G13-pure-fluid-fixtures-v1.json`（16 fixtures，independentOwner=T606/T609
tester，CPUPhysicsStepperAllowed=false）。
证据：`test/t606/`（independent-baseline.json + closed-form-checks.json）。

## 交付 1：独立基准目录（16 case，全部独立推导）

`independent-baseline.json` 逐 case 给出：独立可证物理量、闭式/有界公式来源
（自推导，非 designer checker import）、该 case 能独立证明什么（压力/速度/黏度/
对流/能量/事务/资源）、不能证明什么（分域如实）：

| case | 独立证明的量 | 不能证明 |
|---|---|---|
| F01 静水 | 压力重建（phi 均值规范）+重力即势（无速度喷涂） | 黏性/对流 |
| F02 制造投影 | 投影算子+散自由残差（MAC 面自己求和） | 全 NS 时进 |
| F03 周期剪切衰减 | **黏性项系数精确性**（u=U0 sin(ky)e^{-νk²t}，率 k²ν=π²/16） | 压力/非线性对流 |
| F04 Taylor-Green | **对流+压力+黏性联合**（全 NS）+动能衰减恒等式 | 3D/壁边界（周期域） |
| F05 闭盒制造+旋转 | 旋转势拆分+Coriolis 静止归零 | 自由表面 |
| F06 Couette | 壁 no-slip+黏性时间尺度（起动级数 N=40 尾界 3.1e-107<1e-6 有界） | x 向压力 |
| F07 Poiseuille | 压力梯度→流量转换（Q=G H³/12ρν）+黏度 | 旋转/对流 |
| F08 ramp spin | 运动剖面一致性（Omega/alpha/theta 自导数/自积分互核） | 液体惯性响应 |
| F09 corotating | Coriolis/离心势拆分正确性（闭式格点） | — |
| F10 均匀周期流 | 对流输运（tracer x+=U0·t 精确）+周期边界一致性 | 压力求解（均匀平凡） |
| F11 负控 | 门覆盖/无静默修复（五故障注入→FAULT+无 CURRENT 提交） | 物理 |
| F12 能量预算 | 能量收支闭合（dE/dt=−2ν∫|S|²+边界功率 自恒等式） | 直接证压力场 |
| F13 事务 | replay/cancel/late/failure 状态机语义 | 物理 |
| F14 配置门 | 参数域边界逐项独立复推（ν* 2^-20..1/8 等）越界须 FAULT | 物理 |
| F15 渲染/冷证据 | accepted-only 渲染+页 SHA 链格式 | 求解数学 |
| F16 成本/资源 | 资源界（config 128U32+状态尺寸）与 GPU pass 计数表 | 数值精度 |

## 交付 2：解析/有界期望自检（closed-form-checks.json）

- F03 样点 0.925791（公式自实现）；F04 压力样点 1.4e-18、衰减率 4νk²=2.4674
- F06 起动级数 N=40 尾界 **3.097e-107**（远 <1e-6，级数项数足够）
- F07 流量 Q=5.333（单位自洽 ρ*=1）
- sin/cos/exp 采样误差按 T605 门（绝对 ≤2^-21 / 相对 ≤2^-20）独立计入门限，
  制造解误差门**不含未量化的 GPU 构造误差**（合同条款如实执行）

## 测量格式与负控

- 每 case 期望输出=独立闭式值/有界界；测量=GPU 快照抽样点位对照+残差范数
  （RMS/Max 分列，空间收敛 order 最小 0.8 按 F04 给定）
- 负控五项（F11）各自带独立违例探测器（法向壁面非零/gauge 错/符号错/混叠重复
  更新/和超界）
- **阈值未定项如实**：T605 侧门限字段未冻结的部分记录为"推导+度量就绪、阈值
  待产品适配阶段冻结"，不当本卡缺陷或产品 PASS

## 未实现域

产品 G13 路线 NOT_IMPLEMENTED（T605 明示）——GPU 实测与 T607 工程实现属后续
阶段（T609 独立 GPU 定量验证），本卡不含实施。
