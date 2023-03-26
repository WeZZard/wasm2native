#include <llvm/ADT/SmallString.h>
#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/Option.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Path.h>
#include <set>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/PrimarySpecificPaths.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/Frontend/FrontendOptions.h>
#include <w2n/Options/Options.h>

using namespace w2n;
using namespace llvm::opt;

static bool parseFrontendOptions(
  FrontendOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
);

static bool parseLanguageOptions(
  LanguageOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
);

static bool parseSearchPathOptions(
  SearchPathOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
);

static bool parseIRGenOptions(
  IRGenOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
);

static bool derivePrimarySpecificPaths(
  Input& I,
  PrimarySpecificPaths& PSPs,
  FrontendOptions& Opts,
  DiagnosticEngine& Diag
);

CompilerInvocation::CompilerInvocation(){};

bool CompilerInvocation::parseArgs(
  llvm::ArrayRef<const char *> Args,
  DiagnosticEngine& Diagstic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> *
    ConfigurationFileBuffers
) {
  using namespace options;

  if (Args.empty()) {
    return false;
  }

  // Parse frontend command line options using W2N's option table.
  unsigned MissingIndex;
  unsigned MissingCount;
  std::unique_ptr<llvm::opt::OptTable> Table = createW2NOptTable();
  llvm::opt::InputArgList ParsedArgs =
    Table->ParseArgs(Args, MissingIndex, MissingCount, FrontendOption);
  if (MissingCount != 0) {
    // TODO: Diagnose arguments of missing options
    return true;
  }

  if (ParsedArgs.hasArg(OPT_UNKNOWN)) {
    for (__attribute__((__unused__)) const Arg * A :
         ParsedArgs.filtered(OPT_UNKNOWN)) {
      // TODO: Diagnose arguments of unkown options
    }
    return true;
  }

  if (parseFrontendOptions(
        FrontendOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers
      )) {
    return true;
  }

  if (parseLanguageOptions(
        LanguageOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers
      )) {
    return true;
  }

  if (parseSearchPathOptions(
        SearchPathOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers
      )) {
    return true;
  }

  if (parseIRGenOptions(
        IRGenOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers
      )) {
    return true;
  }

  return false;
}

bool parseFrontendOptions(
  FrontendOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
) {
  std::set<StringRef> AllInputFiles;
  bool HadDuplicates = false;
  for (const Arg * A : Args.filtered(options::OPT_INPUT)) {
    HadDuplicates =
      !AllInputFiles.insert(A->getValue()).second || HadDuplicates;
  }

  if (HadDuplicates) {
    // TODO: Diagnose duplicates
  }

  // Derive InputsAndOutputs
  FrontendInputsAndOutputs InputsAndOutputs;
  for (const StringRef EachInputFile : AllInputFiles) {
    // FIXME: Sets all as primary input before add support to .wat file.
    Input EachInput(EachInputFile, /*IsPrimary*/ true);
    if (!file_types::isInputType(EachInput.getType())) {
      // TODO: Diagnose invalid input files.
      continue;
    }
    PrimarySpecificPaths ISPs;
    derivePrimarySpecificPaths(EachInput, ISPs, Options, Diagnostic);
    EachInput.setPrimarySpecificPaths(std::move(ISPs));
    // Resolve input specific paths
    InputsAndOutputs.addInput(EachInput);
  }
  Options.InputsAndOutputs = InputsAndOutputs;

  // Derive ModuleName
  if (InputsAndOutputs.hasSingleInput()) {
    const auto& FirstISPs =
      InputsAndOutputs.firstInput().getPrimarySpecificPaths();
    const auto& OutputFileName = FirstISPs.OutputFilename;
    StringRef Stem = llvm::sys::path::stem(OutputFileName);
    Options.ModuleName = Stem.str();
  } else {
    if (Arg * OutputArg = Args.getLastArg(options::OPT_o)) {
      Options.ModuleName = std::string(OutputArg->getValue());
    } else {
      // TODO: Diagnose ambiguous output
      return true;
    }
  }

  // Derive ModuleABIName
  Options.ModuleABIName = Options.ModuleName;

  // Derive ModuleLinkName
  Options.ModuleLinkName = Options.ModuleName;

  // Derive RequestedAction
  if (Args.hasArg(options::OPT_emit_object)) {
    Options.RequestedAction = FrontendOptions::ActionType::EmitObject;
  } else if (Args.hasArg(options::OPT_emit_assembly)) {
    Options.RequestedAction = FrontendOptions::ActionType::EmitAssembly;
  } else if (Args.hasArg(options::OPT_emit_ir)) {
    Options.RequestedAction = FrontendOptions::ActionType::EmitIR;
  } else if (Args.hasArg(options::OPT_emit_irgen)) {
    Options.RequestedAction = FrontendOptions::ActionType::EmitIRGen;
  } else if (InputsAndOutputs.hasInputs()) {
    Options.RequestedAction = FrontendOptions::ActionType::EmitObject;
  }

  return false;
}

