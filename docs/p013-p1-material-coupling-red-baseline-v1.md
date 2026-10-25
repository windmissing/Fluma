# P-013 P1 材质耦合五量 RED 基线 v1

日期：2026-10-25。责任：tester。任务：[6adcc00b…0dda](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adcc00be4b0e6ce82a70dda)。状态：**RED_BASELINE / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED / NO_CAPTURE_AUTHORIZATION**。

本稿记录 P1 材质耦合五量（`R_B(Ω)`/dark 绝对幅度/`Q_couple`/`C_band`/`ΔY_face`）与 CPU fixture 之外的全部待测量在**今天的真实状态**：P1 实现未授权、未开工，因此不存在任何 P1 OFF/ON 渲染、GL 读回或三层合成截图。**本稿不提供、也不接受任何五量数值结论；不伪造运行结果；不追认任何旧栈结论为 P1 证据。** 与 CPU fixture 的合成复算（已完成，见登记稿 §1）严格区分：合成复算验证参考数据自洽，不代表任何 GPU/屏幕测量。

## 1. 基线与依据

- 协议/口径依据：`docs/architecture-P013-p1-material-composite-v1.md` rev2 §5.4（五量测量契约）与 §9（验证矩阵「材质耦合」组）；`docs/design-P013-p1-material-recipe-slope-confirmation-v1.md` §7（五量可测定义与采样方法）。
- 用例登记：`docs/p013-p1-cpu-fixture-and-coupling-test-registration-v1.md` §3（TC-P013-MCP-01..05，含状态分流定义）；CPU fixture 用例为 §1 的 TC-P013-FIX-01..09（属合成层，与本稿的五量 RED 严格分层）。
- 执行证据：`test/reports/p013_cpu_reference_execution.json`（仅 CPU fixture 合成复算 58/58 MATCH；不含任何五量测量）。
- 方法身份（引用冻结，不重定义）：`methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1`；主测 `Ycode=0.2126·R8+0.7152·G8+0.0722·B8`，单位 8bit sRGB 显示码；CPU 先沿唯一色彩链输出 sRGB 再用相同 Ycode；`Ylinear` 只作色彩链诊断，不混入主测分母。

## 2. 归档取证（为什么是 NOT_IMPLEMENTED）

对两棵树做内容级取证（执行器 `test/p013_cpu_reference_execution.py::run_red_scan`，结果记入执行证据 JSON `redBaselineScan`）：

- **正式归档** `test/artifacts/`：扫描 3463 个文件，其中文本 919 个逐内容扫描、二进制 2544 个按格式跳过（均有伴随文本记录）。
- **并行会话工作区** `.codex/local-test/`（未提交、非归档）：扫描 914 个文件，其中文本 854 个逐内容扫描。
- 对每个文本归档检索 P1 冻结身份 token：`P1_ALBEDO_DELTA_V1`、`E2_GAUSS_COL6_R18_P1_V2`、`P013_P1_MATERIAL_COUPLING_YCODE_V1`、`LIN_SRGB_REINHARD_V1`、`materialRecipeVersion`——**两棵树合计命中 0 处**。
- 正式归档中文件名含 `material/gles/p013` 的 10 个文件全部位于 `test/artifacts/p013-measurements/`，经内容核对为 **P0 骨架时代**遥测（s137 RSS 曲线、hilog、骨架构建截图），不是 P1 材质包、GLES 合成帧或五量测量记录。
- 并行工作区中文件名含 `p013` 的 33 个文件经内容核对为 **G2 范畴的桥接/生命周期本地实验**（`p013_bridge` host 测试、lifecycle 八故障注入、host/gpu result 日志）——未提交、无六图资产字节、无 GLES 材质 pass、无 OFF/ON 诊断格，**不构成五量估计对象，也不算归档证据**；本基线如实登记其存在与范畴，不将其计为 P1 产物、也不用其冒充任何测量。
- 结论：`NO_P1_MATERIAL_ARTIFACT`——不存在任何可被五量定义消费的 P1 OFF/ON × B × C 诊断格。

因此五量的 RED 不是"测了不达标"，而是**估计对象尚不存在**：没有 P1 渲染产物 → 没有 ROI、没有支持集、没有分子分母 → 按 rev2 §5.4 分流规则记 `NOT_IMPLEMENTED`（附缺失项清单），而不是记 0 分、记 BLOCKED 于旧口径、或用 Canvas 旧数据冒名。

## 3. 五量逐项 RED 登记

