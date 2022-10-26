#include <llvm/Support/Casting.h>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/Module.h>

using namespace w2n;

DeclContextKind DeclContext::getContextKind() const {
  switch (ParentAndKind.getInt()) {
  case ASTHierarchy::FileUnit: return DeclContextKind::FileUnit;
  case ASTHierarchy::Decl: {
    // Decl shall be right after DeclContext in the inheritance list of a
    // declaration class.
    const auto * D = reinterpret_cast<const Decl *>(this + 1);
    switch (D->getKind()) {
    case DeclKind::Module: return DeclContextKind::Module;
    }
    llvm_unreachable("Unhandled Decl kind");
  }
  }
  llvm_unreachable("Unhandled DeclContext ASTHierarchy");
}

ASTContext& DeclContext::getASTContext() const {
  return getParentModule()->getASTContext();
}

ModuleDecl * DeclContext::getParentModule() const {
  const DeclContext * DC = this;
  while (!DC->isModuleContext())
    DC = DC->getParent();
  return const_cast<ModuleDecl *>(cast<ModuleDecl>(DC));
}

DeclContext * Decl::getDeclContextForModule() const {
  if (const auto * module = dyn_cast<ModuleDecl>(this))
    return const_cast<ModuleDecl *>(module);

  return nullptr;
}

DeclContext * DeclContext::getModuleScopeContext() const {
  auto * DC = const_cast<DeclContext *>(this);

  while (true) {
    if (DC->ParentAndKind.getInt() == ASTHierarchy::FileUnit)
      return DC;
    if (auto * NextDC = DC->getParent()) {
      DC = NextDC;
    } else {
      assert(isa<ModuleDecl>(DC->getAsDecl()));
      return DC;
    }
  }
}

SourceLoc w2n::extractNearestSourceLoc(const DeclContext * DC) {
  switch (DC->getContextKind()) {
  case DeclContextKind::Module: return SourceLoc();
  case DeclContextKind::FileUnit: return SourceLoc();
  }
  llvm_unreachable("Unhandled DeclCopntextKind in switch");
}