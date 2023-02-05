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
#include <w2n/AST/Decl.h>
#include <w2n/AST/Expr.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Stmt.h>
#include <w2n/Basic/Defer.h>
#include <w2n/Basic/Unimplemented.h>

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

#define DECL(Id, Parent) bool visit##Id##Decl(Id##Decl * D);
#include <w2n/AST/DeclNodes.def>

#define EXPR(Id, Parent) Expr * visit##Id##Expr(Id##Expr * E);
#include <w2n/AST/ExprNodes.def>

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

//===----------------------------------------------------------------===//
//                               Decls
//===----------------------------------------------------------------===//

bool Traversal::visitModuleDecl(ModuleDecl * D) {
  return llvm::any_of(D->getSectionDecls(), [this](auto * Sect) {
#define DECL(Id, Parent)
#define SECTION_DECL(Id, Parent)                                         \
  if (doIt(Sect)) {                                                      \
    return true;                                                         \
  }

#include <w2n/AST/DeclNodes.def>
    return false;
  });
}

bool Traversal::visitNameSectionDecl(NameSectionDecl * D) {
#define DECL(Id, Parent)
#define NAME_SUB_SECTION_DECL(Id, Parent)                                \
  if (doIt(D->get##Id())) {                                              \
    return true;                                                         \
  }

#include <w2n/AST/DeclNodes.def>
  return false;
}

bool Traversal::visitTypeSectionDecl(TypeSectionDecl * D) {
  return llvm::any_of(D->getTypes(), [this](auto * FuncType) {
    return doIt(FuncType);
  });
}

bool Traversal::visitImportSectionDecl(ImportSectionDecl * D) {
  return llvm::any_of(D->getImports(), [this](auto * Import) {
#define DECL(Id, Parent)
#define IMPORT_DECL(Id, Parent)                                          \
  if (doIt(Import)) {                                                    \
    return true;                                                         \
  }

#include <w2n/AST/DeclNodes.def>
    return false;
  });
}

bool Traversal::visitFuncSectionDecl(FuncSectionDecl * D) {
  return false;
}

bool Traversal::visitTableSectionDecl(TableSectionDecl * D) {
  return llvm::any_of(D->getTables(), [this](auto * Tables) {
    return doIt(Tables);
  });
}

bool Traversal::visitMemorySectionDecl(MemorySectionDecl * D) {
  return llvm::any_of(D->getMemories(), [this](auto * Memory) {
    return doIt(Memory);
  });
}

bool Traversal::visitGlobalSectionDecl(GlobalSectionDecl * D) {
  return llvm::any_of(D->getGlobals(), [this](auto * Global) {
    return doIt(Global);
  });
}

bool Traversal::visitExportSectionDecl(ExportSectionDecl * D) {
  return llvm::any_of(D->getExports(), [this](auto * Export) {
    return doIt(Export);
  });
}

bool Traversal::visitStartSectionDecl(StartSectionDecl * D) {
  return w2n_proto_implemented([] { return false; });
}

bool Traversal::visitElementSectionDecl(ElementSectionDecl * D) {
  return w2n_proto_implemented([] { return false; });
}

bool Traversal::visitCodeSectionDecl(CodeSectionDecl * D) {
  return llvm::any_of(D->getCodes(), [this](auto * Code) {
    return visitCodeDecl(Code);
  });
}

bool Traversal::visitDataSectionDecl(DataSectionDecl * D) {
  return llvm::any_of(D->getDataSegments(), [this](auto * Data) {
#define DECL(Id, Parent)
#define DATA_SEG_DECL(Id, Parent)                                        \
  if (dyn_cast<Id##Decl>(Data)) {                                        \
    if (visit##Id##Decl(dyn_cast<Id##Decl>(Data))) {                     \
      return true;                                                       \
    }                                                                    \
  }

#include <w2n/AST/DeclNodes.def>
    return false;
  });
}

bool Traversal::visitDataCountSectionDecl(DataCountSectionDecl * D) {
  return w2n_proto_implemented([] { return false; });
}

bool Traversal::visitFuncTypeDecl(FuncTypeDecl * D) {
  return false;
}

bool Traversal::visitImportFuncDecl(ImportFuncDecl * D) {
  return false;
}

bool Traversal::visitImportTableDecl(ImportTableDecl * D) {
  return false;
}

bool Traversal::visitImportMemoryDecl(ImportMemoryDecl * D) {
  return false;
}

bool Traversal::visitImportGlobalDecl(ImportGlobalDecl * D) {
  return false;
}

bool Traversal::visitTableDecl(TableDecl * D) {
  return false;
}

bool Traversal::visitMemoryDecl(MemoryDecl * D) {
  return false;
}

bool Traversal::visitGlobalDecl(GlobalDecl * D) {
  return false;
}

bool Traversal::visitExportTableDecl(ExportTableDecl * D) {
  return false;
}

bool Traversal::visitExportMemoryDecl(ExportMemoryDecl * D) {
  return false;
}

bool Traversal::visitExportGlobalDecl(ExportGlobalDecl * D) {
  return false;
}

bool Traversal::visitCodeDecl(CodeDecl * D) {
  return false;
}

bool Traversal::visitFuncDecl(FuncDecl * D) {
  return false;
}

bool Traversal::visitLocalDecl(LocalDecl * D) {
  return false;
}

bool Traversal::visitDataSegmentActiveDecl(DataSegmentActiveDecl * D) {
  return false;
}

bool Traversal::visitDataSegmentPassiveDecl(DataSegmentPassiveDecl * D) {
  return false;
}

bool Traversal::visitExpressionDecl(ExpressionDecl * D) {
  return false;
}

bool Traversal::visitExportFuncDecl(ExportFuncDecl * D) {
  return false;
}

bool Traversal::visitFuncNameSubsectionDecl(FuncNameSubsectionDecl * D) {
  return false;
}

bool Traversal::visitLocalNameSubsectionDecl(LocalNameSubsectionDecl * D
) {
  return false;
}

bool Traversal::visitModuleNameSubsectionDecl(ModuleNameSubsectionDecl * D
) {
  return false;
}

//===----------------------------------------------------------------===//
//                               Exprs
//===----------------------------------------------------------------===//

Expr * Traversal::visitCallExpr(CallExpr * E) {
  return E;
}

Expr * Traversal::visitDropExpr(DropExpr * E) {
  return E;
}

Expr * Traversal::visitLoadExpr(LoadExpr * E) {
  return E;
}

Expr * Traversal::visitStoreExpr(StoreExpr * E) {
  return E;
}

Expr * Traversal::visitLocalGetExpr(LocalGetExpr * E) {
  return E;
}

Expr * Traversal::visitLocalSetExpr(LocalSetExpr * E) {
  return E;
}

Expr * Traversal::visitGlobalGetExpr(GlobalGetExpr * E) {
  return E;
}

Expr * Traversal::visitGlobalSetExpr(GlobalSetExpr * E) {
  return E;
}

Expr * Traversal::visitFloatConstExpr(FloatConstExpr * E) {
  return E;
}

Expr * Traversal::visitCallBuiltinExpr(CallBuiltinExpr * E) {
  return E;
}

Expr * Traversal::visitCallIndirectExpr(CallIndirectExpr * E) {
  return E;
}

Expr * Traversal::visitIntegerConstExpr(IntegerConstExpr * E) {
  return E;
}

//===----------------------------------------------------------------===//
//                               Stmts
//===----------------------------------------------------------------===//

Stmt * Traversal::visitBrStmt(BrStmt * S) {
  return S;
}

Stmt * Traversal::visitIfStmt(IfStmt * S) {
  return S;
}

Stmt * Traversal::visitEndStmt(EndStmt * S) {
  return S;
}

Stmt * Traversal::visitBrIfStmt(BrIfStmt * S) {
  return S;
}

Stmt * Traversal::visitElseStmt(ElseStmt * S) {
  return S;
}

Stmt * Traversal::visitLoopStmt(LoopStmt * S) {
  return S;
}

Stmt * Traversal::visitBlockStmt(BlockStmt * S) {
  return S;
}

Stmt * Traversal::visitReturnStmt(ReturnStmt * S) {
  return S;
}

Stmt * Traversal::visitBrTableStmt(BrTableStmt * S) {
  return S;
}

Stmt * Traversal::visitUnreachableStmt(UnreachableStmt * S) {
  return S;
}

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
