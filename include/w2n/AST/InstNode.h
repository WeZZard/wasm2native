//===--- InstNode.h - Web Assembly Instructions -----------*- C++ -*-===//
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
// This file defines the InstNode, which is a union of Stmt, Expr,
// Pattern, TypeRepr, StmtCondition, and CaseLabelItem.
//
//===----------------------------------------------------------------===//

#ifndef W2N_AST_INSTNODE_H
#define W2N_AST_INSTNODE_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/PointerUnion.h>
#include <w2n/AST/ASTWalker.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/Basic/Debug.h>
#include <w2n/Basic/Unimplemented.h>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace w2n {
class Expr;
class Stmt;
class EndStmt;
class DeclContext;
class SourceLoc;
class SourceRange;
enum class ExprKind : uint8_t;
enum class StmtKind : uint8_t;

struct InstNode : public llvm::PointerUnion<Expr *, Stmt *> {
  // Inherit the constructors from PointerUnion.
  using PointerUnion::PointerUnion;

  SourceRange getSourceRange() const;

  /// Return the location of the start of the statement.
  SourceLoc getStartLoc() const;

  /// Return the location of the end of the statement.
  SourceLoc getEndLoc() const;

  InstNode walk(ASTWalker& Walker);

  InstNode walk(ASTWalker&& Walker) {
    return walk(Walker);
  }

  /// Provides some utilities to decide detailed node kind.
#define W2N_IS_NODE(T) bool is##T(T##Kind Kind) const;
  W2N_IS_NODE(Stmt)
  W2N_IS_NODE(Expr)
#undef W2N_IS_NODE

  W2N_DEBUG_DUMP;
  void dump(llvm::raw_ostream& OS, unsigned Indent = 0) const;

  friend llvm::hash_code hash_value(InstNode N) {
    return llvm::hash_value(N.getOpaqueValue());
  }
};
} // namespace w2n

namespace llvm {
using w2n::InstNode;

template <>
struct DenseMapInfo<InstNode> {
  static inline InstNode getEmptyKey() {
    return DenseMapInfo<w2n::Expr *>::getEmptyKey();
  }

  static inline InstNode getTombstoneKey() {
    return DenseMapInfo<w2n::Expr *>::getTombstoneKey();
  }

  static unsigned getHashValue(const InstNode Val) {
    return DenseMapInfo<void *>::getHashValue(Val.getOpaqueValue());
  }

  static bool isEqual(const InstNode LHS, const InstNode RHS) {
    return LHS.getOpaqueValue() == RHS.getOpaqueValue();
  }
};
} // namespace llvm

#endif // W2N_AST_INSTNODE_H
