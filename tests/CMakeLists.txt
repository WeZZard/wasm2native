list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(W2NTestUtils)

function(w2n_configure_lit_site_cfg source_path destination_path installed_name)
  if(CMAKE_CFG_INTDIR STREQUAL ".")
    set(W2N_BUILD_MODE ".")
  else()
    set(W2N_BUILD_MODE "%(build_mode)s")
  endif()

  string(REPLACE ${CMAKE_CFG_INTDIR} ${W2N_BUILD_MODE} LLVM_TOOLS_DIR ${LLVM_TOOLS_BINARY_DIR})
  string(REPLACE ${CMAKE_CFG_INTDIR} ${W2N_BUILD_MODE} LLVM_LIBS_DIR ${LLVM_LIBRARY_DIR})

  if(XCODE)
    string(REPLACE ${CMAKE_CFG_INTDIR} Debug LIT_W2NLIB_DIR ${W2NLIB_DIR})
  else()
    set(LIT_W2NLIB_DIR ${W2NLIB_DIR})
  endif()

  configure_file("${source_path}" "${destination_path}" @ONLY)

  # FIXME: Commented for MVP
  # if(NOT "${installed_name}" STREQUAL "")
  # w2n_install_in_component(FILES "${destination_path}"
  # RENAME "${installed_name}"
  # DESTINATION "share/w2n/testsuite"
  # COMPONENT testsuite-tools)
  # endif()
endfunction()

function(normalize_boolean_spelling var_name)
  if(${var_name})
    set("${var_name}" TRUE PARENT_SCOPE)
  else()
    set("${var_name}" FALSE PARENT_SCOPE)
  endif()
endfunction()

function(get_test_dependencies SDK result_var_name)
  set(deps)

  if(W2N_BUILD_STDLIB)
    list(APPEND deps SwiftUnitTests)
  endif()

  if(W2N_BUILD_SDK_OVERLAY OR W2N_BUILD_TEST_SUPPORT_MODULES)
    list(APPEND deps sdk-overlay)
  endif()

  set(deps_binaries)

  if(W2N_INCLUDE_TOOLS)
    list(APPEND deps_binaries
      w2n-frontend)

    if(W2N_BUILD_SOURCEKIT)
      list(APPEND deps_binaries sourcekitd-test complete-test)
    endif()
  endif()

  if(NOT W2N_BUILT_STANDALONE)
    list(APPEND deps_binaries
      clang
      llc
      llvm-ar
      llvm-as
      llvm-bcanalyzer
      llvm-cov
      llvm-dis
      llvm-dwarfdump
      llvm-link
      llvm-nm
      llvm-objdump
      llvm-profdata
      llvm-readelf
      llvm-readobj
      llvm-size
      llvm-strings)
  endif()

  if(("${SDK}" STREQUAL "IOS") OR
    ("${SDK}" STREQUAL "TVOS") OR
    ("${SDK}" STREQUAL "WATCHOS") OR
    ("${SDK}" STREQUAL "OSX") OR
    ("${SDK}" STREQUAL "IOS_SIMULATOR") OR
    ("${SDK}" STREQUAL "TVOS_SIMULATOR") OR
    ("${SDK}" STREQUAL "WATCHOS_SIMULATOR") OR
    ("${SDK}" STREQUAL "FREESTANDING") OR
    ("${SDK}" STREQUAL "LINUX") OR
    ("${SDK}" STREQUAL "CYGWIN") OR
    ("${SDK}" STREQUAL "FREEBSD") OR
    ("${SDK}" STREQUAL "OPENBSD") OR
    ("${SDK}" STREQUAL "ANDROID") OR
    ("${SDK}" STREQUAL "WINDOWS") OR
    ("${SDK}" STREQUAL "HAIKU"))
  # No extra dependencies.
  else()
    message(FATAL_ERROR "Unknown SDK: ${SDK}")
  endif()

  # Just use target names for dependency generation. This works for both Xcode
  # and non-Xcode build systems. In the case of Xcode, its build paths have a
  # configuration variable in them, so CMake can't match them at compile time.
  list(APPEND deps ${deps_binaries})

  set("${result_var_name}" "${deps}" PARENT_SCOPE)
