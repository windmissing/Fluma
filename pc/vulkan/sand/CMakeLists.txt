cmake_minimum_required(VERSION 3.21)
project(hello_world_pc_sand LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows x64 required")
endif()
if(NOT SAND_FAMILY MATCHES "^(dm2|r1)$")
  message(FATAL_ERROR "Set SAND_FAMILY to dm2 or r1")
endif()
if(NOT EXISTS "${VULKAN_HEADERS}/vulkan/vulkan.h" OR NOT EXISTS "${G13_GLSLANG}")
  message(FATAL_ERROR "Set VULKAN_HEADERS and G13_GLSLANG")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ASSETS "${CMAKE_CURRENT_BINARY_DIR}/sand_assets")
set(SAND_ROLE_VARIANT "baseline" CACHE STRING "PC sand SOURCE role set")
set(REPO "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(GPU "${REPO}/entry/src/main/cpp/gpu_resident")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sand_variant.txt" CONTENT "${SAND_FAMILY}/${SAND_ROLE_VARIANT}\n")
file(GLOB_RECURSE ROLE_INPUTS CONFIGURE_DEPENDS
    "${GPU}/g9dm/*.glsl" "${GPU}/g9dm/*.comp" "${GPU}/g9dm/*.json" "${GPU}/g9dm/*.h"
    "${GPU}/g9spf/*.glsl" "${GPU}/g9spf/*.comp" "${GPU}/g9spf/*.h"
    "${GPU}/g11vis/*.glsl" "${GPU}/g11vis/*.comp" "${GPU}/g11vis/*.h"
    "${GPU}/acceptance_t647/*.glsl" "${GPU}/acceptance_t647/*.h"
    "${REPO}/tools/glsl/*.py" "${REPO}/tools/g9dm/*.py")
set(ROLE_HEADER "${ASSETS}/sand_passes.h")
add_custom_command(OUTPUT "${ROLE_HEADER}" "${ASSETS}/roles.json"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../build_sand_roles.py"
        "${SAND_FAMILY}" --compiler "${G13_GLSLANG}" --out "${ASSETS}" --role-variant "${SAND_ROLE_VARIANT}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sand_variant.txt" "${G13_GLSLANG}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../build_sand_roles.py" "${CMAKE_CURRENT_SOURCE_DIR}/t913_roles.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/t913_split_roles.py" "${GPU}/integer_primitives.glsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/../build_t909_angle_probes.py" "${CMAKE_CURRENT_SOURCE_DIR}/t909_g15.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/g15_dm2.glsl" "${CMAKE_CURRENT_SOURCE_DIR}/../t933_g15_qualify.py"
        "${GPU}/g11vis/scene.cpp" "${REPO}/docs/architecture-T552-g9-seed-manifest-v2.json"
        "${REPO}/docs/architecture-T660-g11-manifest-v2.json" "${REPO}/tools/g11/generate_contract.cjs"
        "${REPO}/test/t566_generate_composite.cjs" ${ROLE_INPUTS}
    VERBATIM)
add_custom_target(sand_roles DEPENDS "${ROLE_HEADER}" "${ASSETS}/roles.json")
set_source_files_properties(pass.cpp owner.cpp host.cpp host_device.cpp host_present.cpp angle_probe.cpp
    ${SAND_FAMILY}.cpp PROPERTIES OBJECT_DEPENDS "${ROLE_HEADER}")
