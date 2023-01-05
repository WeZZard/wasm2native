#ifndef W2N_AST_TYPE_H
#define W2N_AST_TYPE_H

#include "llvm/ADT/Optional.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Wasm.h>
#include <llvm/Support/ErrorHandling.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/Basic/Compiler.h>
#include <w2n/Basic/LLVMRTTI.h>

namespace w2n {

class ASTContext;

enum class TypeKind {
#define TYPE(Id, Parent) Id,
#define LAST_TYPE(Id)    Last_Type = Id,
#define TYPE_RANGE(Id, FirstId, LastId)                                  \
  First_##Id##Type = FirstId, Last_##Id##Type = LastId,
#include <w2n/AST/TypeNodes.def>
};

class Type : public ASTAllocated<Type> {
private:

  TypeKind Kind;

protected:

  Type(TypeKind Kind) : Kind(Kind) {
  }

public:

  TypeKind getKind() const {
    return Kind;
  }

  LLVM_RTTI_CLASSOF_ROOT_CLASS(Type);
};

enum class ValueTypeKind {
#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent) Id,
#include <w2n/AST/TypeNodes.def>
};

class ValueType : public Type {
protected:

  ValueType(TypeKind Kind) : Type(Kind) {
  }

public:

  ValueTypeKind getValueTypeKind() const {
    switch (getKind()) {
#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent)                                           \
  case TypeKind::Id:                                                     \
    return ValueTypeKind::Id;
#include <w2n/AST/TypeNodes.def>
    default: llvm_unreachable("invalid value type kind");
    }
  }

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, ValueType);
};

class NumberType : public ValueType {
protected:

  NumberType(TypeKind Kind) : ValueType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, NumberType);
};

class IntegerType : public NumberType {
protected:

  IntegerType(TypeKind Kind) : NumberType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, IntegerType);
};

class SignedIntegerType : public IntegerType {
protected:

  SignedIntegerType(TypeKind Kind) : IntegerType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, SignedIntegerType);
};

class I8Type : public SignedIntegerType {
private:

  I8Type() : SignedIntegerType(TypeKind::I8) {
  }

public:

  static I8Type * create(ASTContext& Context) {
    return new (Context) I8Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I8);
};

class I16Type : public SignedIntegerType {
private:

  I16Type() : SignedIntegerType(TypeKind::I16) {
  }

public:

  static I16Type * create(ASTContext& Context) {
    return new (Context) I16Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I16);
};

class I32Type : public SignedIntegerType {
private:

  I32Type() : SignedIntegerType(TypeKind::I32) {
  }

public:

  static I32Type * create(ASTContext& Context) {
    return new (Context) I32Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I32);
};

class I64Type : public SignedIntegerType {
private:

  I64Type() : SignedIntegerType(TypeKind::I64) {
  }

public:

  static I64Type * create(ASTContext& Context) {
    return new (Context) I64Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I64);
};

class UnsignedIntegerType : public IntegerType {
protected:

  UnsignedIntegerType(TypeKind Kind) : IntegerType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, UnsignedIntegerType);
};

class U8Type : public UnsignedIntegerType {
private:

  U8Type() : UnsignedIntegerType(TypeKind::U8) {
  }

public:

  static U8Type * create(ASTContext& Context) {
    return new (Context) U8Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, U8);
};

class U16Type : public UnsignedIntegerType {
private:

  U16Type() : UnsignedIntegerType(TypeKind::U16) {
  }

public:

  static U16Type * create(ASTContext& Context) {
    return new (Context) U16Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, U16);
};

class U32Type : public UnsignedIntegerType {
private:

  U32Type() : UnsignedIntegerType(TypeKind::U32) {
  }

public:

  static U32Type * create(ASTContext& Context) {
    return new (Context) U32Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, U32);
};

class U64Type : public UnsignedIntegerType {
private:

  U64Type() : UnsignedIntegerType(TypeKind::U64) {
  }

public:

  static U64Type * create(ASTContext& Context) {
    return new (Context) U64Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, U64);
};

class FloatType : public NumberType {
protected:

  FloatType(TypeKind Kind) : NumberType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, FloatType);
};

class F32Type : public FloatType {
private:

  F32Type() : FloatType(TypeKind::F32) {
  }

public:

  static F32Type * create(ASTContext& Context) {
    return new (Context) F32Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, F32);
};

class F64Type : public FloatType {
private:

  F64Type() : FloatType(TypeKind::F64) {
  }

public:

  static F64Type * create(ASTContext& Context) {
    return new (Context) F64Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, F64);
};

class VectorType : public ValueType {
protected:

  VectorType(TypeKind Kind) : ValueType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, VectorType);
};

class V128Type : public VectorType {
private:

