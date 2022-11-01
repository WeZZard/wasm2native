#ifndef W2N_AST_ASTALLOCATED_H
#define W2N_AST_ASTALLOCATED_H

#include <cassert>
#include <cstddef>
#include <w2n/AST/PointerLikeTraits.h>

namespace w2n {
class ASTContext;

/**
 * @brief The arena in which a particular ASTContext allocation will go.
 *
 */
enum class AllocationArena {
  /**
   * @brief The permanent arena, which is tied to the lifetime of the
   * \c ASTContext.
   *
   * @note All global declarations and types need to be allocated into
   * this arena. At present, everything that is not a type involving a
   * type variable is allocated in this arena.
   */
  Permanent,
};

namespace detail {
void * allocateInASTContext(
  size_t bytes,
  const ASTContext& ctx,
  AllocationArena arena,
  unsigned alignment
);
}

/**
 * @brief Types inheriting from this class are intended to be allocated in
 * an \c ASTContext allocator; you cannot allocate them by using a normal
 * \c new , and instead you must either provide an \c ASTContext or use a
 * placement \c new .
 *
 * @tparam AlignTy The desired alignment type. It is usually, but not
 * always, the type that is inheriting \c ASTAllocated.
 */
template <typename AlignTy>
class LLVM_POINTER_LIKE_ALIGNMENT(ASTAllocated) ASTAllocated {
public:

  // Make vanilla new/delete illegal.

  void * operator new(size_t Bytes) throw() = delete;

  void operator delete(void * Data) throw() = delete;

  // Only allow allocation using the allocator in ASTContext or by doing a
  // placement new.

  void * operator new(
    size_t bytes,
    const ASTContext& ctx,
    AllocationArena arena = AllocationArena::Permanent,
    unsigned alignment = alignof(AlignTy)
  ) {
    return detail::allocateInASTContext(bytes, ctx, arena, alignment);
  }

  void * operator new(size_t Bytes, void * Mem) throw() {
    assert(Mem && "placement new into failed allocation");
    return Mem;
  }
};

} // namespace w2n

#endif
