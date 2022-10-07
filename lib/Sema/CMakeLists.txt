add_w2n_host_library(w2nSema SHARED
  Sema.cpp
  TypeCheck.cpp)

set_w2n_llvm_is_available(w2nSema)
