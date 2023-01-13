#ifndef W2N_AST_DECL_H
#define W2N_AST_DECL_H

#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringRef.h>
#include <cstdint>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTWalker.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/NameAssociation.h>
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

  /// This recursively walks the AST rooted at this expression.
  bool walk(ASTWalker& Walker);

  bool walk(ASTWalker&& Walker) {
    return walk(Walker);
  }
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

class FuncSectionDecl final : public SectionDecl {
private:

  std::vector<uint32_t> FuncTypes;

  FuncSectionDecl(ASTContext * Ctx, std::vector<uint32_t> FuncTypes) :
    SectionDecl(DeclKind::FuncSection, Ctx),
    FuncTypes(FuncTypes) {
  }

public:

  static FuncSectionDecl *
  create(ASTContext& Ctx, std::vector<uint32_t> FuncTypes) {
    return new (Ctx) FuncSectionDecl(&Ctx, FuncTypes);
  }

  std::vector<uint32_t>& getFuncTypes() {
    return FuncTypes;
  }

  const std::vector<uint32_t>& getFuncTypes() const {
    return FuncTypes;
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

class GlobalDecl;

class GlobalSectionDecl final : public SectionDecl {
private:

  std::vector<GlobalDecl *> Globals;

  GlobalSectionDecl(ASTContext * Ctx, std::vector<GlobalDecl *> Globals) :
    SectionDecl(DeclKind::GlobalSection, Ctx),
    Globals(Globals) {
  }

public:

  static GlobalSectionDecl *
  create(ASTContext& Ctx, std::vector<GlobalDecl *> Globals) {
    return new (Ctx) GlobalSectionDecl(&Ctx, Globals);
  }

  std::vector<GlobalDecl *>& getGlobals() {
    return Globals;
  }

  const std::vector<GlobalDecl *>& getGlobals() const {
    return Globals;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, GlobalSection);
};

class ExportDecl;

class ExportSectionDecl final : public SectionDecl {
private:

  std::vector<ExportDecl *> Exports;

  ExportSectionDecl(ASTContext * Ctx, std::vector<ExportDecl *> Exports) :
    SectionDecl(DeclKind::ExportSection, Ctx),
    Exports(Exports) {
  }

public:

  static ExportSectionDecl *
  create(ASTContext& Ctx, std::vector<ExportDecl *> Exports) {
    return new (Ctx) ExportSectionDecl(&Ctx, Exports);
  }

  std::vector<ExportDecl *>& getGlobals() {
    return Exports;
  }

  const std::vector<ExportDecl *>& getGlobals() const {
    return Exports;
  }

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

class CustomSectionDecl : public SectionDecl {
protected:

  CustomSectionDecl(DeclKind Kind, ASTContext * Context) :
    SectionDecl(Kind, Context) {
  }

public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, CustomSectionDecl);
};

class ModuleNameSubsectionDecl;
class FuncNameSubsectionDecl;
class LocalNameSubsectionDecl;

class NameSectionDecl final : public CustomSectionDecl {
private:

  ModuleNameSubsectionDecl * ModuleNames;

  FuncNameSubsectionDecl * FuncNames;

  LocalNameSubsectionDecl * LocalNames;

  NameSectionDecl(
    ASTContext * Context,
    ModuleNameSubsectionDecl * ModuleNames,
    FuncNameSubsectionDecl * FuncNames,
    LocalNameSubsectionDecl * LocalNames
  ) :
    CustomSectionDecl(DeclKind::NameSection, Context),
    ModuleNames(ModuleNames),
    FuncNames(FuncNames),
    LocalNames(LocalNames) {
  }

public:

  static NameSectionDecl * create(
    ASTContext& Context,
    ModuleNameSubsectionDecl * ModuleNames,
    FuncNameSubsectionDecl * FuncNames,
    LocalNameSubsectionDecl * LocalNames
  ) {
    return new (Context)
      NameSectionDecl(&Context, ModuleNames, FuncNames, LocalNames);
  }

  ModuleNameSubsectionDecl * getModuleNames() {
    return ModuleNames;
  }

  const ModuleNameSubsectionDecl * getModuleNames() const {
    return ModuleNames;
  }

  FuncNameSubsectionDecl * getFuncNames() {
    return FuncNames;
  }

  const FuncNameSubsectionDecl * getFuncNames() const {
    return FuncNames;
  }

  LocalNameSubsectionDecl * getLocalNames() {
    return LocalNames;
  }

  const LocalNameSubsectionDecl * getLocalNames() const {
    return LocalNames;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, NameSection);
};

class CodeDecl;

class CodeSectionDecl final : public SectionDecl {
private:

  std::vector<CodeDecl *> Codes;

  CodeSectionDecl(ASTContext * Ctx, std::vector<CodeDecl *> Codes) :
    SectionDecl(DeclKind::CodeSection, Ctx),
    Codes(Codes) {
  }

public:

  static CodeSectionDecl *
  create(ASTContext& Ctx, std::vector<CodeDecl *> Codes) {
    return new (Ctx) CodeSectionDecl(&Ctx, Codes);
  }

  std::vector<CodeDecl *>& getCodes() {
    return Codes;
  }

  const std::vector<CodeDecl *>& getCodes() const {
    return Codes;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, CodeSection);
};

class DataDecl;

class DataSectionDecl final : public SectionDecl {
private:

  std::vector<DataDecl *> Data;

  DataSectionDecl(ASTContext * Ctx, std::vector<DataDecl *> Data) :
    SectionDecl(DeclKind::CodeSection, Ctx),
    Data(Data) {
  }

public:

  static DataSectionDecl *
  create(ASTContext& Ctx, std::vector<DataDecl *> Data) {
    return new (Ctx) DataSectionDecl(&Ctx, Data);
  }

  std::vector<DataDecl *>& getData() {
    return Data;
  }

  const std::vector<DataDecl *>& getData() const {
    return Data;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, DataSection);
};

class NameSubsectionDecl : public TypeDecl {
protected:

  NameSubsectionDecl(DeclKind Kind, ASTContext * Context) :
    TypeDecl(Kind, Context){};

public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, NameSubsectionDecl);
};

class ModuleNameSubsectionDecl final : public NameSubsectionDecl {
private:

