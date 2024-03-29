#include "IRGenModule.h"
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/ErrorHandling.h>
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
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/Defer.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/IRGen.h>

using namespace w2n;
using namespace w2n::irgen;
using namespace llvm;

template <typename... ArgTypes>
void diagnoseSync(
  DiagnosticEngine& Diags,
  llvm::sys::Mutex * DiagMutex,
  SourceLoc Loc,
  Diag<ArgTypes...> ID,
  typename w2n::detail::PassArgument<ArgTypes>::Type... Args
) {
  if (DiagMutex) {
    DiagMutex->lock();
  }

  Diags.diagnose(Loc, ID, std::move(Args)...);

  if (DiagMutex) {
    DiagMutex->unlock();
  }
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
        Diags,
        DiagMutex,
        SourceLoc(),
        diag::error_opening_output,
        OutputFilename,
        EC.message()
      );
      RawOS->clear_error();
      return true;
    }
    if (Opts.OutputKind == IRGenOutputKind::LLVMAssemblyBeforeOptimization) {
      Module->print(RawOS.value(), nullptr);
      return false;
    }
  } else {
    w2n_assert(
      Opts.OutputKind == IRGenOutputKind::Module, "no output specified"
    );
  }

  if (!RawOS) {
    return false;
  }

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
        Out,
        /*ShouldPreserveUseListOrder*/ false,
        EmitRegularLTOSummary
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

  if (Stats != nullptr) {
    if (DiagMutex != nullptr) {
      DiagMutex->lock();
    }
    Stats->getFrontendCounters().NumLLVMBytesOutput += Out.tell();
    if (DiagMutex != nullptr) {
      DiagMutex->unlock();
    }
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
  std::vector<std::string> TargetFeaturesArray;
  std::tie(TargetOpts, CPU, TargetFeaturesArray, EffectiveClangTriple) =
    getIRTargetOptions(Opts, Ctx);
  const llvm::Triple& EffectiveTriple =
    llvm::Triple(EffectiveClangTriple);
  std::string TargetFeatures;
  if (!TargetFeaturesArray.empty()) {
    llvm::SubtargetFeatures Features;
    for (const std::string& Feature : TargetFeaturesArray) {
      // FIXME: thumb-mode: if (!shouldRemoveTargetFeature(feature)) {
      Features.AddFeature(Feature);
      // }
    }
    TargetFeatures = Features.getString();
  }

  // TODO: Set up pointer-authentication

  std::string Error;
  const Target * Target =
    TargetRegistry::lookupTarget(EffectiveTriple.str(), Error);
  if (Target == nullptr) {
    Ctx.Diags.diagnose(
      SourceLoc(), diag::no_llvm_target, EffectiveTriple.str(), Error
    );
    return nullptr;
  }

  // On Cygwin 64 bit, dlls are loaded above the max address for 32 bits.
  // This means that the default CodeModel causes generated code to
  // segfault when run.
  Optional<CodeModel::Model> CModel = None;
  if (EffectiveTriple.isArch64Bit() && EffectiveTriple.isWindowsCygwinEnvironment()) {
    CModel = CodeModel::Large;
  }

  // Create a target machine.
  llvm::TargetMachine * TargetMachine = Target->createTargetMachine(
    EffectiveTriple.str(),
    CPU,
    TargetFeatures,
    TargetOpts,
    Reloc::PIC_,
    CModel,
    OptLevel
  );
  if (TargetMachine == nullptr) {
    Ctx.Diags.diagnose(
      SourceLoc(),
      diag::no_llvm_target,
      EffectiveTriple.str(),
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
  ArrayRef<std::string> ParallelOutputFilenames,
  llvm::GlobalVariable ** OutModuleHash
) {
  w2n_unimplemented();
}

GeneratedModule w2n::performIRGeneration(
  FileUnit * File,
  const IRGenOptions& Opts,
  const TBDGenOptions& TBDOpts,
  ModuleDecl * Mod,
  StringRef ModuleName,
  const PrimarySpecificPaths& PSPs,
  llvm::GlobalVariable ** OutModuleHash
) {
  auto Desc = IRGenDescriptor::forFile(
    File,
    Opts,
    TBDOpts,
    *File->getModule(),
    ModuleName,
    PSPs,
    /*symsToEmit*/ None,
    OutModuleHash
  );
  return llvm::cantFail(File->getASTContext().Eval(IRGenRequest{Desc}));
}

static void initLLVMModule(const IRGenModule& IGM, ModuleDecl& ModDecl) {
  auto * Module = IGM.getModule();
  assert(Module && "Expected llvm:Module for IR generation!");

  auto& TargetMachine = *IGM.TargetMachine;

  Module->setTargetTriple(TargetMachine.getTargetTriple().getTriple());

  if (IGM.Context.LangOpts.SDKVersion) {
    if (Module->getSDKVersion().empty()) {
      Module->setSDKVersion(*IGM.Context.LangOpts.SDKVersion);
    } else {
      assert(Module->getSDKVersion() == *IGM.Context.LangOpts.SDKVersion);
    }
  }

  // Set the module's string representation.
  Module->setDataLayout(TargetMachine.createDataLayout());

  __unused auto * MDNode =
    IGM.getModule()->getOrInsertNamedMetadata("wasm2native.module.flags");
  // FIXME: Swift inserts a flag here to show if it is stdlib
}

/// Run the IRGen preparation AST pipeline. Passes have access to the
/// \c IRGenModule.
static void
runIRGenPreparePasses(ModuleDecl& Module, irgen::IRGenModule& IRModule) {
  w2n_proto_implemented();
}

static void setModuleFlags(IRGenModule& IGM) {
  auto * Module = IGM.getModule();

  // These module flags don't affect code generation; they just let us
  // error during LTO if the user tries to combine files across ABIs.
  Module->addModuleFlag(
    llvm::Module::Error, "WebAssembly Version", IRGenModule::WasmVersion
  );

  // FIXME: Virtual Function Elimation Flag
}

std::unique_ptr<llvm::TargetMachine> IRGenerator::createTargetMachine() {
  return ::createTargetMachine(Opts, Module.getASTContext());
}

// With -embed-bitcode, save a copy of the llvm IR as data in the
// __LLVM,__bitcode section and save the command-line options in the
// __LLVM,__swift_cmdline section.
static void embedBitcode(llvm::Module * M, const IRGenOptions& Opts) {
  if (Opts.EmbedMode == IRGenEmbedMode::None) {
    return;
  }

  w2n_proto_implemented();
}

/// Generates LLVM IR, runs the LLVM passes and produces the output file.
/// All this is done in a single thread.
GeneratedModule
IRGenRequest::evaluate(Evaluator& Eval, IRGenDescriptor Desc) const {
  const auto& Opts = Desc.Opts;
  const auto& PSPs = Desc.PSPs;
  auto * M = Desc.getParentModule();
  auto& Ctx = M->getASTContext();
  assert(!Ctx.hadError());

  // FIXME: getSymbolSourcesToEmit()

  auto& WasmModule = Desc.Mod;

  auto FilesToEmit = Desc.getFilesToEmit();
  auto * PrimaryFile =
    dyn_cast_or_null<SourceFile>(Desc.Ctx.dyn_cast<FileUnit *>());

  IRGenerator IRGen(Opts, WasmModule);

  auto TargetMachine = IRGen.createTargetMachine();
  if (!TargetMachine) {
    return GeneratedModule::null();
  }

  // Create the IR emitter.
  IRGenModule IGM(
    IRGen,
    std::move(TargetMachine),
    PrimaryFile,
    Desc.ModuleName,
    PSPs.OutputFilename,
    PSPs.MainInputFilenameForDebugInfo
  );

  initLLVMModule(IGM, WasmModule);

  // Run SIL level IRGen preparation passes.
  runIRGenPreparePasses(WasmModule, IGM);

  {
    FrontendStatsTracer Tracer(Ctx.Stats, "IRGen");

    // Emit the module contents.
    IRGen.emitGlobalTopLevel(Desc.getLinkerDirectives());

    for (auto * File : FilesToEmit) {
      if (auto * NextSf = dyn_cast<SourceFile>(File)) {
        IGM.emitSourceFile(*NextSf);
        // FIXME: file->getSynthesizedFile() : IGM.emitSynthesizedFileUnit
      } else {
        File->collectLinkLibraries([&IGM](LinkLibrary LinkLib) {
          IGM.addLinkLibrary(LinkLib);
        });
      }
    }

    // Okay, emit any definitions that we suddenly need.
    IRGen.emitLazyDefinitions();

    // TODO: emiting IR using IGM or irgen

    // Emit coverage mapping info. This needs to happen after we've
    // emitted any lazy definitions, as we need to know whether or not we
    // emitted a profiler increment for a given coverage map.
    IGM.emitCoverageMapping();

    // TODO: Emit symbols for eliminated dead methods.

    // TODO: Verify type layout if we were asked to.

    std::for_each(
      Opts.LinkLibraries.begin(),
      Opts.LinkLibraries.end(),
      [&](LinkLibrary LinkLib) { IGM.addLinkLibrary(LinkLib); }
    );

    if (!IGM.finalize()) {
      return GeneratedModule::null();
    }

    setModuleFlags(IGM);
  }

  // Bail out if there are any errors.
  if (Ctx.hadError()) {
    return GeneratedModule::null();
  }

  embedBitcode(IGM.getModule(), Opts);

  // TODO: Turn the module hash into an actual output.
  if (auto ** OutModuleHash = Desc.OutModuleHash) {
    *OutModuleHash = IGM.ModuleHash;
  }
  return std::move(IGM).intoGeneratedModule();
}

GeneratedModule OptimizedIRRequest::evaluate(
  Evaluator& Eval, IRGenDescriptor Desc
) const {
  w2n_unimplemented();
}

StringRef SymbolObjectCodeRequest::evaluate(
  Evaluator& Eval, IRGenDescriptor Desc
) const {
  auto& Ctx = Desc.getParentModule()->getASTContext();
  auto Mod = cantFail(Eval(OptimizedIRRequest{Desc}));
  auto * TargetMachine = Mod.getTargetMachine();

  // Add the passes to emit the LLVM module as object code.
  // TODO: Use compileAndWriteLLVM.
  legacy::PassManager EmitPasses;
  EmitPasses.add(createTargetTransformInfoWrapperPass(
    TargetMachine->getTargetIRAnalysis()
  ));

  SmallString<0> Output;
  raw_svector_ostream OS(Output);
  TargetMachine->addPassesToEmitFile(
    EmitPasses, OS, nullptr, CGFT_ObjectFile
  );
  EmitPasses.run(*Mod.getModule());
  OS << '\0';
  return Ctx.allocateCopy(Output.str());
}
