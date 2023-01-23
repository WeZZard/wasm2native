//===--- WasmTargetInfo.h ---------------------------------*- C++ -*-===//
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
// This file declares the WasmTargetInfo abstract base class. This class
// provides an interface to target-dependent attributes of interest to
// Swift.
//
//===----------------------------------------------------------------===//

#ifndef WASM_IRGEN_WASMTARGETINFO_H
#define WASM_IRGEN_WASMTARGETINFO_H

#include "IRGenInternal.h"
#include <llvm/ADT/Triple.h>
#include <w2n/Basic/ClusteredBitVector.h>
#include <w2n/Basic/LLVM.h>

namespace w2n {
namespace irgen {
class IRGenModule;

class WasmTargetInfo {
  explicit WasmTargetInfo(
    llvm::Triple::ObjectFormatType outputObjectFormat,
    unsigned numPointerBits
  );

public:

  /// Produces a SwiftTargetInfo object appropriate to the target.
  static WasmTargetInfo get(IRGenModule& IGM);

  /// The target's object format type.
  llvm::Triple::ObjectFormatType OutputObjectFormat;

  /// The spare bit mask for pointers. Bits set in this mask are unused by
  /// pointers of any alignment.
  SpareBitVector PointerSpareBits;

  /// The spare bit mask for (ordinary C) thin function pointers.
  SpareBitVector FunctionPointerSpareBits;

  /// The alignment of heap objects.  By default, assume pointer
  /// alignment.
  Alignment HeapObjectAlignment;

  /// The least integer value that can theoretically form a valid pointer.
  /// By default, assume that there's an entire page free.
  ///
  /// This excludes addresses in the null page(s) guaranteed to be
  /// unmapped by the platform.
  ///
  /// Changes to this must be kept in sync with swift/Runtime/Metadata.h.
  uint64_t LeastValidPointerValue;

  /// Poison sentinel value recognized by LLDB as a former reference to a
  /// potentially deinitialized object. It uses no spare bits and cannot
  /// point to readable memory.
  uint64_t ReferencePoisonDebugValue;

  /// The maximum number of scalars that we allow to be returned directly.
  unsigned MaxScalarsForDirectResult = 3;

  /// The value stored in a Builtin.once predicate to indicate that an
  /// initialization has already happened, if known.
  Optional<int64_t> OnceDonePredicateValue = None;
};

} // namespace irgen
} // namespace w2n

#endif
