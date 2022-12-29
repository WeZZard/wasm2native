#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/ParseRequests.h>
#include <w2n/AST/TBDGenRequests.h>
#include <w2n/AST/TypeCheckerRequests.h>
#include <w2n/Basic/Filesystem.h>
#include <w2n/Frontend/Frontend.h>
#include <w2n/Sema/Sema.h>

using namespace w2n;

const PrimarySpecificPaths&
CompilerInvocation::getPrimarySpecificPathsForPrimary(StringRef Filename
) const {
  return getFrontendOptions().getPrimarySpecificPathsForPrimary(Filename);
}

const PrimarySpecificPaths&
CompilerInvocation::getPrimarySpecificPathsForSourceFile(
  const SourceFile& SF
) const {
  return getPrimarySpecificPathsForPrimary(SF.getFilename());
}

CompilerInstance::CompilerInstance() {
}

bool CompilerInstance::setup(
  const CompilerInvocation& Invocation, std::string& Error
) {
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

void CompilerInstance::freeASTContext() {
  Context.reset();
  MainModule = nullptr;
  PrimaryBufferIDs.clear();
}

bool CompilerInstance::setUpVirtualFileSystemOverlays() {
  // FIXME: Set overlay filesystem to SourceMgr when introduce search
  // paths.
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

    if (!BufferID.has_value() || !EachInput.isPrimary()) {
      continue;
    }

    recordPrimaryInputBuffer(*BufferID);
  }

  return HasFailed;
}

bool CompilerInstance::setUpASTContextIfNeeded() {
  Context.reset(ASTContext::get(
    Invocation.getLanguageOptions(), SourceMgr, Diagnostics
  ));

  registerParseRequestFunctions(Context->Eval);
  registerTypeCheckerRequestFunctions(Context->Eval);
  registerTBDGenRequestFunctions(Context->Eval);
  registerIRGenRequestFunctions(Context->Eval);

  return false;
}

Optional<unsigned> CompilerInstance::getRecordedBufferID(
  const Input& I, bool ShouldRecover, bool& Failed
) {
  if (I.getBuffer() == nullptr) {
    if (Optional<unsigned> ExistingBufferId = SourceMgr.getIDForBufferIdentifier(I.getFileName())) {
      return ExistingBufferId;
    }
  }
  auto BuffersForInput = getInputBuffersIfPresent(I);

  // Recover by dummy buffer if requested.
  if (!BuffersForInput.has_value() && I.getType() == file_types::TY_Wasm && ShouldRecover) {
    BuffersForInput = ModuleBuffers(llvm::MemoryBuffer::getMemBuffer(
      "// missing file\n", I.getFileName()
    ));
  }

  if (!BuffersForInput.has_value()) {
    Failed = true;
    return None;
  }

  // Transfer ownership of the MemoryBuffer to the SourceMgr.
  unsigned BufferId =
    SourceMgr.addNewSourceBuffer(std::move(BuffersForInput->ModuleBuffer)
    );

  InputSourceCodeBufferIDs.push_back(BufferId);
  return BufferId;
}

Optional<ModuleBuffers>
CompilerInstance::getInputBuffersIfPresent(const Input& I) {
  if (auto * B = I.getBuffer()) {
    return ModuleBuffers(llvm::MemoryBuffer::getMemBufferCopy(
      B->getBuffer(), B->getBufferIdentifier()
    ));
  }
  // FIXME: Working with filenames is fragile, maybe use the real path
  // or have some kind of FileManager.
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError InputFileOrErr = w2n::vfs::getFileOrSTDIN(
    getFileSystem(),
    I.getFileName(),
    /*FileSize*/ -1,
    /*RequiresNullTerminator*/ true,
    /*IsVolatile*/ false,
    /*Bad File Descriptor Retry*/
    getInvocation().getFrontendOptions().BadFileDescriptorRetryCount
  );
  if (!InputFileOrErr) {
    // Diagnose error open input file.
    return None;
  }

  return ModuleBuffers(std::move(*InputFileOrErr));
}

