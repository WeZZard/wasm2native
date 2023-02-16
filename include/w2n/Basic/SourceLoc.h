
#ifndef W2N_BASIC_SOURCELOC_H
#define W2N_BASIC_SOURCELOC_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/SMLoc.h>
#include <functional>
#include <w2n/Basic/Debug.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LLVMHashing.h>

namespace w2n {
class SourceManager;

/// SourceLoc in w2n is just an SMLoc.  We define it as a different type
/// (instead of as a typedef) just to remove the "getFromPointer" methods
/// and enforce purity in the w2n codebase.
class SourceLoc {
  friend class SourceManager;
  friend class SourceRange;
  friend class CharSourceRange;
  friend class DiagnosticConsumer;

  llvm::SMLoc Value;

public:

  SourceLoc() {
  }

  explicit SourceLoc(llvm::SMLoc Value) : Value(Value) {
  }

  bool isValid() const {
    return Value.isValid();
  }

  bool isInvalid() const {
    return !isValid();
  }

  /// An explicit bool operator so one can check if a SourceLoc is valid
  /// in an if statement:
  ///
  /// if (auto x = getSourceLoc()) { ... }
  explicit operator bool() const {
    return isValid();
  }

  bool operator==(const SourceLoc& RHS) const {
    return RHS.Value == Value;
  }

  bool operator!=(const SourceLoc& RHS) const {
    return !operator==(RHS);
  }

  /// Return a source location advanced a specified number of bytes.
  SourceLoc getAdvancedLoc(int ByteOffset) const {
    assert(isValid() && "Can't advance an invalid location");
    return SourceLoc(
      llvm::SMLoc::getFromPointer(Value.getPointer() + ByteOffset)
    );
  }

  SourceLoc getAdvancedLocOrInvalid(int ByteOffset) const {
    if (isValid()) {
      return getAdvancedLoc(ByteOffset);
    }
    return SourceLoc();
  }

  const void * getOpaquePointerValue() const {
    return Value.getPointer();
  }

  /// Print out the SourceLoc.  If this location is in the same buffer
  /// as specified by \c LastBufferID, then we don't print the filename.
  /// If not, we do print the filename, and then update \c LastBufferID
  /// with the BufferID printed.
  void print(
    raw_ostream& OS, const SourceManager& SM, unsigned& LastBufferID
  ) const;

  void printLineAndColumn(
    raw_ostream& OS, const SourceManager& SM, unsigned BufferID = 0
  ) const;

  void print(raw_ostream& OS, const SourceManager& SM) const {
    unsigned Tmp = ~0U;
    print(OS, SM, Tmp);
  }

  W2N_DEBUG_DUMPER(dump(const SourceManager& SM));

  friend size_t hash_value(SourceLoc Loc) {
    return reinterpret_cast<uintptr_t>(Loc.getOpaquePointerValue());
  }

  friend void simple_display(raw_ostream& os, const SourceLoc& ss) {
    // Nothing meaningful to print.
  }
};

/// SourceRange in w2n is a pair of locations.  However, note that the end
/// location is the start of the last token in the range, not the last
/// character in the range.  This is unlike SMRange, so we use a distinct
/// type to make sure that proper conversions happen where important.
class SourceRange {
public:

  SourceLoc Start, End;

  SourceRange() {
  }

  SourceRange(SourceLoc Loc) : Start(Loc), End(Loc) {
  }

  SourceRange(SourceLoc Start, SourceLoc End) : Start(Start), End(End) {
    assert(
      Start.isValid() == End.isValid()
      && "Start and end should either both be valid or both be invalid!"
    );
  }

  bool isValid() const {
    return Start.isValid();
  }

  bool isInvalid() const {
    return !isValid();
  }

  /// An explicit bool operator so one can check if a SourceRange is valid
  /// in an if statement:
  ///
  /// if (auto x = getSourceRange()) { ... }
  explicit operator bool() const {
    return isValid();
  }

  /// Extend this SourceRange to the smallest continuous SourceRange that
  /// includes both this range and the other one.
  void widen(SourceRange Other);

  /// Checks whether this range contains the given location. Note that the
  /// given location should correspond to the start of a token, since
  /// locations inside the last token may be considered outside the range
  /// by this function.
  bool contains(SourceLoc Loc) const;

  /// Checks whether this range overlaps with the given range.
  bool overlaps(SourceRange Other) const;

  bool operator==(const SourceRange& Other) const {
    return Start == Other.Start && End == Other.End;
  }

  bool operator!=(const SourceRange& Other) const {
    return !operator==(Other);
  }

  /// Print out the SourceRange.  If the locations are in the same buffer
  /// as specified by LastBufferID, then we don't print the filename.  If
  /// not, we do print the filename, and then update LastBufferID with the
  /// BufferID printed.
  void print(
    raw_ostream& OS,
    const SourceManager& SM,
    unsigned& LastBufferID,
    bool PrintText = true
  ) const;

  void print(
    raw_ostream& OS, const SourceManager& SM, bool PrintText = true
  ) const {
    unsigned Tmp = ~0U;
    print(OS, SM, Tmp, PrintText);
  }

  W2N_DEBUG_DUMPER(dump(const SourceManager& SM));
};

/// A half-open character-based source range.
class CharSourceRange {
  SourceLoc Start;
  unsigned ByteLength;

public:

