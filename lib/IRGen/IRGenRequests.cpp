#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/TBDGenRequests.h>
#include <w2n/TBDGen/TBDGen.h>

using namespace w2n;
using namespace llvm;

namespace w2n {
// Implement the IRGen type zone (zone 20).
#define W2N_TYPEID_ZONE   IRGen
#define W2N_TYPEID_HEADER <w2n/AST/IRGenTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
} // end namespace w2n

llvm::orc::ThreadSafeModule GeneratedModule::intoThreadSafeContext() && {
  return {std::move(Module), std::move(Context)};
}

void w2n::simple_display(
  llvm::raw_ostream& out, const IRGenDescriptor& desc
) {
  auto * MD = desc.Ctx.dyn_cast<ModuleDecl *>();
  if (MD) {
    out << "IR Generation for module " << MD->getName();
  } else {
    auto * file = desc.Ctx.get<FileUnit *>();
    out << "IR Generation for file ";
    simple_display(out, file);
  }
}

SourceLoc w2n::extractNearestSourceLoc(const IRGenDescriptor& desc) {
  return SourceLoc();
}

TinyPtrVector<FileUnit *> IRGenDescriptor::getFilesToEmit() const {
  // If we've been asked to emit a specific set of symbols, we don't emit
  // any whole files.
  if (SymbolsToEmit)
    return {};

  // For a whole module, we emit IR for all files.
  if (auto * mod = Ctx.dyn_cast<ModuleDecl *>())
    return TinyPtrVector<FileUnit *>(mod->getFiles());

  // For a primary file, we emit IR for both it and potentially its
  // SynthesizedFileUnit.
  auto * primary = Ctx.get<FileUnit *>();
  TinyPtrVector<FileUnit *> files;
  files.push_back(primary);

  return files;
}

ModuleDecl * IRGenDescriptor::getParentModule() const {
  if (auto * file = Ctx.dyn_cast<FileUnit *>())
    return file->getParentModule();
  return Ctx.get<ModuleDecl *>();
}

TBDGenDescriptor IRGenDescriptor::getTBDGenDescriptor() const {
  if (auto * file = Ctx.dyn_cast<FileUnit *>()) {
    return TBDGenDescriptor::forFile(file, TBDOpts);
  }
  auto * M = Ctx.get<ModuleDecl *>();
  return TBDGenDescriptor::forModule(M, TBDOpts);
}

std::vector<std::string> IRGenDescriptor::getLinkerDirectives() const {
  auto desc = getTBDGenDescriptor();
  desc.getOptions().LinkerDirectivesOnly = true;
  return getPublicSymbols(std::move(desc));
}

evaluator::DependencySource
IRGenRequest::readDependencySource(const evaluator::DependencyRecorder& e
) const {
  auto& desc = std::get<0>(getStorage());

  // We don't track dependencies in whole-module mode.
  if (auto * mod = desc.Ctx.dyn_cast<ModuleDecl *>()) {
    return nullptr;
  }

  auto * primary = desc.Ctx.get<FileUnit *>();
  return dyn_cast<SourceFile>(primary);
}

// Define request evaluation functions for each of the IRGen requests.
static AbstractRequestFunction * irGenRequestFunctions[] = {
#define W2N_REQUEST(Zone, Name, Sig, Caching, LocOptions)                \
  reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include <w2n/AST/IRGenTypeIDZone.def>
#undef W2N_REQUEST
};

void w2n::registerIRGenRequestFunctions(Evaluator& evaluator) {
  evaluator.registerRequestFunctions(Zone::IRGen, irGenRequestFunctions);
}
