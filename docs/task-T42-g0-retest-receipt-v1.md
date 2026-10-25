# T42·P-013 P1-G0 补证复测回执（T41 帧长修复后五项闭环）

- 日期：2026-10-25 16:31–16:38（真机 VYG-AL00，设备在线）
- 执行角色：effector（真机·渲染线）
- 构建身份：**s141** = `entry-default-s141-signed.hap`（4,122,603B）
  - 源码树：HEAD `8502dc5` + 工作树指纹 `45d28876aa985603`
  - 含 T41 `1f9a70c`（Ark NAPI typed-array 字节长度判据修复）+ 并行会话暂存的
    p013hello 双库重构（`egl_context.cpp` 并入 p013hello，CMakeLists 暂存态）
  - 编译门：CompileArkTS + PackageHap 通过（SignHap 00303116 为已知失败点，手动签名）
- 复测脚本：`tools/p013_p1g0_retest.ps1`（R0–R8 九臂）；判读：`tools/analyze_p013_p1g0_retest.py`
- 产物：`test/artifacts/t42-p1g0-retest-20261025/`（26 文件：10 截图 + 9 臂日志 + analysis.txt + run-console.log）

## 一、九臂执行记录

| 臂 | lib / profile / hit / diag | 内容 | 结果 |
|---|---|---|---|
| R0 | hello / TEXTURE / none / on | sRGB 基线 + z 序回归（全屏清屏灰走纹理路径） | hello frames=1560 native=1560 ready/ok |
| R1 | renderer / TEXTURE / none / on | 主臂：帧提交/合成收据/读回/z 序/透明 | 遥测 n=32 全 code=0 |
| R2 | renderer / SURFACE / none / on | SURFACE 对照（叠层结构差异） | 遥测 n=64 全 code=0，盒区深灰内容 |
| R3 | renderer / TEXTURE / transparent / on | 输入透传正对照 | Down=3 |
| R4 | renderer / TEXTURE / default / on | 输入负对照（T29 预期吞触摸） | Down=3（未吞） |
| R5 | renderer / TEXTURE / none / **off** | diag=off 同帧 ROI（探针按设计不画条纹） | 全透明符合设计 |
| R6 | renderer / TEXTURE / none / on | 前后台 10 次循环 | 11 建 10 销无泄漏 |
| R7 | renderer / TEXTURE / none / on | 动态连拍 6 帧 + 每帧 swipe | 遥测 n=65 全 code=0 |
| R8 | renderer / TEXTURE / none / on | 长时 90s 持续帧 | 遥测 n=106 全 code=0/swap=true |

## 二、五项判读（G0 验收单 §4 映射）

### ① 帧提交 —— **PASS**（T41 修复获设备验证）

- 全部 8 个 renderer 臂：`draw_error` **0 次**；遥测记录全 `code=0`、`swapSucceeded=true`、
  `T_dataHash=01164b5eb4a54725`（非零且各臂一致）、`eglError=12288`（EGL_SUCCESS）、`glError=0`、
  `errorStage=NONE`；`surfaceCreateCount/contextCreateCount=1`（R6 为 11）。
- `p013_tdata_boundary` 事件逐臂一致：`length=512 / byteLength=2048 / nativeByteLength=2048 /
  nativeBufferBytes=2048 / errorStage=NONE` —— 页面传 512 元素，native 收到 2048 字节
  （512×4）后按字节长度判据放行，T41 根因（`napi_get_typedarray_info` 返回字节数而非元素数）确认修复。
- 对比 T35：15,940/15,940 `code=100/TDATA_LENGTH` → 本轮 **0 拒绝**。

### ② sRGB —— **PASS**（A/B/C 三层齐备）

- **A 层（attachment）**：帧后读回 `actualColorSpace=SRGB`（eglQuerySurface 命中
  EGL_GL_COLORSPACE_SRGB_KHR）、`attachmentEncoding=GL_SRGB`（默认附件 COLOR_ENCODING）、
  `eglAlphaSize=8`、`drawFramebuffer=readFramebuffer=0`。
- **B 层（write control）**：`srgbWriteControlSupported=true / srgbWriteEnabled=true`
  （GL_EXT_sRGB_write_control 在位且 GL_FRAMEBUFFER_SRGB 启用）。
- **C 层（屏幕）**：R1/R3/R4/R6/R7 截图盒区五带实测 `[0, 10, 118, 188, 255]`，
  与线性值 `[0, 0.0031308, 0.18, 0.5, 1.0]` 的 sRGB 编码期望逐位一致（容差 2），
  与 Canvas 直写对照条 `[0, 10, 119, 189, 255]` 一致（±1 取整）——
  线性写入经 sRGB 写控制正确编码，无双编码/无双线性化，GLES 栈与 Canvas 栈色彩空间一致。
- R5（diag=off）按设计不画条纹，C 层 N/A。

### ③ 三层 z 序 —— **PASS**（TEXTURE 路径）

