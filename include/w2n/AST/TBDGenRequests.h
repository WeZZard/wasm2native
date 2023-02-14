//===--- TBDGenRequests.h - TBDGen Requests ---------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
//  This file defines TBDGen requests.
//
//===----------------------------------------------------------------===//

#ifndef W2N_AST_TBDGENREQUESTS_H
#define W2N_AST_TBDGENREQUESTS_H

#include <w2n/AST/ASTTypeIDs.h>
#include <w2n/AST/SimpleRequest.h>
#include <w2n/IRGen/Linking.h>
#include <w2n/TBDGen/TBDGen.h>

namespace llvm {

class DataLayout;
class Triple;

namespace MachO {
class InterfaceFile;
} // end namespace MachO

template <class AllocatorTy>
class StringSet;
} // end namespace llvm

namespace w2n {

class FileUnit;
class ModuleDecl;

namespace apigen {
class API;
} // end namespace apigen

class TBDGenDescriptor final {
  using FileOrModule = llvm::PointerUnion<FileUnit *, ModuleDecl *>;
  FileOrModule Input;
  TBDGenOptions Opts;

  TBDGenDescriptor(FileOrModule Input, const TBDGenOptions& Opts) :
    Input(Input),
    Opts(Opts) {
    assert(Input);
  }

public:

  /// Returns the file or module we're emitting TBD for.
  FileOrModule getFileOrModule() const {
    return Input;
  }

  /// If the input is a single file, returns that file. Otherwise returns
  /// \c nullptr.
  FileUnit * getSingleFile() const;

  /// Returns the parent module for TBD emission.
  ModuleDecl * getParentModule() const;

  /// Returns the TBDGen options.
  const TBDGenOptions& getOptions() const {
    return Opts;
  }

  TBDGenOptions& getOptions() {
    return Opts;
  }

  const StringRef getDataLayoutString() const;
  const llvm::Triple& getTarget() const;

  bool operator==(const TBDGenDescriptor& Other) const;

  bool operator!=(const TBDGenDescriptor& Other) const {
    return !(*this == Other);
  }

  static TBDGenDescriptor
  forFile(FileUnit * File, const TBDGenOptions& Opts) {
    return TBDGenDescriptor(File, Opts);
  }

  static TBDGenDescriptor
  forModule(ModuleDecl * M, const TBDGenOptions& Opts) {
    return TBDGenDescriptor(M, Opts);
  }
};

llvm::hash_code hash_value(const TBDGenDescriptor& hs);
void simple_display(llvm::raw_ostream& os, const TBDGenDescriptor& ss);
SourceLoc extractNearestSourceLoc(const TBDGenDescriptor& Desc);

using TBDFile = llvm::MachO::InterfaceFile;

/// Computes the TBD file for a given w2n module or file.
class GenerateTBDRequest :
  public SimpleRequest<
    GenerateTBDRequest,
    TBDFile(TBDGenDescriptor),
    RequestFlags::Uncached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  TBDFile evaluate(Evaluator& Eval, TBDGenDescriptor Desc) const;
};

/// Retrieve the public symbols for a file or module.
class PublicSymbolsRequest :
  public SimpleRequest<
    PublicSymbolsRequest,
    std::vector<std::string>(TBDGenDescriptor),
    RequestFlags::Uncached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  std::vector<std::string>
  evaluate(Evaluator& Eval, TBDGenDescriptor Desc) const;
};

/// Retrieve API information for a file or module.
class APIGenRequest :
  public SimpleRequest<
    APIGenRequest,
    apigen::API(TBDGenDescriptor),
    RequestFlags::Uncached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  apigen::API evaluate(Evaluator& Eval, TBDGenDescriptor Desc) const;
};

/// Describes the origin of a particular symbol, including the stage of
/// compilation it is introduced, as well as information on what decl
/// introduces it.
class SymbolSource {
public:

  enum class Kind {
    /// A symbol introduced when emitting LLVM IR.
    IR,

    /// A symbol used to customize linker behavior, introduced by TBDGen.
    LinkerDirective,

    /// A symbol with an unknown origin.
    // FIXME: This should be eliminated.
    Unknown
  };
  Kind SSKind;

private:

  union {
    irgen::LinkEntity IREntity;
  };

  explicit SymbolSource(irgen::LinkEntity Entity) : SSKind(Kind::IR) {
    IREntity = Entity;
  }

  explicit SymbolSource(Kind Kind) : SSKind(Kind) {
    assert(Kind == Kind::LinkerDirective || Kind == Kind::Unknown);
  }

public:

  static SymbolSource forIRLinkEntity(irgen::LinkEntity Entity) {
    return SymbolSource{Entity};
  }

  static SymbolSource forLinkerDirective() {
    return SymbolSource{Kind::LinkerDirective};
  }

  static SymbolSource forUnknown() {
    return SymbolSource{Kind::Unknown};
  }

  bool isLinkerDirective() const {
    return SSKind == Kind::LinkerDirective;
  }

  irgen::LinkEntity getIRLinkEntity() const {
    assert(SSKind == Kind::IR);
    return IREntity;
  }
};

/// Maps a symbol back to its source for lazy compilation.
class SymbolSourceMap {
  friend class SymbolSourceMapRequest;

  using Storage = llvm::StringMap<SymbolSource>;
  const Storage * SSStorage;

  explicit SymbolSourceMap(const Storage * Storage) : SSStorage(Storage) {
    assert(Storage);
  }

public:

  Optional<SymbolSource> find(StringRef Symbol) const {
    auto Result = SSStorage->find(Symbol);
    if (Result == SSStorage->end()) {
      return None;
    }
    return Result->second;
  }

  friend bool
  operator==(const SymbolSourceMap& Lhs, const SymbolSourceMap& Rhs) {
    return Lhs.SSStorage == Rhs.SSStorage;
  }

  friend bool
  operator!=(const SymbolSourceMap& Lhs, const SymbolSourceMap& Rhs) {
    return !(Lhs == Rhs);
  }

  friend void
  simple_display(llvm::raw_ostream& os, const SymbolSourceMap& ss) {
    os << "(symbol storage map)";
  }
};

/// Computes a map of symbols to their SymbolSource for a file or module.
class SymbolSourceMapRequest :
  public SimpleRequest<
    SymbolSourceMapRequest,
    SymbolSourceMap(TBDGenDescriptor),
    RequestFlags::Cached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  // Evaluation.
  SymbolSourceMap evaluate(Evaluator& Eval, TBDGenDescriptor Desc) const;

public:

  // Cached.
  bool isCached() const {
    return true;
  }
};

/// Report that a request of the given kind is being evaluated, so it
/// can be recorded by the stats reporter.
template <typename Request>
void reportEvaluatedRequest(
  UnifiedStatsReporter& Stats, const Request& Req
);

/// The zone number for TBDGen.
#define W2N_TYPEID_ZONE   TBDGen
#define W2N_TYPEID_HEADER "w2n/AST/TBDGenTypeIDZone.def"
#include "w2n/Basic/DefineTypeIDZone.h"
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define W2N_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)         \
  template <>                                                            \
  inline void reportEvaluatedRequest(                                    \
    UnifiedStatsReporter& stats, const RequestType& request              \
  ) {                                                                    \
    ++stats.getFrontendCounters().RequestType;                           \
  }
#include "w2n/AST/TBDGenTypeIDZone.def"
#undef W2N_REQUEST

} // end namespace w2n

#endif // W2N_AST_TBDGENREQUESTS_H
