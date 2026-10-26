# T56 developer 回执：G4 完整新管线场景

2026-10-26。开发交付，状态：结束待验收。未使用真机。

## 实现

- 原生场景 compositor 以同一个 FrameToken 组织背景、沙区、粒子、材质和前景，完成后交换，再提交同代场景租约；失败给出 FAULT 或明确标记 STALE 的上一有效帧。
- GL_SRGB8_ALPHA8 全帧双缓冲目标和场景 shader 路径接入 XComponent。页面不挂 Canvas；非双场模式明确报不支持场景故障，异步液体回调也不会触发旧 Canvas。
- 场景准备、配置绑定和源发布使用同代身份；高度场独立路径和双场路径均可发布，源数组保持只读。

## 离线验证

- `node test/run_t56_scene_units.cjs`：5/5 PASS。
- `test/run_t56_scene_core.cmd`：PASS，覆盖层顺序、首帧故障、八种 pass/present 故障、STALE、交换原子性、尺寸及 context 失效、跨代配置和租约守卫。
- `test/run_t56_scene_bridge.cmd`：PASS，包含真实 NAPI 桥入参、身份及故障路径。
- `test/run_t56_scene_gpu.ps1`：本机 ANGLE/OpenGL ES 3.0 离屏 PASS，覆盖 sRGB 全帧、材质 OFF/ON、交换后租约提交和 EGL context 重建。
- hvigor `CompileArkTS`、`BuildNativeWithNinja`、`PackageHap`：BUILD SUCCESSFUL，零错误。

上述为开发自验；设备显示、色彩同一性、真实帧时和端到端触摸呈现仍由既定 tester/effector 任务验证，不作为本卡开发交付结论。
