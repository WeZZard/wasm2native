//===--- ASTWalker.h - Class for walking the AST ----------*- C++ -*-===//
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

#ifndef W2N_AST_ASTWALKER_H
#define W2N_AST_ASTWALKER_H

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/PointerUnion.h>
#include <utility>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class Decl;
class Expr;
class ModuleDecl;
class Stmt;

/// An abstract class used to traverse an AST.
class ASTWalker {
public:

  enum class ParentKind {
    Module,
    Decl,
    Stmt,
    Expr,
  };

  class ParentTy {
    ParentKind Kind;
    void * Ptr = nullptr;

  public:

    ParentTy(ModuleDecl * Mod) : Kind(ParentKind::Module), Ptr(Mod) {
    }

    ParentTy(Decl * D) : Kind(ParentKind::Decl), Ptr(D) {
    }

    ParentTy(Stmt * S) : Kind(ParentKind::Stmt), Ptr(S) {
    }

    ParentTy(Expr * E) : Kind(ParentKind::Expr), Ptr(E) {
    }

    ParentTy() : Kind(ParentKind::Module), Ptr(nullptr) {
    }

    bool isNull() const {
      return Ptr == nullptr;
    }

    ParentKind getKind() const {
      assert(!isNull());
      return Kind;
    }

    ModuleDecl * getAsModule() const {
      return Kind == ParentKind::Module ? static_cast<ModuleDecl *>(Ptr)
                                        : nullptr;
    }

    Decl * getAsDecl() const {
      return Kind == ParentKind::Decl ? static_cast<Decl *>(Ptr)
                                      : nullptr;
    }

    Stmt * getAsStmt() const {
      return Kind == ParentKind::Stmt ? static_cast<Stmt *>(Ptr)
                                      : nullptr;
    }

    Expr * getAsExpr() const {
      return Kind == ParentKind::Expr ? static_cast<Expr *>(Ptr)
                                      : nullptr;
    }
  };

  /// The parent of the node we are visiting.
  ParentTy Parent;

  struct Detail {
    Detail() = delete;

    // The 'Action' set of types, which do not take a payload.
    struct ContinueWalkAction {};

    struct SkipChildrenIfWalkAction {
      bool Cond;
    };

    struct StopIfWalkAction {
      bool Cond;
    };

    struct StopWalkAction {};

    // The 'Result' set of types, which do take a payload.
    template <typename T>
    struct ContinueWalkResult {
      T Value;
    };

    template <typename T>
    struct SkipChildrenIfWalkResult {
      bool Cond;
      T Value;
    };

    template <typename T>
    struct StopIfWalkResult {
      bool Cond;
      T Value;
    };
  };

  /// A namespace for ASTWalker actions that may be returned from pre-walk
  /// and post-walk functions.
  ///
  /// Only certain AST nodes support being replaced during a walk. The
  /// visitation methods for such nodes use the
  /// PreWalkResult/PostWalkResult types, which store both the action to
  /// take, along with the AST node to splice into the tree in place of
  /// the old node. The node must be provided to \c Action::<action>
  /// function, e.g \c Action::Continue(E) or \c Action::SkipChildren(E).
  /// The only exception is \c Action::Stop(), which never accepts a node.
  ///
  /// AST nodes that do not support being replaced during a walk use
  /// visitation methods that return PreWalkAction/PostWalkAction. These
  /// just store the walking action to perform, and you return e.g \c
  /// Action::Continue() or \c Action::SkipChildren().
  ///
  /// Each function here returns a separate underscored type, which is
  /// then consumed by
  /// PreWalkAction/PreWalkResult/PostWalkAction/PostWalkAction. It is
  /// designed this way to achieve a pseudo form of return type
  /// overloading, where e.g \c Action::Continue() can become either a
  /// pre-walk action or a post-walk action, but \c Action::SkipChildren()
  /// can only become a pre-walk action.
  struct Action {
    Action() = delete;

    /// Continue the current walk, replacing the current node with \p
    /// node.
    template <typename T>
    static Detail::ContinueWalkResult<T> Continue(T Node) {
      return {std::move(Node)};
    }

    /// Continue the current walk, replacing the current node with \p
    /// node. However, skip visiting the children of \p node, and instead
    /// resume the walk of the parent node.
    template <typename T>
    static Detail::SkipChildrenIfWalkResult<T> SkipChildren(T Node) {
      return SkipChildrenIf(true, std::move(Node));
    }

    /// If \p cond is true, this is equivalent to \c
    /// Action::SkipChildren(node). Otherwise, it is equivalent to \c
    /// Action::Continue(node).
    template <typename T>
    static Detail::SkipChildrenIfWalkResult<T>
    SkipChildrenIf(bool Cond, T Node) {
      return {Cond, std::move(Node)};
    }

