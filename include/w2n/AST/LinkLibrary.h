//===--- LinkLibrary.h - A module-level linker dependency -*- C++ -*-===//
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

#ifndef W2N_AST_LINKLIBRARY_H
#define W2N_AST_LINKLIBRARY_H

#include <llvm/ADT/StringRef.h>
#include <string>
#include <w2n/Basic/LLVM.h>

namespace w2n {

// Must be kept in sync with diag::error_immediate_mode_missing_library.
enum class LibraryKind {
  Library = 0,
  Framework
};

/// Represents a linker dependency for an imported module.
// FIXME: This is basically a slightly more generic version of Clang's
// Module::LinkLibrary.
class LinkLibrary {
private:

  std::string Name;
  unsigned Kind : 1;
  unsigned ForceLoad : 1;

public:

  LinkLibrary(StringRef N, LibraryKind K, bool ForceLoad = false) :
    Name(N),
    Kind(static_cast<unsigned>(K)),
    ForceLoad(ForceLoad) {
    assert(getKind() == K && "not enough bits for the kind");
  }

  LibraryKind getKind() const {
    return static_cast<LibraryKind>(Kind);
  }

  StringRef getName() const {
    return Name;
  }

  bool shouldForceLoad() const {
    return ForceLoad;
  }
};

} // namespace w2n

#endif // W2N_AST_LINKLIBRARY_H
