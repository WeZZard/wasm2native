#ifndef W2N_AST_ASTCONTEXT_H
#define W2N_AST_ASTCONTEXT_H

#include "llvm/ADT/Optional.h"
#include <llvm/ADT/SetVector.h>
#include <llvm/Support/Allocator.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/Decl.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/Evaluator.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/Type.h>
#include <w2n/Basic/LLVM.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/Malloc.h>
#include <w2n/Basic/SourceManager.h>
#include <w2n/Frontend/Frontend.h>

namespace w2n {

/**
 * @brief The context of ASTs of a WebAssebly compilation.
 *
 */
class ASTContext final {
private:

  ASTContext(
    LanguageOptions& LangOpts,
    SourceManager& SourceMgr,
    DiagnosticEngine& Diags
  );

  /// Members that should only be used by \file w2n/AST/ASTContext.cpp.
  struct Implementation;
  Implementation& getImpl() const;

public:

  ASTContext(const ASTContext&) = delete;
  void operator=(const ASTContext&) = delete;

  ~ASTContext();

  /// Optional table of counters to report, nullptr when not collecting.
  ///
  /// This must be initialized early so that Allocate() doesn't try to
  /// access it before being set to null.
  UnifiedStatsReporter * Stats = nullptr;

  void operator delete(void * Data) throw();

  /**
   * @brief \c ASTContext factory method.
   *
   * @note \c ASTContext has a complicated implementation setup to store
   * convenient/transient data. To hide this implementation details, it is
   * declared as separately at \file w2n/AST/ASTContext.cpp as
   * \c ASTContext::Implementation and this factory method is designed to
   * allocate memory both for \c ASTContext and
   * \c ASTContext::Implementation such that we can put
   * \c ASTContext::Implementation right after \c ASTContext .
   */
  static ASTContext * get(
    LanguageOptions& LangOpts,
    SourceManager& SourceMgr,
    DiagnosticEngine& Diags
  );

#pragma mark Public Members

  /// The language options used for translation.
  const LanguageOptions& LangOpts;

  /// The source manager object.
  SourceManager& SourceMgr;

  /// Diags - The diagnostics engine.
  DiagnosticEngine& Diags;

  /// The request-evaluator that is used to process various requests.
  Evaluator Eval;

#pragma mark Error Handling

  bool hadError() const {
    return false;
  }

#pragma mark ASTContext Managed Resource Allocation

private:

  /**
   * @brief Retrieve the allocator for the given arena.
   */
  llvm::BumpPtrAllocator&
  getAllocator(AllocationArena Arena = AllocationArena::Permanent) const;

public:

  /// Allocate - Allocate memory from the ASTContext bump pointer.
  void * allocate(
    unsigned long Bytes,
    unsigned Alignment,
    AllocationArena Arena = AllocationArena::Permanent
  ) const {
    if (Bytes == 0) {
      return nullptr;
    }
    if (LangOpts.UsesMalloc) {
      return alignedAlloc(Bytes, Alignment);
    }
    // FIXME: statistics

    return getAllocator(Arena).Allocate(Bytes, Alignment);
  }

  template <typename T>
  T * allocate(AllocationArena Arena = AllocationArena::Permanent) const {
    T * Res = (T *)allocate(sizeof(T), alignof(T), Arena);
    new (Res) T();
    return Res;
  }

  template <typename T>
  MutableArrayRef<T> allocateUninitialized(
    unsigned NumElts, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    T * Data = (T *)allocate(sizeof(T) * NumElts, alignof(T), Arena);
    return {Data, NumElts};
  }

  template <typename T>
  MutableArrayRef<T> allocate(
    unsigned NumElts, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    T * Res = (T *)allocate(sizeof(T) * NumElts, alignof(T), Arena);
    for (unsigned I = 0; I != NumElts; ++I) {
      new (Res + I) T();
    }
    return {Res, NumElts};
  }

  /**
   * @brief Allocate a copy of the specified object.
   */
  template <typename T>
  typename std::remove_reference<T>::type * allocateObjectCopy(
    T&& O, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    // This function cannot be named allocateCopy because it would always
    // win overload resolution over the allocateCopy(ArrayRef<T>).
    using TNoRef = typename std::remove_reference<T>::type;
    TNoRef * Res =
      (TNoRef *)allocate(sizeof(TNoRef), alignof(TNoRef), Arena);
    new (Res) TNoRef(std::forward<T>(O));
    return Res;
  }