  V128Type() : VectorType(TypeKind::V128) {
  }

public:

  static V128Type * create(ASTContext& Context) {
    return new (Context) V128Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, V128);
};

class ReferenceType : public ValueType {
protected:

  ReferenceType(TypeKind Kind) : ValueType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, ReferenceType);
};

class FuncRefType : public ReferenceType {
private:

  FuncRefType() : ReferenceType(TypeKind::FuncRef) {
  }

public:

  static FuncRefType * create(ASTContext& Context) {
    return new (Context) FuncRefType();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, FuncRef);
};

class ExternRefType : public ReferenceType {
private:

  ExternRefType() : ReferenceType(TypeKind::ExternRef) {
  }

public:

  static ExternRefType * create(ASTContext& Context) {
    return new (Context) ExternRefType();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, ExternRef);
};

class ResultType : public Type {
private:

  std::vector<ValueType *> ValueTypes;

  ResultType(std::vector<ValueType *> ValueTypes) :
    Type(TypeKind::Result),
    ValueTypes(ValueTypes) {
  }

public:

  std::vector<ValueType *>& getValueTypes() {
    return ValueTypes;
  }

  const std::vector<ValueType *>& getValueTypes() const {
    return ValueTypes;
  }

  static ResultType *
  create(ASTContext& Context, std::vector<ValueType *> ValueTypes) {
    return new (Context) ResultType(ValueTypes);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Result);
};

class FuncType : public Type {
private:

  ResultType * Parameters;

  ResultType * Returns;

  FuncType(ResultType * Parameters, ResultType * Returns) :
    Type(TypeKind::Func),
    Parameters(Parameters),
    Returns(Returns) {
  }

public:

  ResultType * getParameters() {
    return Parameters;
  }

  const ResultType * getParameters() const {
    return Parameters;
  }

  ResultType * getReturns() {
    return Returns;
  }

  const ResultType * getReturns() const {
    return Returns;
  }

  static FuncType * create(
    ASTContext& Context, ResultType * Parameters, ResultType * Returns
  ) {
    return new (Context) FuncType(Parameters, Returns);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Func);
};

class LimitsType : public Type {
private:

  uint64_t Min;

  llvm::Optional<uint64_t> Max;

  LimitsType(uint64_t Min, llvm::Optional<uint64_t> Max) :
    Type(TypeKind::Limits),
    Min(Min),
    Max(Max) {
  }

public:

  uint64_t getMin() const {
    return Min;
  }

  bool hasMax() const {
    return Max.has_value();
  }

  llvm::Optional<uint64_t> getMax() const {
    return Max;
  }

  static LimitsType * create(
    ASTContext& Context, uint64_t Min, llvm::Optional<uint64_t> Max
  ) {
    return new (Context) LimitsType(Min, Max);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Limits);
};

class MemoryType : public Type {
private:

  LimitsType * Limits;

  MemoryType(LimitsType * Limits) :
    Type(TypeKind::Memory),
    Limits(Limits) {
  }

public:

  LimitsType * getLimits() {
    return Limits;
  }

  const LimitsType * getLimits() const {
    return Limits;
  }

  static MemoryType * create(ASTContext& Context, LimitsType * Limits) {
    return new (Context) MemoryType(Limits);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Memory);
};

class TableType : public Type {
private:

  ReferenceType * ElementType;

  LimitsType * Limits;

  TableType(ReferenceType * ElementType, LimitsType * Limits) :
    Type(TypeKind::Table),
    ElementType(ElementType),
    Limits(Limits) {
  }

public:

  ReferenceType * getElementType() {
    return ElementType;
  }

  const ReferenceType * getElementType() const {
    return ElementType;
  }

  LimitsType * getLimits() {
    return Limits;
  }

  const LimitsType * getLimits() const {
    return Limits;
  }

  static TableType * create(
    ASTContext& Context, ReferenceType * ElementType, LimitsType * Limits
  ) {
    return new (Context) TableType(ElementType, Limits);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Table);
};

class GlobalType : public Type {
private:

  ValueType * Ty;

  bool IsMutable;

  GlobalType(ValueType * Ty, bool IsMutable) :
    Type(TypeKind::Table),
    Ty(Ty),
    IsMutable(IsMutable) {
  }

public:

  ValueType * getType() {
    return Ty;
  }

  const ValueType * getType() const {
    return Ty;
  }

  bool isMutable() const {
    return IsMutable;
  }

  static GlobalType *
  create(ASTContext& Context, ValueType * Ty, bool IsMutable) {
    return new (Context) GlobalType(Ty, IsMutable);
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, Global);
};

} // namespace w2n

#endif // W2N_AST_TYPE_H
