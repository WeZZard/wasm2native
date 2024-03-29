#include "IRGenModule.h"
#include "Address.h"
#include "GenDecl.h"
#include "IRGenConstructor.h"
#include "IRGenFunction.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <cassert>
#include <cstdio>
#include <format>
#include <memory>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/IRGenRequests.h>
#include <w2n/AST/Linkage.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/Linking.h>
#include <w2n/Sema/Sema.h>

using namespace w2n;
using namespace w2n::irgen;

// FIXME: Move to IRGenerator
static llvm::DataLayout getDataLayout(const llvm::TargetMachine& Target) {
  return Target.createDataLayout();
}

// FIXME: Move to IRGenerator
static llvm::Triple getTriple(const llvm::TargetMachine& Target) {
  return Target.getTargetTriple();
}

IRGenModule::IRGenModule(
  IRGenerator& IRGen,
  std::unique_ptr<llvm::TargetMachine>&& Target,
  SourceFile * SF,
  StringRef ModuleName,
  StringRef OutputFilename,
  StringRef MainInputFilenameForDebugInfo
) :
  LLVMContext(new llvm::LLVMContext()),
  DataLayout(getDataLayout(*Target)),
  Triple(getTriple(*Target)),
  IRGen(IRGen),
  Context(IRGen.Module.getASTContext()),
  Module(std::make_unique<llvm::Module>(ModuleName, *LLVMContext)),
  TargetMachine(std::move(Target)),
  OutputFilename(OutputFilename),
  MainInputFilenameForDebugInfo(MainInputFilenameForDebugInfo),
  TargetInfo(WasmTargetInfo::get(*this)),
  ModuleHash(nullptr),
  VoidTy(llvm::Type::getVoidTy(getLLVMContext())),
  I1Ty(llvm::Type::getInt1Ty(getLLVMContext())),
  I8Ty(llvm::Type::getInt8Ty(getLLVMContext())),
  I16Ty(llvm::Type::getInt16Ty(getLLVMContext())),
  I32Ty(llvm::Type::getInt32Ty(getLLVMContext())),
  I64Ty(llvm::Type::getInt64Ty(getLLVMContext())),
  U8Ty(llvm::Type::getInt8Ty(getLLVMContext())),
  U16Ty(llvm::Type::getInt16Ty(getLLVMContext())),
  U32Ty(llvm::Type::getInt32Ty(getLLVMContext())),
  U64Ty(llvm::Type::getInt64Ty(getLLVMContext())),
  F32Ty(llvm::Type::getFloatTy(getLLVMContext())),
  F64Ty(llvm::Type::getDoubleTy(getLLVMContext())) {
  IRGen.addGenModule(SF, this);
}

IRGenModule::~IRGenModule() {
  // FIXME: Release resources
}

GeneratedModule IRGenModule::intoGeneratedModule() && {
  return GeneratedModule{
    std::move(LLVMContext), std::move(Module), std::move(TargetMachine)};
}

void IRGenModule::emitGlobalVariable(GlobalVariable * V) {
  ForDefinition_t Def = ForDefinition;
  if (V->isImported()) {
    Def = NotForDefinition;
  }
  Address Addr = getAddrOfGlobalVariable(V, Def);
  if (Def != NotForDefinition) {
    auto * InitFn = emitFunction(V->getInit());
    emitGlobalVariableConstructor(*this, V, Addr, InitFn);
  }
}

llvm::Function * IRGenModule::emitFunction(Function * F) {
  llvm::errs() << "[IRGenModule] " << __FUNCTION__ << "\n";
  if (F->isExternalDeclaration()) {
    return nullptr;
  }

  // TODO: PrettyStackTraceFunction stackTrace("emitting IR", f);
  return IRGenFunction(*this, F).emitFunction();
}

void IRGenModule::emitCoverageMapping() {
  w2n_proto_implemented();
}

void IRGenModule::finishEmitAfterTopLevel() {
  w2n_proto_implemented();
}

bool IRGenModule::finalize() {
  return w2n_proto_implemented([]() -> bool { return true; });
}

void IRGenModule::addLinkLibrary(const LinkLibrary& LinkLib) {
  w2n_unimplemented();
}

/// Emit all the top-level code in the source file.
void IRGenModule::emitSourceFile(SourceFile& SF) {
  for (Decl * D : SF.getTopLevelDecls()) {
    ModuleDecl * M = dyn_cast<ModuleDecl>(D);
    if (M == nullptr) {
      continue;
    }
    for (GlobalVariable& V : M->getGlobals()) {
      GlobalDecl * VarDecl = V.getDecl();
      CurrentIGMPtr IGM = IRGen.getGenModule(
        VarDecl != nullptr ? VarDecl->getDeclContext() : nullptr
      );
      IGM->emitGlobalVariable(&V);
    }

    for (Function& F : M->getFunctions()) {
      auto * DC = F.getDeclContext();
      CurrentIGMPtr IGM = IRGen.getGenModule(DC);
      IGM->emitFunction(&F);
    }
  }
  w2n_proto_implemented();
}

