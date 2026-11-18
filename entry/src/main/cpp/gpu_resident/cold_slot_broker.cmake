# A shared DSO supplies the single process-wide object to grsm0/grsm1 and all
# route libraries. Do not compile this source into grs_foundation STATIC.
if(NOT TARGET grs_cold_slots)
 add_library(grs_cold_slots SHARED "${CMAKE_CURRENT_LIST_DIR}/cold_slot_broker.cpp")
 target_compile_features(grs_cold_slots PRIVATE cxx_std_17)
endif()
target_link_libraries(grs_foundation PUBLIC grs_cold_slots)
