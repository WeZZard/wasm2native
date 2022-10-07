add_w2n_host_library(w2nFrontend STATIC
  CompilerInstance.cpp
  CompilerInvocation.cpp
  Filesystem.cpp
  Frontend.cpp
  FrontendInputsAndOutputs.cpp
  FrontendOptions.cpp
  Input.cpp)

add_dependencies(w2nFrontend
  w2nOptions)

target_link_libraries(w2nFrontend PRIVATE
  w2nAST
  w2nIRGen
  w2nOptions
  w2nParse
  w2nSema)

set_w2n_llvm_is_available(w2nFrontend)
