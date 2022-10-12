#ifndef W2N_AST_ASTCONTEXT_H
#define W2N_AST_ASTCONTEXT_H

#include <llvm/ADT/SetVector.h>
#include <llvm/Support/Allocator.h>
#include <w2n/AST/ASTAllocated.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/Evaluator.h>
#include <w2n/AST/Identifier.h>
#include <w2n/AST/Module.h>
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
    DiagnosticEngine& Diags);

  /// Members that should only be used by \file w2n/AST/ASTContext.cpp.
  struct Implementation;
  Implementation& getImpl() const;

public:
  ASTContext(const ASTContext&) = delete;
  void operator=(const ASTContext&) = delete;

  ~ASTContext();

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
    DiagnosticEngine& Diags);

#pragma Public Members

  /// The language options used for translation.
  const LanguageOptions& LangOpts;

  /// The source manager object.
  SourceManager& SourceMgr;

  /// Diags - The diagnostics engine.
  DiagnosticEngine& Diags;

  /// The request-evaluator that is used to process various requests.
  Evaluator Eval;

#pragma Error Handling

public:
  bool hadError() const { return false; }

#pragma ASTContext Managed Resource Allocation

private:
  /**
   * @brief Retrieve the allocator for the given arena.
   */
  llvm::BumpPtrAllocator&
  getAllocator(AllocationArena arena = AllocationArena::Permanent) const;

public:
  /// Allocate - Allocate memory from the ASTContext bump pointer.
  void * Allocate(
    unsigned long bytes,
    unsigned alignment,
    AllocationArena arena = AllocationArena::Permanent) const {
    if (bytes == 0)
      return nullptr;

    if (LangOpts.UsesMalloc)
      return AlignedAlloc(bytes, alignment);

    // FIXME: statistics

    return getAllocator(arena).Allocate(bytes, alignment);
  }

  template <typename T>
  T * Allocate(AllocationArena arena = AllocationArena::Permanent) const {
    T * res = (T *)Allocate(sizeof(T), alignof(T), arena);
    new (res) T();
    return res;
  }

  template <typename T>
  MutableArrayRef<T> AllocateUninitialized(
    unsigned NumElts,
    AllocationArena Arena = AllocationArena::Permanent) const {
    T * Data = (T *)Allocate(sizeof(T) * NumElts, alignof(T), Arena);
    return {Data, NumElts};
  }

  template <typename T>
  MutableArrayRef<T> Allocate(
    unsigned numElts,
    AllocationArena arena = AllocationArena::Permanent) const {
    T * res = (T *)Allocate(sizeof(T) * numElts, alignof(T), arena);
    for (unsigned i = 0; i != numElts; ++i)
      new (res + i) T();
    return {res, numElts};
  }

  /**
   * @brief Allocate a copy of the specified object.
   */
  template <typename T>
  typename std::remove_reference<T>::type * AllocateObjectCopy(
    T&& t,
    AllocationArena arena = AllocationArena::Permanent) const {
    // This function cannot be named AllocateCopy because it would always win
    // overload resolution over the AllocateCopy(ArrayRef<T>).
    using TNoRef = typename std::remove_reference<T>::type;
    TNoRef * res = (TNoRef *)Allocate(sizeof(TNoRef), alignof(TNoRef), arena);
    new (res) TNoRef(std::forward<T>(t));
    return res;
  }

  template <typename T, typename It>
  T * AllocateCopy(
    It start,
    It end,
    AllocationArena arena = AllocationArena::Permanent) const {
    T * res = (T *)Allocate(sizeof(T) * (end - start), alignof(T), arena);
    for (unsigned i = 0; start != end; ++start, ++i)
      new (res + i) T(*start);
    return res;
  }

  template <typename T, size_t N>
  MutableArrayRef<T> AllocateCopy(
    T (&array)[N],
    AllocationArena arena = AllocationArena::Permanent) const {
    return MutableArrayRef<T>(AllocateCopy<T>(array, array + N, arena), N);
  }

  template <typename T>
  MutableArrayRef<T> AllocateCopy(
    ArrayRef<T> array,
    AllocationArena arena = AllocationArena::Permanent) const {
    return MutableArrayRef<T>(
      AllocateCopy<T>(array.begin(), array.end(), arena), array.size());
  }

  template <typename T>
  MutableArrayRef<T> AllocateCopy(
    const std::vector<T>& vec,
    AllocationArena arena = AllocationArena::Permanent) const {
    return AllocateCopy(ArrayRef<T>(vec), arena);
  }

  template <typename T>
  ArrayRef<T> AllocateCopy(
    const SmallVectorImpl<T>& vec,
    AllocationArena arena = AllocationArena::Permanent) const {
    return AllocateCopy(ArrayRef<T>(vec), arena);
  }

  template <typename T>
  MutableArrayRef<T> AllocateCopy(
    SmallVectorImpl<T>& vec,
    AllocationArena arena = AllocationArena::Permanent) const {
    return AllocateCopy(MutableArrayRef<T>(vec), arena);
  }

  StringRef AllocateCopy(
    StringRef Str,
    AllocationArena arena = AllocationArena::Permanent) const {
    ArrayRef<char> Result =
      AllocateCopy(llvm::makeArrayRef(Str.data(), Str.size()), arena);
    return StringRef(Result.data(), Result.size());
  }

  template <typename T, typename Vector, typename Set>
  MutableArrayRef<T> AllocateCopy(
    llvm::SetVector<T, Vector, Set> setVector,
    AllocationArena arena = AllocationArena::Permanent) const {
    return MutableArrayRef<T>(
      AllocateCopy<T>(setVector.begin(), setVector.end(), arena),
      setVector.size());
  }

#pragma Configure Compilations

  void addLoadedModule(ModuleDecl * Module);

  /// Add a cleanup function to be called when the ASTContext is deallocated.
  void addCleanup(std::function<void(void)> cleanup);

  /// Add a cleanup to run the given object's destructor when the ASTContext is
  /// deallocated.
  template <typename T>
  void addDestructorCleanup(T& object) {
    addCleanup([&object] { object.~T(); });
  }

#pragma Getting ASTContext Managed Resources

public:
  /**
   * @brief Return the uniqued and AST-Context-owned version of the
   *  specified string.
   *
   * @param Str
   * @return Identifier
   */
  Identifier getIdentifier(StringRef Str) const;
};

} // namespace w2n

#endif // W2N_AST_ASTCONTEXT_H
