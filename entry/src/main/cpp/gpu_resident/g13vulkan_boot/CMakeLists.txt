set(GRS_G13_SPLIT ON)
include("${CMAKE_CURRENT_LIST_DIR}/../g13flow/kernels.cmake")
if(NOT EXISTS "${G13_GLSLANG}")
  message(FATAL_ERROR "T742 requires G13_GLSLANG pointing to glslang with SPIRV-Tools")
endif()
set(T742_BUILDER "${G13_REPO_DIR}/tools/g13/vulkan_boot_assets.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${T742_BUILDER}"
  "${G13_GLSLANG}" "${G13_REPO_DIR}/docs/architecture-T741-evidence-v1.json"
  "${G13_REPO_DIR}/entry/src/main/ets/physics/G13ResearchConfig.ets")
execute_process(COMMAND "${G13_PYTHON}" "${T742_BUILDER}"
  --out "${G13_ASSET_DIR}" --compiler "${G13_GLSLANG}"
  RESULT_VARIABLE T742_RESULT ERROR_VARIABLE T742_ERROR)
if(NOT T742_RESULT STREQUAL "0")
  message(FATAL_ERROR "T742 BOOT generation failed: ${T742_ERROR}")
endif()

add_library(grs_g13vk_boot SHARED boot.cpp bridge.cpp)
target_compile_features(grs_g13vk_boot PRIVATE cxx_std_17)
target_compile_definitions(grs_g13vk_boot PRIVATE VK_USE_PLATFORM_OHOS VK_NO_PROTOTYPES)
target_compile_options(grs_g13vk_boot PRIVATE -fexceptions -fno-fast-math -ffp-contract=off)
target_include_directories(grs_g13vk_boot BEFORE PRIVATE
  "${G13_ASSET_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../g13flow")
target_link_libraries(grs_g13vk_boot PRIVATE
  libace_napi.z.so libace_ndk.z.so libnative_window.so libhilog_ndk.z.so libdl.so)
