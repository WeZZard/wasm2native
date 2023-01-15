///  This file defines type checking requests.

#ifndef W2N_TYPE_CHECK_REQUESTS_H
#define W2N_TYPE_CHECK_REQUESTS_H

#include <memory>
#include <w2n/AST/Evaluator.h>
#include <w2n/AST/EvaluatorDependencies.h>
#include <w2n/AST/GlobalVariable.h>
#include <w2n/AST/Module.h>
#include <w2n/AST/SimpleRequest.h>

namespace w2n {
class SourceFile;
class ModuleDecl;

/// Retrieves the primary source files in the main module.
// FIXME: This isn't really a type-checking request, if we ever split off
// a zone for more basic AST requests, this should be moved there.
class PrimarySourceFilesRequest :
  public SimpleRequest<
    PrimarySourceFilesRequest,
    ArrayRef<SourceFile *>(ModuleDecl *),
    RequestFlags::Cached> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  ArrayRef<SourceFile *>
  evaluate(Evaluator& Eval, ModuleDecl * Mod) const;

public:

  // Cached.
  bool isCached() const {
    return true;
  }
};

/// Retrieves the global variables in the a module.
/// FIXME: This isn't really a type-checking request, if we ever split off
/// a zone for more basic AST requests, this should be moved there.
class GlobalVariableRequest :
  public SimpleRequest<
    GlobalVariableRequest,
    std::shared_ptr<ModuleDecl::GlobalListType>(ModuleDecl *),
    RequestFlags::SeparatelyCached | RequestFlags::DependencySource> {
public:

  using SimpleRequest::SimpleRequest;

private:

  friend SimpleRequest;

  OutputType evaluate(Evaluator& Eval, ModuleDecl * Mod) const;

public:

  // Cached.
  bool isCached() const {
    return true;
  }

  Optional<OutputType> getCachedResult() const;

  void cacheResult(OutputType Result) const;

  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyRecorder&) const;
};

#define W2N_TYPEID_ZONE   TypeChecker
#define W2N_TYPEID_HEADER <w2n/AST/TypeCheckerTypeIDZone.def>
#include <w2n/Basic/DefineTypeIDZone.h>
#undef W2N_TYPEID_ZONE
#undef W2N_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define W2N_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)         \
  template <>                                                            \
  inline void reportEvaluatedRequest(                                    \
    UnifiedStatsReporter& stats, const RequestType& request              \
  ) {                                                                    \
    ++stats.getFrontendCounters().RequestType;                           \
  }
// FIXME: statistics
// #include <w2n/AST/TypeCheckerTypeIDZone.def>
#undef W2N_REQUEST

} // end namespace w2n

#endif // W2N_TYPE_CHECK_REQUESTS_H
