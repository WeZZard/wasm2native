#include <llvm/Target/TargetMachine.h>
#include <iostream>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/LLVMInitialize.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/FrontendTool/FrontendTool.h>
#include <w2n/IRGen/IRGen.h>

using namespace w2n;

#pragma mark - Pipeline Function Prototypes

/**
 * @brief Performs the compile requested by the user.
 *
 * @param Instance Will be reset after performIRGeneration when the
 * verifier mode is NoVerify and there were no errors.
 * @param ReturnValue
 * @return \c true if there are erros happen while compiling and vise
 * versa.
 */
static bool performCompile(CompilerInstance& Instance, int& ReturnValue);

static bool performAction(CompilerInstance& Instance, int& ReturnValue);

static bool withSemanticAnalysis(
  CompilerInstance& Instance,
  llvm::function_ref<bool(CompilerInstance&)> Continuation,
  bool RunDespiteErrors = false
);

using ModuleOrSourceFile = PointerUnion<ModuleDecl *, SourceFile *>;

static GeneratedModule generateIR(
  const IRGenOptions& IRGenOpts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  const PrimarySpecificPaths& PSPs,
  StringRef OutputFilename,
  ModuleOrSourceFile MSF,
  llvm::GlobalVariable *& HashGlobal,
  ArrayRef<std::string> ParallelOutputFilenames
);

static bool
performCompileStepsPostSema(CompilerInstance& Instance, int& ReturnValue);

static void freeASTContextIfPossible(CompilerInstance& Instance);

static bool generateCode(
  CompilerInstance& Instance,
  StringRef OutputFilename,
  llvm::Module * IRModule,
  llvm::GlobalVariable * HashGlobal
);

/// Perform any actions that must have access to the ASTContext, and need
/// to be delayed until the w2n compile pipeline has finished. This may be
/// called before or after LLVM depending on when the ASTContext gets
/// freed.
static void performEndOfPipelineActions(CompilerInstance& Instance);

#pragma mark - Implementations

int w2n::performFrontend(
  llvm::ArrayRef<const char *> Args, const char * Argv0, void * MainAddr
) {
  INITIALIZE_LLVM();

  CompilerInvocation Invocation;
  CompilerInstance Instance;

  Invocation.parseArgs(Args, Instance.getDiags(), nullptr);

  std::string SetupErr;
  if (Instance.setup(Invocation, SetupErr)) {
    std::cout << SetupErr;
    return 1;
  }

  int RetVal = 0;
  if (performCompile(Instance, RetVal)) {
    // TODO: Diagnose error.
    return 1;
  }

  return RetVal;
}

bool performCompile(CompilerInstance& Instance, int& ReturnValue) {
  bool HadError = performAction(Instance, ReturnValue);
  const auto& Invocation = Instance.getInvocation();
  const auto& FrontendOpts = Invocation.getFrontendOptions();
  const FrontendOptions::ActionType Action = FrontendOpts.RequestedAction;

  // We might have freed the ASTContext already, but in that case we would
  // have already performed these actions.
  if (Instance.hasASTContext() && FrontendOptions::doesActionPerformEndOfPipelineActions(Action)) {
    performEndOfPipelineActions(Instance);
    HadError |= Instance.getASTContext().hadError();
  }
  return HadError;
}

bool performAction(CompilerInstance& Instance, int& ReturnValue) {
  const auto& Invocation = Instance.getInvocation();
  const auto& FrontendOpts = Invocation.getFrontendOptions();
  const FrontendOptions::ActionType Action = FrontendOpts.RequestedAction;

  switch (Action) {
  case FrontendOptions::ActionType::NoneAction: return false;
  case FrontendOptions::ActionType::EmitIR:
  case FrontendOptions::ActionType::EmitIRGen:
  case FrontendOptions::ActionType::EmitAssembly:
  case FrontendOptions::ActionType::EmitBC:
  case FrontendOptions::ActionType::EmitObject:
    return withSemanticAnalysis(
      Instance,
      [&](CompilerInstance& Instance) {
        return performCompileStepsPostSema(Instance, ReturnValue);
      }
    );
  case FrontendOptions::ActionType::PrintVersion: break;
  }

  return false;
}

bool withSemanticAnalysis(
  CompilerInstance& Instance,
  llvm::function_ref<bool(CompilerInstance&)> Continuation,
  bool RunDespiteErrors
) {
  Instance.performSemanticAnalysis();

  bool HadError = Instance.getASTContext().hadError();
  if (HadError) {
    return true;
  }

  return Continuation(Instance) || HadError;
}