  std::vector<Identifier> Names;

  ModuleNameSubsectionDecl(
    ASTContext * Context, std::vector<Identifier> Names
  ) :
    NameSubsectionDecl(DeclKind::ModuleNameSubsection, Context),
    Names(Names) {
  }

public:

  static ModuleNameSubsectionDecl *
  create(ASTContext& Context, std::vector<Identifier> Names) {
    return new (Context) ModuleNameSubsectionDecl(&Context, Names);
  }

  std::vector<Identifier>& getNameMaps() {
    return Names;
  }

  const std::vector<Identifier>& getNameMaps() const {
    return Names;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ModuleNameSubsection);
};

class FuncNameSubsectionDecl final : public NameSubsectionDecl {
private:

  std::vector<NameAssociation> NameMap;

  FuncNameSubsectionDecl(
    ASTContext * Context, std::vector<NameAssociation> NameMap
  ) :
    NameSubsectionDecl(DeclKind::ModuleNameSubsection, Context),
    NameMap(NameMap) {
  }

public:

  static FuncNameSubsectionDecl *
  create(ASTContext& Context, std::vector<NameAssociation> NameMap) {
    return new (Context) FuncNameSubsectionDecl(&Context, NameMap);
  }

  std::vector<NameAssociation>& getNameMap() {
    return NameMap;
  }

  const std::vector<NameAssociation>& getNameMap() const {
    return NameMap;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, FuncNameSubsection);
};

class LocalNameSubsectionDecl final : public NameSubsectionDecl {
private:

  std::vector<IndirectNameAssociation> IndirectNameMap;

  LocalNameSubsectionDecl(
    ASTContext * Context,
    std::vector<IndirectNameAssociation> IndirectNameMaps
  ) :
    NameSubsectionDecl(DeclKind::ModuleNameSubsection, Context),
    IndirectNameMap(IndirectNameMaps) {
  }

public:

  static LocalNameSubsectionDecl * create(
    ASTContext& Context,
    std::vector<IndirectNameAssociation> IndirectNameMap
  ) {
    return new (Context)
      LocalNameSubsectionDecl(&Context, IndirectNameMap);
  }

  std::vector<IndirectNameAssociation>& getIndirectNameMap() {
    return IndirectNameMap;
  }

  const std::vector<IndirectNameAssociation>& getIndirectNameMap() const {
    return IndirectNameMap;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, LocalNameSubsection);
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

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Memory);
};

