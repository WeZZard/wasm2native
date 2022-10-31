#include <llvm/ADT/MapVector.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/ASTContext.h>

using namespace w2n;

#pragma mark ASTAllocated

void * detail::allocateInASTContext(
  size_t bytes,
  const ASTContext& ctx,
  AllocationArena arena,
  unsigned alignment
) {
  return ctx.Allocate(bytes, alignment, arena);
}

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
  auto * pointer =
    reinterpret_cast<char *>(const_cast<ASTContext *>(this));
  auto offset = llvm::alignAddr(
    (void *)sizeof(*this), llvm::Align(alignof(Implementation))
  );
  return *reinterpret_cast<Implementation *>(pointer + offset);
}

void ASTContext::operator delete(void * Data) throw() {
  AlignedFree(Data);
}

ASTContext * ASTContext::get(
  LanguageOptions& langOpts,
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
  return new (RetAddr) ASTContext(langOpts, SourceMgr, Diags);
}

llvm::BumpPtrAllocator& ASTContext::getAllocator(AllocationArena arena
) const {
  switch (arena) {
  case AllocationArena::Permanent: return getImpl().Allocator;
  }
  llvm_unreachable("bad AllocationArena");
}

/// Set a new stats reporter.
void ASTContext::setStatsReporter(UnifiedStatsReporter * stats) {
  if (stats) {
    stats->getFrontendCounters().NumASTBytesAllocated =
      getAllocator().getBytesAllocated();
  }
  Eval.setStatsReporter(stats);
  Stats = stats;
}

void ASTContext::addLoadedModule(ModuleDecl * Module) {
  assert(Module);
  // Add a loaded module using an actual module name (physical name on
  // disk).
  getImpl().LoadedModules[Module->getName()] = Module;
}

void ASTContext::addCleanup(std::function<void(void)> cleanup) {
  getImpl().Cleanups.push_back(std::move(cleanup));
}

Identifier ASTContext::getIdentifier(StringRef Str) const {
  // Make sure null pointers stay null.
  if (Str.data() == nullptr)
    return Identifier(nullptr);

  auto pair = std::make_pair(Str, Identifier::Aligner());
  auto I = getImpl().IdentifierTable.insert(pair).first;
  return Identifier(I->getKeyData());
}
