# P-013 P1 材质耦合 CPU fixture 与五量用例登记 v1

日期：2026-10-25。责任：tester。任务：[6adcc00b…0dda](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adcc00be4b0e6ce82a70dda)。状态：**TEST_REGISTRATION_ONLY / NOT_IMPLEMENTED / NOT_CALIBRATED / NOT_VALIDATED / G1_NOT_UNFROZEN**。

本稿登记 P1 材质耦合的 CPU fixture 可执行用例、精度边界、五量用例与 RED 基线结论，并核对 G1 解冻前置。**不实现任何产品代码、不修改任何冻结口径、不新冻数值容差、不宣布协议或 G1 生效**（宣布权在 manager）。全部算术为合成复算，非真机测量；真机取证（色块、GL 读回、三层合成）属后续 effector 窗口。

## 0. 依据与身份

| 项 | 值 |
|---|---|
| 算法签收稿 | `docs/design-P013-p1-material-recipe-slope-confirmation-v1.md`（v1，manager 2026-10-24 21:55 验收） |
| 工程稿 | `docs/architecture-P013-p1-material-composite-v1.md` rev2（manager 22:30 验收） |
| 数学参考 | `docs/reference-P013-p1-material-recipe-v1.json`（`P013_P1_RECIPE_ANALYTIC_V1`，SHA-256 `d214aa447f9f186eba159fd6953f2b42874420926a1b01b7c0977b940f1d63a6`） |
| 上游效果目标 | `docs/design-P013-E2-effect-target-acceptance-v1.md` |
| 运行时版本 | `materialRecipeVersion=P1_ALBEDO_DELTA_V1` / `slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2` / `zOriginVersion=1` / `colorPipelineVersion=LIN_SRGB_REINHARD_V1` / `textureSamplingVersion=P1_TEXEL_CANVAS_PX_V1` / `strataSamplingVersion=CONTINUOUS_COLUMN_WARP_V1` / `T_dataLayoutVersion=2` |
| 五量方法身份 | `methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1`，主测 `Ycode=0.2126·R8+0.7152·G8+0.0722·B8`（8bit sRGB 显示码） |
| 执行证据 | `test/reports/p013_cpu_reference_execution.json`（执行器 `test/p013_cpu_reference_execution.py`；58/58 项 MATCH，合成复算非测量；RED 取证双树内容级扫描：正式归档 919 文本 + 并行工作区 854 文本，P1 身份 token 0 命中） |
| RED 基线 | `docs/p013-p1-material-coupling-red-baseline-v1.md` |
| 本稿状态 | 用例登记 + 口径引用；不含任何 estimand 变更、数值容差或 PASS 结论 |

fixture 期望值一律以参考 JSON 为唯一数值来源，本稿只做登记与追溯，不改签收稿任何公式、版本号或数值。参考 JSON 中 pb 为 Float64 设计算术、pb32 为上传值（`Math.fround`），`normal` 明确标注由哪种 pb 算出。

## 1. CPU fixture 用例登记（六个 fixture + 解析对拍 + 镜像）

共同前提（designer 确认稿 §3.1）：N=128 列；boxW=128 Canvas px、boxH=capacity=256，故 dx=sy=1；`S=bh+bl+mh+ml`；waterTop=256 只进深度不进坡度；非倒置；同源无光照材质参数。**"只给 bh 不够确定法线"**——各 fixture 固定其它层，若 bh 变而 bl 反变使 S 不变，pb/n 必须不变（F6 层重排即此反例）。期望输出判定方向均为"与参考 JSON 逐值一致"；容差层面见 §2。

### 1.1 TC-P013-FIX-01 常数 fixture

