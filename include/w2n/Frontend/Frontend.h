#ifndef W2N_FRONTEND_FRONTEND_H
#define W2N_FRONTEND_FRONTEND_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <list>
#include <memory>
#include <utility>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Basic/Statistic.h>
#include <w2n/Frontend/FrontendOptions.h>
#include <w2n/Frontend/Input.h>
#include <w2n/TBDGen/TBDGen.h>

namespace w2n {

class SearchPathOptions_t {};

/**
 * @brief A suite of module buffers.
 *
 */
struct ModuleBuffers {

  std::unique_ptr<llvm::MemoryBuffer> ModuleBuffer;

  ModuleBuffers(std::unique_ptr<llvm::MemoryBuffer> ModuleBuffer)
    : ModuleBuffer(std::move(ModuleBuffer)) {
  }
};

class CompilerInvocation {

private:
  FrontendOptions FrontendOpts;
  LanguageOptions LanguageOpts;
  SearchPathOptions_t SearchPathOpts;
  IRGenOptions IRGenOpts;
  TBDGenOptions TBDGenOpts;

public:
  CompilerInvocation();

  /**
   * @brief Configs the compiler invocation by parsing given arguments.
   *
   * @param Args C-string format arguments. Usually come from command line
   *  invocation.
   * @param Diagstic Diagnostic engine used for this time of argument
   * parsing.
   * @param ConfigurationFileBuffers A vector of configuration file buffer
   *  specified  in \c Args .
   * @return Returns \c true if there are errors happend while parsing the
   * arguments, otherwise \c false .
   */
  bool parseArgs(
    llvm::ArrayRef<const char *> Args,
    DiagnosticEngine& Diagstic,
    SmallVectorImpl<std::unique_ptr<llvm::MemoryBuffer>> *
      ConfigurationFileBuffers
  );

  FrontendOptions& getFrontendOptions() {
    return FrontendOpts;
  }

  const FrontendOptions& getFrontendOptions() const {
    return FrontendOpts;
  }

  LanguageOptions& getLanguageOptions() {
    return LanguageOpts;
  }

  const LanguageOptions& getLanguageOptions() const {
    return LanguageOpts;
  }

  SearchPathOptions_t& getSearchPathOptions() {
    return SearchPathOpts;
  }

  const SearchPathOptions_t& getSearchPathOptions() const {
    return SearchPathOpts;
  }

  IRGenOptions& getIRGenOptions() {
    return IRGenOpts;
  }

  const IRGenOptions& getIRGenOptions() const {
    return IRGenOpts;
  }

  TBDGenOptions& getTBDGenOptions() {
    return TBDGenOpts;
  }

  const TBDGenOptions& getTBDGenOptions() const {
    return TBDGenOpts;
  }

  const std::string& getModuleName() const {
    return getFrontendOptions().ModuleName;
  }

  const std::string& getModuleABIName() const {
    return getFrontendOptions().ModuleABIName;
  }

  const std::string& getModuleLinkName() const {
    return getFrontendOptions().ModuleLinkName;
  }

  const PrimarySpecificPaths&
  getPrimarySpecificPathsForPrimary(StringRef filename) const;
  const PrimarySpecificPaths&
  getPrimarySpecificPathsForSourceFile(const SourceFile& SF) const;
};

class CompilerInstance {

private:
  CompilerInvocation Invocation;

  SourceManager SourceMgr;

  DiagnosticEngine Diagnostics{SourceMgr};

  std::unique_ptr<ASTContext> Context;

  /// If there is no stats output directory by the time the
  /// instance has completed its setup, this will be null.
  std::unique_ptr<UnifiedStatsReporter> Stats;

  mutable ModuleDecl * MainModule = nullptr;

  /// Contains buffer IDs for input source code files.
  std::vector<unsigned> InputSourceCodeBufferIDs;

  /// Identifies the set of input buffers in the SourceManager that are
  /// considered primaries.
  llvm::SetVector<unsigned> PrimaryBufferIDs;

  /// Return whether there is an entry in PrimaryInputs for buffer \p
  /// BufID.
  bool isPrimaryInput(unsigned BufID) const {
    return PrimaryBufferIDs.count(BufID) != 0;
  }

  /// Record in PrimaryBufferIDs the fact that \p BufID is a primary.
  /// If \p BufID is already in the set, do nothing.
  void recordPrimaryInputBuffer(unsigned BufID);

public:
  CompilerInstance();

