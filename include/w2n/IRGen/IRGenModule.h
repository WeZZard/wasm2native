#ifndef W2N_IRGEN_IRGENMODULE_H
#define W2N_IRGEN_IRGENMODULE_H

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <cstdint>
#include <map>
#include <memory>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/FileSystem.h>

namespace w2n {

class FuncDecl;
class GeneratedModule;

namespace irgen {

class IRGenModule;

/// The principal singleton which manages all of IR generation.
///
/// The IRGenerator delegates the emission of different top-level entities
/// to different instances of IRGenModule, each of which creates a
/// different llvm::Module.
///
/// In single-threaded compilation, the IRGenerator creates only a single
/// IRGenModule. In multi-threaded compilation, it contains multiple
/// IRGenModules - one for each LLVM module (= one for each input/output
/// file).
class IRGenerator {
public:

  const IRGenOptions& Opts;

  ModuleDecl& Module;

private:

  static constexpr const uint8_t AssumedMaxQueueCount = 8;

  llvm::DenseMap<SourceFile *, IRGenModule *> GenModules;

  // Stores the IGM from which a function is referenced the first time.
  // It is used if a function has no source-file association.
  llvm::DenseMap<FuncDecl *, IRGenModule *> DefaultIGMForFunction;

  // The IGM of the first source file.
  IRGenModule * PrimaryIGM = nullptr;

  // The current IGM for which IR is generated.
  IRGenModule * CurrentIGM = nullptr;

  // TODO: data for IR emission.

  /// The order in which all the SIL function definitions should
  /// appear in the translation unit.
  llvm::DenseMap<FuncDecl *, unsigned> FunctionOrder;

  /// The queue of IRGenModules for multi-threaded compilation.
  SmallVector<IRGenModule *, AssumedMaxQueueCount> Queue;

  std::atomic<int> QueueIndex;

public:

  explicit IRGenerator(const IRGenOptions& Opts, ModuleDecl& Module);

  /// Attempt to create an llvm::TargetMachine for the current target.
  std::unique_ptr<llvm::TargetMachine> createTargetMachine();

  /// Add an IRGenModule for a source file.
  /// Should only be called from IRGenModule's constructor.
  void addGenModule(SourceFile * SF, IRGenModule * IGM);

  /// Get an IRGenModule for a source file.
  IRGenModule * getGenModule(SourceFile * SF) {
    IRGenModule * IGM = GenModules[SF];
    assert(IGM);
    return IGM;
  }

  SourceFile * getSourceFile(IRGenModule * Module) {
    for (auto Pair : GenModules) {
      if (Pair.second == Module) {
        return Pair.first;
      }
    }
    return nullptr;
  }

  /// Get an IRGenModule for a declaration context.
  /// Returns the IRGenModule of the containing source file, or if this
  /// cannot be determined, returns the primary IRGenModule.
  IRGenModule * getGenModule(DeclContext * DC);

  /// Get an IRGenModule for a function.
  /// Returns the IRGenModule of the containing source file, or if this
  /// cannot be determined, returns the IGM from which the function is
  /// referenced the first time.
  IRGenModule * getGenModule(FuncDecl * F);

  /// Returns the primary IRGenModule. This is the first added
  /// IRGenModule. It is used for everything which cannot be correlated to
  /// a specific source file. And of course, in single-threaded
  /// compilation there is only the primary IRGenModule.
  IRGenModule * getPrimaryIGM() const {
    assert(PrimaryIGM);
    return PrimaryIGM;
  }

  bool hasMultipleIGMs() const {
    return GenModules.size() >= 2;
  }

  llvm::DenseMap<SourceFile *, IRGenModule *>::iterator begin() {
    return GenModules.begin();
  }

  llvm::DenseMap<SourceFile *, IRGenModule *>::iterator end() {
    return GenModules.end();
  }

  // TODO: IR emission

  /// Emit functions, variables and tables which are needed anyway, e.g.
  /// because they are externally visible.
  void emitGlobalTopLevel(const std::vector<std::string>& LinkerDirectives
  );

  // Emit info that describes the entry point to the module, if it has
  // one.
  void emitEntryPointInfo();

  /// Emit coverage mapping info.
  void emitCoverageMapping();

  /// Emit everything which is reachable from already emitted IR.
  void emitLazyDefinitions();

  unsigned getFunctionOrder(FuncDecl * F) {
    auto It = FunctionOrder.find(F);
    assert(
      It != FunctionOrder.end()
      && "no order number for SIL function definition?"
    );
    return It->second;
  }

  /// In multi-threaded compilation fetch the next IRGenModule from the
  /// queue.
  IRGenModule * fetchFromQueue() {
    int Idx = QueueIndex++;
    if (Idx < (int)Queue.size()) {
      return Queue[Idx];
    }
    return nullptr;
  }
};

class IRGenModule {
public:

  static const unsigned WasmVersion = 0;

  std::unique_ptr<llvm::LLVMContext> LLVMContext;

  IRGenerator& IRGen;

  ASTContext& Context;

  std::unique_ptr<llvm::Module> Module;

  std::unique_ptr<llvm::TargetMachine> TargetMachine;

  ModuleDecl * getModuleDecl() const;

  const IRGenOptions& getOptions() const {
    return IRGen.Opts;
  }

  llvm::StringMap<ModuleDecl *> OriginalModules;

  llvm::SmallString<CommonPathLength> OutputFilename;

  llvm::SmallString<CommonPathLength> MainInputFilenameForDebugInfo;

  // FIXME: TargetInfo

  // FIXME: DebugInfo

  /// A global variable which stores the hash of the module. Used for
  /// incremental compilation.
  llvm::GlobalVariable * ModuleHash;

  std::map<std::string, llvm::AllocaInst *> NamedValues;

  IRGenModule(
    IRGenerator& IRGen,
    std::unique_ptr<llvm::TargetMachine>&& Target,
    SourceFile * SF,
    StringRef ModuleName,
    StringRef OutputFilename,
    StringRef MainInputFilenameForDebugInfo
  );

  ~IRGenModule();

  GeneratedModule intoGeneratedModule() &&;

  llvm::LLVMContext& getLLVMContext() const {
    return *LLVMContext;
  }

  void emitSourceFile(SourceFile& SF);
  // FIXME: void emitSynthesizedFileUnit(SynthesizedFileUnit& SFU);
  void addLinkLibrary(const LinkLibrary& LinkLib);
  void emitCoverageMapping();

  /// Attempt to finalize the module.
  ///
  /// This can fail, in which it will return false and the module will be
  /// invalid.
  bool finalize();

  //--- Runtime ----------------------------------------------------------

  llvm::Module * getModule() const;
};

} // namespace irgen

} // namespace w2n

#endif // W2N_IRGEN_IRGENMODULE_H