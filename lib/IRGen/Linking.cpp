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
#include "llvm/Support/Casting.h"
#include <llvm/ADT/Triple.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/Linkage.h>
#include <w2n/Basic/Unimplemented.h>
#include <w2n/IRGen/Linking.h>

using namespace w2n;
using namespace irgen;

#pragma mark - IRLinkage

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

static IRLinkage getIRLinkage(
  const UniversalLinkageInfo& info,
  ASTLinkage linkage,
  ForDefinition_t isDefinition,
  bool isWeakImported,
  bool isKnownLocal = false
) {
#define RESULT(LINKAGE, VISIBILITY, DLL_STORAGE)                         \
  IRLinkage{                                                             \
    llvm::GlobalValue::LINKAGE##Linkage,                                 \
    llvm::GlobalValue::VISIBILITY##Visibility,                           \
    llvm::GlobalValue::DLL_STORAGE##StorageClass}

  // Use protected visibility for public symbols we define on ELF.  ld.so
  // doesn't support relative relocations at load time, which interferes
  // with our metadata formats.  Default visibility should suffice for
  // other object formats.
  llvm::GlobalValue::VisibilityTypes PublicDefinitionVisibility =
    info.IsELFObject ? llvm::GlobalValue::ProtectedVisibility
                     : llvm::GlobalValue::DefaultVisibility;
  llvm::GlobalValue::DLLStorageClassTypes ExportedStorage =
    info.UseDLLStorage ? llvm::GlobalValue::DLLExportStorageClass
                       : llvm::GlobalValue::DefaultStorageClass;
  W2N_UNUSED
  llvm::GlobalValue::DLLStorageClassTypes ImportedStorage =
    info.UseDLLStorage ? llvm::GlobalValue::DLLImportStorageClass
                       : llvm::GlobalValue::DefaultStorageClass;

  switch (linkage) {
  case ASTLinkage::Public:
    return {
      llvm::GlobalValue::ExternalLinkage,
      PublicDefinitionVisibility,
      info.Internalize ? llvm::GlobalValue::DefaultStorageClass
                       : ExportedStorage};

  case ASTLinkage::Internal: {
    if (info.forcePublicDecls() && !isDefinition)
      return getIRLinkage(
        info,
        ASTLinkage::Public,
        isDefinition,
        isWeakImported,
        isKnownLocal
      );

    auto linkage = info.needLinkerToMergeDuplicateSymbols()
                   ? llvm::GlobalValue::LinkOnceODRLinkage
                   : llvm::GlobalValue::InternalLinkage;
    auto visibility = info.shouldAllPrivateDeclsBeVisibleFromOtherFiles()
                      ? llvm::GlobalValue::HiddenVisibility
                      : llvm::GlobalValue::DefaultVisibility;
    return {linkage, visibility, llvm::GlobalValue::DefaultStorageClass};
  }
  }

  llvm_unreachable("bad AST linkage");
}

#pragma mark - irgen Standalone Functions

bool w2n::irgen::useDllStorage(const llvm::Triple& triple) {
  return triple.isOSBinFormatCOFF() && !triple.isOSCygMing();
}

#pragma mark - UniversalLinkageInfo

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

#pragma mark - LinkEntity

LinkEntity LinkEntity::forGlobalVariable(GlobalVariable * G) {
  LinkEntity Entity;
  Entity.Pointer = G;
  Entity.SecondaryPointer = nullptr;
  Entity.Data = W2N_LINK_ENTITY_SET_FIELD(
    Kind,
    unsigned(
      G->isMutable() ? Kind::GlobalVariable : Kind::ReadonlyGlobalVariable
    )
  );
  return Entity;
}

LinkEntity LinkEntity::forFunction(Function * F) {
  LinkEntity Entity;
  Entity.Pointer = F;
  Entity.SecondaryPointer = nullptr;
  Entity.Data = W2N_LINK_ENTITY_SET_FIELD(Kind, unsigned(Kind::Function));
  return Entity;
}

LinkEntity LinkEntity::forTable(Table * T) {
  LinkEntity Entity;
  Entity.Pointer = T;
  Entity.SecondaryPointer = nullptr;
  Entity.Data = W2N_LINK_ENTITY_SET_FIELD(Kind, unsigned(Kind::Table));
  return Entity;
}

LinkEntity LinkEntity::forMemory(Memory * M) {
  LinkEntity Entity;
  Entity.Pointer = M;
  Entity.SecondaryPointer = nullptr;
  Entity.Data = W2N_LINK_ENTITY_SET_FIELD(Kind, unsigned(Kind::Memory));
  return Entity;
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
  case Kind::ReadonlyGlobalVariable:
  case Kind::GlobalVariable: {
    auto G = getGlobalVariable();
    auto& M = G->getModule();
    return (Twine(M.getName().str()) + Twine(".global$")
            + Twine(G->getIndex()))
      .str();
  }
  }
  llvm_unreachable("bad entity kind!");
}