  CompilerInstance(const CompilerInstance&) = delete;
  void operator=(const CompilerInstance&) = delete;
  CompilerInstance(CompilerInstance&&) = delete;
  void operator=(CompilerInstance&&) = delete;

  bool setup(const CompilerInvocation& Invocation, std::string& Error);

  const CompilerInvocation& getInvocation() const {
    return Invocation;
  }

  SourceManager& getSourceMgr() {
    return SourceMgr;
  }

  const SourceManager& getSourceMgr() const {
    return SourceMgr;
  }

  DiagnosticEngine& getDiags() {
    return Diagnostics;
  }

  const DiagnosticEngine& getDiags() const {
    return Diagnostics;
  }

  llvm::vfs::FileSystem& getFileSystem() const {
    return *SourceMgr.getFileSystem();
  }

  ASTContext& getASTContext() {
    return *Context;
  }

  const ASTContext& getASTContext() const {
    return *Context;
  }

  bool hasASTContext() const {
    return Context != nullptr;
  }

  UnifiedStatsReporter * getStatsReporter() const {
    return Stats.get();
  }

  void freeASTContext();

  const PrimarySpecificPaths&
  getPrimarySpecificPathsForPrimary(StringRef filename) const;
  const PrimarySpecificPaths&
  getPrimarySpecificPathsForSourceFile(const SourceFile& SF) const;

private:
  /// Set up the file system by loading and validating all VFS overlay
  /// YAML files. If the process of validating VFS files failed, or the
  /// overlay file system could not be initialized, this function returns
  /// true. Else it returns false if setup succeeded.
  bool setUpVirtualFileSystemOverlays();
  bool setUpInputs();
  bool setUpASTContextIfNeeded();

  /**
   * @brief Find a buffer for a given input file and ensure it is recorded
   * in \c SourceMgr or \c InputSourceCodeBufferIDs as appropriate.
   *
   * @param I The given \c Input .
   * @param ShouldRecover Set \c true to recover from no-buffer with a
   * dummy empty file.
   * @param Failed Set \c true on failure.
   * @return Optional<unsigned> The buffer ID if it is not already
   * compiled, or \c None if so.
   *
   * @note Consider error recover when introduce .wat files.
   */
  Optional<unsigned>
  getRecordedBufferID(const Input& I, bool ShouldRecover, bool& Failed);

  /**
   * @brief Returns the input file's buffer suite.
   *
   * @param I The given input file.
   * @return Optional<ModuleBuffers> A buffer to use for the given input
   * file's contents and buffers for corresponding module contents. On
   * failure, the first field of the returned struct comes to be a null
   * pointer.
   */
  Optional<ModuleBuffers> getInputBuffersIfPresent(const Input& I);

private:
  /**
   * @brief Creates a new wasm file for the main module.
   */
  SourceFile * createSourceFileForMainModule(
    SourceFileKind Kind,
    ModuleDecl * Module,
    Optional<unsigned> BufferID,
    bool IsMainBuffer = false
  ) const;

  bool createFilesForMainModule(
    ModuleDecl * Module,
    SmallVectorImpl<FileUnit *>& Files
  ) const;

public:
  /**
   * @brief Retrieve the main module containing the files being compiled.
   *
   * @return ModuleDecl*
   */
  ModuleDecl * getMainModule() const;

  /**
   * @brief Gets the set of SourceFiles which are the primary inputs for
   *  this CompilerInstance
   *
   * @return ArrayRef<SourceFile *>
   */
  ArrayRef<SourceFile *> getPrimarySourceFiles() const {
    return getMainModule()->getPrimarySourceFiles();
  }

  /// Parses and type-checks all input files.
  void performSemanticAnalysis();

  /// Parses and performs import resolution on all input files.
  ///
  /// This is similar to a parse-only invocation, but module imports will
  /// also be processed.
  bool performParseAndResolveImportsOnly();

  /// If \p fn returns true, exits early and returns true.
  bool forEachFileToTypeCheck(llvm::function_ref<bool(SourceFile&)> fn);

private:
  friend class WasmFile;
  friend class WatFile;

  WasmFile::ParsingOptions getWasmFileParsingOptions() const;

  WatFile::ParsingOptions getWatFileParsingOptions() const;

  void finishTypeChecking();
};

} // namespace w2n

#endif // W2N_FRONTEND_FRONTEND_H