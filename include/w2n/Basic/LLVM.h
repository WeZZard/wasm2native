#ifndef W2N_BASIC_LLVM_H
#define W2N_BASIC_LLVM_H

// Do not proliferate #includes here, require clients to #include their
// dependencies.
// Casting.h has complex templates that cannot be easily forward declared.
#include <llvm/Support/Casting.h>
// None.h includes an enumerator that is desired & cannot be forward
// declared without a definition of NoneType.
#include <llvm/ADT/None.h>

// Don't pre-declare certain LLVM types, which must not put things in
// namespace llvm for ODR(one-definition-rule) reasons.
#define W2N_LLVM_ODR_SAFE 1

// Forward declarations.
namespace llvm {
// Containers.
class StringRef;
class StringLiteral;
class Twine;
template <typename T>
class SmallPtrSetImpl;
template <typename T, unsigned N>
class SmallPtrSet;
#if W2N_LLVM_ODR_SAFE
template <typename T>
class SmallVectorImpl;
template <typename T, unsigned N>
class SmallVector;
#endif
template <unsigned N>
class SmallString;
template <typename T, unsigned N>
class SmallSetVector;
#if W2N_LLVM_ODR_SAFE
template <typename T>
class ArrayRef;
template <typename T>
class MutableArrayRef;
#endif
template <typename T>
class TinyPtrVector;
#if W2N_LLVM_ODR_SAFE
template <typename T>
class Optional;
#endif
template <typename... PTs>
class PointerUnion;
template <typename IteratorT>
class iterator_range;
class SmallBitVector;

// Other common classes.
class raw_ostream;
class APInt;
class APFloat;
#if W2N_LLVM_ODR_SAFE
template <typename Fn>
class function_ref;
#endif
} // end namespace llvm

namespace w2n {
// Casting operators.
using llvm::cast;
using llvm::cast_or_null;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::isa;
using llvm::isa_and_nonnull;

// Containers.
#if W2N_LLVM_ODR_SAFE
using llvm::ArrayRef;
using llvm::MutableArrayRef;
#endif
using llvm::iterator_range;
using llvm::None;
#if W2N_LLVM_ODR_SAFE
using llvm::Optional;
#endif
using llvm::PointerUnion;
using llvm::SmallBitVector;
using llvm::SmallPtrSet;
using llvm::SmallPtrSetImpl;
using llvm::SmallSetVector;
using llvm::SmallString;
#if W2N_LLVM_ODR_SAFE
using llvm::SmallVector;
using llvm::SmallVectorImpl;
#endif
using llvm::StringLiteral;
using llvm::StringRef;
using llvm::TinyPtrVector;
using llvm::Twine;

// Other common classes.
using llvm::APFloat;
using llvm::APInt;
#if W2N_LLVM_ODR_SAFE
using llvm::function_ref;
#endif
using llvm::NoneType;
using llvm::raw_ostream;
} // end namespace w2n

#endif // W2N_BASIC_LLVM_H
