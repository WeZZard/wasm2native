/// This file defines type-erasing wrappers for requests used by the
/// \c Evaluator class.

#ifndef W2N_AST_ANYREQUEST_H
#define W2N_AST_ANYREQUEST_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/PointerIntPair.h>
#include <string>
#include <w2n/Basic/LLVMHashing.h>
#include <w2n/Basic/SourceLoc.h>
#include <w2n/Basic/TypeID.h>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace w2n {

using llvm::hash_code;
using llvm::hash_value;

class DiagnosticEngine;

/// A collection of functions that describe how to perform operations for
/// a specific concrete request, obtained using \c
/// AnyRequestVTable::get<ConcreteRequest>().
struct AnyRequestVTable {
  template <typename Request>
  struct Impl {
    static hash_code getHash(const void * Ptr) {
      return hash_value(*static_cast<const Request *>(Ptr));
    }

    static bool isEqual(const void * Lhs, const void * Rhs) {
      return *static_cast<const Request *>(Lhs)
          == *static_cast<const Request *>(Rhs);
    }

    static void simpleDisplay(const void * Ptr, llvm::raw_ostream& Out) {
      simple_display(Out, *static_cast<const Request *>(Ptr));
    }

    static void diagnoseCycle(const void * Ptr, DiagnosticEngine& Diags) {
      static_cast<const Request *>(Ptr)->diagnoseCycle(Diags);
    }

    static void noteCycleStep(const void * Ptr, DiagnosticEngine& Diags) {
      static_cast<const Request *>(Ptr)->noteCycleStep(Diags);
    }
  };

  const uint64_t RequestTypeID;
  const std::function<hash_code(const void *)> GetHash;
  const std::function<bool(const void *, const void *)> IsEqual;
  const std::function<void(const void *, llvm::raw_ostream&)>
    SimpleDisplay;
  const std::function<void(const void *, DiagnosticEngine&)>
    DiagnoseCycle;
  const std::function<void(const void *, DiagnosticEngine&)>
    NoteCycleStep;

  template <typename Request>
  static const AnyRequestVTable * get() {
    static const AnyRequestVTable VTable = {
      TypeID<Request>::value,
      &Impl<Request>::getHash,
      &Impl<Request>::isEqual,
      &Impl<Request>::simpleDisplay,
      &Impl<Request>::diagnoseCycle,
      &Impl<Request>::noteCycleStep};
    return &VTable;
  }
};

/// Base class for request type-erasing wrappers.
template <typename Derived>
class AnyRequestBase {
  friend llvm::DenseMapInfo<Derived>;

protected:

  static hash_code hashForHolder(uint64_t TypeId, hash_code RequestHash) {
    return hash_combine(TypeId, RequestHash);
  }

  enum class StorageKind : uint8_t {
    Normal,
    Empty,
    Tombstone,
  };

  /// The vtable and storage kind.
  llvm::PointerIntPair<const AnyRequestVTable *, 2, StorageKind>
    VTableAndKind;

  StorageKind getStorageKind() const {
    return VTableAndKind.getInt();
  }

  /// Whether this object is storing a value, and is not empty or a
  /// tombstone.
  bool hasStorage() const {
    switch (getStorageKind()) {
    case StorageKind::Empty:
    case StorageKind::Tombstone: return false;
    case StorageKind::Normal: return true;
    }
    llvm_unreachable("Unhandled case in switch");
  }

  /// Retrieve the vtable to perform operations on the type-erased
  /// request.
  const AnyRequestVTable * getVTable() const {
    assert(hasStorage() && "Shouldn't be querying empty or tombstone");
    return VTableAndKind.getPointer();
  }

  AnyRequestBase(const AnyRequestVTable * VTable, StorageKind Kind) {
    VTableAndKind.setPointer(VTable);
    VTableAndKind.setInt(Kind);
    assert(
      (bool)VTable == hasStorage() && "Must have a vtable with storage"
    );
  }

  AnyRequestBase(const AnyRequestBase& X) {
    VTableAndKind = X.VTableAndKind;
  }

  AnyRequestBase& operator=(const AnyRequestBase& X) {
    VTableAndKind = X.VTableAndKind;
    return *this;
  }

private:

  Derived& asDerived() {
    return *static_cast<Derived *>(this);
  }

  const Derived& asDerived() const {
    return *static_cast<const Derived *>(this);
  }

  const void * getRawStorage() const {
    return asDerived().getRawStorage();
  }

public:

