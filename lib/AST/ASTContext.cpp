#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/Unimplemented.h>

using namespace w2n;

#pragma mark ASTAllocated

void * detail::allocateInASTContext(
  size_t Bytes,
  const ASTContext& Ctx,
  AllocationArena Arena,
  unsigned Alignment
) {
  return Ctx.Allocate(Bytes, Alignment, Arena);
}

namespace w2n {

#pragma mark Type Keys

template <typename... Inputs>
class TypeKey {
  friend struct llvm::DenseMapInfo<TypeKey>;

  enum class StorageKind : uint8_t {
    Normal,
    Empty,
    Tombstone
  };
  StorageKind Kind;

  std::tuple<Inputs...> Storage;

  TypeKey(StorageKind Kind) : Kind(Kind), Storage() {
  }

public:

  static TypeKey getEmpty() {
    return TypeKey(StorageKind::Empty);
  }

  static TypeKey getTombstone() {
    return TypeKey(StorageKind::Tombstone);
  }

  TypeKey(const Inputs&... Storage) :
    Kind(StorageKind::Normal),
    Storage(Storage...) {
  }
};

using ResultTypeKey = TypeKey<std::vector<ValueType *>>;

using FuncTypeKey = TypeKey<ResultType *, ResultType *>;

using TableTypeKey = TypeKey<ReferenceType *, LimitsType *>;

using LimitsKey = TypeKey<uint64_t, llvm::Optional<uint64_t>>;

using GlobalTypeKey = TypeKey<ValueType *, bool>;

using MemoryTypeKey = TypeKey<LimitsType *>;

using TypeIndexTypeKey = TypeKey<uint32_t>;

/// \c TypeKey storage hash support.
template <typename T>
hash_code hash_value(const std::vector<T *>& Vec) {
  return llvm::hash_combine_range(Vec.begin(), Vec.end());
}

} // namespace w2n

namespace llvm {

template <typename... Inputs>
struct DenseMapInfo<TypeKey<Inputs...>> {
  using Key = TypeKey<Inputs...>;

  static inline Key getEmptyKey() {
    return Key::getEmpty();
  }

  static inline Key getTombstoneKey() {
    return Key::getTombstone();
  }

  static unsigned getHashValue(const Key& K) {
    return llvm::hash_combine(K.Kind, K.Storage);
  }

  static bool isEqual(const Key& Lhs, const Key& Rhs) {
    return Lhs.Kind == Rhs.Kind && Lhs.Storage == Rhs.Storage;
  }
};

} // namespace llvm

#pragma mark ASTContext::Implementation

struct ASTContext::Implementation {
  /**
   * @brief Structure that captures data to different kinds of "seats".
   */
  struct Arena {
    ~Arena() {
    }

    size_t getTotalMemory() const {
      return sizeof(*this)
           // Adds up sizes of seats in arena with
           // llvm::capacity_in_bytes
           + 0;
    }
  };

  llvm::BumpPtrAllocator Allocator; // used in later initializations

  /// The set of cleanups to be called when the ASTContext is destroyed.
  std::vector<std::function<void(void)>> Cleanups;

  /// The set of top-level modules we have loaded.
  /// This map is used for iteration, therefore it's a MapVector and not a
  /// DenseMap.
  llvm::MapVector<Identifier, ModuleDecl *> LoadedModules;

  /// FIXME: This is a \c StringMap rather than a \c StringSet because
  /// \c StringSet doesn't allow passing in a pre-existing allocator.
  llvm::StringMap<Identifier::Aligner, llvm::BumpPtrAllocator&>
    IdentifierTable;

  Arena Permanent;

  // ASTContext owned ValueType storages

#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent) Id##Type * Id##Type = nullptr;
#include <w2n/AST/TypeNodes.def>

  llvm::DenseMap<ResultTypeKey, ResultType *> ResultTypes;

  llvm::DenseMap<FuncTypeKey, FuncType *> FuncTypes;

  llvm::DenseMap<TableTypeKey, TableType *> TableTypes;

  llvm::DenseMap<LimitsKey, LimitsType *> Limits;

