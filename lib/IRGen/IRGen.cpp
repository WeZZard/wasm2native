#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Instrumentation.h>
#include <llvm/Transforms/Instrumentation/AddressSanitizer.h>
#include <llvm/Transforms/Instrumentation/InstrProfiling.h>
#include <llvm/Transforms/Instrumentation/SanitizerCoverage.h>
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include <llvm/Transforms/ObjCARC.h>
#include <llvm/Transforms/Scalar.h>
#include <w2n/AST/DiagnosticsCommon.h>
#include <w2n/AST/DiagnosticsIRGen.h>
#include <w2n/IRGen/IRGen.h>

using namespace w2n;
using namespace llvm;

template <typename... ArgTypes>
void diagnoseSync(
  DiagnosticEngine& Diags,
  llvm::sys::Mutex * DiagMutex,
  SourceLoc Loc,
  Diag<ArgTypes...> ID,
  typename w2n::detail::PassArgument<ArgTypes>::type... Args
) {
  if (DiagMutex)
    DiagMutex->lock();

  Diags.diagnose(Loc, ID, std::move(Args)...);

  if (DiagMutex)
    DiagMutex->unlock();
}

bool w2n::performLLVM(
  const IRGenOptions& Opts,
  DiagnosticEngine& Diags,
  llvm::sys::Mutex * DiagMutex,
  llvm::GlobalVariable * HashGlobal,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  StringRef OutputFilename,
  UnifiedStatsReporter * Stats
) {
  Optional<raw_fd_ostream> RawOS;
  if (!OutputFilename.empty()) {
    // Try to open the output file.  Clobbering an existing file is fine.
    // Open in binary mode if we're doing binary output.
    llvm::sys::fs::OpenFlags OSFlags = llvm::sys::fs::OF_None;
    std::error_code EC;
    RawOS.emplace(OutputFilename, EC, OSFlags);

    if (RawOS->has_error() || EC) {
      diagnoseSync(
        Diags, DiagMutex, SourceLoc(), diag::error_opening_output,
        OutputFilename, EC.message()
      );
      RawOS->clear_error();
      return true;
    }
    if (Opts.OutputKind == IRGenOutputKind::LLVMAssemblyBeforeOptimization) {
      Module->print(RawOS.value(), nullptr);
      return false;
    }
  } else {
    assert(
      Opts.OutputKind == IRGenOutputKind::Module && "no output specified"
    );
  }

  if (!RawOS)
    return false;

  return compileAndWriteLLVM(
    Module, TargetMachine, Opts, Stats, Diags, *RawOS, DiagMutex
  );
}

void w2n::performLLVMOptimizations(
  const IRGenOptions& Opts,
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine
) {
}

bool w2n::compileAndWriteLLVM(
  llvm::Module * Module,
  llvm::TargetMachine * TargetMachine,
  const IRGenOptions& Opts,
  UnifiedStatsReporter * Stats,
  DiagnosticEngine& Diags,
  llvm::raw_pwrite_stream& Out,
  llvm::sys::Mutex * DiagMutex
) {
  legacy::PassManager EmitPasses;

  // Set up the final emission passes.
  switch (Opts.OutputKind) {
  case IRGenOutputKind::LLVMAssemblyBeforeOptimization:
    llvm_unreachable("Should be handled earlier.");
  case IRGenOutputKind::Module: break;
  case IRGenOutputKind::LLVMAssemblyAfterOptimization:
    EmitPasses.add(createPrintModulePass(Out));
    break;
  case IRGenOutputKind::LLVMBitcode: {
    // Emit a module summary by default for Regular LTO except ld64-based
    // ones (which use the legacy LTO API).
    bool EmitRegularLTOSummary =
      TargetMachine->getTargetTriple().getVendor() != llvm::Triple::Apple;

    if (EmitRegularLTOSummary || Opts.LLVMLTOKind == IRGenLLVMLTOKind::Thin) {
      // Rename anon globals to be able to export them in the summary.
      EmitPasses.add(createNameAnonGlobalPass());
    }

    if (Opts.LLVMLTOKind == IRGenLLVMLTOKind::Thin) {
      EmitPasses.add(createWriteThinLTOBitcodePass(Out));
    } else {
      if (EmitRegularLTOSummary) {
        Module->addModuleFlag(
          llvm::Module::Error, "ThinLTO", uint32_t(0)
        );
        // Assume other sources are compiled with -fsplit-lto-unit (it's
        // enabled by default when -flto is specified on platforms that
        // support regular lto summary.)
        Module->addModuleFlag(
          llvm::Module::Error, "EnableSplitLTOUnit", uint32_t(1)
        );
      }
      EmitPasses.add(createBitcodeWriterPass(
        Out, /*ShouldPreserveUseListOrder*/ false, EmitRegularLTOSummary
      ));
    }
    break;
  }
  case IRGenOutputKind::NativeAssembly:
  case IRGenOutputKind::ObjectFile: {
    CodeGenFileType FileType;
    FileType =
      (Opts.OutputKind == IRGenOutputKind::NativeAssembly
         ? CGFT_AssemblyFile
         : CGFT_ObjectFile);
    EmitPasses.add(createTargetTransformInfoWrapperPass(
      TargetMachine->getTargetIRAnalysis()
    ));

    bool Failed = TargetMachine->addPassesToEmitFile(
      EmitPasses, Out, nullptr, FileType, !Opts.Verify
    );
    if (Failed) {
      diagnoseSync(
        Diags, DiagMutex, SourceLoc(), diag::error_codegen_init_fail
      );
      return true;
    }
    break;
  }
  }

  EmitPasses.run(*Module);

  if (Stats) {
    if (DiagMutex)
      DiagMutex->lock();
    Stats->getFrontendCounters().NumLLVMBytesOutput += Out.tell();
    if (DiagMutex)
      DiagMutex->unlock();
  }
  return false;
}

