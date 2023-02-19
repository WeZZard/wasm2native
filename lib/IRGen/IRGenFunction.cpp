#include "IRGenFunction.h"
#include "Address.h"
#include "IRBuilder.h"
#include "IRGenInternal.h"
#include "IRGenModule.h"
#include "Reduction.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/ErrorHandling.h>
#include <cassert>
#include <cstdio>
#include <iterator>
#include <memory>
#include <utility>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/ASTWalker.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/Expr.h>
#include <w2n/AST/Lowering.h>
#include <w2n/AST/Stmt.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/Linking.h>

using namespace w2n;
using namespace w2n::irgen;

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

  auto Locals = emitProlog(
    Fn->getDeclContext(),
    Fn->getLocals(),
    Fn->getType()->getType()->getParameters(),
    Fn->getType()->getType()->getReturns()
  );

  auto Return = prepareEpilog(Fn->getType()->getType()->getReturns());

  RootConfig = std::make_unique<Configuration>(
    &getASTContext(), Fn, std::move(Locals), std::move(Return)
  );
  TopConfig = RootConfig.get();

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

std::vector<Address> IRGenFunction::emitProlog(
  DeclContext * DC,
  const std::vector<LocalDecl *>& Locals,
  ResultType * ParamsTy,
  ResultType * ResultTy
) {
  // Set up the IRBuilder.
  llvm::BasicBlock * EntryBB = createBasicBlock("entry");
  assert(
    CurFn->getBasicBlockList().empty() && "prologue already emitted?"
  );
  CurFn->getBasicBlockList().push_back(EntryBB);
  Builder.SetInsertPoint(EntryBB);

  // Set up the alloca insertion point.
  AllocaIP = Builder.IRBuilderBase::CreateAlloca(
    IGM.I1Ty,
    /*array size*/ nullptr,
    "alloca point"
  );
  EarliestIP = AllocaIP;

  std::vector<Address> FuncLocals;

  // Emit locals in activation record for funciton arguments
  for (auto& EachArg : CurFn->args()) {
    // FIXME: Needs cache?
    auto * Ty = EachArg.getType();
    // FIXME: Alignment
    Alignment FixedAlignment = Alignment(4);
    auto DebugName =
      (llvm::Twine("$local") + llvm::Twine(FuncLocals.size())
       + llvm::Twine(" aka $arg") + llvm::Twine(FuncLocals.size()))
        .str();
    auto Addr = createAlloca(Ty, FixedAlignment, DebugName);
    // TODO: zero-initialize Addr
    FuncLocals.emplace_back(Addr);
  }

  // Emit locals in activation record for local decls
  for (auto * EachLocal : Locals) {
    for (uint32_t I = 0; I < EachLocal->getCount(); I++) {
      // FIXME: Needs cache?
      auto * Ty = IGM.getType(EachLocal->getType());
      // FIXME: Alignment
      Alignment FixedAlignment = Alignment(4);
      auto DebugName =
        (llvm::Twine("$local") + llvm::Twine(FuncLocals.size())).str();
      auto Addr = createAlloca(Ty, FixedAlignment, DebugName);
      // TODO: zero-initialize Addr
      FuncLocals.emplace_back(Addr);
    }
  }

  return FuncLocals;
}

Address IRGenFunction::prepareEpilog(ResultType * ResultTy) {
  auto * ReturnTy = CurFn->getReturnType();
  if (ReturnTy->isVoidTy()) {
    return Address();
  }
  auto Name = llvm::Twine("$return-value").str();
  // FIXME: Alignment
  Alignment Align = Alignment(4);
  auto ReturnAddress = createAlloca(ReturnTy, Align, Name);
  // TODO: zero-initialize ReturnAddress
  return ReturnAddress;
}

// TODO: void IRGenFunction::emitProfilerIncrement(ExpressionDecl * Expr)

