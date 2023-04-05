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

#ifndef IRGEN_REDUCTION_H
#define IRGEN_REDUCTION_H

#include "Address.h"
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

namespace irgen {

enum class ExecutionStackRecordKind {
  Unspecified,
  Operand,
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

  Label(Label&& X) {
    *this = std::move(X);
  }

  Label& operator=(Label&& X) {
    this->Builder = X.Builder;
    this->EnterBB = X.EnterBB;
    this->ExitBB = X.ExitBB;
    this->DebugLabel = X.DebugLabel;
    X.Builder = nullptr;
    X.EnterBB = nullptr;
    X.ExitBB = nullptr;
    X.DebugLabel = nullptr;
    return *this;
  }

  llvm::BasicBlock * getEnterBB();

  const llvm::BasicBlock * getEnterBB() const;

  llvm::BasicBlock * getExitBB();

  const llvm::BasicBlock * getExitBB() const;

  llvm::DILabel * getDebugLabel();

  const llvm::DILabel * getDebugLabel() const;

  static ExecutionStackRecordKind kindof() {
    return ExecutionStackRecordKind::Label;
  }
};

/// Represents an instruction operands on the execution stack. The IR-gen
/// process calls this a.k.a. r-value.
///
class Operand {
private:

  llvm::Value * Val;

public:

  explicit Operand(llvm::Value * Val) : Val(Val) {
  }

  // cannot copy, only can move.
  Operand(const Operand&) = delete;
  Operand& operator=(const Operand&) = delete;

  Operand(Operand&& X) {
    *this = std::move(X);
  }

  Operand& operator=(Operand&& X) {
    this->Val = X.Val;
    X.Val = nullptr;
    return *this;
  }

  llvm::Value * getLowered() {
    return Val;
  }

  const llvm::Value * getLowered() const {
    return Val;
  }

  bool isNull() const {
    return Val == nullptr;
  }

  static ExecutionStackRecordKind kindof() {
    return ExecutionStackRecordKind::Operand;
  }
};

/// Represents the active record of a function call.
///
class Frame {
private:

  w2n::Function * Func;

  std::vector<Address> Locals;

  Address Return;

public:

  explicit Frame(
    w2n::Function * Func, std::vector<Address>&& Locals, Address&& Returns
  ) :
    Func(Func),
    Locals(std::move(Locals)),
    Return(std::move(Returns)) {
  }

  // cannot copy, only can move.
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;

  Frame(Frame&& X) :
    Func(std::move(X.Func)),
    Locals(std::move(X.Locals)),
    Return(std::move(X.Return)) {
    X.Func = nullptr;
  }

  Frame& operator=(Frame&& X) {
    this->Func = X.Func;
    X.Func = nullptr;
    this->Locals = std::move(X.Locals);
    this->Return = std::move(X.Return);
    return *this;
  }

  w2n::Function * getFunc() {
    return Func;
  }

  const w2n::Function * getFunc() const {
    return Func;
  }

  std::vector<Address>& getLocals() {
    return Locals;
  }

  const std::vector<Address>& getLocals() const {
    return Locals;
  }

  Address& getReturn() {
    return Return;
  }

  const Address& getReturn() const {
    return Return;
  }

  bool hasNoReturn() const {
    return getFunc()
      ->getType()
      ->getType()
      ->getReturns()
      ->getValueTypes()
      .empty();
  }

  static ExecutionStackRecordKind kindof() {
    return ExecutionStackRecordKind::Frame;
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
      Operand V;
      Label L;
    };

    ExecutionStackRecordKind Kind;

    Node(Operand&& V, Node * Prev) :
      Prev(Prev),
      V(std::move(V)),
      Kind(Operand::kindof()) {
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
    create(ASTContext& Ctx, Operand&& V, Node * Prev = nullptr) {
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
      case ExecutionStackRecordKind::Operand: V.~Operand(); break;
      case ExecutionStackRecordKind::Frame: F.~Frame(); break;
      case ExecutionStackRecordKind::Label: L.~Label(); break;
      case ExecutionStackRecordKind::Unspecified:
        llvm_unreachable("unspecified node kind.");
      }
    }

    Node * getPrevious() {
      return Prev;
    }

    const Node * getPrevious() const {
      return Prev;
    }

    ExecutionStackRecordKind getKind() {
      return Kind;
    }

    ExecutionStackRecordKind getKind() const {
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
    Operand& get<Operand>() {
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

  Configuration(
    ASTContext * Context,
    w2n::Function * Func,
    std::vector<Address>&& Locals,
    Address&& Return
  ) :
    Configuration(
      Context, Frame(Func, std::move(Locals), std::move(Return))
    ) {
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
  void push(Args&&... AA) {
    Top = Node::create(
      *Context, std::move(ContentTy(std::forward<Args>(AA)...)), Top
    );
  }

  template <typename ContentTy>
  ContentTy * pop() {
    Node * Popped = Top;
    Top = Top->getPrevious();
    return &Popped->assertingGet<ContentTy>();
  }

  ExecutionStackRecordKind pop() {
    Node * Popped = Top;
    Top = Top->getPrevious();
    return Popped->getKind();
  }

  /// Returns the top content's as \c ContentTy .
  ///
  /// There is an assertion inside the type casting process.
  ///
  template <typename ContentTy>
  ContentTy& top() {
    return Top->assertingGet<ContentTy>();
  }

  /// Returns the top content's as \c ContentTy .
  ///
  /// There is an assertion inside the type casting process.
  ///
  template <typename ContentTy>
  const ContentTy& top() const {
    return Top->assertingGet<ContentTy>();
  }

  ExecutionStackRecordKind topKind() const {
    return Top->getKind();
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

  template <typename ContentTy>
  std::vector<ContentTy *> pop(uint32_t K) {
    std::vector<ContentTy *> PoppedContents;
    pop(PoppedContents, K);
    return PoppedContents;
  }

  template <typename ContentTy>
  ContentTy * findTopmostNth(uint32_t N) const {
    assert(N >= 1);
    bool ShouldStop = false;
    Node * ExaminedNode = Top;
    while (!ShouldStop) {
      Node * PreviousNode = ExaminedNode;
      ExaminedNode = ExaminedNode->getPrevious();
      if (PreviousNode->getKind() == ContentTy::kindof()) {
        N -= 1;
        if (N == 0) {
          ShouldStop = true;
          return &PreviousNode->get<ContentTy>();
        }
      }
    };
    return nullptr;
  }

  template <typename ContentTy>
  ContentTy * findTopmost() const {
    return findTopmostNth<ContentTy>(1);
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

} // namespace irgen

} // namespace w2n

#endif // IRGEN_REDUCTION_H