  template <typename T, typename It>
  T * allocateCopy(
    It Start, It End, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    T * Res = (T *)allocate(sizeof(T) * (End - Start), alignof(T), Arena);
    for (unsigned I = 0; Start != End; ++Start, ++I) {
      new (Res + I) T(*Start);
    }
    return Res;
  }

  template <typename T, size_t N>
  MutableArrayRef<T> allocateCopy(
    T (&Array)[N], AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return MutableArrayRef<T>(
      AllocateCopy<T>(Array, Array + N, Arena), N
    );
  }

  template <typename T>
  MutableArrayRef<T> allocateCopy(
    ArrayRef<T> Array, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return MutableArrayRef<T>(
      allocateCopy<T>(Array.begin(), Array.end(), Arena), Array.size()
    );
  }

  template <typename T>
  MutableArrayRef<T> allocateCopy(
    const std::vector<T>& Vec,
    AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return AllocateCopy(ArrayRef<T>(Vec), Arena);
  }

  template <typename T>
  ArrayRef<T> allocateCopy(
    const SmallVectorImpl<T>& Vec,
    AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return AllocateCopy(ArrayRef<T>(Vec), Arena);
  }

  template <typename T>
  MutableArrayRef<T> allocateCopy(
    SmallVectorImpl<T>& Vec,
    AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return allocateCopy(MutableArrayRef<T>(Vec), Arena);
  }

  StringRef allocateCopy(
    StringRef Str, AllocationArena Arena = AllocationArena::Permanent
  ) const {
    ArrayRef<char> Result =
      allocateCopy(llvm::makeArrayRef(Str.data(), Str.size()), Arena);
    return StringRef(Result.data(), Result.size());
  }

  template <typename T, typename Vector, typename Set>
  MutableArrayRef<T> allocateCopy(
    llvm::SetVector<T, Vector, Set> SetVector,
    AllocationArena Arena = AllocationArena::Permanent
  ) const {
    return MutableArrayRef<T>(
      AllocateCopy<T>(SetVector.begin(), SetVector.end(), Arena),
      SetVector.size()
    );
  }

  /// Set a new stats reporter.
  void setStatsReporter(UnifiedStatsReporter * NewValue);

#pragma mark Configure Compilations

  void addLoadedModule(ModuleDecl * Module);

  /// Add a cleanup function to be called when the ASTContext is
  /// deallocated.
  void addCleanup(std::function<void(void)> Cleanup);

  /// Add a cleanup to run the given object's destructor when the
  /// ASTContext is deallocated.
  template <typename T>
  void addDestructorCleanup(T& Object) {
    addCleanup([&Object] { Object.~T(); });
  }

#pragma mark Getting ASTContext Managed Resources

  /**
   * @brief Return the uniqued and AST-Context-owned version of the
   *  specified string.
   *
   * @param Str
   * @return Identifier
   */
  Identifier getIdentifier(StringRef Str) const;

  /**
   * @brief Return the uniqued and AST-Context-owned \c ValueType instance
   * for given \c ValueTypeKind .
   *
   * @param Kind
   * @return ValueType *
   */
  ValueType * getValueTypeForKind(ValueTypeKind Kind) const;

#define TYPE(Id, Parent)
#define VALUE_TYPE(Id, Parent) Id##Type * get##Id##Type() const;
#include <w2n/AST/TypeNodes.def>

  ResultType * getResultType(std::vector<ValueType *> ValueTypes) const;

  FuncType * getFuncType(ResultType * Params, ResultType * Returns) const;

  GlobalType * getGlobalType(ValueType * Type, bool IsMutable) const;

  LimitsType *
  getLimits(uint64_t Min, llvm::Optional<uint64_t> Max) const;

  TableType *
  getTableType(ReferenceType * ElementType, LimitsType * Limits) const;

  MemoryType * getMemoryType(LimitsType * Limits) const;

  TypeIndexType * getTypeIndexType(uint32_t TypeIndex) const;
};

} // namespace w2n

#endif // W2N_AST_ASTCONTEXT_H