endfunction()

set(LIT "${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py")

set(W2N_LIT_ARGS "" CACHE STRING "Arguments to pass to lit")

set(W2N_LIT_ENVIRONMENT "" CACHE STRING "Environment to use for lit invocations")

if(NOT W2N_INCLUDE_TOOLS)
  if(W2N_RUN_TESTS_WITH_HOST_COMPILER)
    precondition(CMAKE_Swift_COMPILER MESSAGE "Can only run tests if a Swift compiler is specified")
    get_filename_component(W2N_COMPILER_DIR "${CMAKE_Swift_COMPILER}" DIRECTORY)
    precondition(W2N_COMPILER_DIR)

    # We assume that we are building against a toolchain where all tools are
    # next to swiftc.
    list(APPEND W2N_LIT_ARGS
      "--path=${W2N_COMPILER_DIR}")
  else()
    list(APPEND W2N_LIT_ARGS
      "--path=${W2N_NATIVE_LLVM_TOOLS_PATH}"
      "--param" "w2n_native_llvm_tools_path=${W2N_NATIVE_LLVM_TOOLS_PATH}"
      "--path=${W2N_NATIVE_CLANG_TOOLS_PATH}"
      "--param" "w2n_native_clang_tools_path=${W2N_NATIVE_CLANG_TOOLS_PATH}"
      "--path=${W2N_NATIVE_W2N_TOOLS_PATH}"
      "--param" "w2n_native_w2n_tools_path=${W2N_NATIVE_W2N_TOOLS_PATH}"
    )
  endif()

  if(W2N_BUILD_STDLIB)
    list(APPEND W2N_LIT_ARGS
      "--param" "test_resource_dir=${W2NLIB_DIR}")
  endif()
endif()

option(W2N_TEST_USE_LEAKS "Run Swift stdlib tests under leaks" FALSE)

if(W2N_TEST_USE_LEAKS)
  list(APPEND W2N_LIT_ARGS "--param" "leaks-all")
endif()

if(W2N_ENABLE_ARRAY_COW_CHECKS)
  list(APPEND W2N_LIT_ARGS
    "--param" "array_cow_checks")
endif()

if(NOT CMAKE_CFG_INTDIR STREQUAL ".")
  list(APPEND W2N_LIT_ARGS
    "--param" "build_mode=${CMAKE_CFG_INTDIR}")
endif()

if(LLVM_USE_SANITIZER STREQUAL "Address")
  set(W2N_ASAN_BUILD TRUE)
endif()

# Normalize spelling of boolean values.
normalize_boolean_spelling(LLVM_ENABLE_ASSERTIONS)
normalize_boolean_spelling(W2N_STDLIB_ASSERTIONS)
normalize_boolean_spelling(W2N_ASAN_BUILD)
normalize_boolean_spelling(W2N_OPTIMIZED)
normalize_boolean_spelling(W2N_HAVE_LIBXML2)
normalize_boolean_spelling(W2N_INCLUDE_TOOLS)

# FIXME: Swift uses SWIFT_STDLIB_BUILD_TYPE, but we don't have stdlib now.
# Here we use CMAKE_BUILD_TYPE instead.
is_build_type_optimized("${CMAKE_BUILD_TYPE}" W2N_OPTIMIZED)

# FIXME: profdata_merge_worker
# set(profdata_merge_worker
# "${CMAKE_CURRENT_SOURCE_DIR}/../utils/profdata_merge/main.py")
set(TEST_MODES
  optimize_none
  optimize
)

set(TEST_SUBSETS
  primary
  validation
  all
  only_validation
  only_long
  only_stress
)