bool parseLanguageOptions(
  LanguageOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
) {
  if (Args.hasArg(options::OPT_target)) {
    Options.Target =
      llvm::Triple(Args.getLastArgValue(options::OPT_target));
  } else {
    Options.Target = llvm::Triple(llvm::sys::getDefaultTargetTriple());
  }

  if (Args.hasArg(options::OPT_sdk)) {
    Options.SDKName = Args.getLastArgValue(options::OPT_sdk);
  }

  if (Args.hasArg(options::OPT_entry_point)) {
    Options.EntryPointFunctionName =
      Args.getLastArgValue(options::OPT_entry_point);
  }

  if (Args.hasArg(options::OPT_use_malloc)) {
    Options.EntryPointFunctionName = true;
  }

  return false;
}

bool parseSearchPathOptions(
  SearchPathOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
) {
  return false;
}

bool parseIRGenOptions(
  IRGenOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers
) {
  w2n_proto_implemented([&]() -> void {
    if (Args.hasArg(options::OPT_emit_object)) {
      Options.OutputKind = IRGenOutputKind::ObjectFile;
    } else if (Args.hasArg(options::OPT_emit_assembly)) {
      Options.OutputKind = IRGenOutputKind::NativeAssembly;
    } else if (Args.hasArg(options::OPT_emit_ir)) {
      Options.OutputKind = IRGenOutputKind::LLVMAssemblyAfterOptimization;
    } else if (Args.hasArg(options::OPT_emit_irgen)) {
      Options.OutputKind =
        IRGenOutputKind::LLVMAssemblyBeforeOptimization;
    } else if (Args.hasArg(options::OPT_emit_bc)) {
      Options.OutputKind = IRGenOutputKind::LLVMBitcode;
    }
    Options.EnableStackProtection = Args.hasFlag(
      options::OPT_enable_stack_protector,
      options::OPT_disable_stack_protector,
      Options.EnableStackProtection
    );
  });
  return false;
}

bool derivePrimarySpecificPaths(
  Input& I,
  PrimarySpecificPaths& PSPs,
  FrontendOptions& Opts,
  DiagnosticEngine& Diag
) {
  switch (I.getType()) {
  case file_types::ID::TY_Wasm: {
    using namespace llvm::sys;
    StringRef FilenameRef(I.getFileName());
    StringRef Stem = path::stem(I.getFileName());
    StringRef ParentPath = path::parent_path(I.getFileName());

    SmallString<PathLength256> FilenameBodyBuf;
    path::append(
      FilenameBodyBuf, path::begin(ParentPath), path::end(ParentPath)
    );
    path::append(FilenameBodyBuf, path::begin(Stem), path::end(Stem));

    Twine FilenameBody(FilenameBodyBuf); // NOLINT(llvm-twine-local)

    // Supplementary output paths
    SupplementaryOutputPaths SOPs;
    SOPs.DependenciesFilePath = FilenameBody.concat(".d").str();
    SOPs.SerializedDiagnosticsPath =
      FilenameBody.concat(".serialized-diagnostics").str();
    SOPs.FixItsOutputPath = FilenameBody.concat("-fixit.json").str();
    SOPs.TBDPath = FilenameBody.concat(".tbd").str();

    std::string OutputFilename;
    if (Opts.RequestedAction != FrontendOptions::ActionType::EmitObject) {
      OutputFilename = "-";
    } else {
      OutputFilename = FilenameBody.concat(".o").str();
    }
    PSPs = PrimarySpecificPaths(OutputFilename, "", SOPs);
    return false;
  }
  case file_types::ID::TY_INVALID:
    // TODO: Diagnostic invalid.
    return true;
  default:
    // TODO: Diagnostic illegal inputs.
    return true;
  }
}