GeneratedModule generateIR(
  const IRGenOptions& IRGenOpts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  const PrimarySpecificPaths& PSPs,
  StringRef OutputFilename,
  ModuleOrSourceFile MSF,
  llvm::GlobalVariable *& HashGlobal,
  ArrayRef<std::string> ParallelOutputFilenames
) {
  if (auto * File = MSF.dyn_cast<SourceFile *>()) {
    return performIRGeneration(
      File, IRGenOpts, TBDOpts, Mod, OutputFilename, PSPs, &HashGlobal
    );
  }

  return performIRGeneration(
    MSF.get<ModuleDecl *>(),
    IRGenOpts,
    TBDOpts,
    Mod,
    OutputFilename,
    PSPs,
    ParallelOutputFilenames,
    &HashGlobal
  );
}

bool performCompileStepsPostSema(
  CompilerInstance& Instance, int& ReturnValue
) {
  const auto& Invocation = Instance.getInvocation();
  const auto& Opts = Invocation.getFrontendOptions();
  if (!Instance.getPrimarySourceFiles().empty()) {
    bool Result = false;
    for (auto * PrimaryFile : Instance.getPrimarySourceFiles()) {
      const PrimarySpecificPaths& PSPs =
        Instance.getPrimarySpecificPathsForSourceFile(*PrimaryFile);
      IRGenOptions IRGenOpts = Invocation.getIRGenOptions();
      StringRef OutputFilename = PSPs.OutputFilename;
      std::vector<std::string> ParallelOutputFilenames =
        Opts.InputsAndOutputs.copyOutputFilenames();
      llvm::GlobalVariable * HashGlobal;
      auto * Mod = PrimaryFile->getModule();
      auto IRModule = generateIR(
        IRGenOpts,
        Invocation.getTBDGenOptions(),
        Mod,
        PSPs,
        OutputFilename,
        PrimaryFile,
        HashGlobal,
        ParallelOutputFilenames
      );
      Result |= generateCode(
        Instance, OutputFilename, IRModule.getModule(), HashGlobal
      );
    }
    return Result;
  }

  return false;
}

/// Perform any actions that must have access to the ASTContext, and need
/// to be delayed until the w2n compile pipeline has finished. This may be
/// called before or after LLVM depending on when the ASTContext gets
/// freed.
void performEndOfPipelineActions(CompilerInstance& Instance) {
}

void freeASTContextIfPossible(CompilerInstance& Instance) {
  // If the stats reporter is installed, we need the ASTContext to live
  // through the entire compilation process.
  if (Instance.getASTContext().Stats != nullptr) {
    return;
  }

  // If this instance is used for multiple compilations, we need the
  // ASTContext to live.
  if (Instance.getInvocation()
        .getFrontendOptions()
        .ReuseFrontendForMultipleCompilations) {
    return;
  }

  const auto& Opts = Instance.getInvocation().getFrontendOptions();

  // If there are multiple primary inputs it is too soon to free
  // the ASTContext, etc.. OTOH, if this compilation generates code for >
  // 1 primary input, then freeing it after processing the last primary is
  // unlikely to reduce the peak heap size. So, only optimize the
  // single-primary-case (or WMO).
  if (Opts.InputsAndOutputs.hasMultiplePrimaryInputs()) {
    return;
  }

  // Make sure to perform the end of pipeline actions now, because they
  // need access to the ASTContext.
  performEndOfPipelineActions(Instance);

  Instance.freeASTContext();
}

bool generateCode(
  CompilerInstance& Instance,
  StringRef OutputFilename,
  llvm::Module * IRModule,
  llvm::GlobalVariable * HashGlobal
) {
  IRModule->dump();

  const auto& Opts = Instance.getInvocation().getIRGenOptions();
  std::unique_ptr<llvm::TargetMachine> TargetMachine =
    createTargetMachine(Opts, Instance.getASTContext());

  // Free up some compiler resources now that we have an IRModule.
  freeASTContextIfPossible(Instance);

  // If we emitted any errors while performing the end-of-pipeline
  // actions, bail.
  if (Instance.getDiags().hadAnyError()) {
    return true;
  }

  // Now that we have a single IR Module, hand it over to performLLVM.
  return performLLVM(
    Opts,
    Instance.getDiags(),
    nullptr,
    HashGlobal,
    IRModule,
    TargetMachine.get(),
    OutputFilename,
    Instance.getStatsReporter()
  );
}