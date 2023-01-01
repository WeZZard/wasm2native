#ifndef W2N_AST_DECL_H
#define W2N_AST_DECL_H

#include <_types/_uint32_t.h>
#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringRef.h>
#include <cstdint>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/InlineBitfield.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMRTTI.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/Unimplemented.h>

namespace w2n {
class ASTContext;

/// Decl subclass prototyping helper
///
/// There are \c static_asserts in implementation files that checks if
/// each subclass of \c Decl implements a set of functions declared in
/// \c Decl class. This macro can be used for quickly supress errors
/// produced by these \c static_asserts when we are prototyping a \c Decl
/// subclass.
///
#define USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE                              \
  SourceLoc getLocFromSource() const {                                   \
    return w2n_proto_implemented([] { return SourceLoc(); });            \
  }

enum class DeclKind {
#define DECL(Id, Parent) Id,
#define LAST_DECL(Id)    Last_Decl = Id,
#define DECL_RANGE(Id, FirstId, LastId)                                  \
  First_##Id##Decl = FirstId, Last_##Id##Decl = LastId,
#include <w2n/AST/DeclNodes.def>
};

enum : unsigned {
  NumDeclKindBits =
    countBitsUsed(static_cast<unsigned>(DeclKind::Last_Decl))
};

/// Fine-grained declaration kind that provides a description of the
/// kind of entity a declaration represents, as it would be used in
/// diagnostics.
///
/// FIXME: Currently use \c DeclKind instead.
enum class DescriptiveDeclKind : uint8_t {
#define DECL(Id, Parent) Id,
#define LAST_DECL(Id)    Last_Decl = Id,
#define DECL_RANGE(Id, FirstId, LastId)                                  \
  First_##Id##Decl = FirstId, Last_##Id##Decl = LastId,
#include <w2n/AST/DeclNodes.def>
};

/**
 * @brief Base class of all decls in w2n.
 */
class LLVM_POINTER_LIKE_ALIGNMENT(Decl) Decl : public ASTAllocated<Decl> {
private:

  DeclKind Kind;

  llvm::PointerUnion<DeclContext *, ASTContext *> Context;

  Decl(const Decl&) = delete;
  void operator=(const Decl&) = delete;

  SourceLoc getLocFromSource() const;

protected:

  Decl(
    DeclKind Kind, llvm::PointerUnion<DeclContext *, ASTContext *> Context
  ) :
    Kind(Kind),
    Context(Context) {
  }

  DeclContext * getDeclContextForModule() const;

public:

  DeclKind getKind() const {
    return Kind;
  }

  DescriptiveDeclKind getDescriptiveKind() const;

  StringRef getDescriptiveKindName(DescriptiveDeclKind K) const;

  LLVM_READONLY
  DeclContext * getDeclContext() const {
    if (auto * DC = Context.dyn_cast<DeclContext *>()) {
      return DC;
    }

    return getDeclContextForModule();
  }

  void setDeclContext(DeclContext * DC);

  /// getASTContext - Return the ASTContext that this decl lives in.
  LLVM_READONLY
  ASTContext& getASTContext() const {
    if (auto * DC = Context.dyn_cast<DeclContext *>()) {
      return DC->getASTContext();
    }

    return *Context.get<ASTContext *>();
  }

  /// Returns the starting location of the entire declaration.
  SourceLoc getStartLoc() const {
    return getSourceRange().Start;
  }

  /// Returns the end location of the entire declaration.
  SourceLoc getEndLoc() const {
    return getSourceRange().End;
  }

  /// Returns the preferred location when referring to declarations
  /// in diagnostics.
  SourceLoc getLoc(bool SerializedOK = true) const;

  /// Returns the source range of the entire declaration.
  SourceRange getSourceRange() const;
};

SourceLoc extractNearestSourceLoc(const Decl * Decl);

void simple_display(llvm::raw_ostream& Out, const Decl * Decl);

class ValueDecl : public Decl {
protected:

  ValueDecl(DeclKind Kind, ASTContext * Ctx) : Decl(Kind, Ctx) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, ValueDecl);
};

class TypeDecl : public ValueDecl {
protected:

  TypeDecl(DeclKind Kind, ASTContext * Ctx) : ValueDecl(Kind, Ctx) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, TypeDecl);
};

class SectionDecl : public TypeDecl {
protected:

  SectionDecl(DeclKind Kind, ASTContext * Ctx) : TypeDecl(Kind, Ctx) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, SectionDecl);
};

SourceLoc extractNearestSourceLoc(const SectionDecl * Decl);

void simple_display(llvm::raw_ostream& Out, const SectionDecl * Decl);

