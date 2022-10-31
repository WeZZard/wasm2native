#include "llvm/Support/ErrorHandling.h"
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/TypeCheckerRequests.h>

using namespace w2n;

ModuleDecl::ModuleDecl(Identifier Name, ASTContext& Context)
  : DeclContext(DeclContextKind::Module, nullptr),
    Decl(DeclKind::Module, const_cast<ASTContext *>(&Context)) {
  Context.addDestructorCleanup(*this);
}

Identifier ModuleDecl::getName() const {
  return Name;
}

void ModuleDecl::addFile(FileUnit& NewFile) {
  Files.push_back(&NewFile);
  // FIXME: clearLookupCache();
}

ArrayRef<SourceFile *> PrimarySourceFilesRequest::evaluate(
  Evaluator& Eval,
  ModuleDecl * Module
) const {
  assert(
    Module->isMainModule() && "Only the main module can have primaries"
  );

  SmallVector<SourceFile *, 8> primaries;
  for (auto * file : Module->getFiles()) {
    if (auto * SF = dyn_cast<SourceFile>(file)) {
      if (SF->isPrimary())
        primaries.push_back(SF);
    }
  }
  return Module->getASTContext().AllocateCopy(primaries);
}

ArrayRef<SourceFile *> ModuleDecl::getPrimarySourceFiles() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return evaluateOrDefault(Eval, PrimarySourceFilesRequest{Mutable}, {});
}

// FIXME: Forwards to synthesized file if needed.
#define FORWARD(name, args)                                              \
  for (const FileUnit * file : getFiles()) {                             \
    file->name args;                                                     \
  }

void ModuleDecl::collectLinkLibraries(LinkLibraryCallback callback
) const {
  // FIXME: The proper way to do this depends on the decls used.
  FORWARD(collectLinkLibraries, (callback));
}

void SourceFile::collectLinkLibraries(
  ModuleDecl::LinkLibraryCallback callback
) const {
  llvm_unreachable("not implemented.");
}
