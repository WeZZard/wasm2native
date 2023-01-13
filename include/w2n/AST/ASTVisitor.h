//===--- ASTVisitor.h - Decl, Expr and Stmt Visitor -------*- C++ -*-===//
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
// This file defines the ASTVisitor class, and the DeclVisitor,
// ExprVisitor, and StmtVisitor template typedefs.
//
//===----------------------------------------------------------------===//

#ifndef SWIFT_AST_ASTVISITOR_H
#define SWIFT_AST_ASTVISITOR_H

#include <llvm/Support/ErrorHandling.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Expr.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Stmt.h>

namespace w2n {

/// ASTVisitor - This is a simple visitor class for Swift expressions.
template <
  typename ImplClass,
  typename ExprRetTy = void,
  typename StmtRetTy = void,
  typename DeclRetTy = void,
  typename... Args>
class ASTVisitor {
public:

  typedef ASTVisitor ASTVisitorType;

  DeclRetTy visit(Decl * D, Args... AA) {
    switch (D->getKind()) {
#define DECL(CLASS, PARENT)                                              \
  case DeclKind::CLASS:                                                  \
    return static_cast<ImplClass *>(this)->visit##CLASS##Decl(           \
      static_cast<CLASS##Decl *>(D), ::std::forward<Args>(AA)...         \
    );
#include <w2n/AST/DeclNodes.def>
    }
    llvm_unreachable("Not reachable, all cases handled");
  }

#define DECL(CLASS, PARENT)                                              \
  DeclRetTy visit##CLASS##Decl(CLASS##Decl * D, Args... AA) {            \
    return static_cast<ImplClass *>(this)->visit##PARENT(                \
      D, ::std::forward<Args>(AA)...                                     \
    );                                                                   \
  }
#define ABSTRACT_DECL(CLASS, PARENT) DECL(CLASS, PARENT)
#include <w2n/AST/DeclNodes.def>

  ExprRetTy visit(Expr * E, Args... AA) {
    switch (E->getKind()) {
#define EXPR(CLASS, PARENT)                                              \
  case ExprKind::CLASS:                                                  \
    return static_cast<ImplClass *>(this)->visit##CLASS##Expr(           \
      static_cast<CLASS##Expr *>(E), ::std::forward<Args>(AA)...         \
    );
#include <w2n/AST/ExprNodes.def>
    }
    llvm_unreachable("Not reachable, all cases handled");
  }

  // Provide default implementations of abstract "visit" implementations
  // that just chain to their base class.  This allows visitors to just
  // implement the base behavior and handle all subclasses if they desire.
  // Since this is a template, it will only instantiate cases that are
  // used and thus we still require full coverage of the AST nodes by the
  // visitor.
#define ABSTRACT_EXPR(CLASS, PARENT)                                     \
  ExprRetTy visit##CLASS##Expr(CLASS##Expr * E, Args... AA) {            \
    return static_cast<ImplClass *>(this)->visit##PARENT(                \
      E, ::std::forward<Args>(AA)...                                     \
    );                                                                   \
  }
#define EXPR(CLASS, PARENT) ABSTRACT_EXPR(CLASS, PARENT)
#include <w2n/AST/ExprNodes.def>

  StmtRetTy visit(Stmt * S, Args... AA) {
    switch (S->getKind()) {
#define STMT(CLASS, PARENT)                                              \
  case StmtKind::CLASS:                                                  \
    return static_cast<ImplClass *>(this)->visit##CLASS##Stmt(           \
      static_cast<CLASS##Stmt *>(S), ::std::forward<Args>(AA)...         \
    );
#include <w2n/AST/StmtNodes.def>
    }
    llvm_unreachable("Not reachable, all cases handled");
  }
};

template <typename ImplClass, typename ExprRetTy = void, typename... Args>
using ExprVisitor = ASTVisitor<ImplClass, ExprRetTy, void, void, Args...>;

template <typename ImplClass, typename StmtRetTy = void, typename... Args>
using StmtVisitor = ASTVisitor<ImplClass, void, StmtRetTy, void, Args...>;

template <typename ImplClass, typename DeclRetTy = void, typename... Args>
using DeclVisitor = ASTVisitor<ImplClass, void, void, DeclRetTy, Args...>;

} // namespace w2n

#endif
