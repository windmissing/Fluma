# P-013 P1-G0 真机平台探针回执 v1（effector）

- 日期：2026-10-25；执行：effector；设备 VYG-AL00（OpenHarmony 7.0/API 26），GPU vendor=HUAWEI
- 任务：6add6012【P-013·P1-G0真机probe】surface / sRGB attachment / 三层 z 序 / 透明 / 输入透传五项平台探针取证（G1 解冻前置之二·第二项，渲染线 XComponent+shader 相关，P5）
- 探针实现：新增 `entry/src/main/ets/pages/P013P1G0Probe.ets`（PROBE-ONLY）+ `EntryAbility.ets` 探针门（`p013g0/p013g0lib/p013g0hit/p013g0diag`）+ `main_pages.json` 登记；工具 `tools/p013_p1g0_probe.ps1`、`tools/p013_p1g0_surface_cycles.ps1`、`tools/analyze_p013_p1g0.py`
- 构建：`entry-default-s139h-signed.hap`（含 P-013 native renderer + 探针页）；SignHap 00303116 仍为环境问题，手动 `hap-sign-tool` 签名
- dida365 MCP 全程 403/unauthorized，任务正文未能读取，按标题 + 预注册口径执行，本回执本地归档

## 一、总体结论

| # | 探针 | 结论 |
|---|---|---|
| ① | surface 生命周期 | **PASS**：surface/上下文重建路径可用；6 次前后台切换后同一进程存活、窗口内 7 次 `EGL init ok (GLES3); vendor=HUAWEI`、3 次 `OnSurfaceCreated`、零崩溃 |
| ② | sRGB attachment | **PASS（当前内容下无错配）**：GLES 区实测 (46,51,55) **等于** authored 8bit 值；若 framebuffer 被当线性处理应读 (118,124,129)。**但代码层未请求 `EGL_GL_COLORSPACE_SRGB`**（见 §4.4） |
| ③ | 三层 z 序 | **FAIL（结构性）**：XComponent SURFACE **恒在所有 ArkUI 内容之上**，应用前景 Canvas 被完全覆盖 → 提案 §2.3 的"Canvas 背景 / GLES 沙区 / Canvas 前景"三层职责在当前叠层下**不可实现** |
| ④ | 透明 | **BLOCKED**：依赖 P1 诊断 pass 的 alpha 合成行为，而该路径的帧提交在真机上 100% 被 native 拒绝（见 §4.1）。代码级条件已具备（shader 盒外输出 `vec4(0.0)`、EGL `ALPHA_SIZE=8`） |
| ⑤ | 输入透传 | **PASS（但必须显式配置）**：`hitTestBehavior=Default` 吞掉下层 Canvas **全部**触摸（0 次 Down）；`None` 与 `Transparent` 各 2 次 Down → 透传成立。SandCanvas 现有 `.hitTestBehavior(HitTestMode.None)` 正确且**必要** |

**头号发现（P1 阻塞级，见 §4.1）**：P-013 P1 渲染栈的 ArkTS→native **帧提交路径在真机上 100% 被拒**（1742/1742 帧 `code=100 / FRAME_REJECTED`），而 `init` 成功（surface 已到手）。这使 P1 的全部验收（热图断言、帧时、合成）在该构建上不可执行。

## 二、探针设计与叠层

探针页刻意镜像 P-013 真实集成（`SandCanvas` 在 Stack 末位追加 XComponent 为顶层）：

```
Stack { Canvas bg(底色 #F0F0F0 + sRGB 对照色块 RGB(46,51,56))
      ; Canvas fg(应用画布角色：洋红竖条 24px + 底部洋红条 + 触摸计数)
      ; XComponent(SURFACE, libraryname=p013hello|p013renderer, hitTestBehavior 可切) }
```

- ②sRGB 判据：GLES 恒定清屏色 `glClearColor(0.18,0.20,0.22,1.0)`（`hello_surface.cpp:63`）的 8bit 直写值 = (46,51,56)；背景 Canvas 用**同值**直写作对照。若平台对 GLES 帧缓冲做 linear→sRGB 转换，同一数值应呈现为 (118,124,129)。
- ③z 序判据：GLES 层若覆盖洋红前景条 → XComponent surface 在应用 UI 之上；若洋红可见 → 应用前景在 GLES 之上。
- ⑤输入判据：下层 fg Canvas 的 `onTouch` Down 计数（swipe 一次 = 1 次 Down；启动时另有 1 次 USB 弹窗消解点击）。
- native 库二选一（want `p013g0lib`）：`hello` = `libp013hello.so`（P0a 已验证可用，恒定色）；`renderer` = `libp013renderer.so`（P1 诊断 pass）。

