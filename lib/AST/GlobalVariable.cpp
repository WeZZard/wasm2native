#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Type.h>

using namespace w2n;

GlobalVariable::GlobalVariable(
  ModuleDecl& Module,
  LinkageKind Linkage,
  uint32_t Index,
  llvm::Optional<Identifier> Name,
  ValueType * Ty,
  bool IsMutable,
  // Function& Init,
  GlobalDecl * Decl
) :
  Module(Module),
  Linkage(Linkage),
  Name(Name),
  Ty(Ty),
  IsMutable(IsMutable),
  // Init(Init),
  Decl(Decl) {
}

GlobalVariable * GlobalVariable::create(
  ModuleDecl& Module,
  LinkageKind Linkage,
  uint32_t Index,
  llvm::Optional<Identifier> Name,
  ValueType * Ty,
  bool IsMutable,
  // Function& Init,
  GlobalDecl * Decl
) {
  return new (Module.getASTContext())
    GlobalVariable(Module, Linkage, Index, Name, Ty, IsMutable, Decl);
}
