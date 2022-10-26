#ifndef W2N_IRGEN_IRGENMODULE_H
#define W2N_IRGEN_IRGENMODULE_H

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <map>
#include <memory>
#include <w2n/AST/SourceFile.h>

namespace w2n {

/// The principal singleton which manages all of IR generation.
///
/// The IRGenerator delegates the emission of different top-level entities
/// to different instances of IRGenModule, each of which creates a
/// different llvm::Module.
///
/// In single-threaded compilation, the IRGenerator creates only a single
/// IRGenModule. In multi-threaded compilation, it contains multiple
/// IRGenModules - one for each LLVM module (= one for each input/output
/// file).
class IRGenerator {};

class IRGenModule {
  llvm::LLVMContext * Context;
  llvm::Module * Module;
  llvm::IRBuilder<> * Builder;
  std::map<std::string, llvm::AllocaInst *> NamedValues;

  IRGenModule(
    IRGenerator& Generator,
    std::unique_ptr<llvm::TargetMachine>&& Target,
    SourceFile * SF,
    StringRef ModuleName,
    StringRef OutputFilename,
    StringRef MainInputFilenameForDebugInfo
  );
};

} // namespace w2n

#endif // W2N_IRGEN_IRGENMODULE_H