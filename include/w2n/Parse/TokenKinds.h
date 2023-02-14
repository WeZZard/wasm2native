//===--- TokenKinds.h - Token Kinds Interface -------------*- C++ -*-===//
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
//  This file defines the Token kinds.
//
//===----------------------------------------------------------------===//

#ifndef W2N_TOKENKINDS_H
#define W2N_TOKENKINDS_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

namespace w2n {

enum class TokenKind : uint8_t {
#define TOKEN(X) X,
#include "w2n/Parse/TokenKinds.def"

  NUM_TOKENS,
};

/// Check whether a token kind is known to have any specific text content.
/// e.g., tol::l_paren has determined text however TokenKind::identifier
/// doesn't.
bool isTokenTextDetermined(TokenKind Kind);

/// If a token kind has determined text, return the text; otherwise
/// assert.
llvm::StringRef getTokenText(TokenKind Kind);

void simple_display(llvm::raw_ostream& os, TokenKind ss);
} // end namespace w2n

#endif // W2N_TOKENKINDS_H
