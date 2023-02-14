//===--- LLVMEquatable.h ----------------------------------*- C++ -*-===//
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
// This file defines overloads of == for LLVM types.
//
//===----------------------------------------------------------------===//

#ifndef W2N_BASIC_LLVMEQUATABLE_H
#define W2N_BASIC_LLVMEQUATABLE_H

#include <llvm/ADT/PointerUnion.h> // to define hash_value
#include <llvm/ADT/TinyPtrVector.h>
#include <w2n/Basic/TypeID.h>

namespace llvm {

template <typename T>
bool operator==(
  const TinyPtrVector<T>& Lhs, const TinyPtrVector<T>& Rhs
) {
  if (Lhs.size() != Rhs.size()) {
    return false;
  }

  for (unsigned I = 0, N = Lhs.size(); I != N; ++I) {
    if (Lhs[I] != Rhs[I]) {
      return false;
    }
  }

  return true;
}

template <typename T>
bool operator!=(
  const TinyPtrVector<T>& Lhs, const TinyPtrVector<T>& Rhs
) {
  return !(Lhs == Rhs);
}

} // end namespace llvm

#endif // W2N_BASIC_LLVMEQUATABLE_H
