# T576 (problem T567): G12 water kernels. The pipeline header is generated
# by test/t576_generate_pipeline.cjs (parent g9dm + contract + water core).
set(G12WATER_GEN "${CMAKE_CURRENT_SOURCE_DIR}/../../../test/t576_generate_pipeline.cjs")
if(EXISTS "${G12WATER_GEN}")
  execute_process(COMMAND "C:/Program Files/nodejs/node.exe" "${G12WATER_GEN}" "${CMAKE_CURRENT_BINARY_DIR}" OUTPUT_VARIABLE G12WATER_GEN_OUT ERROR_VARIABLE G12WATER_GEN_ERR RESULT_VARIABLE G12WATER_GEN_RESULT)
  if(NOT G12WATER_GEN_RESULT EQUAL 0)
    message(WARNING "g12water pipeline generation failed: ${G12WATER_GEN_ERR}")
  endif()
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/g12water/water_core.glsl" "${CMAKE_CURRENT_SOURCE_DIR}/g12water/contract_generated.glsl")
target_sources(grs_foundation PRIVATE g12water/kernel_assets.cpp g12water/transport.cpp g12water/scene.cpp g12water/cold.cpp)