  llvm::DenseMap<GlobalTypeKey, GlobalType *> GlobalTypes;

  llvm::DenseMap<MemoryTypeKey, MemoryType *> MemoryTypes;

  llvm::DenseMap<TypeIndexTypeKey, TypeIndexType *> TypeIndexTypes;

  Implementation() : IdentifierTable(Allocator) {
  }

  ~Implementation() {
    for (auto& EachCleanup : Cleanups) {
      EachCleanup();
    }
  }
};

#pragma mark ASTContext

ASTContext::ASTContext(
  LanguageOptions& LangOpts,
  SourceManager& SourceMgr,
  DiagnosticEngine& Diags
) :
  LangOpts(LangOpts),
  SourceMgr(SourceMgr),
  Diags(Diags),
  Eval(Diags, LangOpts) {
}

ASTContext::~ASTContext() {
  getImpl().~Implementation();
}

ASTContext::Implementation& ASTContext::getImpl() const {
  auto * Pointer =
    reinterpret_cast<char *>(const_cast<ASTContext *>(this));
  auto Offset = llvm::alignAddr(
    (void *)sizeof(*this), llvm::Align(alignof(Implementation))
  );
  return *reinterpret_cast<Implementation *>(Pointer + Offset);
}

void ASTContext::operator delete(void * Data) throw() {
  AlignedFree(Data);
}

ASTContext * ASTContext::get(
  LanguageOptions& LangOpts,
  SourceManager& SourceMgr,
  DiagnosticEngine& Diags
) {
  // If more than two data structures are concatentated, then the
  // aggregate size math needs to become more complicated due to
  // per-struct alignment constraints.
  auto Align = std::max(alignof(ASTContext), alignof(Implementation));
  auto Size =
    llvm::alignTo(sizeof(ASTContext) + sizeof(Implementation), Align);
  auto * RetAddr = AlignedAlloc(Size, Align);
  auto * ImplAddr =
    reinterpret_cast<void *>((char *)RetAddr + sizeof(ASTContext));
  ImplAddr = reinterpret_cast<void *>(
    llvm::alignAddr(ImplAddr, llvm::Align(alignof(Implementation)))
  );
  new (ImplAddr) Implementation();
  return new (RetAddr) ASTContext(LangOpts, SourceMgr, Diags);
}

llvm::BumpPtrAllocator& ASTContext::getAllocator(AllocationArena Arena
) const {
  switch (Arena) {
  case AllocationArena::Permanent: return getImpl().Allocator;
  }
  llvm_unreachable("bad AllocationArena");
}

/// Set a new stats reporter.
void ASTContext::setStatsReporter(UnifiedStatsReporter * NewValue) {
  if (NewValue != nullptr) {
    NewValue->getFrontendCounters().NumASTBytesAllocated =
      getAllocator().getBytesAllocated();
  }
  Eval.setStatsReporter(NewValue);
  Stats = NewValue;
}

void ASTContext::addLoadedModule(ModuleDecl * Module) {
  assert(Module);
  // Add a loaded module using an actual module name (physical name on
  // disk).
  getImpl().LoadedModules[Module->getName()] = Module;
}

void ASTContext::addCleanup(std::function<void(void)> Cleanup) {
  getImpl().Cleanups.push_back(std::move(Cleanup));
}

Identifier ASTContext::getIdentifier(StringRef Str) const {
  // Make sure null pointers stay null.
  if (Str.data() == nullptr) {
    return Identifier(nullptr);
  }

  auto Pair = std::make_pair(Str, Identifier::Aligner());
  auto I = getImpl().IdentifierTable.insert(Pair).first;
  return Identifier(I->getKeyData());
}

ValueType * ASTContext::getValueTypeForKind(ValueTypeKind Kind) const {
  switch (Kind) {
  case ValueTypeKind::None: return nullptr;
#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent)                                           \
  case ValueTypeKind::Id:                                                \
    return get##Id##Type();
#include <w2n/AST/TypeNodes.def>
  }
}