- **输入**：`fixtures.flat`——bh=20、bl=20、mh=ml=12 → S=64 全列恒定（`rawBh[128]×20`，参考 JSON L212–350）。
- **期望输出**：所有列 pb=0、n=(0,0,1)；无额外左右光照分区（`expectedPb=0`、`expectedNormal=[0,0,1]`）。
- **判定方向**：精确零。执行器断言 pb64 全列 `==0.0`、pb32 全列 `==0.0`（不接受浮点尾差；显示四遍 + Gaussian 对常数是恒等，任何非零都是实现缺陷）。
- **失败定位**：若 pb≠0 → 查 (a) 显示四遍是否误用已平滑数组原地污染（designer §2.1 明令 scratch 再 set）；(b) Gaussian 权重未一次归一化（37 项 sum 必须=1）；(c) 端点 clamp 写成环绕/镜像。

### 1.2 TC-P013-FIX-02 线性坡面（内域）

- **输入**：`fixtures.ramp`——bh[c]=20+0.2c、bl=20、mh=ml=12 → S=64+0.2c（`rawBh` L483–611）。
- **期望输出**：c=23…104 内域 pb64 数学值 **0.2**；pb32=**0.20000000298023224**；n(pb32)≈**(-0.19611613794814378, 0, 0.9805806751289282)**（参考 JSON L662–676；designer 稿记作 −0.196116137948/0.980580675129，同值）。
- **判定方向**：内域 pb64 与 0.2 的相对偏差 <1e-12（复算实测最大偏离 2.5590640717609858e-14，来源为累加 Float64 舍入）；pb32 为 Float32 逐位同一（`f32(0.2)`）；normal 由 pb32 归一化复算一致。
- **失败定位**：内域 pb≠0.2 → 查 (a) 四遍核是否用错（必须是 [1,1,1]/3 复合，等价 [1,4,10,16,19,16,10,4,1]/81，而非 [1,4,6,4,1]/16 之类）；(b) sy/dx 换算（boxH/capacity ÷ boxW/128）错用屏幕 px；(c) Gaussian 深度方向误加 σz。

### 1.3 TC-P013-FIX-03 同坡面边缘

- **输入**：同 F2 输入，完整四遍 + Gaussian + 端点差分（`fixtures.ramp.selectedExpected`，参考 JSON L616–736）。
- **期望输出**：c0/127 pb64≈**0.047385794656442215**、pb32≈0.047385793179273605；c1/126≈**0.10134662147899576**；c22/105≈**0.19999908624895824**。normal 从 pb32：c0 为 (-0.04733268225716399, 0, 0.9988791804769694)。
- **判定方向**：逐列与参考 JSON 一致（Float64 1e-12 / pb32 逐位）；**端点不得被硬改成 0.2**——端点衰减是冻结规则的直接结果（差分端点仍除 2dx，复制端列而不删 tap 重归一化）。
- **失败定位**：端点=0.2 → 实现把端点特殊处理成内域值或做了单边 dx；端点=0 → 卷积端点补零；c1 偏差大 → 核半径/权重错。

### 1.4 TC-P013-FIX-04 raw 脉冲

- **输入**：`fixtures.rawImpulse`——bh[c]=40+10·1[c=64]、bl=20、mh=ml=20 → S=100+10δ64（`rawBh` L870–998）。
- **期望输出**：pb64[63]≈**+0.01618533041693837**、[64]=0（数学值；复算数值零 -1.4210854715202004e-14）、[65]≈**−0.01618533041693837**；pb32[63]=0.016185330227017403；normal 左右相反、中心竖直（参考 JSON L1003–1113，另含 c41/42/43/62/66/85/86/87 衰减肩部值）。
- **判定方向**：63/65 严格反对称（|pb[63]|=|pb[65]|）、中心为数值零（|pb[64]|<1e-12）；肩部点（c62=0.031151152748513766 等）逐值一致。
- **失败定位**：63/65 不对称 → 差分方向或核 j 递增顺序错；量值偏大 → 漏做显示四遍（应见 F4 与 F3 差 10.7%）；偏小 → Gaussian 半径用错（18 列而非 6）。

### 1.5 TC-P013-FIX-05 Gaussian 单元隔离

