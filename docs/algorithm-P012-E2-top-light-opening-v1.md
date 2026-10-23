# P-012 E2 顶光立体感开题与技术路线约束 v1

日期：2026-10-23；责任：designer。状态：**DESIGN_ONLY / EFFECT_ANCHOR_CANDIDATE / ROUTE_DECISION_PENDING**。依据：[gap E2](gap.md)、[现有渲染算法](algorithm-P012-render-v1.md)、[F1 归因](fps-attribution-P012-v1.md)、[F1 模型验证](f1-offscreen-model-validation-v1.md)、[E3 性能归因](f1-dyn-ablation-P012-v1.md)及用户 2026-10-22 19:27 补充的 fragment shader 路线约束。本稿只定义效果、测量与候选工程路径；不修改代码、冻结锚点、goals 或帧率指标，不授权换栈。

## 1. 目标、现状与归属

E2 的目标是在同风格高亮、低饱和画面中，让沙丘**迎光面与背光面形成连续可读的整片明暗塑形**，并使上层白色云雾有柔和的受光层次，下面沙体依然像有颗粒和沉积层次的沙。[用户效果锚定](p012-water-cloud-sand-demand-anchor-v1.md)要求上层云雾、下层沙质感同时可辨。顶光不靠画一道高光线、画固定黑白横条、提高整体曝光或覆盖均匀白雾冒充。

当前 `P012WaterSandRenderer.ets::drawSandColumns` 已有 `n=normalize(-∂x sandTop,0,1)`、固定左上光向 `(-0.55,-0.55,0.63)`、`La=0.85/Ld=0.15`、LUT 和逐列游程。它是**已有弱光照基线**，不是 E2 已完成的证据：当前法线只由一个截面表面坡度估计，底色三带按列同档，未验证整片明暗分区，更未证明 B 颗粒/C 层理/上层云雾随同一光场协调变化。`C` 的深度条纹仍归层理，`D` 的运动流束仍归流痕，`E3` 的雾连续性仍独立验收；E2 只负责受光结构和材质-光照耦合。

## 2. 光照场与纹理耦合：算法候选

输入只读 `WaterSandSnapshot` 的 `sandTop,bh,bl,mh,ml,suspH,suspL,waterTop`，沿用 renderer 的 `P012MaterialColumnSampler` 与沙/水/雾 mask；物理 128 列不变。对显示横坐标 x，取连续沙面 `S(x)`，先在显示空间低通得到宽面坡度 `p_b(x)=∂x G_σ*S`（σ 为**光学宽面尺度**，不得改物理表面），再设截面法线 `n(x)=normalize(-p_b(x),0,1)`。固定屏幕顶光方向 `ℓ` 默认有微弱横向分量，使相反朝向坡面可区分；光强

```text
Lface(x) = clamp(La + Ld max(0,n(x)·ℓ), Lmin, Lmax)
Lmat(x,z) = 1 + w(z) [Lface(x) − Lref],  w(z)=exp(−z/ellLight)
z = 显示沙面到该沙像素的材料深度，且仅在沙 mask 内定义
```

`Lref` 取同一画面参考漫反射，防把整片沙无条件提亮；`ellLight` 控制整片坡面向沙体内部衰减的可读深度，不是物理穿透长度。平台沙面 `p_b≈0` 时不制造左右亮暗分区；坡向翻转时面级亮暗响应也应翻转。若 1D 截面坡度不足以唯一确定真实三维法线，应把这一限制作为风格代理登记，不能声称物理光线追踪。

**纹理-光照耦合是一等条件**：同一个 `Lmat` 作用于 E 材质底色、B 颗粒亮暗和 C 层纹的最终组合，形式上先合成材料反照率/纹理 `Rmat=Rbase+δB+δC`，再作 `ToneMap(Lmat×Rmat)`；具体混合顺序由 architect 按现有 Canvas 管线细化。B/C 纹理不得像“贴纸”一样在暗面保持原亮度，也不能被光照乘到消失。材料类型仍控制原色；重轻沙对比、颗粒 HF/std、层理行均值结构在 E2 ON/OFF 同掩膜配对中核查。水云雾走独立的有限光学调制（例如同方向、低幅度的散射/高光项），不复用沙面法线去给整片水制造硬分区；E3 的雾幅度 `stdRatio` 和连续性指标须回归。

上述 `La,Ld,σ,ellLight,Lmin/Lmax` 全为**候选参数**；目前没有足够参考 ROI 或真机数据可冻结数值。第一步用既有光向/光照作为 OFF 基线，测参考帧的面级明暗，再选 ON 候选；不得拿 ON 结果反向定目标带。

## 3. 可测效果与锚点冻结前置

