#ifndef IRGEN_IRGENCONSTRUCTOR_H
#define IRGEN_IRGENCONSTRUCTOR_H

#include <llvm/IR/Function.h>

namespace w2n {
class GlobalVariable;

namespace irgen {
class IRGenModule;
class Address;

void emitGlobalVariableConstructor(
  IRGenModule& Module,
  GlobalVariable * V,
  Address Addr,
  llvm::Function * Init
);

} // namespace irgen

} // namespace w2n

#endif // IRGEN_IRGENCONSTRUCTOR_H