  /// Constructs an invalid range.
  CharSourceRange() = default;

  CharSourceRange(SourceLoc Start, unsigned ByteLength) :
    Start(Start),
    ByteLength(ByteLength) {
  }

  /// Constructs a character range which starts and ends at the
  /// specified character locations.
  CharSourceRange(
    const SourceManager& SM, SourceLoc Start, SourceLoc End
  );

  /// Use Lexer::getCharSourceRangeFromSourceRange() instead.
  CharSourceRange(const SourceManager& SM, SourceRange Range) = delete;

  bool isValid() const {
    return Start.isValid();
  }

  bool isInvalid() const {
    return !isValid();
  }

  bool operator==(const CharSourceRange& Other) const {
    return Start == Other.Start && ByteLength == Other.ByteLength;
  }

  bool operator!=(const CharSourceRange& Other) const {
    return !operator==(Other);
  }

  SourceLoc getStart() const {
    return Start;
  }

  SourceLoc getEnd() const {
    return Start.getAdvancedLocOrInvalid(ByteLength);
  }

  /// Returns true if the given source location is contained in the range.
  bool contains(SourceLoc Loc) const {
    auto Less = std::less<const char *>();
    auto LessEqual = std::less_equal<const char *>();
    return LessEqual(
             getStart().Value.getPointer(), Loc.Value.getPointer()
           )
        && Less(Loc.Value.getPointer(), getEnd().Value.getPointer());
  }

  bool contains(CharSourceRange Other) const {
    auto LessEqual = std::less_equal<const char *>();
    return contains(Other.getStart())
        && LessEqual(
             Other.getEnd().Value.getPointer(),
             getEnd().Value.getPointer()
        );
  }

  /// expands *this to cover Other
  void widen(CharSourceRange Other) {
    auto Diff =
      Other.getEnd().Value.getPointer() - getEnd().Value.getPointer();
    if (Diff > 0) {
      ByteLength += Diff;
    }
    const char * const MyStartPtr = getStart().Value.getPointer();
    Diff = MyStartPtr - Other.getStart().Value.getPointer();
    if (Diff > 0) {
      ByteLength += Diff;
      Start = SourceLoc(llvm::SMLoc::getFromPointer(MyStartPtr - Diff));
    }
  }

  bool overlaps(CharSourceRange Other) const {
    if (getByteLength() == 0 || Other.getByteLength() == 0) {
      return false;
    }
    return contains(Other.getStart()) || Other.contains(getStart());
  }

  StringRef str() const {
    return StringRef(Start.Value.getPointer(), ByteLength);
  }

  /// Return the length of this valid range in bytes.  Can be zero.
  unsigned getByteLength() const {
    assert(
      isValid() && "length does not make sense for an invalid range"
    );
    return ByteLength;
  }

  /// Print out the CharSourceRange.  If the locations are in the same
  /// buffer as specified by LastBufferID, then we don't print the
  /// filename.  If not, we do print the filename, and then update
  /// LastBufferID with the BufferID printed.
  void print(
    raw_ostream& OS,
    const SourceManager& SM,
    unsigned& LastBufferID,
    bool PrintText = true
  ) const;

  void print(
    raw_ostream& OS, const SourceManager& SM, bool PrintText = true
  ) const {
    unsigned Tmp = ~0U;
    print(OS, SM, Tmp, PrintText);
  }

  W2N_DEBUG_DUMPER(dump(const SourceManager& SM));
};

} // end namespace w2n

namespace llvm {

template <>
struct DenseMapInfo<w2n::SourceLoc> {
  static w2n::SourceLoc getEmptyKey() {
    return w2n::SourceLoc(
      SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey())
    );
  }

  static w2n::SourceLoc getTombstoneKey() {
    // Make this different from empty key. See for context:
    // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
    return w2n::SourceLoc(
      SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey())
    );
  }

  static unsigned getHashValue(const w2n::SourceLoc& Val) {
    return DenseMapInfo<const void *>::getHashValue(
      Val.getOpaquePointerValue()
    );
  }

  static bool
  isEqual(const w2n::SourceLoc& LHS, const w2n::SourceLoc& RHS) {
    return LHS == RHS;
  }
};

template <>
struct DenseMapInfo<w2n::SourceRange> {
  static w2n::SourceRange getEmptyKey() {
    return w2n::SourceRange(w2n::SourceLoc(
      SMLoc::getFromPointer(DenseMapInfo<const char *>::getEmptyKey())
    ));
  }

  static w2n::SourceRange getTombstoneKey() {
    // Make this different from empty key. See for context:
    // http://lists.llvm.org/pipermail/llvm-dev/2015-July/088744.html
    return w2n::SourceRange(w2n::SourceLoc(
      SMLoc::getFromPointer(DenseMapInfo<const char *>::getTombstoneKey())
    ));
  }

  static unsigned getHashValue(const w2n::SourceRange& Val) {
    return hash_combine(
      Val.Start.getOpaquePointerValue(), Val.End.getOpaquePointerValue()
    );
  }

  static bool
  isEqual(const w2n::SourceRange& LHS, const w2n::SourceRange& RHS) {
    return LHS == RHS;
  }
};
} // namespace llvm

#endif // W2N_BASIC_SOURCELOC_H
