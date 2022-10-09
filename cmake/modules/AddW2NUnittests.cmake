
include(AddW2N)

add_custom_target(W2NUnitTests)

set_target_properties(W2NUnitTests PROPERTIES FOLDER "Tests")

function(add_w2n_unittest test_dirname)
  # *NOTE* Even though "add_unittest" does not have llvm in its name, it is a
  # function defined by AddLLVM.cmake.
  set(add_unittst_exists false)
  add_unittest(W2NUnitTests ${test_dirname} ${ARGN})

  # TODO: _add_variant_c_compile_link_flags and these tests should share some
  # sort of logic.
  #
  # *NOTE* The unittests are never built for the target, so we always enable LTO
  # *if we are asked to.
  _compute_lto_flag("${W2N_TOOLS_ENABLE_LTO}" _lto_flag_out)
  if (_lto_flag_out)
    set_property(TARGET "${test_dirname}" APPEND_STRING PROPERTY COMPILE_FLAGS " ${_lto_flag_out} ")
    set_property(TARGET "${test_dirname}" APPEND_STRING PROPERTY LINK_FLAGS " ${_lto_flag_out} ")
  endif()

  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    # FIXME: Add an @rpath to the w2n library directory
    # and one to the OS dylibs we require but
    # are not building ourselves (e.g Foundation overlay)
  endif()

  # FIXME: Consider threading package usage for variant hosting platforms

  # FIXME: Consider coverage report

  if(W2N_RUNTIME_USE_SANITIZERS)
    if("Thread" IN_LIST W2N_RUNTIME_USE_SANITIZERS)
      set_property(TARGET "${test_dirname}" APPEND_STRING PROPERTY COMPILE_FLAGS
        " -fsanitize=thread")
      set_property(TARGET "${test_dirname}" APPEND_STRING PROPERTY
        LINK_FLAGS " -fsanitize=thread")
    endif()
  endif()
endfunction()

