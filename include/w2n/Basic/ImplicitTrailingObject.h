#ifndef W2N_BASIC_ALLOCATION_H
#define W2N_BASIC_ALLOCATION_H

#include <llvm/Support/Alignment.h>
#include <w2n/Basic/Malloc.h>

namespace w2n {

template <class BodyTy, class TrailingObjectTy>
inline std::pair<BodyTy *, TrailingObjectTy *>
alignedAllocWithImplicitTrailingObject() {
  // If more than two data structures are concatentated, then the
  // aggregate size math needs to become more complicated due to
  // per-struct alignment constraints.
  size_t Align = std::max(alignof(BodyTy), alignof(TrailingObjectTy));
  uint64_t AlignedSize =
    llvm::alignTo(sizeof(BodyTy) + sizeof(TrailingObjectTy), Align);
  BodyTy * Body =
    reinterpret_cast<BodyTy *>(alignedAlloc(AlignedSize, Align));
  TrailingObjectTy * TrailingObject =
    reinterpret_cast<TrailingObjectTy *>((char *)Body + sizeof(BodyTy));
  TrailingObject = reinterpret_cast<TrailingObjectTy *>(llvm::alignAddr(
    TrailingObject, llvm::Align(alignof(TrailingObjectTy))
  ));
  return std::make_pair(Body, TrailingObject);
}

template <class BodyTy, class TrailingObjectTy>
inline TrailingObjectTy& getImplicitTrailingObject(BodyTy * Body) {
  return getImplicitTrailingObject<BodyTy, TrailingObjectTy>(
    const_cast<const BodyTy *>(Body)
  );
}

template <class BodyTy, class TrailingObjectTy>
inline TrailingObjectTy& getImplicitTrailingObject(const BodyTy * Body) {
  auto * Pointer = reinterpret_cast<char *>(const_cast<BodyTy *>(Body));
  auto Offset = llvm::alignAddr(
    (void *)sizeof(*Body), llvm::Align(alignof(TrailingObjectTy))
  );
  return *reinterpret_cast<TrailingObjectTy *>(Pointer + Offset);
}

} // namespace w2n

#endif // W2N_BASIC_ALLOCATION_H
