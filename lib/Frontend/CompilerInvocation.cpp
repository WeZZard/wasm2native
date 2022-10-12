#include <llvm/ADT/SmallString.h>
#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/Option.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Path.h>
#include <set>
#include <w2n/Basic/LLVM.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/Options/Options.h>

using namespace w2n;
using namespace llvm::opt;

static bool parseFrontendOptions(
  FrontendOptions& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers);

static bool parseLanguageOptions(
  LanguageOptions& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers);

static bool parseSearchPathOptions(
  SearchPathOptions_t& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers);

static bool parseIRGenOptions(
  IRGenOptions& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers);

CompilerInvocation::CompilerInvocation(){};

bool CompilerInvocation::parseArgs(
  llvm::ArrayRef<const char *> Args,
  DiagnosticEngine& Diagstic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> *
    ConfigurationFileBuffers) {
  using namespace options;

  if (Args.empty())
    return false;

  // Parse frontend command line options using W2N's option table.
  unsigned MissingIndex;
  unsigned MissingCount;
  std::unique_ptr<llvm::opt::OptTable> Table = createW2NOptTable();
  llvm::opt::InputArgList ParsedArgs =
    Table->ParseArgs(Args, MissingIndex, MissingCount, FrontendOption);
  if (MissingCount) {
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
        FrontendOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers)) {
    return true;
  }

  if (parseLanguageOptions(
        LanguageOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers)) {
    return true;
  }

  if (parseSearchPathOptions(
        SearchPathOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers)) {
    return true;
  }

  if (parseIRGenOptions(
        IRGenOpts, ParsedArgs, Diagstic, ConfigurationFileBuffers)) {
    return true;
  }

  return false;
}

bool parseFrontendOptions(
  FrontendOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers) {

  std::set<StringRef> AllInputFiles;
  bool hadDuplicates = false;
  for (const Arg * A : Args.filtered(options::OPT_INPUT)) {
    hadDuplicates =
      !AllInputFiles.insert(A->getValue()).second || hadDuplicates;
  }

  if (hadDuplicates) {
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
    InputSpecificPaths ISPs;
    EachInput.deriveInputSpecificPaths(ISPs, Diagnostic);
    EachInput.setInputSpecificPaths(std::move(ISPs));
    // Resolve input specific paths
    InputsAndOutputs.addInput(EachInput);
  }
  Options.InputsAndOutputs = InputsAndOutputs;

  // Derive ModuleName
  if (InputsAndOutputs.hasSingleInput()) {
    auto& FirstISPs = InputsAndOutputs.firstInput().getInputSpecificPaths();
    auto& OutputFileName = FirstISPs.OutputFilename;
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
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers) {
  if (Args.hasArg(options::OPT_target)) {
    Options.Target = llvm::Triple(Args.getLastArgValue(options::OPT_target));
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
  SearchPathOptions_t& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers) {
  return false;
}

bool parseIRGenOptions(
  IRGenOptions& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers) {
  return false;
}
