enable_testing()

set(DRIVER_TARGET "w2n-frontend")
set(WASM_2_NATIVE_SOURCE_DIR "${W2N_SOURCE_ROOT}/tools/driver")

add_executable(
  ${DRIVER_TARGET}_unittests
  driver_tests.cpp
)

target_include_directories(${DRIVER_TARGET}_unittests
  PRIVATE
  "${WASM_2_NATIVE_SOURCE_DIR}"
)

target_link_libraries(
  ${DRIVER_TARGET}_unittests
  ${DRIVER_TARGET}
  gtest_main
)

include(GoogleTest)

gtest_discover_tests(${DRIVER_TARGET}_unittests)