void IRGenModule::unimplemented(SourceLoc Loc, StringRef Message) {
  Context.Diags.diagnose(Loc, diag::irgen_unimplemented, Message);
}

void IRGenModule::fatalUnimplemented(SourceLoc Loc, StringRef Message) {
  Context.Diags.diagnose(Loc, diag::irgen_unimplemented, Message);
  llvm::report_fatal_error(
    llvm::Twine("unimplemented IRGen feature! ") + Message
  );
}

void IRGenModule::error(SourceLoc Loc, const Twine& Message) {
#define W2N_ERROR_MSG_BUFFER_LENGTH 128
  SmallVector<char, W2N_ERROR_MSG_BUFFER_LENGTH> Buffer;
#undef W2N_ERROR_MSG_BUFFER_LENGTH
  Context.Diags.diagnose(
    Loc, diag::irgen_failure, Message.toStringRef(Buffer)
  );
}

llvm::Module * IRGenModule::getModule() const {
  return Module.get();
}

Address IRGenModule::getAddrOfGlobalVariable(
  GlobalVariable * Global, ForDefinition_t ForDefinition
) {
  std::string GName = Global->getFullQualifiedDescriptiveName();

  auto * GVar = Module->getGlobalVariable(GName, /*allowInternal*/ true);

  llvm::Type * StorageType = getType(Global->getType());

  LinkEntity Entity = LinkEntity::forGlobalVariable(Global);
  LinkInfo Info = LinkInfo::get(*this, Entity, ForDefinition);

  if (GVar == nullptr) {
    // FIXME: Alignment
    Alignment FixedAlignment = Alignment(4);
    GVar = createGlobalVariable(*this, Info, StorageType, FixedAlignment);

    /// Add a zero initializer.
    if (ForDefinition != 0) {
      GVar->setInitializer(llvm::Constant::getNullValue(StorageType));
    } else {
      GVar->setComdat(nullptr);
    }
  }

  llvm::Constant * Addr = GVar;
  Addr =
    llvm::ConstantExpr::getBitCast(Addr, StorageType->getPointerTo());

  return Address(Addr, StorageType, Alignment(GVar->getAlignment()));
}

StackProtectorMode IRGenModule::shouldEmitStackProtector(Function * F) {
  const auto& Opts = IRGen.getOptions();
  return (Opts.EnableStackProtection) != 0
         ? StackProtectorMode::StackProtector
         : StackProtectorMode::NoStackProtector;
}

llvm::Type * IRGenModule::getStorageType(Type * T) const {
#define TYPE(Id, Parent)
#define NUMBER_TYPE(Id, Parent)                                          \
  case TypeKind::Id: return Id##Ty;

  switch (T->getKind()) {
#include <w2n/AST/TypeNodes.def>
  case TypeKind::V128:
  case TypeKind::FuncRef:
  case TypeKind::ExternRef:
  case TypeKind::Void:
  case TypeKind::Result:
  case TypeKind::Func:
  case TypeKind::Limits:
  case TypeKind::Memory:
  case TypeKind::Table:
  case TypeKind::Global:
  case TypeKind::TypeIndex:
  case TypeKind::Block: w2n_unimplemented();
  }
}

/// Add the given global value to @llvm.used.
///
/// This value must have a definition by the time the module is finalized.
void IRGenModule::addUsedGlobal(llvm::GlobalValue * Global) {
  // As of reviews.llvm.org/D97448 "ELF: Create unique SHF_GNU_RETAIN
  // sections for llvm.used global objects" LLVM creates separate sections
  // for globals in llvm.used on ELF.  Therefore we use llvm.compiler.used
  // on ELF instead.
  if (TargetInfo.OutputObjectFormat == llvm::Triple::ELF) {
    addCompilerUsedGlobal(Global);
    return;
  }

  LLVMUsed.push_back(Global);
}

/// Add the given global value to @llvm.compiler.used.
///
/// This value must have a definition by the time the module is finalized.
void IRGenModule::addCompilerUsedGlobal(llvm::GlobalValue * Global) {
  LLVMCompilerUsed.push_back(Global);
}

Signature IRGenModule::getSignature(FuncType * Ty) {
  return Signature::getUncached(*this, Ty);
}