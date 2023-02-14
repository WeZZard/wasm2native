/// DO NOT INCLUDE <llvm/ADT/Hashing.h> DIRECTLY. INCLUDE THIS FILE
/// INSTEAD.

#ifndef W2N_BASIC_LLVMHASHING_H
#define W2N_BASIC_LLVMHASHING_H

#pragma mark Templates Forward Declaration

#include <vector>

namespace llvm {
class hash_code;
template <typename... PTs>
class PointerUnion;

template <typename PT1, typename PT2>
hash_code hash_value(const llvm::PointerUnion<PT1, PT2>& Ptr);

/// \c std::vector hash support.
template <typename T>
hash_code hash_value(const std::vector<T *>& Vec);

} // namespace llvm

#pragma mark Including llvm/ADT/Hashing.h

#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/PointerUnion.h>

#pragma mark Templates Implementation

namespace llvm {

template <typename PT1, typename PT2>
hash_code hash_value(const llvm::PointerUnion<PT1, PT2>& Ptr) {
  return hash_value(Ptr.getOpaqueValue());
}

template <typename T>
hash_code hash_value(const std::vector<T *>& Vec) {
  return hash_combine_range(Vec.begin(), Vec.end());
}

} // namespace llvm

#endif // W2N_BASIC_LLVMHASHING_H
