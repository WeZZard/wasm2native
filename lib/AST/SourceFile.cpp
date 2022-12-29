#include <w2n/AST/ParseRequests.h>
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
  SourceFileKind Kind, const LanguageOptions& Opts
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

StringRef SourceFile::getFilename() const {
  if (BufferID == -1)
    return "";
  SourceManager& SM = getASTContext().SourceMgr;
  return SM.getIdentifierForBuffer(BufferID);
}

WasmFile::WasmFile(
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
) :
  SourceFile(Module, SourceFileKind::Wasm, BufferID, Opts, IsPrimary) {
  Module.getASTContext().addDestructorCleanup(*this);
};

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
  auto& Ctx = getASTContext();
  auto * MutableThis = const_cast<WasmFile *>(this);
  return evaluateOrDefault(
           Ctx.Eval, ParseWasmFileRequest{MutableThis}, {}
  )
    .TopLevelDecls;
}

WatFile::WatFile(
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  ParsingOptions Opts,
  bool IsPrimary
) :
  SourceFile(Module, SourceFileKind::Wasm, BufferID, Opts, IsPrimary) {
  Module.getASTContext().addDestructorCleanup(*this);
};

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