ASTLinkage LinkEntity::getLinkage(ForDefinition_t forDefinition) const {
  switch (getKind()) {
  case Kind::Function: w2n_unimplemented();
  case Kind::Table: w2n_unimplemented();
  case Kind::Memory: w2n_unimplemented();
  case Kind::ReadonlyGlobalVariable:
  case Kind::GlobalVariable:
    // FIXME: Check if the global variable is exported.
    return w2n_proto_implemented([&] { return ASTLinkage::Internal; });
  }
  llvm_unreachable("bad link entity kind");
}

DeclContext * LinkEntity::getDeclContextForEmission() const {
  switch (getKind()) {
  case Kind::Function: return getFunction()->getDeclContext();
  case Kind::Table:
  case Kind::Memory: w2n_unimplemented(); break;
  case Kind::GlobalVariable:
  case Kind::ReadonlyGlobalVariable:
    return getGlobalVariable()->getDecl()->getDeclContext();
  }
}

Alignment LinkEntity::getAlignment(IRGenModule& IGM) const {
  llvm_unreachable("alignment not specified");
}

#pragma mark - LinkInfo

LinkInfo LinkInfo::get(
  IRGenModule& IGM,
  const LinkEntity& Entity,
  ForDefinition_t ForDefinition
) {
  return LinkInfo::get(
    UniversalLinkageInfo(IGM), IGM.getWasmModule(), Entity, ForDefinition
  );
}

LinkInfo LinkInfo::get(
  const UniversalLinkageInfo& Info,
  ModuleDecl * WasmModule,
  const LinkEntity& entity,
  ForDefinition_t isDefinition
) {
  LinkInfo result;
  entity.mangle(result.Name);

  bool isKnownLocal = entity.isAlwaysSharedLinkage();
  if (const auto * DC = entity.getDeclContextForEmission()) {
    if (const auto * MD = DC->getParentModule())
      isKnownLocal = MD == WasmModule || MD->isStaticLibrary();
  }

  result.IRL = getIRLinkage(
    Info, entity.getLinkage(isDefinition), isDefinition, isKnownLocal
  );
  result.ForDefinition = isDefinition;
  return result;
}

LinkInfo LinkInfo::get(
  const UniversalLinkageInfo& linkInfo,
  StringRef name,
  ASTLinkage linkage,
  ForDefinition_t isDefinition
) {
}

bool LinkInfo::isUsed(IRLinkage IRL) {
  // Everything externally visible is considered used in Swift.
  // That mostly means we need to be good at not marking things external.
  return IRL.Linkage == llvm::GlobalValue::ExternalLinkage
      && (IRL.Visibility == llvm::GlobalValue::DefaultVisibility
          || IRL.Visibility == llvm::GlobalValue::ProtectedVisibility)
      && (IRL.DLLStorage == llvm::GlobalValue::DefaultStorageClass
          || IRL.DLLStorage == llvm::GlobalValue::DLLExportStorageClass);
}
