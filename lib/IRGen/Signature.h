//===--- Signature.h - An IR function signature -----------*- C++ -*-===//
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
//  This file defines the Signature type, which encapsulates all the
//  information necessary to call a function value correctly.
//
//===----------------------------------------------------------------===//

#ifndef W2N_IRGEN_SIGNATURE_H
#define W2N_IRGEN_SIGNATURE_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/CallingConv.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/ExternalUnion.h>

namespace llvm {
class FunctionType;
} // namespace llvm

namespace w2n {
class Identifier;

namespace irgen {

class IRGenModule;
class TypeInfo;

/// A signature represents something which can actually be called.
class Signature {
  llvm::FunctionType * Type;
  llvm::AttributeList Attributes;
  llvm::CallingConv::ID CallingConv;

public:

  Signature() : Type(nullptr) {
  }

  Signature(
    llvm::FunctionType * FnType,
    llvm::AttributeList Attrs,
    llvm::CallingConv::ID CallingConv
  ) :
    Type(FnType),
    Attributes(Attrs),
    CallingConv(CallingConv) {
  }

  bool isValid() const {
    return Type != nullptr;
  }

  /// Compute the signature of the given type.
  ///
  /// This is a private detail of the implementation of
  /// IRGenModule::getSignature(CanSILFunctionType), which is what
  /// clients should generally be using.
  static Signature getUncached(IRGenModule& IGM, FuncType * FormalType);

  llvm::FunctionType * getType() const {
    assert(isValid());
    return Type;
  }

  llvm::CallingConv::ID getCallingConv() const {
    assert(isValid());
    return CallingConv;
  }

  llvm::AttributeList getAttributes() const {
    assert(isValid());
    return Attributes;
  }

  // The mutators below should generally only be used when building up
  // a callee.
  void setType(llvm::FunctionType * T) {
    Type = T;
  }

  llvm::AttributeList& getMutableAttributes() & {
    assert(isValid());
    return Attributes;
  }
};

} // end namespace irgen
} // namespace w2n

#endif // W2N_IRGEN_SIGNATURE_H
