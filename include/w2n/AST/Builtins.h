//===--- Builtins.h - wasm2native Builtin Functions -------*- C++ -*-===//
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
// This file defines the interface to builtin functions.
//
//===----------------------------------------------------------------===//

#ifndef W2N_AST_BUILTINS_H
#define W2N_AST_BUILTINS_H

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/AtomicOrdering.h>
#include <cstdint>
#include <w2n/AST/ASTContext.h>

namespace w2n {

enum class BuiltinTypeKind : std::underlying_type<TypeKind>::type {
#define TYPE(Id, parent)
#define BUILTIN_TYPE(Id, parent)                                         \
  Id = std::underlying_type<TypeKind>::type(TypeKind::Id),
#include <w2n/AST/TypeNodes.def>
};

/// Get the builtin type for the given name.
///
/// Returns a null type if the name is not a known builtin type name.
Type * getBuiltinType(ASTContext& Context, StringRef Name);

/// OverloadedBuiltinKind - Whether and how a builtin is overloaded.
enum class OverloadedBuiltinKind : uint8_t {
  /// The builtin is not overloaded.
  None,

  /// The builtin is overloaded over all integer types.
  Integer,

  /// The builtin is overloaded over all floating-point types.
  Float,

  /// The builtin is overloaded over all floating-point types and vectors
  /// of floating-point types.
  FloatOrVector,
};

/// BuiltinValueKind - The set of (possibly overloaded) builtin functions.
enum class BuiltinValueKind {
  None = 0,
#define BUILTIN(Id, Name, Attrs) Id,
#include <w2n/AST/Builtins.def>
};

/// Returns true if this is a polymorphic builtin that is only valid
/// in raw sil and thus must be resolved to have concrete types by the
/// time we are in canonical SIL.
bool isPolymorphicBuiltin(BuiltinValueKind Id);

/// Decode the type list of a builtin (e.g. mul_Int32) and return the base
/// name (e.g. "mul").
StringRef getBuiltinBaseName(
  ASTContext& C, StringRef Name, SmallVectorImpl<Type>& Types
);

/// Given an LLVM IR intrinsic name with argument types remove (e.g. like
/// "bswap") return the LLVM IR IntrinsicID for the intrinsic or
/// not_intrinsic (0) if the intrinsic name doesn't match anything.
llvm::Intrinsic::ID getLLVMIntrinsicID(StringRef Name);

/// FIXME: Requires \c getLLVMIntrinsicIDForBuiltinWithOverflow ?

/// Create a ValueDecl for the builtin with the given name.
///
/// Returns null if the name does not identifier a known builtin value.
ValueDecl * getBuiltinValueDecl(ASTContext& Context, Identifier Name);

/// Returns the name of a builtin declaration given a builtin ID.
StringRef getBuiltinName(BuiltinValueKind ID);

/// The information identifying the builtin - its kind and types.
class BuiltinInfo {
public:

  BuiltinValueKind ID;
  SmallVector<Type *, 4> Types;
  bool isReadNone() const;
};

/// The information identifying the llvm intrinsic - its id and types.
class IntrinsicInfo {
  mutable llvm::AttributeList Attrs =
    llvm::DenseMapInfo<llvm::AttributeList>::getEmptyKey();

public:

  llvm::Intrinsic::ID ID;
  SmallVector<Type *, 4> Types;
  const llvm::AttributeList& getOrCreateAttributes(ASTContext& Ctx) const;
};

/// Turn a string like "release" into the LLVM enum.
llvm::AtomicOrdering decodeLLVMAtomicOrdering(StringRef O);

/// Returns true if the builtin with ID \p ID has a defined static
/// overload for the type \p Ty.
bool canBuiltinBeOverloadedForType(BuiltinValueKind ID, Type * Ty);

} // namespace w2n

#endif // W2N_AST_BUILTINS_H
