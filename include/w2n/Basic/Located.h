// Provides a currency data type Located<T> that should be used instead
// of std::pair<T, SourceLoc>.

#ifndef W2N_BASIC_LOCATED_H
#define W2N_BASIC_LOCATED_H

#include <w2n/Basic/Debug.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SourceLoc.h>

namespace w2n {

/// A currency type for keeping track of items which were found in the
/// source code. Several parts of the compiler need to keep track of a
/// `SourceLoc` corresponding to an item, in case they need to report some
/// diagnostics later. For example, the ClangImporter needs to keep track
/// of where imports were originally written. Located makes it easy to do
/// so while making the code more readable, compared to using `std::pair`.
template <typename T>
struct Located {
  /// The main item whose source location is being tracked.
  T Item;

  /// The original source location from which the item was parsed.
  SourceLoc Loc;

  Located() : Item() {
  }

  Located(T Item, SourceLoc Loc) : Item(Item), Loc(Loc) {
  }

  W2N_DEBUG_DUMP;
  void dump(raw_ostream& os) const;
};

template <typename T>
bool operator==(const Located<T>& Lhs, const Located<T>& Rhs) {
  return Lhs.Item == Rhs.Item && Lhs.Loc == Rhs.Loc;
}

} // end namespace w2n

namespace llvm {

template <typename T, typename Enable>
struct DenseMapInfo;

template <typename T>
struct DenseMapInfo<w2n::Located<T>> {
  static inline w2n::Located<T> getEmptyKey() {
    return w2n::Located<T>(
      DenseMapInfo<T>::getEmptyKey(),
      DenseMapInfo<w2n::SourceLoc>::getEmptyKey()
    );
  }

  static inline w2n::Located<T> getTombstoneKey() {
    return w2n::Located<T>(
      DenseMapInfo<T>::getTombstoneKey(),
      DenseMapInfo<w2n::SourceLoc>::getTombstoneKey()
    );
  }

  static unsigned getHashValue(const w2n::Located<T>& LocatedVal) {
    return combineHashValue(
      DenseMapInfo<T>::getHashValue(LocatedVal.Item),
      DenseMapInfo<w2n::SourceLoc>::getHashValue(LocatedVal.Loc)
    );
  }

  static bool
  isEqual(const w2n::Located<T>& LHS, const w2n::Located<T>& RHS) {
    return DenseMapInfo<T>::isEqual(LHS.Item, RHS.Item)
        && DenseMapInfo<T>::isEqual(LHS.Loc, RHS.Loc);
  }
};
} // namespace llvm

#endif // W2N_BASIC_LOCATED_H