#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent)                                           \
  Id##Type * ASTContext::get##Id##Type() const {                         \
    if (getImpl().Id##Type != nullptr) {                                 \
      return getImpl().Id##Type;                                         \
    }                                                                    \
                                                                         \
    getImpl().Id##Type =                                                 \
      Id##Type::create(const_cast<ASTContext&>(*this));                  \
    return getImpl().Id##Type;                                           \
  }
#include <w2n/AST/TypeNodes.def>

ResultType * ASTContext::getResultType(std::vector<ValueType *> ValueTypes
) const {
  auto Key = ResultTypeKey(ValueTypes);
  auto Iter = getImpl().ResultTypes.find(Key);
  if (Iter != getImpl().ResultTypes.end()) {
    return Iter->getSecond();
  }
  ResultType * Ty =
    ResultType::create(const_cast<ASTContext&>(*this), ValueTypes);
  getImpl().ResultTypes.insert({Key, Ty});
  return Ty;
}

FuncType *
ASTContext::getFuncType(ResultType * Params, ResultType * Returns) const {
  auto Key = FuncTypeKey(Params, Returns);
  auto Iter = getImpl().FuncTypes.find(Key);
  if (Iter != getImpl().FuncTypes.end()) {
    return Iter->getSecond();
  }
  FuncType * Ty =
    FuncType::create(const_cast<ASTContext&>(*this), Params, Returns);
  getImpl().FuncTypes.insert({Key, Ty});
  return Ty;
}

GlobalType *
ASTContext::getGlobalType(ValueType * Type, bool IsMutable) const {
  auto Key = GlobalTypeKey(Type, IsMutable);
  auto Iter = getImpl().GlobalTypes.find(Key);
  if (Iter != getImpl().GlobalTypes.end()) {
    return Iter->getSecond();
  }
  GlobalType * Ty =
    GlobalType::create(const_cast<ASTContext&>(*this), Type, IsMutable);
  getImpl().GlobalTypes.insert({Key, Ty});
  return Ty;
}

LimitsType *
ASTContext::getLimits(uint64_t Min, llvm::Optional<uint64_t> Max) const {
  auto Key = LimitsKey(Min, Max);
  auto Iter = getImpl().Limits.find(Key);
  if (Iter != getImpl().Limits.end()) {
    return Iter->getSecond();
  }
  LimitsType * Limits =
    LimitsType::create(const_cast<ASTContext&>(*this), Min, Max);
  getImpl().Limits.insert({Key, Limits});
  return Limits;
}

TableType * ASTContext::getTableType(
  ReferenceType * ElementType, LimitsType * Limits
) const {
  auto Key = TableTypeKey(ElementType, Limits);
  auto Iter = getImpl().TableTypes.find(Key);
  if (Iter != getImpl().TableTypes.end()) {
    return Iter->getSecond();
  }
  TableType * Ty = TableType::create(
    const_cast<ASTContext&>(*this), ElementType, Limits
  );
  getImpl().TableTypes.insert({Key, Ty});
  return Ty;
}

MemoryType * ASTContext::getMemoryType(LimitsType * Limits) const {
  auto Key = MemoryTypeKey(Limits);
  auto Iter = getImpl().MemoryTypes.find(Key);
  if (Iter != getImpl().MemoryTypes.end()) {
    return Iter->getSecond();
  }
  MemoryType * Ty =
    MemoryType::create(const_cast<ASTContext&>(*this), Limits);
  getImpl().MemoryTypes.insert({Key, Ty});
  return Ty;
}

TypeIndexType * ASTContext::getTypeIndexType(uint32_t TypeIndex) const {
  auto Key = TypeIndexTypeKey(TypeIndex);
  auto Iter = getImpl().TypeIndexTypes.find(Key);
  if (Iter != getImpl().TypeIndexTypes.end()) {
    return Iter->getSecond();
  }
  TypeIndexType * Ty =
    TypeIndexType::create(const_cast<ASTContext&>(*this), TypeIndex);
  getImpl().TypeIndexTypes.insert({Key, Ty});
  return Ty;
}
