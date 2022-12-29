#ifndef W2N_FORMAT_FORMAT_H
#define W2N_FORMAT_FORMAT_H

#include <llvm/BinaryFormat/Wasm.h>

#define W2N_FORMAT_GET_SEC_TYPE(Id) _W2N_FORMAT_GET_SEC_TYPE_##Id

#define _W2N_FORMAT_GET_SEC_TYPE_CustomSection  llvm::wasm::WASM_SEC_CUSTOM
#define _W2N_FORMAT_GET_SEC_TYPE_TypeSection    llvm::wasm::WASM_SEC_TYPE
#define _W2N_FORMAT_GET_SEC_TYPE_ImportSection  llvm::wasm::WASM_SEC_IMPORT
#define _W2N_FORMAT_GET_SEC_TYPE_FuncSection    llvm::wasm::WASM_SEC_FUNCTION
#define _W2N_FORMAT_GET_SEC_TYPE_TableSection   llvm::wasm::WASM_SEC_TABLE
#define _W2N_FORMAT_GET_SEC_TYPE_MemorySection  llvm::wasm::WASM_SEC_MEMORY
#define _W2N_FORMAT_GET_SEC_TYPE_GlobalSection  llvm::wasm::WASM_SEC_GLOBAL
#define _W2N_FORMAT_GET_SEC_TYPE_ExportSection  llvm::wasm::WASM_SEC_EXPORT
#define _W2N_FORMAT_GET_SEC_TYPE_StartSection   llvm::wasm::WASM_SEC_START
#define _W2N_FORMAT_GET_SEC_TYPE_ElementSection llvm::wasm::WASM_SEC_ELEM
#define _W2N_FORMAT_GET_SEC_TYPE_CodeSection    llvm::wasm::WASM_SEC_CODE
#define _W2N_FORMAT_GET_SEC_TYPE_DataSection    llvm::wasm::WASM_SEC_DATA
#define _W2N_FORMAT_GET_SEC_TYPE_DataCountSection                        \
  llvm::wasm::WASM_SEC_DATACOUNT

#endif // W2N_FORMAT_FORMAT_H