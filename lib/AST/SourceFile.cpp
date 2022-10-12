#include <w2n/AST/SourceFile.h>
#include <w2n/Frontend/Frontend.h>

using namespace w2n;

SourceFile * SourceFile::createSourceFile(
  SourceFileKind Kind,
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  bool IsPrimary) {
  switch (Kind) {
  case SourceFileKind::Wasm:
    return WasmFile::create(Instance, Module, BufferID, IsPrimary);
  case SourceFileKind::Wat:
    return WatFile::create(Instance, Module, BufferID, IsPrimary);
  }
}

SourceFile::SourceFile(
  ModuleDecl& Module,
  SourceFileKind Kind,
  Optional<unsigned> BufferID,
  bool IsPrimary)
  : FileUnit(FileUnitKind::Source, Module), BufferID(BufferID ? *BufferID : -1),
    Kind(Kind), IsPrimary(IsPrimary) {
  Module.getASTContext().addDestructorCleanup(*this);
}

WasmFile * WasmFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  bool IsPrimary) {
  ParsingOptions Opts = Instance.getWasmFileParsingOptions();
  return new (Instance.getASTContext())
    WasmFile(Module, BufferID, Opts, IsPrimary);
}

WasmFile::ParsingOptions
WasmFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WasmFile::ParsingOptions();
}

WatFile * WatFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID,
  bool IsPrimary) {
  ParsingOptions Opts = Instance.getWatFileParsingOptions();
  return new (Instance.getASTContext())
    WatFile(Module, BufferID, Opts, IsPrimary);
}

WatFile::ParsingOptions
WatFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WatFile::ParsingOptions();
}