//===--- GenDecl.cpp - IR Generation for Declarations ---------------===//
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
//  This file implements IR generation for local and global
//  declarations in Swift.
//
//===----------------------------------------------------------------===//

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalAlias.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/SaveAndRestore.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/IRGenOptions.h>
#include <w2n/AST/Module.h>
#include <w2n/IRGen/Linking.h>

#include "GenDecl.h"
#include "IRGenFunction.h"
#include "IRGenModule.h"

using namespace w2n;
using namespace irgen;

static void
addLLVMFunctionAttributes(Function * f, Signature& signature) {
  // TODO: Swift deals with inline and readonly here.
}

// Eagerly emit functions that are externally visible. Functions that are
// dynamic replacements must also be eagerly emitted.
static bool isLazilyEmittedFunction(Function * F, IRGenModule * M) {
  if (F->isPossiblyUsedExternally())
    return false;

  // Needed by lldb to print global variables which are propagated by the
  // mandatory GlobalOpt.
  if (M->getOptions().OptMode == OptimizationMode::NoOptimization && F->isGlobalInit())
    return false;

  return true;
}

llvm::Function * IRGenModule::getAddrOfFunction(
  Function * F, ForDefinition_t forDefinition
) {
  LinkEntity entity = LinkEntity::forFunction(F);

  // Check whether we've created the function already.
  // FIXME: We should integrate this into the LinkEntity cache more
  // cleanly.
  llvm::Function * Fn = Module->getFunction(entity.mangleAsString());
  if (Fn) {
    if (forDefinition) {
      updateLinkageForDefinition(*this, Fn, entity);
    }
    return Fn;
  }

  LinkInfo Link = LinkInfo::get(*this, entity, forDefinition);
  bool isDefinition = F->isDefinition();
  bool hasOrderNumber = isDefinition;
  unsigned orderNumber = ~0U;
  llvm::Function * InsertBefore = nullptr;

  // If the SIL function has a definition, we should have an order
  // number for it; make sure to insert it in that position relative
  // to other ordered functions.
  if (hasOrderNumber) {
    orderNumber = IRGen.getFunctionOrder(F);
    if (auto emittedFunctionIterator = EmittedFunctionsByOrder.findLeastUpperBound(orderNumber))
      InsertBefore = *emittedFunctionIterator;
  }

  if (isDefinition && !forDefinition && isLazilyEmittedFunction(F, this)) {
    IRGen.addLazyFunction(F);
  }

  Signature Sig = getSignature(F->getType()->getType());
  addLLVMFunctionAttributes(F, Sig);

  Fn = createFunction(
    *this,
    Link,
    Sig,
    InsertBefore,
    getOptions().OptMode,
    shouldEmitStackProtector(F)
  );

  if (!forDefinition) {
    Fn->setComdat(nullptr);
  }

  // If we have an order number for this function, set it up as
  // appropriate.
  if (hasOrderNumber) {
    EmittedFunctionsByOrder.insert(orderNumber, Fn);
  }
  return Fn;
}

#pragma mark - GenDecl

/// Given that we're going to define a global value but already have a
/// forward-declaration of it, update its linkage.
void irgen::updateLinkageForDefinition(
  IRGenModule& IGM, llvm::GlobalValue * global, const LinkEntity& entity
) {
}

llvm::Function * irgen::createFunction(
  IRGenModule& IGM,
  LinkInfo& LinkInfo,
  const Signature& Signature,
  llvm::Function * InsertBefore,
  OptimizationMode FuncOptMode,
  StackProtectorMode StackProtect
) {
}

static void markGlobalAsUsedBasedOnLinkage(
  IRGenModule& IGM, LinkInfo& link, llvm::GlobalValue * Global
) {
  // FIXME: InternalizeAtLink
  // If we're internalizing public symbols at link time, don't make
  // globals unconditionally externally visible.
  // if (IGM.getOptions().InternalizeAtLink)
  //  return;

  // Everything externally visible is considered used in Swift.
  // That mostly means we need to be good at not marking things external.
  if (link.isUsed())
    IGM.addUsedGlobal(Global);
  else if (!IGM.IRGen.Opts.shouldOptimize() &&
          // WebAssembly does not have object abstraction. The following
          // things might shall be removed.
          // FIXME: !IGM.IRGen.Opts.ConditionalRuntimeRecords &&
          // FIXME: !IGM.IRGen.Opts.VirtualFunctionElimination &&
          // FIXME: !IGM.IRGen.Opts.WitnessMethodElimination &&
           !Global->isDeclaration()) {
    // llvm's pipeline has decide to run GlobalDCE as part of the O0
    // pipeline. Mark non public symbols as compiler used to counter act
    // this.
    IGM.addCompilerUsedGlobal(Global);
  }
}

