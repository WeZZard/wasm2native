/// This file defines data structures to efficiently support the request
/// evaluator's per-request caching and dependency tracking maps.

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <vector>
#include <w2n/AST/DependencyCollector.h>
#include <w2n/Basic/TypeID.h>

#ifndef W2N_AST_REQUESTCACHE_H
#define W2N_AST_REQUESTCACHE_H

namespace w2n {

namespace evaluator {

namespace detail {

// Remove this when the compiler bumps to C++17.
template <typename...>
using void_t = void;

template <typename T, typename = void_t<>>

struct TupleHasDenseMapInfo {};

template <typename... Ts>
struct TupleHasDenseMapInfo<
  std::tuple<Ts...>,
  void_t<decltype(llvm::DenseMapInfo<Ts>::getEmptyKey)...>> {
  using Type = void_t<>;
};

} // end namespace detail

namespace {

/// Wrapper for a request with additional empty and tombstone states.
template <typename Request, typename = detail::void_t<>>
class RequestKey {
  friend struct llvm::DenseMapInfo<RequestKey>;

  union {
    char Empty;
    Request Req;
  };

  enum class StorageKind : uint8_t {
    Normal,
    Empty,
    Tombstone
  };
  StorageKind Kind;

  static RequestKey getEmpty() {
    return RequestKey(StorageKind::Empty);
  }

  static RequestKey getTombstone() {
    return RequestKey(StorageKind::Tombstone);
  }

  RequestKey(StorageKind Kind) : Empty(), Kind(Kind) {
    assert(Kind != StorageKind::Normal);
  }

public:

  explicit RequestKey(Request Req) :
    Req(std::move(Req)),
    Kind(StorageKind::Normal) {
  }

  RequestKey(const RequestKey& X) : Empty(), Kind(X.Kind) {
    if (Kind == StorageKind::Normal) {
      new (&Req) Request(X.Req);
    }
  }

  RequestKey(RequestKey&& X) : Empty(), Kind(X.Kind) {
    if (Kind == StorageKind::Normal) {
      new (&Req) Request(std::move(X.Req));
    }
  }

  RequestKey& operator=(const RequestKey& X) {
    if (&X != this) {
      this->~RequestKey();
      new (this) RequestKey(X);
    }
    return *this;
  }

  RequestKey& operator=(RequestKey&& X) {
    if (&X != this) {
      this->~RequestKey();
      new (this) RequestKey(std::move(X));
    }
    return *this;
  }

  ~RequestKey() {
    if (Kind == StorageKind::Normal) {
      Req.~Request();
    }
  }

  bool isStorageEqual(const Request& Req) const {
    if (Kind != StorageKind::Normal) {
      return false;
    }
    return Req == Req;
  }

  friend bool operator==(const RequestKey& Lhs, const RequestKey& Rhs) {
    if (Lhs.Kind == StorageKind::Normal && Rhs.Kind == StorageKind::Normal) {
      return Lhs.Req == Rhs.Req;
    }
    return Lhs.Kind == Rhs.Kind;
  }

  friend bool operator!=(const RequestKey& Lhs, const RequestKey& Rhs) {
    return !(Lhs == Rhs);
  }

  friend llvm::hash_code hash_value(const RequestKey& hs) {
    if (hs.Kind != StorageKind::Normal) {
      return 1;
    }
    return hash_value(hs.Req);
  }
};

template <typename Request>
class RequestKey<
  Request,
  typename detail::TupleHasDenseMapInfo<
    typename Request::Storage>::type> {
  friend struct llvm::DenseMapInfo<RequestKey>;
  using Info = llvm::DenseMapInfo<typename Request::Storage>;

  Request Req;

  static RequestKey getEmpty() {
    return RequestKey(Request(Info::getEmptyKey()));
  }

  static RequestKey getTombstone() {
    return RequestKey(Request(Info::getTombstoneKey()));
  }

public:

  explicit RequestKey(Request Req) : Req(std::move(Req)) {
  }

  bool isStorageEqual(const Request& Req) const {
    return Req == Req;
  }

  friend bool operator==(const RequestKey& Lhs, const RequestKey& Rhs) {
    return Lhs.Req == Rhs.Req;
  }

  friend bool operator!=(const RequestKey& Lhs, const RequestKey& Rhs) {
    return !(Lhs == Rhs);
  }

  friend llvm::hash_code hash_value(const RequestKey& hs) {
    return hash_value(hs.Req);
  }
};

} // end namespace

/// Type-erased wrapper for caching results of a single type of request.
class PerRequestCache {
  void * Storage;
  std::function<void(void *)> Deleter;

