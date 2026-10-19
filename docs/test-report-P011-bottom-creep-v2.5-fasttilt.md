# P-011 客观基线测试报告：bottom-creep-v2.5-fasttilt

> 日期：2026-10-14  
> 问题：P-011 倾斜跟手性 / 底层运动响应  
> 技术路线：`bottom-creep-v2.5-fasttilt`  
> 实现摘要：底层 `bh/bl` 蠕变 + `maxTiltOmegaDegPerSec=720°/s` + 传感器平滑 `0.4/0.6`  
> 结论：**BLOCKED / NOT PASSED**

## 1. 执行环境

| 项 | 值 |
|---|---|
| 工程路径 | `D:\hmprojects\HelloWorld` |
| 真机设备 | `62T0225B10010020` |
| 安装包 | `.ws-bottom-creep-fast-signed.hap` |
| 客观用例 | `docs/test-cases.md` P-011 |
| 主观报告 | `docs/effect-report-P011-bottom-creep-v2.5-fasttilt.md` |

## 2. 可用客观检查

| 检查 | 命令 / 依据 | 结果 | 说明 |
|---|---|---:|---|
| 速率与 dormant 解析 | `python test/water_sand_check.py --self-check-rate` | **PASS** | `rate`、dormant skip、legacy inference 均通过 |
| 缺陷哨兵 | `python test/water_sand_check.py --self-check-defect` | **PASS** | 合成守恒缺陷按预期 FAIL，说明哨兵有效 |
| 三相守恒哨兵 | 同上 | **PASS** | 能识别合成缺陷；不代表 P-011 全量通过 |

## 3. P-011 用例结果

| 用例 | 结果 | 原因 |
|---|---|---|
| TC-P011-01 输入倾角到有效倾角响应时延 | **BLOCKED(missing_hook)** | 缺 `commandTime`、`acceptedTime`、首个消费输入的 `physicsTime` 与逐样本 `effectiveTilt` |
| TC-P011-02 输入变化与底层 `bh/bl` 位移相关性 | **BLOCKED(missing_hook)** | 缺逐物理子步 `bh/bl` 数组与输入活动量时间序列 |
| TC-P011-03 快速倾斜、回正、反向连续性 | **BLOCKED(missing_hook)** | 缺输入事件与逐子步底层位移数据，无法计算 `maxSubstepBh/Bl`、`zeroResidual`、`reverseDirectionError` |
| TC-P011-04 三相守恒与数值稳定性压力测试 | **BLOCKED(missing_hook)** | 缺长时逐采样三相总量与状态快照钩子；现有哨兵只能证明检查器有效，不能替代本用例 |

## 4. 阻塞项

1. **输入时间戳钩子缺失**
   - 需要：`commandTime`、`acceptedTime`、首个消费输入的 `physicsTime`
   - 影响：TC-P011-01、TC-P011-03

2. **有效倾角逐样本钩子缺失**
   - 需要：物理子步级 `effectiveTilt`
   - 影响：TC-P011-01、TC-P011-02、TC-P011-03

3. **底层逐列状态钩子缺失**
   - 需要：物理子步级 `bh[]`、`bl[]`
   - 影响：TC-P011-02、TC-P011-03

4. **三相总量逐采样钩子缺失**
   - 需要：`Vw/Vh/Vl` 或等价 `waterTop/bh/bl/mh/ml/suspH/suspL` 聚合
   - 影响：TC-P011-04

5. **P-011 自动化脚本缺失**
   - 需要：独立 `test/p011_check.py` 或等价脚本
   - 影响：无法自动计算 T50/T90、Pearson、lag、FE、单子步跳变与漂移

## 5. 结论与下一步

- 当前技术路线无法完成 P-011 客观验收，正式结论为 **BLOCKED(missing_hook)**。
- 现有速率与守恒哨兵通过，说明基础检查器未回归。
- 需要补齐 P-011 专用输入、有效倾角、底层状态与三相总量钩子，再实现自动化判定脚本。
- 在钩子补齐前，不得将本报告解释为客观通过或失败，只能作为 **RED/BLOCKED 基线**。

