cmake_minimum_required(VERSION 3.21)
project(hello_world_pc_vulkan LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows x64 required")
endif()
set(G13_GLSLANG "" CACHE FILEPATH "glslang with SPIR-V validation")
set(VULKAN_HEADERS "" CACHE PATH "Directory containing vulkan/vulkan.h")
if(NOT EXISTS "${G13_GLSLANG}" OR NOT EXISTS "${VULKAN_HEADERS}/vulkan/vulkan.h")
  message(FATAL_ERROR "Set G13_GLSLANG and VULKAN_HEADERS")
endif()
# Use only Vulkan headers. An OHOS SDK sysroot also has libc headers that
# would otherwise shadow the Windows CRT in an MSVC build.
set(PC_VK_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/vk_include")
file(COPY "${VULKAN_HEADERS}/vulkan" DESTINATION "${PC_VK_INCLUDE}")
if(IS_DIRECTORY "${VULKAN_HEADERS}/vk_video")
  file(COPY "${VULKAN_HEADERS}/vk_video" DESTINATION "${PC_VK_INCLUDE}")
endif()
set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(GPU "${ROOT}/entry/src/main/cpp/gpu_resident")
set(GRS_G13_SPLIT ON)
include("${GPU}/g13flow/kernels.cmake")
foreach(tool "${ROOT}/tools/g13/vulkan_boot_assets.py" "${CMAKE_CURRENT_SOURCE_DIR}/build_view.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/build_roles.py")
  execute_process(COMMAND "${G13_PYTHON}" "${tool}" --compiler "${G13_GLSLANG}" --out "${G13_ASSET_DIR}"
                  COMMAND_ERROR_IS_FATAL ANY)
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ROOT}/tools/architecture/analyze_t750.py"
    "${ROOT}/tools/g13/vulkan_boot_assets.py" "${CMAKE_CURRENT_SOURCE_DIR}/build_view.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/build_roles.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/boot_view.vert" "${CMAKE_CURRENT_SOURCE_DIR}/boot_view.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/m1_source_view.frag" "${CMAKE_CURRENT_SOURCE_DIR}/m1_dry_scene.frag")
add_executable(hello_world_pc main.cpp wsi.cpp "${GPU}/g13vulkan_boot/boot.cpp")
set_property(SOURCE "${GPU}/g13vulkan_boot/boot.cpp" APPEND PROPERTY OBJECT_DEPENDS
    "${GPU}/g13vulkan_boot/m1_source.inc" "${GPU}/g13vulkan_boot/m1_quantum.inc"
    "${GPU}/g13vulkan_boot/m1_cold_format.h"
    "${GPU}/g13vulkan_boot/t849_writes.inc" "${GPU}/g13vulkan_boot/t854_writes.inc" "${GPU}/g13vulkan_boot/boot.h"
    "${GPU}/g13vulkan_boot/t833_candidate.inc" "${CMAKE_CURRENT_SOURCE_DIR}/root_plan.h"
    "${GPU}/m1/quantum_passes.h" "${G13_ASSET_DIR}/pc_steady_spirv.h")
set_property(SOURCE wsi.cpp APPEND PROPERTY OBJECT_DEPENDS "${G13_ASSET_DIR}/pc_view_spirv.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/wsi.h" "${CMAKE_CURRENT_SOURCE_DIR}/m1_hot_capture.h")
set_property(SOURCE main.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${GPU}/g13vulkan_boot/boot.h" "${CMAKE_CURRENT_SOURCE_DIR}/m1_ingress.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/m1_hot_capture.h" "${CMAKE_CURRENT_SOURCE_DIR}/m1_d1_plan.h")
set_property(SOURCE "${GPU}/g13vulkan_boot/boot.cpp" APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/m1_ingress.h" "${CMAKE_CURRENT_SOURCE_DIR}/m1_hot_capture.h")
target_sources(hello_world_pc PRIVATE "${GPU}/m1/source_owner.cpp" "${GPU}/integer.cpp")
set_property(SOURCE "${GPU}/m1/source_owner.cpp" APPEND PROPERTY OBJECT_DEPENDS
    "${GPU}/m1/source_owner.h" "${GPU}/m1/quantum_owner.inc")
target_compile_features(hello_world_pc PRIVATE cxx_std_17)
target_compile_definitions(hello_world_pc PRIVATE VK_NO_PROTOTYPES NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(hello_world_pc PRIVATE /EHsc /fp:strict /utf-8)
target_include_directories(hello_world_pc PRIVATE "${PC_VK_INCLUDE}" "${G13_ASSET_DIR}"
    "${GPU}/g13flow" "${GPU}/g13vulkan_boot" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(hello_world_pc PRIVATE user32)
