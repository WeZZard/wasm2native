add_w2n_host_library(w2nDriverTool STATIC
  DriverTool.cpp)

target_link_libraries(w2nDriverTool PUBLIC
  w2nDriver
  w2nFrontendTool)
