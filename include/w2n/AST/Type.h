#ifndef W2N_AST_TYPE_H
#define W2N_AST_TYPE_H

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

class ValueType : public Type {
protected:

  ValueType(TypeKind Kind) : Type(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, ValueType);
};

class NumberType : public ValueType {
protected:

  NumberType(TypeKind Kind) : ValueType(Kind) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Type, NumberType);
};

class I32Type : public NumberType {
protected:

  I32Type() : NumberType(TypeKind::I32) {
  }

public:

  static I32Type * create(ASTContext& Context) {
    return new (Context) I32Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I32);
};

class I64Type : public NumberType {
protected:

  I64Type() : NumberType(TypeKind::I64) {
  }

public:

  static I64Type * create(ASTContext& Context) {
    return new (Context) I64Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, I64);
};

class F32Type : public NumberType {
protected:

  F32Type() : NumberType(TypeKind::F32) {
  }

public:

  static F32Type * create(ASTContext& Context) {
    return new (Context) F32Type();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, F32);
};

class F64Type : public NumberType {
protected:

  F64Type() : NumberType(TypeKind::F64) {
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
protected:

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
protected:

  FuncRefType() : ReferenceType(TypeKind::FuncRef) {
  }

public:

  static FuncRefType * create(ASTContext& Context) {
    return new (Context) FuncRefType();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, FuncRef);
};

class ExternRefType : public ReferenceType {
protected:

  ExternRefType() : ReferenceType(TypeKind::ExternRef) {
  }

public:

  static ExternRefType * create(ASTContext& Context) {
    return new (Context) ExternRefType();
  }

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Type, ExternRef);
};

} // namespace w2n

#endif // W2N_AST_TYPE_H
