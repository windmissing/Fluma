# T631 v1 · 目标 13 独立基准公式参数与覆盖返修（T628 裁定消费）

日期：2026-11-19，tester，不使用真机，无 CPU 物理 stepper。
依据：`architecture-T628-v1.md` 逐 case 裁定；修 `test/t606/` 独立资产，**不重开
T606**（历史报告/原件保留，superseded 字段在资产内标注）。
证据：`test/t606/independent-baseline.json`（v2）、`closed-form-checks-v2.json`、
`coverage-table.json`。

## 返修明细（七项裁定全部落实）

| case | 修正内容 | 自检值 |
|---|---|---|
| **F06** | **数学错误修复**：起动级数补 `(-1)^n` 因子；正式 N=128、t≥1/120；尾界改为 `U·exp(-aN²)/(πaN²)`（a=νπ²t/H²） | N=128 尾界 **2.71e-12**<1e-6 ✓（旧 N40/3.1e-107 系未知参数产物，弃用） |
| **F07** | 流量修正为 **1/6**（f_x=1/32, ν=1/64, ρ*=1, H=1：Q=f_xH³/12ν）；补半格偏置 f_xh²/(8ν)、速度 max 偏置 +2^-12、流量/剪切 1/32 | Q=0.1666…**精确=1/6** ✓（旧 5.333 系 G=1 错误驱动） |
| **F03** | 正式参数 ν*=1/16、A=1/4、T=1/4、J∈{1/2,1,4}（旧 ν=1/64 率 π²/16 不属本 fixture）；BE 离散幅值、ν 倍增检查、时间阶 0.8/空间阶 1.8 | 率 νk²=2.4674 |
| **F05** | 补非零闭盒制造解 u=a·sin²πx·sin2πy、v=−a·sin2πx·sin²πy、π=P·e^(−2t)cosπxcosπy + REST/RAMP_SPIN 两组；外力由自推符号导数构造 | 样点 u=0.125、π=0.0487（覆盖缺口已补） |
| **F02** | 补物理压力分量：离散压力 (k/d_h)P·cosπxcosπy（P=1/4）；RMS≤min(2^-16,…)、max≤2^-12、阶≥1.8；禁止解析压力直写待测解 | P=0.25 绑定 |
| **F11** | 五负控映射产品 hook1–5；**alias duplicate=hook6 工程缺口 NOT_OBSERVABLE**（test-only 构建）；PRESSURE_DISABLED 不冒第五项 | — |
| **F12** | 离散逐算子账/实际 midpoint 功率/正功不退款/120 quanta 冻结窗替代连续恒等式；ρ*=1 耗散 2ν∫S:S（不得多乘 ν/μ） | — |

F01/F04/F08–F10/F13–F16：每行加 binding 注记（测量时绑定 fixture 输入 SHA/网格/
J/时刻/变量位置/压力规范/RMS-max 分母/收敛比较对；T605 已冻结门原样引用，不再
泛称"阈值未定"）。

## 纪律

- 全部公式自推自实现，无 designer checker import，无 CPU stepper
- T606 v1 报告与 closed-form-checks.json 原件保留为历史；错值（F06 漏 (-1)^n、
  F07 5.333、F03 参数错位）在资产 superseded 字段如实标注
- 产品 G13 当前 CONFIG_PENDING（T609 v1）不阻塞独立解析返修；本卡不冒 GPU PASS