set(VK_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/vk_include")
file(COPY "${VULKAN_HEADERS}/vulkan" DESTINATION "${VK_INCLUDE}")
if(IS_DIRECTORY "${VULKAN_HEADERS}/vk_video")
  file(COPY "${VULKAN_HEADERS}/vk_video" DESTINATION "${VK_INCLUDE}")
endif()
add_library(pc_sand_${SAND_FAMILY} STATIC pass.cpp owner.cpp ${SAND_FAMILY}.cpp "${ROLE_HEADER}")
add_dependencies(pc_sand_${SAND_FAMILY} sand_roles)
target_sources(pc_sand_${SAND_FAMILY} PRIVATE "${GPU}/acceptance_t647/contract.cpp"
    "${GPU}/g9dm/completion_gate.cpp")
target_compile_features(pc_sand_${SAND_FAMILY} PUBLIC cxx_std_17)
target_compile_definitions(pc_sand_${SAND_FAMILY} PUBLIC VK_NO_PROTOTYPES NOMINMAX)
if(SAND_FAMILY STREQUAL "r1")
  target_compile_definitions(pc_sand_${SAND_FAMILY} PUBLIC PC_SAND_G11_R1)
  target_sources(pc_sand_${SAND_FAMILY} PRIVATE "${GPU}/g9spf/completion_gate.cpp"
      "${GPU}/g11vis/composite_gate.cpp")
endif()
target_include_directories(pc_sand_${SAND_FAMILY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${VK_INCLUDE}" "${ASSETS}")
target_compile_options(pc_sand_${SAND_FAMILY} PRIVATE /EHsc /utf-8 /W4)
add_executable(pc_sand_${SAND_FAMILY}_host host.cpp host_device.cpp
    "${GPU}/g11vis/parent_adapter.cpp"
    "${GPU}/g9dm/progress_clock_v3.cpp" "${GPU}/g9dm/quota_snapshot_v3.cpp")
if(SAND_FAMILY STREQUAL "r1")
  target_sources(pc_sand_${SAND_FAMILY}_host PRIVATE host_present.cpp)
endif()
target_link_libraries(pc_sand_${SAND_FAMILY}_host PRIVATE pc_sand_${SAND_FAMILY} user32)
target_compile_options(pc_sand_${SAND_FAMILY}_host PRIVATE /EHsc /utf-8 /W4)
if(SAND_FAMILY STREQUAL "dm2")
  set(VIEW "${CMAKE_CURRENT_BINARY_DIR}/view_assets")
  add_custom_command(OUTPUT "${VIEW}/pc_view_spirv.h"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${VIEW}"
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../build_view.py"
              --compiler "${G13_GLSLANG}" --out "${VIEW}"
      DEPENDS "${G13_GLSLANG}" "${CMAKE_CURRENT_SOURCE_DIR}/../build_view.py"
              "${CMAKE_CURRENT_SOURCE_DIR}/../g15_dm_scene.frag" "${CMAKE_CURRENT_SOURCE_DIR}/../boot_view.vert"
              "${CMAKE_CURRENT_SOURCE_DIR}/../boot_view.frag" "${CMAKE_CURRENT_SOURCE_DIR}/../m1_source_view.frag"
              "${CMAKE_CURRENT_SOURCE_DIR}/../m1_dry_scene.frag"
              "${CMAKE_CURRENT_SOURCE_DIR}/../g16_scene.vert" "${CMAKE_CURRENT_SOURCE_DIR}/../g16_scene.frag" VERBATIM)
  set_source_files_properties(../wsi.cpp PROPERTIES OBJECT_DEPENDS "${VIEW}/pc_view_spirv.h")
endif()
if(SAND_ROLE_VARIANT MATCHES "^g15-dm2(-op23|-dry-front)?$")
  target_compile_definitions(pc_sand_${SAND_FAMILY}_host PRIVATE PC_SAND_G15_SCENE)
  target_include_directories(pc_sand_${SAND_FAMILY}_host PRIVATE "${VIEW}" "${CMAKE_CURRENT_SOURCE_DIR}/.."
      "${GPU}/g13vulkan_boot")
  target_sources(pc_sand_${SAND_FAMILY}_host PRIVATE host_g15_present.cpp ../wsi.cpp "${VIEW}/pc_view_spirv.h")
endif()
if(SAND_FAMILY STREQUAL "dm2")
  function(add_angle_probe target)
    add_executable(${target} angle_probe.cpp g16_multi.cpp host_device.cpp ../wsi.cpp "${VIEW}/pc_view_spirv.h")
    target_include_directories(${target} PRIVATE "${VIEW}" "${GPU}/g13vulkan_boot")
    target_link_libraries(${target} PRIVATE pc_sand_dm2 user32)
    target_compile_options(${target} PRIVATE /EHsc /utf-8 /W4)
  endfunction()
  add_angle_probe(t909_angle_probe)
  option(PC_G16_FREEZE_FIXTURE "Build isolated G16 FREEZE fault injection executable" OFF)
  if(PC_G16_FREEZE_FIXTURE)
    add_angle_probe(t936_g16_freeze_fixture)
    target_compile_definitions(t936_g16_freeze_fixture PRIVATE PC_G16_FREEZE_FIXTURE)
  endif()
endif()
