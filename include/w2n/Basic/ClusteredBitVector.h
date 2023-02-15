//===--- ClusteredBitVector.h - Size-optimized bit vector -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
// This file defines the ClusteredBitVector class, a bitset data
// structure.
//
// For example, this would be reasonable to use to describe the
// unoccupied bits in a memory layout.
//
// Primary mutators:
//   - appending another vector to this vector
//   - appending a constant vector (<0,0,...,0> or <1,1,...,1>) to this
//   vector
//
// Primary observers:
//   - testing a specific bit
//   - converting to llvm::APInt
//
//===----------------------------------------------------------------===//

#ifndef W2N_BASIC_CLUSTEREDBITVECTOR_H
#define W2N_BASIC_CLUSTEREDBITVECTOR_H

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/Optional.h>
#include <cassert>
#include <w2n/Basic/Debug.h>

namespace w2n {

/// A vector of bits.
class ClusteredBitVector {
  using APInt = llvm::APInt;

  /// Represents the bit vector as an integer.
  /// The least-significant bit of the integer corresponds to the bit
  /// at index 0. If the optional does not have a value then the bit
  /// vector has a length of 0 bits.
  llvm::Optional<APInt> Bits;

  /// Copy constructor from APInt.
  ClusteredBitVector(const APInt& Bits) : Bits(Bits) {
  }

  /// Move constructor from APInt.
  ClusteredBitVector(APInt&& Bits) : Bits(std::move(Bits)) {
  }

public:

  /// Create a new bit vector of zero length.  This does not perform
  /// any allocations.
  ClusteredBitVector() = default;

  ClusteredBitVector(const ClusteredBitVector& X) : Bits(X.Bits) {
  }

  ClusteredBitVector(ClusteredBitVector&& X) : Bits(std::move(X.Bits)) {
  }

  /// Create a new ClusteredBitVector from the provided APInt,
  /// with a size of 0 if the optional does not have a value.
  ClusteredBitVector(const llvm::Optional<APInt>& X) : Bits(X) {
  }

  /// Create a new ClusteredBitVector from the provided APInt,
  /// with a size of 0 if the optional does not have a value.
  ClusteredBitVector(llvm::Optional<APInt>&& X) : Bits(std::move(X)) {
  }

  ClusteredBitVector& operator=(const ClusteredBitVector& X) {
    this->Bits = X.Bits;
    return *this;
  }

  ClusteredBitVector& operator=(ClusteredBitVector&& X) {
    this->Bits = std::move(X.Bits);
    return *this;
  }

  ~ClusteredBitVector() = default;

  /// Return true if this vector is zero-length (*not* if it does not
  /// contain any set bits).
  bool empty() const {
    return !Bits.has_value();
  }

  /// Return the length of this bit-vector.
  size_t size() const {
    return Bits ? Bits.value().getBitWidth() : 0;
  }

  /// Append the bits from the given vector to this one.
  void append(const ClusteredBitVector& Other) {
    // Nothing to do if the other vector is empty.
    if (!Other.Bits) {
      return;
    }
    if (!Bits) {
      Bits = Other.Bits;
      return;
    }
    APInt& Val = Bits.value();
    unsigned Width = Val.getBitWidth();
    Val = Val.zext(Width + Other.Bits.value().getBitWidth());
    Val.insertBits(Other.Bits.value(), Width);
  }

  /// Add the low N bits from the given value to the vector.
  void add(size_t NumBits, uint64_t Value) {
    append(fromAPInt(APInt(NumBits, Value)));
  }

  /// Append a number of clear bits to this vector.
  void appendClearBits(size_t NumBits) {
    if (NumBits == 0) {
      return;
    }
    if (Bits) {
      APInt& Value = Bits.value();
      Value = Value.zext(Value.getBitWidth() + NumBits);
      return;
    }
    Bits = APInt::getNullValue(NumBits);
  }

  /// Extend the vector out to the given length with clear bits.
  void extendWithClearBits(size_t NewSize) {
    assert(NewSize >= size());
    appendClearBits(NewSize - size());
  }