void IRGenFunction::emitEpilog() {
  w2n_proto_implemented([&] {
    assert(RootConfig != nullptr);
    assert(TopConfig != nullptr);
    auto& RetVal = RootConfig->top<Frame>().getReturn();
    auto ReturnType = CurFn->getReturnType();
    if (ReturnType->isVoidTy()) {
      Builder.CreateRetVoid();
    } else {
      auto * LoadInst =
        Builder.CreateLoad(RetVal, "$loaded-return-value");
      Builder.CreateRet(LoadInst);
    }
  });
}

void IRGenFunction::mergeCleanupBlocks() {
}

#pragma mark Expression Emission

namespace {

class StmtEmitter : public Lowering::ASTVisitor<StmtEmitter> {
public:

  Configuration& Config;

  StmtEmitter(Configuration& Config) : Config(Config){};

  StmtEmitter(const StmtEmitter&) = delete;
  StmtEmitter& operator=(const StmtEmitter&) = delete;

  StmtEmitter(StmtEmitter&&) = delete;
  StmtEmitter& operator=(StmtEmitter&&) = delete;

#define STMT(Id, Parent) void visit##Id##Stmt(Id##Stmt * S);
#include <w2n/AST/StmtNodes.def>
};

class RValueEmitter :
  public Lowering::ExprVisitor<RValueEmitter, RValue> {
public:

  Function * Fn;

  IRGenModule& IGM;

  IRBuilder& Builder;

  Configuration& Config;

  RValueEmitter(
    Function * Fn,
    IRGenModule& IGM,
    IRBuilder& Builder,
    Configuration& Config
  ) :
    Fn(Fn),
    IGM(IGM),
    Builder(Builder),
    Config(Config){};

  RValueEmitter(const RValueEmitter&) = delete;
  RValueEmitter& operator=(const RValueEmitter&) = delete;

  RValueEmitter(RValueEmitter&&) = delete;
  RValueEmitter& operator=(RValueEmitter&&) = delete;

  RValue visitExpr(Expr * E) {
    llvm_unreachable("Unhandled expression found!");
  }

#define W2N_LOG_VISIT()                                                  \
  llvm::outs() << "[RValueEmitter] " << __FUNCTION__ << "\n";

  // Grab a global variable address an push to the stack.
  RValue visitGlobalGetExpr(GlobalGetExpr * E) {
    W2N_LOG_VISIT();
    auto GlobalIter = Fn->getModule()->global_begin();
    std::advance(GlobalIter, E->getGlobalIndex());
    auto& Global = *GlobalIter;
    auto Addr = IGM.getAddrOfGlobalVariable(&Global, NotForDefinition);
    Config.push<Operand>(Addr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitGlobalSetExpr(GlobalSetExpr * E) {
    W2N_LOG_VISIT();
    auto * Op = Config.pop<Operand>();
    auto GlobalIter = Fn->getModule()->global_begin();
    std::advance(GlobalIter, E->getGlobalIndex());
    auto& Global = *GlobalIter;
    auto Addr = IGM.getAddrOfGlobalVariable(&Global, NotForDefinition);
    Builder.CreateStore(Op->getLowered(), Addr);
    Config.push<Operand>(Addr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitLocalSetExpr(LocalSetExpr * E) {
    W2N_LOG_VISIT();
    auto * Op = Config.pop<Operand>();
    auto * F = Config.findTopmost<Frame>();
    assert(F);
    auto Asignee = F->getLocals().at(E->getLocalIndex());
    // FIXME: Alignment
    Alignment FixedAlignment = Alignment(4);
    auto * Load = Builder.CreateLoad(
      Asignee, llvm::Twine("local$") + llvm::Twine(E->getLocalIndex())
    );
    Builder.CreateStore(
      Op->getLowered(), Load->getPointerOperand(), FixedAlignment
    );
    return RValue();
  }

  RValue visitIntegerConstExpr(IntegerConstExpr * E) {
    W2N_LOG_VISIT();
    auto * Ty = IGM.getType(E->getIntegerType());
    auto * ConstVal = llvm::ConstantInt::get(Ty, E->getValue());
    Config.push<Operand>(ConstVal);
    return RValue(Config.top<Operand>());
  }

  RValue visitLocalGetExpr(LocalGetExpr * E) {
    W2N_LOG_VISIT();
    auto * F = Config.findTopmost<Frame>();
    auto LocalAddr = F->getLocals().at(E->getLocalIndex());
    Config.push<Operand>(LocalAddr.getAddress());
    return RValue(Config.top<Operand>());
  }

  RValue visitDropExpr(DropExpr * E) {
    W2N_LOG_VISIT();
    Config.pop<Operand>();
    return RValue();
  }

  RValue visitStoreExpr(StoreExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitLoadExpr(LoadExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitCallExpr(CallExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

  RValue visitCallBuiltinExpr(CallBuiltinExpr * E) {
    W2N_LOG_VISIT();
    return w2n_proto_implemented([] { return RValue(); });
  }

#undef LOG_VISIT
};

} // namespace

void IRGenFunction::emitExpression(ExpressionDecl * D) {
  for (auto& EachInst : D->getInstructions()) {
    if (Expr * E = EachInst.dyn_cast<Expr *>()) {
      emitRValue(E);
    } else if (Stmt * S = EachInst.dyn_cast<Stmt *>()) {
      emitStmt(S);
    } else {
      llvm_unreachable("unexpected kind of instruction node.");
    }
  }
}

void IRGenFunction::emitStmt(Stmt * S) {
  StmtEmitter(*TopConfig).visit(S);
}

RValue IRGenFunction::emitRValue(Expr * E) {
  return RValueEmitter(Fn, IGM, Builder, *TopConfig).visit(E);
}

Address IRGenFunction::createAlloca(
  llvm::Type * Ty, Alignment Align, const llvm::Twine& Name
) {
  llvm::AllocaInst * Alloca = new llvm::AllocaInst(
    Ty, IGM.DataLayout.getAllocaAddrSpace(), Name, AllocaIP
  );
  Alloca->setAlignment(llvm::MaybeAlign(Align.getValue()).valueOrOne());
  return Address(Alloca, Ty, Align);
}

Address IRGenFunction::createAlloca(
  llvm::Type * Ty,
  llvm::Value * ArraySize,
  Alignment Align,
  const llvm::Twine& Name
) {
  llvm::AllocaInst * Alloca = new llvm::AllocaInst(
    Ty,
    IGM.DataLayout.getAllocaAddrSpace(),
    ArraySize,
    llvm::MaybeAlign(Align.getValue()).valueOrOne(),
    Name,
    AllocaIP
  );
  return Address(Alloca, Ty, Align);
}

#pragma mark IRGenFunction - Control Flow

llvm::BasicBlock * IRGenFunction::createBasicBlock(const llvm::Twine& Name
) const {
  return llvm::BasicBlock::Create(IGM.getLLVMContext(), Name);
}

#pragma mark StmtEmitter

void StmtEmitter::visitUnreachableStmt(UnreachableStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitBrStmt(BrStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitEndStmt(EndStmt * S) {
  w2n_proto_implemented();
  // Prototyped implementation
  w2n_proto_implemented([&] {
    while (Config.topKind() != StackContentKind::Frame) {
      Config.pop();
    }
  });
}

void StmtEmitter::visitBrIfStmt(BrIfStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitElseStmt(ElseStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitLoopStmt(LoopStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitBlockStmt(BlockStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitReturnStmt(ReturnStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitBrTableStmt(BrTableStmt * S) {
  w2n_proto_implemented();
}

void StmtEmitter::visitIfStmt(IfStmt * S) {
  w2n_proto_implemented();
}

#pragma mark ExprEmitter