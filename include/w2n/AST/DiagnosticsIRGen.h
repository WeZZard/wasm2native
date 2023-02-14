//===--- DiagnosticsIRGen.h - Diagnostic Definitions ------*- C++ -*-===//
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
/// \file
/// This file defines diagnostics for IR generation.
//
//===----------------------------------------------------------------===//

#ifndef W2N_DIAGNOSTICSIRGEN_H
#define W2N_DIAGNOSTICSIRGEN_H

#include <w2n/AST/DiagnosticsCommon.h>

namespace w2n {
namespace diag {

// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND, ID, Options, Text, Signature)                         \
  extern detail::DiagWithArguments<void Signature>::Type ID;
#include "DiagnosticsIRGen.def"

} // namespace diag
} // namespace w2n

#endif