    /// If \p cond is true, this is equivalent to \c
    /// Action::Continue(node). Otherwise, it is equivalent to \c
    /// Action::SkipChildren(node).
    template <typename T>
    static Detail::SkipChildrenIfWalkResult<T>
    VisitChildrenIf(bool Cond, T Node) {
      return SkipChildrenIf(!Cond, std::move(Node));
    }

    /// If \p cond is true, this is equivalent to \c Action::Stop().
    /// Otherwise, it is equivalent to \c Action::Continue(node).
    template <typename T>
    static Detail::StopIfWalkResult<T> StopIf(bool Cond, T Node) {
      return {Cond, std::move(Node)};
    }

    /// Continue the current walk.
    static Detail::ContinueWalkAction Continue() {
      return {};
    }

    /// Continue the current walk, but do not visit the children of the
    /// current node. Instead, resume at the parent's post-walk.
    static Detail::SkipChildrenIfWalkAction SkipChildren() {
      return SkipChildrenIf(true);
    }

    /// If \p cond is true, this is equivalent to \c
    /// Action::SkipChildren(). Otherwise, it is equivalent to \c
    /// Action::Continue().
    static Detail::SkipChildrenIfWalkAction SkipChildrenIf(bool Cond) {
      return {Cond};
    }

    /// If \p cond is true, this is equivalent to \c Action::Continue().
    /// Otherwise, it is equivalent to \c Action::SkipChildren().
    static Detail::SkipChildrenIfWalkAction VisitChildrenIf(bool Cond) {
      return SkipChildrenIf(!Cond);
    }

    /// Terminate the walk, returning without visiting any other nodes.
    static Detail::StopWalkAction Stop() {
      return {};
    }

    /// If \p cond is true, this is equivalent to \c Action::Stop().
    /// Otherwise, it is equivalent to \c Action::Continue().
    static Detail::StopIfWalkAction StopIf(bool Cond) {
      return {Cond};
    }
  };

  /// Do not construct directly, use \c Action::<action> instead.
  ///
  /// A pre-visitation action for AST nodes that do not support being
  /// replaced while walking.
  struct PreWalkAction {
    enum Kind {
      Stop,
      SkipChildren,
      Continue
    };

    Kind Action;

    PreWalkAction(Kind Action) : Action(Action) {
    }

    PreWalkAction(Detail::ContinueWalkAction _) : Action(Continue) {
    }

    PreWalkAction(Detail::StopWalkAction _) : Action(Stop) {
    }

    PreWalkAction(Detail::SkipChildrenIfWalkAction Action) :
      Action(Action.Cond ? SkipChildren : Continue) {
    }

    PreWalkAction(Detail::StopIfWalkAction Action) :
      Action(Action.Cond ? Stop : Continue) {
    }
  };

  /// Do not construct directly, use \c Action::<action> instead.
  ///
  /// A post-visitation action for AST nodes that do not support being
  /// replaced while walking.
  struct PostWalkAction {
    enum Kind {
      Stop,
      Continue
    };

    Kind Action;

    PostWalkAction(Kind Action) : Action(Action) {
    }

    PostWalkAction(Detail::ContinueWalkAction _) : Action(Continue) {
    }

    PostWalkAction(Detail::StopWalkAction _) : Action(Stop) {
    }

    PostWalkAction(Detail::StopIfWalkAction Action) :
      Action(Action.Cond ? Stop : Continue) {
    }
  };

  /// Do not construct directly, use \c Action::<action> instead.
  ///
  /// A pre-visitation result for AST nodes that support being replaced
  /// while walking. Stores both the walking action to take, along with
  /// the node to splice into the AST in place of the old node.
  template <typename T>
  struct PreWalkResult {
    PreWalkAction Action;
    Optional<T> Value;

    template <
      typename U,
      typename std::enable_if<std::is_convertible<U, T>::value>::type * =
        nullptr>
    PreWalkResult(const PreWalkResult<U>& Other) : Action(Other.Action) {
      if (Other.Value) {
        Value = *Other.Value;
      }
    }

    template <
      typename U,
      typename std::enable_if<std::is_convertible<U, T>::value>::type * =
        nullptr>
    PreWalkResult(PreWalkResult<U>&& Other) : Action(Other.Action) {
      if (Other.Value) {
        Value = std::move(*Other.Value);
      }
    }

    template <typename U>
    PreWalkResult(Detail::ContinueWalkResult<U> Result) :
      Action(PreWalkAction::Continue),
      Value(std::move(Result.Value)) {
    }

    template <typename U>
    PreWalkResult(Detail::SkipChildrenIfWalkResult<U> Result) :
      Action(
        Result.Cond ? PreWalkAction::SkipChildren
                    : PreWalkAction::Continue
      ),
      Value(std::move(Result.Value)) {
    }

