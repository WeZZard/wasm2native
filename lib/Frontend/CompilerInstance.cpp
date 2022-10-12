#include <w2n/AST/Identifier.h>
#include <w2n/Basic/Filesystem.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/Sema/Sema.h>

using namespace w2n;

CompilerInstance::CompilerInstance() {}

bool CompilerInstance::setup(
  const CompilerInvocation& Invocation,
  std::string& Error) {
  this->Invocation = Invocation;

  // If initializing the overlay file system fails there's no sense in
  // continuing because the compiler will read the wrong files.
  if (setUpVirtualFileSystemOverlays()) {
    Error = "Setting up virtual file system overlays failed";
    return true;
  }

  // FIXME: assert Invocation.getModuleName() is legal Identifier.

  if (setUpInputs()) {
    Error = "Setting up inputs failed";
    return true;
  }

  if (setUpASTContextIfNeeded()) {
    Error = "Setting up ASTContext failed";
    return true;
  }

  return false;
}

bool CompilerInstance::setUpVirtualFileSystemOverlays() {
  // FIXME: Set overlay filesystem to SourceMgr when introduce search paths.
  return false;
}

bool CompilerInstance::setUpInputs() {
  const auto& IO = Invocation.getFrontendOptions().InputsAndOutputs;
  const auto& Inputs = IO.getAllInputs();
  // FIXME: Currently always no recover
  const bool ShouldRecover = IO.shouldRecoverMissingInputs();

  bool HasFailed = false;
  for (const Input& EachInput : Inputs) {
    bool HasEachFailed = false;
    Optional<unsigned> BufferID =
      getRecordedBufferID(EachInput, ShouldRecover, HasEachFailed);
    HasFailed |= HasEachFailed;

    if (!BufferID.has_value() || !EachInput.isPrimary())
      continue;

    recordPrimaryInputBuffer(*BufferID);
  }

  if (HasFailed)
    return true;

  return false;
}

bool CompilerInstance::setUpASTContextIfNeeded() {
  Context.reset(
    ASTContext::get(Invocation.getLanguageOptions(), SourceMgr, Diagnostics));
  return false;
}

Optional<unsigned> CompilerInstance::getRecordedBufferID(
  const Input& I,
  bool ShouldRecover,
  bool& Failed) {
  if (!I.getBuffer()) {
    if (
      Optional<unsigned> existingBufferID =
        SourceMgr.getIDForBufferIdentifier(I.getFileName())) {
      return existingBufferID;
    }
  }
  auto BuffersForInput = getInputBuffersIfPresent(I);

  // Recover by dummy buffer if requested.
  if (
    !BuffersForInput.has_value() && I.getType() == file_types::TY_Wasm &&
    ShouldRecover) {
    BuffersForInput = ModuleBuffers(
      llvm::MemoryBuffer::getMemBuffer("// missing file\n", I.getFileName()));
  }

  if (!BuffersForInput.has_value()) {
    Failed = true;
    return None;
  }

  // Transfer ownership of the MemoryBuffer to the SourceMgr.
  unsigned bufferID =
    SourceMgr.addNewSourceBuffer(std::move(BuffersForInput->ModuleBuffer));

  InputSourceCodeBufferIDs.push_back(bufferID);
  return bufferID;
}

Optional<ModuleBuffers>
CompilerInstance::getInputBuffersIfPresent(const Input& I) {
  if (auto b = I.getBuffer()) {
    return ModuleBuffers(llvm::MemoryBuffer::getMemBufferCopy(
      b->getBuffer(), b->getBufferIdentifier()));
  }
  // FIXME: Working with filenames is fragile, maybe use the real path
  // or have some kind of FileManager.
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError inputFileOrErr = w2n::vfs::getFileOrSTDIN(
    getFileSystem(), I.getFileName(),
    /*FileSize*/ -1,
    /*RequiresNullTerminator*/ true,
    /*IsVolatile*/ false,
    /*Bad File Descriptor Retry*/
    getInvocation().getFrontendOptions().BadFileDescriptorRetryCount);
  if (!inputFileOrErr) {
    // Diagnose error open input file.
    return None;
  }

  return ModuleBuffers(std::move(*inputFileOrErr));
}

ModuleDecl * CompilerInstance::getMainModule() const {
  if (!MainModule) {
    Identifier Id = Context->getIdentifier(Invocation.getModuleName());
    MainModule = ModuleDecl::createMainModule(*Context, Id);

    // Register the main module with the AST context.
    Context->addLoadedModule(MainModule);

    // Create and add the module's files.
    SmallVector<FileUnit *, 16> Files;
    if (!createFilesForMainModule(MainModule, Files)) {
      for (auto * EachFile : Files)
        MainModule->addFile(*EachFile);
    } else {
      // If we failed to load a partial module, mark the main module as having
      // "failed to load", as it will contain no files. Note that we don't try
      // to add any of the successfully loaded partial modules. This ensures
      // that we don't encounter cases where we try to resolve a cross-reference
      // into a partial module that failed to load.
      MainModule->setFailedToLoad();
    }
  }
  return MainModule;
}

bool CompilerInstance::createFilesForMainModule(
  ModuleDecl * Module,
  SmallVectorImpl<FileUnit *>& Files) const {
  // Try to pull out the main source file, if any. This ensures that it
  // is at the start of the list of files.
  Optional<unsigned> MainBufferID = None;
  
  // Finally add the library files.
  // FIXME: This is the only demand point for InputSourceCodeBufferIDs. We
  // should compute this list of source files lazily.
  for (auto BufferID : InputSourceCodeBufferIDs) {
    // Skip the main buffer, we've already handled it.
    if (BufferID == MainBufferID)
      continue;

    // FIXME: Probe file kind when .wat file support was added.
    auto * File =
      createSourceFileForMainModule(SourceFileKind::Wasm, Module, BufferID);
    Files.push_back(File);
  }
  return false;
}

SourceFile * CompilerInstance::createSourceFileForMainModule(
  SourceFileKind Kind,
  ModuleDecl * Module,
  Optional<unsigned> BufferID,
  bool IsMainBuffer) const {
  auto IsPrimary = BufferID && isPrimaryInput(*BufferID);
  auto * InputFile =
    SourceFile::createSourceFile(Kind, *this, *Module, BufferID, IsPrimary);

  // if (IsMainBuffer)
  // FIXME: InputFile->SyntaxParsingCache = Invocation.getMainFileSyntaxParsingCache();

  return InputFile;
}

void CompilerInstance::performSemanticAnalysis() {
  performParseAndResolveImportsOnly();

  forEachFileToTypeCheck([&](SourceFile& SF) {
    performTypeChecking(SF);
    return false;
  });

  finishTypeChecking();
}

bool CompilerInstance::performParseAndResolveImportsOnly() {
  return false;
}

bool CompilerInstance::forEachFileToTypeCheck(
  llvm::function_ref<bool(SourceFile&)> fn) {
  for (auto * SF : getPrimarySourceFiles()) {
    if (fn(*SF))
      return true;
  }
  return false;
}

void CompilerInstance::recordPrimaryInputBuffer(unsigned BufID) {
  PrimaryBufferIDs.insert(BufID);
}

WasmFile::ParsingOptions CompilerInstance::getWasmFileParsingOptions() const {
  return WasmFile::getDefaultParsingOptions(getASTContext().LangOpts);
}

WatFile::ParsingOptions CompilerInstance::getWatFileParsingOptions() const {
  return WatFile::getDefaultParsingOptions(getASTContext().LangOpts);
}

void CompilerInstance::finishTypeChecking() {
  forEachFileToTypeCheck([](SourceFile& SF) { return false; });
}
