//===--- Linking.cpp - Name mangling for IRGen entities -------------===//
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
//  This file implements name mangling for IRGen entities with linkage.
//
//===----------------------------------------------------------------===//

#include "IRGenModule.h"
#include <llvm/ADT/Triple.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/Linking.h>

using namespace w2n;
using namespace irgen;

const IRLinkage IRLinkage::InternalLinkOnceODR = {
  llvm::GlobalValue::LinkOnceODRLinkage,
  llvm::GlobalValue::HiddenVisibility,
  llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::InternalWeakODR = {
  llvm::GlobalValue::WeakODRLinkage,
  llvm::GlobalValue::HiddenVisibility,
  llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::Internal = {
  llvm::GlobalValue::InternalLinkage,
  llvm::GlobalValue::DefaultVisibility,
  llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::ExternalCommon = {
  llvm::GlobalValue::CommonLinkage,
  llvm::GlobalValue::DefaultVisibility,
  llvm::GlobalValue::DLLExportStorageClass,
};

const IRLinkage IRLinkage::ExternalImport = {
  llvm::GlobalValue::ExternalLinkage,
  llvm::GlobalValue::DefaultVisibility,
  llvm::GlobalValue::DLLImportStorageClass,
};

const IRLinkage IRLinkage::ExternalWeakImport = {
  llvm::GlobalValue::ExternalWeakLinkage,
  llvm::GlobalValue::DefaultVisibility,
  llvm::GlobalValue::DLLImportStorageClass,
};

const IRLinkage IRLinkage::ExternalExport = {
  llvm::GlobalValue::ExternalLinkage,
  llvm::GlobalValue::DefaultVisibility,
  llvm::GlobalValue::DLLExportStorageClass,
};

bool w2n::irgen::useDllStorage(const llvm::Triple& triple) {
  return triple.isOSBinFormatCOFF() && !triple.isOSCygMing();
}

UniversalLinkageInfo::UniversalLinkageInfo(IRGenModule& IGM) :
  UniversalLinkageInfo(
    IGM.Triple,
    IGM.IRGen.hasMultipleIGMs(),
    IGM.IRGen.Opts.ForcePublicLinkage,
    IGM.IRGen.Opts.InternalizeSymbols
  ) {
}

UniversalLinkageInfo::UniversalLinkageInfo(
  const llvm::Triple& triple,
  bool hasMultipleIGMs,
  bool forcePublicDecls,
  bool isStaticLibrary
) :
  IsELFObject(triple.isOSBinFormatELF()),
  UseDLLStorage(useDllStorage(triple)),
  Internalize(isStaticLibrary),
  HasMultipleIGMs(hasMultipleIGMs),
  ForcePublicDecls(forcePublicDecls) {
}

/// Mangle this entity into the given buffer.
void LinkEntity::mangle(SmallVectorImpl<char>& buffer) const {
  llvm::raw_svector_ostream stream(buffer);
  mangle(stream);
}

/// Mangle this entity into the given stream.
void LinkEntity::mangle(raw_ostream& buffer) const {
  std::string Result = mangleAsString();
  buffer.write(Result.data(), Result.size());
}

/// Mangle this entity as a std::string.
std::string LinkEntity::mangleAsString() const {
  switch (getKind()) {
  case Kind::Function: w2n_unimplemented();
  case Kind::Table: w2n_unimplemented();
  case Kind::Memory: w2n_unimplemented();
  case Kind::GlobalVariable: w2n_unimplemented();
  }
  llvm_unreachable("bad entity kind!");
}

ASTLinkage LinkEntity::getLinkage(ForDefinition_t forDefinition) const {
  llvm_unreachable("bad link entity kind");
}

Alignment LinkEntity::getAlignment(IRGenModule& IGM) const {
  llvm_unreachable("alignment not specified");
}
