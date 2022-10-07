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

private:

  /// The ID for the memory buffer containing this file's source.
  ///
  /// May be -1, to indicate no association with a buffer.
  int BufferID;

  /// Describes what kind of file this is, which can affect some type checking
  /// and other behavior.
  const SourceFileKind Kind;
  
public:
  static SourceFile * createSourceFile(
    SourceFileKind Kind,
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID);

  SourceFile(ModuleDecl& M, SourceFileKind K, Optional<unsigned> BufferID);

  /// The buffer ID for the file that was imported, or None if there
  /// is no associated buffer.
  Optional<unsigned> getBufferID() const {
    if (BufferID == -1)
      return None;
    return BufferID;
  }

  static bool classof(const FileUnit * file) {
    return file->getKind() == FileUnitKind::Source;
  }

  static bool classof(const DeclContext * DC) {
    return isa<FileUnit>(DC) && classof(cast<FileUnit>(DC));
  }
};

class WasmFile : public SourceFile {
public:
  /// Flags that direct how the source file is parsed.
  enum class ParsingFlags : uint8_t {

  };

  using ParsingOptions = OptionSet<ParsingFlags>;

private:
  WasmFile(ModuleDecl& Module, Optional<unsigned> BufferID, ParsingOptions Opts)
    : SourceFile(Module, SourceFileKind::Wasm, BufferID){};

public:
  /// Retrieve the parsing options specified in the LangOptions.
  static ParsingOptions getDefaultParsingOptions(const LanguageOptions& Opts);

  static WasmFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID);
};

class WatFile : public SourceFile {
public:
  /// Flags that direct how the source file is parsed.
  enum class ParsingFlags : uint8_t {

  };

  using ParsingOptions = OptionSet<ParsingFlags>;

private:
  WatFile(ModuleDecl& Module, Optional<unsigned> BufferID, ParsingOptions Opts)
    : SourceFile(Module, SourceFileKind::Wasm, BufferID){};

public:
  /// Retrieve the parsing options specified in the LangOptions.
  static ParsingOptions getDefaultParsingOptions(const LanguageOptions& Opts);

  static WatFile * create(
    const CompilerInstance& Instance,
    ModuleDecl& Module,
    Optional<unsigned> BufferID);
};

} // namespace w2n

#endif // W2N_AST_SOURCEFILE_H
