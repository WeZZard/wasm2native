#include "llvm/Support/ErrorHandling.h"
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Type.h>

using namespace w2n;

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
