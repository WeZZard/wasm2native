#ifndef W2N_BASIC_OPTIONSET_H
#define W2N_BASIC_OPTIONSET_H

#include <llvm/ADT/None.h>
#include <cstdint>
#include <initializer_list>
#include <type_traits>

namespace w2n {

using llvm::None;

/// The class template \c OptionSet captures a set of options stored as
/// the bits in an unsigned integral value.
///
/// Each option corresponds to a particular flag value in the provided
/// enumeration type (\c Flags). The option set provides ways to add
/// options, remove options, intersect sets, etc., providing a thin
/// type-safe layer over the underlying unsigned value.
///
/// \tparam Flags An enumeration type that provides the individual flags
/// for options. Each enumerator should have a power-of-two value,
/// indicating which bit it is associated with.
///
/// \tparam StorageType The unsigned integral type to use to store the
/// flags enabled within this option set. This defaults to the unsigned
/// form of the underlying type of the enumeration.
template <
  typename Flags,
  typename StorageType = typename std::make_unsigned<
    typename std::underlying_type<Flags>::type>::type>
class OptionSet {
  StorageType Storage;

public:

  /// Create an empty option set.
  constexpr OptionSet() : Storage() {
  }

  /// Create an empty option set.
  constexpr OptionSet(llvm::NoneType /*unused*/) : Storage() {
  }

  /// Create an option set with only the given option set.
  constexpr OptionSet(Flags Flag) :
    Storage(static_cast<StorageType>(Flag)) {
  }

  /// Create an option set containing the given options.
  constexpr OptionSet(std::initializer_list<Flags> F) :
    Storage(combineFlags(F)) {
  }

  /// Create an option set from raw storage.
  explicit constexpr OptionSet(StorageType Storage) : Storage(Storage) {
  }

  /// Check whether an option set is non-empty.
  explicit constexpr operator bool() const {
    return Storage != 0;
  }

  /// Explicitly convert an option set to its underlying storage.
  explicit constexpr operator StorageType() const {
    return Storage;
  }

  /// Explicitly convert an option set to intptr_t, for use in
  /// llvm::PointerIntPair.
  ///
  /// This member is not present if the underlying type is bigger than
  /// a pointer.
  template <typename T = std::intptr_t>
  explicit constexpr operator typename std::enable_if<
    sizeof(StorageType) <= sizeof(T),
    std::intptr_t>::type() const {
    return static_cast<intptr_t>(Storage);
  }

  /// Retrieve the "raw" representation of this option set.
  StorageType toRaw() const {
    return Storage;
  }

  /// Determine whether this option set contains all of the options in the
  /// given set.
  constexpr bool contains(OptionSet Set) const {
    return !static_cast<bool>(Set - *this);
  }

  /// Check if this option set contains the exact same options as the
  /// given set.
  constexpr bool containsOnly(OptionSet Set) const {
    return Storage == Set.Storage;
  }

  // '==' and '!=' are deliberately not defined because they provide a
  // pitfall where someone might use '==' but really want 'contains'. If
  // you actually want '==' behavior, use 'containsOnly'.

  /// Produce the union of two option sets.
  friend constexpr OptionSet operator|(OptionSet Lhs, OptionSet Rhs) {
    return OptionSet(Lhs.Storage | Rhs.Storage);
  }

  /// Produce the union of two option sets.
  friend constexpr OptionSet& operator|=(OptionSet& Lhs, OptionSet Rhs) {
    Lhs.Storage |= Rhs.Storage;
    return Lhs;
  }

  /// Produce the intersection of two option sets.
  friend constexpr OptionSet operator&(OptionSet Lhs, OptionSet Rhs) {
    return OptionSet(Lhs.Storage & Rhs.Storage);
  }

  /// Produce the intersection of two option sets.
  friend constexpr OptionSet& operator&=(OptionSet& Lhs, OptionSet Rhs) {
    Lhs.Storage &= Rhs.Storage;
    return Lhs;
  }

  /// Produce the difference of two option sets.
  friend constexpr OptionSet operator-(OptionSet Lhs, OptionSet Rhs) {
    return OptionSet(Lhs.Storage & ~Rhs.Storage);
  }

  /// Produce the difference of two option sets.
  friend constexpr OptionSet& operator-=(OptionSet& Lhs, OptionSet Rhs) {
    Lhs.Storage &= ~Rhs.Storage;
    return Lhs;
  }

private:

  template <typename T>
  static auto checkResultTypeOperatorOr(T Val) -> decltype(Val | Val) {
    return T();
  }

  static void checkResultTypeOperatorOr(...) {
  }

  static constexpr StorageType
  combineFlags(const std::initializer_list<Flags>& F) {
    OptionSet Result;
    for (Flags Flag : F) {
      Result |= Flag;
    }
    return Result.Storage;
  }

  static_assert(
    !std::is_same<decltype(checkResultTypeOperatorOr(Flags())), Flags>::
      value,
    "operator| should produce an OptionSet"
  );
};

} // end namespace w2n

#endif // W2N_BASIC_OPTIONSET_H
