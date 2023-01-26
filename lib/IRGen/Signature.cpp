#include "Signature.h"
#include "IRGenModule.h"
#include <llvm/IR/CallingConv.h>

using namespace w2n;
using namespace w2n::irgen;

Signature
Signature::getUncached(IRGenModule& IGM, FuncType * FormalType) {
  // Create the appropriate LLVM type.
  llvm::FunctionType * LLVMType = IGM.getFuncType(FormalType);

  // Currently uses C calling convention
  auto CallingConv = llvm::CallingConv::C;

  Signature Result;
  Result.Type = LLVMType;
  Result.CallingConv = CallingConv;

  // Currently no attributes get applied on an LLVM function type.
  // result.Attributes = ...

  return Result;
}
