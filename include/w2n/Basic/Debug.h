#ifndef W2N_BASIC_DEBUG_H
#define W2N_BASIC_DEBUG_H

#include <llvm/Support/Compiler.h>

/// Adds attributes to the provided method signature indicating that it is
/// a debugging helper that should never be called directly from compiler
/// code. This deprecates the method so it won't be called directly and
/// marks it as used so it won't be eliminated as dead code.
#define W2N_DEBUG_HELPER(method)                                         \
  LLVM_ATTRIBUTE_DEPRECATED(                                             \
    method LLVM_ATTRIBUTE_USED, "only for use in the debugger"           \
  )

/// Declares a const void instance method with the name and parameters
/// provided. Methods declared with this macro should never be called
/// except in the debugger.
#define W2N_DEBUG_DUMPER(nameAndParams)                                  \
  W2N_DEBUG_HELPER(void nameAndParams const)

/// Declares an instance `void dump() const` method. Methods declared with
/// this macro should never be called except in the debugger.
#define W2N_DEBUG_DUMP W2N_DEBUG_DUMPER(dump())

#endif
