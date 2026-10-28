# T236 foundation slice; no M0 route is enabled by compiling this library.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/integer_primitives.glsl")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/integer_primitives.glsl" GRS_PRIMITIVES)
string(SHA256 GRS_PRIMITIVE_SHA "${GRS_PRIMITIVES}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/grs_primitive_source.h" "#pragma once\nstatic constexpr const char* kGrsPrimitiveSource=R\"GRS(${GRS_PRIMITIVES})GRS\";\nstatic constexpr const char* kGrsPrimitiveSha=\"${GRS_PRIMITIVE_SHA}\";\n")
add_library(grs_foundation STATIC integer.cpp control.cpp resources.cpp foundation.cpp)
target_include_directories(grs_foundation PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(grs_foundation PRIVATE -fno-fast-math -ffp-contract=off)
set_target_properties(grs_foundation PROPERTIES POSITION_INDEPENDENT_CODE ON)