  PerRequestCache(void * Storage, std::function<void(void *)> Deleter) :
    Storage(Storage),
    Deleter(Deleter) {
  }

public:

  PerRequestCache() : Storage(nullptr), Deleter([](void *) {}) {
  }

  PerRequestCache(PerRequestCache&& X) :
    Storage(X.Storage),
    Deleter(std::move(X.Deleter)) {
    X.Storage = nullptr;
  }

  PerRequestCache& operator=(PerRequestCache&& X) {
    if (&X != this) {
      this->~PerRequestCache();
      new (this) PerRequestCache(std::move(X));
    }
    return *this;
  }

  PerRequestCache(const PerRequestCache&) = delete;
  PerRequestCache& operator=(const PerRequestCache&) = delete;

  template <typename Request>
  static PerRequestCache makeEmpty() {
    using Map =
      llvm::DenseMap<RequestKey<Request>, typename Request::OutputType>;
    return PerRequestCache(new Map(), [](void * Ptr) {
      delete static_cast<Map *>(Ptr);
    });
  }

  template <typename Request>
  llvm::DenseMap<RequestKey<Request>, typename Request::OutputType> *
  get() const {
    using Map =
      llvm::DenseMap<RequestKey<Request>, typename Request::OutputType>;
    assert(Storage);
    return static_cast<Map *>(Storage);
  }

  bool isNull() const {
    return Storage == nullptr;
  }

  ~PerRequestCache() {
    if (Storage != nullptr) {
      Deleter(Storage);
    }
  }
};

/// Data structure for caching results of requests. Sharded by the type ID
/// zone and request kind, with a PerRequestCache for each request kind.
///
/// Conceptually equivalent to DenseMap<AnyRequest, AnyValue>, but without
/// type erasure overhead for keys and values.
class RequestCache {
#define W2N_TYPEID_ZONE(Name, Id)                                        \
  std::vector<PerRequestCache> Name##ZoneCache;                          \
                                                                         \
  template <                                                             \
    typename Request,                                                    \
    typename ZoneTypes = TypeIDZoneTypes<Zone::Name>,                    \
    typename std::enable_if<                                             \
      TypeID<Request>::zone == Zone::Name>::type * = nullptr>            \
  llvm::DenseMap<RequestKey<Request>, typename Request::OutputType> *    \
  getCache() {                                                           \
    auto& caches = Name##ZoneCache;                                      \
    if (caches.empty()) {                                                \
      caches.resize(ZoneTypes::Count);                                   \
    }                                                                    \
    auto idx = TypeID<Request>::localID;                                 \
    if (caches[idx].isNull()) {                                          \
      caches[idx] = PerRequestCache::makeEmpty<Request>();               \
    }                                                                    \
    return caches[idx].template get<Request>();                          \
  }
#include <w2n/Basic/TypeIDZones.def>
#undef W2N_TYPEID_ZONE

public:

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    typename Request::OutputType>::const_iterator
  find_as(const Request& Req) {
    auto * Cache = getCache<Request>();
    return Cache->find_as(Req);
  }

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    typename Request::OutputType>::const_iterator
  end() {
    auto * Cache = getCache<Request>();
    return Cache->end();
  }

  template <typename Request>
  void insert(Request Req, typename Request::OutputType Val) {
    auto * Cache = getCache<Request>();
    auto Result =
      Cache->insert({RequestKey<Request>(std::move(Req)), std::move(Val)}
      );
    assert(Result.second && "Request result was already cached");
    (void)Result;
  }

  template <typename Request>
  void erase(Request Req) {
    auto * Cache = getCache<Request>();
    Cache->erase(RequestKey<Request>(std::move(Req)));
  }

  void clear() {
#define W2N_TYPEID_ZONE(Name, Id) Name##ZoneCache.clear();
#include <w2n/Basic/TypeIDZones.def>
#undef W2N_TYPEID_ZONE
  }
};

/// Type-erased wrapper for caching dependencies from a single type of
/// request.
class PerRequestReferences {
  void * Storage;
  std::function<void(void *)> Deleter;

  PerRequestReferences(
    void * Storage, std::function<void(void *)> Deleter
  ) :
    Storage(Storage),
    Deleter(Deleter) {
  }

public:

  PerRequestReferences() : Storage(nullptr), Deleter([](void *) {}) {
  }

  PerRequestReferences(PerRequestReferences&& X) :
    Storage(X.Storage),
    Deleter(std::move(X.Deleter)) {
    X.Storage = nullptr;
  }

