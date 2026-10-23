# P-013 立项任务书：XComponent+EGL 渲染栈更换（E2 shader 化 + 90Hz 单拍窗口）

日期：2026-10-23。责任：architect。状态：**PROPOSAL / PENDING_MANAGER_INCORPORATION_RULING**——纯文书零代码；本任务是立项批准的**输入**，不是批准本身；P0 开工等 manager 正式立项签发。依据：[路线③评估报告](architecture-P012-E2-route3-xcomponent-egl-assessment-v1.md)（已验收——数字引用不重估）、manager 10:34 裁决「路线③尽早要做」、[帧时三段拆分基建](fps-seg3-P012-v0.md)（0f62f48 已交付，真机出数归 effector）、[路线②原型](…)（2b7de23 已入库）。并行输入：designer 的 E2 效果目标设计（6adafde7…666，**产出时合流**——本任务的 E2 效果里程碑引用其锚点候选，设计稿落地后回填引用并复核里程碑判据）。

## 1. 目标与非目标

### 1.1 目标（双里程碑）

| # | 里程碑 | 判据 |
|---|---|---|
| M1 | **E2 shader 化**：顶光立体感（宽面坡度法线+深度衰减 Lmat）以 fragment shader 逐像素实现，纹理-光照耦合一等（B 颗粒/C 层理随同一光场变化，无贴纸感） | designer 效果目标设计的锚点候选全过（合流后回填）；暗面纹理保留率配对达标 |
| M2 | **90Hz 单拍窗口验证**：动态轻载段呈现帧率进入 90Hz 单拍（帧间隔 ≈11.1ms） | seg3 数据的渲染+合成段 <11.1ms 预算（**条件性**——见 §5 数据依赖；物理段治理不在本项目范围） |

### 1.2 非目标（明确排除，防范围蔓延）

| 排除项 | 理由与归属 |
|---|---|
| 物理重载治理 | shader 只降 E2 调制成本；物理 substeps 帧时归既有物理线（评估报告 §5.3 红线重申） |
| 突破 90Hz 面板上限 | VYG-AL00 硬上限（F1 实测）；更高刷新须换设备，非本项目 |
| G2.7 fpsP95≥100 指标达成 | 90Hz 上限下不可达；指标处置是 Q1 开放裁决（本立项不预判结论） |
| Canvas2D 旧栈的 E2 实现 | 路线②原型（2b7de23）已完成使命（T_data 生成器+CPU 参照）；不为 Canvas 扩大 E2 投资（评估报告 §4 联动决策） |
| 物理引擎/GLES 物理 | 物理保持 ArkTS 128 列零改动；native 侧只做渲染 |

### 1.3 范围（照评估报告 §3 分级，不重述数字）

P0 最小可见（8-12 人日）→ P1 纹理耦合（10-14）→ P2 雾/水层（10-14）→ P3 全量迁移+旧栈退役（10-12）；总计 38-52 人日（±40%）。**双栈并存 P0-P2，P3 才退役 Canvas**（评估报告 §1.2 裁定不变）。

## 2. P0 开工级详细设计（8-12 人日段）

### 2.0 前置里程碑 P0a：hello-surface（首踩成本对冲，1-2 人日，P0 内第一步）

**最小 native 验证**：cpp 模块编译通过 + XComponent surface 创建 + EGL context 初始化 + 清屏色 swapchain 呈现（一个纯色三角形都不要——只要 surface 上能出指定清屏色）。**验收标准**：真机上 XComponent 区域显示清屏色、旋转/前后台切换不崩溃、帧回调内 eglSwapBuffers 返回 EGL_TRUE。**这是全部后续工作的地基**——工具链（ohos.toolchain.cmake）、签名、napi 桥、调试链全部在此步打通；hello-surface 不过，P0 其余项不开始（风险对冲，见 §3）。

### 2.1 cpp 模块骨架

```text
entry/src/main/cpp/
  CMakeLists.txt            # ohos.toolchain.cmake 引用 + libace_napi + libEGL + libGLESv3
  hello/                    # P0a：surface 初始化（后保留为诊断入口）
  p013renderer/
    napi_bridge.cpp         # ArkTS↔C++：init(surfaceId)/feedTData(Float32Array)/
                            #   setParams(对象)/drawFrame(frameCtx)/release()
    egl_context.cpp/.h      # display/config/context/surface 生命周期（含丢失恢复）
    shader_pass.cpp/.h      # P0：单 pass E2；P1+：多 pass 管理
    tdata_uploader.cpp/.h   # T_data 纹理上传（128×R RGBA32F）
    glsl/e2_lmat.frag/.vert # §2.5 GLSL
```

