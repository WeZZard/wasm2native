#ifndef W2N_IRGEN_TYPELOWERING_H
#define W2N_IRGEN_TYPELOWERING_H

#include <llvm/IR/Type.h>
#include <cstdint>
#include <w2n/Basic/LLVMHashing.h>

#pragma mark Support Types for IRGenModule Lowered Type Memoization

namespace w2n::irgen {

struct VectorTyKey {
  llvm::Type * ElementTy;

  uint32_t Count;
};

struct StructTyKey {
  std::vector<llvm::Type *> ElementTypes;
  bool IsTombstone = false;
};

struct FuncTyKey {
  std::vector<llvm::Type *> ArgumentTypes;

  std::vector<llvm::Type *> ResultTypes;

  struct {
    size_t ArgumentEltCount;

    size_t ResultEltCount;

  } Discriminator;
};

} // namespace w2n::irgen

namespace llvm {

template <>
struct DenseMapInfo<w2n::irgen::VectorTyKey> {
  using VectorTyKey = w2n::irgen::VectorTyKey;

  static inline VectorTyKey getEmptyKey() {
    return {nullptr, 0};
  }

  static inline VectorTyKey getTombstoneKey() {
    return {nullptr, UINT32_MAX};
  }

  static unsigned getHashValue(const VectorTyKey& Key) {
    return hash_combine(Key.ElementTy, Key.Count);
  }

  static bool isEqual(const VectorTyKey& LHS, const VectorTyKey& RHS) {
    return LHS.ElementTy == RHS.ElementTy && LHS.Count == RHS.Count;
  }
};

template <>
struct DenseMapInfo<w2n::irgen::StructTyKey> {
  using StructTyKey = w2n::irgen::StructTyKey;

  static inline StructTyKey getEmptyKey() {
    return {{}, false};
  }

  static inline StructTyKey getTombstoneKey() {
    return {{}, true};
  }

  static unsigned getHashValue(const StructTyKey& Key) {
    return hash_combine(Key.ElementTypes, Key.IsTombstone);
  }

  static bool isEqual(const StructTyKey& LHS, const StructTyKey& RHS) {
    return LHS.ElementTypes == RHS.ElementTypes
        && LHS.IsTombstone == RHS.IsTombstone;
  }
};

template <>
struct DenseMapInfo<w2n::irgen::FuncTyKey> {
  using FuncTyKey = w2n::irgen::FuncTyKey;

  static inline FuncTyKey getEmptyKey() {
    return {{}, {}, {0, 0}};
  }

  static inline FuncTyKey getTombstoneKey() {
    return {{}, {}, {SIZE_T_MAX, SIZE_T_MAX}};
  }

  static unsigned getHashValue(const FuncTyKey& Key) {
    return hash_combine(
      Key.ArgumentTypes,
      Key.ResultTypes,
      Key.Discriminator.ArgumentEltCount,
      Key.Discriminator.ResultEltCount
    );
  }

  static bool isEqual(const FuncTyKey& LHS, const FuncTyKey& RHS) {
    return LHS.ArgumentTypes == RHS.ArgumentTypes
        && LHS.ResultTypes == RHS.ResultTypes
        && LHS.Discriminator.ArgumentEltCount
             == RHS.Discriminator.ArgumentEltCount
        && LHS.Discriminator.ResultEltCount
             == RHS.Discriminator.ResultEltCount;
  }
};
} // namespace llvm

#endif // W2N_IRGEN_TYPELOWERING_H