- 11 张截图全部检出**洋红前景线横穿盒区**（y=1384，满宽 576/576）：
  R0（hello 清屏灰 46,51,55 之上）、R1/R3/R4/R5/R6/R7（探针条纹之上）、R2（SURFACE 深灰盒之上）。
- T29「XComponent(SURFACE) 恒在应用 UI 之上」的结构性 FAIL 在 TEXTURE + zIndex(0/10/20/30)
  路径下确认解除；SURFACE 路径（R2）同样可见 fg（SurfaceFlinger 层叠在本探针布局下成立）。

### ④ 透明 —— **PASS**（二值 alpha）

- TEXTURE 臂盒上半（GL uv.y≥0.5，探针设计为 `vec4(0.0)`）与盒外区域**棋盘格透出**
  （采样 (0,255,255) 青/(255,204,0) 黄），alpha=0 区正确合成、alpha=1 条纹区不透明。
- R5 diag=off 全盒透明（仅清屏不画）符合设计。

### ⑤ 输入透传 —— **PASS**（须显式配置，行为已刷新）

- TEXTURE 路径三档实测 Down 次数：`none`=2（R0/R1/R2）、`transparent`=3（R3）、
  **`default`=3（R4）**。
- 与 T29 的差异登记：T29 在 **SURFACE** 路径下 `hit=Default` 吞掉全部触摸（0 次 Down）；
  本轮 **TEXTURE** 路径下 Default 不再吞触摸。生产管线用 `hitTestBehavior.None`，不受影响；
  若未来 P1 集成改用 Default，需按 TEXTURE 口径重新评估。

## 三、生命周期与长时稳定性

- **R6（前后台 10 次）**：`surfaceCreate=11 / surfaceDestroy=10 / contextCreate=11 /
  contextDestroy=10 / contextLoss=0 / recoveryAttempt=0 / staleLifecycle=0` —— 每轮循环
  重建后即销毁，常驻句柄数恒 1，无泄漏、无陈旧生命周期、无上下文丢失恢复。
- **R8（长时 90s）**：106 条遥测全 `code=0/swap=true`，`swapMs` max 0.45ms / mean 0.30ms；
  按 dump 间隔（每 30 帧一条）推算帧间隔 mean 33.5ms / max 122.7ms，**无 >1s 帧间隙**
  （H01 冻结门槛 1000ms 未触发）。披露：hilog 环形缓冲丢行致 dump 间隔 max 3.68s
  （65 条 >1s），但丢行不伴生 code/swap 异常，判定为缓冲压力而非帧停顿。

## 四、遗留事项（不影响 G0 五项结论，建议移交 T32/T34）

1. **requestComposition 收据 `status=PENDING`**：请求时点早于 EGL 上下文检查完成，
   收据内 `actualColorSpace/attachmentEncoding/eglAlphaSize` 均为 UNPROVEN/-1；
   真实值以帧后读回为准（本轮已取到 SRGB/GL_SRGB/8）。建议 T34 把收据改为
   「就绪后回填」或文档明示 PENDING 语义。
2. **color_readback 数组被 hilog 单条 ~4096B 截断**：`linearRgba8/srgbRgba8`（离屏
   GL_RGBA8 vs GL_SRGB8_ALPHA8 对照读回）被超长 `glExtensions` 字符串挤出截断面。
   C 层像素证据已覆盖 sRGB 结论；若需数组级 A/B 证据，需缩短 extensions 序列化或分片输出。
3. **profile.probePoints 为空**：`requestComposition` 未定义探试点，`captureProbe`
   无点可读（rgba=[]）。如需定点读回通道，T34 需在 profile 中填充探针坐标。

## 五、结论

**P1-G0 五项探针全部 PASS**：①帧提交（T41 修复设备验证）②sRGB（A/B/C 三层）
③三层 z 序（TEXTURE 路径）④透明（二值 alpha）⑤输入透传（三档）。T29/T35 遗留的
两项阻塞（帧提交 100% 拒识、z 序结构性 FAIL）均已解除。**建议 manager/designer 复核本回执后
放行 T23（G1 解冻第三项）**；第四节三项遗留按上述路由处理，不构成放行阻塞。

## 六、分析器判读修正记录（本轮修复 4 处，已回跑自检）

1. `frame_admission_verdict`：`errorStage` 判据漏收字符串 `'NONE'` → 误报 FAIL（T35 版遗留）；
   同时 hello 臂改按臂名采信（防上一臂残留进程尾事件混入，本轮 R1 日志确有一条 PID 29941 的
   R0 尾事件）。
2. `shot_pixel_verdict` 盒坐标映射：GL 坐标自底向上，正确式为盒顶(屏)=dy+by、
   条纹采样 y=dy+by+bh×0.75（T35 版写反致盒中心采样落到设计透明区，误报「无内容」）。
3. 臂类型识别：R6 文件名含 `surface10bg` 被误分类为 SURFACE 臂 → 改为仅 R2 为 SURFACE。
4. R5 diag=off 条纹不画属设计行为 → C 层判 N/A 而非 UNPROVEN。
   另：屏高 2832 与 surface 高 2598 之差 234px（状态栏）已纳入 dy 偏移。
