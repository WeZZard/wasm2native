#ifndef IRGEN_IRGENMODULE_H
#define IRGEN_IRGENMODULE_H

#include "IRGenerator.h"
#include "Signature.h"
#include "TypeLowering.h"
#include "WasmTargetInfo.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/IR/Type.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/TargetMachine.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <w2n/AST/Function.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/LinkLibrary.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/FileSystem.h>
#include <w2n/Basic/LLVMHashing.h>
#include <w2n/Basic/SuccessorMap.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/Linking.h>

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

  SourceFile * getSourceFile() const {
    return IRGen.getSourceFile(this);
  }

  const IRGenOptions& getOptions() const {
    return IRGen.Opts;
  }

  llvm::StringMap<ModuleDecl *> OriginalModules;

  llvm::SmallString<PathLength128> OutputFilename;

  llvm::SmallString<PathLength128> MainInputFilenameForDebugInfo;

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

  llvm::Function * emitFunction(Function * Func);

  void emitCoverageMapping();

  void finishEmitAfterTopLevel();

  /// Attempt to finalize the module.
  ///
  /// This can fail, in which it will return false and the module will be
  /// invalid.
  bool finalize();

#pragma mark Reporting Errors

  void unimplemented(SourceLoc, StringRef Message);

  W2N_NO_RETURN
  void fatalUnimplemented(SourceLoc, StringRef Message);

  void error(SourceLoc Loc, const Twine& Message);

#pragma mark Accessing Module Contents

  llvm::Module * getModule() const;

  Address getAddrOfGlobalVariable(
    GlobalVariable * Global, ForDefinition_t ForDefinition
  );

  llvm::Function *
  getAddrOfFunction(Function * F, ForDefinition_t ForDefinition);

#pragma mark Types

  llvm::Type * VoidTy;
  llvm::IntegerType * I1Ty;  /// i1, ported from Swift
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

private:

  mutable llvm::DenseMap<VectorTyKey, llvm::ArrayType *> VectorTys;

  mutable llvm::DenseMap<StructTyKey, llvm::StructType *> StructTys;

  mutable llvm::DenseMap<FuncTyKey, llvm::FunctionType *> FuncTys;

public:

  llvm::Type * getType(Type * Ty) const {
#define TYPE(Id, Parent)
#define NUMBER_TYPE(Id, Parent)                                          \
  case TypeKind::Id: return Id##Ty;
    switch (Ty->getKind()) {
#include <w2n/AST/TypeNodes.def>
    case TypeKind::V128:
      llvm_unreachable("vector type in WebAssembly does not have fixed "
                       "reflection in LLVM.");
    case TypeKind::Void: return VoidTy;
    case TypeKind::Func: return getFuncType(dyn_cast<FuncType>(Ty));
    case TypeKind::Result: return getResultType(dyn_cast<ResultType>(Ty));
    case TypeKind::Global: return getGlobalTy(dyn_cast<GlobalType>(Ty));
    case TypeKind::Block:
    case TypeKind::ExternRef:
    case TypeKind::FuncRef:
    case TypeKind::Limits:
    case TypeKind::Memory:
    case TypeKind::Table:
    case TypeKind::TypeIndex: w2n_unimplemented();
    }
    llvm_unreachable("unexpected value type.");
  }

  llvm::Type * getGlobalTy(GlobalType * Ty) const {
    return getType(Ty->getType());
  }

private:

  std::vector<llvm::Type *> lowerResultType(ResultType * Ty) const {
    auto& ResultType = Ty->getValueTypes();

    std::vector<llvm::Type *> LoweredResultTypes;
    LoweredResultTypes.reserve(ResultType.size());
    std::transform(
      ResultType.cbegin(),
      ResultType.cend(),
      std::back_inserter(LoweredResultTypes),
      [&](ValueType * Ty) -> llvm::Type * { return getType(Ty); }
    );

    return LoweredResultTypes;
  }

public:

  llvm::FunctionType * getFuncType(FuncType * Ty) const {
    auto LoweredParamTypes = lowerResultType(Ty->getParameters());

    std::vector<llvm::Type *> LoweredResultTypes =
      lowerResultType(Ty->getReturns());

    FuncTyKey FuncKey = FuncTyKey(LoweredParamTypes, LoweredResultTypes);

    auto Iter = FuncTys.find(FuncKey);

    if (Iter != FuncTys.end()) {
      return Iter->second;
    }

    auto * ResultTy = getResultType(Ty->getReturns());

    auto * FuncTy = llvm::FunctionType::get(
      ResultTy, LoweredParamTypes, /*variadic*/ false
    );

    FuncTys.insert({FuncKey, FuncTy});

    return FuncTy;
  }

  llvm::Type * getResultType(ResultType * Ty) const {
    std::vector<llvm::Type *> Subtypes = lowerResultType(Ty);

    if (Subtypes.empty()) {
      return llvm::Type::getVoidTy(getLLVMContext());
    }

    if (Subtypes.size() == 1) {
      return Subtypes[0];
    }

    StructTyKey Key = {Subtypes};

    auto Iter = StructTys.find(Key);

    if (Iter != StructTys.end()) {
      return Iter->second;
    }

    auto * StructTy = llvm::StructType::create(*LLVMContext, Subtypes);

    StructTys.insert({Key, StructTy});

    return StructTy;
  }

  llvm::ArrayType *
  getVectorOfType(uint32_t Size, llvm::Type * ElementTy) const {
    VectorTyKey Key = VectorTyKey{ElementTy, Size};
    const auto I = VectorTys.find(Key);
    if (I != VectorTys.end()) {
      return I->second;
    }
    auto * Ty = llvm::ArrayType::get(ElementTy, Size);
    VectorTys.insert(std::make_pair(Key, Ty));
    return Ty;
  }

  llvm::Type * getStorageType(Type * T) const;

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

  /// A mapping from order numbers to the LLVM functions which we
  /// created for the SIL functions with those orders.
  SuccessorMap<unsigned, llvm::Function *> EmittedFunctionsByOrder;

  /// List of all of the functions, which can be lookup by name
  /// up at runtime.
  SmallVector<Function *, 4> AccessibleFunctions;

#pragma mark Function

  StackProtectorMode shouldEmitStackProtector(Function * F);

  Signature getSignature(FuncType * Ty);
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

#endif // IRGEN_IRGENMODULE_H
