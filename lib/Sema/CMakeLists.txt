add_w2n_host_library(w2nSema STATIC
  ImportResolution.cpp
  Sema.cpp
  TypeCheck.cpp
  TypeCheckRequestFunctions.cpp)

target_link_libraries(w2nSema PRIVATE
  w2nAST
  w2nParse)

set_w2n_llvm_is_available(w2nSema)
