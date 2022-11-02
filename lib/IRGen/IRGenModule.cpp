#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/ErrorHandling.h>
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
  w2n_not_implemented();
}

void IRGenerator::emitGlobalTopLevel(
  const std::vector<std::string>& LinkerDirectives
) {
  proto_impl();
}

void IRGenerator::emitEntryPointInfo() {
  w2n_not_implemented();
}

void IRGenerator::emitCoverageMapping() {
  w2n_not_implemented();
}

void IRGenerator::emitLazyDefinitions() {
  proto_impl();
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
  proto_impl();
}

bool IRGenModule::finalize() {
  return proto_impl<bool>([]() -> bool { return true; });
}

void IRGenModule::addLinkLibrary(const LinkLibrary& linkLib) {
  w2n_not_implemented();
}

#pragma mark Gen Decls

/// Emit all the top-level code in the source file.
void IRGenModule::emitSourceFile(SourceFile& SF) {
  proto_impl<void>([&]() -> void {
    auto Builder = llvm::IRBuilder<>(*LLVMContext);
    auto * GlobalVar =
      Module->getOrInsertGlobal("globalVar", Builder.getDoubleTy());
  });
}
