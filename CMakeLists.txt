cmake_minimum_required(VERSION 3.23)

# ${W2N_MONOREPO_ROOT}: w2n monorepo root. The parent directory of the git repo.
option(W2N_INCLUDE_TOOLS "Generate build targets for w2n tools" TRUE)

option(W2N_INCLUDE_TESTS "Create targets for building/running tests." TRUE)

option(W2N_INCLUDE_TEST_BINARIES
  "Create targets for building/running test binaries even if W2N_INCLUDE_TESTS is disabled"
  TRUE)

# Modules
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_MODULE_PATH
  "${W2N_MONOREPO_ROOT}/llvm-project/llvm/cmake/modules")

# Setting up C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-rtti")

# Set optimization levels
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# Product compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#
# Find required dependencies.
#
find_package(Python3 COMPONENTS Interpreter REQUIRED)

include(W2NSharedCMakeConfig)
include(AddW2N)
include(W2NConfigureSDK)
include(W2NList)

set(W2N_BUILT_STANDALONE FALSE)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(W2N_BUILT_STANDALONE TRUE)
endif()

if(W2N_BUILT_STANDALONE)
  project(wasm2native C CXX ASM)
endif()

if(W2N_BUILT_STANDALONE)
  w2n_common_standalone_build_config(W2N)
else()
  w2n_common_unified_build_config(W2N)
endif()

set(W2N_LIBRARY_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if("${W2N_HOST_VARIANT_SDK}" MATCHES "(OSX|IOS*|TVOS*|WATCHOS*)")
  set(W2N_HOST_VARIANT "macosx" CACHE STRING
    "Deployment OS for Swift host tools (the compiler) [macosx, iphoneos].")

  # Display Xcode toolchain version.
  # The SDK configuration below prints each SDK version.
  execute_process(
    COMMAND "xcodebuild" "-version"
    OUTPUT_VARIABLE xcode_version
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\n" ", " xcode_version "${xcode_version}")
  message(STATUS "${xcode_version}")
  message(STATUS "")

  include(DarwinSDKs)

  # Primary variant is always OSX; even on iOS hosts.
  set(W2N_PRIMARY_VARIANT_SDK_default "OSX")
  set(W2N_PRIMARY_VARIANT_ARCH_default "${CMAKE_HOST_SYSTEM_PROCESSOR}")
else()
  message(FATAL_ERROR "Unsupported host variant sdk ${W2N_HOST_VARIANT_SDK}")
endif()

include_directories(SYSTEM "${W2N_SOURCE_ROOT}/include")
include_directories(SYSTEM "${W2N_BUILD_ROOT}/include")

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(unittests)