- **输入**：`fixtures.postDisplayImpulse`——**已显示** S_D=100+10δ64（`displayS` L1118–1246，`stage` 标记 `GAUSSIAN_UNIT_INPUT_ONLY_DO_NOT_APPLY_DISPLAY_FILTER_AGAIN`），只做 Gaussian，不再四遍。
- **期望输出**：pb64[63]=**5(w0−w2)≈0.01800228755409705**、[64]=0（精确 0）、[65]=−0.01800228755409705；w0=0.06662513187754393、w2=0.06302467436672465（`gaussian.summary`）。
- **判定方向**：与 5(w0−w2) 精确一致（1e-12）；与 F3（0.01618533）**必须不同**——二者之差正是"漏做/重复做 DISPLAY"的判别信号（实测差 11.2%）。
- **失败定位**：F4 值=F3 值 → 对已显示数据又做了四遍（重复 DISPLAY）；F4 值≠5(w0−w2) → 核归一化或端点复制错；[64]≠0 → 核不对称。

### 1.6 TC-P013-FIX-06 内部层重排

- **输入**：bh=40+2δ64、bl=20−2δ64、mh=ml=20 → S 恒 100（designer 稿 §3.1 表第 6 行；参考 JSON 无独立节，输入由本登记固定）。
- **期望输出**：pb 全部 0、n=(0,0,1)；**不得由 bh 峰产生宽面法线**。
- **判定方向**：全列精确零（同 F1）。材质 mask 可随 bh/bl 变，pb/n 必须不变。
- **失败定位**：出现非零 pb → 坡度输入用了 bh 或 M_D 而非 S_D（"只给 bh 不够确定法线"反例被触发）；这是把重沙内部起伏当沙面光照的典型缺陷。

### 1.7 TC-P013-FIX-07 脉冲独立解析对拍

- **方法**：内域组合核 `v = w ⊗ displayKernel`（w=37 项 Gaussian，displayKernel=[1,4,10,16,19,16,10,4,1]/81），常数部分消去，`pb[64+d]=5·(v[d+1]−v[d−1])`（dx=sy=1）。该式**不复用 F3 的管线代码**，从核的数学定义独立推导。
- **期望输出**：pb[63]=+0.01618533041694…、[64]=0、[65]=−0.01618533041694…，与 F3 管线值、参考 JSON 值三方一致（1e-12 内）。
- **判定方向**：解析值、管线复算值、JSON 发布值三方两两一致（实测：解析 0.016185330416942055 / 管线 0.01618533041693837 / 发布 0.01618533041693837，最大相对差 2.3e-13）。
- **失败定位**：仅管线与 JSON 一致而解析不一致 → 两份共用实现同一错误（本对拍的存在意义：比比较两份共用实现更能暴露错误）；三方一致 → 核形、归一化、复合顺序同时被验证。

### 1.8 TC-P013-FIX-08 镜像坡面（附加反例）

- **输入**：F2 输入整体取负（S=−64−0.2c）。
- **期望输出**：pb 精确为 F2 的相反数、nz 不变（designer §3.1：正/负坡镜像时 pb 反号、nz 不变）。
- **判定方向**：pb64 逐列 `a+b==0.0` 精确成立；nz 差 <1e-15。
- **失败定位**：反号不精确 → 端点 clamp 在负值域行为不一致；nz 变 → 归一化前对 pb 取了绝对值或做了 clamp。

### 1.9 TC-P013-FIX-09 常数层（Q 表 / C3 块值 / palette 与 M 基线）

