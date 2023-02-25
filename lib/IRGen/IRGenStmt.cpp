#include "IRGenFunction.h"
#include "Reduction.h"
#include <w2n/AST/Lowering.h>

using namespace w2n;
using namespace w2n::irgen;

#pragma mark - StmtEmitter Definition

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

} // namespace

#pragma mark - IRGenFunction

void IRGenFunction::emitStmt(Stmt * S) {
  StmtEmitter(*TopConfig).visit(S);
}

#pragma mark - StmtEmitter Implementation

void StmtEmitter::visitUnreachableStmt(UnreachableStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitBrStmt(BrStmt * S) {
  w2n_unimplemented();
}

void StmtEmitter::visitEndStmt(EndStmt * S) {
  w2n_unimplemented();
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