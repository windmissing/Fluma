# P-012 E2 路线③评估：XComponent+EGL 换栈工程量与架构评估 v1

日期：2026-10-23。责任：architect。状态：**ASSESSMENT_ONLY / NOT_IMPLEMENTED / ROUTE_DECISION_INPUT**——纯评估零代码，供 manager 与 F1 指标②（P-013 换栈决策）合并裁决；不授权换栈、不冻结锚点、不改变 EAB/E3/C 已验证结论。依据：[E2 开题 §4 路线③+§5](algorithm-P012-E2-top-light-opening-v1.md)（manager 10:22 批「两条都要」）、[F1 归因](fps-attribution-P012-v1.md)、[F1 动态消融](f1-dyn-ablation-P012-v1.md)、[F1 离屏模型验证](f1-offscreen-model-validation-v1.md)。方法：现有代码盘点（renderer 2187 行/36 draw 点/双 E3 buffer）+ 本地 SDK 26.0.0 native 面核查 + ArkTS/GLES API 文档级评估。

## 0. 执行摘要

| 项 | 结论 | 证据强度 |
|---|---|---|
| 换栈总工程量 | **P0-P3 全量 ≈ 38-52 人日**（P0 最小可见 8-12 人日）；对比：路线② Canvas 原型 developer 并行任务量级 3-5 人日 | 代码盘点+API 面静态评估，**±40% 区间**（无 native 模块先例，首踩成本主导方差） |
| 换栈能否达成 fpsP95≥100 | **不能**——VYG-AL00 面板 90Hz 硬上限（F1 §1 实测）；换栈上限 = 90fps | F1 实测（强） |
| 换栈能否达成 90Hz 单拍 | **条件可能**——渲染+合成须 <11.1ms；当前 15-17ms 中渲染占多少未拆分（F1 未做渲染/合成分段归因），shader 化物理不动的前提下能否压进 11.1ms **未知** | 中（帧循环分段数据缺失） |
| E2 效果收益 | shader 逐像素 Lmat 耦合 B/C 是三条路线中唯一能保证「纹理-光照一等工作」的实现形态；CPU 路线①有 52 倍退化前科、伪 shader②有贴纸风险 | 算法层（强）/ 性能层（未测） |
| 建议 | **P-013 立项与否拆两问**：a) 90Hz 目标值不值得（F1 指标冻结先决）；b) 若立项，E2 作首批 shader 需求与 P0 里程碑合流，避免 Canvas 侧重复投资 | — |

## 1. 栈替换边界盘点（任务 1）

### 1.1 现状基线（代码盘点 2026-10-23）

- 渲染栈：`SandCanvas` ArkUI `Canvas` + `CanvasRenderingContext2D`（:452/:2805）→ `P012WaterSandRenderer.render(ctx,...)`（:311），**36 个 draw 调用点**、2187 行；纯 ArkTS 工程（无 cpp 目录、无 napi/native 模块先例——**首踩成本**）。
- 资产：3 张 256² 颗粒 ImageBitmap + 3 张 64×256 层理位图 + 2 个 E3 雾 buffer（P012E3FogBuffer，OffscreenCanvas+transferToImageBitmap）+ sampler/motionCache/appearance 配置层（§13 全量刚交付）。
- 帧驱动：UIContext.postFrameCallback（vsync 对齐，60Hz 节拍实测锁定——F1 §2.1）。
- 生命周期挂钩：resetContinuity（:663，reset/inject/invert/后台恢复）、fieldEpoch/geometryEpoch/continuityEpoch、appearance configRevision。

### 1.2 XComponent surface 接管面

**XComponent 接入本身**（ArkTS 侧）：`XComponent(id, 'surface', '2d')` 声明式挂载 + `onSurfaceCreated/Changed/Destroyed` 回调拿 native window handle → 经 napi 传给 C++ 侧 EGL。**关键改造点：页面结构从单 Canvas 变 Canvas+XComponent 叠层或纯 XComponent**——Canvas 上的既有 UI（调参面板按钮、测试模式按钮、手势）不在 Canvas 里（面板是独立组件），**手势/传感器/物理循环零改动**（都在 SandCanvas 页面层，不经渲染 API）。

