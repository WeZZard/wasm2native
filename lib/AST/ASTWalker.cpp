//===--- ASTWalker.cpp - AST Traversal ------------------------------===//
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
//  This file implements a recursive traversal of every node in an AST.
//
//  It's important to update this traversal whenever the AST is
//  changed, whether by adding a new node class or adding a new child
//  to an existing node.  Many walker implementations rely on being
//  invoked with every node in the AST.
//
//  Please follow these general rules when implementing traversal for
//  a node:
//
//    - Every node should be walked.  If a node has both syntactic and
//      semantic components, you should make sure you visit every node
//      in both.
//
//    - Nodes should only be walked once.  So if a node has both
//      syntactic and semantic components, but the type-checker builds
//      the semantic components directly on top of the syntactic
//      components, walking the semantic components will be sufficient
//      to visit all the nodes in both.
//
//    - Explicitly-written nodes should be walked in left-to-right
//      syntactic order.  The ordering of implicit nodes isn't
//      particularly important.
//
//      Note that semantic components will generally preserve the
//      syntactic order of their children because doing something else
//      could illegally change order of evaluation.  This is why, for
//      example, shuffling a TupleExpr creates a DestructureTupleExpr
//      instead of just making a new TupleExpr with the elements in
//      different order.
//
//    - Sub-expressions and sub-statements should be replaceable.
//      It's reasonable to expect that the replacement won't be
//      completely unrelated to the original, but try to avoid making
//      assumptions about the exact representation type.  For example,
//      assuming that a child expression is literally a TupleExpr may
//      only be a reasonable assumption in an unchecked parse tree.
//
//    - Avoid relying on the AST being type-checked or even
//      well-formed during traversal.
//
//===----------------------------------------------------------------===//

#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/ASTWalker.h>

using namespace w2n;

void ASTWalker::anchor() {
}

namespace {

/// Traversal - This class implements a simple expression/statement
/// recursive traverser which queries a user-provided walker class
/// on every node in an AST.
class Traversal :
  public ASTVisitor<
    Traversal,
    Expr *,
    Stmt *,
    /* Decl */ bool> {
private:

  friend class ASTVisitor<
    Traversal,
    Expr *,
    Stmt *,
    /* Decl */ bool>;
  typedef ASTVisitor<Traversal, Expr *, Stmt *, bool> Inherited;

  ASTWalker& Walker;

  /// RAII object that sets the parent of the walk context
  /// appropriately.
  class SetParentRAII {
    ASTWalker& Walker;
    decltype(ASTWalker::Parent) PriorParent;

  public:

    template <typename T>
    SetParentRAII(ASTWalker& Walker, T * NewParent) :
      Walker(Walker),
      PriorParent(Walker.Parent) {
      Walker.Parent = NewParent;
    }

    ~SetParentRAII() {
      Walker.Parent = PriorParent;
    }
  };

  LLVM_NODISCARD
  bool visit(Decl * D) {
    SetParentRAII SetParent(Walker, D);
    return Inherited::visit(D);
  }

  LLVM_NODISCARD
  Expr * visit(Expr * E) {
    SetParentRAII SetParent(Walker, E);
    return Inherited::visit(E);
  }

  LLVM_NODISCARD
  Stmt * visit(Stmt * S) {
    SetParentRAII SetParent(Walker, S);
    return Inherited::visit(S);
  }

  //===--------------------------------------------------------------===//
  //                            Decls
  //===--------------------------------------------------------------===//

#define DECL(Id, Parent) Decl * visit##Id##Decl(Id##Decl * S);
#include <w2n/AST/DeclNodes.def>

  //===--------------------------------------------------------------===//
  //                            Exprs
  //===--------------------------------------------------------------===//

#define EXPR(Id, Parent) Expr * visit##Id##Expr(Id##Expr * S);
#include <w2n/AST/ExprNodes.def>

  //===--------------------------------------------------------------===//
  //                            Stmts
  //===--------------------------------------------------------------===//

#define STMT(Id, Parent) Stmt * visit##Id##Stmt(Id##Stmt * S);
#include <w2n/AST/StmtNodes.def>

  using Action = ASTWalker::Action;

  using PreWalkAction = ASTWalker::PreWalkAction;
  using PostWalkAction = ASTWalker::PostWalkAction;

  template <typename T>
  using PreWalkResult = ASTWalker::PreWalkResult<T>;

  template <typename T>
  using PostWalkResult = ASTWalker::PostWalkResult<T>;

  LLVM_NODISCARD
  bool traverse(
    PreWalkAction Pre,
    llvm::function_ref<bool(void)> VisitChildren,
    llvm::function_ref<PostWalkAction(void)> WalkPost
  ) {
    switch (Pre.Action) {
    case PreWalkAction::Stop: return true;
    case PreWalkAction::SkipChildren: return false;
    case PreWalkAction::Continue: break;
    }
    if (VisitChildren()) {
      return true;
    }
    switch (WalkPost().Action) {
    case PostWalkAction::Stop: return true;
    case PostWalkAction::Continue: return false;
    }
    llvm_unreachable("Unhandled case in switch!");
  }

  template <typename T>
  LLVM_NODISCARD T * traverse(
    PreWalkResult<T *> Pre,
    llvm::function_ref<T *(T *)> VisitChildren,
    llvm::function_ref<PostWalkResult<T *>(T *)> WalkPost
  ) {
    switch (Pre.Action.Action) {
    case PreWalkAction::Stop: return nullptr;
    case PreWalkAction::SkipChildren:
      assert(
        *Pre.Value && "Use Action::Stop instead of returning nullptr"
      );
      return *Pre.Value;
    case PreWalkAction::Continue: break;
    }
    assert(*Pre.Value && "Use Action::Stop instead of returning nullptr");
    auto * Value = VisitChildren(*Pre.Value);
    if (!Value) {
      return nullptr;
    }

    auto Post = WalkPost(Value);
    switch (Post.Action.Action) {
    case PostWalkAction::Stop: return nullptr;
    case PostWalkAction::Continue:
      assert(
        *Post.Value && "Use Action::Stop instead of returning nullptr"
      );
      return *Post.Value;
    }
    llvm_unreachable("Unhandled case in switch!");
  }

public:

  Traversal(ASTWalker& Walker) : Walker(Walker) {
  }

  LLVM_NODISCARD
  Expr * doIt(Expr * E) {
    return traverse<Expr>(
      Walker.walkToExprPre(E),
      [&](Expr * E) { return visit(E); },
      [&](Expr * E) { return Walker.walkToExprPost(E); }
    );
  }

  LLVM_NODISCARD
  Stmt * doIt(Stmt * S) {
    return traverse<Stmt>(
      Walker.walkToStmtPre(S),
      [&](Stmt * S) { return visit(S); },
      [&](Stmt * S) { return Walker.walkToStmtPost(S); }
    );
  }

  /// Returns true on failure.
  LLVM_NODISCARD
  bool doIt(Decl * D) {
    return traverse(
      Walker.walkToDeclPre(D),
      [&]() { return visit(D); },
      [&]() { return Walker.walkToDeclPost(D); }
    );
  }
};

} // end anonymous namespace

#pragma mark Traversal

#pragma mark AST Node Walk

bool Decl::walk(ASTWalker& Walker) {
  return Traversal(Walker).doIt(this);
}

Expr * Expr::walk(ASTWalker& Walker) {
  return Traversal(Walker).doIt(this);
}

Stmt * Stmt::walk(ASTWalker& Walker) {
  return Traversal(Walker).doIt(this);
}
