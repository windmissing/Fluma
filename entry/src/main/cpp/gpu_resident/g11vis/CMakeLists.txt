cmake_minimum_required(VERSION 3.16)
project(grs_g11 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
if(NOT TARGET grs_cold_slots)
 add_library(grs_cold_slots SHARED ../cold_slot_broker.cpp)
endif()
add_library(grs_foundation STATIC)
set_target_properties(grs_foundation PROPERTIES POSITION_INDEPENDENT_CODE ON)
include("${CMAKE_CURRENT_LIST_DIR}/kernels.cmake")
target_sources(grs_foundation PRIVATE
 ../acceptance_t647/contract.cpp ../g9spf/completion_gate.cpp ../g9dm/completion_gate.cpp
 ../g9spf/progress_clock_v3.cpp ../g9dm/progress_clock_v3.cpp
 ../g9spf/quota_snapshot_v3.cpp ../g9dm/quota_snapshot_v3.cpp
 ../g9spf/stage_capability.cpp ../../p013renderer/fullframe_targets.cpp)
target_compile_features(grs_foundation PUBLIC cxx_std_17)
target_compile_options(grs_foundation PRIVATE -fno-fast-math -ffp-contract=off)
target_include_directories(grs_foundation PUBLIC "${G11_ASSET_DIR}" "${CMAKE_CURRENT_LIST_DIR}/..")
target_link_libraries(grs_foundation PUBLIC grs_cold_slots libEGL.so libGLESv3.so libhilog_ndk.z.so)
add_library(grs_g11 SHARED bridge.cpp abi.cpp)
target_compile_options(grs_g11 PRIVATE -fno-fast-math -ffp-contract=off)
target_link_options(grs_g11 PRIVATE -Wl,--build-id=sha1 -Wl,--no-undefined)
target_link_libraries(grs_g11 PRIVATE grs_foundation libace_napi.z.so libace_ndk.z.so libnative_window.so libdl.so)
include("${CMAKE_CURRENT_LIST_DIR}/../napi_provider_profile.cmake")
grs_bind_napi_provider_sdk(grs_g11)
