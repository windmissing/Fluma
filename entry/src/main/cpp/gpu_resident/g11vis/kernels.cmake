# T566 (problem T563): G11 composite visual kernels.
# The composite pipeline is generated from the frozen parent pipelines plus
# the g11vis visual stage (test/t566_generate_composite.cjs); generation is a
# pre-commit step that writes g11vis_pipeline.h next to this build tree.
set(G11VIS_GEN "${CMAKE_CURRENT_SOURCE_DIR}/../../../test/t566_generate_composite.cjs")
if(EXISTS "${G11VIS_GEN}")
  execute_process(COMMAND "C:/Program Files/nodejs/node.exe" "${G11VIS_GEN}" "${CMAKE_CURRENT_BINARY_DIR}" OUTPUT_VARIABLE G11VIS_GEN_OUT ERROR_VARIABLE G11VIS_GEN_ERR RESULT_VARIABLE G11VIS_GEN_RESULT)
  if(NOT G11VIS_GEN_RESULT EQUAL 0)
    message(WARNING "g11vis composite generation failed: ${G11VIS_GEN_ERR}")
  endif()
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/g11vis/visual_stage.glsl")
target_sources(grs_foundation PRIVATE g11vis/kernel_assets.cpp g11vis/visual_transport.cpp g11vis/scene.cpp g11vis/cold.cpp)
