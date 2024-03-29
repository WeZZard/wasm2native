/// This file defines and implements the \c NullablePtr class.

#ifndef W2N_BASIC_NULLABLEPTR_H
#define W2N_BASIC_NULLABLEPTR_H

#include <llvm/Support/PointerLikeTypeTraits.h>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace w2n {
/// NullablePtr pointer wrapper - NullablePtr is used for APIs where a
/// potentially-null pointer gets passed around that must be explicitly
/// handled in lots of places.  By putting a wrapper around the null
/// pointer, it makes it more likely that the null pointer case will be
/// handled correctly.
template <class T>
class NullablePtr {
  T * Ptr;

  struct PlaceHolder {};

public:

  NullablePtr(T * P = 0) : Ptr(P) {
  }

  template <typename OtherT>
  NullablePtr(
    NullablePtr<OtherT> Other,
    typename std::enable_if<
      std::is_convertible<OtherT *, T *>::value,
      PlaceHolder>::type /*unused*/
    = PlaceHolder()
  ) :
    Ptr(Other.getPtrOrNull()) {
  }

  bool isNull() const {
    return Ptr == 0;
  }

  bool isNonNull() const {
    return Ptr != 0;
  }

  /// get - Return the pointer if it is non-null.
  const T * get() const {
    assert(Ptr && "Pointer wasn't checked for null!");
    return Ptr;
  }

  /// get - Return the pointer if it is non-null.
  T * get() {
    assert(Ptr && "Pointer wasn't checked for null!");
    return Ptr;
  }

  T * getPtrOrNull() {
    return getPtrOr(nullptr);
  }

  const T * getPtrOrNull() const {
    return getPtrOr(nullptr);
  }

  T * getPtrOr(T * DefaultValue) {
    return Ptr ? Ptr : DefaultValue;
  }

  const T * getPtrOr(const T * DefaultValue) const {
    return Ptr ? Ptr : DefaultValue;
  }

  explicit operator bool() const {
    return Ptr;
  }

  bool operator==(const NullablePtr<T>& Other) const {
    return Other.Ptr == Ptr;
  }

  bool operator!=(const NullablePtr<T>& Other) const {
    return !(*this == Other);
  }

  bool operator==(const T * Other) const {
    return Other == Ptr;
  }

  bool operator!=(const T * Other) const {
    return !(*this == Other);
  }
};

} // end namespace w2n

namespace llvm {
template <typename T>
struct PointerLikeTypeTraits;

template <typename T>
struct PointerLikeTypeTraits<w2n::NullablePtr<T>> {
public:

  static inline void * getAsVoidPointer(w2n::NullablePtr<T> Ptr) {
    return static_cast<void *>(Ptr.getPtrOrNull());
  }

  static inline w2n::NullablePtr<T> getFromVoidPointer(void * Ptr) {
    return w2n::NullablePtr<T>(static_cast<T *>(Ptr));
  }

  enum {
    NumLowBitsAvailable = PointerLikeTypeTraits<T *>::NumLowBitsAvailable
  };
};

} // namespace llvm

#endif // W2N_BASIC_NULLABLEPTR_H