llvm::GlobalVariable * irgen::createGlobalVariable(
  IRGenModule& IGM,
  LinkInfo& LinkInfo,
  llvm::Type * ObjectType,
  Alignment Alignment,
  DebugTypeInfo DebugType,
  Optional<SourceLoc> DebugLoc,
  StringRef DebugName
) {
  auto Name = LinkInfo.getName();
  llvm::GlobalValue * ExistingValue = IGM.Module->getNamedGlobal(Name);
  if (ExistingValue) {
    auto existingVar = dyn_cast<llvm::GlobalVariable>(ExistingValue);
    if (existingVar && existingVar->getValueType() == ObjectType)
      return existingVar;

    IGM.error(
      SourceLoc(),
      "program too clever: variable collides with existing symbol " + Name
    );

    // Note that this will implicitly unique if the .unique name is also
    // taken.
    ExistingValue->setName(Name + ".unique");
  }

  auto Var = new llvm::GlobalVariable(
    *IGM.Module,
    ObjectType,
    /*constant*/ false,
    LinkInfo.getLinkage(),
    /*initializer*/ nullptr,
    Name
  );
  ApplyIRLinkage({LinkInfo.getLinkage(),
                  LinkInfo.getVisibility(),
                  LinkInfo.getDLLStorage()})
    .to(Var, LinkInfo.isForDefinition());
  Var->setAlignment(llvm::MaybeAlign(Alignment.getValue()));

  markGlobalAsUsedBasedOnLinkage(IGM, LinkInfo, Var);

  // TODO: IGM.DebugInfo->emitGlobalVariableDeclaration
  // if (IGM.DebugInfo && !DbgTy.isNull() && LinkInfo.isForDefinition()) {
  //   IGM.DebugInfo->emitGlobalVariableDeclaration
  // }

  return Var;
}

llvm::GlobalVariable *
irgen::createLinkerDirectiveVariable(IRGenModule& IGM, StringRef Name) {
  // A prefix of \1 can avoid further mangling of the symbol (prefixing
  // _).
  llvm::SmallString<32> NameWithFlag;
  NameWithFlag.push_back('\1');
  NameWithFlag.append(Name);
  Name = NameWithFlag.str();
  static const uint8_t Size = 8;
  static const uint8_t Alignment = 8;

  // Use a char type as the type for this linker directive.
  auto ProperlySizedIntTy = Type::getBuiltinIntegerType(
    Size, IGM.getWasmModule()->getASTContext()
  );
  auto StorageType = IGM.getStorageType(ProperlySizedIntTy);

  llvm::GlobalValue * ExistingValue = IGM.Module->getNamedGlobal(Name);
  if (ExistingValue) {
    auto ExistingVar = dyn_cast<llvm::GlobalVariable>(ExistingValue);
    if (ExistingVar && ExistingVar->getValueType() == StorageType)
      return ExistingVar;

    IGM.error(
      SourceLoc(),
      "program too clever: variable collides with existing symbol " + Name
    );

    // Note that this will implicitly unique if the .unique name is also
    // taken.
    ExistingValue->setName(Name + ".unique");
  }

  llvm::GlobalValue::LinkageTypes Linkage =
    llvm::GlobalValue::LinkageTypes::ExternalLinkage;
  auto var = new llvm::GlobalVariable(
    *IGM.Module,
    StorageType,
    /*constant*/ true,
    Linkage,
    /*Init to zero*/ llvm::Constant::getNullValue(StorageType),
    Name
  );
  ApplyIRLinkage(
    {Linkage,
     llvm::GlobalValue::VisibilityTypes::DefaultVisibility,
     llvm::GlobalValue::DLLStorageClassTypes::DefaultStorageClass}
  )
    .to(var);
  var->setAlignment(llvm::MaybeAlign(Alignment));
  disableAddressSanitizer(IGM, var);
  IGM.addUsedGlobal(var);
  return var;
}

void irgen::disableAddressSanitizer(
  IRGenModule& IGM, llvm::GlobalVariable * var
) {
  // Add an operand to llvm.asan.globals denylisting this global variable.
  llvm::Metadata * metadata[] = {
    // The global variable to denylist.
    llvm::ConstantAsMetadata::get(var),

    // Source location. Optional, unnecessary here.
    nullptr,

    // Name. Optional, unnecessary here.
    nullptr,

    // Whether the global is dynamically initialized.
    llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
      llvm::Type::getInt1Ty(IGM.Module->getContext()), false
    )),

    // Whether the global is denylisted.
    llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
      llvm::Type::getInt1Ty(IGM.Module->getContext()), true
    ))};

  auto * globalNode =
    llvm::MDNode::get(IGM.Module->getContext(), metadata);
  auto * asanMetadata =
    IGM.Module->getOrInsertNamedMetadata("llvm.asan.globals");
  asanMetadata->addOperand(globalNode);
}