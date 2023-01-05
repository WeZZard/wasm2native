//===--- Builtins.cpp - wasm2native Builtin ASTs --------------------===//
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
//===----------------------------------------------------------------------===//
//
//  This file implements the interface to the Builtin APIs.
//
//===----------------------------------------------------------------------===//

#include <w2n/AST/Builtins.h>

using namespace w2n;

StringRef w2n::getBuiltinName(BuiltinValueKind ID) {
  switch (ID) {
  case BuiltinValueKind::None: llvm_unreachable("no builtin kind");
#define BUILTIN(Id, Name, Attrs)                                         \
  case BuiltinValueKind::Id:                                             \
    return Name;                                                         \
    break;
#include <w2n/AST/Builtins.def>
  }
  llvm_unreachable("bad BuiltinValueKind");
}
