#include <w2n/AST/Function.h>
#include <w2n/AST/Module.h>

using namespace w2n;

Function * Function::createInit(
  ModuleDecl * Module,
  uint32_t Index,
  ValueType * ReturnType,
  ExpressionDecl * Expression,
  llvm::Optional<Identifier> Name
) {
  auto * ParamTys = ResultType::create(Module->getASTContext(), {});
  auto * ResultTys =
    ResultType::create(Module->getASTContext(), {ReturnType});
  auto * FnTy =
    FuncType::create(Module->getASTContext(), ParamTys, ResultTys);
  auto * FnTyDecl = FuncTypeDecl::create(Module->getASTContext(), FnTy);
  return new (Expression->getASTContext()) Function(
    Module,
    FunctionKind::GlobalInit,
    Index,
    Name,
    FnTyDecl,
    {},
    Expression,
    false
  );
}

std::string Function::getDescriptiveName() const {
  return (llvm::Twine(getDescriptiveKindName()) + llvm::Twine("$")
          + llvm::Twine(Index))
    .str();
}

std::string Function::getFullQualifiedDescriptiveName() const {
  return (llvm::Twine(Module->getName().str()) + llvm::Twine(".")
          + llvm::Twine(getDescriptiveName()))
    .str();
}

void Function::dump(raw_ostream& OS, unsigned Indent) const {
  OS << getDescriptiveKindName().str().data();
  OS << " $" << getIndex();
  auto Name = getName();
  if (Name.has_value()) {
    OS << " " << Name.value();
  }
  OS << " : ";
  Type->getType()->dump(OS);
}
