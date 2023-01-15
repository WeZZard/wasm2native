#ifndef W2N_AST_MODULE_H
#define W2N_AST_MODULE_H

#include "llvm/ADT/STLExtras.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/None.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>
#include <llvm/Support/ErrorHandling.h>
#include <functional>
#include <memory>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class FileUnit;
class SourceFile;
class ASTContext;
class GlobalVariable;

class ModuleDecl :
  public DeclContext,
  public TypeDecl,
  public ASTAllocated<ModuleDecl> {
public:

  friend class Decl;
  friend class GlobalVariable;
  friend class GlobalVariableRequest;

  using GlobalListType = llvm::ilist<GlobalVariable>;

  using global_iterator = GlobalListType::iterator;
  using const_global_iterator = GlobalListType::const_iterator;

private:

  bool IsMainModule = false;

  bool FailedToLoad = false;

  bool HasResolvedImports = false;

  llvm::Optional<uint32_t> Magic;

  llvm::Optional<uint32_t> Version;

  Identifier Name;

  SmallVector<FileUnit *, 2> Files;

  llvm::SmallVector<SectionDecl *> SectionDecls;

  ModuleDecl(Identifier Name, ASTContext& Context);

  mutable std::shared_ptr<GlobalListType> Globals = nullptr;

public:

#pragma mark Creating Module Decl

  /**
   * @brief Creates a new module with a given \p Name.
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

#pragma mark Accessing Module Basic Properties

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

#pragma mark Managing Files

  ArrayRef<FileUnit *> getFiles() {
    assert(!Files.empty() || failedToLoad());
    return Files;
  }

  ArrayRef<const FileUnit *> getFiles() const {
    return {Files.begin(), Files.size()};
  }

  void addFile(FileUnit& NewFile);

  /**
   * @brief For the main module, retrieves the list of primary source
   * files being compiled, that is, the files we're generating code for.
   */
  ArrayRef<SourceFile *> getPrimarySourceFiles() const;

#pragma mark Managing Section Decls

  llvm::SmallVector<SectionDecl *>& getSectionDecls() {
    return SectionDecls;
  }

  const llvm::SmallVector<SectionDecl *>& getSectionDecls() const {
    return SectionDecls;
  }

  void addSectionDecl(SectionDecl * SectionDecl) {
    SectionDecls.push_back(SectionDecl);
  }

#pragma mark Accessing Section by Name

  template <typename Ty>
  Ty * getSection() {
    return const_cast<Ty *>(
      const_cast<const ModuleDecl *>(this)->getSection<Ty>()
    );
  }

  template <typename Ty>
  const Ty * getSection() const {
    for (auto * Sect : SectionDecls) {
      if (Sect->getSectionKind() == SectionKind::GlobalSection) {
        printf("");
      }
      if (Ty * TySect = dyn_cast<Ty>(Sect)) {
        return TySect;
      }
    }
    return nullptr;
  }

#define DECL(Id, Parent)
#define SECTION_DECL(Id, Parent)                                         \
  Id##Decl * get##Id() {                                                 \
    return getSection<Id##Decl>();                                       \
  }                                                                      \
  const Id##Decl * get##Id() const {                                     \
    return getSection<Id##Decl>();                                       \
  }

#include <w2n/AST/DeclNodes.def>

#pragma mark Accessing AST Context

  using Decl::getASTContext;

#pragma mark Accessing Globals

private:

  const GlobalListType& getGlobalListImpl() const;

public:

  GlobalListType& getGlobalList() {
    return const_cast<GlobalListType&>(
      const_cast<const ModuleDecl *>(this)->getGlobalList()
    );
  }

  const GlobalListType& getGlobalList() const {
    return getGlobalListImpl();
  }

  global_iterator global_begin() {
    return getGlobalList().begin();
  }

  global_iterator global_end() {
    return getGlobalList().end();
  }

  const_global_iterator global_begin() const {
    return getGlobalList().begin();
  }

  const_global_iterator global_end() const {
    return getGlobalList().end();
  }

  iterator_range<global_iterator> getGlobals() {
    return {global_begin(), global_end()};
  }

  iterator_range<const_global_iterator> getGlobals() const {
    return {global_begin(), global_end()};
  }

#pragma mark Accessing Linkage Infos

  using LinkLibraryCallback = llvm::function_ref<void(LinkLibrary)>;

  /// Generate the list of libraries needed to link this module, based on
  /// its imports.
  void collectLinkLibraries(LinkLibraryCallback Callback) const;

#pragma mark Implementing LLVM RTTI Methods

  static bool classof(const DeclContext * DC) {
    if (const auto * D = DC->getAsDecl()) {
      return classof(D);
    }
    return false;
  }

  static bool classof(const Decl * D) {
    return D->getKind() == DeclKind::Module;
  }

#pragma mark Overloading new and delete

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