std::tuple<
  llvm::TargetOptions,
  std::string,
  std::vector<std::string>,
  std::string>
w2n::getIRTargetOptions(const IRGenOptions& Opts, ASTContext& Ctx) {
  // Things that maybe we should collect from the command line:
  //   - relocation model
  //   - code model
  // FIXME: We should do this entirely through Clang, for consistency.
  TargetOptions TargetOpts;

  // Explicitly request debugger tuning for LLDB which is the default
  // on Darwin platforms but not on others.
  TargetOpts.DebuggerTuning = llvm::DebuggerKind::LLDB;
  TargetOpts.FunctionSections = Opts.FunctionSections;

  // WebAssembly doesn't support atomics yet, see
  // https://github.com/apple/swift/issues/54533 for more details.
  // FIXME: Thus we need to use single thread model?
  TargetOpts.ThreadModel = llvm::ThreadModel::Single;

  if (Opts.EnableGlobalISel) {
    TargetOpts.EnableGlobalISel = true;
    TargetOpts.GlobalISelAbort = GlobalISelAbortMode::DisableWithDiag;
  }

  return std::make_tuple(
    TargetOpts,
    "generic",                  // FIXME: currently hard coded
    std::vector<std::string>(), // FIXME: currently hard coded
    sys::getDefaultTargetTriple()
  );
}

std::unique_ptr<llvm::TargetMachine>
w2n::createTargetMachine(const IRGenOptions& Opts, ASTContext& Ctx) {
  CodeGenOpt::Level OptLevel = Opts.shouldOptimize()
                               ? CodeGenOpt::Default // -Os
                               : CodeGenOpt::None;

  // Set up TargetOptions and create the target features string.
  TargetOptions TargetOpts;
  std::string CPU;
  std::string EffectiveClangTriple;
  std::vector<std::string> targetFeaturesArray;
  std::tie(TargetOpts, CPU, targetFeaturesArray, EffectiveClangTriple) =
    getIRTargetOptions(Opts, Ctx);
  const llvm::Triple& EffectiveTriple =
    llvm::Triple(EffectiveClangTriple);
  std::string targetFeatures;
  if (!targetFeaturesArray.empty()) {
    llvm::SubtargetFeatures features;
    for (const std::string& feature : targetFeaturesArray) {
      // FIXME: thumb-mode: if (!shouldRemoveTargetFeature(feature)) {
      features.AddFeature(feature);
      // }
    }
    targetFeatures = features.getString();
  }

  // TODO: Set up pointer-authentication

  std::string Error;
  const Target * Target =
    TargetRegistry::lookupTarget(EffectiveTriple.str(), Error);
  if (!Target) {
    Ctx.Diags.diagnose(
      SourceLoc(), diag::no_llvm_target, EffectiveTriple.str(), Error
    );
    return nullptr;
  }

  // On Cygwin 64 bit, dlls are loaded above the max address for 32 bits.
  // This means that the default CodeModel causes generated code to
  // segfault when run.
  Optional<CodeModel::Model> cmodel = None;
  if (EffectiveTriple.isArch64Bit() && EffectiveTriple.isWindowsCygwinEnvironment())
    cmodel = CodeModel::Large;

  // Create a target machine.
  llvm::TargetMachine * TargetMachine = Target->createTargetMachine(
    EffectiveTriple.str(), CPU, targetFeatures, TargetOpts, Reloc::PIC_,
    cmodel, OptLevel
  );
  if (!TargetMachine) {
    Ctx.Diags.diagnose(
      SourceLoc(), diag::no_llvm_target, EffectiveTriple.str(),
      "no LLVM target machine"
    );
    return nullptr;
  }
  return std::unique_ptr<llvm::TargetMachine>(TargetMachine);
}

GeneratedModule w2n::performIRGeneration(
  ModuleDecl * M,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  ArrayRef<std::string> parallelOutputFilenames,
  llvm::GlobalVariable ** outModuleHash
) {
  llvm_unreachable("not implemented.");
}

GeneratedModule w2n::performIRGeneration(
  FileUnit * file,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  llvm::GlobalVariable ** outModuleHash
) {
  llvm_unreachable("not implemented.");
}