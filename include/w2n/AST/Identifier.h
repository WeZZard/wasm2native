#ifndef W2N_AST_IDENTIFIER_H
#define W2N_AST_IDENTIFIER_H

#include <llvm/ADT/StringRef.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {

class Identifier {
  friend class ASTContext;

private:
  const char * Pointer;

public:
  enum : size_t {
    NumLowBitsAvailable = 3,
    RequiredAlignment = 1 << NumLowBitsAvailable,
    SpareBitMask = ((intptr_t)1 << NumLowBitsAvailable) - 1
  };

private:
  /**
   * @brief Constructor, only accessible by \c ASTContext, which handles
   * the uniquing.
   *
   * @param Ptr
   */
  explicit Identifier(const char * Ptr) : Pointer(Ptr) {
    assert(
      ((uintptr_t)Ptr & SpareBitMask) == 0 &&
      "Identifier pointer does not use any spare bits"
    );
  }

  /**
   * @brief A type with the alignment expected of a valid
   *  \c Identifier::Pointer .
   *
   */
  struct alignas(uint64_t) Aligner {};

  static_assert(
    alignof(Aligner) >= RequiredAlignment,
    "Identifier table will provide enough spare bits"
  );

public:
  explicit Identifier() : Pointer(nullptr) {
  }

  const char * get() const {
    return Pointer;
  }

  StringRef str() const {
    return Pointer;
  }

  /**
   * @brief Forbids implicit \c std::string conversion.
   *
   * @return std::string
   */
  explicit operator std::string() const {
    return std::string(Pointer);
  }

  unsigned getLength() const {
    assert(
      Pointer != nullptr && "Tried getting length of empty identifier"
    );
    return ::strlen(Pointer);
  }

  bool empty() const {
    return Pointer == nullptr;
  }

  bool is(StringRef string) const {
    return str().equals(string);
  }

  const void * getAsOpaquePointer() const {
    return static_cast<const void *>(Pointer);
  }

  static Identifier getFromOpaquePointer(void * P) {
    return Identifier((const char *)P);
  }

  /**
   * @brief Compare two identifiers, producing \c -1 if \c *this comes
   *  before \c other, \c 1 if \c *this comes after \c other, and \c 0 if
   *  they are equal.
   *
   * @param other
   * @return int
   *
   * @note Null identifiers come after all other identifiers.
   */
  int compare(Identifier other) const;

  friend llvm::hash_code hash_value(Identifier ident) {
    return llvm::hash_value(ident.getAsOpaquePointer());
  }

  bool operator==(Identifier RHS) const {
    return Pointer == RHS.Pointer;
  }

  bool operator!=(Identifier RHS) const {
    return !(*this == RHS);
  }

  bool operator<(Identifier RHS) const {
    return Pointer < RHS.Pointer;
  }

  static Identifier getEmptyKey() {
    uintptr_t Val = static_cast<uintptr_t>(-1);
    Val <<= NumLowBitsAvailable;
    return Identifier((const char *)Val);
  }

  static Identifier getTombstoneKey() {
    uintptr_t Val = static_cast<uintptr_t>(-2);
    Val <<= NumLowBitsAvailable;
    return Identifier((const char *)Val);
  }
};

using DeclBaseName = Identifier;

} // namespace w2n

namespace llvm {
raw_ostream& operator<<(raw_ostream& OS, w2n::Identifier I);

// Identifiers hash just like pointers.
template <>
struct DenseMapInfo<w2n::Identifier> {
  static w2n::Identifier getEmptyKey() {
    return w2n::Identifier::getEmptyKey();
  }

  static w2n::Identifier getTombstoneKey() {
    return w2n::Identifier::getTombstoneKey();
  }

  static unsigned getHashValue(w2n::Identifier Val) {
    return DenseMapInfo<const void *>::getHashValue(Val.get());
  }

  static bool isEqual(w2n::Identifier LHS, w2n::Identifier RHS) {
    return LHS == RHS;
  }
};

// An Identifier is "pointer like".
template <typename T>
struct PointerLikeTypeTraits;

template <>
struct PointerLikeTypeTraits<w2n::Identifier> {
public:
  static inline void * getAsVoidPointer(w2n::Identifier I) {
    return const_cast<void *>(I.getAsOpaquePointer());
  }

  static inline w2n::Identifier getFromVoidPointer(void * P) {
    return w2n::Identifier::getFromOpaquePointer(P);
  }

  enum {
    NumLowBitsAvailable = w2n::Identifier::NumLowBitsAvailable
  };
};

} // namespace llvm

#endif // W2N_AST_IDENTIFIER_H