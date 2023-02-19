//===--- WasmTargetInfo.cpp ----------------------------------------===//
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
// This file defines the WasmTargetInfo abstract base class. This class
// provides an interface to target-dependent attributes of interest to
// Swift.
//
//===----------------------------------------------------------------===//

#include "WasmTargetInfo.h"
#include "IRGenModule.h"
#include <llvm/ADT/Triple.h>
#include <llvm/IR/DataLayout.h>
#include <w2n/ABI/System.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/IRGenOptions.h>

using namespace w2n;
using namespace irgen;

/// Initialize a bit vector to be equal to the given bit-mask.
static void
setToMask(SpareBitVector& bits, unsigned size, uint64_t mask) {
  bits.clear();
  bits.add(size, mask);
}

/// Configures target-specific information for arm64 platforms.
static void configureARM64(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  if (triple.isAndroid()) {
    setToMask(
      target.PointerSpareBits,
      64,
      SWIFT_ABI_ANDROID_ARM64_SWIFT_SPARE_BITS_MASK
    );
  } else {
    setToMask(
      target.PointerSpareBits, 64, SWIFT_ABI_ARM64_SWIFT_SPARE_BITS_MASK
    );
  }

  if (triple.isOSDarwin()) {
    target.LeastValidPointerValue =
      SWIFT_ABI_DARWIN_ARM64_LEAST_VALID_POINTER;
  }
}

/// Configures target-specific information for arm64_32 platforms.
static void configureARM64_32(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 32, SWIFT_ABI_ARM_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for x86-64 platforms.
static void configureX86_64(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 64, SWIFT_ABI_X86_64_SWIFT_SPARE_BITS_MASK
  );

  if (triple.isOSDarwin()) {
    target.LeastValidPointerValue =
      SWIFT_ABI_DARWIN_X86_64_LEAST_VALID_POINTER;
  }
}

/// Configures target-specific information for 32-bit x86 platforms.
static void configureX86(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 32, SWIFT_ABI_I386_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for 32-bit arm platforms.
static void configureARM(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 32, SWIFT_ABI_ARM_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for powerpc platforms.
static void configurePowerPC(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 32, SWIFT_ABI_POWERPC_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for powerpc64 platforms.
static void configurePowerPC64(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 64, SWIFT_ABI_POWERPC64_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for SystemZ platforms.
static void configureSystemZ(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  setToMask(
    target.PointerSpareBits, 64, SWIFT_ABI_S390X_SWIFT_SPARE_BITS_MASK
  );
}

/// Configures target-specific information for wasm32 platforms.
static void configureWasm32(
  IRGenModule& IGM, const llvm::Triple& triple, WasmTargetInfo& target
) {
  target.LeastValidPointerValue = SWIFT_ABI_WASM32_LEAST_VALID_POINTER;
}

/// Configure a default target.
WasmTargetInfo::WasmTargetInfo(
  llvm::Triple::ObjectFormatType OutputObjectFormat,
  unsigned NumPointerBits
) :
  OutputObjectFormat(OutputObjectFormat),
  HeapObjectAlignment(NumPointerBits / 8),
  LeastValidPointerValue(SWIFT_ABI_DEFAULT_LEAST_VALID_POINTER) {
  setToMask(
    PointerSpareBits,
    NumPointerBits,
    SWIFT_ABI_DEFAULT_SWIFT_SPARE_BITS_MASK
  );
  setToMask(
    FunctionPointerSpareBits,
    NumPointerBits,
    SWIFT_ABI_DEFAULT_FUNCTION_SPARE_BITS_MASK
  );
  if (NumPointerBits == 64) {
    ReferencePoisonDebugValue =
      SWIFT_ABI_DEFAULT_REFERENCE_POISON_DEBUG_VALUE_64;
  } else {
    ReferencePoisonDebugValue =
      SWIFT_ABI_DEFAULT_REFERENCE_POISON_DEBUG_VALUE_32;
  }
}

WasmTargetInfo WasmTargetInfo::get(IRGenModule& IGM) {
  const llvm::Triple& triple = IGM.Context.LangOpts.Target;
  auto pointerSize = IGM.DataLayout.getPointerSizeInBits();

  // Prepare generic target information.
  WasmTargetInfo target(triple.getObjectFormat(), pointerSize);

  // On Apple platforms, we implement "once" using dispatch_once,
  // which exposes a barrier-free inline path with -1 as the "done" value.
  if (triple.isOSDarwin())
    target.OnceDonePredicateValue = -1L;
  // Other platforms use std::call_once() and we don't
  // assume that they have a barrier-free inline fast path.

  switch (triple.getArch()) {
  case llvm::Triple::x86_64: configureX86_64(IGM, triple, target); break;

  case llvm::Triple::x86: configureX86(IGM, triple, target); break;

  case llvm::Triple::arm:
  case llvm::Triple::thumb: configureARM(IGM, triple, target); break;

  case llvm::Triple::aarch64:
  case llvm::Triple::aarch64_32:
    if (triple.getArchName() == "arm64_32")
      configureARM64_32(IGM, triple, target);
    else
      configureARM64(IGM, triple, target);
    break;

  case llvm::Triple::ppc: configurePowerPC(IGM, triple, target); break;

  case llvm::Triple::ppc64:
  case llvm::Triple::ppc64le:
    configurePowerPC64(IGM, triple, target);
    break;

  case llvm::Triple::systemz:
    configureSystemZ(IGM, triple, target);
    break;
  case llvm::Triple::wasm32: configureWasm32(IGM, triple, target); break;

  default:
    // FIXME: Complain here? Default target info is unlikely to be
    // correct.
    break;
  }

  return target;
}