先冻结同一物理快照、seed、显示尺度、容器几何和沙/水 mask，取有左右有效坡面的厚沙丘关键帧；平沙及薄沙场景仅用于“不可虚构分区”的反例。对迎光/背光两侧分别按**相同材料类型、相近深度**配对 ROI，避免把 heavy/light 基色差或 C 条纹差算成 E2。记 `Y` 为线性亮度或预注册的显示亮度通道，冻结 methodId 后不可混用。

| 候选观察量 | 提取口径与预期 | 冻结前证据 |
|---|---|---|
| 面级明暗 `ΔY_face` | 同材质/深度匹配 ROI 的迎光 `median(Y)` 减背光 `median(Y)`；ON 应比 OFF 更接近目标帧方向与幅度。提议先考察 **8–20/255 灰度级** 的可读区间，纯属设计扫描域，不是 PASS 线。 | 参考与 OFF 同尺度成像各自测分布、跨帧误差带及遮挡支持量；若参考不在此域，以参考预注册测量修订候选，不按 ON 结果调线。 |
| 方向正确/连续 | 左右坡面 `sign(ΔY_face)` 与光向一致；同一坡面横向亮度低频剖面无列阶梯/突变；平沙不新增虚假整片分区。 | E2 OFF/ON/光向镜像诊断，E3 `J95/J90/K90/P64` 与同尺度 edge map 一并看。 |
| 纹理保留 | 沙区 B 颗粒 `std/HF`、C 层纹 prominence/row-mean 与原 EAB+C 基线配对；亮面和暗面分别测，不能用整区均值掩盖暗面颗粒消失。 | 既有 B/C 冻结锚点原样回归；新增 ON/OFF 保留率候选先量误差带，不擅加冻结比例。 |
| 上层雾与材质边界 | 水雾明暗随顶光温和改变，但水体仍近白斑驳、沙体仍可辨；E3 s133 设备侧 stdRatio 及后续同方法锚点不退化。 | 同帧水/雾/沙分区截图 + 既有 `SV-05/06/07` 评分项；E3 未完成的参考带仍按其独立状态，不写成已过。 |
| 性能 | E2 ON/OFF 配对 `CPU烘焙/P95`、上传/缓存失效次数、render+RS、真实呈现间隔 P50/P95、30s 动态窗 FPS。 | 同构建、同输入、冷热分层；静置/普通倾斜/重载动态分别报。E2 不改变第一梯队已通过 ≥30fps 口径，也不预签 G2.7/F1。 |

主观上重点请 effector 判断“迎光/背光面是否像同一座有体积的沙丘”“颗粒与层纹是否仍附着在受光面”“上方云雾是否仍柔和有层次”。沿用既有 `SV-05/06/07` 量表和及格线，E2 单项描述与定量归属先走**候选→基线/参考测量→manager 冻结**；本稿不自创正式评分线。

## 4. ArkUI Canvas2D 无项目可用 fragment shader：三条技术路线

