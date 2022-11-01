//===--- TBDGenVisitor.h - AST Visitor for TBD generation
//-----------------===//
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
//  This file defines the visitor that finds all symbols in a swift AST.
//
//===----------------------------------------------------------------------===//
#ifndef W2N_TBDGEN_TBDGENVISITOR_H
#define W2N_TBDGEN_TBDGENVISITOR_H

#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/TextAPI/InterfaceFile.h>
#include <map>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/IRGen/Linking.h>

using namespace w2n::irgen;
using StringSet = llvm::StringSet<>;

namespace llvm {
class DataLayout;
}

namespace w2n {

class TBDGenDescriptor;
struct TBDGenOptions;
class SymbolSource;

namespace tbdgen {

enum class LinkerPlatformId : uint8_t {
#define LD_PLATFORM(Name, Id) Name = Id,
#include "ldPlatformKinds.def"
};

struct InstallNameStore {
  // The default install name to use when no specific install name is
  // specified.
  std::string InstallName;
  // The install name specific to the platform id. This takes precedence
  // over the default install name.
  std::map<uint8_t, std::string> PlatformInstallName;
  StringRef getInstallName(LinkerPlatformId Id) const;
};

/// A set of callbacks for recording APIs.
class APIRecorder {
public:

  virtual ~APIRecorder() {
  }

  virtual void addSymbol(
    StringRef name, llvm::MachO::SymbolKind kind, SymbolSource source
  ) {
  }
};

class SimpleAPIRecorder final : public APIRecorder {
public:

  using SymbolCallbackFn = llvm::function_ref<
    void(StringRef, llvm::MachO::SymbolKind, SymbolSource)>;

  SimpleAPIRecorder(SymbolCallbackFn func) : func(func) {
  }

  void addSymbol(
    StringRef symbol, llvm::MachO::SymbolKind kind, SymbolSource source
  ) override {
    func(symbol, kind, source);
  }

private:

  SymbolCallbackFn func;
};

} // end namespace tbdgen
} // end namespace w2n

#endif
