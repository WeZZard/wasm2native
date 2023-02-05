//===--- Reduction.h - Runtime Stack Reduction *- C++ -------------*-===//
//
// This source file is part of the wasm2native open source project
//
// Copyright (c) 2023 the wasm2native project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://github.com/WeZZard/wasm2native/LICENSE.txt for license
// information
// See https://github.com/WeZZard/wasm2native/CONTRIBUTORS.txt for the
// list of Swift project authors
//
//===----------------------------------------------------------------===//
//
// This file defines the configuration of WebAssembly runtime stack
// reduction.
//
//===----------------------------------------------------------------===//

#ifndef W2N_AST_REDUCTION_H
#define W2N_AST_REDUCTION_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/ErrorHandling.h>
#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Function.h>

namespace w2n {

enum class StackContentKind {
  Unspecified,
  Value,
  Frame,
  Label,
};

/// Represents an active structured control instruction.
///
class Label {
private:

  llvm::IRBuilder<> * Builder;

  llvm::BasicBlock * EnterBB;

  /// Label'c construction guarantees \c ExitBB. Sets the Builder's insert
  /// point to \c ExitBB while deconstructing \c Configuration.
  llvm::BasicBlock * ExitBB;

  llvm::DILabel * DebugLabel;

public:

  explicit Label(
    llvm::IRBuilder<> * Builder,
    llvm::BasicBlock * EnterBB,
    llvm::DILabel * DebugLabel
  );

  // cannot copy, only can move.
  Label(const Label&) = delete;
  Label& operator=(const Label&) = delete;

  Label(Label&&);
  Label& operator=(const Label&&);

  llvm::BasicBlock * getEnterBB();

  const llvm::BasicBlock * getEnterBB() const;

  llvm::BasicBlock * getExitBB();

  const llvm::BasicBlock * getExitBB() const;

  llvm::DILabel * getDebugLabel();

  const llvm::DILabel * getDebugLabel() const;

  static StackContentKind kindof() {
    return StackContentKind::Label;
  }
};

/// Represents an instruction operands.
class Value {
private:

  llvm::Value * Val;

public:

  explicit Value(llvm::Value * Val) : Val(Val) {
  }

  // cannot copy, only can move.
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;

  Value(Value&&);
  Value& operator=(const Value&&);

  static StackContentKind kindof() {
    return StackContentKind::Value;
  }
};

/// Represents an active function call.
///
class Frame {
private:

  w2n::Function * Func;

public:

  explicit Frame(w2n::Function * Func) : Func(Func) {
  }

  // cannot copy, only can move.
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;

  Frame(Frame&&);
  Frame& operator=(const Frame&&);

  static StackContentKind kindof() {
    return StackContentKind::Frame;
  }
};

/// Represents a configuration of WebAsssembly runtime stack reduction.
///
/// When emmiting LLVM-IR for instructions:
/// 1. \c ExpressionDecl have responsibility to create an instance as the
/// stack root.
/// 2. Structured control instructions related \c InstNode umbrellaed
/// classes copy the instance
/// 3. Other instructions related \c InstNode umbrellaed receive the
/// instance by-ref.
///
class Configuration {
public:

private:

  class Node : ASTAllocated<Node> {
    Node * Prev;

    union {
      Frame F;
      Value V;
      Label L;
    };

    StackContentKind Kind;

    Node(Value&& V, Node * Prev) :
      Prev(Prev),
      V(std::move(V)),
      Kind(Value::kindof()) {
    }

    Node(Frame&& F, Node * Prev) :
      Prev(Prev),
      F(std::move(F)),
      Kind(Frame::kindof()) {
    }

    Node(Label&& L, Node * Prev) :
      Prev(Prev),
      L(std::move(L)),
      Kind(Label::kindof()) {
    }

  public:

    static Node *
    create(ASTContext& Ctx, Value&& V, Node * Prev = nullptr) {
      return new (Ctx) Node(std::move(V), Prev);
    }

    static Node *
    create(ASTContext& Ctx, Frame&& F, Node * Prev = nullptr) {
      return new (Ctx) Node(std::move(F), Prev);
    }

    static Node *
    create(ASTContext& Ctx, Label&& L, Node * Prev = nullptr) {
      return new (Ctx) Node(std::move(L), Prev);
    }

    // cannot copy, cannot move.
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    Node(Node&&) = delete;
    Node& operator=(const Node&&) = delete;

