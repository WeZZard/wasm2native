#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/Twine.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Type.h>

using namespace w2n;

GlobalVariable::GlobalVariable(
  ModuleDecl * Module,
  ASTLinkage Linkage,
  uint32_t Index,
  llvm::Optional<Identifier> Name,
  ValueType * Ty,
  bool IsMutable,
  bool IsExported,
  bool IsImported,
  Function * Init,
  GlobalDecl * Decl
) :
  Module(Module),
  Linkage(Linkage),
  Index(Index),
  Name(Name),
  Ty(Ty),
  IsMutable(IsMutable),
  IsExported(IsExported),
  InitAndIsImported(llvm::PointerIntPair<Function *, 1>(
    Init, static_cast<unsigned int>(IsImported)
  )),
  Decl(Decl) {
}

GlobalVariable * GlobalVariable::create(
  ModuleDecl * Module,
  ASTLinkage Linkage,
  uint32_t Index,
  llvm::Optional<Identifier> Name,
  ValueType * Ty,
  bool IsMutable,
  bool IsExported,
  Function * Init,
  GlobalDecl * Decl
) {
  return new (Module->getASTContext()) GlobalVariable(
    Module,
    Linkage,
    Index,
    Name,
    Ty,
    IsMutable,
    IsExported,
    false,
    Init,
    Decl
  );
}

GlobalVariable * GlobalVariable::create(
  ModuleDecl * Module,
  ASTLinkage Linkage,
  uint32_t Index,
  llvm::Optional<Identifier> Name,
  ValueType * Ty,
  bool IsMutable
) {
  return new (Module->getASTContext()) GlobalVariable(
    Module,
    Linkage,
    Index,
    Name,
    Ty,
    IsMutable,
    true,
    true,
    nullptr,
    nullptr
  );
}

std::string GlobalVariable::getDescriptiveName() const {
  return (llvm::Twine("global$") + llvm::Twine(getIndex())).str();
}

std::string GlobalVariable::getFullQualifiedDescriptiveName() const {
  return (llvm::Twine(Module->getName().str()) + llvm::Twine(".")
          + llvm::Twine(getDescriptiveName()))
    .str();
}