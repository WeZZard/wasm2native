#ifndef IRGEN_TYPELOWERING_H
#define IRGEN_TYPELOWERING_H

#include <llvm/IR/Type.h>
#include <cstdint>
#include <w2n/Basic/LLVMHashing.h>

#pragma mark Support Types for IRGenModule Lowered Type Memoization

namespace w2n::irgen {

struct VectorTyKey {
  llvm::Type * ElementTy;

  union {
    uint32_t Count;

    struct {
      bool IsEmpty;
      bool IsTombstone;
    };
  };

  VectorTyKey(llvm::Type * ElementTy, uint32_t Count) :
    ElementTy(ElementTy),
    Count(Count) {
  }
};

struct StructTyKey {
  std::vector<llvm::Type *> ElementTypes;
  bool IsEmpty = false;
  bool IsTombstone = false;
};

struct FuncTyKey {
  std::vector<llvm::Type *> ArgumentTypes;

  std::vector<llvm::Type *> ResultTypes;

  struct {
    union {
      size_t ArgumentEltCount;
      bool IsEmpty;
    };

    union {
      size_t ResultEltCount;
      bool IsTombstone;
    };
  };

  FuncTyKey(
    std::vector<llvm::Type *> ArgumentTypes,
    std::vector<llvm::Type *> ResultTypes
  ) :
    ArgumentTypes(ArgumentTypes),
    ResultTypes(ResultTypes),
    ArgumentEltCount(ArgumentTypes.size()),
    ResultEltCount(ResultTypes.size()) {
  }
};

} // namespace w2n::irgen

namespace llvm {

template <>
struct DenseMapInfo<w2n::irgen::VectorTyKey> {
  using VectorTyKey = w2n::irgen::VectorTyKey;

  static inline VectorTyKey getEmptyKey() {
    auto Key = VectorTyKey(nullptr, 0);
    Key.IsEmpty = true;
    return Key;
  }

  static inline VectorTyKey getTombstoneKey() {
    auto Key = VectorTyKey(nullptr, 0);
    Key.IsTombstone = true;
    return Key;
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
    return {{}, true, false};
  }

  static inline StructTyKey getTombstoneKey() {
    return {{}, false, true};
  }

  static unsigned getHashValue(const StructTyKey& Key) {
    return hash_combine(Key.ElementTypes, Key.IsEmpty, Key.IsTombstone);
  }

  static bool isEqual(const StructTyKey& LHS, const StructTyKey& RHS) {
    return LHS.ElementTypes == RHS.ElementTypes
        && LHS.IsEmpty == RHS.IsEmpty
        && LHS.IsTombstone == RHS.IsTombstone;
  }
};

template <>
struct DenseMapInfo<w2n::irgen::FuncTyKey> {
  using FuncTyKey = w2n::irgen::FuncTyKey;

  static inline FuncTyKey getEmptyKey() {
    auto Key = FuncTyKey({}, {});
    Key.IsEmpty = true;
    return Key;
  }

  static inline FuncTyKey getTombstoneKey() {
    auto Key = FuncTyKey({}, {});
    Key.IsTombstone = true;
    return Key;
  }

  static unsigned getHashValue(const FuncTyKey& Key) {
    return hash_combine(
      Key.ArgumentTypes,
      Key.ResultTypes,
      Key.ArgumentEltCount,
      Key.ResultEltCount
    );
  }

  static bool isEqual(const FuncTyKey& LHS, const FuncTyKey& RHS) {
    return LHS.ArgumentTypes == RHS.ArgumentTypes
        && LHS.ResultTypes == RHS.ResultTypes
        && LHS.ArgumentEltCount == RHS.ArgumentEltCount
        && LHS.ResultEltCount == RHS.ResultEltCount;
  }
};
} // namespace llvm

#endif // IRGEN_TYPELOWERING_H