**双栈并存 vs 全替换（裁定：P0-P2 双栈并存，P3 才退役 Canvas）**：

| 阶段 | Canvas2D 层保留 | GLES 层接管 | 理由 |
|---|---|---|---|
| P0 | 全部（E/A/B/C/E3 原样） | +1 个 E2 沙区光照 pass（XComponent 叠在 Canvas 之上或之下） | 最小可见：只验「shader Lmat 与现有画面合成」；旧锚点回归不受影响 |
| P1 | B/C 位图生成+CPU 侧 | B/C 平移为 GPU 纹理采样+shader 合成 | 纹理耦合一等化的实质阶段 |
| P2 | 水雾（E3 buffer） | 雾/水层 shader 化（alpha 合成移 GPU） | E3 锚点双栈等价验证 |
| P3 | 退役 | 全量（含 OFF 分支/orientation 兼容渲染） | 旧栈删除，单一维护面 |

**交接点清单**（每项都是实现风险源）：①Surface 生命周期（前后台切换 surface 销毁/重建——EGL context 丢失恢复、GPU 资源重建）；②输入事件透传（XComponent 默认不消费手势，但要验证与 PanGesture/LongPress 的层级共存）；③180° 反转/旋转：当前 Canvas 靠页面 transform；XComponent surface 的方向跟随须验证（OH native window 的 transform hint）；④vsync 对齐：postFrameCallback 继续驱动、shader pass 在回调内提交 vs 独立渲染线程（后者引入双缓冲同步）；⑤色彩空间一致性（Canvas sRGB vs EGL surface 默认色彩——锚点逐位对比的最大坑）；⑥Node 1:1 测试通道断代——GLES 无 Node 模拟，单测体系须重构为「光照图数据（CPU 侧）单测 + shader 黄金图真机回归」双层。

## 2. 光照 shader 契约（任务 2）

### 2.1 输入纹理布局（E2 开题 §2 的 Lmat 公式 → fragment 输入）

```text
纹理 T_data（128×R，R=行数按需，GL_R32F 或 RGBA32F 打包）：
  x 方向 = 128 物理列；每行一个属性层：
  row0: sandTop  row1: bh  row2: mixStart(=bh+bl)  row3: waterTop
  row4: suspH    row5: suspL（q 项按 depth 归一在 shader 内算或 CPU 预算 row6）
上传频率：仅物理 step 后（≈60Hz），128×6 floats ≈ 3KB/帧——上传成本可忽略。
纹理 T_param（1×N uniform block 或小纹理）：
  E2 参数（σ/ellLight/La/Ld/Lref/Lmin/Lmax/光向）+ 六参数 appearance
  （fine/sigma/f/λ/强度/c）+ palette——appearance 变更走 §13 configRevision
  失效钩子重传。
纹理 T_tex：B 颗粒×3、C 层纹×3 现有位图 → glTexImage2D（初始化/
  重烘时上传；f/c 等参数变更的失效矩阵照 §13.4 平移）。
```

**宽面坡度 p_b(x)=∂x G_σ*S 的归属（裁定）**：σ 低通在 **CPU 侧**预计算（128 列卷积，O(128×k) 廉价，且与 sampler 的 smooth 数组共用）后作为 T_data row7 上传——shader 内只做双线性插值+法线构造。避免在 shader 里做高斯核卷积（每 fragment 重复计算），也保证与 Canvas 路线②的**光照图数据结构完全同源**（见 §5 复用节）。

### 2.2 沙/水/雾 mask 判定（裁定：纹理通道重建）

沙 mask 不上传位图——由 T_data 的 sandTop/waterTop/bh/mixStart 在 fragment 内重建（`y > sandTop(x)` → 沙区，`sandTop..waterTop` → 雾区等），与 CPU 侧 sampler 判定同式。理由：mask 上传（0.9Mpx×1bit）成本高且引入两处真相；重建的代价是每 fragment 4 次纹理采样+分支，GPU 可承受。**风险**：CPU/GPU 两处 mask 判定式漂移——收敛为共享 GLSL 与 TS 双实现的**同一公式文档**（§4 等价验证锚点覆盖）。

