# P-013 P0a hello-surface（首个 native 模块——工具链踩坑记录见 docs/p013-p0a-toolchain-notes-v1.md）
# 问题 ID：P-013；方案版本：proposal-P013-incorporation-v1 §2.1
cmake_minimum_required(VERSION 3.16)
project(helloworld_p013)

set(CMAKE_CXX_STANDARD 17)

# ohos.toolchain.cmake 由 hvigor 通过 -DCMAKE_TOOLCHAIN_FILE 注入
# （DevEco 标准流程：build-profile.json5 externalNativeOptions.path 指向本文件）

add_library(p013hello SHARED
  hello/hello_surface.cpp
)

target_link_libraries(p013hello PUBLIC
  libace_napi.z.so     # napi 桥
  libace_ndk.z.so      # OH_NativeXComponent（sysroot 实名；旧文档写 libace_napi_arkui 不存在）
  libEGL.so            # EGL（§2.2 初始化链）
  libGLESv3.so         # GLES3（清屏+viewport）
  libhilog_ndk.z.so    # hilog 埋点（验收②：EGL 错误确认）
)

target_include_directories(p013hello PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hello
)
