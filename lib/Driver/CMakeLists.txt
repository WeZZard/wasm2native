add_w2n_host_library(w2nDriver STATIC
  Driver.cpp
  FrontendUtil.cpp)

add_dependencies(w2nDriver
  w2nOptions)
target_link_libraries(w2nDriver PRIVATE
  w2nAST
  w2nBasic
  w2nOptions)

set_w2n_llvm_is_available(w2nDriver)
