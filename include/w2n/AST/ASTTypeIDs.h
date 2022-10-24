//===--- ASTTypeIDs.h - AST Type Ids ----------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project
// authors
//
//===----------------------------------------------------------------===//
//
//  This file defines TypeID support for AST types.
//
//===-----------------------------------------------------------------===//

#ifndef W2N_AST_ASTTYPEIDS_H
#define W2N_AST_ASTTYPEIDS_H

#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/TypeID.h>

namespace w2n {

// Define the AST type zone (zone 1)
#define W2N_TYPEID_ZONE   AST
#define W2N_TYPEID_HEADER <w2n/AST/ASTTypeIDZone.def>
#include <w2n/Basic/DefineTypeIDZone.h>

} // end namespace w2n

#endif // W2N_AST_ASTTYPEIDS_H
