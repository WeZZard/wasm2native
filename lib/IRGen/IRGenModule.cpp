#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/ErrorHandling.h>
#include <memory>
#include <w2n/AST/IRGenRequests.h>
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
  llvm_unreachable("not implemented.");
}

void IRGenerator::emitGlobalTopLevel(
  const std::vector<std::string>& LinkerDirectives
) {
  llvm_unreachable("not implemented.");
}

void IRGenerator::emitEntryPointInfo() {
  llvm_unreachable("not implemented.");
}

void IRGenerator::emitCoverageMapping() {
  llvm_unreachable("not implemented.");
}

void IRGenerator::emitLazyDefinitions() {
  llvm_unreachable("not implemented.");
}

#pragma mark - IRGenModule

IRGenModule::IRGenModule(
  IRGenerator& irgen,
  std::unique_ptr<llvm::TargetMachine>&& target,
  SourceFile * SF,
  StringRef ModuleName,
  StringRef OutputFilename,
  StringRef MainInputFilenameForDebugInfo
) :
  LLVMContext(new llvm::LLVMContext()),
  IRGen(irgen),
  Context(irgen.Module.getASTContext()),
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
    std::move(LLVMContext),
    // FIXME: std::unique_ptr<llvm::Module>{ClangCodeGen->ReleaseModule()}
    std::unique_ptr<llvm::Module>(),
    std::move(TargetMachine)};
}

llvm::Module * IRGenModule::getModule() const {
  // Swift uses ClangCodeGen->GetModule() here. Seems like this method is
  // for code-gen with clang.
  llvm_unreachable("not implemented.");
}

void IRGenModule::emitCoverageMapping() {
  llvm_unreachable("not implemented.");
}

bool IRGenModule::finalize() {
  llvm_unreachable("not implemented.");
}

void IRGenModule::addLinkLibrary(const LinkLibrary& linkLib) {
  llvm_unreachable("not implemented.");
}

#pragma mark Gen Decls

/// Emit all the top-level code in the source file.
void IRGenModule::emitSourceFile(SourceFile& SF) {
  llvm_unreachable("not implemented.");
}