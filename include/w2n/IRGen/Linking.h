//===--- Linking.h - Named declarations and linkage -------*- C++ -*-===//
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

#ifndef W2N_IRGEN_LINKING_H
#define W2N_IRGEN_LINKING_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/GlobalObject.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Module.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Table.h>
#include <w2n/Basic/Unimplemented.h>

namespace llvm {
class Triple;
} // namespace llvm

namespace w2n {

/// ported from SIL in Swift
enum ForDefinition_t : bool {
  NotForDefinition = false,
  ForDefinition = true
};

namespace irgen {
class IRGenModule;
class Alignment;

/// Determine if the triple uses the DLL storage.
bool useDllStorage(const llvm::Triple& Triple);

class UniversalLinkageInfo {
public:

  bool IsELFObject;
  bool UseDLLStorage;
  bool Internalize;

  /// True iff are multiple llvm modules.
  bool HasMultipleIGMs;

  /// When this is true, the linkage for forward-declared private symbols
  /// will be promoted to public external. Used by the LLDB expression
  /// evaluator.
  bool ForcePublicDecls;

  explicit UniversalLinkageInfo(IRGenModule& IGM);

  UniversalLinkageInfo(
    const llvm::Triple& Triple,
    bool HasMultipleIgMs,
    bool ForcePublicDecls,
    bool IsStaticLibrary
  );

  /// In case of multiple llvm modules (in multi-threaded compilation) all
  /// private decls must be visible from other files.
  bool shouldAllPrivateDeclsBeVisibleFromOtherFiles() const {
    return HasMultipleIGMs;
  }

  /// In case of multiple llvm modules, private lazy protocol
  /// witness table accessors could be emitted by two different IGMs
  /// during IRGen into different object files and the linker would
  /// complain about duplicate symbols.
  bool needLinkerToMergeDuplicateSymbols() const {
    return HasMultipleIGMs;
  }

  /// This is used by the LLDB expression evaluator since an expression's
  /// llvm::Module may need to access private symbols defined in the
  /// expression's context. This flag ensures that private accessors are
  /// forward-declared as public external in the expression's module.
  bool forcePublicDecls() const {
    return ForcePublicDecls;
  }
};

/// A link entity is some sort of named declaration, combined with all
/// the information necessary to distinguish specific implementations
/// of the declaration from each other.
///
/// For example, functions may be uncurried at different levels, each of
/// which potentially creates a different top-level function.
class LinkEntity {
  /// Function *, Table *, Memory *, or GlobalVariable *, depending on
  /// Kind.
  /// TODO: Can optimize with llvm::PointerUnion?
  void * Pointer;

  /// ProtocolConformance*, depending on Kind.
  void * SecondaryPointer;

  /// A hand-rolled bitfield with the following layout.
  unsigned Data;

