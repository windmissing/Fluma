# P-013 P1-G0 补证复测回执（T35）

日期：2026-10-25（Asia/Shanghai）。责任：effector。任务卡：6add77dfe4b06ee35724fd95。标签：渲染线·阶段二·effector·真机。
构建：`entry-default-s140-signed.hap`（4,030,811 B；源码树指纹 4685c9a7，构建前后一致；hvigor 编译门 GREEN，SignHap 00303116 为已知失败点、手动签名）。
设备：VYG-AL00（API 26 / OpenHarmony 7.0）。页面画布 365.7×742.3 vp → 1280×2598 px（vp2px=3.5）。
复测对象：T32（042ba21 绑定推迟+拒绝分阶段）+ T34（2ef21f9 TEXTURE 合成 profile/显式 sRGB/读回）在稳定工作树上的设备行为。脚本 `tools/p013_p1g0_retest.ps1`（R0-R7 八臂），判读 `tools/analyze_p013_p1g0_retest.py`。
**总览：五项未闭环。③ z 序在 TEXTURE 路径获实质推进（T29 FAIL → 条件 PASS）；①帧提交为新 P1 阻塞（15,940/15,940 拒绝，阶段 TDATA_LENGTH）；②④ 依赖①。G0 仍不过，T23 不放行。**

## 一、五项逐项结论

| 项 | 结论 | 一句话依据 |
|---|---|---|
| ① 帧提交 | **FAIL（P1 阻塞，阶段前移）** | R1-R7 共 15,940 次 draw_error 全部 code=100，errorStage=TDATA_LENGTH，T_dataHash=0，swap=false |
| ② sRGB | C 层一致 / A、B 层 **UNPROVEN** | hello 内容 authored (46,51,56)→屏上 (46,51,55)；合成收据 actualColorSpace/attachmentEncoding=UNPROVEN（EGL 表面未建） |
| ③ 三层 z 序 | **PASS（TEXTURE 路径，含渲染内容）** | R0：hello 全屏灰之上 fg 洋红三标记完整可见；renderer 臂无内容不可判 |
| ④ 透明 | 条件性 PASS（空纹理）/ 二值 mask **UNPROVEN** | 空 TEXTURE 区棋盘透出；盒内 alpha=255 未验证（帧全拒） |
| ⑤ 输入透传 | **PASS（须显式配置）**，TEXTURE 下 Default 不再吞触摸 | none/transparent 均透传；default 3 次 Down（T29 SURFACE 下为 0） |

## 二、①帧提交：拒绝阶段定位（本轮唯一新阻塞级发现）

T32 修复已生效的部分（相对 T29 的进步）：

- `p013_g0_init` code=0，state=WINDOW_AVAILABLE；native OnSurfaceCreated 收到；XComponent onLoad → native 挂接成功（T29 时绑定早退导致 init 即失败）。
- `requestComposition` code=0（profile 五字段+probeOnly 校验通过），`p013_g0_composition` 收据落出。

仍未解决（P1 阻塞）：

- R1-R7 每个 drawFrame 均被拒：draw_error 15,940 次，码分布 {100: 100%}；遥测 64/64 条 errorStage=TDATA_LENGTH；T_dataHash 恒 0（拒绝发生在拷贝前）；swapSucceeded 恒 false；surfaceCreateCount/contextCreateCount 恒 0（EGL 表面从未创建——拒绝先于 EnsureCurrent）。
- 根因定位：ArkTS 探针页与生产路径（P013RenderController.submit → P013FrameAdapter.data）均传 `Float32Array(128*4=512)`，与 native `kFloats=512`（render_contract.h）一致；native 判型链显示 a[1] 确是 float32 typed array（通过 TDATA_TYPE），仅长度不符。宿主 C++ 测试（`test/p013_native/bridge_host_test.cpp`，view->length=512）通过 → 非 native 校验逻辑错误，指向**设备上 ArkTS→napi typed array 迁移后的实际长度与 512 不符**（或该校验对设备迁移形态的假设错误）。
- 影响面：探针与生产两条路径同构（均 512），GLES 诊断/生产 overlay 均不可用；②A/B 层、④二值 mask、③含内容 z 序全部依赖帧提交，无法闭环。
- 移交 T32/T34：在 TDATA_LENGTH 拒绝分支补 diagnostics（hilog 打实际 length/bytes/offset），确认设备迁移形态后修复；修复后本回执①-④项需重采（不叠加其他变更）。

## 三、②sRGB：三层证据现状（T33 §4.2 口径）

| 层 | 状态 | 数据 |
|---|---|---|
| A API/attachment | UNPROVEN | 收据：requestedColorSpace=SRGB、actualColorSpace=UNPROVEN、attachmentEncoding=UNPROVEN、eglAlphaSize=-1、srgbWriteControlSupported=false、profileStatus=PENDING、surfaceEpoch=2、contextEpoch=0（EGL 表面未创建，显式 colorspace 请求与 GL_BACK 编码查询未执行） |
| B GL 读回 | UNPROVEN | readbackStage=PRE_SWAP_DEFAULT_BACK、readbackStatus=UNPROVEN；captureProbe 无数据（swap 从未成功） |
| C compositor/截图 | 一致（hello 内容） | R0：authored 8bit (46,51,56) → 屏上 (46,51,55)，±1/通道（JPEG 量化内）；与 T29 结论一致：当前内容无线性/sRGB 错配 |