class ExpressionDecl;

class GlobalDecl final : public TypeDecl {
private:

  GlobalType * Type;

  ExpressionDecl * Init;

  GlobalDecl(
    ASTContext * Context, GlobalType * Type, ExpressionDecl * Init
  ) :
    TypeDecl(DeclKind::Global, Context),
    Type(Type),
    Init(Init) {
  }

public:

  static GlobalDecl *
  create(ASTContext& Context, GlobalType * Type, ExpressionDecl * Init) {
    return new (Context) GlobalDecl(&Context, Type, Init);
  }

  GlobalType * getType() {
    return Type;
  }

  const GlobalType * getType() const {
    return Type;
  }

  ExpressionDecl * getInit() {
    return Init;
  }

  const ExpressionDecl * getInit() const {
    return Init;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Global);
};

class ExportDecl : public TypeDecl {
private:

  Identifier Name;

protected:

  ExportDecl(DeclKind Kind, ASTContext * Context, Identifier Name) :
    TypeDecl(Kind, Context),
    Name(Name) {
  }

public:

  Identifier& getName() {
    return Name;
  }

  const Identifier& getName() const {
    return Name;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, ExportDecl);
};

class ExportFuncDecl : public ExportDecl {
private:

  uint32_t TypeIndex;

  ExportFuncDecl(
    ASTContext * Context, Identifier Name, uint32_t TypeIndex
  ) :
    ExportDecl(DeclKind::ExportFunc, Context, Name),
    TypeIndex(TypeIndex) {
  }

public:

  static ExportFuncDecl *
  create(ASTContext& Context, Identifier Name, uint32_t TypeIndex) {
    return new (Context) ExportFuncDecl(&Context, Name, TypeIndex);
  }

  uint32_t getTypeIndex() const {
    return TypeIndex;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ExportFunc);
};

class ExportTableDecl : public ExportDecl {
private:

  uint32_t TableIndex;

  ExportTableDecl(
    ASTContext * Context, Identifier Name, uint32_t TableIndex
  ) :
    ExportDecl(DeclKind::ExportTable, Context, Name),
    TableIndex(TableIndex) {
  }

public:

  static ExportTableDecl *
  create(ASTContext& Context, Identifier Name, uint32_t TableIndex) {
    return new (Context) ExportTableDecl(&Context, Name, TableIndex);
  }

  uint32_t getTableIndex() const {
    return TableIndex;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ExportTable);
};

class ExportMemoryDecl : public ExportDecl {
private:

  uint32_t MemoryIndex;

  ExportMemoryDecl(
    ASTContext * Context, Identifier Name, uint32_t MemoryIndex
  ) :
    ExportDecl(DeclKind::ExportMemory, Context, Name),
    MemoryIndex(MemoryIndex) {
  }

public:

  static ExportMemoryDecl *
  create(ASTContext& Context, Identifier Name, uint32_t MemoryIndex) {
    return new (Context) ExportMemoryDecl(&Context, Name, MemoryIndex);
  }

  uint32_t getMemoryIndex() const {
    return MemoryIndex;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ExportMemory);
};

class ExportGlobalDecl : public ExportDecl {
private:

  uint32_t GlobalIndex;

  ExportGlobalDecl(
    ASTContext * Context, Identifier Name, uint32_t GlobalIndex
  ) :
    ExportDecl(DeclKind::ExportGlobal, Context, Name),
    GlobalIndex(GlobalIndex) {
  }

public:

  static ExportGlobalDecl *
  create(ASTContext& Context, Identifier Name, uint32_t GlobalIndex) {
    return new (Context) ExportGlobalDecl(&Context, Name, GlobalIndex);
  }

  uint32_t getGlobalIndex() const {
    return GlobalIndex;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ExportGlobal);
};

class FuncDecl;

class CodeDecl : public TypeDecl {
  uint32_t Size;

  FuncDecl * Func;

  CodeDecl(ASTContext * Context, uint32_t Size, FuncDecl * Func) :
    TypeDecl(DeclKind::Func, Context),
    Size(Size),
    Func(Func) {
  }

public:

  static CodeDecl *
  create(ASTContext& Context, uint32_t Size, FuncDecl * Func) {
    return new (Context) CodeDecl(&Context, Size, Func);
  }