class FuncTypeDecl;

class TypeSectionDecl final : public SectionDecl {
private:

  std::vector<FuncTypeDecl *> Types;

  TypeSectionDecl(ASTContext * Ctx, std::vector<FuncTypeDecl *> Types) :
    SectionDecl(DeclKind::TypeSection, Ctx),
    Types(Types) {
  }

public:

  static TypeSectionDecl *
  create(ASTContext& Ctx, std::vector<FuncTypeDecl *> Types) {
    return new (Ctx) TypeSectionDecl(&Ctx, Types);
  }

  std::vector<FuncTypeDecl *>& getTypes() {
    return Types;
  }

  const std::vector<FuncTypeDecl *>& getTypes() const {
    return Types;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, TypeSection);
};

class ImportDecl;

class ImportSectionDecl final : public SectionDecl {
private:

  std::vector<ImportDecl *> Imports;

  ImportSectionDecl(ASTContext * Ctx, std::vector<ImportDecl *> Imports) :
    SectionDecl(DeclKind::ImportSection, Ctx),
    Imports(Imports) {
  }

public:

  static ImportSectionDecl *
  create(ASTContext& Ctx, std::vector<ImportDecl *> Imports) {
    return new (Ctx) ImportSectionDecl(&Ctx, Imports);
  }

  std::vector<ImportDecl *>& getImports() {
    return Imports;
  }

  const std::vector<ImportDecl *>& getImports() const {
    return Imports;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportSection);
};

class FuncDecl;

class FuncSectionDecl final : public SectionDecl {
private:

  std::vector<FuncDecl *> Functions;

  FuncSectionDecl(ASTContext * Ctx, std::vector<FuncDecl *> Functions) :
    SectionDecl(DeclKind::FuncSection, Ctx),
    Functions(Functions) {
  }

public:

  static FuncSectionDecl *
  create(ASTContext& Ctx, std::vector<FuncDecl *> Functions) {
    return new (Ctx) FuncSectionDecl(&Ctx, Functions);
  }

  std::vector<FuncDecl *>& getFunctions() {
    return Functions;
  }

  const std::vector<FuncDecl *>& getFunctions() const {
    return Functions;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, FuncSection);
};

class TableDecl;

class TableSectionDecl final : public SectionDecl {
private:

  std::vector<TableDecl *> Tables;

  TableSectionDecl(ASTContext * Ctx, std::vector<TableDecl *> Tables) :
    SectionDecl(DeclKind::TableSection, Ctx),
    Tables(Tables) {
  }

public:

  static TableSectionDecl *
  create(ASTContext& Ctx, std::vector<TableDecl *> Tables) {
    return new (Ctx) TableSectionDecl(&Ctx, Tables);
  }

  std::vector<TableDecl *>& getTables() {
    return Tables;
  }

  const std::vector<TableDecl *>& getTables() const {
    return Tables;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, TableSection);
};

class MemoryDecl;

class MemorySectionDecl final : public SectionDecl {
private:

  std::vector<MemoryDecl *> Memories;

  MemorySectionDecl(
    ASTContext * Ctx, std::vector<MemoryDecl *> Memories
  ) :
    SectionDecl(DeclKind::MemorySection, Ctx),
    Memories(Memories) {
  }

public:

  static MemorySectionDecl *
  create(ASTContext& Ctx, std::vector<MemoryDecl *> Memories) {
    return new (Ctx) MemorySectionDecl(&Ctx, Memories);
  }

  std::vector<MemoryDecl *>& getMemories() {
    return Memories;
  }

  const std::vector<MemoryDecl *>& getMemories() const {
    return Memories;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, MemorySection);
};

class GlobalSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, GlobalSection);
};

class ExportSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ExportSection);
};

class StartSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, StartSection);
};

class ElementSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ElementSection);
};

class DataCountSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, DataCountSection);
};

class CodeSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, CodeSection);
};

class DataSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, DataSection);
};

class CustomSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, CustomSection);
};

class FuncTypeDecl final : public TypeDecl {
private:

  FuncType * Ty;

  FuncTypeDecl(ASTContext * Context, FuncType * Ty) :
    TypeDecl(DeclKind::FuncType, Context),
    Ty(Ty){};

public:

  static FuncTypeDecl * create(ASTContext& Context, FuncType * Ty) {
    return new (Context) FuncTypeDecl(&Context, Ty);
  }

  FuncType * getType() {
    return Ty;
  }

  const FuncType * getType() const {
    return Ty;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, FuncType);
};

class ImportDecl : public TypeDecl {
private:

  Identifier Module;

  Identifier Name;

protected:

