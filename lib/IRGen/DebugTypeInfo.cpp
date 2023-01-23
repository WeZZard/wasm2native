//===--- DebugTypeInfo.cpp - Type Info for Debugging ----------------===//
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
// This file defines the data structure that holds all the debug info
// we want to emit for types.
//
//===----------------------------------------------------------------===//

#include "DebugTypeInfo.h"
#include "FixedTypeInfo.h"
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;
using namespace irgen;

DebugTypeInfo::DebugTypeInfo(
  w2n::Type * Ty,
  llvm::Type * FragmentStorageTy,
  Optional<Size> SizeInBytes,
  Alignment AlignInBytes,
  bool HasDefaultAlignment,
  bool IsMetadata,
  bool SizeIsFragmentSize
) :
  Ty(Ty),
  FragmentStorageType(FragmentStorageTy),
  SizeInBytes(SizeInBytes),
  Align(AlignInBytes),
  DefaultAlignment(HasDefaultAlignment),
  IsMetadataType(IsMetadata),
  SizeIsFragmentSize(SizeIsFragmentSize) {
  assert(Align.getValue() != 0);
}

static bool hasDefaultAlignment(w2n::Type * Ty) {
  return w2n_proto_implemented([] { return true; });
}

DebugTypeInfo DebugTypeInfo::getFromTypeInfo(
  w2n::Type * Ty, const TypeInfo& Info, bool IsFragmentTypeInfo
) {
  Optional<Size> SizeInBytes;
  if (Info.isFixedSize()) {
    const FixedTypeInfo& FixTy = *cast<const FixedTypeInfo>(&Info);
    SizeInBytes = FixTy.getFixedSize();
  }
  assert(Info.getStorageType() && "StorageType is a nullptr");
  return DebugTypeInfo(
    Ty,
    Info.getStorageType(),
    SizeInBytes,
    Info.getBestKnownAlignment(),
    ::hasDefaultAlignment(Ty),
    false,
    IsFragmentTypeInfo
  );
}

bool DebugTypeInfo::operator==(DebugTypeInfo T) const {
  return (
    getType() == T.getType() && SizeInBytes == T.SizeInBytes
    && Align == T.Align
  );
}

bool DebugTypeInfo::operator!=(DebugTypeInfo T) const {
  return !operator==(T);
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void DebugTypeInfo::dump() const {
  llvm::errs() << "[";
  if (SizeInBytes) {
    llvm::errs() << "Size " << SizeInBytes->getValue() << " ";
  }
  llvm::errs() << "Alignment " << Align.getValue() << "] ";
  getType()->dump(llvm::errs());

  if (FragmentStorageType != nullptr) {
    llvm::errs() << "FragmentStorageType=";
    FragmentStorageType->dump();
  } else {
    llvm::errs() << "forward-declared\n";
  }
}
#endif
