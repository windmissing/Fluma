# T80：H1 目标链常量 shader 矩阵补证 v1

日期：2026-10-26。角色：tester。任务卡：T80（材质色彩客观补证）。依据：`docs/analysis-T77-material-color-v1.md` §6.1（H1 目标链最小矩阵）。执行器：`test/p013_native/t80_target_matrix_test.cpp` + `test/p013_native/build_t80.cmd` + 补充导出 `test/p013_native/t80_libEGL.def` / `t80_libGLESv2.def` + `mk_t80_libs.cmd`。证据：`test/reports/t80/h1-target-matrix.json`（canonical）+ `t80-target-matrix.json`（原始矩阵）+ `environment.txt`（EGL 版本/扩展）+ 三个 target 的 raw RGBA。独立不可变目录 `.codex/local-test/t80-matrix/`，未覆写 T55 任何历史。

## 1. 环境（如实记录）

ANGLE 2.1.18846（git 4a65a669e11b），GLES 3.0.0，GL_RENDERER = ANGLE (Intel, Intel(R) UHD Graphics 630 Direct3D11 vs_5_0 ps_5_0, D3D11-31.0.101.2140)。与 T55 同宿主。**EGL_KHR_gl_colorspace 不在 display 扩展列表**；**GL_EXT_sRGB_write_control 不在 GL 扩展列表**。

## 2. 矩阵结果（常量 shader，同进程，逐 target 独立 program）

shader 输出线性 L（uniform uL），六个 L 值：0、0.0031308（sRGB 折点）、0.18、0.5、1.0、OETF(145/255)=0.215760（C 侧 float32 计算，作为第 6 个常量线性光）。

| target | attachment/query 实况 | L=0 | knee | 0.18 | 0.5 | 1.0 | L6 |
|---|---|---:|---:|---:|---:|---:|---:|
| T1 default pbuffer（未请求 colorspace） | eglQuerySurface colorspace=0x308A(12426)；GL_BACK impl read format=0x32993 | 0 | 1 | 46 | 128 | 255 | 55 |
| T2 sRGB pbuffer | **SKIP：EGL_KHR_gl_colorspace 缺失，无法创建** | — | — | — | — | — | — |
| T3 RGBA8 FBO | attachment0=LINEAR，FBO complete | 0 | 1 | 46 | 128 | 255 | 55 |
| T4 SRGB8_ALPHA8 FBO | attachment0=SRGB，FBO complete | 0 | 10 | 118 | 187 | 255 | 128 |

线性读回与 255·L 精确一致（0.18→45.9→46；0.5→127.5→128；L6→55.0→55；knee→0.798→1）。T4 与 255·OETF(L) 一致，**唯一例外**：L=0.5 理论 round-to-nearest 为 188（187.516），ANGLE 实测 187（截断）——按 T77 §5 口径记为实现取整行为，不冻结容差。

## 3. 归因（不越权）

1. **H1 在常量 shader 层成立**：本宿主 default pbuffer 的 fragment 写入→存储→ReadPixels 链读回**线性码**（非 sRGB 显示码），且与显式 RGBA8 FBO 逐值相同；SRGB8_ALPHA8 FBO 上硬件 OETF 正确激活。T55 的 73.72 码主差异与本矩阵 T1 的码域特征一致。
2. **T55 的 sRGB pbuffer 请求静默降级**：该 ANGLE 版本 display 扩展无 EGL_KHR_gl_colorspace，T55 传入的 `EGL_GL_COLORSPACE_SRGB_KHR` 属性不被支持；T1 surface query 返回 0x308A。执行器确实请求了 sRGB 但宿主无法兑现——这是**能力缺失**，不是 product 漏调用。
3. **write-control**：GL_EXT_sRGB_write_control 缺失，产品的条件 enable 分支（仅在扩展存在时 enable 0x8DB9）符合 GLES 合同，本证据不触发 §7 的"被错误关闭"hunk。
4. **取整差异观察**：T4 在 L=0.5 处 187 vs 188，属实现 float→unorm 转换行为，与 T77 §5 的 baseByte 取整分叉同类但独立成点；**不用于解释 1.956 码残差**，不新开配方返修。

## 4. 边界与未决项

- 离屏 pbuffer/FBO，无 eglSwapBuffers、无呈现证明；**不得推断设备（Harmony XComponent）色彩正确或错误**。
- T2 在本宿主不可得 → sRGB 请求域的 default-pbuffer 对照在设备上仍需 T57 A/B/C 采集。
- §6.1 phase 2（fractional-alpha 合成探针）与 §6.2（(88,78) F64/F32 baseByte 探针）未跑，属独立后续；本卡只交付 §6.1 phase 1 矩阵。
- 按 §7 路由：本证据支持"记录该 backend 能力未通过（default 域线性读回 + 无 colorspace 扩展）"；**不授权**任何 shader OETF/ gamma 修复，不以 FBO 替 default 签 PASS。
- 未改产品/shader/配方/主测阈值；未用真机；无 Canvas。

## 5. 执行器自身修正（非判据变更）

- program 必须在**每 context** makeCurrent 后创建（跨 context 不共享 → T3/T4 首轮全零的根因）；
- FBO-only 路径必须显式 `glViewport(0,0,64,64)`（无 surface 的默认 viewport 未定义）；
- def 文件需补 6 个导出（glGetIntegerv/glGetProgramInfoLog/glTexImage2D/glDeleteFramebuffers/eglGetCurrentSurface/eglQuerySurface）——补充 def 与重建 lib 均入 git 可复现。
