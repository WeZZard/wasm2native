//===--- DebugTypeInfo.h - Type Info for Debugging --------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
// This file defines the data structure that holds all the debug info
// we want to emit for types.
//
//===----------------------------------------------------------------===//

#ifndef W2N_IRGEN_DEBUGTYPEINFO_H
#define W2N_IRGEN_DEBUGTYPEINFO_H

#include "IRGenInternal.h"
#include <w2n/AST/Decl.h>
#include <w2n/AST/Type.h>

namespace llvm {
class Type;
}

namespace w2n {

namespace irgen {
class TypeInfo;

/// This data structure holds everything needed to emit debug info
/// for a type.
class DebugTypeInfo {
protected:

  /// The type we need to emit may be different from the type
  /// mentioned in the Decl, for example, stripped of qualifiers.
  Type * Ty = nullptr;
  /// Needed to determine the size of basic types and to determine
  /// the storage type for undefined variables.
  llvm::Type * FragmentStorageType = nullptr;
  Optional<Size> SizeInBytes;
  Alignment Align;
  bool DefaultAlignment = true;
  bool IsMetadataType = false;
  bool SizeIsFragmentSize;

public:

  DebugTypeInfo() = default;
  DebugTypeInfo(
    Type * Ty,
    llvm::Type * StorageTy,
    Optional<Size> SizeInBytes,
    Alignment AlignInBytes,
    bool HasDefaultAlignment,
    bool IsMetadataType,
    bool IsFragmentTypeInfo
  );

  // TODO: Create type for a local variable.

  // TODO: Global variables.

  static DebugTypeInfo getFromTypeInfo(
    Type * Ty, const TypeInfo& Info, bool IsFragmentTypeInfo
  );

  Type * getType() const {
    return Ty;
  }

  // i32, i64, f32 and f64 has no user input decl. We have to add things
  // like `BuiltinDecl` to implement this method.
  // TODO: TypeDecl * getDecl() const;

  llvm::Type * getFragmentStorageType() const {
    if (SizeInBytes && SizeInBytes->isZero()) {
      assert(FragmentStorageType && "only defined types may have a size");
    }
    return FragmentStorageType;
  }

  Optional<Size> getTypeSize() const {
    return SizeIsFragmentSize ? llvm::None : SizeInBytes;
  }

  Optional<Size> getRawSize() const {
    return SizeInBytes;
  }

  void setSize(Size NewSize) {
    SizeInBytes = NewSize;
  }

  Alignment getAlignment() const {
    return Align;
  }

  bool isNull() const {
    return Ty == nullptr;
  }

  bool isForwardDecl() const {
    return FragmentStorageType == nullptr;
  }

  bool isMetadataType() const {
    return IsMetadataType;
  }

  bool hasDefaultAlignment() const {
    return DefaultAlignment;
  }

  bool isSizeFragmentSize() const {
    return SizeIsFragmentSize;
  }

  bool operator==(DebugTypeInfo T) const;
  bool operator!=(DebugTypeInfo T) const;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD void dump() const;
#endif
};

/// A DebugTypeInfo with a defined size (that may be 0).
class CompletedDebugTypeInfo : public DebugTypeInfo {
  CompletedDebugTypeInfo(DebugTypeInfo DbgTy) : DebugTypeInfo(DbgTy) {
  }

public:

  static Optional<CompletedDebugTypeInfo> get(DebugTypeInfo DbgTy) {
    if (!DbgTy.getRawSize() || DbgTy.isSizeFragmentSize()) {
      return {};
    }
    return CompletedDebugTypeInfo(DbgTy);
  }

  static Optional<CompletedDebugTypeInfo>
  getFromTypeInfo(Type * Ty, const TypeInfo& Info) {
    return CompletedDebugTypeInfo::get(
      DebugTypeInfo::getFromTypeInfo(Ty, Info, /*IsFragment*/ false)
    );
  }

  Size::IntTy getSizeValue() const {
    return SizeInBytes->getValue();
  }
};

} // namespace irgen
} // namespace w2n

namespace llvm {

// Dense map specialization.
template <>
struct DenseMapInfo<w2n::irgen::DebugTypeInfo> {
  static w2n::irgen::DebugTypeInfo getEmptyKey() {
    return {};
  }

  static w2n::irgen::DebugTypeInfo getTombstoneKey() {
    return w2n::irgen::DebugTypeInfo(
      llvm::DenseMapInfo<w2n::Type *>::getTombstoneKey(),
      nullptr,
      w2n::irgen::Size(0),
      w2n::irgen::Alignment(),
      false,
      false,
      false
    );
  }

  static unsigned getHashValue(w2n::irgen::DebugTypeInfo Val) {
    return DenseMapInfo<w2n::Type *>::getHashValue(Val.getType());
  }

  static bool
  isEqual(w2n::irgen::DebugTypeInfo LHS, w2n::irgen::DebugTypeInfo RHS) {
    return LHS == RHS;
  }
};
} // namespace llvm

#endif // W2N_IRGEN_DEBUGTYPEINFO_H