- **输入**：参考 JSON `colors` 节——`qTableSelected` 12 点、`c3` 两块、`grainBaseBytes` 三层、palette 低端字节。
- **期望输出**（复算与发布值一致）：EOTF/Q 按 `c≤0.04045→c/12.92，否则((c+0.055)/1.055)^2.4`；`L=min(EOTF(c),1−1/4096)`；`Q=L/(1−L)`——byte 255 → 保护值 L=0.999755859375、Q=4095；byte 254 → Q=111.38603216880189；byte 248 → Q=15.309416630892823；C1 中性 Q(128/255)=0.2752832884266815；C3 两块 shadeIdx [48,16] → shade (0.8062539403120994, 0.6244082498155039) → 字节 [206,159] → Q (1.6123749830222691, 0.5306998454952045) → δC3=(**0.13370916945955877**, **0.02554165570685231**)（aC3=0.10）；palette 低端 H=(145,168,111)/L=(216,196,150)；M 零噪声字节 (181,182,131)（raw 混合 (180.5,182,130.5)，`floor(v+0.5)` 量化，不得先 round raw 再生成整图）。
- **判定方向**：逐点一致（Float64 1e-12；字节逐位相同）；C1 字节 128 增量严格 0；δC3 两档为相对 128 基线的正增量，不去均值、不反号。
- **失败定位**：Q(255)≠4095 → 255 保护缺失（会把亮粒 B 推向上界跳变 3983.614）；M 字节≠181 → 先 round 了 raw 混合值；δC3 反号 → 基线取错（用了线性 0.5 或 127.5）。

### 1.10 不涉及面（本节用例一律不触碰）

- `ellLight`/`Lface`/`Lmat`/ToneMap/OETF 的数值——属 G3/G5 真机与 shader 链，CPU fixture 只到 pb/pb32/normal 与 Q/C3 常数层；
- B/C 资产字节导出与 texel 取样——属 G1/G2 资产交付与 TC-P013-AST 条目；
- 真机色块、GL 读回、三层合成——effector 队列（G0/G5）；
- 参考 JSON 的 rawS/weights/selectedExpected 本身——只读引用，任何更正须由 designer 另版并重发参考。

## 2. 精度边界登记（三类误差分层）

**CPU 参考消费实际上传的 Float32 包。** CPU 参考与 shader 必须同坐标、同 mask、同 fH/warp、同上传字节流（pbUpload=Float32(pb64)，shader 只对上传 pb 做连续列插值）。**禁止用 CPU Float64 对 GPU Float32 假称逐位等价**；发布小数不是"±某误差"PASS 线。三类误差分开由 tester 分层测量：

| 层 | 误差来源 | 本卡可测范围 | 归属 |
|---|---|---|---|
| L1 Float32 量化 | pb64→pbUpload 的 fround；Q 常数查表 Float32 化 | 合成层可测：pb64→pb32 最大 gap 实测 7.140720015286206e-09（ULP 量级）；normal 从 pb32 与从 pb64 的差已在参考 JSON 双列（如 interior nz 0.9805806756909197 vs 0.9805806751289282）；核自洽事实：37 项 sum=1.0000000000000002、对称、峰在 j=0、截断方差 35.240624301084985 列² | tester 合成复算（本执行器） |
| L2 shader 算术 | GPU 侧 EOTF/Q/pow、卷积不存在（pb 已 CPU 差分）、插值 | 需 GL 读回对照，**未测** | G3（developer 实现 + tester 对拍） |
| L3 读回/截图 | glReadPixels 编码、PNG/JPEG、系统 compositor/色彩 profile | 需真机，**未测** | G0 probe / G5（effector） |

**容差一律不由本卡冻结**（红线）。L1 的数字是合成事实描述，不是 PASS 线；L2/L3 的误差带由 tester 在 G0/G3/G5 出重复性分布 → designer 复核 → manager 冻结。未冻结前，任何涉及 L2/L3 的结论标 `MEASURED_NOT_CALIBRATED`，不记 VALIDATED。

## 3. 五量用例登记（R_B / dark / Q_couple / C_band / ΔY_face）

