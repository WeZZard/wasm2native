add_w2n_host_library(w2nFrontendTool STATIC
  FrontendTool.cpp)
target_link_libraries(w2nFrontendTool PRIVATE
  w2nFrontend
  w2nIRGen
  w2nOptions)

set_w2n_llvm_is_available(w2nFrontendTool)
