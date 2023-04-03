#include <cassert>
#include <cstdio>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/NameAssociation.h>
#include <w2n/AST/Type.h>
#include <w2n/AST/TypeCheckerRequests.h>

using namespace w2n;

#pragma mark - GlobalVariableRequest

GlobalVariableRequest::OutputType
GlobalVariableRequest::evaluate(Evaluator& Eval, ModuleDecl * Mod) const {
  assert(Mod);
  GlobalSectionDecl * G = Mod->getGlobalSection();

  auto Globals = std::make_shared<ModuleDecl::GlobalListType>();

  if (G == nullptr || G->getGlobals().empty()) {
    return Globals;
  }

  auto GetASTLinkage = [](GlobalDecl * D) -> ASTLinkage {
    return ASTLinkage::Internal;
  };

  uint32_t GlobalCount = 0;

  for (GlobalDecl * D : G->getGlobals()) {
    GlobalVariable * V = GlobalVariable::create(
      *Mod,
      GetASTLinkage(D),
      D->getIndex(),
      None,
      D->getType()->getType(),
      D->getType()->isMutable(),
      D
    );
    Globals->push_back(V);
    GlobalCount += 1;
  }

  llvm::errs() << "[GlobalVariableRequest] [evaluate] global count = "
               << GlobalCount << "\n";

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
  TypeSectionDecl * TypeSection = Mod->getTypeSection();
  CodeSectionDecl * CodeSection = Mod->getCodeSection();
  FuncSectionDecl * FuncSection = Mod->getFuncSection();
  ExportSectionDecl * ExportSection = Mod->getExportSection();
  NameSectionDecl * NameSection = Mod->getNameSection();

  auto Functions = std::make_shared<ModuleDecl::FunctionListType>();

  if (TypeSection == nullptr || TypeSection->getTypes().empty()) {
    return Functions;
  }

  if (CodeSection == nullptr || CodeSection->getCodes().empty()) {
    return Functions;
  }

  size_t CodeCount = CodeSection->getCodes().size();
  size_t FuncCount = FuncSection->getFuncTypes().size();

  // TODO: Diagnostic info rather than assertion.
  assert(CodeCount == FuncCount);

  struct WorkItem {
    uint32_t Index;
    Optional<Identifier> Name;
    FuncTypeDecl * Type;
    std::vector<LocalDecl *> LocalVariables;
    ExpressionDecl * Expression;
    bool IsExported;
  };

  std::vector<WorkItem> WorkItems;
  WorkItems.reserve(CodeCount);

  uint32_t WorkItemIndex = 0;
  for (CodeDecl * C : CodeSection->getCodes()) {
    WorkItems.push_back({
      WorkItemIndex,
      None,
      nullptr,
      C->getFunc()->getLocals(),
      C->getFunc()->getExpression(),
      false,
    });
    WorkItemIndex += 1;
  }

  auto& FuncTypeIndices = FuncSection->getFuncTypes();
  auto& Types = TypeSection->getTypes();

  auto FindFuncName = [&](uint32_t Index) -> Optional<Identifier> {
    if (NameSection == nullptr || NameSection->getFuncNameSubsection() == nullptr) {
      return None;
    }

    auto& FuncNameMap =
      NameSection->getFuncNameSubsection()->getNameMap();
    auto Iter = std::find_if(
      FuncNameMap.begin(),
      FuncNameMap.end(),
      [&](NameAssociation Entry) -> bool { return Entry.Index == Index; }
    );

    if (Iter != FuncNameMap.end()) {
      return Iter->Name;
    }

    return None;
  };

  auto GetExport = [&](uint32_t Index) -> bool {
    if (ExportSection == nullptr) {
      return false;
    }
    auto& Exports = ExportSection->getExports();

    auto Iter = std::find_if(
      Exports.begin(),
      Exports.end(),
      [&](ExportDecl * D) -> bool {
        if (auto * F = dyn_cast<ExportFuncDecl>(D)) {
          return F->getFuncIndex() == Index;
        }
        return false;
      }
    );

    return Iter != Exports.end();
  };

  for (size_t Index = 0; Index < FuncCount; Index++) {
    auto FuncTypeIdx = FuncTypeIndices[Index];
    auto * Type = Types[FuncTypeIdx];
    WorkItems[Index].Type = Type;
    WorkItems[Index].Name = FindFuncName(Index);
    WorkItems[Index].IsExported = GetExport(Index);
  }

  for (const auto& WorkItem : WorkItems) {
    Function * F = Function::createFunction(
      Mod,
      WorkItem.Index,
      WorkItem.Name,
      WorkItem.Type,
      WorkItem.LocalVariables,
      WorkItem.Expression,
      WorkItem.IsExported
    );
    Functions->push_back(F);
  }

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