方法身份 `methodId=P013_P1_MATERIAL_COUPLING_YCODE_V1`（rev2 §5.4）。全部对照固定 `physicalSnapshotHash/FrameToken/seed/assetGeneration/palette/appearance/light/geometry/原生尺寸/色彩profile`；保存 P1 OFF/ON × B开关 × C开关诊断格；所有臂同一支持集，不可看过 ON 再选 ROI；迎/背光由几何 `n·ℓ` 与固定光向指定，不从图像亮度排序；静态四场景至少 3 次独立渲染/读回；原生 PNG 主测，JPEG 单列敏感性；CPU 中间输出、GL 读回、最终三层截图分开保存。**数值容差未冻结 → 一律 `MEASURED_NOT_CALIBRATED`，不记 VALIDATED。**

| 用例 ID | 量 | 定义（引用 rev2 §5.4 / designer §7，不重定义） | 状态分流 | 今日 RED |
|---|---|---|---|---|
| TC-P013-MCP-01 | `R_B(Ω)` 含 `R_B,dark` | `A_B^t=sqrt(mean_Ω(H_B(I_B^t)^2))` 码值 RMS；`b^t=median_Ω Ycode(F,t,B关,C固定)`；`N_B^t=A_B^t/b^t`；`R_B=N_B^ON/N_B^OFF`；亮/暗 ROI 分开报；`H_B=I−G_B*I`，σB=active grainFeaturePx×Canvas→native 等比尺度，R=ceil(3σB)，ROI 内缩支持半径 | `EMPTY_ROI`（ROI 空）/`UNDEFINED_RATIO`（b^t=0，不加 EPS、不默认 1）/`INSUFFICIENT_SUPPORT`（支持半径不足） | **NOT_IMPLEMENTED**（无 P1 OFF/ON 渲染产物；`[0.8,1.2]` 仅为旧扫描域，不作 PASS 线） |
| TC-P013-MCP-02 | dark 绝对幅度族 | 固定 Ωdark 报 `darkBOn/Off=A_B^ON/OFF` 及 CPU 值；C 用去趋势层纹 RMS 报 `darkCOn/Off`；另保留 C1 prominence/周期/row-mean | 同上；绝对幅度不设"越低越好" | **NOT_IMPLEMENTED**（缺 P1 渲染与读回链） |
| TC-P013-MCP-03 | `Q_couple,X`（X=B/C） | 匹配 Ωlit/Ωdark：`Q_X^t=A_X^t(Ωlit)/A_X^t(Ωdark)`；报 `D_X=Q_X^ON/Q_X^OFF`、各 A 绝对值及同输入 CPU 预期；B 用高通 RMS、C 用去趋势层纹 RMS，不混频带 | `UNDEFINED_RATIO`（分母 0）；不规定 Q>1，不凭单比值推结构 | **NOT_IMPLEMENTED**（CPU 预测需先有 P1 shader 配方实现） |
| TC-P013-MCP-04 | `C_band` 连续性向量 | B/C OFF 光照隔离臂：固定 x、同材质内部扫 z，报 `Y_ON−Y_OFF`、CPU 曲线、相邻 native px 梯度/残差；x 横扫在物理列边界 ±1px 与列中央匹配支持算 `g_edge=P95(abs(ΔxY))`、`g_mid` 及比值；真 H/L 边界单列排除；C3 样式边界仅在全材质臂标记另报、**不得删去** | `INSUFFICIENT_SUPPORT`（同材质段不足）；阶跃事件计数阈值未冻结（tester 先定的阈值也须 manager 冻结） | **NOT_IMPLEMENTED**（隔离臂需 GLES OFF 与完整 C 臂，均无产物） |
| TC-P013-MCP-05 | `ΔY_face` | 匹配材质/z/遮挡的迎背光 ROI：`ΔY_face^t=median(Y_t,Ωlit)−median(Y_t,Ωdark)`；报 `ΔΔY=ON−OFF`、CPU 预期、参考目标距离、ROI 材质构成与像素数 | `EMPTY_ROI`；**8–20/255 仍是旧扫描域，不转 PASS 线** | **NOT_IMPLEMENTED**（同 MCP-01） |

