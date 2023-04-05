#include "Address.h"
#include "IRGenFunction.h"
#include "Reduction.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <cassert>
#include <stack>
#include <w2n/AST/Lowering.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;
using namespace w2n::irgen;

#pragma mark - StmtEmitter Definition

namespace {

class StmtEmitter : public Lowering::ASTVisitor<StmtEmitter> {
public:

  IRGenModule& IGM;

  IRBuilder& Builder;

  Configuration& Config;

  StmtEmitter(
    IRGenModule& IGM, IRBuilder& Builder, Configuration& Config
  ) :
    IGM(IGM),
    Builder(Builder),
    Config(Config){};

  StmtEmitter(const StmtEmitter&) = delete;
  StmtEmitter& operator=(const StmtEmitter&) = delete;

  StmtEmitter(StmtEmitter&&) = delete;
  StmtEmitter& operator=(StmtEmitter&&) = delete;

#define STMT(Id, Parent) void visit##Id##Stmt(Id##Stmt * S);
#include <w2n/AST/StmtNodes.def>
};

} // namespace

#pragma mark - IRGenFunction

void IRGenFunction::emitStmt(Stmt * S) {
  StmtEmitter(IGM, Builder, *TopConfig).visit(S);
}

#pragma mark - StmtEmitter Implementation

void StmtEmitter::visitUnreachableStmt(UnreachableStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitBrStmt(BrStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitEndStmt(EndStmt * S) {
  auto NextTopKind = Config.topKind();
  std::stack<Operand *> PoppedOps;
  while (NextTopKind == ExecutionStackRecordKind::Operand) {
    PoppedOps.push(Config.pop<Operand>());
    NextTopKind = Config.topKind();
  }
  if (NextTopKind == ExecutionStackRecordKind::Frame) {
    auto& F = Config.top<Frame>();
    auto& RetAddr = F.getReturn();
    if (!PoppedOps.empty()) {
      assert(PoppedOps.size() == 1);
      // FIXME: Alignment
      Builder.CreateStore(PoppedOps.top()->getLowered(), RetAddr);
    } else {
      assert(F.hasNoReturn());
    }
  } else if (NextTopKind == ExecutionStackRecordKind::Label) {
    Config.pop<Label>();
    while (!PoppedOps.empty()) {
      auto * Node = PoppedOps.top();
      PoppedOps.pop();
      Config.push(Node);
    }
  }
}

void StmtEmitter::visitBrIfStmt(BrIfStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitElseStmt(ElseStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitLoopStmt(LoopStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitBlockStmt(BlockStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitReturnStmt(ReturnStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitBrTableStmt(BrTableStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitIfStmt(IfStmt * S) {
  w2n_unimplemented();
}