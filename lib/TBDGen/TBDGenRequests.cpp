//===--- TBDGenRequests.cpp - Requests for TBD Generation
//----------------===//
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
//===----------------------------------------------------------------------===//

#include <clang/Basic/TargetInfo.h>
#include <llvm/TextAPI/InterfaceFile.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Evaluator.h>
#include <w2n/AST/FileUnit.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/TBDGenRequests.h>
#include <w2n/TBDGen/TBDGen.h>

#include "APIGen.h"

using namespace w2n;

namespace w2n {
// Implement the TBDGen type zone (zone 14).
#define W2N_TYPEID_ZONE   TBDGen
#define W2N_TYPEID_HEADER <w2n/AST/TBDGenTypeIDZone.def>
#include <w2n/Basic/ImplementTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
} // end namespace w2n

//----------------------------------------------------------------------------//
// GenerateTBDRequest computation.
//----------------------------------------------------------------------------//

FileUnit * TBDGenDescriptor::getSingleFile() const {
  return Input.dyn_cast<FileUnit *>();
}

ModuleDecl * TBDGenDescriptor::getParentModule() const {
  if (auto * module = Input.dyn_cast<ModuleDecl *>())
    return module;
  return Input.get<FileUnit *>()->getParentModule();
}

const StringRef TBDGenDescriptor::getDataLayoutString() const {
  // FIXME: prototype implementation.
  /// \see https://llvm.org/docs/LangRef.html#data-layout
  return llvm::StringRef("E");
}

const llvm::Triple& TBDGenDescriptor::getTarget() const {
  return getParentModule()->getASTContext().LangOpts.Target;
}

bool TBDGenDescriptor::operator==(const TBDGenDescriptor& other) const {
  return Input == other.Input && Opts == other.Opts;
}

llvm::hash_code w2n::hash_value(const TBDGenDescriptor& desc) {
  return llvm::hash_combine(desc.getFileOrModule(), desc.getOptions());
}

void w2n::simple_display(
  llvm::raw_ostream& out,
  const TBDGenDescriptor& desc
) {
  out << "Generate TBD for ";
  if (auto * module = desc.getFileOrModule().dyn_cast<ModuleDecl *>()) {
    out << "module ";
    simple_display(out, module);
  } else {
    out << "file ";
    simple_display(out, desc.getFileOrModule().get<FileUnit *>());
  }
}

SourceLoc w2n::extractNearestSourceLoc(const TBDGenDescriptor& desc) {
  return extractNearestSourceLoc(desc.getFileOrModule());
}

// Define request evaluation functions for each of the TBDGen requests.
static AbstractRequestFunction * tbdGenRequestFunctions[] = {
#define W2N_REQUEST(Zone, Name, Sig, Caching, LocOptions)                \
  reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include <w2n/AST/TBDGenTypeIDZone.def>
#undef W2N_REQUEST
};

void w2n::registerTBDGenRequestFunctions(Evaluator& evaluator) {
  evaluator.registerRequestFunctions(
    Zone::TBDGen, tbdGenRequestFunctions
  );
}
