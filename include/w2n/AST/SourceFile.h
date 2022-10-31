#ifndef W2N_AST_SOURCEFILE_H
#define W2N_AST_SOURCEFILE_H

#include <memory>
#include <w2n/AST/FileUnit.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/OptionSet.h>

namespace w2n {

class CompilerInstance;

enum class SourceFileKind {
  /**
   * @brief A .wasm file.
   */
  Wasm,
  /**
   * @brief A .wat file.
   */
  Wat,
};

/**
 * @brief Represents a .wasm file or .wat file.
 *
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

class SourceFile : public FileUnit {
#pragma clang diagnostic pop

  friend void performImportResolution(SourceFile& SF);

public:
  /// Flags that direct how the source file is parsed.
  enum class ParsingFlags : uint8_t {
    SuppressWarnings,
  };

  using ParsingOptions = OptionSet<ParsingFlags>;

  enum class ASTStage {
    Unresolved,
    ImportsResolved,
    TypeChecked,
  };

private:
  /// The ID for the memory buffer containing this file's source.
  ///
  /// May be -1, to indicate no association with a buffer.
  int BufferID;

  /// Describes what kind of file this is, which can affect some type
  /// checking and other behavior.
  const SourceFileKind Kind;

  bool IsPrimary;

  ParsingOptions ParsingOpts;

  ASTStage Stage;

  Optional<std::vector<Decl *>> Decls;

public:
  /// Retrieve the parsing options specified in the \c LanguageOptions for
  /// specific \c SourceFileKind .
  static ParsingOptions getDefaultParsingOptions(
    SourceFileKind Kind, const LanguageOptions& Opts
  );

  static SourceFile * createSourceFile(
    SourceFileKind Kind,
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  );

  SourceFile(
    ModuleDecl& Module,
    SourceFileKind Kind,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  ) :
    FileUnit(FileUnitKind::Source, Module),
    BufferID(BufferID ? *BufferID : -1),
    Kind(Kind),
    IsPrimary(IsPrimary),
    ParsingOpts(Opts),
    Stage(ASTStage::Unresolved) {
  }

  /// The buffer ID for the file that was imported, or None if there
  /// is no associated buffer.
  Optional<unsigned> getBufferID() const {
    if (BufferID == -1)
      return None;
    return BufferID;
  }

  ParsingOptions getParsingOptions() const {
    return ParsingOpts;
  }

  /// Whether this source file is a primary file, meaning that we're
  /// generating code for it. Note this method returns \c false in WMO.
  bool isPrimary() const {
    return IsPrimary;
  }

  virtual ArrayRef<Decl *> getTopLevelDecls() const = 0;

  /// Retrieves an immutable view of the top-level decls if they have
  /// already been parsed, or \c None if they haven't. Should only be used
  /// for dumping.
  Optional<ArrayRef<Decl *>> getCachedTopLevelDecls() const {
    if (!Decls)
      return None;
    return llvm::makeArrayRef(*Decls);
  }

  bool hasCachedTopLevelDecls() const {
    return Decls.has_value();
  }

  void setCachedTopLevelDecls(Optional<std::vector<Decl *>> Decls) {
    this->Decls = Decls;
  }

  /// If this buffer corresponds to a file on disk, returns the path.
  /// Otherwise, return an empty string.
  StringRef getFilename() const;

  ASTStage getASTStage() const {
    return Stage;
  }

  virtual void
  collectLinkLibraries(ModuleDecl::LinkLibraryCallback callback
  ) const override;

  static bool classof(const FileUnit * file) {
    return file->getKind() == FileUnitKind::Source;
  }

  static bool classof(const DeclContext * DC) {
    return isa<FileUnit>(DC) && classof(cast<FileUnit>(DC));
  }
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

class WasmFile : public SourceFile {
#pragma clang diagnostic pop

private:
  WasmFile(
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  );

public:
  /// Retrieve the parsing options specified in the LanguageOptions.
  static ParsingOptions
  getDefaultParsingOptions(const LanguageOptions& Opts);

  static WasmFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary = false
  );

  ArrayRef<Decl *> getTopLevelDecls() const override;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

class WatFile : public SourceFile {
#pragma clang diagnostic pop

private:
  WatFile(
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  );

public:
  /// Retrieve the parsing options specified in the LanguageOptions.
  static ParsingOptions
  getDefaultParsingOptions(const LanguageOptions& Opts);

  static WatFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary = false
  );

  ArrayRef<Decl *> getTopLevelDecls() const override {
    llvm_unreachable("not implemented.");
  }
};

} // namespace w2n

#endif // W2N_AST_SOURCEFILE_H