## 三、五项探针实测数据

### 3.1 ② sRGB attachment（PASS）

| 采样点 | 实测 | 判读 |
|---|---|---|
| GLES 区（屏幕中心，hello 模式） | **(46, 51, 55)** | == authored 8bit 值 → GLES 输出按 sRGB 呈现，与 ArkUI Canvas 的 sRGB 直写一致 |
| 若被当线性处理的预期值 | (118, 124, 129) | 实测未出现 → **无色空间错配** |
| 洋红前景条（x=6） | (46, 51, 55) | 被 GLES 完全覆盖（见 3.2） |
| 页面底部（surface 之外） | (250, 250, 250) | 应用背景正常 |

### 3.2 ③ 三层 z 序（FAIL，结构性）

| 场景 | GLES 区（中心） | 洋红前景条（x=6） | 判读 |
|---|---|---|---|
| hello 模式 | (46, 51, 55) | (46, 51, 55) | GLES 覆盖前景条 |
| renderer 模式（P5） | (0, 0, 0) | (0, 0, 0) | 同样覆盖（黑 = 未绘制） |

**判定：XComponent（SURFACE 类型）的合成层恒在 ArkUI 内容之上**，与应用内 Stack 声明顺序无关——即使 XComponent 声明在末位（顶层），它下面的 Canvas 背景层与前景层仍被完全遮挡。

**对 P-013 方案的影响（需 manager/designer 裁决）**：提案 §2.3 与 P1 附件把"Canvas 背景 / GLES 沙区 / Canvas 前景"三层的责任分配给应用与 GLES 两侧，但当前平台行为下**GLES 层无法被应用前景覆盖**，因此"Canvas 前景"职责在现叠层下不可实现。可选路径（不在本卡裁决范围）：① 应用前景改由 GLES 侧合成（把前景也画进 GLES pass）；② 放弃应用前景层、全部视觉归 GLES；③ 换用非 SURFACE 型 XComponent 或调整层级接口。

### 3.3 ⑤ 输入透传（PASS，须显式配置）

| `hitTestBehavior` | 下层 Canvas 收到的 Down | 判定 |
|---|---|---|
| `Default`（平台默认） | **0** | XComponent **吞掉全部触摸** → 应用画布收不到任何手势 |
| `None` | 2（弹窗消解 1 + swipe 1） | 透传成立 |
| `Transparent` | 2 | 透传成立 |

**判定：输入透传必须显式声明 `hitTestBehavior(HitTestMode.None/Transparent)`；平台默认会完全拦截。** `SandCanvas.ets:3164` 现用的 `.hitTestBehavior(HitTestMode.None)` 是正确且必需的——若省略，P-013 一旦上线将直接打断 P-011 的手势掉沙（`routeGestureSand` 依赖 Canvas 的 Pan 事件）。

### 3.4 ① surface 生命周期（PASS）

采集：`p013g0lib=hello` 启动 → 6 轮「Home 到后台 → `aa start` 回前台」，**每轮结束即 dump 一次 hilog**（避免环形缓冲滚落），dump 文件按行去重后统计：

| 事件 | 计数（去重后保留窗口） | 备注 |
|---|---|---|
| `P013Hello: OnSurfaceCreated: window available; EGL creation deferred to draw` | 3 | 窗口交付 → EGL 延迟到首帧创建 |
| `P013Hello: EGL init ok (GLES3); vendor=HUAWEI` | 7 | 每次 surface 重建后重新初始化上下文 |
| `XComponent[p013_g0_hello] triggers onLoad and OnSurfaceCreated callback` | 3 | 框架侧回调正常 |
| 崩溃 / ANR | 0 | 全程单一进程（pid 33192）存活 |

补充：hello 稳态帧计数 1500（`nativeFrames=1500`，与 ArkTS 帧计数一致）→ surface 上 swap 持续成功。

