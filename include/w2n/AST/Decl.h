#ifndef W2N_AST_DECL_H
#define W2N_AST_DECL_H

#include <_types/_uint32_t.h>
#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringRef.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/DeclContext.h>
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

class SectionDecl : public Decl {
protected:

  SectionDecl(DeclKind Kind, ASTContext * Ctx) : Decl(Kind, Ctx) {
  }

public:

  LLVM_RTTI_CLASSOF_NONLEAF_CLASS(Decl, SectionDecl);
};

SourceLoc extractNearestSourceLoc(const SectionDecl * Decl);

void simple_display(llvm::raw_ostream& Out, const SectionDecl * Decl);

class FuncTypeDecl;

class TypeSectionDecl final : public SectionDecl {
private:

  std::vector<FuncTypeDecl *> TypeDecls;

  TypeSectionDecl(
    ASTContext * Ctx, std::vector<FuncTypeDecl *> TypeDecls
  ) :
    SectionDecl(DeclKind::TypeSection, Ctx),
    TypeDecls(TypeDecls) {
  }

public:

  static TypeSectionDecl *
  create(ASTContext& Ctx, std::vector<FuncTypeDecl *> TypeDecls) {
    return new (Ctx) TypeSectionDecl(&Ctx, TypeDecls);
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, TypeSection);
};

class ImportSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, ImportSection);
};

class FuncSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, FuncSection);
};

class TableSectionDecl final : public SectionDecl {
public:

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, TableSection);
};

class MemorySectionDecl final : public SectionDecl {
public:

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

class FuncTypeDecl final : public Decl {
private:

  SmallVector<Type *, 4> Parameters;

  SmallVector<Type *, 1> Returns;

  FuncTypeDecl(
    ASTContext * Context,
    SmallVector<Type *, 4> Parameters,
    SmallVector<Type *, 1> Returns
  ) :
    Decl(DeclKind::FuncType, Context),
    Parameters(Parameters),
    Returns(Returns){};

public:

  static FuncTypeDecl * create(
    ASTContext& Context,
    SmallVector<Type *, 4> Parameters,
    SmallVector<Type *, 1> Returns
  ) {
    return new (Context) FuncTypeDecl(&Context, Parameters, Returns);
  }

  SmallVector<Type *, 4>& getParameters() {
    return Parameters;
  }

  const SmallVector<Type *, 4>& getParameters() const {
    return Parameters;
  }

  SmallVector<Type *, 1>& getReturns() {
    return Returns;
  }

  const SmallVector<Type *, 1>& getReturns() const {
    return Returns;
  }

  USE_DEFAULT_DECL_IMPL_FOR_PROTOTYPE;

  LLVM_RTTI_CLASSOF_LEAF_CLASS(Decl, FuncType);
};

} // namespace w2n

#endif // W2N_AST_DECL_H