# FIXME: custom target touch-covering-tests
# if(NOT "${COVERAGE_DB}" STREQUAL "")
# add_custom_target("touch-covering-tests"
# COMMAND "${W2N_SOURCE_DIR}/utils/coverage/coverage-touch-tests" "--w2n-dir" "${W2N_SOURCE_DIR}" "--coverage-db" "${COVERAGE_DB}"
# COMMENT "Touching covering tests")
# endif()
#
foreach(SDK ${W2N_SDKS})
  message(STATUS "ARCHS ${W2N_SDK_${SDK}_ARCHITECTURES}")

  foreach(ARCH ${W2N_SDK_${SDK}_ARCHITECTURES})
    get_w2n_test_build_flavors(build_flavors "${SDK}")

    foreach(BUILD_FLAVOR ${build_flavors})
      # Configure variables for this subdirectory.
      set(VARIANT_SDK "${W2N_SDK_${SDK}_ARCH_${ARCH}_PATH}")
      get_w2n_test_variant_suffix(VARIANT_SUFFIX "${SDK}" "${ARCH}" "${BUILD_FLAVOR}")
      get_w2n_test_variant_suffix(DEFAULT_OSX_VARIANT_SUFFIX "${SDK}" "${ARCH}" "default")
      get_w2n_test_versioned_target_triple(VARIANT_TRIPLE "${SDK}" "${ARCH}" "${W2N_SDK_${SDK}_DEPLOYMENT_VERSION}" "${BUILD_FLAVOR}")

      # A directory where to put the xUnit-style XML test results.
      set(W2N_TEST_RESULTS_DIR
        "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/w2n-test-results/${VARIANT_TRIPLE}")

      set(command_clean_test_results_dir
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${W2N_TEST_RESULTS_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${W2N_TEST_RESULTS_DIR}")

      set(test_bin_dir "${CMAKE_CURRENT_BINARY_DIR}${VARIANT_SUFFIX}")

      # TODO: validation test
      # set(validation_test_bin_dir
      # "${CMAKE_CURRENT_BINARY_DIR}/../validation-test${VARIANT_SUFFIX}")
      #
      if(LLVM_ENABLE_LIBXML2)
        set(W2N_HAVE_LIBXML2 TRUE)
      else()
        set(W2N_HAVE_LIBXML2 FALSE)
      endif()

      w2n_configure_lit_site_cfg(
        "${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in"
        "${test_bin_dir}/lit.site.cfg"
        "test${VARIANT_SUFFIX}.lit.site.cfg")

      # TODO: Unit
      # w2n_configure_lit_site_cfg
      #
      # TODO: validation test
      # w2n_configure_lit_site_cfg
      #
      set(test_dependencies)
      get_test_dependencies("${SDK}" test_dependencies)

      if(NOT "${COVERAGE_DB}" STREQUAL "")
        list(APPEND test_dependencies "touch-covering-tests")
      endif()

      set(validation_test_dependencies)

      foreach(test_mode ${TEST_MODES})
        set(LIT_ARGS "${W2N_LIT_ARGS} ${LLVM_LIT_ARGS}")
        separate_arguments(LIT_ARGS)

        execute_process(COMMAND
          $<TARGET_FILE:Python3::Interpreter> "-c" "import psutil"
          RESULT_VARIABLE python_psutil_status
          TIMEOUT 1 # second
          ERROR_QUIET)

        if(NOT python_psutil_status)
          list(APPEND LIT_ARGS "--timeout=3000") # 50 minutes
        endif()

        list(APPEND LIT_ARGS "--xunit-xml-output=${W2N_TEST_RESULTS_DIR}/lit-tests.xml")

        list(APPEND LIT_ARGS "--param" "threading=${W2N_SDK_${SDK}_THREADING_PACKAGE}")

        foreach(test_subset ${TEST_SUBSETS})
          set(directories)
          set(dependencies ${test_dependencies})

          if((test_subset STREQUAL "primary") OR
            (test_subset STREQUAL "validation") OR
            (test_subset STREQUAL "only_long") OR
            (test_subset STREQUAL "only_stress") OR
            (test_subset STREQUAL "all"))
            list(APPEND directories "${test_bin_dir}")
          endif()

          if((test_subset STREQUAL "validation") OR
            (test_subset STREQUAL "only_validation") OR
            (test_subset STREQUAL "only_long") OR
            (test_subset STREQUAL "only_stress") OR
            (test_subset STREQUAL "all"))
            list(APPEND directories "${validation_test_bin_dir}")
            list(APPEND dependencies ${validation_test_dependencies})
          endif()

          set(test_subset_target_suffix "-${test_subset}")

          if(test_subset STREQUAL "primary")
            set(test_subset_target_suffix "")
          endif()

          set(test_mode_target_suffix "")

          if(NOT test_mode STREQUAL "optimize_none")
            set(test_mode_target_suffix "-${test_mode}")
          endif()

          set(test_target_name
            "check-w2n${test_subset_target_suffix}${test_mode_target_suffix}${VARIANT_SUFFIX}")
          add_custom_target("${test_target_name}"
            COMMAND
            ${CMAKE_COMMAND} -E env ${W2N_LIT_ENVIRONMENT}
            $<TARGET_FILE:Python3::Interpreter> "${LIT}"
            ${LIT_ARGS}
            "--param" "w2n_test_subset=${test_subset}"
            "--param" "w2n_test_mode=${test_mode}"
            ${directories}
            DEPENDS ${dependencies}
            COMMENT "Running ${test_subset} wasm2native tests for ${VARIANT_TRIPLE}"
            USES_TERMINAL)

          message(STATUS "add_custom_target ${test_target_name}")

          set(test_dependencies_target_name
            "w2n${test_subset_target_suffix}${test_mode_target_suffix}${VARIANT_SUFFIX}-test-depends")
          add_custom_target("${test_dependencies_target_name}"
            DEPENDS ${dependencies})

          add_custom_target("${test_target_name}-custom"
            COMMAND
            ${CMAKE_COMMAND} -E env ${W2N_LIT_ENVIRONMENT}
            $<TARGET_FILE:Python3::Interpreter> "${LIT}"
            ${LIT_ARGS}
            "--param" "w2n_test_subset=${test_subset}"
            "--param" "w2n_test_mode=${test_mode}"
            ${W2N_LIT_TEST_PATHS}
            DEPENDS ${dependencies}
            COMMENT "Running ${test_subset} wasm2native tests for ${VARIANT_TRIPLE} from custom test locations"
            USES_TERMINAL)
          set_property(TARGET
            "${test_target_name}"
            "${test_target_name}-custom"
            "${test_dependencies_target_name}"
            PROPERTY FOLDER "Tests/check-w2n")

          message(STATUS "add_custom_target ${test_target_name}-custom")
        endforeach()
      endforeach()
    endforeach()
  endforeach()
endforeach()

# Add shortcuts for the default variant.
foreach(test_mode ${TEST_MODES})
  foreach(test_subset ${TEST_SUBSETS})
    set(test_mode_target_suffix "")

    if(NOT test_mode STREQUAL "optimize_none")
      set(test_mode_target_suffix "-${test_mode}")
    endif()

    set(test_subset_target_suffix "-${test_subset}")

    if(test_subset STREQUAL "primary")
      set(test_subset_target_suffix "")
    endif()

    set(test_target_name
      "check-w2n${test_subset_target_suffix}${test_mode_target_suffix}")
    add_custom_target("${test_target_name}"
      DEPENDS "${test_target_name}${W2N_PRIMARY_VARIANT_SUFFIX}")
    set_property(TARGET "${test_target_name}"
      PROPERTY FOLDER "Tests/check-w2n")

    set(test_depends_target_name
      "w2n${test_subset_target_suffix}${test_mode_target_suffix}-test-depends")
    add_custom_target("${test_depends_target_name}"
      DEPENDS "w2n${test_subset_target_suffix}${test_mode_target_suffix}${W2N_PRIMARY_VARIANT_SUFFIX}-test-depends")
    set_property(TARGET "${test_depends_target_name}"
      PROPERTY FOLDER "Tests/check-w2n")
  endforeach()
endforeach()
