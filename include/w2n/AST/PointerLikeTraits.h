#ifndef W2N_AST_POINTERLIKETRAITS_H
#define W2N_AST_POINTERLIKETRAITS_H

/// \file This file defines the alignment of various w2n AST classes.
///
/// It's useful to do this in a dedicated place to avoid recursive header
/// problems. To make sure we don't have any ODR violations, this header
/// should be included in every header that defines one of the forward-
/// declared types listed here.

#include <cstddef>

/**
 * @brief Attributes a custom type's alignment to make it usable in LLVM
 * data structures which has pointer-alignment-oriented optimizations.
 */
#define LLVM_POINTER_LIKE_ALIGNMENT(TYPE_NAME)                           \
  alignas(1 << w2n::TYPE_NAME##AlignInBits)

namespace w2n {
template <typename AlignTy>
class ASTAllocated;
class ASTContext;
class Decl;
class DeclContext;
class FileUnit;

/// We frequently use three tag bits on all of these types.
constexpr size_t ASTAllocatedAlignInBits = 3;
constexpr size_t DeclAlignInBits = 3;
constexpr size_t DeclContextAlignInBits = 3;

constexpr size_t ASTContextAlignInBits = 2;

// Well, this is the *minimum* pointer alignment; it's going to be 3 on
// 64-bit targets, but that doesn't matter.
constexpr size_t PointerAlignInBits = 2;
} // namespace w2n

namespace llvm {
/// Helper class for declaring the expected alignment of a pointer.
/// TODO: LLVM should provide this.
template <class T, size_t AlignInBits>
struct MoreAlignedPointerTraits {
  enum {
    NumLowBitsAvailable = AlignInBits
  };

  static inline void * getAsVoidPointer(T * ptr) {
    return ptr;
  }

  static inline T * getFromVoidPointer(void * ptr) {
    return static_cast<T *>(ptr);
  }
};

template <class T>
struct PointerLikeTypeTraits;
} // namespace llvm

/**
 * @brief Declare the expected alignment of pointers to the given type.
 * This macro should be invoked from a top-level file context.
 */
#define LLVM_DECLARE_TYPE_ALIGNMENT(CLASS, ALIGNMENT)                    \
  namespace llvm {                                                       \
  template <>                                                            \
  struct PointerLikeTypeTraits<CLASS *> :                                \
    public MoreAlignedPointerTraits<CLASS, ALIGNMENT> {};                \
  }

#define LLVM_DECLARE_TEMPLATE_TYPE_ALIGNMENT_1(CLASS, ALIGNMENT)         \
  namespace llvm {                                                       \
  template <typename Arg0>                                               \
  struct PointerLikeTypeTraits<CLASS<Arg0> *> :                          \
    public MoreAlignedPointerTraits<CLASS<Arg0>, ALIGNMENT> {};          \
  }

LLVM_DECLARE_TYPE_ALIGNMENT(w2n::Decl, w2n::DeclAlignInBits)

LLVM_DECLARE_TEMPLATE_TYPE_ALIGNMENT_1(
  w2n::ASTAllocated, w2n::ASTAllocatedAlignInBits
)
LLVM_DECLARE_TYPE_ALIGNMENT(w2n::ASTContext, w2n::ASTContextAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(w2n::DeclContext, w2n::DeclContextAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(w2n::FileUnit, w2n::DeclContextAlignInBits)

static_assert(alignof(void *) >= 2, "pointer alignment is too small");

#endif // W2N_AST_POINTERLIKETRAITS_H
