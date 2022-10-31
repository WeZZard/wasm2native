//===--- TBDGen.cpp - wasm2native TBD Generation --------------------===//
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
//  This file implements the entrypoints into TBD file generation.
//
//===----------------------------------------------------------------===//

#include <clang/Basic/TargetInfo.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>
#include <llvm/TextAPI/InterfaceFile.h>
#include <llvm/TextAPI/Symbol.h>
#include <llvm/TextAPI/TextAPIReader.h>
#include <llvm/TextAPI/TextAPIWriter.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/DiagnosticsFrontend.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/AST/TBDGenRequests.h>
#include <w2n/Basic/Defer.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/IRGen/Linking.h>
#include <w2n/TBDGen/TBDGen.h>

#include "APIGen.h"
#include "TBDGenVisitor.h"

using namespace w2n;
using namespace w2n::irgen;
using namespace w2n::tbdgen;
using namespace llvm::yaml;
using StringSet = llvm::StringSet<>;
using SymbolKind = llvm::MachO::SymbolKind;

static StringRef getLinkerPlatformName(uint8_t Id) {
#define LD_PLATFORM(Name, Id)                                            \
  case Id: return #Name;
  switch (Id) {
#include "ldPlatformKinds.def"
  default: llvm_unreachable("unrecognized platform id");
  }
}

static Optional<uint8_t> getLinkerPlatformId(StringRef Platform) {
  return llvm::StringSwitch<Optional<uint8_t>>(Platform)
#define LD_PLATFORM(Name, Id) .Case(#Name, Id)
#include "ldPlatformKinds.def"
    .Default(None);
}

StringRef InstallNameStore::getInstallName(LinkerPlatformId Id) const {
  auto It = PlatformInstallName.find((uint8_t)Id);
  if (It == PlatformInstallName.end())
    return InstallName;
  else
    return It->second;
}

static std::string getScalaNodeText(Node * N) {
  SmallString<32> Buffer;
  return cast<ScalarNode>(N)->getValue(Buffer).str();
}

static std::set<int8_t>
getSequenceNodePlatformList(ASTContext& Ctx, Node * N) {
  std::set<int8_t> Results;
  for (auto& E : *cast<SequenceNode>(N)) {
    auto Platform = getScalaNodeText(&E);
    auto Id = getLinkerPlatformId(Platform);
    if (Id.has_value()) {
      Results.insert(*Id);
    } else {
      // Diagnose unrecognized platform name.
      Ctx.Diags.diagnose(
        SourceLoc(), diag::unknown_platform_name, Platform
      );
    }
  }
  return Results;
}

/// The kind of version being parsed, used for diagnostics.
/// Note: Must match the order in DiagnosticsFrontend.def
enum DylibVersionKind_t : unsigned {
  CurrentVersion,
  CompatibilityVersion
};

/// Converts a version string into a packed version, truncating each
/// component if necessary to fit all 3 into a 32-bit packed structure.
///
/// For example, the version '1219.37.11' will be packed as
///
///  Major (1,219)       Minor (37) Patch (11)
/// ┌───────────────────┬──────────┬──────────┐
/// │ 00001100 11000011 │ 00100101 │ 00001011 │
/// └───────────────────┴──────────┴──────────┘
///
/// If an individual component is greater than the highest number that can
/// be represented in its alloted space, it will be truncated to the
/// maximum value that fits in the alloted space, which matches the
/// behavior of the linker.
static Optional<llvm::MachO::PackedVersion> parsePackedVersion(
  DylibVersionKind_t kind, StringRef versionString, ASTContext& ctx
) {
  if (versionString.empty())
    return None;

  llvm::MachO::PackedVersion version;
  auto result = version.parse64(versionString);
  if (!result.first) {
    ctx.Diags.diagnose(
      SourceLoc(),
      diag::tbd_err_invalid_version,
      (unsigned)kind,
      versionString
    );
    return None;
  }
  if (result.second) {
    ctx.Diags.diagnose(
      SourceLoc(),
      diag::tbd_warn_truncating_version,
      (unsigned)kind,
      versionString
    );
  }
  return version;
}

TBDFile GenerateTBDRequest::evaluate(
  Evaluator& evaluator, TBDGenDescriptor desc
) const {
  llvm_unreachable("not implemented.");
}

std::vector<std::string> PublicSymbolsRequest::evaluate(
  Evaluator& evaluator, TBDGenDescriptor desc
) const {
  llvm_unreachable("not implemented.");
}

std::vector<std::string> w2n::getPublicSymbols(TBDGenDescriptor Desc) {
  auto& Eval = Desc.getParentModule()->getASTContext().Eval;
  return llvm::cantFail(Eval(PublicSymbolsRequest{Desc}));
}

void w2n::writeTBDFile(
  ModuleDecl * M, llvm::raw_ostream& os, const TBDGenOptions& opts
) {
  auto& evaluator = M->getASTContext().Eval;
  auto desc = TBDGenDescriptor::forModule(M, opts);
  auto file = llvm::cantFail(evaluator(GenerateTBDRequest{desc}));
  llvm::cantFail(
    llvm::MachO::TextAPIWriter::writeToStream(os, file),
    "YAML writing should be error-free"
  );
}

apigen::API APIGenRequest::evaluate(
  Evaluator& evaluator, TBDGenDescriptor desc
) const {
  llvm_unreachable("not implemented.");
}

void w2n::writeAPIJSONFile(
  ModuleDecl * M, llvm::raw_ostream& os, bool PrettyPrint
) {
  TBDGenOptions opts;
  auto& evaluator = M->getASTContext().Eval;
  auto desc = TBDGenDescriptor::forModule(M, opts);
  auto api = llvm::cantFail(evaluator(APIGenRequest{desc}));
  api.writeAPIJSONFile(os, PrettyPrint);
}

SymbolSourceMap SymbolSourceMapRequest::evaluate(
  Evaluator& evaluator, TBDGenDescriptor desc
) const {
  llvm_unreachable("not implemented.");
}
