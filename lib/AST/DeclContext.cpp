#include <llvm/Support/Casting.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Module.h>

using namespace w2n;

ASTContext& DeclContext::getASTContext() const {
  return getParentModule()->getASTContext();
}

ModuleDecl * DeclContext::getParentModule() const {
  const DeclContext * DC = this;
  while (!DC->isModuleContext())
    DC = DC->getParent();
  return const_cast<ModuleDecl *>(cast<ModuleDecl>(DC));
}