  PerRequestReferences& operator=(PerRequestReferences&& X) {
    if (&X != this) {
      this->~PerRequestReferences();
      new (this) PerRequestReferences(std::move(X));
    }
    return *this;
  }

  PerRequestReferences(const PerRequestReferences&) = delete;
  PerRequestReferences& operator=(const PerRequestReferences&) = delete;

  template <typename Request>
  static PerRequestReferences makeEmpty() {
    using Map = llvm::DenseMap<
      RequestKey<Request>,
      std::vector<DependencyCollector::Reference>>;
    return PerRequestReferences(new Map(), [](void * Ptr) {
      delete static_cast<Map *>(Ptr);
    });
  }

  template <typename Request>
  llvm::DenseMap<
    RequestKey<Request>,
    std::vector<DependencyCollector::Reference>> *
  get() const {
    using Map = llvm::DenseMap<
      RequestKey<Request>,
      std::vector<DependencyCollector::Reference>>;
    assert(Storage);
    return static_cast<Map *>(Storage);
  }

  bool isNull() const {
    return Storage == nullptr;
  }

  ~PerRequestReferences() {
    if (Storage != nullptr) {
      Deleter(Storage);
    }
  }
};

/// Data structure for caching dependencies from requests. Sharded by the
/// type ID zone and request kind, with a PerRequestReferences for each
/// request kind.
///
/// Conceptually equivalent to DenseMap<AnyRequest, vector<Reference>>,
/// but without type erasure overhead for keys.
class RequestReferences {
#define W2N_TYPEID_ZONE(Name, Id)                                        \
  std::vector<PerRequestReferences> Name##ZoneRefs;                      \
                                                                         \
  template <                                                             \
    typename Request,                                                    \
    typename ZoneTypes = TypeIDZoneTypes<Zone::Name>,                    \
    typename std::enable_if<                                             \
      TypeID<Request>::zone == Zone::Name>::type * = nullptr>            \
  llvm::DenseMap<                                                        \
    RequestKey<Request>,                                                 \
    std::vector<DependencyCollector::Reference>> *                       \
  getRefs() {                                                            \
    auto& refs = Name##ZoneRefs;                                         \
    if (refs.empty()) {                                                  \
      refs.resize(ZoneTypes::Count);                                     \
    }                                                                    \
    auto idx = TypeID<Request>::localID;                                 \
    if (refs[idx].isNull()) {                                            \
      refs[idx] = PerRequestReferences::makeEmpty<Request>();            \
    }                                                                    \
    return refs[idx].template get<Request>();                            \
  }
#include <w2n/Basic/TypeIDZones.def>
#undef W2N_TYPEID_ZONE

public:

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    std::vector<DependencyCollector::Reference>>::const_iterator
  find_as(const Request& Req) {
    auto * Refs = getRefs<Request>();
    return Refs->find_as(Req);
  }

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    std::vector<DependencyCollector::Reference>>::const_iterator
  end() {
    auto * Refs = getRefs<Request>();
    return Refs->end();
  }

  template <typename Request>
  void
  insert(Request Req, std::vector<DependencyCollector::Reference> Val) {
    auto * Refs = getRefs<Request>();
    Refs->insert({RequestKey<Request>(std::move(Req)), std::move(Val)});
  }

  template <typename Request>
  void erase(Request Req) {
    auto * Refs = getRefs<Request>();
    Refs->erase(RequestKey<Request>(std::move(Req)));
  }

  void clear() {
#define W2N_TYPEID_ZONE(Name, Id) Name##ZoneRefs.clear();
#include <w2n/Basic/TypeIDZones.def>
#undef W2N_TYPEID_ZONE
  }
};

} // end namespace evaluator

} // end namespace w2n

namespace llvm {

template <typename Request, typename Info>
struct DenseMapInfo<w2n::evaluator::RequestKey<Request, Info>> {
  using RequestKey = w2n::evaluator::RequestKey<Request, Info>;

  static inline RequestKey getEmptyKey() {
    return RequestKey::getEmpty();
  }

  static inline RequestKey getTombstoneKey() {
    return RequestKey::getTombstone();
  }

  static unsigned getHashValue(const RequestKey& Key) {
    return hash_value(Key);
  }

  static unsigned getHashValue(const Request& Req) {
    return hash_value(Req);
  }

  static bool isEqual(const RequestKey& Lhs, const RequestKey& Rhs) {
    return Lhs == Rhs;
  }

  static bool isEqual(const Request& Lhs, const RequestKey& Rhs) {
    return Rhs.isStorageEqual(Lhs);
  }
};

} // end namespace llvm

#endif // W2N_AST_REQUESTCACHE_H
