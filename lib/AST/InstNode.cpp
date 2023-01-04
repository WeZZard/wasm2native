//===--- InstNode.cpp - Web Assembly Instructions -------------------===//
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
// This file implements the InstNode, which is a union of Stmt and Expr.
//
//===----------------------------------------------------------------===//

#include <w2n/AST/Expr.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/Stmt.h>
#include <w2n/Basic/SourceLoc.h>

using namespace w2n;

SourceRange InstNode::getSourceRange() const {
  if (const auto * E = this->dyn_cast<Expr *>()) {
    return E->getSourceRange();
  }
  if (const auto * S = this->dyn_cast<Stmt *>()) {
    return S->getSourceRange();
  }
  llvm_unreachable("unsupported instruction node");
}

/// Return the location of the start of the statement.
SourceLoc InstNode::getStartLoc() const {
  return getSourceRange().Start;
}

/// Return the location of the end of the statement.
SourceLoc InstNode::getEndLoc() const {
  return getSourceRange().End;
}

void InstNode::dump(raw_ostream& OS, unsigned Indent) const {
  if (isNull()) {
    OS << "(null)";
  } else if (auto S = dyn_cast<Stmt *>()) {
    S->dump(OS, /*context=*/nullptr, Indent);
  } else if (auto E = dyn_cast<Expr *>()) {
    E->dump(OS, Indent);
  } else {
    llvm_unreachable("unsupported AST node");
  }
}

void InstNode::dump() const {
  dump(llvm::errs());
}

#define FUNC(T)                                                          \
  bool InstNode::is##T(T##Kind Kind) const {                             \
    if (!is<T *>())                                                      \
      return false;                                                      \
    return get<T *>()->getKind() == Kind;                                \
  }
FUNC(Stmt)
FUNC(Expr)
#undef FUNC

bool InstNode::isEndStmt() const {
  return isStmt(StmtKind::End);
}