**限制**：hilog 为环形缓冲，轮次间仍有滚落，上表为**下界**；精确计数需改为日志文件直写（非 hilog 缓冲）。renderer 侧的权威计数（`surfaceCreateCount/contextCreateCount/contextLossCount/recoveryAttemptCount/staleLifecycleCount`）因 §4.1 的帧提交被拒而全为 0，无法作为交叉校验。

### 3.5 ④ 透明（BLOCKED）

- 代码级条件已具备：`e2_lmat.frag` 盒外与非沙区输出 `outColor=vec4(0.0)`（全透明），沙区内输出不透明热图；`egl_context.cpp` 的 EGL config 含 `EGL_ALPHA_SIZE=8`（帧缓冲带 alpha 通道）。
- **未能实测**：透明是否成立取决于 P1 诊断 pass 真正绘制出 alpha，而该路径的帧提交 100% 被拒（§4.1）。
- 复测前置：修复 §4.1 后，以 `p013g0lib=renderer` + `p013g0diag=true` 复采，判据 = 盒外/非沙区像素与 `p013g0diag=false` 逐像素相同。

## 四、阻塞与工程约束（移交 developer）

### 4.1 【P1 阻塞级】ArkTS→native 帧提交在真机 100% 被拒

**现象**：`p013g0lib=renderer` 连续提交 1742 帧，全部返回 `code=100`（`Status::INVALID_ARGUMENT`），遥测 `errorStage=FRAME_REJECTED`、`T_dataHash=0000000000000000`（未进入数据拷贝）、`surfaceCreateCount=0`；而 `init` 成功（`state=WINDOW_AVAILABLE`、`ownerBound=true`、`surfaceEpoch=1`）→ **surface 已到手，拒在入参判型阶段**。

**已排除的假设**：
- 不是 EGL/shader 问题：拒绝发生在 EGL 初始化之前（`eglInitMs/makeCurrentMs/swapMs` 全 0，`surfaceCreateCount=0`）。
- 不是帧尺寸/盒区越界：那些校验在 `Validate()` 内，位于拷贝之后（此时 T_dataHash 已被算出），而实测 T_dataHash=0。
- 不是 class 实例问题：传参形态①（`new P013FrameInput()` 类实例）、形态②（纯对象字面量 + `as P013FrameInput`）**均被拒**。

**native 侧的三项严格判据**（`napi_bridge.cpp`）：`ParseFrame` 用 `napi_typeof(...)==napi_number` 逐字段读 20 个数值 + 2 个布尔；`ParseLight` 同法读 10 个数值；`slopeKernelVersion` 必须是字符串 `"E2_APROX_P0"`（实际常量 `E2_APPROX_P0`）；tdata 必须是**长度恰为 512 的 Float32Array**（`kFloats=512`）且非 detached。

**建议**（任一即可解冻）：
1. native 侧把拒因细分（把 `parsed/typed/version` 三个布尔分别落日志或并入 `errorStage`），一次即可定位；当前 `errorStage` 只有 `FRAME_REJECTED/DIAG_RANGE` 两档，无法区分。
2. ArkTS 侧改为严格 plain object（`JSON.parse(JSON.stringify(...))` 兜底）+ 逐字段自检（本地断言 22 个字段类型），并把 512 长度 Float32Array 的 `byteLength/byteOffset` 一并自检。
3. 同一问题**影响生产路径**：`P013RenderController.ets:35` 的 `private native: P013NativeRenderer = p013renderer as P013NativeRenderer;` 与本探针页初版同型（见 §4.2），生产路径需一并复核。

### 4.2 【平台约束·高】napi 绑定加载时序：必须在 XComponent 创建之后才触碰 .so 绑定

**现象**：探针页初版用**字段初始化器**触碰 `p013renderer` 绑定 → .so 在 XComponent 创建之前被 dlopen → napi `Init` 执行时 `OH_NATIVE_XCOMPONENT_OBJ` 尚不存在 → native `Hook` 早退（`if (!has) return;`）→ **回调从未注册**，native 永远收不到 window：`state` 停在 `WAIT_WINDOW`、`widthPx=0`、surface 全黑并盖住下层 Canvas。

**修正**：把绑定取用移到 `XComponent.onLoad` 之后（本探针页已改，`P013P1G0Probe.ets` 的 `nativeRef`/`helloRef` 注释记录了原因）。修正后同一构建即得 `state=WINDOW_AVAILABLE`、`ownerBound=true`。

