# P-012 E3 锚点冻结 revision v1（s133 设备侧范围）

日期：2026-10-23。责任：designer。状态：**s133 设备侧核心锚点 `PASS / VALIDATED`；`e3-grid-display-v1` 全量同方法参考锚点 `MEASUREMENT_PENDING`**。本 revision 解除 [旧阻断记录](e3-grid-display-v1-freeze-blocker.md) 中由 s132 `stdRatio=0.684` 引起的冻结阻断；旧记录保留为历史，不改写其结论。这里的 `VALIDATED` 只涵盖下表已冻结、已出数的同构设备侧指标，不代表 E3 全部交付、参考带或所有档位通过。

依据：[A/B 归因回执](e3-std-ablation-receipt-P012-v1.md)（A：64 宽缓冲上屏分段未等比缩放；B：dirty 欠更新排除；修复入 `9d11849`）、[s133 重出数](e3-renumber-report-P012-v1.md)、[冻结 mask revision](mask-freeze-revision-P012-e3q1-v1.md)。构建 `entry-default-s133-signed.hap@9d11849`，HAP SHA-256 `b0c67920c9e2f0b5b54f86f66f5f3de5e4900ca76c7783bd3e2e4d9df124d6f3`；证据目录 `test/artifacts/p012-e3-renumber-20261022/`。`masks-e3q1-frozen-v1`（源 JSON SHA-256 `a114b23ea0bf4e2841fa094f59a6edcb3a719de6058464d8b9d907545d875c14`）冻结参考帧的雾带/清水语义；设备侧本批使用下述矩形窗，**不能把该矩形声称为参考多边形 mask 的逐像素应用**。

## 1. 冻结的设备侧测量档

`methodId=e3-grid-device-native-s133-v1`。同构 OFF=`e3i2`、ON=E3+seg 修复；同构建、`p012seed=20261016`、thick 初态、静置截图配对。设备原生 JPEG 直接读取，不做 200/100 分析像素重采样或图像锐化；亮度 `Y=0.299R+0.587G+0.114B`。s133 几何雾带窗为 **x∈[200,1080)、rows∈[950,1100)**，即 880×150 设备像素、132,000 个亮度样本和 131,850 个水平相邻像素对（完整矩形内；边界按数组切片右/下不含）。每次换构建/几何必须重新登记雾带窗，不沿用 s132 行号。`analysis-e3-renumber-v1.json` 使用失效旧窗 1370–1650，已作废；本 revision 只引用 `analysis-renumber-fogROI-v1.json` 与 s133 报告。

| 锚点 | 冻结边界 | s133 OFF → ON / 两对配对 | 判定 |
|---|---|---|---|
| 设备侧 `J95_ON/J95_OFF` | `≤0.70`，OFF 必须 `>0` | `5.00→2.00`，`0.400`；pair1/2 一致 | **PASS / FROZEN** |
| 设备侧 `J90_ON/J90_OFF` | `≤0.50`，OFF 必须 `>0` | `3.00→1.00`，`0.333`；pair1/2 一致 | **PASS / FROZEN** |
| 防抹平 `pxStd_ON/pxStd_OFF` | `[0.90,1.10]`，同一雾带窗、同输入配对 | `13.52→12.59`，`0.931`；pair1/2 一致 | **PASS / FROZEN** |

分位数定义沿 [E3 候选稿 §2.2](algorithm-P012-E3-grid-removal-v1.md) 的排序位置 `(n−1)p` 线性插值。以上数值只对本 `methodId`、当前设备显示尺度/几何和 OFF/ON 配对成立；不得以跨几何绝对 J 值替代比例。冻结 `J95≤0.70`、`J90≤0.50` 是相对本批 OFF 的改善界，不是从参考帧反推的自然纹理上界。s132 的 `0.684` 与 s133 的 `0.931` 不能作为同 ROI 配对计算，只用于修复前后方向追踪。

## 2. 已报告但尚不可冒称原设计正式算子的项目

| 项 | s133 报告值与本次候选边界 | 冻结裁定与缺口 |
|---|---|---|
| 块边陡变代理 `K_ratio` | OFF `0.092`、ON `0.009`；候选 ON `≤0.02`，当前值在界内 | **PROXY_PASS / FORMAL_K90_PENDING**。`0.009` 是按雾边界每 8 设备像素采样的陡变占比，不是原设计按等值线弧长归一的 `K90`；正式 K90 的阈值与 PASS 均不冻结。当前归档 Python 脚本仍写旧窗，需先归档与 `0.009` 一致的 s133 可重算算子。 |
| 周期印花 `P64/P256` | s133 行均值 `lag64` OFF `−0.527`、ON `−0.443`；报告描述“无孤立正周期峰” | **DESCRIPTIVE / FORMAL_P64_PENDING**。负 `lag64` 不能单独证明无印花；归档仅六个行均值滞后点，且 ON `lag128=+0.533` 仍须按全谱解释。未执行原设计的像素域二维去趋势相关、有效对数和逻辑像素到显示像素换算，故不冻结正式 P64 PASS。参考帧清水 mask 全 EMPTY，该 cell 仍为 `N/A`，不制造参考带。 |
| `SV-06≥3` | 既有 E/A/B 正式评分 `3`；s133 effector 记录雾幅度恢复和块状细碎化 | **历史合格维持 / s133 正式评分待回执**。不升分、不把描述性观察伪装成新评分；正式 E3 主观映射需同尺度静置/慢倾/回正/稳定配对。 |
| `Lgrid`、`Nseg` | 本批无同方法数值 | **DIAGNOSTIC_PENDING**；不填零、不设通过线。 |

原 `e3-grid-display-v1-proposed` 的主档 200 分析像素、100 敏感性档、f0/f5/f2/f6 冻结参考多边形、静/动态分带，以及正式 `K90/P64/Lgrid/Nseg` 定义保持不变。需要 tester 按这些算子重算参考和设备、报有效支持量；f2 手/花残留的污染方向按 mask revision C-1 登记。若尺度或极端组合使结论翻转，记 `COMPARABILITY_PENDING`，走方法 revision。**设备侧代理 PASS 不等于原 `e3-grid-display-v1` 全量锚点 PASS。**

## 3. 释放结论与下游边界

**原冻结阻断中“雾被抹平”这一项已解除：A 归因证毕、seg 等比修复入库、s133 `stdRatio=0.931` 回界；设备侧 J95/J90 改善亦已验证。**这构成可审计的 E3 s133 默认外观基线，可供参数可调化实现绑定 `rendererVersion/sourceCommit/seed/默认六参数` 并验证“推荐值恢复链”；其开发解锁不依赖把尚无数值的参考指标写成 PASS。参数改变后的锚点适用域需另行验证，不能从默认 s133 外推。第一梯队 E/A/B 的 VALIDATED 和既有 SV-06=3 均不回退；E3 全量冻结及最终交付须待 §2 缺口补齐并由 manager 审核。

本次仅冻结上述三个已定义且可追溯的设备侧数值界；不修改物理场、checker、既有冻结附表或评分量表。