    template <typename U>
    PreWalkResult(Detail::StopIfWalkResult<U> Result) :
      Action(Result.Cond ? PreWalkAction::Stop : PreWalkAction::Continue),
      Value(std::move(Result.Value)) {
    }

    PreWalkResult(Detail::StopWalkAction) :
      Action(PreWalkAction::Stop),
      Value(None) {
    }
  };

  /// Do not construct directly, use \c Action::<action> instead.
  ///
  /// A post-visitation result for AST nodes that support being replaced
  /// while walking. Stores both the walking action to take, along with
  /// the node to splice into the AST in place of the old node.
  template <typename T>
  struct PostWalkResult {
    PostWalkAction Action;
    Optional<T> Value;

    template <
      typename U,
      typename std::enable_if<std::is_convertible<U, T>::value>::type * =
        nullptr>
    PostWalkResult(const PostWalkResult<U>& Other) :
      Action(Other.Action) {
      if (Other.Value) {
        Value = *Other.Value;
      }
    }

    template <
      typename U,
      typename std::enable_if<std::is_convertible<U, T>::value>::type * =
        nullptr>
    PostWalkResult(PostWalkResult<U>&& Other) : Action(Other.Action) {
      if (Other.Value) {
        Value = std::move(*Other.Value);
      }
    }

    template <typename U>
    PostWalkResult(Detail::ContinueWalkResult<U> Result) :
      Action(PostWalkAction::Continue),
      Value(std::move(Result.Value)) {
    }

    template <typename U>
    PostWalkResult(Detail::StopIfWalkResult<U> Result) :
      Action(
        Result.Cond ? PostWalkAction::Stop : PostWalkAction::Continue
      ),
      Value(std::move(Result.Value)) {
    }

    PostWalkResult(Detail::StopWalkAction) :
      Action(PostWalkAction::Stop),
      Value(None) {
    }
  };

  /// This method is called when first visiting an expression
  /// before walking into its children.
  ///
  /// \param E The expression to check.
  ///
  /// \returns A result that contains a potentially re-written expression,
  /// along with the walk action to perform. The default implementation
  /// returns \c Action::Continue(E).
  ///
  virtual PreWalkResult<Expr *> walkToExprPre(Expr * E) {
    return Action::Continue(E);
  }

  /// This method is called after visiting an expression's children. If a
  /// new expression is returned, it is spliced in where the old
  /// expression previously appeared.
  ///
  /// \param E The expression that was walked.
  ///
  /// \returns A result that contains a potentially re-written expression,
  /// along with the walk action to perform. The default implementation
  /// returns \c Action::Continue(E).
  ///
  virtual PostWalkResult<Expr *> walkToExprPost(Expr * E) {
    return Action::Continue(E);
  }

  /// This method is called when first visiting a statement before
  /// walking into its children.
  ///
  /// \param S The statement to check.
  ///
  /// \returns A result that contains a potentially re-written statement,
  /// along with the walk action to perform. The default implementation
  /// returns \c Action::Continue(S).
  ///
  virtual PreWalkResult<Stmt *> walkToStmtPre(Stmt * S) {
    return Action::Continue(S);
  }

  /// This method is called after visiting an statements's children. If a
  /// new statement is returned, it is spliced in where the old statement
  /// previously appeared.
  ///
  /// \param S The statement that was walked.
  ///
  /// \returns A result that contains a potentially re-written statement,
  /// along with the walk action to perform. The default implementation
  /// returns \c Action::Continue(S).
  ///
  virtual PostWalkResult<Stmt *> walkToStmtPost(Stmt * S) {
    return Action::Continue(S);
  }

  /// walkToDeclPre - This method is called when first visiting a decl,
  /// before walking into its children.
  ///
  /// \param D The declaration to check. The callee may update this
  /// declaration in-place.
  ///
  /// \returns The walking action to perform. By default, this
  /// is \c Action::Continue().
  ///
  virtual PreWalkAction walkToDeclPre(Decl * D) {
    return Action::Continue();
  }

  /// walkToDeclPost - This method is called after visiting the children
  /// of a decl.
  ///
  /// \param D The declaration that was walked.
  ///
  /// \returns The walking action to perform. By default, this
  /// is \c Action::Continue().
  ///
  virtual PostWalkAction walkToDeclPost(Decl * D) {
    return Action::Continue();
  }

protected:

  ASTWalker() = default;
  ASTWalker(const ASTWalker&) = default;
  virtual ~ASTWalker() = default;

  virtual void anchor();
};

} // namespace w2n

#endif // W2N_AST_ASTWALKER_H
