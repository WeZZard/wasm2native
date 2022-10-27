add_w2n_unittest(w2nBasicTests
  StableHasher.cpp
)

target_include_directories(w2nBasicTests PRIVATE ./)

target_link_libraries(w2nBasicTests
   PRIVATE
   w2nBasic
)
