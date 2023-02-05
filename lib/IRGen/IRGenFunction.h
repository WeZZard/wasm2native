#ifndef IRGEN_IRGENFUNCTION_H
#define IRGEN_IRGENFUNCTION_H

#include "IRBuilder.h"
#include <llvm/IR/Function.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Reduction.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/OptimizationMode.h>
#include <w2n/Basic/Unimplemented.h>

namespace w2n {
namespace irgen {

class IRGenModule;

/// IRGenFunction - The primary class for emitting LLVM instructions for a
/// specific function with RAII-style.
class IRGenFunction : public ASTVisitor<IRGenFunction> {
public:

  IRGenModule& IGM;
  IRBuilder Builder;

  /// If != OptimizationMode::NotSet, the optimization mode specified with
  /// an function attribute.
  OptimizationMode OptMode;

  llvm::Function * CurFn;

  Function * Fn;

  std::vector<Lowering::Value> FuncLocals;

  Optional<Lowering::Value> FuncReturn;

  ModuleDecl * getWasmModule() const;
  const IRGenOptions& getOptions() const;

  IRGenFunction(
    IRGenModule& IGM,
    Function * Fn,
    OptimizationMode Mode = OptimizationMode::NotSet
    // const DebugScope * DbgScope = nullptr,
    // Optional<Location> DbgLoc = None
  );
  ~IRGenFunction();

  ASTContext& getASTContext() const {
    return Fn->getDeclContext()->getASTContext();
  }

  void emitFunction();

  void unimplemented(SourceLoc Loc, StringRef Message);

#pragma mark Function prologue and epilogue

  /// Generates prolog code to allocate and clean up mutable storage for
  /// local arguments.
  void emitProlog(
    DeclContext * DC,
    const std::vector<LocalDecl *>& Locals,
    ResultType * ParamsTy,
    ResultType * ResultTy
  );

  /// Create (but do not emit) the epilog branch, and save the
  /// current cleanups depth as the destination for return statement
  /// branches.
  void prepareEpilog(ResultType * ResultTy);

  /// Emit code to increment a counter for profiling.
  void emitProfilerIncrement(ExpressionDecl * Expr) {
    w2n_proto_implemented([] {});
  }

  /// Emits a standard epilog which runs top-level cleanups then returns
  /// the function return value, if any.
  void emitEpilog();

  void mergeCleanupBlocks();

#pragma mark Local Emission

  // FIXME: Do we need an explicit l-value emission infrastructure?
  Lowering::Value emitLocal(LocalDecl * Local);

#pragma mark Expression Emission

  using ASTVisitorType::visit;

  void visit(ExpressionDecl * E) = delete;

  void emitExpression(ExpressionDecl * Expr);

  void visit(Stmt * S) = delete;

  void emitStmt(Stmt * S);

  void visit(Expr * E) = delete;

  // FIXME: Do we need an R-value class to express explosion?
  Lowering::Value emitLoweredValue(Expr * E);
};

} // namespace irgen
} // namespace w2n

#endif // IRGEN_IRGENFUNCTION_H