ModuleDecl * CompilerInstance::getMainModule() const {
  if (MainModule == nullptr) {
    Identifier Id = Context->getIdentifier(Invocation.getModuleName());
    MainModule = ModuleDecl::createMainModule(*Context, Id);

    // Register the main module with the AST context.
    Context->addLoadedModule(MainModule);

    // Create and add the module's files.
    SmallVector<FileUnit *, 16> Files;
    if (!createFilesForMainModule(MainModule, Files)) {
      for (auto * EachFile : Files) {
        MainModule->addFile(*EachFile);
      }
    } else {
      // If we failed to load a partial module, mark the main module as
      // having "failed to load", as it will contain no files. Note that
      // we don't try to add any of the successfully loaded partial
      // modules. This ensures that we don't encounter cases where we try
      // to resolve a cross-reference into a partial module that failed to
      // load.
      MainModule->setFailedToLoad();
    }
  }
  return MainModule;
}

bool CompilerInstance::createFilesForMainModule(
  ModuleDecl * Module, SmallVectorImpl<FileUnit *>& Files
) const {
  // Try to pull out the main source file, if any. This ensures that it
  // is at the start of the list of files.
  Optional<unsigned> MainBufferID = None;

  // Finally add the library files.
  // FIXME: This is the only demand point for InputSourceCodeBufferIDs. We
  // should compute this list of source files lazily.
  for (auto BufferID : InputSourceCodeBufferIDs) {
    // Skip the main buffer, we've already handled it.
    if (BufferID == MainBufferID) {
      continue;
    }

    // FIXME: Probe file kind when .wat file support was added.
    auto * File = createSourceFileForMainModule(
      SourceFileKind::Wasm, Module, BufferID
    );
    Files.push_back(File);
  }
  return false;
}

SourceFile * CompilerInstance::createSourceFileForMainModule(
  SourceFileKind Kind,
  ModuleDecl * Module,
  Optional<unsigned> BufferID,
  bool IsMainBuffer
) const {
  auto IsPrimary = BufferID && isPrimaryInput(*BufferID);
  const auto& LangOpts = this->getInvocation().getLanguageOptions();
  auto ParsingOpts = SourceFile::getDefaultParsingOptions(Kind, LangOpts);
  auto * InputFile = SourceFile::createSourceFile(
    Kind, *this, *Module, BufferID, ParsingOpts, IsPrimary
  );

  // if (IsMainBuffer)
  // FIXME: InputFile->SyntaxParsingCache =
  // Invocation.getMainFileSyntaxParsingCache();

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
  FrontendStatsTracer Tracer(
    getStatsReporter(), "parse-and-resolve-imports"
  );

  auto * MainModule = getMainModule();

  // Resolve imports for all the source files.
  for (auto * File : MainModule->getFiles()) {
    if (auto * SF = dyn_cast<SourceFile>(File)) {
      performImportResolution(*SF);
    }
  }

  assert(
    llvm::all_of(
      MainModule->getFiles(),
      [](const FileUnit * File) -> bool {
        auto * SF = dyn_cast<SourceFile>(File);
        if (!SF)
          return true;
        return SF->getASTStage() >= SourceFile::ASTStage::ImportsResolved;
      }
    )
    && "some files have not yet had their imports resolved"
  );
  MainModule->setHasResolvedImports();

  return Context->hadError();
}

bool CompilerInstance::forEachFileToTypeCheck(
  llvm::function_ref<bool(SourceFile&)> Fn
) {
  for (auto * SF : getPrimarySourceFiles()) {
    if (Fn(*SF)) {
      return true;
    }
  }
  return false;
}

void CompilerInstance::recordPrimaryInputBuffer(unsigned BufID) {
  PrimaryBufferIDs.insert(BufID);
}

WasmFile::ParsingOptions
CompilerInstance::getWasmFileParsingOptions() const {
  return WasmFile::getDefaultParsingOptions(getASTContext().LangOpts);
}

WatFile::ParsingOptions
CompilerInstance::getWatFileParsingOptions() const {
  return WatFile::getDefaultParsingOptions(getASTContext().LangOpts);
}

void CompilerInstance::finishTypeChecking() {
  forEachFileToTypeCheck([](SourceFile& SF) { return false; });
}

const PrimarySpecificPaths&
CompilerInstance::getPrimarySpecificPathsForPrimary(StringRef Filename
) const {
  return Invocation.getPrimarySpecificPathsForPrimary(Filename);
}

const PrimarySpecificPaths&
CompilerInstance::getPrimarySpecificPathsForSourceFile(
  const SourceFile& SF
) const {
  return getPrimarySpecificPathsForPrimary(SF.getFilename());
}
