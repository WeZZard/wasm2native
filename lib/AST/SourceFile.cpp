#include <w2n/AST/SourceFile.h>
#include <w2n/Frontend/Frontend.h>

using namespace w2n;

void w2n::simple_display(llvm::raw_ostream& out, const FileUnit * file) {
  if (!file) {
    out << "(null)";
    return;
  }

  switch (file->getKind()) {
  case FileUnitKind::Source:
    out << '\"' << cast<SourceFile>(file)->getFilename() << '\"';
    return;
  case FileUnitKind::Builtin: out << "(Builtin)"; return;
  }
  llvm_unreachable("Unhandled case in switch");
}

SourceFile::ParsingOptions SourceFile::getDefaultParsingOptions(
  SourceFileKind Kind,
  const LanguageOptions& Opts
) {
  switch (Kind) {
  case SourceFileKind::Wasm:
    return WasmFile::getDefaultParsingOptions(Opts);
  case SourceFileKind::Wat:
    return WatFile::getDefaultParsingOptions(Opts);
  }
}

SourceFile * SourceFile::createSourceFile(
  SourceFileKind Kind,
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
) {
  switch (Kind) {
  case SourceFileKind::Wasm:
    return WasmFile::create(Instance, Module, BufferID, Opts, IsPrimary);
  case SourceFileKind::Wat:
    return WatFile::create(Instance, Module, BufferID, Opts, IsPrimary);
  }
}

SourceFile::SourceFile(
  ModuleDecl& Module,
  SourceFileKind Kind,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
)
  : FileUnit(FileUnitKind::Source, Module),
    BufferID(BufferID ? *BufferID : -1), Kind(Kind), IsPrimary(IsPrimary),
    ParsingOpts(Opts), Stage(ASTStage::Unresolved) {
  Module.getASTContext().addDestructorCleanup(*this);
}

StringRef SourceFile::getFilename() const {
  if (BufferID == -1)
    return "";
  SourceManager& SM = getASTContext().SourceMgr;
  return SM.getIdentifierForBuffer(BufferID);
}

WasmFile * WasmFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
) {
  return new (Instance.getASTContext())
    WasmFile(Module, BufferID, Opts, IsPrimary);
}

WasmFile::ParsingOptions
WasmFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WasmFile::ParsingOptions();
}

ArrayRef<Decl *> WasmFile::getTopLevelDecls() const {
  return ArrayRef<Decl *>(); // FIXME: not implemented
}

WatFile * WatFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
) {
  return new (Instance.getASTContext())
    WatFile(Module, BufferID, Opts, IsPrimary);
}

WatFile::ParsingOptions
WatFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WatFile::ParsingOptions();
}