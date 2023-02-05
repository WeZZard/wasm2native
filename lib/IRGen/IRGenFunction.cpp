#include "IRGenFunction.h"
#include "IRGenInternal.h"
#include "IRGenModule.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Use.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/ASTWalker.h>
#include <w2n/AST/Reduction.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;
using namespace w2n::irgen;

namespace {

static Lowering::Value emitValue(IRGenModule& IGM, ValueType * Ty) {
  switch (Ty->getValueTypeKind()) {
  case ValueTypeKind::I8:
  case ValueTypeKind::I16:
  case ValueTypeKind::I32:
  case ValueTypeKind::I64:
    return Lowering::Value(
      llvm::ConstantInt::get(IGM.getType(Ty), 0, true)
    );
  case ValueTypeKind::U8:
  case ValueTypeKind::U16:
  case ValueTypeKind::U32:
  case ValueTypeKind::U64:
    return Lowering::Value(
      llvm::ConstantInt::get(IGM.getType(Ty), 0, false)
    );
  case ValueTypeKind::F32:
  case ValueTypeKind::F64:
    return Lowering::Value(llvm::ConstantFP::get(IGM.getType(Ty), 0));
  case ValueTypeKind::V128:
  case ValueTypeKind::FuncRef:
  case ValueTypeKind::ExternRef: w2n_unimplemented();
  case ValueTypeKind::Void: return Lowering::Value(nullptr);
  case ValueTypeKind::None:
    llvm_unreachable("unexpected value type kind.");
  }
}

static llvm::AllocaInst * createEntryBlockAlloca(
  IRGenModule& IGM, llvm::Function * Fn, llvm::Type * Ty, StringRef Name
) {
  llvm::IRBuilder<> Builder(
    &Fn->getEntryBlock(), Fn->getEntryBlock().begin()
  );
  return Builder.CreateAlloca(Ty, nullptr, Name);
}

} // namespace

IRGenFunction::IRGenFunction(
  IRGenModule& IGM, Function * Fn, OptimizationMode Mode
) :
  IGM(IGM),
  // FIXME: Derive IRBuilder DebugInfo from IGM & Mode
  Builder(IGM.getLLVMContext(), true),
  OptMode(Mode),
  CurFn(nullptr),
  Fn(Fn) {
}

IRGenFunction::~IRGenFunction() {
}

void IRGenFunction::emitFunction() {
  if (CurFn != nullptr) {
    return;
  }

  llvm::FunctionType * FnTy = IGM.getFuncType(Fn->getType()->getType());

  CurFn = llvm::Function::Create(
    FnTy,
    llvm::Function::ExternalLinkage,
    Fn->getUniqueName(),
    IGM.getModule()
  );

  llvm::BasicBlock * Entry =
    llvm::BasicBlock::Create(IGM.getLLVMContext(), "entry", CurFn);

  Builder.SetInsertPoint(Entry);

  emitProlog(
    Fn->getDeclContext(),
    Fn->getLocals(),
    Fn->getType()->getType()->getParameters(),
    Fn->getType()->getType()->getReturns()
  );

  prepareEpilog(Fn->getType()->getType()->getReturns());

  emitProfilerIncrement(Fn->getExpression());

  // Emit the actual function body as usual
  emitExpression(Fn->getExpression());

  emitEpilog();

  mergeCleanupBlocks();
}

void IRGenFunction::unimplemented(SourceLoc Loc, StringRef Message) {
  return IGM.unimplemented(Loc, Message);
}

#pragma mark Function prologue and epilogue

void IRGenFunction::emitProlog(
  DeclContext * DC,
  const std::vector<LocalDecl *>& Locals,
  ResultType * ParamsTy,
  ResultType * ResultTy
) {
  uint32_t LocalIndex = 0;
  for (auto EachLocal : Locals) {
    for (uint32_t I = 0; I < EachLocal->getCount(); I++) {
      auto Val = emitLocal(EachLocal);
      auto DebugName =
        (llvm::Twine("$local") + llvm::Twine(LocalIndex)).str();
      llvm::AllocaInst * Alloca = createEntryBlockAlloca(
        IGM, CurFn, Val.getLowered()->getType(), DebugName
      );
      // FIXME: Temporary workaround
      Alignment FixedAlignment = Alignment(4);
      Builder.CreateStore(Val.getLowered(), Alloca, FixedAlignment);
      FuncLocals.emplace_back(std::move(Val));
    }
  }
}

void IRGenFunction::prepareEpilog(ResultType * ResultTy) {
  auto ReturnType = CurFn->getReturnType();
  if (ReturnType->isVoidTy()) {
    FuncReturn = Lowering::Value(nullptr);
    return;
  }
  llvm::Value * Val = nullptr;
  if (auto * StructTy = dyn_cast<llvm::StructType>(ReturnType)) {
    Val = llvm::ConstantStruct::get(StructTy, {});
  } else if (auto * IntTy = dyn_cast<llvm::IntegerType>(ReturnType)) {
    Val = llvm::ConstantInt::get(IntTy, 0);
  } else {
    w2n_unimplemented();
  }
  auto DebugName = llvm::Twine("$return-value").str();
  // FIXME: Insert to epilog block when this is not a prototype.
  llvm::AllocaInst * Alloca =
    createEntryBlockAlloca(IGM, CurFn, ReturnType, DebugName);
  // FIXME: Temporary workaround
  Alignment FixedAlignment = Alignment(4);
  Builder.CreateStore(Val, Alloca, FixedAlignment);
  FuncReturn = Lowering::Value(Val);
}

// TODO: void IRGenFunction::emitProfilerIncrement(ExpressionDecl * Expr)

void IRGenFunction::emitEpilog() {
  w2n_proto_implemented([&] {
    assert(FuncReturn.has_value());
    auto& RetVal = FuncReturn.value();
    auto ReturnType = CurFn->getReturnType();
    if (ReturnType->isVoidTy()) {
      Builder.CreateRetVoid();
    } else {
      Builder.CreateRet(RetVal.getLowered());
    }
  });
}

void IRGenFunction::mergeCleanupBlocks() {
}

#pragma mark Local Emission

Lowering::Value IRGenFunction::emitLocal(LocalDecl * Local) {
  // FIXME: Needs cache?
  return emitValue(IGM, Local->getType());
}

#pragma mark Expression Emission

void IRGenFunction::emitExpression(ExpressionDecl * Expr) {
}

void IRGenFunction::emitStmt(Stmt * S) {
}

Lowering::Value IRGenFunction::emitLoweredValue(Expr * E) {
}