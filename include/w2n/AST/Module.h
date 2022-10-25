#ifndef W2N_AST_MODULE_H
#define W2N_AST_MODULE_H

#include <llvm/ADT/ArrayRef.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Identifier.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class FileUnit;
class SourceFile;
class ASTContext;

class ModuleDecl : public DeclContext,
                   public Decl,
                   public ASTAllocated<ModuleDecl> {

private:
  friend class Decl;

  bool IsMainModule = false;

  bool FailedToLoad = false;

  bool HasResolvedImports = false;

  Identifier Name;

  SmallVector<FileUnit *, 2> Files;

  ModuleDecl(Identifier Name, ASTContext& Context);

  SourceLoc getLocFromSource() const {
    return SourceLoc();
  }

public:
  /**
   * @brief Retrieve the module name for this module
   */
  Identifier getName() const;

  bool isMainModule() const {
    return IsMainModule;
  }

  /**
   * @brief For the main module, retrieves the list of primary source
   * files being compiled, that is, the files we're generating code for.
   */
  ArrayRef<SourceFile *> getPrimarySourceFiles() const;

  /**
   * @brief Creates a new module with a given \p name.
   */
  static ModuleDecl * create(Identifier Name, ASTContext& Context) {
    return new (Context) ModuleDecl(Name, Context);
  }

  static ModuleDecl *
  createMainModule(ASTContext& Context, Identifier Name) {
    auto * Module = ModuleDecl::create(Name, Context);
    Module->IsMainModule = true;
    return Module;
  }

  ArrayRef<FileUnit *> getFiles() {
    assert(!Files.empty() || failedToLoad());
    return Files;
  }

  ArrayRef<const FileUnit *> getFiles() const {
    return {Files.begin(), Files.size()};
  }

  void addFile(FileUnit& NewFile);

  /**
   * @brief Returns \c true if there was an error trying to load this
   * module.
   */
  bool failedToLoad() const {
    return FailedToLoad;
  }

  void setFailedToLoad(bool Failed = true) {
    FailedToLoad = Failed;
  }

  bool hasResolvedImports() const {
    return HasResolvedImports;
  }

  void setHasResolvedImports() {
    HasResolvedImports = true;
  }

  using Decl::getASTContext;

  static bool classof(const DeclContext * DC) {
    if (auto D = DC->getAsDecl())
      return classof(D);
    return false;
  }

  static bool classof(const Decl * D) {
    return D->getKind() == DeclKind::Module;
  }

  using ASTAllocated<ModuleDecl>::operator new;
  using ASTAllocated<ModuleDecl>::operator delete;
};

inline bool DeclContext::isModuleContext() const {
  if (auto D = getAsDecl())
    return ModuleDecl::classof(D);
  return false;
}

/// Extract the source location from the given module declaration.
inline SourceLoc extractNearestSourceLoc(const ModuleDecl * mod) {
  return extractNearestSourceLoc(static_cast<const Decl *>(mod));
}

} // namespace w2n

#endif // W2N_AST_MODULE_H