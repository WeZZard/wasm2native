#include "IRGenConstructor.h"
#include "Address.h"
#include "IRGenModule.h"
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <w2n/AST/GlobalVariable.h>

/// We currently use an implementation like C++ static initializer.
void w2n::irgen::emitGlobalVariableConstructor(
  IRGenModule& Module,
  GlobalVariable * V,
  Address Addr,
  llvm::Function * Init
) {
  std::string FnName =
    V->getFullQualifiedDescriptiveName() + "-initializer";
  llvm::Function * Constructor = Module.getModule()->getFunction(FnName);

  if (Constructor != nullptr) {
    return;
  }

  llvm::FunctionType * FTy = llvm::FunctionType::get(
    llvm::Type::getVoidTy(Module.getLLVMContext()), false
  );

  Constructor = llvm::Function::Create(
    FTy,
    llvm::GlobalValue::InternalLinkage,
    "constructor",
    Module.getModule()
  );

  llvm::appendToGlobalCtors(*Module.getModule(), Constructor, 0);

  llvm::BasicBlock * EntryBB = llvm::BasicBlock::Create(
    Module.getLLVMContext(), "entry", Constructor
  );
  llvm::IRBuilder<> Builder(EntryBB);
  llvm::Value * Result = Builder.CreateCall(Init);
  llvm::PointerType * PtrType =
    llvm::cast<llvm::PointerType>(Addr->getType());
  Builder.CreateStore(
    Result, Builder.CreateBitCast(Addr.getAddress(), PtrType)
  );
  Builder.CreateRetVoid();
}
