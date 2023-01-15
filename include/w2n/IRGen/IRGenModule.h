#ifndef W2N_IRGEN_IRGENMODULE_H
#define W2N_IRGEN_IRGENMODULE_H

#include <_types/_uint32_t.h>
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Support/ErrorHandling.h"
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
#include <w2n/AST/Type.h>
#include <w2n/Basic/FileSystem.h>
#include <w2n/IRGen/IRGenerator.h>
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
  void addLinkLibrary(const LinkLibrary& LinkLib);

  void emitGlobalVariable(GlobalVariable * V);

  void emitCoverageMapping();

  void finishEmitAfterTopLevel();

  /// Attempt to finalize the module.
  ///
  /// This can fail, in which it will return false and the module will be
  /// invalid.
  bool finalize();

#pragma mark Accessing Module Contents

  llvm::Module * getModule() const;

  Address getAddrOfGlobalVariable(
    GlobalVariable * Global, ForDefinition_t ForDefinition
  );

#pragma mark Accessing Types

private:

  llvm::IntegerType * Int32Ty; /// i32
  llvm::IntegerType * Int64Ty; /// i64
  llvm::Type * FloatTy;        /// f32
  llvm::Type * DoubleTy;       /// f64

  mutable llvm::DenseMap<VectorTyKey, llvm::StructType *> VectorTys;

public:

  llvm::Type * getType(ValueType * Ty) const {
    if (isa<I32Type>(Ty)) {
      return Int32Ty;
    }
    if (isa<I64Type>(Ty)) {
      return Int64Ty;
    }
    if (isa<F32Type>(Ty)) {
      return FloatTy;
    }
    if (isa<F64Type>(Ty)) {
      return DoubleTy;
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