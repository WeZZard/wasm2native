//  This file provides an implementation of C11 aligned_alloc(3) for
//  platforms that don't have it yet, using posix_memalign(3).

#ifndef W2N_BASIC_MALLOC_H
#define W2N_BASIC_MALLOC_H

#include <cassert>
#if defined(_WIN32)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace w2n {

inline void * AlignedAlloc(size_t size, size_t align) {
  void * r = aligned_alloc(align, size);
  assert(r != nullptr && "aligned_alloc failed");
  return r;
}

inline void AlignedFree(void * p) { free(p); }

} // end namespace w2n

#endif // W2N_BASIC_MALLOC_H