项目当前 Canvas2D API 为 `Path2D/clip/gradient/pattern/drawImage/ImageBitmap` 等 2D 绘制接口；本地 SDK `26.0.0/ets/component/canvas.d.ts` 未声明 shader/fragment 入口，**在本项目所用 ArkUI Canvas2D 路径上没有可交付的自定义 fragment shader 入口**。这是一条当前工程能力边界，不代表系统没有 GPU：Canvas2D 仍经渲染服务合成。OpenHarmony 的 [Canvas2D API 文档](https://gitee.com/openharmony/docs/blob/87dfd55a178780683e9b58c0f5742691c773f936/en/application-dev/reference/apis-arkui/arkui-ts/ts-canvasrenderingcontext2d.md?force_mobile=true)列的是 2D 绘图接口；[Native XComponent 文档](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-arkui/capi-oh-nativexcomponent-native-xcomponent.md)明确其 Surface 可用于 EGL/OpenGLES。换栈后能编写 fragment shader，但设备具体 shader 吞吐仍须实测。

以 F1 报告的约 `0.9Mpx` 全幅画布、90Hz 面板为示例量纲。下表的复杂度和内存是**计算/存储下界或候选网格**，不是实测时间或承诺收益：

| 路线 | 光照调制与数据流 | 每次失效/每帧成本及证据强度 | 与 F1 的交点和主要风险 |
|---|---|---|---|
| **① CPU 逐像素调制**（扩展 E3 I2 buffer） | ArkTS 遍历沙 mask 像素，按 sampler 法线/深度求 `Lmat`，对底色/颗粒/层理复合后再上传或分段上屏。 | 全幅最坏约 `0.9M` 像素/帧，90Hz 即约 `81M` 像素/秒；单 RGBA 缓冲约 `3.6MB`，双缓冲约 `7.2MB`，另计上传与混合。E3 I2 曾因每帧重烘、内层函数与 transfer 等叠加出现 **52 倍帧时退化**，修复后 E3 链约 `0–1ms/帧`；这既不证明 E2 全幅安全，也不能把旧退化全部归于“逐像素”。必须真机最小原型和逐段计时。 | F1 目标要求动态轻载尽量进入 90Hz 单拍约 `11.11ms`；CPU 全幅每帧工作可能与物理重载争主线程。除非测得小面积/低频更新足够且纹理仍协调，否则不作主路线。 |
| **② 伪 shader：低频预烘焙光照图 + 多层合成** | 128 列坡度→法线→Lambert；候选 `128×256` 光照图约 `0.125MB`/RGBA 张、双缓冲约 `0.25MB`，在几何/光向/参数失效时更新；运行期按沙 mask 拉伸/连续插值，把同一光场用于 E/B/C 合成。 | 预烘焙 `O(128×256)`/失效，平稳帧仅查表/`drawImage` 与若干合成；实际 draw/RS/上传成本**未测**。E3 I2 的层高、相位、尺寸、配置/epoch 失效矩阵可复用框架，但不得照搬 `LAYER_EPS=0.25` 就宣称不会迟滞。缓存失效频率决定平均与 P95 峰值。 | 与 F1 的分层预合成兼容，预计工程变动较小，推荐先做**审计和原型的基线方案**。Canvas2D 的 source-over 不能天然做到精确 `L×(底色+纹理)`；若多层合成使暗面颗粒/层理仍像贴纸，须返回算法评审，不能只凭亮暗差转正。F1 的全幅 blit `0.04–2ms` 仍属低置信外推。 |
| **③ XComponent + EGL/OpenGLES 自管栈** | 只读 128 列物理快照上传小型高度/材质参数纹理，GPU fragment shader 在沙 mask 中逐像素算同一 `Lmat` 并耦合 B/C；水/雾单独着色。 | GPU 仍需处理约 `0.9M` fragments/帧（90Hz 约 `81M`/秒）及纹理带宽；CPU 侧小量上传、draw/swap，具体 ms **未知**。需新 surface、同步、输入/生命周期、色彩及证据等价性工程，P-013 级，工作量需 architect 估算。 | 若 F1 指标裁决②选择换栈，E2 可作为同一 shader 管线的**需求输入**，避免在 Canvas2D 重复投入；shader 只降低 E2 调制的 CPU 负担，不能替代 F1 物理重载治理，也不能突破 90Hz 面板上限。F1②未正式立项时，不为 E2 单独默认换栈。 |

**联动决策**：近期若保持 Canvas2D，先评审路线②最小原型，再用路线①小 ROI 作为画质/成本对照；二者都须通过纹理耦合及真机 P95。若 manager 的 F1 指标②/P-013 换栈获批，暂停为 Canvas2D 专门扩大的 E2 缓冲投资，改提交统一 shader 输入/测量契约。路线①若在热态或重载窗超预算而②无法保留纹理耦合，则记 `E2_ROUTE_BLOCKED`，交 manager 在效果、排期与 P-013 之间裁决；不为过线降低 B/C/E3 锚点或隐藏物理开销。

## 5. 缓存失效、配置与验证排期

路线②至少区分：`sandTop` 形状/显示几何变更→重算法线/光照图；材料层边界/配比变更→更新材质 mask 与组合，光照图是否重烘按输入依赖决定；光向/强度/衰减参数变更→重烘；纹理相位随材料运动→B/C 各自更新，**不应为了颗粒相位每帧重烘坡面光照**；水雾浓度变化→水层更新，不应改沙法线；reset/inject/invert/尺寸改变→明确 epoch、首帧基线和断配对。逐帧输出各类 `dirty reason / bake count / bake ms / upload bytes / composite ms`，静置零无故重烘，动态变化不漏烘。E3 曾出现阈值让纹理欠更新的风险；光照阈值须靠 ON/OFF 配对画质与帧时共同定，不仅靠 Node 单测。

E2 参数建议在 [外观参数可调化候选](algorithm-P012-appearance-controls-candidates-v1.md)的 `appearance` 配置域内新增**候选** `topLightDirection/topLightStrength/faceDepth`，沿用草稿→校验→应用/恢复推荐值和 renderer 资源原子切换；不要塞进 `WaterSandParams.configure()`，不改物理配置 epoch。参数范围、默认值及组合风格适用域待 E2 测量后提出 revision。EAB 已 `VALIDATED`，Q1 仍 `PENDING/NON_BLOCKING`；E2 新效果不回退历史结论。若修改上层雾色，须同时按用户“白色云雾”锚定做真机复核，不能让顶光成为提高白度的替代品。

建议下一步：effector/tester 冻结同尺度厚沙丘左右坡面及平沙反例素材、测参考与 OFF 的 `ΔY_face` 和误差带；architect 审查路线②准确合成及 E3/F1 缓存接口，并把三路线的真实 CPU/RS/呈现帧时采集分解写入方案；developer 在获批后只做最小可撤销原型；manager 根据画质归属证据与 F1 指标裁决决定产品路线。正式效果锚点与性能 PASS 需后续独立冻结，本开题不预签。