  ImportDecl(
    DeclKind Kind,
    ASTContext * Context,
    Identifier Module,
    Identifier Name
  ) :
    TypeDecl(Kind, Context),
    Module(Module),
    Name(Name){};

public:

  Identifier getModule() {
    return Module;
  }

  Identifier getModule() const {
    return Module;
  }

  Identifier getName() {
    return Name;
  }

  Identifier getName() const {
    return Name;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, ImportDecl);
};

class ImportFuncDecl final : public ImportDecl {
private:

  Identifier Module;

  Identifier Name;

  uint32_t TypeIndex;

  ImportFuncDecl(
    ASTContext * Context,
    Identifier Module,
    Identifier Name,
    uint32_t TypeIndex
  ) :
    ImportDecl(DeclKind::ImportFunc, Context, Module, Name),
    TypeIndex(TypeIndex){};

public:

  uint32_t getTypeIndex() const {
    return TypeIndex;
  }

  static ImportFuncDecl * create(
    ASTContext& Context,
    Identifier Module,
    Identifier Name,
    uint32_t TypeIndex
  ) {
    return new (Context)
      ImportFuncDecl(&Context, Module, Name, TypeIndex);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportFunc);
};

class ImportTableDecl final : public ImportDecl {
private:

  Identifier Module;

  Identifier Name;

  TableType * Type;

  ImportTableDecl(
    ASTContext * Context,
    Identifier Module,
    Identifier Name,
    TableType * Type
  ) :
    ImportDecl(DeclKind::ImportTable, Context, Module, Name),
    Type(Type){};

public:

  TableType * getType() const {
    return Type;
  }

  static ImportTableDecl * create(
    ASTContext& Context,
    Identifier Module,
    Identifier Name,
    TableType * Type
  ) {
    return new (Context) ImportTableDecl(&Context, Module, Name, Type);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportTable);
};

class ImportMemoryDecl final : public ImportDecl {
private:

  Identifier Module;

  Identifier Name;

  MemoryType * Type;

  ImportMemoryDecl(
    ASTContext * Context,
    Identifier Module,
    Identifier Name,
    MemoryType * Type
  ) :
    ImportDecl(DeclKind::ImportMemory, Context, Module, Name),
    Type(Type){};

public:

  MemoryType * getType() const {
    return Type;
  }

  static ImportMemoryDecl * create(
    ASTContext& Context,
    Identifier Module,
    Identifier Name,
    MemoryType * Type
  ) {
    return new (Context) ImportMemoryDecl(&Context, Module, Name, Type);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportMemory);
};

class ImportGlobalDecl final : public ImportDecl {
private:

  Identifier Module;

  Identifier Name;

  GlobalType * Type;

  ImportGlobalDecl(
    ASTContext * Context,
    Identifier Module,
    Identifier Name,
    GlobalType * Type
  ) :
    ImportDecl(DeclKind::ImportGlobal, Context, Module, Name),
    Type(Type){};

public:

  GlobalType * getType() const {
    return Type;
  }

  static ImportGlobalDecl * create(
    ASTContext& Context,
    Identifier Module,
    Identifier Name,
    GlobalType * Type
  ) {
    return new (Context) ImportGlobalDecl(&Context, Module, Name, Type);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportGlobal);
};

class FuncDecl final : public TypeDecl {
private:

  uint32_t TypeIndex;

  FuncDecl(ASTContext * Context, uint32_t TypeIndex) :
    TypeDecl(DeclKind::Func, Context),
    TypeIndex(TypeIndex) {
  }

public:

  static FuncDecl * create(ASTContext& Context, uint32_t TypeIndex) {
    return new (Context) FuncDecl(&Context, TypeIndex);
  }

  uint32_t getTypeIndex() const {
    return TypeIndex;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Func);
};

class TableDecl final : public TypeDecl {
private:

  TableType * Type;

  TableDecl(ASTContext * Context, TableType * Type) :
    TypeDecl(DeclKind::Func, Context),
    Type(Type) {
  }

public:

  static TableDecl * create(ASTContext& Context, TableType * Type) {
    return new (Context) TableDecl(&Context, Type);
  }

  TableType * getType() const {
    return Type;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Table);
};

class MemoryDecl final : public TypeDecl {
private:

  MemoryType * Type;

  MemoryDecl(ASTContext * Context, MemoryType * Type) :
    TypeDecl(DeclKind::Func, Context),
    Type(Type) {
  }

public:

  static MemoryDecl * create(ASTContext& Context, MemoryType * Type) {
    return new (Context) MemoryDecl(&Context, Type);
  }

  MemoryType * getType() const {
    return Type;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Table);
};

} // namespace w2n

#endif // W2N_AST_DECL_H