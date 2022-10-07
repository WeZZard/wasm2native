#include <w2n/AST/SourceFile.h>
#include <w2n/Frontend/Frontend.h>

using namespace w2n;

SourceFile * SourceFile::createSourceFile(
  SourceFileKind Kind,
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID) {
  switch (Kind) {
  case SourceFileKind::Wasm:
    return WasmFile::create(Instance, Module, BufferID);
  case SourceFileKind::Wat:
    return WatFile::create(Instance, Module, BufferID);
  }
}

SourceFile::SourceFile(
  ModuleDecl& Module,
  SourceFileKind Kind,
  Optional<unsigned> BufferID)
  : FileUnit(FileUnitKind::Source, Module), BufferID(BufferID ? *BufferID : -1), Kind(Kind) {
  Module.getASTContext().addDestructorCleanup(*this);
}

WasmFile * WasmFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID) {
  ParsingOptions Opts = Instance.getWasmFileParsingOptions();
  return new (Instance.getASTContext()) WasmFile(Module, BufferID, Opts);
}

WasmFile::ParsingOptions
WasmFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WasmFile::ParsingOptions();
}

WatFile * WatFile::create(
  const CompilerInstance& Instance,
  ModuleDecl& Module,
  Optional<unsigned> BufferID) {
  ParsingOptions Opts = Instance.getWatFileParsingOptions();
  return new (Instance.getASTContext()) WatFile(Module, BufferID, Opts);
}

WatFile::ParsingOptions
WatFile::getDefaultParsingOptions(const LanguageOptions& Opts) {
  return WatFile::ParsingOptions();
}