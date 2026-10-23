# P-013 P0a hello-surface 工具链踩坑记录 v1

任务：6adb2dcee4b0e6ce8294ca07（P0a 首个 native 模块——工具链全通验证）。
日期：2026-10-23。构建产物：`entry-default-s135-signed.hap`（含
`libs/arm64-v8a/libp013hello.so` 12952B + `libc++_shared.so`）。

本工程此前零 native 模块先例——本文是 P0 主体的路标（立项书 §2.0 验收④）。

## 1. 工具链版本与环境

| 组件 | 路径/版本 |
|---|---|
| NDK | `C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/`（oh-uni-package） |
| ohos.toolchain.cmake | `native/build/cmake/ohos.toolchain.cmake`（hvigor 自动注入） |
| cmake/ninja | `native/build-tools/cmake/bin/`（cmake.exe + ninja.exe——Windows 下 ninja 不在 PATH，独立跑 CMake 须 `-DCMAKE_MAKE_PROGRAM` 显式指） |
| clang | `native/llvm/bin/clang++.exe`（--target=aarch64-linux-ohos --sysroot=native/sysroot） |
| hvigor | DevEco Studio 自带（既有构建流程 + externalNativeOptions 零额外参数） |

## 2. hvigor 集成（build-profile.json5）

```json5
"buildOption": {
  "externalNativeOptions": {
    "path": "./src/main/cpp/CMakeLists.txt",
    "arguments": "",
    "cppFlags": "",
    "abiFilters": ["arm64-v8a"]
  }
}
```

- **无需任何其它配置**：toolchain/ndk 路径 hvigor 从 `DEVECO_SDK_HOME` 自动解析
- native 编译产物自动进 HAP `libs/arm64-v8a/`（含自动追加 `libc++_shared.so`）

## 3. CMakeLists 链接库名（★坑 1）

立项书 §2.1 写 `libace_napi_arkui`——**sysroot 里不存在**。实际：

| 文档名 | sysroot 实名 | 用途 |
|---|---|---|
| libace_napi_arkui.z.so | **`libace_ndk.z.so`** | OH_NativeXComponent |
| libace_napi.z.so | ✓ 同名 | napi 桥 |
| libEGL.so / libGLESv3.so | ✓ 同名 | EGL/GLES3 |
| libhilog_ndk.z.so | ✓ 同名 | OH_LOG_Print |

## 4. cpp 代码 API 坑（★坑 2-4）

1. **XComponent 属性宏**：`OH_NATIVE_XCOMPONENT_OBJ`（不是 `OH_XCOMPONENT_OBJ`）
   ——定义在 `native_interface_xcomponent.h:59`，值为 `"__NATIVE_XCOMPONENT_OBJ__"`。
   用法：`napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &v)` +
   `napi_unwrap` 取 `OH_NativeXComponent*`，再 `RegisterCallback`。
2. **`OH_NativeXComponent_GetXComponentSize` 签名**：第 2/3 参是
   `uint64_t*`（不是 uint32_t）；hilog 格式串配套 `%{public}llu` +
   `static_cast<unsigned long long>`。
3. **`eglCreateWindowSurface` 第三参**：OpenHarmony EGL 头的
   `EGLNativeWindowType` = `unsigned long`——`NativeWindow*`（不完整类型）不能
   隐式转换，必须 `reinterpret_cast<EGLNativeWindowType>(window)`。

## 5. ArkTS 侧

- 页面 import：`import p013hello from 'libp013hello.so'`——napi 未验证
  warning（可忽略；SDK 后续版本会要求 .d.ts，届时补声明文件）
- **`() => void` 不能传 postFrameCallback**（ArkTS 严格模式）：须
  `class X extends FrameCallback { onFrame(t) {...} }`（SandCanvas 同模式）
- so 模块调用前显式接口声明（`declare interface` + `as`）避开 any/unknown

## 6. 构建链验证结果（验收①）

| 步骤 | 结果 |
|---|---|
| CMake 独立验证（bypass hvigor） | ✓ libp013hello.so（AArch64 DYN） |
| hvigor 整链（native+ArkTS） | ✓ 0 错误（native warning 已清零） |
| HAP 内容 | ✓ libs/arm64-v8a/libp013hello.so |
| 签名 | ✓ 手动 hap-sign-tool（既有流程，s135） |

**已知既有坑复现**：SignHap 的 00303116 密码长度校验（<32 位拒）依旧——
手动签名链照旧（dev-cert-chain.pem + OpenHarmony.p12）。

## 7. 并行会话干扰的处理（协作记录）

P-011 S3 湿度会话同仓并行（WaterSandField/P011Moisture 中间态未提交）。
本任务验证构建/单测时用 `git stash push -- <精确文件列表>` + 移走未跟踪的
P011Moisture.ets 隔离——**只动他们的文件，验证完立即恢复**（stash pop+mv 回）。
隔离态下：hvigor 0 错误、render 单测 74/74。恢复后 P011 中间态自身导致的
单测失败（MotionCache null 视图）与 P0a 无关（他们的 WaterSandField 改动
破坏他们的链，归他们收尾）。

## 8. 待真机验收（②③④——设备空闲后 effector 执行）

- ② `aa start` 带 `--ps uri` 路由到 `pages/P013HelloSurface`？——当前
  want 参数路由查 Index 转发；测试页需经 UI 导航进入（或临时加 want 参数，
  P0a 不动 Index 转发逻辑，effector 手动点进）
- ② hilog 关键词：`P013Hello`（OnSurfaceCreated 尺寸/hello frame swap ok
  每 300 帧心跳/EGL 错误即 ERROR）
- ③ 前后台 10 次切换：`hidumper -s WindowManager` 或页面级观察；native
  内存：`hidumper -s mem` 进程 RSS 曲线
- ③ 旋转：设备方向锁定场景需开发者选项解锁旋转（viewport 跟随由
  OnSurfaceChanged 重查尺寸+下帧 viewport 更新保证）

## 9. P0 主体的路标（移交 P0 开工）

1. §2.1 骨架其余文件（napi_bridge/egl_context/shader_pass/tdata_uploader）
   按 §2.4/§2.5 填充——hello_surface.cpp 的 EGL 链可整体搬入 egl_context.cpp
2. feedTData 的 `napi_get_typedarray_info` 零拷贝（§2.4）
3. GLSL（§2.5）——GLES3 着色器编译链已在本次工具链验证覆盖（libGLESv3 链接通）
4. 独立诊断层（§2.3-c）：XComponent 位置/透明度参数化在 P0 页面接入时处理
