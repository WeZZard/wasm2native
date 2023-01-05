#ifndef W2N_AST_EXPR_H
#define W2N_AST_EXPR_H

#include <_types/_uint32_t.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include <cstdint>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Identifier.h>
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
private:

  uint32_t FuncIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  CallExpr(uint32_t FuncIndex) :
    Expr(ExprKind::Call, nullptr),
    FuncIndex(FuncIndex) {
  }

public:

  static CallExpr * create(ASTContext& Context, uint32_t FuncIndex) {
    return new (Context) CallExpr(FuncIndex);
  }

  uint32_t getFuncIndex() const {
    return FuncIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Call);
};

class CallIndirectExpr : public Expr {
private:

  uint32_t TypeIndex;

  uint32_t TableIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  CallIndirectExpr(uint32_t TypeIndex, uint32_t TableIndex) :
    Expr(ExprKind::CallIndirect, nullptr),
    TypeIndex(TypeIndex),
    TableIndex(TableIndex) {
  }

public:

  static CallIndirectExpr *
  create(ASTContext& Context, uint32_t TypeIndex, uint32_t TableIndex) {
    return new (Context) CallIndirectExpr(TypeIndex, TableIndex);
  }

  uint32_t getTypeIndex() const {
    return TypeIndex;
  }

  uint32_t getTableIndex() const {
    return TableIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, CallIndirect);
};

class DropExpr : public Expr {
private:

  /// FIXME: May need \c void type here.
  DropExpr() : Expr(ExprKind::Drop, nullptr) {
  }

public:

  static DropExpr * create(ASTContext& Context) {
    return new (Context) DropExpr();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Drop);
};

class LocalGetExpr : public Expr {
private:

  uint32_t LocalIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  LocalGetExpr(uint32_t LocalIndex) :
    Expr(ExprKind::LocalGet, nullptr),
    LocalIndex(LocalIndex) {
  }

public:

  static LocalGetExpr * create(ASTContext& Context, uint32_t LocalIndex) {
    return new (Context) LocalGetExpr(LocalIndex);
  }

  uint32_t getLocalIndex() const {
    return LocalIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, LocalGet);
};

class LocalSetExpr : public Expr {
private:

  uint32_t LocalIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  LocalSetExpr(uint32_t LocalIndex) :
    Expr(ExprKind::LocalSet, nullptr),
    LocalIndex(LocalIndex) {
  }

public:

  static LocalSetExpr * create(ASTContext& Context, uint32_t LocalIndex) {
    return new (Context) LocalSetExpr(LocalIndex);
  }

  uint32_t getLocalIndex() const {
    return LocalIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, LocalSet);
};

class GlobalGetExpr : public Expr {
private:

  uint32_t GlobalIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  GlobalGetExpr(uint32_t GlobalIndex) :
    Expr(ExprKind::GlobalGet, nullptr),
    GlobalIndex(GlobalIndex) {
  }

public:

  static GlobalGetExpr *
  create(ASTContext& Context, uint32_t GlobalIndex) {
    return new (Context) GlobalGetExpr(GlobalIndex);
  }

  uint32_t getGlobalIndex() const {
    return GlobalIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, GlobalGet);
};

class GlobalSetExpr : public Expr {
private:

  uint32_t GlobalIndex;

  // FIXME: Can have type since wasm file is one-pass parsable.
  GlobalSetExpr(uint32_t GlobalIndex) :
    Expr(ExprKind::GlobalSet, nullptr),
    GlobalIndex(GlobalIndex) {
  }

public:

  static GlobalSetExpr *
  create(ASTContext& Context, uint32_t GlobalIndex) {
    return new (Context) GlobalSetExpr(GlobalIndex);
  }

  uint32_t getGlobalIndex() const {
    return GlobalIndex;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, GlobalSet);
};

class LoadExpr : public Expr {
private:

  ValueType * SourceType;

  ValueType * DestinationType;

  LoadExpr(ValueType * SourceType, ValueType * DestinationType) :
    Expr(ExprKind::Load, DestinationType) {
  }

public:

  static LoadExpr * create(
    ASTContext& Context,
    ValueType * SourceType,
    ValueType * DestinationType
  ) {
    return new (Context) LoadExpr(SourceType, DestinationType);
  }

  ValueType * getSourceType() {
    return SourceType;
  }

  const ValueType * getSourceType() const {
    return SourceType;
  }

  ValueType * getDestinationType() {
    return DestinationType;
  }

  const ValueType * getDestinationType() const {
    return DestinationType;
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, Load);
};

class StoreExpr : public Expr {
private:

  ValueType * SourceType;

  ValueType * DestinationType;

  StoreExpr(ValueType * SourceType, ValueType * DestinationType) :
    Expr(ExprKind::Store, DestinationType) {
  }

public:

  static StoreExpr * create(
    ASTContext& Context,
    ValueType * SourceType,
    ValueType * DestinationType
  ) {
    return new (Context) StoreExpr(SourceType, DestinationType);
  }

  ValueType * getSourceType() {
    return SourceType;
  }

  const ValueType * getSourceType() const {
    return SourceType;
  }

  ValueType * getDestinationType() {
    return DestinationType;
  }

  const ValueType * getDestinationType() const {
    return DestinationType;
  }

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
  Identifier BuiltinName;

  CallBuiltinExpr(Identifier BuiltinName, ValueType * Ty) :
    Expr(ExprKind::CallBuiltin, Ty),
    BuiltinName(BuiltinName) {
  }

public:

  Identifier getBuiltinName() const {
    return BuiltinName;
  }

  static CallBuiltinExpr *
  create(ASTContext& Context, Identifier BuiltinName, ValueType * Ty) {
    new (Context) CallBuiltinExpr(BuiltinName, Ty);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Expr, CallBuiltin);
};

} // namespace w2n

#endif // W2N_AST_EXPR_H