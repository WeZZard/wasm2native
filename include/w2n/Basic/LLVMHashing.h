#ifndef W2N_BASIC_LLVMHASHING_H
#define W2N_BASIC_LLVMHASHING_H

#include <llvm/ADT/Hashing.h>

namespace llvm {

/// \c TypeKey storage hash support.
template <typename T>
hash_code hash_value(const std::vector<T *>& Vec) {
  return llvm::hash_combine_range(Vec.begin(), Vec.end());
}

} // namespace llvm

#endif // W2N_BASIC_LLVMHASHING_H