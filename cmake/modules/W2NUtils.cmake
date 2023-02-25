# Translate a yes/no variable to the presence of a given string in a
# variable.
#
# Usage:
# translate_flag(is_set flag_name var_name)
#
# If is_set is true, sets ${var_name} to ${flag_name}. Otherwise,
# unsets ${var_name}.
function(translate_flag is_set flag_name var_name)
  if(${is_set})
    set("${var_name}" "${flag_name}" PARENT_SCOPE)
  else()
    set("${var_name}" "" PARENT_SCOPE)
  endif()
endfunction()

macro(translate_flags prefix options)
  foreach(var ${options})
    translate_flag("${${prefix}_${var}}" "${var}" "${prefix}_${var}_keyword")
  endforeach()
endmacro()

function(is_build_type_optimized build_type result_var_name)
  if("${build_type}" STREQUAL "Debug")
    set("${result_var_name}" FALSE PARENT_SCOPE)
  elseif("${build_type}" STREQUAL "RelWithDebInfo" OR
    "${build_type}" STREQUAL "Release" OR
    "${build_type}" STREQUAL "MinSizeRel")
    set("${result_var_name}" TRUE PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unknown build type: ${build_type}")
  endif()
endfunction()

function(is_sdk_requested name result_var_name)
  if("${W2N_HOST_VARIANT_SDK}" STREQUAL "${name}")
    set("${result_var_name}" "TRUE" PARENT_SCOPE)
  else()
    if("${name}" IN_LIST W2N_SDKS)
      set("${result_var_name}" "TRUE" PARENT_SCOPE)
    else()
      set("${result_var_name}" "FALSE" PARENT_SCOPE)
    endif()
  endif()
endfunction()