按 T33 §4.2：A 层不可得时写 UNPROVEN，不用 C 层近似补 A/B。T34 的显式 sRGB 实现已在代码层就位（egl_context.cpp 请求 EGL_GL_COLORSPACE_SRGB_KHR、查询 actual colorspace 与 GL_BACK 编码、napi 序列化），但设备验证被①阻塞。

## 四、③三层 z 序：TEXTURE 路径实质推进

- **R0（libp013hello + TEXTURE）**：hello 全屏清屏灰（不透明 GLES 渲染内容，覆盖含 fg 标记路径的全部区域）之上，fg 洋红左竖条、中部横线（y≈0.48H，x 15%-85%，正穿沙盒区）、底条**完整可见**；状态文字亦在顶层。→ TEXTURE + zIndex(0/10/20/30) 的声明顺序在合成器生效，T29"XComponent SURFACE 恒在所有 ArkUI 内容之上"的结构性 FAIL 在 TEXTURE 路径**解除**。
- R1-R7（renderer）：GLES 区无任何渲染内容（帧全拒），"fg 在已渲染沙体之上"不可判定（UNPROVEN）；T33 §6 矩阵的完整三对照（原布局 SURFACE / 修正顺序 SURFACE / TEXTURE）需①修复后补采——R2 SURFACE 臂本轮同样处于无内容状态，不构成有效对照。
- SandCanvas 生产叠层（2ef21f9）：Canvas bg zIndex=0 → XComponent TEXTURE zIndex=10 → Canvas fg zIndex=20 → controls zIndex=30，与 R0 验证的结构同构。

## 五、④透明与 R5 同帧对照

- TEXTURE 空内容区：bg 棋盘（青 #00FFFF / 黄 #FFCC00）完整透出 → 平台对空 TEXTURE 的透明合成路径成立（T29"BLOCKED"的平台侧前提已解除）；XComponent 声明 `.opacity(1).backgroundColor(Color.Transparent)`。
- 二值沙 mask（盒内 alpha=255 不透明、盒外 RGBA=(0,0,0,0)）未验证：无帧渲染即无 mask 输出 → UNPROVEN。T33 §3.2 的"诊断开/关非沙 ROI 同帧相同"本轮只能证明 bg 层稳定（R1 与 R5 非沙区同为棋盘同格），不构成该契约证据（需 GLES 诊断内容）。
- R2（SURFACE）：非沙区呈 Stack 底 (240,240,240)——空 surface 透明路径与 TEXTURE 不同呈现，仅作记录。

## 六、⑤输入透传与生命周期

- 输入（每臂含 1 次 USB 弹窗关闭键 (240,620) 的页面 Down 基线）：R1 none=2（1 swipe+弹窗）、R3 transparent=3（2 swipe+弹窗）、**R4 default=3（2 swipe+弹窗）**、R5 none=1（弹窗）、R6 none=1（弹窗）、R7 none=7（6 swipe+弹窗）。
- **行为差异**：TEXTURE 下 hit=Default 不再吞触摸（T29 的 SURFACE 下 Default=0 次 Down）。生产 SandCanvas 对 XComponent 用 None（T29 已证必要且正确），不受影响；负对照语义变化登记供 tester/manager 知悉（TEXTURE 组件触摸竞技行为与 SURFACE 不同）。
- R6 前后台 10 次循环：无崩溃、无 ANR；遥测 contextLoss/stale/recovery 全 0（EGL 未初始化故无计数器跳动）；OnSurfaceCreated 正常记录，页面重建无异常。
- R7 动态 6 连拍：帧仍全拒，无动态错帧证据可采（T33 §6"动态同 token"行留待①修复后）。

## 七、产物索引（test/artifacts/p013-p1g0-retest-20261025/）

| 文件 | 内容 |
|---|---|
| run-console.log | 八臂逐步时间线（Log-Step 打点） |
| R0-hello-texture{,-after-swipe}.jpeg + R0-hello-texture-g0.txt | hello+TEXTURE：z 序含内容证据 + sRGB C 层 |
| R1-renderer-texture{,-after-swipe}.jpeg + R1-renderer-texture-g0.txt | TEXTURE 主臂：init/合成收据/15,940 拒绝子集 |
| R2-renderer-surface{,-after-swipe}.jpeg + R2-renderer-surface-g0.txt | SURFACE 对照臂 |
| R3/R4/R5 截图 + 日志 | 输入三档 / diag=off 同帧对照 |
| R6-renderer-texture-surface10bg.jpeg + 日志 | 前后台 10 次生命周期 |
| R7-renderer-texture-dyn-{1..6}.jpeg + 日志 | 动态连拍 |
| analysis.txt | 分析器完整判读输出 |

日志为 hilog UTF-16LE 原文；截图为 snapshot_display JPEG（有损，仅作结构判读；精确色值以 A/B 层为准，T33 §6 要求的主测无损 PNG 在①修复后补采）。

## 八、红线与遗留

- 未修改任何业务代码；未叠加其他修复混测；未宣布 G0 通过或 G1 解冻。
- 遗留（按依赖序）：P1-A 帧提交 TDATA_LENGTH（T32/T34，阻塞②④及③含内容证据）→ 修复后重采 R1/R2/R5/R7 → 无损 PNG 色彩/透明主测 → tester 登记读回容差 → manager 裁决 T23。
- T31 关联：T31 主观粒子验证仍 BLOCKED 于 T20 页面侧无 setParticleRoute 装配入口（见 docs/task-T31-subjective-particle-e2e-runbook-v1.md §2.1），与本回执无关。

变更记录：2026-10-25 v1，首回复测；构建 s140；八臂真机采集+判读完成。
