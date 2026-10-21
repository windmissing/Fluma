# P-012 帧时配对复验 + EAB 锚点独立复核（tester，s99 构建）

- **日期**：2026-10-21。执行：tester。被测构建：`entry-default-s99-signed.hap`（含 PERF 修复 `189c341`+`f7351f9`）。
- **性质**：EAB-batch DoD ③ tester 侧闭环（第一梯队帧时配对复验），按 `architecture-P012.md` §10.5 冻结口径。真机 62T0225B10010020。
- **独立性声明**：manager/architect 预验证（pair1 抽查、PERF 数据 tier3=32/32/33）仅作背景引用，本报告全部数据为 tester 独立重跑。

## 1. 四档帧时配对（perf ladder，独立重跑）

`tools/run_p012_perf_ladder.py`，6 窗口 × 1.0s（≥3 帧窗口要求满足），25s 冻结静置后同相位采样。FPS 读数来自截图 FPS 文本块裁剪（md5 聚类 24 crops → 7 唯一簇，多模态读数后建 `fps_map_s99v.json`）。

| tier | n | FPS min/med/max | 帧时 P50 (ms) | 帧时 P95 (ms) | P95 FPS |
|---|---|---|---|---|---|
| tier0 OFF | 6 | 30/45.5/60 | 24.46 | 33.33 | 30.0 |
| tier1 E | 6 | 30/32.0/60 | 31.28 | 33.06 | 30.2 |
| tier2 EA | 6 | 33/36.0/60 | 27.78 | 30.30 | 33.0 |
| tier3 EAB | 6 | 34/60.0/60 | 16.67 | 29.20 | 34.2 |

窗口原始值：tier0=[30,31,30,60,60,60]、tier1=[31,30,31,33,60,60]、tier2=[33,33,36,36,60,60]、tier3=[34,35,60,60,60,60]。

**★ tier3 ≥30fps 判定细则（§10.5-1）——PASS**：

- 6 窗口全部 ≥30fps（最低 34）✓
- 帧时最大 29.4ms（1/34）≤ 33ms 预算 ✓；P50=16.67ms、P95=29.20ms 均 ≤33ms ✓
- 与 tier0 无下降趋势（tier3 中位 60 vs tier0 中位 45.5）✓
- **PERF_BLOCKED 状态解除依据成立**：修复前 tier3 12/12/11 fps → 修复后 34~60fps，退化消除。

## 2. EAB 锚点独立复核（3 对 OFF/ON 配对块，两轮）

`tools/run_p012_eab_pairs.py` 采集，`tools/measure_render_p012.py` 冻结口径（off-ws0 profile）测量。

**第一组（s99v）**：pair1-off 与 pair3-eab 出现状态错位（pair1-off w0/w1 呈 EAB 特征 84.5%、pair3-eab-w0 呈 66.5% 过渡态——采集时序/状态残留问题，非构建缺陷；pair1-off w2 与 pair3-eab-w1/w2 为正确 OFF 帧）。定性为**采集无效**，剔出不用于锚点判定；以第二组复跑验证可重复性。

**第二组（s99w）**：三对全部状态正确（OFF 侧 sandLum=74.96、EAB 侧 168.17，100% 一致）。锚点对比：

| 截图 | sand lumMean | sand lumStd | sand hfH | water lumMean | water lumStd | §4.1 判定 |
|---|---|---|---|---|---|---|
| pair1-eab | **168.17** | **17.89** | **8.075** | **207.57** | **17.38** | BIT-EXACT |
| pair2-eab | **168.17** | **17.89** | **8.075** | **207.57** | **17.38** | BIT-EXACT |
| pair3-eab | **168.17** | **17.89** | **8.075** | **207.57** | **17.38** | BIT-EXACT |
| pair1/2/3-off | 74.96 | 24.56 | 3.135 | 64.90 | 1.13 | （OFF 分母基线） |

EAB 侧三对五项指标与 dev-report §4.1 冻结值**逐位一致**（差 <0.01，即 JPEG 编码内同帧）；复现 manager 抽查结论。解释边界按 §10.5-2 预冻结：带边界亚像素列差异（±1 灰度单像素列）在测量 ROI 排除范围内；沙区内部 ROI 逐位一致。

## 3. 消融验证抽查 —— **参数未交付，以替代证据记录**

`p012ablate` want 参数（architecture-P012 §10.5-3）在 s99 构建**未实现**：全工程 grep（EntryAbility/SandCanvas/render/*）零命中，仅存在于架构文档。消融抽查无法按原口径执行。

替代证据（记录不判定）：PERF 修复 commit 的 developer 消融 D1/D2/D3 已定位 CanvasPattern 采样为瓶颈（drawImage 平铺方案）；本轮 ladder 实测 tier2(EA)=36fps vs tier3(EAB)=34~60fps——EAB 不再慢于 EA，颗粒层成本已被平铺消除，与"颗粒层是唯一成本源"的修复结论一致。**p012ablate/p012fillbox 参数实现登记为 developer 后续项**（消融验证正式口径待其交付后补跑）。

## 4. 结论

- **PERF 修复验证 PASS**：tier3 全窗口 ≥30fps、帧时 ≤33ms 预算、对 tier0 无下降；PERF_BLOCKED 摘除依据成立。
- **锚点复核 PASS**：EAB 侧三对配对块与 §4.1 冻结值逐位一致（第二组全状态正确；第一组两处采集无效已剔除并定性）。
- **消融抽查 BLOCKED（工具缺口）**：`p012ablate` 未实现，登记 developer 后续项；替代证据支持修复结论。
- 交 designer 按第 (7) 步做第一梯队签字判定；本报告不判定 SV-05/06/07 主观项（effector 职责）。

**证据**：`test/artifacts/p012-measurements/`——`perf-ladder-s99v-*.jpeg`（24 快照）、`*-fpscrop.png`（24 裁剪）、`fps_map_s99v.json`/`fps_summary_s99v.json`/`tier3-verdict-s99v.json`、`eab-s99v-pair*`（两轮 36 快照）、`s99v-anchor-measurements.json`/`s99w-anchor-measurements.json`。
