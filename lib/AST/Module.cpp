#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <cassert>
#include <memory>
#include <utility>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/TypeCheckerRequests.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;

#pragma mark - ModuleDecl

#pragma mark Creating Module Decl

ModuleDecl::ModuleDecl(Identifier Name, ASTContext& Context) :
  DeclContext(DeclContextKind::Module, nullptr),
  TypeDecl(DeclKind::Module, const_cast<ASTContext *>(&Context)) {
  Context.addDestructorCleanup(*this);
}

#pragma mark Accessing Basic Properties

Identifier ModuleDecl::getName() const {
  return Name;
}

#pragma mark Managing Files

void ModuleDecl::addFile(FileUnit& NewFile) {
  Files.push_back(&NewFile);
  // FIXME: clearLookupCache();
}

ArrayRef<SourceFile *> ModuleDecl::getPrimarySourceFiles() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return evaluateOrDefault(Eval, PrimarySourceFilesRequest{Mutable}, {});
}

const ModuleDecl::GlobalListType& ModuleDecl::getGlobalListImpl() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return *evaluateOrDefault(Eval, GlobalVariableRequest{Mutable}, {});
}

const ModuleDecl::FunctionListType&
ModuleDecl::getFunctionListImpl() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return *evaluateOrDefault(Eval, FunctionRequest{Mutable}, {});
}

const ModuleDecl::TableListType& ModuleDecl::getTableListImpl() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return *evaluateOrDefault(Eval, TableRequest{Mutable}, {});
}

const ModuleDecl::MemoryListType& ModuleDecl::getMemoryListImpl() const {
  auto& Eval = getASTContext().Eval;
  auto * Mutable = const_cast<ModuleDecl *>(this);
  return *evaluateOrDefault(Eval, MemoryRequest{Mutable}, {});
}

#pragma mark Accessing Linkage Infos

// FIXME: Forwards to synthesized file if needed.
#define W2N_FOR_EACH_FILE(name, args)                                    \
  for (const FileUnit * file : getFiles()) {                             \
    file->name args;                                                     \
  }

void ModuleDecl::collectLinkLibraries(LinkLibraryCallback Callback
) const {
  // FIXME: The proper way to do this depends on the decls used.
  W2N_FOR_EACH_FILE(collectLinkLibraries, (Callback));
}

void SourceFile::collectLinkLibraries(
  ModuleDecl::LinkLibraryCallback Callback
) const {
  w2n_unimplemented();
}

#pragma mark - PrimarySourceFilesRequest

PrimarySourceFilesRequest::OutputType PrimarySourceFilesRequest::evaluate(
  Evaluator& Eval, ModuleDecl * Mod
) const {
  assert(
    Mod->isMainModule() && "Only the main module can have primaries"
  );

  SmallVector<SourceFile *, 8> Primaries;
  for (auto * File : Mod->getFiles()) {
    auto * SF = dyn_cast<SourceFile>(File);
    if ((SF != nullptr) && SF->isPrimary()) {
      Primaries.push_back(SF);
    }
  }
  return Mod->getASTContext().allocateCopy(Primaries);
}