
add_w2n_host_library(w2nIRGen STATIC
  IRGen.cpp
  IRGenModule.cpp
  IRGenRequests.cpp
  LLVM_LINK_COMPONENTS
    target
    transformutils
    bitwriter
    lto
)
target_link_libraries(w2nIRGen PRIVATE
  w2nAST
  w2nParse
  w2nTBDGen)

set_w2n_llvm_is_available(w2nIRGen)
