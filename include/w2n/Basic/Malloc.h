//  This file provides an implementation of C11 aligned_alloc(3) for
//  platforms that don't have it yet, using posix_memalign(3).

#ifndef W2N_BASIC_MALLOC_H
#define W2N_BASIC_MALLOC_H

#include <__utility/pair.h>
#include <cassert>
#if defined(_WIN32)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace w2n {

inline void * alignedAlloc(size_t Size, size_t Align) {
  void * Resource = aligned_alloc(Align, Size);
  assert(Resource != nullptr && "aligned_alloc failed");
  return Resource;
}

inline void alignedFree(void * P) {
  free(P);
}

} // end namespace w2n

#endif // W2N_BASIC_MALLOC_H
