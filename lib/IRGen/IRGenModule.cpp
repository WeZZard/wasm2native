#include "Address.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <cassert>
#include <cstdio>
#include <format>
#include <memory>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/IRGenModule.h>
#include <w2n/IRGen/Linking.h>

using namespace w2n;
using namespace w2n::irgen;

IRGenModule::IRGenModule(
  IRGenerator& IRGen,
  std::unique_ptr<llvm::TargetMachine>&& Target,
  SourceFile * SF,
  StringRef ModuleName,
  StringRef OutputFilename,
  StringRef MainInputFilenameForDebugInfo
) :
  LLVMContext(new llvm::LLVMContext()),
  IRGen(IRGen),
  Context(IRGen.Module.getASTContext()),
  Module(std::make_unique<llvm::Module>(ModuleName, *LLVMContext)),
  TargetMachine(std::move(Target)),
  OutputFilename(OutputFilename),
  MainInputFilenameForDebugInfo(MainInputFilenameForDebugInfo),
  ModuleHash(nullptr) {
  IRGen.addGenModule(SF, this);

  Int32Ty = llvm::Type::getInt32Ty(getLLVMContext());
  Int64Ty = llvm::Type::getInt64Ty(getLLVMContext());
  FloatTy = llvm::Type::getFloatTy(getLLVMContext());
  DoubleTy = llvm::Type::getDoubleTy(getLLVMContext());
}

IRGenModule::~IRGenModule() {
  // FIXME: Release resources
}

GeneratedModule IRGenModule::intoGeneratedModule() && {
  return GeneratedModule{
    std::move(LLVMContext), std::move(Module), std::move(TargetMachine)};
}

void IRGenModule::emitGlobalVariable(GlobalVariable * V) {
  getAddrOfGlobalVariable(V, ForDefinition);
}

void IRGenModule::emitCoverageMapping() {
  w2n_proto_implemented();
}

void IRGenModule::finishEmitAfterTopLevel() {
  w2n_proto_implemented();
}

bool IRGenModule::finalize() {
  return w2n_proto_implemented([]() -> bool { return true; });
}

void IRGenModule::addLinkLibrary(const LinkLibrary& LinkLib) {
  w2n_unimplemented();
}

#pragma mark Gen Decls

/// Emit all the top-level code in the source file.
void IRGenModule::emitSourceFile(SourceFile& SF) {
  for (Decl * D : SF.getTopLevelDecls()) {
    ModuleDecl * M = dyn_cast<ModuleDecl>(D);
    if (M == nullptr) {
      continue;
    }
    auto Globals = M->getGlobals();
    for (GlobalVariable& V : Globals) {
      GlobalDecl * VarDecl = V.getDecl();
      CurrentIGMPtr IGM = IRGen.getGenModule(
        VarDecl != nullptr ? VarDecl->getDeclContext() : nullptr
      );
      IGM->emitGlobalVariable(&V);
    }
  }
  w2n_proto_implemented();
}

llvm::Module * IRGenModule::getModule() const {
  return Module.get();
}

Address IRGenModule::getAddrOfGlobalVariable(
  GlobalVariable * Global, ForDefinition_t ForDefinition
) {
  std::string UniqueName = (Twine("$") + Twine(0)).str();

  auto * GVar =
    Module->getGlobalVariable(UniqueName, /*allowInternal*/ true);

  llvm::Type * StorageType = getType(Global->getType());

  if (GVar == nullptr) {
    GVar = new llvm::GlobalVariable(
      *Module,
      StorageType,
      /*constant*/ false,
      // FIXME: linkInfo.getLinkage(),
      llvm::GlobalValue::LinkageTypes::InternalLinkage,
      /*initializer*/ nullptr,
      UniqueName
    );

    // FIXME: Temporary workaround
    GVar->setAlignment(llvm::MaybeAlign(4));
    /// Add a zero initializer.
    if (ForDefinition != 0) {
      GVar->setInitializer(llvm::Constant::getNullValue(StorageType));
    } else {
      GVar->setComdat(nullptr);
    }
  }

  llvm::Constant * Addr = GVar;
  Addr =
    llvm::ConstantExpr::getBitCast(Addr, StorageType->getPointerTo());

  return Address(Addr, StorageType, Alignment(GVar->getAlignment()));
}