  enum : unsigned {
    KindShift = 0,
    KindMask = 0xFF,
  };

#define W2N_LINK_ENTITY_SET_FIELD(Field, Value) (Value << Field##Shift)
#define W2N_LINK_ENTITY_GET_FIELD(Value, Field)                          \
  ((Value & Field##Mask) >> Field##Shift)

  enum class Kind {
    /// A function. Points to a `w2n::Function *`.
    Function,

    /// A table. Points to a `w2n::Table *`.
    Table,

    /// A memory. Points to a `w2n::Memory *`.
    Memory,

    /// A global variable. Points to a `w2n::GlobalVariable *`.
    GlobalVariable,

    /// A readonly global variable. Points to a `w2n::GlobalVariable *`.
    ReadonlyGlobalVariable,
  };

  friend struct llvm::DenseMapInfo<LinkEntity>;

  Kind getKind() const {
    return Kind(W2N_LINK_ENTITY_GET_FIELD(Data, Kind));
  }

  GlobalVariable * getGlobalVariable() const {
    return reinterpret_cast<GlobalVariable *>(Pointer);
  }

  Function * getFunction() const {
    return reinterpret_cast<Function *>(Pointer);
  }

  Table * getTable() const {
    return reinterpret_cast<Table *>(Pointer);
  }

  Memory * getMemory() const {
    return reinterpret_cast<Memory *>(Pointer);
  }

  LinkEntity() = default;

public:

  static LinkEntity forGlobalVariable(GlobalVariable * G);

  static LinkEntity forFunction(Function * F);

  static LinkEntity forTable(Table * T);

  static LinkEntity forMemory(Memory * M);

  void mangle(llvm::raw_ostream& out) const;

  void mangle(SmallVectorImpl<char>& buffer) const;

  std::string mangleAsString() const;

  ASTLinkage getLinkage(ForDefinition_t ForDefinition) const;

  /// Determine whether this entity will be weak-imported.
  bool isWeakImported(ModuleDecl * module) const {
    return w2n_proto_implemented(
      "Standard WebAssembly spec does not have weak import.",
      [&] { return false; }
    );
  }

  bool isAlwaysSharedLinkage() const {
    // TODO: What is always shared linkage in Swift?
    // TODO: How to map always shared linkage from Swift to Wasm?
    return w2n_proto_implemented([&] { return false; });
  }

  /// Return the module scope context whose codegen should trigger
  /// emission of this link entity, if one can be identified.
  DeclContext * getDeclContextForEmission() const;

  /// Get the preferred alignment for the definition of this entity.
  Alignment getAlignment(IRGenModule& IGM) const;
};

struct IRLinkage {
  llvm::GlobalValue::LinkageTypes Linkage;
  llvm::GlobalValue::VisibilityTypes Visibility;
  llvm::GlobalValue::DLLStorageClassTypes DLLStorage;

  static const IRLinkage InternalLinkOnceODR;
  static const IRLinkage InternalWeakODR;
  static const IRLinkage Internal;

  static const IRLinkage ExternalCommon;
  static const IRLinkage ExternalImport;
  static const IRLinkage ExternalWeakImport;
  static const IRLinkage ExternalExport;
};

class ApplyIRLinkage {
  IRLinkage IRL;

public:

  ApplyIRLinkage(IRLinkage IRL) : IRL(IRL) {
  }

  void to(llvm::GlobalValue * GV, bool Definition = true) const {
    llvm::Module * M = GV->getParent();
    const llvm::Triple Triple(M->getTargetTriple());

    GV->setLinkage(IRL.Linkage);
    GV->setVisibility(IRL.Visibility);
    if (Triple.isOSBinFormatCOFF() && !Triple.isOSCygMing()) {
      GV->setDLLStorageClass(IRL.DLLStorage);
    }

    // TODO: BFD and gold do not handle COMDATs properly
    if (Triple.isOSBinFormatELF()) {
      return;
    }

    // COMDATs cannot be applied to declarations.  If we have a
    // definition, apply the COMDAT.
    if (Definition) {
      if (IRL.Linkage == llvm::GlobalValue::LinkOnceODRLinkage || IRL.Linkage == llvm::GlobalValue::WeakODRLinkage) {
        if (Triple.supportsCOMDAT()) {
          if (llvm::GlobalObject * GO = dyn_cast<llvm::GlobalObject>(GV)) {
            GO->setComdat(M->getOrInsertComdat(GV->getName()));
          }
        }
      }
    }
  }
};

/// Encapsulated information about the linkage of an entity.
class LinkInfo {
  LinkInfo() = default;

  llvm::SmallString<32> Name;
  IRLinkage IRL;
  ForDefinition_t ForDefinition;

public:

  /// Compute linkage information for the given
  static LinkInfo get(
    IRGenModule& IGM,
    const LinkEntity& Entity,
    ForDefinition_t ForDefinition
  );

  static LinkInfo get(
    const UniversalLinkageInfo& Info,
    ModuleDecl * WasmModule,
    const LinkEntity& Entity,
    ForDefinition_t ForDefinition
  );

  static LinkInfo get(
    const UniversalLinkageInfo& Info,
    StringRef Name,
    ASTLinkage Linkage,
    ForDefinition_t ForDefinition
  );

  StringRef getName() const {
    return Name.str();
  }

  llvm::GlobalValue::LinkageTypes getLinkage() const {
    return IRL.Linkage;
  }

  llvm::GlobalValue::VisibilityTypes getVisibility() const {
    return IRL.Visibility;
  }

  llvm::GlobalValue::DLLStorageClassTypes getDLLStorage() const {
    return IRL.DLLStorage;
  }

  bool isForDefinition() const {
    return ForDefinition != 0U;
  }

  bool isUsed() const {
    return isForDefinition() && isUsed(IRL);
  }

  static bool isUsed(IRLinkage IRL);
};

StringRef encodeForceLoadSymbolName(
  llvm::SmallVectorImpl<char>& Buf, StringRef Name
);
} // namespace irgen
} // namespace w2n

/// Allow LinkEntity to be used as a key for a DenseMap.
namespace llvm {
template <>
struct DenseMapInfo<w2n::irgen::LinkEntity> {
  using LinkEntity = w2n::irgen::LinkEntity;

  static LinkEntity getEmptyKey() {
    LinkEntity Entity;
    Entity.Pointer = nullptr;
    Entity.SecondaryPointer = nullptr;
    Entity.Data = 0;
    return Entity;
  }

  static LinkEntity getTombstoneKey() {
    LinkEntity Entity;
    Entity.Pointer = nullptr;
    Entity.SecondaryPointer = nullptr;
    Entity.Data = 1;
    return Entity;
  }

  static unsigned getHashValue(const LinkEntity& Entity) {
    return DenseMapInfo<void *>::getHashValue(Entity.Pointer)
         ^ DenseMapInfo<void *>::getHashValue(Entity.SecondaryPointer)
         ^ Entity.Data;
  }

  static bool isEqual(const LinkEntity& LHS, const LinkEntity& RHS) {
    return LHS.Pointer == RHS.Pointer
        && LHS.SecondaryPointer == RHS.SecondaryPointer
        && LHS.Data == RHS.Data;
  }
};
} // namespace llvm

#endif
