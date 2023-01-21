#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/TypeCheckerRequests.h>

using namespace w2n;

#pragma mark - GlobalVariableRequest

GlobalVariableRequest::OutputType
GlobalVariableRequest::evaluate(Evaluator& Eval, ModuleDecl * Mod) const {
  assert(Mod);
  GlobalSectionDecl * G = Mod->getGlobalSection();
  ExportSectionDecl * E = Mod->getExportSection();

  auto Globals = std::make_shared<ModuleDecl::GlobalListType>();

  if (G == nullptr || G->getGlobals().empty()) {
    return Globals;
  }

  auto GetLinkageKind = [](GlobalDecl * D) -> LinkageKind {
    return LinkageKind::Internal;
  };

  auto GetName = [](GlobalDecl * D) -> StringRef {
    return "";
  };

  for (GlobalDecl * D : G->getGlobals()) {
    GlobalVariable * V = GlobalVariable::create(
      *Mod,
      GetLinkageKind(D),
      D->getIndex(),
      GetName(D),
      D->getType()->getType(),
      D->getType()->isMutable(),
      D
    );
    Globals->push_back(V);
  }

  return Globals;
}

evaluator::DependencySource GlobalVariableRequest::readDependencySource(
  const evaluator::DependencyRecorder& E
) const {
  return std::get<0>(getStorage())->getParentSourceFile();
}

Optional<GlobalVariableRequest::OutputType>
GlobalVariableRequest::getCachedResult() const {
  auto * Mod = std::get<0>(getStorage());
  if (Mod == nullptr || Mod->Globals == nullptr) {
    return None;
  }

  return Mod->Globals;
}

void GlobalVariableRequest::cacheResult(
  GlobalVariableRequest::OutputType Result
) const {
  auto * Mod = std::get<0>(getStorage());
  Mod->Globals = Result;
}

#pragma mark - FunctionRequest

FunctionRequest::OutputType
FunctionRequest::evaluate(Evaluator& Eval, ModuleDecl * Mod) const {
  assert(Mod);
  CodeSectionDecl * F = Mod->getCodeSection();
  ExportSectionDecl * E = Mod->getExportSection();

  auto Functions = std::make_shared<ModuleDecl::FunctionListType>();

  if (F == nullptr || F->getCodes().empty()) {
    return Functions;
  }

  auto GetLinkageKind = [](GlobalDecl * D) -> LinkageKind {
    return LinkageKind::Internal;
  };

  auto GetName = [](GlobalDecl * D) -> StringRef {
    return "";
  };

  for (CodeDecl * C : F->getCodes()) {
    //
  }

  // FIXME: not implemented

  return Functions;
}

evaluator::DependencySource FunctionRequest::readDependencySource(
  const evaluator::DependencyRecorder& E
) const {
  return std::get<0>(getStorage())->getParentSourceFile();
}

Optional<FunctionRequest::OutputType>
FunctionRequest::getCachedResult() const {
  auto * Mod = std::get<0>(getStorage());
  if (Mod == nullptr || Mod->Functions == nullptr) {
    return None;
  }

  return Mod->Functions;
}

void FunctionRequest::cacheResult(FunctionRequest::OutputType Result
) const {
  auto * Mod = std::get<0>(getStorage());
  Mod->Functions = Result;
}

#pragma mark - TableRequest

TableRequest::OutputType
TableRequest::evaluate(Evaluator& Eval, ModuleDecl * Mod) const {
  assert(Mod);
  CodeSectionDecl * F = Mod->getCodeSection();
  ExportSectionDecl * E = Mod->getExportSection();

  auto Tables = std::make_shared<ModuleDecl::TableListType>();

  if (F == nullptr || F->getCodes().empty()) {
    return Tables;
  }

  // FIXME: not implemented

  return Tables;
}

evaluator::DependencySource
TableRequest::readDependencySource(const evaluator::DependencyRecorder& E
) const {
  return std::get<0>(getStorage())->getParentSourceFile();
}

Optional<TableRequest::OutputType> TableRequest::getCachedResult() const {
  auto * Mod = std::get<0>(getStorage());
  if (Mod == nullptr || Mod->Tables == nullptr) {
    return None;
  }

  return Mod->Tables;
}

void TableRequest::cacheResult(TableRequest::OutputType Result) const {
  auto * Mod = std::get<0>(getStorage());
  Mod->Tables = Result;
}

#pragma mark - MemoryRequest

MemoryRequest::OutputType
MemoryRequest::evaluate(Evaluator& Eval, ModuleDecl * Mod) const {
  assert(Mod);
  CodeSectionDecl * F = Mod->getCodeSection();
  ExportSectionDecl * E = Mod->getExportSection();

  auto Memories = std::make_shared<ModuleDecl::MemoryListType>();

  if (F == nullptr || F->getCodes().empty()) {
    return Memories;
  }

  // FIXME: not implemented

  return Memories;
}

evaluator::DependencySource
MemoryRequest::readDependencySource(const evaluator::DependencyRecorder& E
) const {
  return std::get<0>(getStorage())->getParentSourceFile();
}

Optional<MemoryRequest::OutputType>
MemoryRequest::getCachedResult() const {
  auto * Mod = std::get<0>(getStorage());
  if (Mod == nullptr || Mod->Memories == nullptr) {
    return None;
  }

  return Mod->Memories;
}

void MemoryRequest::cacheResult(MemoryRequest::OutputType Result) const {
  auto * Mod = std::get<0>(getStorage());
  Mod->Memories = Result;
}

namespace w2n {
// Implement the type checker type zone (zone 10).
#define W2N_TYPEID_ZONE   TypeChecker
#define W2N_TYPEID_HEADER <w2n/AST/TypeCheckerTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
} // namespace w2n
