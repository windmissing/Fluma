get_filename_component(DM_REPO "${CMAKE_CURRENT_LIST_DIR}/../../../../../.." ABSOLUTE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(GLOB DM_INPUTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/frozen_t647/*" "${CMAKE_CURRENT_LIST_DIR}/../acceptance_t647/*" "${DM_REPO}/tools/g9dm/*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DM_INPUTS} "${DM_REPO}/docs/architecture-T647-acceptance-v1.json" "${DM_REPO}/docs/architecture-T554-g9-dm-manifest-v2.json" "${DM_REPO}/docs/architecture-T552-g9-seed-manifest-v2.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DM_REPO}/tools/glsl/qualified_values.py")
set(DM_GENERATE_ARGS)
if(GRS_G9_BOOT_ONLY OR GRS_G9_BOOT_SOURCE)
 if(GRS_G9_BOOT_ONLY AND GRS_G9_BOOT_SOURCE)
  message(FATAL_ERROR "DM BOOT variants are mutually exclusive")
 endif()
 if(GRS_G9_BOOT_SOURCE)
  list(APPEND DM_GENERATE_ARGS --boot-source)
  target_compile_definitions(grs_foundation PRIVATE GRS_G9_BOOT_SOURCE=1)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DM_REPO}/docs/architecture-T736-source-contract-v1.json")
 else()
  list(APPEND DM_GENERATE_ARGS --boot-only)
  target_compile_definitions(grs_foundation PRIVATE GRS_G9_BOOT_ONLY=1)
 endif()
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${DM_REPO}/docs/architecture-T733-closure-v1.json" "${DM_REPO}/tools/glsl/g9_boot.py" "${DM_REPO}/tools/g13/split_shader.py")
 target_include_directories(grs_foundation PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/dm_assets")
 target_link_libraries(grs_foundation PRIVATE libdl.so)
endif()
execute_process(COMMAND "${Python3_EXECUTABLE}" "${DM_REPO}/tools/g9dm/generate_assets.py" --out "${CMAKE_CURRENT_BINARY_DIR}/dm_assets" ${DM_GENERATE_ARGS} RESULT_VARIABLE DM_GENERATION)
if(NOT DM_GENERATION EQUAL 0)
 message(FATAL_ERROR "DM_T647_GENERATION_FAILED; no stale fallback")
endif()
# Canonical checked-in generated headers are compared, never silently rebuilt as old T555.
foreach(DM_GENERATED pipeline.comp schema_generated.h dm_contract_generated.h dm_seed_generated.h)
 execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${CMAKE_CURRENT_LIST_DIR}/${DM_GENERATED}" "${CMAKE_CURRENT_BINARY_DIR}/dm_assets/${DM_GENERATED}" RESULT_VARIABLE DM_DIFFERENT)
 if(NOT DM_DIFFERENT EQUAL 0)
  message(FATAL_ERROR "DM_CANONICAL_STALE: run tools/g9dm/generate_assets.py --canonical")
 endif()
endforeach()
set(DM_MODULE "${CMAKE_CURRENT_LIST_DIR}")
set(G9DM_PIPELINE_FILE "${DM_MODULE}/pipeline.comp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${G9DM_PIPELINE_FILE}")
file(READ "${G9DM_PIPELINE_FILE}" G9DM_PIPELINE)
string(REPLACE "\r\n" "\n" G9DM_PIPELINE "${G9DM_PIPELINE}")
string(SHA256 G9DM_PIPELINE_SHA "${G9DM_PIPELINE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/g9dm_kernel.h" "#pragma once\nstatic constexpr const char* kG9DmPipeline=R\"G9DM(${G9DM_PIPELINE})G9DM\";\nstatic constexpr const char* kG9DmPipelineHash=\"${G9DM_PIPELINE_SHA}\";\n")
set(G9DM_SCENE_HEADER "#pragma once\n")
foreach(G9DM_PAIR "g9dm/scene.vert|kG9DmSceneVertex" "g9dm/scene.frag|kG9DmSceneFragment" "g9dm/packet.vert|kG9DmPacketVertex" "g9dm/packet.frag|kG9DmPacketFragment")
 string(REPLACE "|" ";" G9DM_PARTS "${G9DM_PAIR}")
 list(GET G9DM_PARTS 0 G9DM_FILE)
 list(GET G9DM_PARTS 1 G9DM_SYMBOL)
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DM_MODULE}/../${G9DM_FILE}")
 file(READ "${DM_MODULE}/../${G9DM_FILE}" G9DM_TEXT)
 string(REPLACE "\r\n" "\n" G9DM_TEXT "${G9DM_TEXT}")
 string(APPEND G9DM_SCENE_HEADER "static constexpr const char* ${G9DM_SYMBOL}=R\"G9DMSC(${G9DM_TEXT})G9DMSC\";\n")
endforeach()
string(SHA256 G9DM_SCENE_SHA "${G9DM_SCENE_HEADER}")
string(APPEND G9DM_SCENE_HEADER "static constexpr const char* kG9DmSceneShaderSha=\"${G9DM_SCENE_SHA}\";\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/g9dm_scene.h" "${G9DM_SCENE_HEADER}")
target_sources(grs_foundation PRIVATE
 "${DM_MODULE}/../g9spf/completion_gate.cpp"
 "${DM_MODULE}/../acceptance_t647/contract.cpp"
 "${DM_MODULE}/../g9dm/proof_abi.cpp"
 "${DM_MODULE}/../g9dm/kernel_assets.cpp"
 "${DM_MODULE}/../g9dm/transport.cpp"
 "${DM_MODULE}/../g9dm/completion_gate.cpp"
 "${DM_MODULE}/../g9dm/progress_clock_v3.cpp"
 "${DM_MODULE}/../g9dm/planning_control.cpp"
 "${DM_MODULE}/../g9dm/quota_snapshot_v3.cpp"
 "${DM_MODULE}/../g9dm/quota_reader_v3.cpp"
 "${DM_MODULE}/../g9dm/controller.cpp"
 "${DM_MODULE}/../g9dm/controller_cold.cpp"
 "${DM_MODULE}/../g9dm/cold_capture.cpp"
 "${DM_MODULE}/../g9dm/scene.cpp"
 "${DM_MODULE}/../g9dm/stage_capability.cpp"
)
