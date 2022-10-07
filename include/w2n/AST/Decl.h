#ifndef W2N_AST_DECL_H
#define W2N_AST_DECL_H

#include <llvm/ADT/PointerUnion.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/PointerLikeTraits.h>

namespace w2n {
class ASTContext;

enum class DeclKind {

  Module

};

/**
 * @brief Base class of all decls in w2n.
 */
class LLVM_POINTER_LIKE_ALIGNMENT(Decl) Decl : public ASTAllocated<Decl> {

private:
  DeclKind Kind;

  llvm::PointerUnion<DeclContext *, ASTContext *> Context;

protected:
  Decl(DeclKind Kind, llvm::PointerUnion<DeclContext *, ASTContext *> Context)
    : Kind(Kind), Context(Context) {}

  DeclContext * getDeclContextForModule() const;

public:
  DeclKind getKind() const { return Kind; }

  LLVM_READONLY
  DeclContext * getDeclContext() const {
    if (auto dc = Context.dyn_cast<DeclContext *>())
      return dc;

    return getDeclContextForModule();
  }

  void setDeclContext(DeclContext * DC);

  /// getASTContext - Return the ASTContext that this decl lives in.
  LLVM_READONLY
  ASTContext& getASTContext() const {
    if (auto dc = Context.dyn_cast<DeclContext *>())
      return dc->getASTContext();

    return *Context.get<ASTContext *>();
  }
};

} // namespace w2n

#endif // W2N_AST_DECL_H