结构条件（可先立，无需数值线）：OFF 严格 `Lmat=1`；ON/OFF 共用资产/seed；同一 Lmat 乘全部材质；neutral 增量 0（字节 128 的 C1 增量严格 0）；flat/mirror 法线关系（FIX-08 已在合成层验证）；无额外深度/层纹模糊；同相同坐标 CPU 公式与 mask。数值线（GPU/CPU/截图误差、ΔE、R_B/dark 幅度、Q_couple 误差、C_band 阶跃阈值、ΔY_face 可读幅度、跨帧波动）**必须等实测**，tester 在 G0/G3/G5 出分布，designer 审效果，manager 冻结。

## 4. G1 解冻前置核对与剩余缺口

rev2 §9 实施切片 2 写明：G1（资产/CPU 参考）解冻前置 = **① 本 rev2 经 manager 验收 + ② tester 登记 CPU fixture/材质耦合用例及 RED 基线**；且 G0 真机 probe（sRGB surface/profile、三层 z序/透明/输入透传）仍须先过，不得用 rev2 文档完成代替 G0 实测。

| 前置 | 状态 | 证据 |
|---|---|---|
| ① P1 工程稿 rev2 经 manager 验收 | **已满足** | 任务卡载明 manager 2026-10-24 22:30 验收 |
| ②a CPU fixture 用例登记（六个 + 解析对拍） | **已满足** | 本稿 §1；期望值 58/58 复算 MATCH 且逐值追溯到参考 JSON（执行证据 JSON） |
| ②b 材质耦合五量用例登记 | **已满足** | 本稿 §3 + RED 基线稿（NOT_IMPLEMENTED 如实登记，不伪造） |
| ③ G0 surface/sRGB/三层 z序/透明/输入透传 probe（真机） | **未满足** | 无真机产物；当前真机队列首位为掉沙 V1-V4 返修复测（effector 串行） |
| ④ 数值容差/效果/色彩冻结 | **未满足** | `MEASURED_NOT_CALIBRATED`；tester 分布未出（依赖 G1 后实现） |
| ⑤ P1 实现授权 | **未授权** | manager 未放行 G1 实施 |

**结论：rev2 已验收 + tester 登记（②a/②b）这一半前置已满足；G1 实施放行仍缺 ③（G0 真机 probe，需 manager 建卡排 effector 队列）与随后的实施授权。本卡不自行宣布 G1 解冻。** 缺口清单交 manager：①G0 probe 建卡（含色块取证与 GL 读回编码实测，rev2 §5.3）；②G1 实施卡（六图共享字节导出、shader 色彩/材质 CPU 参考、Gaussian/pb 与七版本身份——注意 CPU 参考须消费实际上传 Float32 包，见 §2）；③G2–G5 门序与真机队列排期。

## 5. 不涉及项

- 不改 E3 的 J95/J90/std 锚点与 E/A/B CLOSED 状态；GLES 侧重采分子分母（阈值本身不改）属 G5；
- 不新建数值容差线：8–20/255、[0.8,1.2] 仅为旧扫描域；
- 不改 designer 确认稿已签收口径（fixture 期望值、公式、版本号只登记）；
- 不改预注册 estimand；不改实现代码；不伪造 RED 数据；不宣称 VALIDATED；不自行宣布 G1 解冻；
- 不使用真机（本卡纯 Node/Python 离线通道）。

## 6. 变更记录

- 2026-10-25，v1：首次发布。登记九个 CPU fixture（六个 designer fixture + 解析对拍 + 镜像反例 + 常数层；期望值全部追溯到参考 JSON，58/58 复算 MATCH）、三类误差精度边界、五量用例与状态分流、G1 前置核对结论（② 半满足，缺 G0 真机 probe 与实施授权）。纯文书 + tester 只读执行器；未动冻结口径、未用真机、未宣布 G1 解冻。
