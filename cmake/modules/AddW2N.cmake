include(W2NUtils)

function(add_w2n_host_tool executable)
  set(options "")
  set(single_parameter_options "")
  set(multiple_parameter_options LLVM_LINK_COMPONENTS)

  cmake_parse_arguments(AWHT
    "${options}"
    "${single_parameter_options}"
    "${multiple_parameter_options}"
    ${ARGN})
  
  add_executable(${executable} ${AWHT_UNPARSED_ARGUMENTS})

  if("support" IN_LIST AWHT_LLVM_LINK_COMPONENTS)
    list(APPEND AWHT_LLVM_LINK_COMPONENTS "demangle")
  endif()

  llvm_update_compile_flags(${executable})
  w2n_common_llvm_config(${executable} ${AWHT_LLVM_LINK_COMPONENTS})

# TODO: Needs to clearify install path and @rpath for MachO files.
endfunction()

function(add_w2n_host_library name)
  set(options
        SHARED
        STATIC
        OBJECT)
  set(single_parameter_options)
  set(multiple_parameter_options
        LLVM_LINK_COMPONENTS)

  cmake_parse_arguments(AWHL
                        "${options}"
                        "${single_parameter_options}"
                        "${multiple_parameter_options}"
                        ${ARGN})
  set(AWHL_SOURCES ${AWHL_UNPARSED_ARGUMENTS})

  translate_flags(AWHL "${options}")

  if(NOT AWHL_SHARED AND NOT AWHL_STATIC AND NOT AWHL_OBJECT)
    message(FATAL_ERROR "One of SHARED/STATIC/OBJECT must be specified")
  endif()

  # Using `support` llvm component ends up adding `-Xlinker /path/to/lib/libLLVMDemangle.a`
  # to `LINK_FLAGS` but `libLLVMDemangle.a` is not added as an input to the linking ninja statement.
  # As a workaround, include `demangle` component whenever `support` is mentioned.
  if("support" IN_LIST AWHL_LLVM_LINK_COMPONENTS)
    list(APPEND AWHL_LLVM_LINK_COMPONENTS "demangle")
  endif()

  if(AWHL_SHARED)
    set(libkind SHARED)
  elseif(AWHL_STATIC)
    set(libkind STATIC)
  elseif(AWHL_OBJECT)
    set(libkind OBJECT)
  endif()

  add_library(${name} ${libkind} ${AWHL_SOURCES})

  # Respect LLVM_COMMON_DEPENDS if it is set.
  #
  # LLVM_COMMON_DEPENDS if a global variable set in ./lib that provides targets
  # such as tblgen that all LLVM based tools depend on. If we don't have it 
  # defined, then do not add the dependency since some parts of w2n host tools
  # do not interact with LLVM tools and do not define LLVM_COMMON_DEPENDS.
  if (LLVM_COMMON_DEPENDS)
    add_dependencies(${name} ${LLVM_COMMON_DEPENDS})
  endif()
  llvm_update_compile_flags(${name})
  w2n_common_llvm_config(${name} ${AWHL_LLVM_LINK_COMPONENTS})
  set_output_directory(${name}
      BINARY_DIR ${W2N_RUNTIME_OUTPUT_INTDIR}
      LIBRARY_DIR ${W2N_LIBRARY_OUTPUT_INTDIR})

  set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)

  # If we are compiling in release or release with deb info, compile w2n code
  # with -cross-module-optimization enabled.
  target_compile_options(${name} PRIVATE
    $<$<AND:$<COMPILE_LANGUAGE:WebAssembly>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:-cross-module-optimization>)

endfunction()

# Declare that files in this library are built with LLVM's support
# libraries available.
function(set_w2n_llvm_is_available name)
  target_compile_definitions(${name} PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>:W2N_LLVM_SUPPORT_IS_AVAILABLE>)
endfunction()
