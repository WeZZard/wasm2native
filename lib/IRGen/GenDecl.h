#ifndef W2N_IRGEN_GENDECL_H
#define W2N_IRGEN_GENDECL_H

#include "DebugTypeInfo.h"
#include "IRGenInternal.h"
#include <llvm/IR/CallingConv.h>
#include <llvm/Support/CommandLine.h>
#include <w2n/Basic/OptimizationMode.h>

namespace llvm {
class AttributeList;
class Function;
class FunctionType;
} // namespace llvm

namespace w2n {

namespace irgen {
class IRGenModule;
class LinkEntity;
class LinkInfo;
class Signature;

void updateLinkageForDefinition(
  IRGenModule& IGM, llvm::GlobalValue * Global, const LinkEntity& Entity
);

llvm::Function * createFunction(
  IRGenModule& IGM,
  LinkInfo& LinkInfo,
  const Signature& Signature,
  llvm::Function * InsertBefore = nullptr,
  OptimizationMode FuncOptMode = OptimizationMode::NotSet,
  StackProtectorMode StackProtect = StackProtectorMode::NoStackProtector
);

llvm::GlobalVariable * createGlobalVariable(
  IRGenModule& IGM,
  LinkInfo& LinkInfo,
  llvm::Type * ObjectType,
  Alignment Alignment,
  DebugTypeInfo DebugType = DebugTypeInfo(),
  Optional<SourceLoc> DebugLoc = None,
  StringRef DebugName = StringRef()
);

llvm::GlobalVariable *
createLinkerDirectiveVariable(IRGenModule& IGM, StringRef Name);

void disableAddressSanitizer(
  IRGenModule& IGM, llvm::GlobalVariable * var
);

} // namespace irgen

} // namespace w2n

#endif // W2N_IRGEN_GENDECL_H