    ~Node() {
      switch (Kind) {
      case StackContentKind::Value: V.~Value(); break;
      case StackContentKind::Frame: F.~Frame(); break;
      case StackContentKind::Label: L.~Label(); break;
      case StackContentKind::Unspecified:
        llvm_unreachable("unspecified node kind.");
      }
    }

    Node * getPrevious() {
      return Prev;
    }

    const Node * getPrevious() const {
      return Prev;
    }

    StackContentKind getKind() {
      return Kind;
    }

    StackContentKind getKind() const {
      return Kind;
    }

    template <typename ContentTy>
    ContentTy& get() {
      llvm_unreachable("unexpected ContentTy!");
    }

    template <>
    Frame& get<Frame>() {
      return F;
    }

    template <>
    Value& get<Value>() {
      return V;
    }

    template <>
    Label& get<Label>() {
      return L;
    }

    template <typename ContentTy>
    const ContentTy& get() const {
      return const_cast<const ContentTy&>(
        const_cast<Node *>(this)->get<ContentTy>()
      );
    }

    template <typename ContentTy>
    ContentTy& assertingGet() {
      assert(Kind == ContentTy::kindof());
      return get<ContentTy>();
    }

    template <typename ContentTy>
    const ContentTy& assertingGet() const {
      return const_cast<const ContentTy&>(
        const_cast<Node *>(this)->assertingGet<ContentTy>()
      );
    }
  };

  ASTContext * Context;

  Node * Top;

  std::unique_ptr<std::function<void()>> CleanUp;

public:

  Configuration(ASTContext * Context, Frame&& F) :
    Context(Context),
    Top(Node::create(*Context, std::move(F))) {
  }

  ~Configuration() {
    if (CleanUp) {
      (*CleanUp)();
      CleanUp = nullptr;
    }
  }

  // Cannot move

  Configuration(const Configuration&&) = delete;
  Configuration& operator=(const Configuration&&) = delete;

  // pushing the function frame or structured instruction label would
  // copy the instance directly.

  Configuration(const Configuration& X) {
    *this = X;
  }

  Configuration& operator=(const Configuration& X) {
    Context = X.Context;
    Top = X.Top;
    return *this;
  }

  template <typename ContentTy>
  void push(ContentTy&& C) {
    Top = Node::create(*Context, std::move(C), Top);
  }

  template <typename ContentTy, typename... Args>
  void push(Args&&... args) {
    Top = Node::create(
      *Context, std::move(ContentTy(std::forward<Args>(args)...)), Top
    );
  }

  template <typename ContentTy>
  ContentTy * pop() {
    Node * Popped = Top;
    Top = Top->getPrevious();
    return &Popped->assertingGet<ContentTy>();
  }

  template <typename ContentTy>
  ContentTy& top() {
    return Top->assertingGet<ContentTy>();
  }

  template <typename ContentTy>
  const ContentTy& top() const {
    return Top->assertingGet<ContentTy>();
  }

  template <typename ContentTy>
  std::vector<ContentTy *> pop(uint32_t K) {
    bool ShouldStop = false;
    std::vector<ContentTy *> PoppedContents;
    while (ShouldStop) {
      Node * Popped = Top;
      Top = Top->getPrevious();
      if (Popped->getKind() == ContentTy::kindof()) {
        K -= 1;
        PoppedContents.emplace_back(&Popped->get<ContentTy>());
        if (K == 0) {
          ShouldStop = true;
        }
      }
    };
    return PoppedContents;
  }

  template <typename ContentTy>
  void pop(llvm::SmallVectorImpl<ContentTy>& V, uint32_t K) {
    bool ShouldStop = false;
    while (ShouldStop) {
      Node * Popped = Top;
      Top = Top->getPrevious();
      if (Popped->getKind() == ContentTy::kindof()) {
        K -= 1;
        V.emplace_back(&Popped->get<ContentTy>());
        if (K == 0) {
          ShouldStop = true;
        }
      }
    };
  }

  /// Actions that triggered in \c Configuration destructor.
  ///
  std::function<void()> * getCleanUp() {
    return CleanUp.get();
  }

  /// Actions that triggered in \c Configuration destructor.
  ///
  const std::function<void()> * getCleanUp() const {
    return CleanUp.get();
  }

  /// Set actions that triggered in \c Configuration destructor.
  ///
  /// @note: Structured instruction can make use of this method to add
  /// actions that triggered in \c Configuration destructor.
  ///
  void setCleanUp(std::function<void()> C) {
    CleanUp = std::make_unique<decltype(C)>(C);
  }
};

} // namespace w2n

#endif // W2N_AST_REDUCTION_H
