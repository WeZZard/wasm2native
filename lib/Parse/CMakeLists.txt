add_w2n_host_library(w2nParse STATIC
  Parser.cpp)

target_link_libraries(w2nParse PRIVATE
  w2nAST
)

set_w2n_llvm_is_available(w2nParse)
