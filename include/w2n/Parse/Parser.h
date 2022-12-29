#ifndef W2N_PARSE_PARSER_H
#define W2N_PARSE_PARSER_H

#include <llvm/ADT/SmallVector.h>
#include <memory>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>

namespace w2n {
class Decl;
class MagicDecl;
class VersionDecl;
class SectionDecl;
class SourceFile;
class DiagnosticEngine;

class Parser {
public:

  Parser(){};

  virtual ~Parser();

  virtual void parseTopLevel(llvm::SmallVectorImpl<Decl *>& Decls) = 0;
};

/// The parser parses .wasm file.
class WasmParser : public Parser {
private:

  unsigned BufferID;
  SourceFile& File;
  SourceManager& SourceMgr;
  DiagnosticEngine * LexerDiags;

  struct Implementation;

  Implementation& getImpl() const;

  WasmParser(
    unsigned BufferID, SourceFile& SF, DiagnosticEngine * LexerDiags
  );

public:

  WasmParser(const WasmParser&) = delete;
  void operator=(const WasmParser&) = delete;

  static std::unique_ptr<WasmParser> createWasmParser(
    unsigned BufferID, SourceFile& SF, DiagnosticEngine * LexerDiags
  );

  ~WasmParser();

private:

  ModuleDecl * parseModuleDecl();

public:

  void parseTopLevel(llvm::SmallVectorImpl<Decl *>& Decls) override;
};

// TODO: WatParser

} // namespace w2n

#endif // W2N_PARSE_PARSER_H
