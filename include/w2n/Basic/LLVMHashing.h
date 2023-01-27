/// DO NOT INCLUDE <llvm/ADT/Hashing.h> DIRECTLY. INCLUDE THIS FILE
/// INSTEAD.

#ifndef W2N_BASIC_LLVMHASHING_H
#define W2N_BASIC_LLVMHASHING_H

#pragma mark Templates Forward Declaration

#include <vector>

namespace llvm {
class hash_code;

/// \c std::vector hash support.
template <typename T>
hash_code hash_value(const std::vector<T *>& Vec);

} // namespace llvm

#pragma mark Including llvm/ADT/Hashing.h

#include <llvm/ADT/Hashing.h>

#pragma mark Templates Implementation

namespace llvm {

template <typename T>
hash_code hash_value(const std::vector<T *>& Vec) {
  return hash_combine_range(Vec.begin(), Vec.end());
}

} // namespace llvm

#endif // W2N_BASIC_LLVMHASHING_H
