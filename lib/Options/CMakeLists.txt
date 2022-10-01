add_w2n_host_library(w2nOptions STATIC
  Options.cpp
  LLVM_LINK_COMPONENTS option)
add_dependencies(w2nOptions
  W2NOptionsTbd)
target_link_libraries(w2nOptions PRIVATE
  w2nBasic)
