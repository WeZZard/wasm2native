#ifndef W2N_BASIC_LLVMINITIALIZE_H
#define W2N_BASIC_LLVMINITIALIZE_H

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#define PROGRAM_START(argc, argv) \
  llvm::InitLLVM _INITIALIZE_LLVM(argc, argv)

#define INITIALIZE_LLVM() \
  do { \
    llvm::InitializeAllTargets(); \
    llvm::InitializeAllTargetMCs(); \
    llvm::InitializeAllAsmPrinters(); \
    llvm::InitializeAllAsmParsers(); \
    llvm::InitializeAllDisassemblers(); \
    llvm::InitializeAllTargetInfos(); \
  } while (0)

#endif // W2N_BASIC_LLVMINITIALIZE_H
