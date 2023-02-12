#ifndef IRGEN_IRGENFUNCTION_H
#define IRGEN_IRGENFUNCTION_H

#include "IRBuilder.h"
#include "Reduction.h"
#include <llvm/IR/Function.h>
#include <memory>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/ASTVisitor.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/InstNode.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/OptimizationMode.h>
#include <w2n/Basic/Unimplemented.h>

namespace w2n {
namespace irgen {

class RValue {
public:

  Operand * LoweredValue;

  explicit RValue(Operand& LoweredValue) : LoweredValue(&LoweredValue) {
  }

  explicit RValue() : LoweredValue(nullptr) {
  }
};

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

  /// The root config for WebAssembly VM stack reduction.
  std::unique_ptr<Configuration> RootConfig;

  /// Current top config for WebAssembly VM stack reduction.
  Configuration * TopConfig;

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
    return Fn->getASTContext();
  }

  void emitFunction();

  void unimplemented(SourceLoc Loc, StringRef Message);

#pragma mark Function prologue and epilogue

  /// Generates prolog code to allocate and clean up mutable storage for
  /// local arguments.
  /// Prepares the root config for wasm VM stack reduction.
  std::vector<Address> emitProlog(
    DeclContext * DC,
    const std::vector<LocalDecl *>& Locals,
    ResultType * ParamsTy,
    ResultType * ResultTy
  );

  /// Create (but do not emit) the epilog branch, and save the
  /// current cleanups depth as the destination for return statement
  /// branches.
  Address prepareEpilog(ResultType * ResultTy);

  /// Emit code to increment a counter for profiling.
  void emitProfilerIncrement(ExpressionDecl * Expr) {
    w2n_proto_implemented([] {});
  }

  /// Emits a standard epilog which runs top-level cleanups then returns
  /// the function return value, if any.
  void emitEpilog();

  void mergeCleanupBlocks();

#pragma mark Expression Emission

  using ASTVisitorType::visit;

  void visit(ExpressionDecl * E) = delete;

  void emitExpression(ExpressionDecl * D);

  void visit(Stmt * S) = delete;

  void emitStmt(Stmt * S);

  void visit(Expr * E) = delete;

  // R-values are values put on WebAssembly evaluation stack at runtime.
  //
  RValue emitRValue(Expr * E);

#pragma mark Control Flow

  llvm::BasicBlock * createBasicBlock(const llvm::Twine& Name) const;

#pragma mark Helper Methods

  Address createAlloca(
    llvm::Type * Ty, Alignment Alignment, const llvm::Twine& Name = ""
  );

  Address createAlloca(
    llvm::Type * Ty,
    llvm::Value * ArraySize,
    Alignment Align,
    const llvm::Twine& Name = ""
  );

private:

  llvm::Instruction * AllocaIP;
  // TODO: const SILDebugScope * DbgScope;
  /// The insertion point where we should but instructions we would
  /// normally put at the beginning of the function. LLVM's coroutine
  /// lowering really does not like it if we put instructions with
  /// side-effectrs before the coro.begin.
  llvm::Instruction * EarliestIP;

public:

  void setEarliestInsertionPoint(llvm::Instruction * Inst) {
    EarliestIP = Inst;
  }

  /// Returns the first insertion point before which we should insert
  /// instructions which have side-effects.
  llvm::Instruction * getEarliestInsertionPoint() const {
    return EarliestIP;
  }
};

} // namespace irgen
} // namespace w2n

#endif // IRGEN_IRGENFUNCTION_H
