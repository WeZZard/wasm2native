#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Option/Option.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/Options/Options.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Host.h>

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

CompilerInvocation::CompilerInvocation() {};

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

#include <set>

bool parseFrontendOptions(
  FrontendOptions& Options,
  const ArgList& Args,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * Buffers) {

  std::set<StringRef> AllInputFiles;
  bool hadDuplicates = false;
  for (const Arg * A : Args.filtered(options::OPT_INPUT)) {
    hadDuplicates = !AllInputFiles.insert(A->getValue()).second || hadDuplicates;
  }

  if (hadDuplicates) {
    // TODO: Diagnose duplicates
  }

  FrontendInputsAndOutputs InputsAndOutputs;
  for (const StringRef EachInputFile : AllInputFiles) {
    Input EachInput(EachInputFile);
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
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers)
{
  if (Args.hasArgNoClaim(options::OPT_target)) {
    Options.Target = llvm::Triple(Args.getLastArgValue(options::OPT_target));
  } else {
    Options.Target = llvm::Triple(llvm::sys::getDefaultTargetTriple());
  }

  if (Args.hasArgNoClaim(options::OPT_sdk)) {
    Options.SDKName = Args.getLastArgValue(options::OPT_sdk);
  }

  if (Args.hasArgNoClaim(options::OPT_entry_point)) {
    Options.entryPointFunctionName = Args.getLastArgValue(options::OPT_entry_point);
  }

  return false;
}

bool parseSearchPathOptions(
  SearchPathOptions_t& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers)
{
  return false;
}

bool parseIRGenOptions(
  IRGenOptions& Options,
  const ArgList& Arguments,
  DiagnosticEngine& Diagnostic,
  SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> * buffers)
{
  return false;
}
