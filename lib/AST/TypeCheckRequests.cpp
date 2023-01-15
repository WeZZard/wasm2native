#include <w2n/AST/ASTContext.h>
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
  if (Mod == nullptr) {
    return None;
  }

  if (Mod->Globals == nullptr) {
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

namespace w2n {
// Implement the type checker type zone (zone 10).
#define W2N_TYPEID_ZONE   TypeChecker
#define W2N_TYPEID_HEADER <w2n/AST/TypeCheckerTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
} // namespace w2n
