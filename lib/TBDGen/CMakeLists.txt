add_w2n_host_library(w2nTBDGen STATIC
  APIGen.cpp
  TBDGen.cpp
  TBDGenRequests.cpp
  LLVM_LINK_COMPONENTS
    demangle
    TextAPI)

target_link_libraries(w2nTBDGen PRIVATE
  w2nAST
  w2nIRGen)

set_w2n_llvm_is_available(w2nTBDGen)
