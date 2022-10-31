#ifndef W2N_AST_DECL_H
#define W2N_AST_DECL_H

#include <llvm/ADT/PointerUnion.h>
#include <llvm/ADT/StringRef.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/PointerLikeTraits.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SourceLoc.h>

namespace w2n {
class ASTContext;

enum class DeclKind {

  Module
};

/// Fine-grained declaration kind that provides a description of the
/// kind of entity a declaration represents, as it would be used in
/// diagnostics.
///
/// For example, \c FuncDecl is a single declaration class, but it has
/// several descriptive entries depending on whether it is an
/// operator, global function, local function, method, (observing)
/// accessor, etc.
enum class DescriptiveDeclKind : uint8_t {
  Module,
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
    if (auto * DC = Context.dyn_cast<DeclContext *>())
      return DC;

    return getDeclContextForModule();
  }

  void setDeclContext(DeclContext * DC);

  /// getASTContext - Return the ASTContext that this decl lives in.
  LLVM_READONLY
  ASTContext& getASTContext() const {
    if (auto * DC = Context.dyn_cast<DeclContext *>())
      return DC->getASTContext();

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

SourceLoc extractNearestSourceLoc(const Decl * decl);

void simple_display(llvm::raw_ostream& out, const Decl * decl);

} // namespace w2n

#endif // W2N_AST_DECL_H