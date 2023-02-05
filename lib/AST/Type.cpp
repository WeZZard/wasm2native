#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;

void Type::dump() const {
  dump(llvm::errs());
}

void Type::dump(raw_ostream& OS, unsigned Indent) const {
  switch (getKind()) {
  case TypeKind::Func: {
    auto * T = dyn_cast<FuncType>(this);
    auto& Params = T->getParameters()->getValueTypes();
    auto& Returns = T->getReturns()->getValueTypes();
    OS << "(";
    if (Params.empty()) {
    } else {
      uint32_t Index = 0;
      for (auto * Param : Params) {
        OS << Param->getKindDescription();
        Index++;
        if (Index < Params.size()) {
          OS << ",";
        }
      }
    }
    OS << ") -> ";
    if (Returns.empty()) {
      OS << "void";
    } else {
      OS << "(";
      uint32_t Index = 0;
      for (auto * Return : Returns) {
        OS << Return->getKindDescription();
        Index++;
        if (Index < Returns.size()) {
          OS << ",";
        }
      }
      OS << ")";
    }
    break;
  }
  case TypeKind::I8: OS << "i8"; break;
  case TypeKind::I16: OS << "i16"; break;
  case TypeKind::I32: OS << "i32"; break;
  case TypeKind::I64: OS << "i64"; break;
  case TypeKind::U8: OS << "u8"; break;
  case TypeKind::U16: OS << "u16"; break;
  case TypeKind::U32: OS << "u32"; break;
  case TypeKind::U64: OS << "u64"; break;
  case TypeKind::F32: OS << "f32"; break;
  case TypeKind::F64: OS << "f64"; break;
  case TypeKind::V128: OS << "v128"; break;
  case TypeKind::FuncRef: OS << "function-ref"; break;
  case TypeKind::ExternRef: OS << "extern-ref"; break;
  case TypeKind::Void: OS << "void"; break;
  case TypeKind::Result:
  case TypeKind::Limits:
  case TypeKind::Memory:
  case TypeKind::Table:
  case TypeKind::Global:
  case TypeKind::TypeIndex:
  case TypeKind::Block: w2n_unimplemented();
  }
  OS << "\n";
}

StringRef Type::getKindDescription() const {
#define TYPE(Id, Parent)                                                 \
  case TypeKind::Id: return #Id;
  switch (getKind()) {
#include <w2n/AST/TypeNodes.def>
  }
}

Type *
Type::getBuiltinIntegerType(unsigned BitWidth, const ASTContext& C) {
  if (BitWidth == 8) {
    return C.getI8Type();
  }
  if (BitWidth == 16) {
    return C.getI16Type();
  }
  if (BitWidth == 32) {
    return C.getI32Type();
  }
  if (BitWidth == 64) {
    return C.getI64Type();
  }
  llvm_unreachable("Unexpected bit width.");
}
