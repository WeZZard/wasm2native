#ifndef W2N_FRONTEND_FRONTEND_H
#define W2N_FRONTEND_FRONTEND_H

#include <list>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VersionTuple.h>
#include <memory>
#include <utility>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Frontend/FrontendOptions.h>
#include <w2n/Frontend/Input.h>

namespace w2n {

class LanguageOptions final {
public:
  /// The target we are building for.
  ///
  /// This represents the minimum deployment target.
  llvm::Triple Target;

  /// The SDK version, if known.
  llvm::Optional<llvm::VersionTuple> SDKVersion;

  /// The SDK canonical name, if known.
  std::string SDKName;

  /// The alternate name to use for the entry point instead of main.
  std::string entryPointFunctionName = "main";
};

class SearchPathOptions_t {};

class IRGenOptions {};

class CompilerInvocation {

private:
  FrontendOptions FrontendOpts;
  LanguageOptions LanguageOpts;
  SearchPathOptions_t SearchPathOpts;
  IRGenOptions IRGenOpts;

public:
  CompilerInvocation();

  /**
   * @brief Configs the compiler invocation by parsing given arguments.
   *
   * @param Args C-string format arguments. Usually come from command line
   *  invocation.
   * @param Diagstic Diagnostic engine used for this time of argument parsing.
   * @param ConfigurationFileBuffers A vector of configuration file buffer
   *  specified  in \c Args .
   * @return Returns \c true if there are errors happend while parsing the
   * arguments, otherwise \c false .
   */
  bool parseArgs(
    llvm::ArrayRef<const char *> Args,
    DiagnosticEngine& Diagstic,
    SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> *
      ConfigurationFileBuffers);

  FrontendOptions& getFrontendOptions() { return FrontendOpts; }
  const FrontendOptions& getFrontendOptions() const { return FrontendOpts; }
  LanguageOptions& getLanguageOptions() { return LanguageOpts; }
  const LanguageOptions& getLanguageOptions() const { return LanguageOpts; }
  SearchPathOptions_t& getSearchPathOptions() { return SearchPathOpts; }
  const SearchPathOptions_t& getSearchPathOptions() const {
    return SearchPathOpts;
  }
  IRGenOptions& getIRGenOptions() { return IRGenOpts; }
  const IRGenOptions& getIRGenOptions() const { return IRGenOpts; }
};

class CompilerInstance {

private:
  CompilerInvocation Invocation;
  SourceManager SourceMgr;
  DiagnosticEngine Diagnostics{SourceMgr};
  std::unique_ptr<ASTContext> Context;
  mutable ModuleDecl * MainModule = nullptr;

public:
  CompilerInstance();

  CompilerInstance(const CompilerInstance&) = delete;
  void operator=(const CompilerInstance&) = delete;
  CompilerInstance(CompilerInstance&&) = delete;
  void operator=(CompilerInstance&&) = delete;

  bool setup(const CompilerInvocation& Invocation, std::string& Error);

  const CompilerInvocation& getInvocation() const { return Invocation; }

  SourceManager& getSourceMgr() { return SourceMgr; }
  const SourceManager& getSourceMgr() const { return SourceMgr; }

  DiagnosticEngine& getDiags() { return Diagnostics; }
  const DiagnosticEngine& getDiags() const { return Diagnostics; }

  ASTContext& getASTContext() { return * Context; }
  const ASTContext& getASTContext() const { return * Context; }

  bool hasASTContext() const { return Context != nullptr; }

};

} // namespace w2n

#endif // W2N_FRONTEND_FRONTEND_H