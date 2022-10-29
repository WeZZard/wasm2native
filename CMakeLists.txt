cmake_minimum_required(VERSION 3.23)

# ${W2N_MONOREPO_ROOT}: w2n monorepo root. The parent directory of the git repo.

option(W2N_INCLUDE_TOOLS
    "Generate build targets for w2n tools"
    TRUE)

# Modules
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_MODULE_PATH
  "${W2N_MONOREPO_ROOT}/llvm-project/llvm/cmake/modules")

# G-test

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # release-1.12.1
)

FetchContent_MakeAvailable(googletest)

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

include(W2NSharedCMakeConfig)
include(AddW2N)

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

include_directories(SYSTEM "${W2N_SOURCE_ROOT}/include")
include_directories(SYSTEM "${W2N_BUILD_ROOT}/include")

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(tools)
