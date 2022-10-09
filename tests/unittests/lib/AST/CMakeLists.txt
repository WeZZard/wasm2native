add_w2n_unittest(w2nASTTests
  ArithmeticEvaluator.cpp
  DiagnosticConsumerTests.cpp
)

target_include_directories(w2nASTTests PRIVATE ./)

target_link_libraries(w2nASTTests
   PRIVATE
   w2nFrontend
   w2nBasic
   w2nAST
   w2nParse
   w2nSema
)