### 2.3 与 P-012 外观配置衔接

`P012AppearanceParams`（§13.2）+ E2 新增候选参数（topLightDirection/topLightStrength/faceDepth，开题 §5）同走 `applyAppearance` 入口 → GLES 侧 = T_param 重传 + 相关 pass 的 uniform 更新；**不新增第二配置通道**。持久化 `p012.appearance`（§13.3）原样；草稿→校验→应用生命周期在 Canvas/GLES 双栈下语义不变（staging 的「烘焙」在 GLES 侧退化为纹理重传，更便宜）。

## 3. 工程量分级（任务 3）

| 阶段 | 内容 | 人日估算 | 风险标注 |
|---|---|---|---|
| **P0 最小可见** | cpp 模块接入（CMake+napi 桥+CMakeSources）+ EGL context/surface 初始化 + XComponent 页面叠层 + T_data/T_param 上传链 + 单 pass E2 沙区 Lmat shader（B/C 纹理以现有位图上传，简单乘法耦合）+ 帧回调提交 | **8-12 人日** | **首踩成本主导**：项目首个 native 模块（工具链/CMake ohos.toolchain/签名/调试链全部首次）；EGL surface 与 Canvas 的 z 序/合成正确性未知 |
| P1 纹理耦合 | B/C 平移 GPU 采样（位图→纹理生命周期、失效矩阵平移、appearance 六参数全通）+ 合成顺序（ToneMap(Lmat×Rmat) 在 shader）+ Canvas 侧 B/C 停用开关 | 10-14 人日 | 贴纸问题的真实检验；锚点等价（§4）首次全量跑 |
| P2 雾/水层 | E3 雾 shader 化（两 buffer 路径改 shader pass 或 buffer 上屏保留）+ 水面/界面带 + 前后台/旋转/surface 重建全生命周期 | 10-14 人日 | E3 s133 冻结锚点双栈等价——J95/J90/std 比的跨栈可比性风险最高 |
| P3 退役+全量 | OFF 分支/orientation 兼容渲染平移、Canvas 渲染代码删除、文档/测试体系重构（Node 通道断代重设计） | 10-12 人日 | 旧功能回归面最大；**不可逆点**（删代码） |
| **合计** | | **38-52 人日** | ±40%（无 native 先例） |

**对比锚**：路线② Canvas 伪 shader 原型（developer 并行任务 6adac509…461）≈3-5 人日——**P0 一项就是路线②全量的 2-3 倍**。这是 manager 裁决的核心成本事实。

## 4. 证据等价性（任务 4）

GLES 与 Canvas2D 输出对齐的验证方案（**换栈后既有冻结锚点的连续性问题——最大工程风险之一**）：

1. **像素级逐位对齐在跨栈下不可行**（shader 浮点路径/合成顺序/GPU 双线性 vs Canvas 的实现差异）——**锚点须按「指标等价」而非「逐位等价」重新建立**：J95/J90（横向硬跳）是差分统计，跨栈可比；std 比（E3 防抹平）可比；hfH/lumStd（B/C 冻结锚点）可比；**但所有「冻结数值」的适用域声明（s133 构建）须追加 renderer 栈维度**——同方法锚点在 GLES 栈下重新出基线（不是重冻阈值，是重出 OFF/ON 配对的分母分子）。
2. **黄金图回归**：冻结一组代表性场景（静态/倾斜/倒立/重载），Canvas 与 GLES 各出图，SSIM/逐像素差异分布报告——差异>阈值即回归。GPU 确定性（同 seed 同输入同帧）须先验证（Adreno/Mali 的 shader 编译器优化可能破坏位确定性——用中位数滤波类稳定算子）。
3. **Node 1:1 通道断代**：现有 run_p011_planning_units.cjs 模式（TS 加载物理源码）对渲染无效（本来也不测渲染），但 renderer 的 Node 单测（E3 buffer/sampler/appearance）在 GLES 侧对应物缺失——P1/P2 的 shader 逻辑须靠 **CPU 参考实现单测**（光照图数据结构 CPU 侧保留，§2.1 裁定使其可测）+ 黄金图真机回归双层补。
4. **双栈并存期的对照**：P0-P2 期间 Canvas 全量保留=天然对照组——同帧双栈出图对比可自动化（开发期诊断，非验收）。

