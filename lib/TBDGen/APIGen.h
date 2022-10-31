//===--- APIGen.h - Public interface to APIGen ------------*- C++ -*-===//
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

#ifndef W2N_APIGEN_APIGEN_H
#define W2N_APIGEN_APIGEN_H

#include <llvm/ADT/BitmaskEnum.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/Error.h>
#include <w2n/Basic/LLVM.h>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace w2n {
namespace apigen {
LLVM_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();
enum class APIAccess : uint8_t {
  Unknown = 0, // No information about access.
  Project = 1, // APIs available within the project.
  Private = 2, // Private unstable APIs.
  Public = 3,  // Public stable APIs.
};

enum class APILinkage : uint8_t {
  Unknown = 0,    // Unknown.
  Internal = 1,   // API is internal.
  External = 2,   // External interface used.
  Reexported = 3, // API is re-exported.
  Exported = 4,   // API is exported.
};

enum class APIFlags : uint8_t {
  None = 0,
  ThreadLocalValue = 1U << 0,
  WeakDefined = 1U << 1,
  WeakReferenced = 1U << 2,
  LLVM_MARK_AS_BITMASK_ENUM(/*LargestValue=*/WeakReferenced)
};

class APILoc {
public:
  APILoc() = default;

  APILoc(std::string file, unsigned line, unsigned col)
    : file(file),
      line(line),
      col(col) {
  }

  StringRef getFilename() const {
    return file;
  }

  unsigned getLine() const {
    return line;
  }

  unsigned getColumn() const {
    return col;
  }

private:
  std::string file;
  unsigned line;
  unsigned col;
};

struct APIAvailability {
  std::string introduced;
  std::string obsoleted;
  bool unavailable = false;

  bool empty() {
    return introduced.empty() && obsoleted.empty() && !unavailable;
  }
};

struct APIRecord {
  std::string name;
  APILoc loc;
  APILinkage linkage;
  APIFlags flags;
  APIAccess access;
  APIAvailability availability;

  APIRecord(
    StringRef name,
    APILoc loc,
    APILinkage linkage,
    APIFlags flags,
    APIAccess access,
    APIAvailability availability
  )
    : name(name.data(), name.size()),
      loc(loc),
      linkage(linkage),
      flags(flags),
      access(access),
      availability(availability) {
  }

  bool isWeakDefined() const {
    return (flags & APIFlags::WeakDefined) == APIFlags::WeakDefined;
  }

  bool isWeakReferenced() const {
    return (flags & APIFlags::WeakReferenced) == APIFlags::WeakReferenced;
  }

  bool isThreadLocalValue() const {
    return (flags & APIFlags::ThreadLocalValue) ==
           APIFlags::ThreadLocalValue;
  }

  bool isExternal() const {
    return linkage == APILinkage::External;
  }

  bool isExported() const {
    return linkage >= APILinkage::Reexported;
  }

  bool isReexported() const {
    return linkage == APILinkage::Reexported;
  }
};

enum class GVKind : uint8_t {
  Unknown = 0,
  Variable = 1,
  Function = 2,
};

struct GlobalRecord : APIRecord {
  GVKind kind;

  GlobalRecord(
    StringRef name,
    APILoc loc,
    APILinkage linkage,
    APIFlags flags,
    APIAccess access,
    GVKind kind,
    APIAvailability availability
  )
    : APIRecord(name, loc, linkage, flags, access, availability),
      kind(kind) {
  }
};

class API {
public:
  API(const llvm::Triple& triple) : target(triple) {
  }

  const llvm::Triple& getTarget() const {
    return target;
  }

  void addSymbol(
    StringRef symbol,
    APILoc loc,
    APILinkage linkage,
    APIFlags flags,
    APIAccess access,
    APIAvailability availability
  );

  void writeAPIJSONFile(raw_ostream& os, bool PrettyPrint = false);

private:
  const llvm::Triple target;

  llvm::BumpPtrAllocator allocator;
  std::vector<GlobalRecord *> globals;
};

} // end namespace apigen
} // end namespace w2n

#endif
