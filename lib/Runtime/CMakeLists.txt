add_w2n_host_library(w2nRuntime SHARED
  Runtime.cpp
  LLVM_LINK_COMPONENTS support)

set_w2n_llvm_is_available(w2nRuntime)