**验收标准**：hvigor 命令行构建通过（既有构建流程 + `--mode module` 增 cpp 目标）；HAP 含 .so；真机加载无 dlopen 失败。

### 2.2 EGL context+surface 初始化

`onSurfaceCreated` 回调（ArkTS 侧经 napi 传 native window）→ `eglGetDisplay/eglInitialize/eglChooseConfig`（RGBA8888 + EGL_WINDOW_BIT）/`eglCreateWindowSurface`/`eglCreateContext`（GLES3）。**surface 尺寸变化**（onSurfaceChanged）：`eglQuerySurface` 重查+viewport 更新。**丢失恢复**：`eglMakeCurrent` 失败/SurfaceDestroyed → 全 GPU 资源释放标记 + 下次 Created 重建（纹理内容可丢——T_data 每帧重传，B/C 纹理按 epoch 重传）。

**验收标准**：前后台切换 10 次无泄漏（native 内存曲线平稳）；旋转后画面几何正确（viewport 跟随）。

### 2.3 XComponent 页面叠层方案（裁定：叠在 Canvas **之上**）

```text
Stack 布局：Canvas（底层，既有全部绘制 E/A/B/C/E3）
          + XComponent（上层，E2 沙区 Lmat 乘性增量层）
```

**裁定**：P0 阶段 XComponent 叠 Canvas 之上，shader 输出 = 沙区 Lmat×现有画面颜色的增量层（blendFunc: DST_COLOR, ONE 或等价乘法混合），非沙区 Lmat≡1（透明调制，完全透出 Canvas）。**裁定理由**：Lmat 是乘性调制（`ToneMap(Lmat×Rmat)`），作用在**已合成的 Canvas 画面**上等价于作用在 Rmat 上（乘法结合律）——底叠方案需 shader 重画全部材质（P1+ 的范围），上叠只需增量层。mask 判定在 fragment 内做（§2.5），非沙区零影响。**验收标准**：E2 开关（frameContext）on/off 时画面差异仅在沙区；水/雾区逐像素不变（截图 diff）。

### 2.4 T_data 上传链

每物理帧（postFrameCallback 内、drawFrame 前）：`feedTData(Float32Array)`——**数据源 = 路线②原型（2b7de23）的 CPU 光照图生成器直接复用**（评估报告 §5.4-a 契约：128 列 sandTop/bh/mixStart/waterTop + CPU 预计算宽面坡度 row7）。napi 侧 `napi_get_typedarray_info` 零拷贝读 → `glTexSubImage2D`（RGBA32F 纹理，128×2 打包：row0-3 物理量，row7 坡度——**P0 打包为 128×1 RGBA32F**（4 通道正好装 4 个标量场，坡度暂由 shader 内差分——P0 帧内每 fragment 4 次采样即够；row7 上传留给 P1 优化槽）。

**验收标准**：上传 60Hz 持续 30 分钟无内存增长；单次上传 <0.1ms（napi+gl 计时埋点）。

### 2.5 单 pass E2 沙区 Lmat shader（GLSL 草案）

```glsl
// e2_lmat.frag（P0 草案——参数槽位照 designer 效果目标设计合流后定稿）
precision highp float;
uniform sampler2D u_tdata;        // 128×1 RGBA32F: (sandTop, waterTop, bh, mixStart)
uniform vec2  u_resolution;       // XComponent surface 尺寸
uniform vec3  u_lightDir;         // 顶光方向（含微弱横向分量）
uniform float u_la, u_ld, u_sigma, u_ellLight, u_lmin, u_lmax, u_lref;
in vec2 v_uv;
out vec4 outColor;

float sandTopAt(float x01) {                      // x01∈[0,1) → 列坐标 → 双线性
  return texture(u_tdata, vec2(x01, 0.5)).r;
}
float mixStartAt(float x01) { return texture(u_tdata, vec2(x01, 0.5)).a; }

void main() {
  float x01 = v_uv.x;
  float yPx = (1.0 - v_uv.y) * u_resolution.y;    // 显示 y（px，底为 0）
  float sTop = sandTopAt(x01) * u_resolution.y;   // 材料高度 → 显示 y（换算按 heightToY 逆）
  // 沙 mask：yPx < sTop 才是沙区（材料系 y 向上）——精确判定随合流设计稿标定
  if (yPx >= sTop) { outColor = vec4(1.0); return; }   // 非沙区：Lmat≡1（透明调制）
  // 宽面坡度（P0：相邻采样差分近似 G_σ*S；P1 换 row7 上传）
  float dx = u_resolution.x / 128.0;
  float pb = (sandTopAt(x01 + dx / u_resolution.x) - sandTopAt(x01 - dx / u_resolution.x))
             * u_resolution.y / (2.0 * dx);
  vec3 n = normalize(vec3(-pb, 0.0, 1.0));
  float lface = clamp(u_la + u_ld * max(0.0, dot(n, u_lightDir)), u_lmin, u_lmax);
  float z = sTop - yPx;                            // 材料深度（显示 px 换算）
  float w = exp(-z / u_ellLight);
  float lmat = 1.0 + w * (lface - u_lref);
  outColor = vec4(lmat, lmat, lmat, 1.0);          // 乘性调制（混合模式见 §2.3）
}
```