  /// Cast to a specific (known) type.
  template <typename Request>
  const Request& castTo() const {
    assert(
      getVTable()->typeID == TypeID<Request>::value
      && "Wrong type in cast"
    );
    return *static_cast<const Request *>(getRawStorage());
  }

  /// Try casting to a specific (known) type, returning \c nullptr on
  /// failure.
  template <typename Request>
  const Request * getAs() const {
    if (getVTable()->typeID != TypeID<Request>::value) {
      return nullptr;
    }

    return static_cast<const Request *>(getRawStorage());
  }

  /// Diagnose a cycle detected for this request.
  void diagnoseCycle(DiagnosticEngine& Diags) const {
    getVTable()->DiagnoseCycle(getRawStorage(), Diags);
  }

  /// Note that this request is part of a cycle.
  void noteCycleStep(DiagnosticEngine& Diags) const {
    getVTable()->NoteCycleStep(getRawStorage(), Diags);
  }

  /// Compare two instances for equality.
  friend bool operator==(
    const AnyRequestBase<Derived>& Lhs, const AnyRequestBase<Derived>& Rhs
  ) {
    // If the storage kinds don't match, we're done.
    if (Lhs.getStorageKind() != Rhs.getStorageKind()) {
      return false;
    }

    // If the storage kinds do match, but there's no storage, they're
    // trivially equal.
    if (!Lhs.hasStorage()) {
      return true;
    }

    // Must be storing the same kind of request.
    if (Lhs.getVTable()->RequestTypeID != Rhs.getVTable()->RequestTypeID) {
      return false;
    }

    return Lhs.getVTable()->IsEqual(
      Lhs.getRawStorage(), Rhs.getRawStorage()
    );
  }

  friend bool operator!=(const Derived& Lhs, const Derived& Rhs) {
    return !(Lhs == Rhs);
  }

  friend hash_code hash_value(const AnyRequestBase<Derived>& Req) {
    // If there's no storage, return a trivial hash value.
    if (!Req.hasStorage()) {
      return 1;
    }

    auto ReqHash = Req.getVTable()->GetHash(Req.getRawStorage());
    return hashForHolder(Req.getVTable()->RequestTypeID, ReqHash);
  }

  friend void simple_display(
    llvm::raw_ostream& out, const AnyRequestBase<Derived>& subject
  ) {
    subject.getVTable()->SimpleDisplay(subject.getRawStorage(), out);
  }
};

/// Provides a view onto a request that is stored on the stack. Objects of
/// this class must not outlive the request they reference.
///
/// Requests must be value types and provide the following API:
///
///   - Equality operator (==)
///   - Hashing support (hash_value)
///   - TypeID support (see w2n/Basic/TypeID.h)
///   - Display support (free function):
///       void simple_display(llvm::raw_ostream &, const T &);
///   - Cycle diagnostics operations:
///       void diagnoseCycle(DiagnosticEngine &diags) const;
///       void noteCycleStep(DiagnosticEngine &diags) const;
///
class ActiveRequest final : public AnyRequestBase<ActiveRequest> {
  template <typename T>
  friend class AnyRequestBase;

  friend llvm::DenseMapInfo<ActiveRequest>;

  /// Pointer to the request stored on the stack.
  const void * Storage;

  /// Creates an \c ActiveRequest without storage.
  explicit ActiveRequest(StorageKind Kind) :
    AnyRequestBase(/*vtable*/ nullptr, Kind) {
  }

  const void * getRawStorage() const {
    return Storage;
  }

public:

  /// Creates a new \c ActiveRequest referencing a concrete request on the
  /// stack.
  template <typename Request>
  explicit ActiveRequest(const Request& Req) :
    AnyRequestBase(
      AnyRequestVTable::get<Request>(), StorageKind::Normal
    ) {
    Storage = &Req;
  }
};

} // end namespace w2n

namespace llvm {
template <>
struct DenseMapInfo<w2n::ActiveRequest> {
  using ActiveRequest = w2n::ActiveRequest;

  static inline ActiveRequest getEmptyKey() {
    return ActiveRequest(ActiveRequest::StorageKind::Empty);
  }

  static inline ActiveRequest getTombstoneKey() {
    return ActiveRequest(ActiveRequest::StorageKind::Tombstone);
  }

  static unsigned getHashValue(const ActiveRequest& Req) {
    return hash_value(Req);
  }

  static bool
  isEqual(const ActiveRequest& Lhs, const ActiveRequest& Rhs) {
    return Lhs == Rhs;
  }
};

} // end namespace llvm

#endif // W2N_AST_ANYREQUEST_H
