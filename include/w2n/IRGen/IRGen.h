#ifndef W2N_IRGEN_IRGEN_H
#define W2N_IRGEN_IRGEN_H

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Mutex.h>
#include <llvm/Target/TargetMachine.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/Basic/Statistic.h>
#include <w2n/TBDGen/TBDGen.h>

namespace w2n {

bool performLLVM(
  const IRGenOptions& Opts,
  DiagnosticEngine& Diags,
  llvm::sys::Mutex * DiagMutex,
  llvm::GlobalVariable * HashGlobal,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  StringRef OutputFilename,
  UnifiedStatsReporter * Stats
);

/// Given an already created LLVM module, construct a pass pipeline and
/// run the Swift LLVM Pipeline upon it. This does not cause the module to
/// be printed, only to be optimized.
void performLLVMOptimizations(
  const IRGenOptions& Opts,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine
);

/// Compiles and writes the given LLVM module into an output stream in the
/// format specified in the \c IRGenOptions.
bool compileAndWriteLLVM(
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  const IRGenOptions& Opts,
  UnifiedStatsReporter * Stats,
  DiagnosticEngine& Diags,
  llvm::raw_pwrite_stream& Out,
  llvm::sys::Mutex * DiagMutex = nullptr
);

/// Get the CPU, subtarget feature options, and triple to use when
/// emitting code.
std::tuple<
  llvm::TargetOptions,
  std::string,
  std::vector<std::string>,
  std::string>
getIRTargetOptions(const IRGenOptions& Opts, ASTContext& Ctx);

/// Creates a TargetMachine from the IRGen options and AST Context.
std::unique_ptr<llvm::TargetMachine>
createTargetMachine(const IRGenOptions& Opts, ASTContext& Ctx);

/// Turn the given Swift module into LLVM IR and return the generated
/// module. To compile and output the generated code, call \c performLLVM.
GeneratedModule performIRGeneration(
  ModuleDecl * M,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  ArrayRef<std::string> ParallelOutputFilenames,
  llvm::GlobalVariable ** OutModuleHash = nullptr
);

/// Turn the given Swift file into LLVM IR and return the generated
/// module. To compile and output the generated code, call \c performLLVM.
GeneratedModule performIRGeneration(
  FileUnit * File,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  llvm::GlobalVariable ** OutModuleHash = nullptr
);

} // namespace w2n

#endif // W2N_IRGEN_IRGEN_H
