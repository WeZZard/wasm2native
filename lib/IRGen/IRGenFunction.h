#ifndef W2N_IRGEN_IRGENFUNCTION_H
#define W2N_IRGEN_IRGENFUNCTION_H

#include "IRBuilder.h"
#include <llvm/IR/Function.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/OptimizationMode.h>

namespace w2n {
namespace irgen {

class IRGenModule;

/// IRGenFunction - Primary class for emitting LLVM instructions for a
/// specific function.
class IRGenFunction {
public:

  IRGenModule& IGM;
  IRBuilder Builder;

  /// If != OptimizationMode::NotSet, the optimization mode specified with
  /// an function attribute.
  OptimizationMode OptMode;

  llvm::Function * CurFn;

  Function * Fn;

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

  void emitFunction();

  void unimplemented(SourceLoc Loc, StringRef Message);

#pragma mark Function prologue and epilogue

#pragma mark Memory Management

#pragma mark Expression Emission
};

} // namespace irgen
} // namespace w2n

#endif // W2N_IRGEN_IRGENFUNCTION_H
