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
#include <w2n/AST/Function.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/AST/Memory.h>
#include <w2n/AST/Table.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/Unimplemented.h>

namespace w2n {

class FileUnit;
class SourceFile;
class ASTContext;
class GlobalVariable;

/// A \c ModuleDecl may be a main module that the being compiled by the
/// \c CompilerInstance or a module represents a source file.
class ModuleDecl :
  public DeclContext,
  public TypeDecl,
  public ASTAllocated<ModuleDecl> {
public:

  friend class Decl;
  friend class GlobalVariableRequest;
  friend class FunctionRequest;
  friend class MemoryRequest;
  friend class TableRequest;

  using GlobalListType = llvm::ilist<GlobalVariable>;
  using FunctionListType = llvm::ilist<Function>;
  using MemoryListType = llvm::ilist<Memory>;
  using TableListType = llvm::ilist<Table>;

  using global_iterator = GlobalListType::iterator;
  using const_global_iterator = GlobalListType::const_iterator;

  using function_iterator = FunctionListType::iterator;
  using const_function_iterator = FunctionListType::const_iterator;

  using memory_iterator = MemoryListType::iterator;
  using const_memory_iterator = MemoryListType::const_iterator;

  using table_iterator = TableListType::iterator;
  using const_table_iterator = TableListType::const_iterator;

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

  mutable std::shared_ptr<FunctionListType> Functions = nullptr;

  mutable std::shared_ptr<TableListType> Tables = nullptr;

  mutable std::shared_ptr<MemoryListType> Memories = nullptr;

  /// Unused functions kept for generating debug info.
  FunctionListType ZombieFunctions;

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
    for (auto * Sect : getSectionDecls()) {
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

#pragma mark Accessing Module Primitives

#define W2N_MODULE_PRIMITIVE_ACCESSOR(Id, id)                            \
  W2N_MODULE_PRIMITIVE_ACCESSOR_2(Id, Id##s, id, id##s)

#define W2N_MODULE_PRIMITIVE_ACCESSOR_2(Id, Ids, id, ids)                \
                                                                         \
private:                                                                 \
                                                                         \
  const Id##ListType& get##Id##ListImpl() const;                         \
                                                                         \
public:                                                                  \
                                                                         \
  Id##ListType& get##Id##List() {                                        \
    return const_cast<Id##ListType&>(                                    \
      const_cast<const ModuleDecl *>(this)->get##Id##List()              \
    );                                                                   \
  }                                                                      \
                                                                         \
  const Id##ListType& get##Id##List() const {                            \
    return get##Id##ListImpl();                                          \
  }                                                                      \
                                                                         \
  id##_iterator id##_begin() {                                           \
    return get##Id##List().begin();                                      \
  }                                                                      \
                                                                         \
  id##_iterator id##_end() {                                             \
    return get##Id##List().end();                                        \
  }                                                                      \
                                                                         \
  const_##id##_iterator id##_begin() const {                             \
    return get##Id##List().begin();                                      \
  }                                                                      \
                                                                         \
  const_##id##_iterator id##_end() const {                               \
    return get##Id##List().end();                                        \
  }                                                                      \
                                                                         \
  iterator_range<id##_iterator> get##Ids() {                             \
    return {id##_begin(), id##_end()};                                   \
  }                                                                      \
                                                                         \
  iterator_range<const_##id##_iterator> get##Ids() const {               \
    return {id##_begin(), id##_end()};                                   \
  }

  W2N_MODULE_PRIMITIVE_ACCESSOR(Global, global);

  W2N_MODULE_PRIMITIVE_ACCESSOR(Function, function);

  W2N_MODULE_PRIMITIVE_ACCESSOR(Table, table);

  W2N_MODULE_PRIMITIVE_ACCESSOR_2(Memory, Memories, memory, memories);

#pragma mark Accessing Linkage Infos

  using LinkLibraryCallback = llvm::function_ref<void(LinkLibrary)>;

  /// Generate the list of libraries needed to link this module, based on
  /// its imports.
  void collectLinkLibraries(LinkLibraryCallback Callback) const;

  bool isStaticLibrary() const {
    return w2n_proto_implemented(
      "Static library has not supported yet.", [&] { return false; }
    );
  }

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
