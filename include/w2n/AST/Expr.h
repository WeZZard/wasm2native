#ifndef W2N_AST_EXPR_H
#define W2N_AST_EXPR_H

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include <cstdint>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMRTTI.h>
#include <w2n/Basic/SourceLoc.h>

namespace w2n {

enum class ExprKind : uint8_t {
#define EXPR(Id, Parent) Id,
#define LAST_EXPR(Id)    Last_Expr = Id,
#define EXPR_RANGE(Id, FirstId, LastId)                                  \
  First_##Id##Expr = FirstId, Last_##Id##Expr = LastId,
#include <w2n/AST/ExprNodes.def>
};

/// Expr - Base class for all expressions in w2n.
class LLVM_POINTER_LIKE_ALIGNMENT(Expr) Expr : public ASTAllocated<Expr> {
  Expr(const Expr&) = delete;
  void operator=(const Expr&) = delete;

protected:

  ExprKind Kind;

private:

  /// Ty - This is the type of the expression.
  Type * Ty;

protected:

  Expr(ExprKind Kind, Type * Ty) : Kind(Kind), Ty(Ty) {
  }

public:

  /// Return the kind of this expression.
  ExprKind getKind() const {
    return Kind;
  }

  /// Retrieve the name of the given expression kind.
  ///
  /// This name should only be used for debugging dumps and other
  /// developer aids, and should never be part of a diagnostic or exposed
  /// to the user of the compiler in any way.
  static StringRef getKindName(ExprKind K);

  /// getType - Return the type of this expression.
  Type * getType() const {
    return Ty;
  }

  /// setType - Sets the type of this expression.
  void setType(Type * T);

  /// Return the source range of the expression.
  SourceRange getSourceRange() const {
    w2n_proto_implemented([] { return SourceRange(); });
  }

  /// getStartLoc - Return the location of the start of the expression.
  SourceLoc getStartLoc() const;

  /// Retrieve the location of the last token of the expression.
  SourceLoc getEndLoc() const;

  /// getLoc - Return the caret location of this expression.
  SourceLoc getLoc() const;

  /// This recursively walks the AST rooted at this expression.
  // FIXME: Expr * walk(ASTWalker& walker);

  // FIXME: Expr * walk(ASTWalker&& walker);

  W2N_DEBUG_DUMP;

  void dump(raw_ostream& OS, unsigned Indent = 0) const {
    w2n_proto_implemented([] {});
  }

  // FIXME: void print(ASTPrinter& P, const PrintOptions& Opts) const;
};

class CallExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Call);
};

class CallIndirectExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, CallIndirect);
};

class DropExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Drop);
};

class LocalGetExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, LocalGet);
};

class LocalSetExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, LocalSet);
};

class GlobalGetExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, GlobalGet);
};

class GlobalSetExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, GlobalSet);
};

class LoadExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Load);
};

class StoreExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Store);
};

class ConstExpr : public Expr {
protected:

  ConstExpr(ExprKind Kind, Type * Ty) : Expr(Kind, Ty) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Expr, ConstExpr);
};

class IntegerConstExpr : public ConstExpr {
private:

  llvm::APInt Value;

  IntegerConstExpr(llvm::APInt Value, IntegerType * Ty) :
    ConstExpr(ExprKind::IntegerConst, Ty),
    Value(Value) {
  }

public:

  IntegerType * getIntegerType() {
    return dyn_cast<IntegerType>(getType());
  }

  const IntegerType * getIntegerType() const {
    return dyn_cast<IntegerType>(getType());
  }

  static IntegerConstExpr *
  create(ASTContext& Ctx, llvm::APInt Value, IntegerType * Ty) {
    return new (&Ctx) IntegerConstExpr(Value, Ty);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, IntegerConst);
};

class FloatConstExpr : public ConstExpr {
private:

  llvm::APFloat Value;

  FloatConstExpr(llvm::APFloat Value, FloatType * Ty) :
    ConstExpr(ExprKind::FloatConst, Ty),
    Value(Value) {
  }

public:

  FloatType * getFloatType() {
    return dyn_cast<FloatType>(getType());
  }

  const FloatType * getFloatType() const {
    return dyn_cast<FloatType>(getType());
  }

  static FloatConstExpr *
  create(ASTContext& Ctx, llvm::APFloat Value, FloatType * Ty) {
    return new (&Ctx) FloatConstExpr(Value, Ty);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, FloatConst);
};

class CallBuiltinExpr : public Expr {
public:

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, CallBuiltin);
};

} // namespace w2n

#endif // W2N_AST_EXPR_H