**验收标准**：左右坡面 sign(ΔY) 与光向一致（designer 锚点候选）；平沙区无虚假分区（Lmat≡近 1）；暗面 B 颗粒/C 层理仍可辨（无乘黑）；帧时单 pass <1ms（真机埋点）。

### 2.6 帧回调提交路径

`postFrameCallback`（既有帧驱动不动）→ 物理步进 → `feedTData` → Canvas 绘制（既有 render() 全链）→ `drawFrame(frameCtx)`（napi → GLES：useProgram/bind/feed uniforms/draw quad/eglSwapBuffers）。**同步语义**：P0 在同一线程顺序提交（无独立渲染线程——评估报告 §1.2-④ 的简化，P2 若需再议双缓冲）。

**验收标准**：E2 on 状态 30 分钟 60fps 无掉帧回归（对照 Canvas-only 基线帧时差 <1ms）；swap 与 vsync 对齐（帧间隔直方图单峰）。

## 3. 风险登记册（六交接点逐项缓解 + 首踩对冲）

| # | 风险（评估报告 §1.2） | 概率 | 影响 | 缓解措施 | 残余风险 |
|---|---|---|---|---|---|
| R1 | Surface 生命周期（前后台/EGL 丢失） | 高 | 中 | §2.2 丢失恢复协议（全释放+重建）；P0a 验收含 10 次切换 | 极端切换时序窗口 |
| R2 | 输入事件透传（Pan/长按与 XComponent 层级） | 中 | 高 | **XComponent 设 hitTestBehavior(HitTestMode.None)**——全部手势仍归页面 Stack；P0a 验收含手势回归 | 低 |
| R3 | 180° 反转/surface transform | 中 | 中 | P0 靠页面 transform 预旋 T_data（坐标在 CPU 侧变换，surface 不感知方向）；native window transform hint 的原生跟随留 P2 槽位 | 每帧 CPU 变换成本（微小） |
| R4 | vsync 对齐（postFrameCallback 与 swap 错拍） | 中 | 中 | §2.6 同线程顺序提交；帧间隔直方图验收 | 多缓冲撕裂（P2 双缓冲解决） |
| R5 | 色彩空间（Canvas sRGB vs EGL 默认） | 高 | 高 | **P0a 首日验证**：同色值两栈出图 diff；若不一致，GLES 侧统一 sRGB framebuffer（EGL_GL_COLORSPACE）或 shader 末尾 gamma 校正 | 校正后人眼可辨的轻微差异（黄金图容差吸收） |
| R6 | Node 1:1 测试断代 | 确定 | 中 | §4 补偿双层（CPU 参考+黄金图）；路线②原型的 render_units（2b7de23）继续覆盖 CPU 侧 | shader 逻辑的覆盖缺口（黄金图频率对冲） |
| R7 | 首踩成本超估（工具链/签名/调试） | 中 | 高 | **P0a hello-surface 前置**（§2.0）：工具链风险在 1-2 人日内暴露，不带入 P0 主体 | 低 |
| R8 | 混合模式与既有画面冲突（乘法调制发暗/过曝） | 中 | 高 | §2.3 叠层裁定的 mask≡1 兜底 + P0 验收含水/雾区零变化断言 | 暗面贴纸（P1 纹理耦合才真正检验） |

**总体对冲**：P0a 未过 → 项目止损在 1-2 人日（远小于评估区间下界 8）；R5 色彩若不可调和 → 升级 manager 裁决（黄金图容差 or 换叠层策略）。

