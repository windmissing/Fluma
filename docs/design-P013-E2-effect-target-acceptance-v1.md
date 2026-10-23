# P-013 E2：GLES 顶光立体感效果目标与验收设计 v1

日期：2026-10-23。责任：P-012/P-013 designer。状态：**DESIGN_ONLY / EFFECT_ANCHOR_CANDIDATE / NOT_CALIBRATED / NOT_IMPLEMENTED**。本稿是 P-013 立项书的效果侧输入，不授权实施，不修改任何冻结锚点或评分量表。依据：[E2 开题 §2](algorithm-P012-E2-top-light-opening-v1.md)、[路线③架构评估 §2/§4](architecture-P012-E2-route3-xcomponent-egl-assessment-v1.md)、[用户效果锚定](p012-water-cloud-sand-demand-anchor-v1.md)、[P-013 立项草案](proposal-P013-incorporation-v1.md)及 Canvas 路线②原型 `2b7de23`。manager 10:34 已裁定路线②效果上限不足、路线③应尽早推进；正式 P0 开工仍按 P-013 立项签发。

## 1. GLES 栈下的效果目标和分阶段可交付性

目标仍是**高亮、低饱和的同一材质空间**：下面沙丘迎光面/背光面形成宽缓、连续的体积明暗；重轻沙色差、B 细颗粒、C 层理在暗面仍可辨，且真正受同一光场调制；上方水/界面保持近白、柔和斑驳的云雾，不被沙面法线染出硬黑白块。GLES 提供逐显示像素求值能力，**不会自动增加 128 列物理几何细节，也不能凭 1D 高度场推断真实三维遮挡或自阴影**。效果应来自更正确的材质合成与连续采样，不靠扩大物理列数或夸大光比。

沿用 E2 已验收的候选数学语义：显示坐标 x 上，先对 `S=sandTop` 做宽面低通 `p_b=∂x(G_σ*S)`；`n=normalize(−p_b,0,1)`；`Lface=clamp(La+Ld·max(0,n·ℓ),Lmin,Lmax)`；沙像素距**本列沙面**的材料深度为 `z=S(x)−height(y)`；`Lmat=1+exp(−z/ellLight)·(Lface−Lref)`。同一像素先合成材料反照率 `Rmat=Rbase+δB+δC`，再执行预先登记的色彩空间转换与 `ToneMap(Lmat·Rmat)`。`Lmat` 只在沙 mask 内；水/雾层 P2 另用低幅柔光，不借用沙法线。光照 OFF 时 `Lmat=1`，不改底材、纹理 seed 或物理快照。

| 路线②候选 | 新栈语义与待核对之处 |
|---|---|
| `σ=6` | **沿用候选宽面尺度，不沿用未核对的单位标签。** 当前 `e2EnsureLightMap` 实际把 `round(6)` 当作 128 列数组中的盒滤波半径（并非已经证明的“6 材料深度单位高斯 σ”）；GLES T_data 的 `p_b` 必须先冻结列距/显示 px/材料单位换算及核形，CPU 参考与 shader 同源。P0 若只对 raw `sandTop` 差分，不等价于此宽面坡度。 |
| `ellLight=24` | 候选**材料深度单位**的衰减尺度；每 fragment 的 z 必须以当前位置的 `S(x)` 为起点，不以容器 `capacity` 为起点。路线②按三材质带中点取一次 shade 档，且现代码 `zMid=capacity−bandCenter` 含 `capacity−S` 偏移；它是原型观感/性能对照，不能当成逐像素 `Lmat` 的数值金标准。 |
| `Lmin=0.80/Lmax=1.12` | 候选 `Lface` 截断界，**不是**沙像素输出亮度硬界；经深度衰减、反照率、ToneMap 后的显示 Y 另验。`La/Ld/Lref/ℓ` 保持开题公式关系，不能把上限 `1.12` 当全画面曝光增益。 |

路线②是 `128列×3材质带` 的带中点采样、8档 `shadeLut` 游程绘制，B/C 后续 Canvas 叠层难以按同一逐像素 `Lmat` 入光；预期 GLES P1 后可获得**同一带内随 z 连续渐变**、跨列宽面明暗更平顺、B/C 暗面纹理与基色同步变暗/变亮。P0 只验证沙区光场与叠层可见性，**不能把 P0 的“整张 Canvas 已合成画面再乘亮度”宣布为已解决贴纸问题**；B/C 真正统一到 `ToneMap(Lmat·Rmat)` 是 P1 的效果里程碑。P2 加水/雾独立受光层次后，才验证用户“上白云雾、下沙颗粒”同画面终态。P3 退役 Canvas 是工程里程碑，不自动增加 E2 评分。