  /// Append a number of set bits to this vector.
  void appendSetBits(size_t NumBits) {
    if (NumBits == 0) {
      return;
    }
    if (Bits) {
      APInt& Value = Bits.value();
      unsigned Width = Value.getBitWidth();
      Value = Value.zext(Width + NumBits);
      Value.setBitsFrom(Width);
      return;
    }
    Bits = APInt::getAllOnesValue(NumBits);
  }

  /// Extend the vector out to the given length with set bits.
  void extendWithSetBits(size_t NewSize) {
    assert(NewSize >= size());
    appendSetBits(NewSize - size());
  }

  /// Test whether a particular bit is set.
  bool operator[](size_t I) const {
    assert(I < size());
    return Bits.value()[I];
  }

  /// Intersect a bit-vector of the same size into this vector.
  ClusteredBitVector& operator&=(const ClusteredBitVector& X) {
    assert(size() == X.size());
    if (Bits) {
      APInt& Value = Bits.value();
      Value &= X.Bits.value();
    }
    return *this;
  }

  /// Union a bit-vector of the same size into this vector.
  ClusteredBitVector& operator|=(const ClusteredBitVector& Other) {
    assert(size() == Other.size());
    if (Bits) {
      APInt& Value = Bits.value();
      Value |= Other.Bits.value();
    }
    return *this;
  }

  /// Set bit i.
  void setBit(size_t I) {
    assert(I < size());
    Bits.value().setBit(I);
  }

  /// Clear bit i.
  void clearBit(size_t I) {
    assert(I < size());
    Bits.value().clearBit(I);
  }

  /// Toggle bit i.
  void flipBit(size_t I) {
    assert(I < size());
    Bits.value().flipBit(I);
  }

  /// Toggle all the bits in this vector.
  void flipAll() {
    if (Bits) {
      Bits.value().flipAllBits();
    }
  }

  /// Set the length of this vector to zero.
  void clear() {
    Bits.reset();
  }

  /// Count the number of set bits in this vector.
  size_t count() const {
    return Bits ? Bits.value().countPopulation() : 0;
  }

  /// Determine if there are any bits set in this vector.
  bool any() const {
    return Bits && Bits.value() != 0;
  }

  /// Determine if there are no bits set in this vector.
  ///
  /// \return \c !any()
  bool none() const {
    return !any();
  }

  friend bool operator==(
    const ClusteredBitVector& Lhs, const ClusteredBitVector& Rhs
  ) {
    if (Lhs.size() != Rhs.size()) {
      return false;
    }
    if (Lhs.size() == 0 /* NOLINT(readability-container-size-empty) */) {
      return true;
    }
    return Lhs.Bits.value() == Rhs.Bits.value();
  }

  friend bool operator!=(
    const ClusteredBitVector& Lhs, const ClusteredBitVector& Rhs
  ) {
    return !(Lhs == Rhs);
  }

  /// Return this bit-vector as an APInt, with low indices becoming
  /// the least significant bits of the number.
  APInt asAPInt() const {
    // Return 1-bit wide zero APInt for a 0-bit vector.
    return Bits ? Bits.value() : APInt();
  }

  /// Construct a bit-vector from an APInt.
  static ClusteredBitVector fromAPInt(const APInt& Value) {
    return ClusteredBitVector(Value);
  }

  /// Construct a bit-vector from an APInt.
  static ClusteredBitVector fromAPInt(APInt&& Value) {
    return ClusteredBitVector(std::move(Value));
  }

  /// Return a constant bit-vector of the given size.
  static ClusteredBitVector getConstant(size_t NumBits, bool Value) {
    if (NumBits == 0) {
      return ClusteredBitVector();
    }
    auto Vec = APInt::getNullValue(NumBits);
    if (Value) {
      Vec.flipAllBits();
    }
    return ClusteredBitVector(Vec);
  }

  /// Pretty-print the vector.
  void print(llvm::raw_ostream& os) const;

  W2N_DEBUG_DUMP;
};

} // namespace w2n

#endif // W2N_BASIC_CLUSTEREDBITVECTOR_H
