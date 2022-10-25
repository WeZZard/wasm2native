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
class SourceFile : public FileUnit {

  friend void performImportResolution(SourceFile& SF);

public:
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

  ASTStage Stage;

public:
  static SourceFile * createSourceFile(
    SourceFileKind Kind,
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    bool IsPrimary
  );

  SourceFile(
    ModuleDecl& M,
    SourceFileKind K,
    Optional<unsigned> BufferID,
    bool IsPrimary
  );

  /// Whether this source file is a primary file, meaning that we're
  /// generating code for it. Note this method returns \c false in WMO.
  bool isPrimary() const {
    return IsPrimary;
  }

  /// The buffer ID for the file that was imported, or None if there
  /// is no associated buffer.
  Optional<unsigned> getBufferID() const {
    if (BufferID == -1)
      return None;
    return BufferID;
  }

  virtual ArrayRef<Decl *> getTopLevelDecls() const = 0;

  static bool classof(const FileUnit * file) {
    return file->getKind() == FileUnitKind::Source;
  }

  static bool classof(const DeclContext * DC) {
    return isa<FileUnit>(DC) && classof(cast<FileUnit>(DC));
  }

  /// If this buffer corresponds to a file on disk, returns the path.
  /// Otherwise, return an empty string.
  StringRef getFilename() const;

  ASTStage getASTStage() const {
    return Stage;
  }
};

class WasmFile : public SourceFile {
public:
  /// Flags that direct how the source file is parsed.
  enum class ParsingFlags : uint8_t {
  };

  using ParsingOptions = OptionSet<ParsingFlags>;

private:
  WasmFile(
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  )
    : SourceFile(Module, SourceFileKind::Wasm, BufferID, IsPrimary){};

public:
  /// Retrieve the parsing options specified in the LanguageOptions.
  static ParsingOptions
  getDefaultParsingOptions(const LanguageOptions& Opts);

  static WasmFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    bool IsPrimary = false
  );

  ArrayRef<Decl *> getTopLevelDecls() const override;
};

class WatFile : public SourceFile {
public:
  /// Flags that direct how the source file is parsed.
  enum class ParsingFlags : uint8_t {
    SuppressWarnings
  };

  using ParsingOptions = OptionSet<ParsingFlags>;

private:
  ParsingOptions Opts;

  WatFile(
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    ParsingOptions Opts,
    bool IsPrimary
  )
    : SourceFile(Module, SourceFileKind::Wasm, BufferID, IsPrimary),
      Opts(Opts){};

public:
  /// Retrieve the parsing options specified in the LanguageOptions.
  static ParsingOptions
  getDefaultParsingOptions(const LanguageOptions& Opts);

  static WatFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID,
    bool IsPrimary = false
  );

  ParsingOptions getParsingOptions() const {
    return Opts;
  }

  ArrayRef<Decl *> getTopLevelDecls() const override {
    llvm_unreachable("not implemented.");
  }
};

} // namespace w2n

#endif // W2N_AST_SOURCEFILE_H
