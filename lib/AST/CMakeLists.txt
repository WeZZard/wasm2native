add_w2n_host_library(w2nAST STATIC
  ASTContext.cpp
  Builtins.cpp
  Decl.cpp
  DeclContext.cpp
  DiagnosticConsumer.cpp
  DiagnosticEngine.cpp
  DiagnosticList.cpp
  Evaluator.cpp
  Identifier.cpp
  InstNode.cpp
  Module.cpp
  SourceFile.cpp
  TypeCheckRequests.cpp
  LLVM_LINK_COMPONENTS
    ${LLVM_TARGETS_TO_BUILD})