**P0a 为何幸免**：`P013HelloSurface.ets` 首次触碰 `p013hello` 绑定是在 `tickFrame()` 内（XComponent 已创建之后），故 Hook 成功。

**对生产的提示**：`P013RenderController.ets:35` 用的是字段初始化器，与失败形态同型——**生产路径需按同样方式复核**（把 native 绑定的首次取用推迟到 XComponent `onLoad` 之后，或确认 `P013RenderController` 的构造时机晚于 XComponent 创建）。

### 4.3 【平台约束·中】ArkUI 页面路由表：新增页必须登记

未在 `entry/src/main/resources/base/profile/main_pages.json` 登记时，`loadContent('pages/P013P1G0Probe')` 失败：
`SetUIContentInner: failed to init or restore uicontent with file pages/P013P1G0Probe. errorCode: 13` → `Parameter error`，页面不加载（表现为白屏/空页且无 ArkTS 日志）。本卡已在 `main_pages.json` 登记 `pages/P013P1G0Probe`。

### 4.4 【建议】显式声明 sRGB 色彩空间（勿依赖平台默认）

现状：`egl_context.cpp` 的 `eglChooseConfig` 属性表**不含** `EGL_GL_COLORSPACE_SRGB`，`eglCreateWindowSurface` 的属性参数为 `nullptr` → 帧缓冲色彩空间取平台默认。实测本设备下 GLES 输出与 Canvas sRGB 直写**一致**（diff=0），但这是默认行为带来的巧合而非显式约定；建议要么在 window surface 属性里请求 `EGL_GL_COLORSPACE_SRGB`，要么在 shader 末尾做 gamma 校正并在文档中登记"由 shader 承担色彩管理"。

### 4.5 【建议】`shake_lift` 类逐帧事件需节流

（承接 P-012 返修复测的移交项）P1 的遥测/事件若按 substep 逐条落日志，长时运行会有日志膨胀风险，建议按帧聚合并落摘要。

## 五、红线与遗留

- 本卡对主干代码的改动仅限**探针脚手架**（探针页 + want 门 + 路由表登记），已用 `PROBE-ONLY` 注释标注，**严禁并入发布分支**；未触碰 developer 的 native 代码与生产渲染路径。
- P-013 native renderer 的 shader（`e2_lmat.frag/vert`）与 `render_contract.h` 的校验口径本卡**只读引用**，未改。
- 探针 ④ 透明与 renderer 侧的 surface 计数交叉校验，待 §4.1 修复后复采（本卡已备好 `p013g0lib=renderer` + `p013g0diag` 参数与复采脚本）。
- 采集密度限制：hdc `snapshot_display` + `file recv` 单帧约 1–2.7s，截图为采样而非连续视频；本卡结论均以**像素采样 + 遥测计数**为准，不以帧连续性推断。
- 滴答清单 403，任务 6add6012e4b06ee35723830b 无法回写，本地归档。

## 六、产物索引

目录 `test/artifacts/p013-p1g0-probe-20261025/`：

- 探针截图：`P1-hello-hitnone.jpeg`、`P1-hello-hitnone-after-swipe.jpeg`、`P2-hello-hitdefault-after-swipe.jpeg`、`P3-hello-hittransparent-after-swipe.jpeg`、`P4-hello-surface10bg.jpeg`、`P4b-hello-after6bg.jpeg`、`P5-renderer-diagon.jpeg`、`P5-renderer-diagon-after-swipe.jpeg`
- 探针日志：`P1-hello-hitnone-g0.txt`、`P2-hello-hitdefault-g0.txt`、`P3-hello-hittransparent-g0.txt`、`P4-hello-surface10bg-g0.txt`、`P4b-surface-cycles.txt`（逐轮 dump）、`P5-renderer-diagon-g0.txt`
- 构建产物：`entry-default-s139h-signed.hap`（及 s139/s139b..s139g 中间构建，保留 s139h）
- 工具：`tools/p013_p1g0_probe.ps1`（五项主采集）、`tools/p013_p1g0_surface_cycles.ps1`（surface 逐轮 dump）、`tools/analyze_p013_p1g0.py`（像素/遥测分析）
- 探针代码：`entry/src/main/ets/pages/P013P1G0Probe.ets`、`EntryAbility.ets`（探针门）、`entry/src/main/resources/base/profile/main_pages.json`（路由登记）
