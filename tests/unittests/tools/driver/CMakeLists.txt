set(DRIVER_TARGET "w2n-frontend")
set(DRIVER_SOURCE_DIR "${W2N_SOURCE_ROOT}/tools/driver")

add_w2n_unittest(
  ${DRIVER_TARGET}_unittests
  driver_tests.cpp
)

target_include_directories(${DRIVER_TARGET}_unittests
  PRIVATE
  "${DRIVER_SOURCE_DIR}"
)

target_link_libraries(${DRIVER_TARGET}_unittests
  PRIVATE
  ${DRIVER_TARGET}
)
