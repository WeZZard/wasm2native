/// This file defines data structures to efficiently support the request
/// evaluator's per-request caching and dependency tracking maps.

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <w2n/AST/DependencyCollector.h>

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
  using type = void_t<>;
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

  RequestKey(StorageKind kind) : Empty(), Kind(kind) {
    assert(kind != StorageKind::Normal);
  }

public:
  explicit RequestKey(Request req)
    : Req(std::move(req)),
      Kind(StorageKind::Normal) {
  }

  RequestKey(const RequestKey& other) : Empty(), Kind(other.Kind) {
    if (Kind == StorageKind::Normal)
      new (&Req) Request(other.Req);
  }

  RequestKey(RequestKey&& other) : Empty(), Kind(other.Kind) {
    if (Kind == StorageKind::Normal)
      new (&Req) Request(std::move(other.Req));
  }

  RequestKey& operator=(const RequestKey& other) {
    if (&other != this) {
      this->~RequestKey();
      new (this) RequestKey(other);
    }
    return *this;
  }

  RequestKey& operator=(RequestKey&& other) {
    if (&other != this) {
      this->~RequestKey();
      new (this) RequestKey(std::move(other));
    }
    return *this;
  }

  ~RequestKey() {
    if (Kind == StorageKind::Normal)
      Req.~Request();
  }

  bool isStorageEqual(const Request& req) const {
    if (Kind != StorageKind::Normal)
      return false;
    return Req == req;
  }

  friend bool operator==(const RequestKey& lhs, const RequestKey& rhs) {
    if (lhs.Kind == StorageKind::Normal && rhs.Kind == StorageKind::Normal) {
      return lhs.Req == rhs.Req;
    } else {
      return lhs.Kind == rhs.Kind;
    }
  }

  friend bool operator!=(const RequestKey& lhs, const RequestKey& rhs) {
    return !(lhs == rhs);
  }

  friend llvm::hash_code hash_value(const RequestKey& key) {
    if (key.Kind != StorageKind::Normal)
      return 1;
    return hash_value(key.Req);
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
  explicit RequestKey(Request req) : Req(std::move(req)) {
  }

  bool isStorageEqual(const Request& req) const {
    return Req == req;
  }

  friend bool operator==(const RequestKey& lhs, const RequestKey& rhs) {
    return lhs.Req == rhs.Req;
  }

  friend bool operator!=(const RequestKey& lhs, const RequestKey& rhs) {
    return !(lhs == rhs);
  }

  friend llvm::hash_code hash_value(const RequestKey& key) {
    return hash_value(key.Req);
  }
};

} // end namespace

/// Type-erased wrapper for caching results of a single type of request.
class PerRequestCache {
  void * Storage;
  std::function<void(void *)> Deleter;

  PerRequestCache(void * storage, std::function<void(void *)> deleter)
    : Storage(storage),
      Deleter(deleter) {
  }

public:
  PerRequestCache() : Storage(nullptr), Deleter([](void *) {}) {
  }

  PerRequestCache(PerRequestCache&& other)
    : Storage(other.Storage),
      Deleter(std::move(other.Deleter)) {
    other.Storage = nullptr;
  }

  PerRequestCache& operator=(PerRequestCache&& other) {
    if (&other != this) {
      this->~PerRequestCache();
      new (this) PerRequestCache(std::move(other));
    }
    return *this;
  }

  PerRequestCache(const PerRequestCache&) = delete;
  PerRequestCache& operator=(const PerRequestCache&) = delete;

  template <typename Request>
  static PerRequestCache makeEmpty() {
    using Map =
      llvm::DenseMap<RequestKey<Request>, typename Request::OutputType>;
    return PerRequestCache(new Map(), [](void * ptr) {
      delete static_cast<Map *>(ptr);
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
    return !Storage;
  }

  ~PerRequestCache() {
    if (Storage)
      Deleter(Storage);
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
  find_as(const Request& req) {
    auto * cache = getCache<Request>();
    return cache->find_as(req);
  }

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    typename Request::OutputType>::const_iterator
  end() {
    auto * cache = getCache<Request>();
    return cache->end();
  }

  template <typename Request>
  void insert(Request req, typename Request::OutputType val) {
    auto * cache = getCache<Request>();
    auto result =
      cache->insert({RequestKey<Request>(std::move(req)), std::move(val)}
      );
    assert(result.second && "Request result was already cached");
    (void)result;
  }

  template <typename Request>
  void erase(Request req) {
    auto * cache = getCache<Request>();
    cache->erase(RequestKey<Request>(std::move(req)));
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
    void * storage, std::function<void(void *)> deleter
  )
    : Storage(storage),
      Deleter(deleter) {
  }

public:
  PerRequestReferences() : Storage(nullptr), Deleter([](void *) {}) {
  }

  PerRequestReferences(PerRequestReferences&& other)
    : Storage(other.Storage),
      Deleter(std::move(other.Deleter)) {
    other.Storage = nullptr;
  }

  PerRequestReferences& operator=(PerRequestReferences&& other) {
    if (&other != this) {
      this->~PerRequestReferences();
      new (this) PerRequestReferences(std::move(other));
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
    return PerRequestReferences(new Map(), [](void * ptr) {
      delete static_cast<Map *>(ptr);
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
    return !Storage;
  }

  ~PerRequestReferences() {
    if (Storage)
      Deleter(Storage);
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
  find_as(const Request& req) {
    auto * refs = getRefs<Request>();
    return refs->find_as(req);
  }

  template <typename Request>
  typename llvm::DenseMap<
    RequestKey<Request>,
    std::vector<DependencyCollector::Reference>>::const_iterator
  end() {
    auto * refs = getRefs<Request>();
    return refs->end();
  }

  template <typename Request>
  void
  insert(Request req, std::vector<DependencyCollector::Reference> val) {
    auto * refs = getRefs<Request>();
    refs->insert({RequestKey<Request>(std::move(req)), std::move(val)});
  }

  template <typename Request>
  void erase(Request req) {
    auto * refs = getRefs<Request>();
    refs->erase(RequestKey<Request>(std::move(req)));
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

  static unsigned getHashValue(const RequestKey& key) {
    return hash_value(key);
  }

  static unsigned getHashValue(const Request& request) {
    return hash_value(request);
  }

  static bool isEqual(const RequestKey& lhs, const RequestKey& rhs) {
    return lhs == rhs;
  }

  static bool isEqual(const Request& lhs, const RequestKey& rhs) {
    return rhs.isStorageEqual(lhs);
  }
};

} // end namespace llvm

#endif // W2N_AST_REQUESTCACHE_H
