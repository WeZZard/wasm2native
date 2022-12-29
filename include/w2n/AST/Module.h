#ifndef W2N_AST_MODULE_H
#define W2N_AST_MODULE_H

#include <_types/_uint32_t.h>
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include <llvm/ADT/ArrayRef.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class FileUnit;
class SourceFile;
class ASTContext;

class ModuleDecl :
  public DeclContext,
  public Decl,
  public ASTAllocated<ModuleDecl> {
private:

  friend class Decl;

  bool IsMainModule = false;

  bool FailedToLoad = false;

  bool HasResolvedImports = false;

  llvm::Optional<uint32_t> Magic;

  llvm::Optional<uint32_t> Version;

  Identifier Name;

  SmallVector<FileUnit *, 2> Files;

  llvm::SmallVector<SectionDecl *> SectionDecls;

  ModuleDecl(Identifier Name, ASTContext& Context);

  SourceLoc getLocFromSource() const {
    return SourceLoc();
  }

public:

  bool hasMagic() const {
    return Magic.has_value();
  }

  llvm::Optional<uint32_t> getMagic() const {
    return Magic;
  }

  bool hasVersion() const {
    return Version.has_value();
  }

  llvm::Optional<uint32_t> getVersion() const {
    return Version;
  }

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

  const llvm::SmallVector<SectionDecl *>& getSectionDecls() const {
    return SectionDecls;
  }

  llvm::SmallVector<SectionDecl *>& getSectionDecls() {
    return SectionDecls;
  }

  void addSectionDecl(SectionDecl * SectionDecl) {
    SectionDecls.push_back(SectionDecl);
  }

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

  using LinkLibraryCallback = llvm::function_ref<void(LinkLibrary)>;

  /// Generate the list of libraries needed to link this module, based on
  /// its imports.
  void collectLinkLibraries(LinkLibraryCallback Callback) const;

  static bool classof(const DeclContext * DC) {
    if (const auto * D = DC->getAsDecl()) {
      return classof(D);
    }
    return false;
  }

  static bool classof(const Decl * D) {
    return D->getKind() == DeclKind::Module;
  }

  using ASTAllocated<ModuleDecl>::operator new;
  using ASTAllocated<ModuleDecl>::operator delete;
};

inline bool DeclContext::isModuleContext() const {
  if (const auto * D = getAsDecl()) {
    return ModuleDecl::classof(D);
  }
  return false;
}

inline bool DeclContext::isModuleScopeContext() const {
  if (ParentAndKind.getInt() == ASTHierarchy::FileUnit) {
    return true;
  }
  return isModuleContext();
}

/// Extract the source location from the given module declaration.
inline SourceLoc extractNearestSourceLoc(const ModuleDecl * Mod) {
  return extractNearestSourceLoc(static_cast<const Decl *>(Mod));
}

} // namespace w2n

#endif // W2N_AST_MODULE_H