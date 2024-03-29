#ifndef W2N_AST_DECLCONTEXT_H
#define W2N_AST_DECLCONTEXT_H

#include <llvm/ADT/PointerIntPair.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/Basic/SourceLoc.h>

namespace w2n {
class Decl;
class ModuleDecl;
class SourceFile;

enum class DeclContextKind {
  FileUnit,
  Module,
  Last_DeclContextKind = Module,
};

class LLVM_POINTER_LIKE_ALIGNMENT(DeclContext) DeclContext :
  public ASTAllocated<DeclContext> {
  enum class ASTHierarchy : unsigned {
    Decl,
    FileUnit,
  };

  llvm::PointerIntPair<DeclContext *, 3, ASTHierarchy> ParentAndKind;

  void setParent(DeclContext * Parent) {
    ParentAndKind.setPointer(Parent);
  }

  static ASTHierarchy getASTHierarchyFromKind(DeclContextKind Kind) {
    switch (Kind) {
    case DeclContextKind::FileUnit: return ASTHierarchy::FileUnit;
    case DeclContextKind::Module: return ASTHierarchy::Decl;
    }
    llvm_unreachable("Unhandled DeclContextKind");
  }

public:

  LLVM_READONLY
  Decl * getAsDecl() {
    return ParentAndKind.getInt() == ASTHierarchy::Decl
           ? reinterpret_cast<Decl *>(this + 1)
           : nullptr;
  }

  const Decl * getAsDecl() const {
    return const_cast<DeclContext *>(this)->getAsDecl();
  }

  DeclContext(DeclContextKind Kind, DeclContext * Parent) :
    ParentAndKind(Parent, getASTHierarchyFromKind(Kind)) {
    if (Kind != DeclContextKind::Module) {
      assert(
        Parent != nullptr && "DeclContext must have a parent context"
      );
    }
  }

  DeclContextKind getContextKind() const;

  /**
   * @brief Return \c true if this is a subclass of Module.
   *
   * @return bool
   *
   * @see \file w2n/AST/Module.h
   */
  LLVM_READONLY
  bool isModuleContext() const;

  /**
   * @brief Return the \c ASTContext for a specified \c DeclContext by
   * walking up to the enclosing module and returning its \c ASTContext.
   *
   * @return ASTContext&
   */
  LLVM_READONLY
  ASTContext& getASTContext() const;

  /// Returns the semantic parent of this context.  A context has a
  /// parent if and only if it is not a module context.
  DeclContext * getParent() const {
    return ParentAndKind.getPointer();
  }

  ModuleDecl * getParentModule() const;

  /// Returns the source file that contains this context, or null if this
  /// is not within a source file.
  LLVM_READONLY
  SourceFile * getParentSourceFile() const;

  /**
   * @brief Returns the module scope context that contains this context.
   *
   * @return DeclContext * either a \c Module or a \c FileUnit.
   */
  LLVM_READONLY
  DeclContext * getModuleScopeContext() const;

  /// \returns true if this is a context with module-wide scope, e.g. a
  /// module or a source file.
  /// \see \file <w2n/AST/Module.h>
  LLVM_READONLY
  bool isModuleScopeContext() const;

  /**
   * @brief Some \c Decl are of \c DeclContext, but not all.
   *
   * @return bool
   *
   * @see \file w2n/AST/Decl.h
   */
  static bool classof(const Decl * D);
};

/// Define simple_display for DeclContexts but not for subclasses in order
/// to avoid ambiguities with Decl* arguments.
template <
  typename ParamT,
  typename = typename std::enable_if<
    std::is_same<ParamT, DeclContext>::value>::type>
void simple_display(llvm::raw_ostream& os, const ParamT * ss) {
  if (ss) {
    ss->printContext(os, 0, true);
  } else {
    os << "(null)";
  }
}

/// Extract the source location from the given declaration context.
SourceLoc extractNearestSourceLoc(const DeclContext * DC);

} // namespace w2n

#endif // W2N_AST_DECLCONTEXT_H
