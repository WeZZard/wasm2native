#ifndef W2N_IRGEN_IRGENMODULE_H
#define W2N_IRGEN_IRGENMODULE_H

#include "IRGenerator.h"
#include "WasmTargetInfo.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/TargetMachine.h>
#include <cstdint>
#include <map>
#include <memory>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/FileSystem.h>
#include <w2n/IRGen/Linking.h>

namespace w2n::irgen {

struct VectorTyKey {
  llvm::Type * ElementTy;
  uint32_t Count;
};

} // namespace w2n::irgen

namespace llvm {

using namespace w2n::irgen;

template <>
struct DenseMapInfo<VectorTyKey> {
  static inline VectorTyKey getEmptyKey() {
    return {nullptr, 0};
  }

  static inline VectorTyKey getTombstoneKey() {
    return {nullptr, UINT32_MAX};
  }

  static unsigned getHashValue(const VectorTyKey& Key) {
    return hash_combine(Key.ElementTy, Key.Count);
  }

  static bool isEqual(const VectorTyKey& LHS, const VectorTyKey& RHS) {
    return LHS.ElementTy == RHS.ElementTy && LHS.Count == RHS.Count;
  }
};
} // namespace llvm

namespace w2n {

class FuncDecl;
class GeneratedModule;

namespace irgen {

class Address;
class IRGenerator;

/// IRGenModule - Primary class for emitting IR for global declarations.
///
class IRGenModule {
public:

  static const unsigned WasmVersion = 0;

  std::unique_ptr<llvm::LLVMContext> LLVMContext;

  const llvm::DataLayout DataLayout;

  const llvm::Triple Triple;

  IRGenerator& IRGen;

  ASTContext& Context;

  std::unique_ptr<llvm::Module> Module;

  std::unique_ptr<llvm::TargetMachine> TargetMachine;

  /**
   * @note: Since wasm2native currently uses code that ported from Swift,
   * the IR generation model is also ported from Swift. The module used
   * returned by this method for \c PrimaryIGM is not a \c SourceFile
   * related module but the main module.
   */
  ModuleDecl * getWasmModule() const {
    return &IRGen.Module;
  }

  const IRGenOptions& getOptions() const {
    return IRGen.Opts;
  }

  llvm::StringMap<ModuleDecl *> OriginalModules;

  llvm::SmallString<CommonPathLength> OutputFilename;

  llvm::SmallString<CommonPathLength> MainInputFilenameForDebugInfo;

  /// Order dependency -- TargetInfo must be initialized after Opts.
  const WasmTargetInfo TargetInfo;

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
  void addLinkLibrary(const LinkLibrary& LinkLib);

  void emitGlobalVariable(GlobalVariable * V);

  void emitCoverageMapping();

  void finishEmitAfterTopLevel();

  /// Attempt to finalize the module.
  ///
  /// This can fail, in which it will return false and the module will be
  /// invalid.
  bool finalize();

#pragma mark Reporting Errors

  void unimplemented(SourceLoc, StringRef Message);

  [[noreturn]] void fatal_unimplemented(SourceLoc, StringRef Message);

  void error(SourceLoc loc, const Twine& message);

#pragma mark Accessing Module Contents

  llvm::Module * getModule() const;

  Address getAddrOfGlobalVariable(
    GlobalVariable * Global, ForDefinition_t ForDefinition
  );

  llvm::Function *
  getAddrOfFunction(Function * F, ForDefinition_t ForDefinition);

#pragma mark Types

private:

  llvm::IntegerType * I8Ty;  /// i8
  llvm::IntegerType * I16Ty; /// i16
  llvm::IntegerType * I32Ty; /// i32
  llvm::IntegerType * I64Ty; /// i64
  llvm::IntegerType * U8Ty;  /// u8
  llvm::IntegerType * U16Ty; /// u16
  llvm::IntegerType * U32Ty; /// u32
  llvm::IntegerType * U64Ty; /// u64
  llvm::Type * F32Ty;        /// f32, float
  llvm::Type * F64Ty;        /// f64, double

  mutable llvm::DenseMap<VectorTyKey, llvm::StructType *> VectorTys;

public:

  llvm::Type * getType(ValueType * Ty) const {
    if (isa<I32Type>(Ty)) {
      return I32Ty;
    }
    if (isa<I64Type>(Ty)) {
      return I64Ty;
    }
    if (isa<F32Type>(Ty)) {
      return F32Ty;
    }
    if (isa<F64Type>(Ty)) {
      return F64Ty;
    }
    llvm_unreachable("unexpected value type.");
  }

  llvm::StructType *
  getVectorType(uint32_t Size, llvm::Type * ElementTy) const {
    VectorTyKey Key = VectorTyKey{ElementTy, Size};
    const auto I = VectorTys.find(Key);
    if (I != VectorTys.end()) {
      return I->second;
    }
    std::vector<llvm::Type *> Types;
    Types.reserve(Size);
    for (uint32_t I = 0; I < Size; I++) {
      Types.insert(Types.end(), ElementTy);
    }
    auto * Ty = llvm::StructType::create(Types);
    VectorTys.insert(std::make_pair(Key, Ty));
    return Ty;
  }

  llvm::Type * getStorageType(Type * T);

#pragma mark Globals

  void addUsedGlobal(llvm::GlobalValue * Global);
  void addCompilerUsedGlobal(llvm::GlobalValue * Global);

private:

  /// LLVMUsed - List of global values which are required to be
  /// present in the object file; bitcast to i8*. This is used for
  /// forcing visibility of symbols which may otherwise be optimized
  /// out.
  SmallVector<llvm::WeakTrackingVH, 4> LLVMUsed;

  /// LLVMCompilerUsed - List of global values which are required to be
  /// present in the object file; bitcast to i8*. This is used for
  /// forcing visibility of symbols which may otherwise be optimized
  /// out.
  ///
  /// Similar to LLVMUsed, but emitted as llvm.compiler.used.
  SmallVector<llvm::WeakTrackingVH, 4> LLVMCompilerUsed;
};

/// Stores a pointer to an IRGenModule.
/// As long as the CurrentIGMPtr is alive, the CurrentIGM in the
/// dispatcher is set to the containing IRGenModule.
class CurrentIGMPtr {
  IRGenModule * IGM;

public:

  CurrentIGMPtr(IRGenModule * IGM) : IGM(IGM) {
    assert(IGM);
    assert(!IGM->IRGen.CurrentIGM && "Another CurrentIGMPtr is alive");
    IGM->IRGen.CurrentIGM = IGM;
  }

  ~CurrentIGMPtr() {
    IGM->IRGen.CurrentIGM = nullptr;
  }

  IRGenModule * get() const {
    return IGM;
  }

  IRGenModule * operator->() const {
    return IGM;
  }
};

} // namespace irgen

} // namespace w2n

#endif // W2N_IRGEN_IRGENMODULE_H