| 量 | 今日状态 | 缺失项（确切路径/条件） | 明确不做的事 |
|---|---|---|---|
| `R_B(Ω)`（亮/暗分开） | `NOT_IMPLEMENTED` | P1 OFF/ON 渲染产物；同臂 B 开/关诊断格；`physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light/geometry/原生尺寸/profile` 固定链；Ωlit/Ωdark ROI 与边界图；σB 定义所需 `active grainFeaturePx` 与 Canvas→native 等比尺度 | 不用 Canvas EAB/旧 B lumStd/HF 锚点冒充 P1 分母；不把 [0.8,1.2] 当 PASS 线；不加 EPS 造 b^t |
| dark 绝对幅度族（`darkBOn/Off`、`darkCOn/Off`） | `NOT_IMPLEMENTED` | 同上 + 固定 Ωdark + 去趋势层纹 RMS 所需的同材质 (x,z) 支持 | 不设"最小可辨幅度"PASS 线（待重复性/manager）；不因全帧 SSIM 给暗面丢纹理开绿灯 |
| `Q_couple,B` / `Q_couple,C` | `NOT_IMPLEMENTED` | 匹配 Ωlit/Ωdark 的 B 高通 RMS 与 C 去趋势 RMS；同输入 CPU 预测（需 P1 shader 配方实现） | 不凭单比值推结构；不把 B/C 频带混用；不默认 Q=1 |
| `C_band` 连续性向量 | `NOT_IMPLEMENTED` | B/C OFF 光照隔离臂；完整 C 开启臂；固定 x 同材质 z 扫描；列边界 ±1px 与列中央匹配支持；C3 样式边界位置标记 | 不从全材质 `C_band` 报告删去 C3 阶跃位置伪装连续；不预设阶跃事件阈值；不靠模糊 C1 压台阶 |
| `ΔY_face` / `ΔΔY` | `NOT_IMPLEMENTED` | 匹配材质/z/遮挡的迎背光 ROI；CPU 预期；光向镜像对拍场景 | 8–20/255 不升级为 PASS 线；不把 OFF 变暗导致的 ON/OFF 提升冒充光照收益；不把重轻沙基色差算作迎背光差 |

状态分流（rev2 §5.4，届时强制执行）：`EMPTY_ROI` / `INSUFFICIENT_SUPPORT` / `UNDEFINED_RATIO` 与原始量一并输出，**不加 EPS、不默认比值为 1**；近零信号可测性由 tester 基于读回重复性标定，不新设 SNR 线。

## 4. 相邻域状态（防止串供）

- **CPU fixture 合成复算：已完成**（58/58 MATCH）。它验证参考 JSON 的期望值可由其输入独立重现（含脉冲解析对拍三方一致）。这是数学自洽性证据，**不是** P1 实现正确性、不是 GPU 一致性、不是效果证据。
- **Q/C3/palette 常数层：已完成合成复算**（Q 表 12 点、C3 两块、M 零噪声字节 (181,182,131)、255 保护值 4095）。同理属数学层。
- **色彩 probe（色块 CPU/GL/屏幕/Canvas 三端）：未测**，属 G0 真机 probe；本稿无数值。
- **E3 J95/J90/std 在 GLES 身份下的重采：未测**，属 G5；阈值本身不改，旧 PASS 留在原 renderer/build/config 身份，不继承为 P1 结论。
- **旧 `peakSusp`/WS-08/endpoint 协议等 P-011 资产：与本稿无关**，不在本基线范围。

## 5. 测量前必须满足的条件（供 G1/G3/G5 放行核对）

1. P1 实现经 manager 授权（G1 放行）且六图资产/shader CPU 参考交付；
2. G0 probe 通过：sRGB attachment 实际编码、三层 z序/透明/输入透传有实测证据（`COLORSPACE_UNSUPPORTED` 时不得继续效果采集）；
3. 诊断格固定链成立：同 `physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light/geometry/原生尺寸/色彩profile`，OFF/ON × B × C 四臂同支持集；
4. CPU 参考沿 §2 精度边界消费实际上传 Float32 包（L1 量化已合成层描述；L2 shader / L3 读回截图分开测量）；
5. 每场景 ≥3 次独立静态渲染/读回；动态帧按预注册 frameId/snapshotHash，不在 ON 曲线择峰；
6. 输出分层保存：CPU 中间输出、GL 读回、最终三层截图各自独立，不把 compositor 误差算成算法误差。

以上任一不满足，五量继续记 `NOT_IMPLEMENTED`/`MEASURED_NOT_CALIBRATED`，不出数值结论。

## 6. 交接与红线

- 交接链：manager 放行 G1（需先过 G0 真机 probe）→ developer 实现 P1 → tester/effector 按 §5 条件采五量并出重复性分布 → designer 审效果 → manager 冻结数值线（此前一律 `MEASURED_NOT_CALIBRATED`）。
- 本稿红线：不伪造 RED 数据；不宣称 VALIDATED；不自行宣布 G1 解冻；不改冻结锚点（E3 J95/J90/std、E/A/B CLOSED）；不新建数值容差线；不用真机；不修改实现、参考 JSON 或已签收设计稿。

## 7. 变更记录

- 2026-10-25，v1：首次发布。双树内容级取证（正式归档 919 文本 + 并行工作区 854 文本，P1 身份 token 合计 0 命中；正式归档 10 个文件名命中经核对为 P0 骨架遥测，并行工作区 33 个文件名命中经核对为 G2 桥接/生命周期本地实验），五量逐项 `NOT_IMPLEMENTED` 登记与缺失项清单，相邻域状态隔离，测量前置条件六条。纯离线文书；未用真机；未宣布任何生效。