## 4. 与既有验证体系的衔接

1. **Node 断代补偿（双层）**：CPU 参考实现单测（路线②原型的光照图数据结构+`run_p012_render_units.cjs` 继续跑，覆盖 T_data 生成正确性）+ 黄金图真机回归（冻结场景组：静态/倾斜 30°/倒立/重载 × E2 on/off = 8 帧/场景，SSIM+逐像素差异分布，差异>容差即回归——容差在合流设计稿冻结时一并定）。
2. **s133 锚点跨栈重出基线（计划槽位）**：P0 验收后、P1 开始前，跑 E3 三锚点（J95/J90/std 比）在 GLES 栈的 OFF/ON 配对——**重出基线不是重冻阈值**（评估报告 §4-1）；数据回填锚点 revision 的 renderer 栈维度列。**P0 期间 Canvas 全量保留**（双栈并存）——E3 锚点的既有测量不受 P0 开发影响。
3. **路线②原型复用声明**：2b7de23 的 CPU 光照图生成器 = P0 T_data 数据源（§2.4，直接调用零改动）；其 toplight 开关/frameContext 扩展 = P0 的 E2 开关底座；其单测（158 行 render_units）= T_data 回归测试。**路线②代码不废弃**（评估报告 §5.4 结论：它是 P0 的生成器与 CPU 参照）。

## 5. 帧时数据依赖处置（条款）

**P0 启动不依赖 seg3 三段数据**（0f62f48 基建已交付、真机出数归 effector）——理由：P0 是双栈并存的增量层（Canvas 零改动），不存在"选错栈"的不可逆风险；P0a 的 hello-surface 验证与帧时数据正交。**P1+ 排期在 seg3 v1 数据出来后校准**：渲染段实测占比决定 P1 纹理耦合的优先序（若渲染段本就 <3ms，P1/P2 的迁移收益重估，可能收缩为"只做 E2+选择性迁移"）；物理段占比若 >5ms，M2 里程碑（90Hz 单拍）标注为"受物理治理制约"并回归 manager 排期裁决。**本条款防两种错误**：为等数据搁置 P0（不必要）、无视数据硬推 P1+（不诚实）。

## 6. 里程碑与退出判据

| 里程碑 | 内容 | 判据（全过才进下一档） | 退出/止损 |
|---|---|---|---|
| **P0a** | hello-surface | §2.0 三项验收 | 不过 → 止损 1-2 人日，风险登记册 R7 升级 |
| **P0** | E2 沙区 Lmat 单 pass | §2.1-2.6 全验收 + E3 锚点 Canvas 臂无回归 + 黄金图组建立 | 贴纸感明显（designer 判）→ 仍进 P1（耦合正是 P1 主题）但 M1 延后警示 |
| **P1** | B/C 纹理 GPU 耦合 | 暗面纹理保留率配对 + 跨栈锚点重出基线 + seg3 数据校准排期 | 帧时超预算 → 降级方案（评估报告 §4 双栈共存到期） |
| **P2** | 雾/水层 shader 化 | E3 s133 锚点 GLES 臂重出 + 全生命周期验收 | E3 锚点跨栈不可比 → 升级 manager（锚点体系修订议题） |
| **P3** | 旧栈退役 | 全量回归 + 文档/测试重构 | 不可逆点——退役前 manager 复核 |

**立项判据（manager 裁决点）**：M1+M2 双里程碑价值 vs 38-52 人日（±40%）+ Q1 指标处置联动。本任务书与 designer 效果目标设计（并行）合流后提交。

## 7. 红线复述

纯文书零代码；评估报告数字引用不重估（38-52 人日/±40%/90Hz 上限）；不授权立即实施（P0 开工等 manager 正式立项签发——本任务是输入不是批准）；物理重载治理/90Hz 突破/G2.7≥100 明确非目标；Q1 开放裁决不预判。

变更记录：2026-10-23，依据 manager【P-013·立项任务书】任务产出。五项工作：立项结构（双里程碑/五项非目标/P0-P3 范围）/P0 开工级细化（P0a hello-surface 前置对冲+六子项各带验收+GLSL 草案+叠层裁定 Canvas 上乘性增量层）/风险登记册（8 项：六交接点+首踩+混合冲突，各带缓解与残余）/帧时数据依赖条款（P0 不等数据/P1+ 校准）/验证体系衔接（双层补偿+跨栈重基线槽位+路线②复用声明）。