  uint32_t getSize() const {
    return Size;
  }

  FuncDecl * getFunc() {
    return Func;
  }

  const FuncDecl * getFunc() const {
    return Func;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Code);
};

class LocalDecl;

class FuncDecl final : public TypeDecl {
private:

  std::vector<LocalDecl *> Locals;

  ExpressionDecl * Expression;

  FuncDecl(
    ASTContext * Context,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression
  ) :
    TypeDecl(DeclKind::Func, Context),
    Locals(Locals),
    Expression(Expression) {
  }

public:

  static FuncDecl * create(
    ASTContext& Context,
    std::vector<LocalDecl *> Locals,
    ExpressionDecl * Expression
  ) {
    return new (Context) FuncDecl(&Context, Locals, Expression);
  }

  std::vector<LocalDecl *>& getLocals() {
    return Locals;
  }

  const std::vector<LocalDecl *>& getLocals() const {
    return Locals;
  }

  ExpressionDecl * getExpression() {
    return Expression;
  }

  const ExpressionDecl * getExpression() const {
    return Expression;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Func);
};

class LocalDecl : public TypeDecl {
  uint32_t Count;

  ValueType * Type;

  LocalDecl(ASTContext * Context, uint32_t Count, ValueType * Type) :
    TypeDecl(DeclKind::Local, Context),
    Count(Count),
    Type(Type) {
  }

public:

  static LocalDecl *
  create(ASTContext& Context, uint32_t Count, ValueType * Type) {
    return new (Context) LocalDecl(&Context, Count, Type);
  }

  uint32_t getCount() const {
    return Count;
  }

  ValueType * getType() {
    return Type;
  }

  const ValueType * getType() const {
    return Type;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Local);
};

class DataDecl : public TypeDecl {
protected:

  std::vector<uint8_t> Data;

  DataDecl(
    DeclKind Kind, ASTContext * Context, std::vector<uint8_t> Data
  ) :
    TypeDecl(Kind, Context),
    Data(Data) {
  }

public:

  std::vector<uint8_t>& getData() {
    return Data;
  }

  const std::vector<uint8_t>& getData() const {
    return Data;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, DataDecl);
};

class ExpressionDecl;

class DataActiveDecl : public DataDecl {
private:

  uint32_t MemoryIndex;

  ExpressionDecl * Expression;

  DataActiveDecl(
    ASTContext * Context,
    uint32_t MemoryIndex,
    ExpressionDecl * Expression,
    std::vector<uint8_t> Data
  ) :
    DataDecl(DeclKind::DataPassive, Context, Data),
    MemoryIndex(MemoryIndex),
    Expression(Expression) {
  }

public:

  static DataActiveDecl * create(
    ASTContext& Context,
    uint32_t MemoryIndex,
    ExpressionDecl * Expression,
    std::vector<uint8_t> Data
  ) {
    return new (Context)
      DataActiveDecl(&Context, MemoryIndex, Expression, Data);
  }

  uint32_t getMemoryIndex() const {
    return MemoryIndex;
  }

  ExpressionDecl * getExpression() {
    return Expression;
  }

  const ExpressionDecl * getExpression() const {
    return Expression;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, DataActive);
};

class DataPassiveDecl : public DataDecl {
private:

  DataPassiveDecl(ASTContext * Context, std::vector<uint8_t> Data) :
    DataDecl(DeclKind::DataPassive, Context, Data) {
  }

public:

  static DataPassiveDecl *
  create(ASTContext& Context, std::vector<uint8_t> Data) {
    return new (Context) DataPassiveDecl(&Context, Data);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, DataPassive);
};

class InstNode;

class ExpressionDecl : public TypeDecl {
private:

  std::vector<InstNode> Instructions;

  ExpressionDecl(
    ASTContext * Context, std::vector<InstNode> Instructions
  ) :
    TypeDecl(DeclKind::Expression, Context),
    Instructions(Instructions) {
  }

public:

  static ExpressionDecl *
  create(ASTContext& Context, std::vector<InstNode> Instructions) {
    return new (Context) ExpressionDecl(&Context, Instructions);
  }

  std::vector<InstNode>& getInstructions() {
    return Instructions;
  }

  const std::vector<InstNode>& getInstructions() const {
    return Instructions;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, Expression);
};

} // namespace w2n

#endif // W2N_AST_DECL_H