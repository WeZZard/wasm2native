//===--- ImplementTypeIDZone.h - Implement a TypeID Zone --------*- C++
//-*-===//
//
// This source file is part of the w2n.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the w2n project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://w2n.org/LICENSE.txt for license information
// See https://w2n.org/CONTRIBUTORS.txt for the list of w2n project
// authors
//
//===----------------------------------------------------------------------===//
//
//  This file should be #included to implement the TypeIDs for a given
//  zone in a C++ file. Two macros should be #define'd before inclusion,
//  and will be #undef'd at the end of this file:
//
//    W2N_TYPEID_ZONE: The ID number of the Zone being defined, which must
//    be unique. 0 is reserved for basic C and LLVM types; 255 is reserved
//    for test cases.
//
//    W2N_TYPEID_HEADER: A (quoted) name of the header to be
//    included to define the types in the zone.
//
//===----------------------------------------------------------------------===//

#ifndef W2N_TYPEID_ZONE
#error Must define the value of the TypeID zone with the given name.
#endif

#ifndef W2N_TYPEID_HEADER
#error Must define the TypeID header name with W2N_TYPEID_HEADER
#endif

// Define a TypeID where the type name and internal name are the same.
#define W2N_REQUEST(Zone, Type, Sig, Caching, LocOptions)                \
  W2N_TYPEID_NAMED(Type, Type)
#define W2N_TYPEID(Type) W2N_TYPEID_NAMED(Type, Type)

// Out-of-line definitions.
#define W2N_TYPEID_NAMED(Type, Name) const uint64_t TypeID<Type>::value;

#define W2N_TYPEID_TEMPLATE1_NAMED(Template, Name, Param1, Arg1)

#include W2N_TYPEID_HEADER

#undef W2N_REQUEST

#undef W2N_TYPEID_NAMED
#undef W2N_TYPEID_TEMPLATE1_NAMED

#undef W2N_TYPEID
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER
