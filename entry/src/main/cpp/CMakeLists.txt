# P-013 P0a hello-surface（首个 native 模块——工具链踩坑记录见 docs/p013-p0a-toolchain-notes-v1.md）
# 问题 ID：P-013；方案版本：proposal-P013-incorporation-v1 §2.1
cmake_minimum_required(VERSION 3.16)
project(helloworld_p013)

set(CMAKE_CXX_STANDARD 17)

# ohos.toolchain.cmake 由 hvigor 通过 -DCMAKE_TOOLCHAIN_FILE 注入
# （DevEco 标准流程：build-profile.json5 externalNativeOptions.path 指向本文件）

add_library(p013hello SHARED
  hello/hello_surface.cpp
  p013renderer/egl_context.cpp
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
# P-013 v1: embed exact shader bytes at configure time, with source SHA256.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/e2_lmat.vert"
 "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/e2_lmat.frag")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/e2_lmat.vert" P013_VERT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/e2_lmat.frag" P013_FRAG)
string(SHA256 P013_HASH "${P013_VERT}${P013_FRAG}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/p013_shader_sources.h"
 "#pragma once\nstatic constexpr const char* kP013Vertex=R\"P013(${P013_VERT})P013\";\nstatic constexpr const char* kP013Fragment=R\"P013(${P013_FRAG})P013\";\nstatic constexpr const char* kP013ShaderHash=\"${P013_HASH}\";\n")
# T55 G3: embed the same T24 helper bytes; P0 shader identity is unchanged.
set(P013_MATERIAL_HELPERS "${CMAKE_CURRENT_SOURCE_DIR}/../ets/render/preparation/p1_material_functions.glsl")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/p1_material.vert"
 "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/p1_material.frag" "${P013_MATERIAL_HELPERS}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/p1_material.vert" P013_MATERIAL_VERT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/p1_material.frag" P013_MATERIAL_FRAG)
file(READ "${P013_MATERIAL_HELPERS}" P013_MATERIAL_FUNCTIONS)
string(REPLACE "/* P013_MATERIAL_FUNCTIONS_V1 */" "${P013_MATERIAL_FUNCTIONS}" P013_MATERIAL_FRAG "${P013_MATERIAL_FRAG}")
string(REPLACE "\r\n" "\n" P013_MATERIAL_VERT "${P013_MATERIAL_VERT}")
string(REPLACE "\r\n" "\n" P013_MATERIAL_FRAG "${P013_MATERIAL_FRAG}")
string(SHA256 P013_MATERIAL_HASH "${P013_MATERIAL_VERT}${P013_MATERIAL_FRAG}")
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/p013_shader_sources.h"
 "static constexpr const char* kP013MaterialVertex=R\"P013(${P013_MATERIAL_VERT})P013\";\nstatic constexpr const char* kP013MaterialFragment=R\"P013(${P013_MATERIAL_FRAG})P013\";\nstatic constexpr const char* kP013MaterialShaderHash=\"${P013_MATERIAL_HASH}\";\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/clear_water_v3.frag")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/p013renderer/glsl/clear_water_v3.frag" CW_FRAG)
string(SHA256 CW_HASH "${CW_FRAG}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/clear_water_shaders.h" "#pragma once\nstatic constexpr const char* kClearWaterFragment=R\"CW(${CW_FRAG})CW\";\nstatic constexpr const char* kClearWaterShaderHash=\"${CW_HASH}\";\n")
set_source_files_properties(p013renderer/clear_water_packet.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
add_library(p013renderer SHARED p013renderer/clear_water_packet.cpp p013renderer/clear_water_gpu.cpp p013renderer/scene_compositor.cpp p013renderer/scene_gpu.cpp p013renderer/fullframe_targets.cpp p013renderer/scene_layers.cpp p013renderer/material_pass.cpp p013renderer/material_uploader.cpp p013renderer/base_strip_uploader.cpp p013renderer/dualfield_pass.cpp p013renderer/composition_probe.cpp p013renderer/napi_bridge.cpp p013renderer/egl_context.cpp
 p013renderer/shader_pass.cpp p013renderer/tdata_uploader.cpp)
target_include_directories(p013renderer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(p013renderer PUBLIC libace_napi.z.so libace_ndk.z.so libEGL.so libGLESv3.so libhilog_ndk.z.so)

# T236 isolated integer/control foundation, not a production routing switch.
add_subdirectory(gpu_resident)
target_link_libraries(p013renderer PRIVATE grs_foundation)
