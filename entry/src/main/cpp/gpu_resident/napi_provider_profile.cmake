# Bind the actual compile SDK header identity; never infer a unit from it.
function(grs_bind_napi_provider_sdk target)
 set(header "${CMAKE_SYSROOT}/usr/include/napi/native_api.h")
 if(EXISTS "${header}")
  file(SHA256 "${header}" sdk_sha)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${header}")
  target_compile_definitions(${target} PUBLIC GRS_NAPI_SDK_SHA="${sdk_sha}")
 endif()
endfunction()