## 2. 数据契约与必须交 architect 澄清的两处效果风险

架构 §2 的 T_data **逻辑字段**为 `sandTop,bh,mixStart(=bh+bl),waterTop,suspH,suspL,p_b` 七个标量列场，均仅由只读物理快照/CPU 坡度预处理派生；`T_tex` 承载现有 B 三张颗粒与 C 三张层理位图，`T_param` 承载光照候选与六参数 appearance。各纹理的打包行号、格式、色彩编码、上传失效依据以 architect 最终契约为准：评估 §2.1 将坡度称为 `row7`，而前列 `row0…row5` 后缺 `row6`；P0 草案又临时只打包四个物理标量并在 shader 内差分。**不能把这三种布局写作同一“七行”已实现契约。**E2 效果比较至少要登记 `T_dataLayoutVersion/slopeKernelVersion/zOriginVersion`；若 P0 用 raw 差分，则标 `E2_APPROX_P0`，不得参与 σ=6 精确等价判定。

**跨 Surface 乘法不是 GL 自动提供的能力。**P-013 草案 §2.3 的 `GL_DST_COLOR` 只读取当前 GLES framebuffer 的 destination，不能据此认定它能读到下层 ArkUI Canvas 的像素；GL/系统合成的目标不相同。[OpenGL ES 规范](https://registry.khronos.org/OpenGL/specs/es/3.2/es_spec_3.2.pdf)把目标颜色限定在当前 framebuffer attachment。且一般情况下 `ToneMap(L·R)≠L·ToneMap(R)`；“Canvas 已 tone-map 的颜色再乘 Lmat”不能由乘法结合律证明与目标式等价。交 architect 的 P0 前置证据：对比同一静态沙像素的 Canvas 底色、XComponent 上叠输出、最终屏幕读回，分别检查深浅两端、沙 mask 边界和非沙区；若无法访问 Canvas 作为同一 framebuffer 纹理，则 P0 应改为**完整沙区底材+纹理在 GLES 一次合成，或明确仅做独立可见光照诊断 pass**。在该问题解决前，`P0_EFFECT_COMPOSITE_UNPROVEN`，不以“有亮暗差”验收 M1。

P0/P1 还需冻结颜色管理：输入 B/C 位图的编码、sRGB→线性光的解码位置、`Lmat` 作用空间、ToneMap 曲线、输出 framebuffer 色彩空间以及 premultiplied alpha 约定。颜色空间或叠层方式变化会改变绝对 Y；跨栈效果按**同法指标等价**评价，不追求逐位相同，也不容许拿色彩漂移冒充光照收益。

## 3. 验收指标候选：面级塑形、连续性、材质耦合

测量身份为 `{physicalSnapshotHash, renderSeed, appearanceConfigRevision, lightConfig, rendererStack, rendererCommit, shaderHash, T_dataLayoutVersion, device/DPI, colorSpace, maskRevision, captureFormat}`。固定沙丘形状与同材质/相近材料深度的左右坡面 ROI；另取平沙反例和水/雾 ROI。先测参考、Canvas 基线、Canvas toplight 原型，再测 GLES OFF/ON；显示 Y 用固定 RGB 亮度定义，线性光只用于校验 shader/CPU 参考运算，不把两种 Y 混报。必要时固定光向镜像，检查迎背光符号交换。全部阈值走参考和重复性测量→designer 修订→manager 冻结，不从 GLES ON 倒推过线值。

| 候选量 | 可执行提取与预期 | 数值状态 |
|---|---|
| `ΔY_face` | 相同材质、相近 z、遮挡匹配的迎光 ROI 中位 Y 减背光 ROI 中位 Y；同快照 Canvas/GLES 的符号、方向及目标帧距离均可比较，需分别登记 renderer 栈、OFF 基线与色彩空间。平沙应接近零**新增**面级差，不要求底材本身零差。 | E2 原开题 **8–20/255** 仅为扫描域，非 PASS；GLES 下重新测参考/误差带后冻结。既有公式与指标定义沿用，不拿逐位差当跨栈验收。 |
| 带内明暗连续性 `C_band` | 选一段有连续坡面且跨越 H/L/M 各带的固定 x，沿每一材料带 z 扫描显示 Y；在 B/C OFF 诊断臂求面级低频剖面，检查 `Lmat(z)` 预期单调衰减、带边是否有非物理跳变。另以 `|ΔY|` 在物理列边界 ±1 显示 px 与列中央配对，报 P95 和边界/中央比；仅以材质边界之外的区域计“伪台阶”。 | 候选：相对 Canvas 原型的带内阶跃事件下降、列锁定额外梯度不升；**具体降幅/容差待定标**。不能靠全局模糊求平顺，因为 B/C 纹理保留与旧 E3 门槛同时约束。 |
| 暗面 B 颗粒保留 `R_B,dark` | 在固定暗面纯沙 ROI、同 seed/同输入，计算 `I_B=Y(B开,C固定)−Y(B关,C固定)` 的高通 RMS；用该 ROI 底材中位亮度归一，再取 `E2 ON/E2 OFF` 比。亮面同法另报；报告有效像素、颗粒频带、JPEG/PNG 方式。 | 探索防丢失区间 **[0.8,1.2]**，仅供扫描；因 ToneMap 非线性与亮度归一误差，正式线须由参考+CPU 光照参考确定，不当作冻结锚点。 |
| B/C 真耦合 `Q_couple` | 对 B 与 C 分别做纹理开关差分 `I_B/I_C`，在匹配材质与 z 的迎/背光 ROI 求增量 RMS 之比；与同一 shader 配方的 `ToneMap(Lmat·Rmat)` CPU 参考预测比值比较，同时报绝对暗面纹理幅度。若 B/C 是后叠贴纸，纹理增量与光场脱钩；若暗面被压没，绝对幅度会暴露。 | 候选“方向一致且相对 CPU 参考误差受控”；误差界待屏幕读回和色彩管理定标。`C` 另保留层纹 prominence/row-mean 原指标，不以 B 高频替代。P1 才能正式测完整耦合。 |
| 水/沙材质边界 | P2 同帧量水/雾亮度、局部 std、雾带连续性及沙颗粒/层理；上层保持近白斑驳，下层沙体仍可辨。 | EAB/E3/B/C 原冻结门槛及 Q1 独立状态不变。P0/P1 保留 Canvas 水/雾时只做“非沙区不得被 E2 调制”回归。 |

`C_band` 的纵向扫描要排除 bh/mixStart 真材质切换造成的合理色阶差，另取同材质内部；`Q_couple` 的纹理差分必须使用**同一物理快照、同 seed、同色调参数**，否则差分不可解释。对 `ΔY_face` 同时报每 ROI 的材质成分和有效像素数，不能把重沙/轻沙本身的基色差算作迎背光差。性能报 CPU、GPU/render+RS、上传/重建、呈现帧间隔 P50/P95；效果候选通过不自动证明 90Hz 单拍。

## 4. 黄金图场景与跨栈证据矩阵

黄金图采用**设备原生无损截图/帧缓冲读回为主，JPEG 仅作另列敏感性**，冻结显示尺寸、曝光/系统色彩设置、mask、截图时点和 `T_data`/纹理版本。每场景至少三次独立渲染以记录读回波动；动态帧以物理 `frameId/snapshotHash` 对齐，不按近似墙钟拼图。跨栈给全帧 SSIM、逐像素差的 P50/P95、分材质 ROI 分布和边界图；**这些是回归诊断候选，不设置事后宽松阈值**。正式结论以同方法已冻结指标和主观效果为准，SSIM 不能因几何大面积相同就掩盖小面积暗面贴纸。

| 场景 | 固定输入与必须暴露的问题 | 分阶段比较 |
|---|---|---|
| 静置 | 同一厚沙丘有明确左右坡面，外加平沙反例；有 H/L/M 层与稳定 B/C 种子 | P0 检沙 mask、符号与绝对色彩；P1 检带内连续及暗面纹理；P2 检水雾近白。 |
| 倾斜 | +/−方向成对、缓慢转向及回正，同一输入脚本的确定帧；同时存宽面坡度与 Lmat 热图 | 检明暗随法线反向、跨列无闪烁、纹理不脱离材质；不以瞬间换向峰帧代替稳定帧。 |
| 倒立 | `invert180` 前后同物理库存映射、重建后的首帧与稳定帧 | 检坐标、mask、top-light 方向约定、GPU 纹理重传/seed 连续性；不把画布翻转导致的左右交换误判为光向错误。 |
| 重载 | 多材料、悬浮/雾、B/C 全开、长窗动态；含参数切换和 surface 前后台恢复的单独子段 | 检性能与帧身份、E3/B/C 回归、资源重建后黄金图一致；90Hz 是否可达仍归 P-013/F1 性能里程碑。 |

每类至少存 `Canvas E2 OFF`、`Canvas toplight 原型`、`GLES E2 OFF`、`GLES E2 ON` 的同输入臂，P0/P1/P2 标注实际负责的沙/纹理/水层，避免把双栈重叠像素当单栈输出。s133 的 J95/J90/std 比等已冻结数值只在其**原 renderer 构建与默认参数**适用；新增 `rendererStack` 与 shader/layout/colorSpace 身份后，GLES 重新采 OFF/ON 分母分子并核对原阈值适用性，**不是修改原阈值，也不能把 s133 PASS 直接继承为 GLES PASS**。跨栈 bit-exact 不作为验收线。

## 5. Canvas toplight 与 GLES 主线、六参数面板

双栈 P0–P2 时，`p012ablate=toplight` 是 Canvas 路线②的**诊断对照开关**；Canvas toplight OFF 是既有画面基线，ON 是三带中点/8 档光照上限对照。GLES OFF/ON 必须有独立显式身份，不能沿用一个布尔值让“Canvas ON + GLES ON”暗中双重照明。产品主线的 E2 效果签字看 GLES P1 后一体化材质 pass；P0 仅可报告坡面明暗与合成可行性。若 GLES 的 `ΔY_face` 与 Canvas 原型接近，但 `C_band/Q_couple` 更好，这是符合换栈效果收益的有效结果；若只是整体变亮、B/C 仍后叠，不能以同一 `ΔY_face` 宣称 M1 达成。

现有 `P012AppearanceParams` 六字段已由 §13 配置层实现；迁入 GLES 时仍以单一 `applyAppearance` 的校验/原子应用/恢复推荐路径经 `T_param` 或资源重传进入 shader，不建立独立面板状态：

| 已有参数 | GLES 效果与更新归属 |
|---|---|
| `grainFeaturePx/grainSigma` | 改 B 粒径与亮度细节；按 §13 重烘/重传 `T_tex(B)`，再同 `Lmat` 耦合；不能因暗面测量而擅改默认 2/20。 |
| `strataLambdaMaterial/strataStrengthScale` | 改 C 层纹周期/强度；同有效 λ 去重规则重烘/重传 `T_tex(C)`，沙面光照不改变物理 bh 或层理“年代”。 |
| `fogDensityScale/clearMottleContrast` | P0/P1 仍由 Canvas 水雾路径处理；P2 才迁入水/雾 shader 的 `T_param` 与相应缓存失效。fog alpha cap 0.78 及水体近白风格约束照旧。 |

E2 还没有六字段中的用户可调旋钮。**新增候选而非已批准 schema**：`topLightDirection`（屏幕内左右微偏与顶向）、`topLightStrength`（控制 `Ld`，同时保高亮低饱和基调）、`faceDepth`（映射 `ellLight`）；可调化若获批应走版本化 appearance schema revision、合法域拒绝而非静默 clamp、`configRevision` 与黄金图/锚点适用域联动。`σ`、`La/Lref/Lmin/Lmax` 先作为经定标的渲染配置保留，不因 shader 逐像素可算就立刻暴露为新滑杆。`textureLightCoupling` 不能做成可关闭的产品参数：B/C 同光场是 E2 验收条件；调节雾受光强度须待 P2 水云雾对照后另提候选。所有外观参数保持物理零触碰与 routeId 隔离。

## 6. 验收顺序与交接

1. **P0 之前的架构澄清**：核准跨 Surface 合成是否真实可实现、ToneMap 次序、`σ` 单位及 T_data 第七字段/打包版本；若仍用近似 P0，证据标明 `E2_APPROX_P0`，不冒称 §2 精确 Lmat。
2. **P0 最小可见**：确认 surface 生命周期、沙区/非沙区 mask、坡向符号、平沙反例及黄金图采集链。只报告“光场可见/合成可行”，纹理耦合 M1 保持 PENDING。
3. **P1 效果主判**：B/C 入同一 shader 材质 pass 后，按 §3 的 `ΔY_face/C_band/R_B,dark/Q_couple` 和四场景黄金图出数；tester/effector 用参考和误差带冻结候选，沿既有 SV-05/06/07 量表复核，不升分或改线。
4. **P2 上下同画面**：水雾 shader 化后按用户锚定复核“上白云雾、下沙颗粒”，E3 三核心锚点及其它已冻结适用项在 GLES 重出基线；未测、不可比或参考 mask EMPTY 的项如实登记。
5. **性能单独签字**：P-013 的呈现帧间隔与物理重载归因另行验收。E2 立体感通过不推断 90Hz 单拍；性能达标也不替代 E2 效果证据。

本设计交 architect 的关键结论：**shader 化的效果价值落在 P1 的材质先合成、再逐像素受光；P0 的跨 Surface 乘法与未平滑坡度不能作为该价值已经实现的证据。**数值候选待定标、冻结门槛不变、实施须另获 P-013 立项授权。
