#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/ErrorHandling.h>
#include <cstdio>
#include <memory>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/IRGenModule.h>

using namespace w2n;
using namespace w2n::irgen;

#pragma mark - IRGenerator

IRGenerator::IRGenerator(const IRGenOptions& Opts, ModuleDecl& Module) :
  Opts(Opts),
  Module(Module),
  QueueIndex(0) {
}

void IRGenerator::addGenModule(SourceFile * SF, IRGenModule * IGM) {
  assert(GenModules.count(SF) == 0);
  GenModules[SF] = IGM;
  if (!PrimaryIGM) {
    PrimaryIGM = IGM;
  }
  Queue.push_back(IGM);
}

IRGenModule * IRGenerator::getGenModule(DeclContext * DC) {
  if (GenModules.size() == 1 || !DC) {
    return getPrimaryIGM();
  }
  SourceFile * SF = DC->getParentSourceFile();
  if (!SF) {
    return getPrimaryIGM();
  }
  IRGenModule * IGM = GenModules[SF];
  assert(IGM);
  return IGM;
}

IRGenModule * IRGenerator::getGenModule(FuncDecl * f) {
  w2n_unimplemented();
}

void IRGenerator::emitGlobalTopLevel(
  const std::vector<std::string>& LinkerDirectives
) {
  w2n_proto_implemented();
}

void IRGenerator::emitEntryPointInfo() {
  w2n_unimplemented();
}

void IRGenerator::emitCoverageMapping() {
  w2n_unimplemented();
}

void IRGenerator::emitLazyDefinitions() {
  w2n_proto_implemented();
}

#pragma mark - IRGenModule

IRGenModule::IRGenModule(
  IRGenerator& IRGen,
  std::unique_ptr<llvm::TargetMachine>&& target,
  SourceFile * SF,
  StringRef ModuleName,
  StringRef OutputFilename,
  StringRef MainInputFilenameForDebugInfo
) :
  LLVMContext(new llvm::LLVMContext()),
  IRGen(IRGen),
  Context(IRGen.Module.getASTContext()),
  Module(std::make_unique<llvm::Module>(ModuleName, *LLVMContext)),
  TargetMachine(std::move(target)),
  OutputFilename(OutputFilename),
  MainInputFilenameForDebugInfo(MainInputFilenameForDebugInfo),
  ModuleHash(nullptr) {
}

IRGenModule::~IRGenModule() {
  // FIXME: Release resources
}

GeneratedModule IRGenModule::intoGeneratedModule() && {
  return GeneratedModule{
    std::move(LLVMContext), std::move(Module), std::move(TargetMachine)};
}

llvm::Module * IRGenModule::getModule() const {
  return Module.get();
}

void IRGenModule::emitCoverageMapping() {
  w2n_proto_implemented();
}

bool IRGenModule::finalize() {
  return w2n_proto_implemented([]() -> bool { return true; });
}

void IRGenModule::addLinkLibrary(const LinkLibrary& linkLib) {
  w2n_unimplemented();
}

#pragma mark Gen Decls

/// Emit all the top-level code in the source file.
void IRGenModule::emitSourceFile(SourceFile& SF) {
  w2n_proto_implemented([&]() -> void {
    auto Builder = llvm::IRBuilder<>(*LLVMContext);
    auto * GlobalVarValue =
      llvm::ConstantFP::get(*LLVMContext, llvm::APFloat(1.0));
    Module->getOrInsertGlobal(
      "kGlobalVar",
      Builder.getDoubleTy(),
      [&]() -> llvm::GlobalVariable * {
        return new llvm::GlobalVariable(
          *Module,
          Builder.getDoubleTy(),
          true,
          llvm::GlobalValue::PrivateLinkage,
          GlobalVarValue,
          "kGlobalVar1"
        );
      }
    );
    Module->dump();
    Module->getOrInsertGlobal("kGlobalVar2", Builder.getDoubleTy());
    Module->dump();
    new llvm::GlobalVariable(
      *Module,
      Builder.getDoubleTy(),
      true,
      llvm::GlobalValue::PrivateLinkage,
      GlobalVarValue,
      "kGlobalVar3"
    );
    Module->dump();
    auto * kGlobalVar4 = new llvm::GlobalVariable(
      Builder.getDoubleTy(),
      true,
      llvm::GlobalValue::PrivateLinkage,
      GlobalVarValue,
      "kGlobalVar4"
    );
    Module->getGlobalList().push_back(kGlobalVar4);
    Module->dump();
    printf("\n");
  });
}
