#include <iostream>
#include <w2n/Frontend/Frontend.h>
#include <w2n/FrontendTool/FrontendTool.h>

using namespace w2n;

/// Perform any actions that must have access to the ASTContext, and need to be
/// delayed until the w2n compile pipeline has finished. This may be called
/// before or after LLVM depending on when the ASTContext gets freed.
static void performEndOfPipelineActions(CompilerInstance &Instance);

static bool withSemanticAnalysis(
  CompilerInstance& Instance,
  llvm::function_ref<bool(CompilerInstance&)> cont,
  bool runDespiteErrors = false);

int w2n::performFrontend(
  llvm::ArrayRef<const char *> Args,
  const char * Argv0,
  void * MainAddr) {
  CompilerInvocation Invocation;
  CompilerInstance Instance;

  Invocation.parseArgs(Args, Instance.getDiags(), nullptr);

  std::string SetupErr;
  if (Instance.setup(Invocation, SetupErr)) {
    std::cout << SetupErr;
    return 1;
  }

  int RetVal;
  if (performCompile(Instance, RetVal)) {
    // TODO: Diagnose error.
    return 1;
  }

  return RetVal;
}

bool w2n::performCompile(CompilerInstance& Instance, int& ReturnValue) {
  bool hadError = performAction(Instance, ReturnValue);
  const auto &Invocation = Instance.getInvocation();
  const auto &FrontendOpts = Invocation.getFrontendOptions();
  const FrontendOptions::ActionType Action = FrontendOpts.RequestedAction;

  // We might have freed the ASTContext already, but in that case we would
  // have already performed these actions.
  if (Instance.hasASTContext() &&
      FrontendOptions::doesActionPerformEndOfPipelineActions(Action)) {
    performEndOfPipelineActions(Instance);
    hadError |= Instance.getASTContext().hadError();
  }
  return hadError;
}

bool w2n::performAction(CompilerInstance &Instance, int &ReturnValue) {
  const auto &Invocation = Instance.getInvocation();
  const auto &FrontendOpts = Invocation.getFrontendOptions();
  const FrontendOptions::ActionType Action = FrontendOpts.RequestedAction;

  switch (Action) {
    case FrontendOptions::ActionType::NoneAction:
      return false;
    case FrontendOptions::ActionType::EmitIR:
      break;
    case FrontendOptions::ActionType::EmitIRGen:
      break;
    case FrontendOptions::ActionType::EmitAssembly:
      break;
    case FrontendOptions::ActionType::EmitBC:
      break;
    case FrontendOptions::ActionType::EmitObject:
      break;
    case FrontendOptions::ActionType::PrintVersion:
      break;
  }

  return false;
}

bool withSemanticAnalysis(
  CompilerInstance& Instance,
  llvm::function_ref<bool(CompilerInstance&)> Continuation)
{
  Instance.performSemanticAnalysis();
  
  bool hadError = Instance.getASTContext().hadError();
  if (hadError) {
    return true;
  }

  return Continuation(Instance) || hadError;

}

/// Perform any actions that must have access to the ASTContext, and need to be
/// delayed until the w2n compile pipeline has finished. This may be called
/// before or after LLVM depending on when the ASTContext gets freed.
void performEndOfPipelineActions(CompilerInstance& Instance) {}