## 5. 与 F1 的合流点（任务 5）

1. **F1 指标②若立项，本栈替换就是其载体**——F1 §4 已裁「当前架构内无部分加速可用项；全量换栈属 P-013」。E2 是 P-013 的**首批实质需求**（无 E2 的换栈=纯性能工程；有 E2=性能+效果双收益），建议 P-013 立项书以「E2 shader 化 + 90Hz 单拍窗口验证」为双里程碑。
2. **90Hz 上限重申**：换栈后 fpsP95≤90（面板硬上限）；G2.7 fpsP95≥100 在 VYG-AL00 上**无论换不换栈都不可达**——F1 §5 已建议指标修订或换设备，本报告背书该结论（证据强度：实测面板刷新率，强）。
3. **shader 只降 E2 调制成本，不解决物理帧时**：F1 动态消融显示物理 substeps 在重载窗的帧时占比可观——换栈后若物理+上传仍 >11.1ms，90Hz 单拍仍不可达。**P-013 立项前置须补「渲染/合成/物理三段帧时拆分」实测**（F1 未做，本报告列为缺口）——没有这个数据，「换栈→90Hz」只是假设。
4. **与路线②的资产复用**（任务约束节）：路线②（Canvas 伪 shader 原型）的产出直接进 GLES 管线的部分——**a) 光照图数据结构**（128 列坡度/法线/Lambert 的 CPU 预计算，§2.1 已裁定同源）＝ GLES 的 T_data row7 生成器；**b) 失效矩阵框架**（E3 的 dirty 机制+§13.4 矩阵）＝ GLES 纹理重传的失效逻辑；**c) 参数域/校验/appearance 配置层（§13 全量）**＝ T_param 的来源，零改动；**d) B/C 位图资产**＝纹理上传源。**不可复用**：合成路径本身（source-over 多层 vs shader 单 pass——这正是换栈的意义）。**结论：路线②原型不是丢弃品，是 P0 的 T_data 生成器与等价验证的 CPU 参照**——即使换栈立项，路线②也应完成（成本 3-5 人日，产出直接进 P0）。

## 6. 结论与裁决输入

**给 manager 的三个问题**（本报告不回答，只供数据）：

| # | 裁决问题 | 本报告数据点 |
|---|---|---|
| Q1 | G2.7 fpsP95≥100 指标处置（修订为 90/60/窗口口径 vs 换更高刷设备） | 90Hz 硬上限实测；换栈不能突破 |
| Q2 | P-013 换栈是否立项 | 总量 38-52 人日（±40%）；E2 双收益背书；**缺渲染/物理帧时拆分数据**——立项前建议先补（约 2-3 人日测量工作，可与路线②原型同批） |
| Q3 | 若不立项换栈，E2 走路线①还是② | ①有 52 倍退化前科（强证据不推荐全幅）；②3-5 人日原型先行+贴纸风险检验——②是稳妥路径，其 CPU 光照图产出保留换栈可复用性 |

**红线自查**：纯评估零代码；未承诺「换栈解决一切」（每项收益已标证据强度）；EAB/E3/C 已验证结论未触碰；Canvas2D 路线②原型的资产复用性已单列（§5.4）。

变更记录：2026-10-23，依据 manager【P-012·E2路线③评估】任务产出。五项工作落盘：栈替换边界（双栈并存 P0-P2/P3 退役+六交接点）/shader 契约（T_data 七行布局+CPU 预计算坡度+mask 重建式）/P0-P3 分级 38-52 人日（±40%）/证据等价（指标等价非逐位+黄金图+Node 断代双层补偿）/F1 合流（90Hz 上限重申+帧时拆分缺口+路线②资产复用）。供 manager 与 F1②合并裁决。
